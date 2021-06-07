#include "../helpers.h"
#include <MaterialXGenEssl/EsslShaderGenerator.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderStage.h>


#include <iostream>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

namespace ems = emscripten;
namespace mx = MaterialX;

void initContext(mx::GenContext& context, mx::FileSearchPath searchPath, mx::DocumentPtr stdLib, mx::UnitConverterRegistryPtr unitRegistry) {
    // Initialize search paths.
    for (const mx::FilePath& path : searchPath)
    {
        context.registerSourceCodeSearchPath(path / "libraries");
    }

    // Initialize color management.
    mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
    cms->loadLibrary(stdLib);
    context.getShaderGenerator().setColorManagementSystem(cms);

    // Initialize unit management.
    mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
    unitSystem->loadLibrary(stdLib);
    unitSystem->setUnitConverterRegistry(unitRegistry);
    context.getShaderGenerator().setUnitSystem(unitSystem);
    context.getOptions().targetDistanceUnit = "meter";
}

void loadStandardLibraries(mx::GenContext& context)
{
    mx::DocumentPtr stdLib;
    mx::LinearUnitConverterPtr _distanceUnitConverter;
    mx::StringVec _distanceUnitOptions;
    mx::UnitConverterRegistryPtr unitRegistry;
    mx::FilePathVec libraryFolders;
    mx::FileSearchPath searchPath;

    // Initialize the standard library.
    try
    {
        std::cout << "loadStandardLibraries: createDocument" << std::endl;
        stdLib = mx::createDocument();
        std::cout << "loadStandardLibraries: loadCoreLibraries" << std::endl;
        mx::StringSet _xincludeFiles = mx::loadCoreLibraries(libraryFolders, searchPath, stdLib);
        if (_xincludeFiles.empty())
        {
            std::cerr << "Could not find standard data libraries on the given search path: " << searchPath.asString() << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
        return;
    }

    std::cout << "loadStandardLibraries: _distanceUnitConverter" << std::endl;
    // Initialize unit management.
    mx::UnitTypeDefPtr distanceTypeDef = stdLib->getUnitTypeDef("distance");
    _distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
    std::cout << "loadStandardLibraries: addUnitConverter1" << std::endl;
    unitRegistry->addUnitConverter(distanceTypeDef, _distanceUnitConverter);
    mx::UnitTypeDefPtr angleTypeDef = stdLib->getUnitTypeDef("angle");
    mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
    std::cout << "loadStandardLibraries: addUnitConverter2" << std::endl;
    unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

    std::cout << "loadStandardLibraries: getUnitScale" << std::endl;
    // Create the list of supported distance units.
    auto unitScales = _distanceUnitConverter->getUnitScale();
    _distanceUnitOptions.resize(unitScales.size());
    for (auto unitScale : unitScales)
    {
        int location = _distanceUnitConverter->getUnitAsInteger(unitScale.first);
        _distanceUnitOptions[location] = unitScale.first;
    }

    initContext(context,searchPath, stdLib, unitRegistry);

}
/*
ems::val getUniforms(mx::ShaderStage& stage) {
    ems::val result = ems::val::object();
    for (const auto& it : stage.getUniformBlocks())
    {
        const mx::VariableBlock& uniforms = *it.second;
        if (!uniforms.empty())
        {
            for (size_t i=0; i<uniforms.size(); ++i)
            {
                auto variable = uniforms[i];
                std::string str;
                // A file texture input needs special handling on GLSL
                if (variable->getType() == mx::Type::FILENAME)
                {
                    // Samplers must always be uniforms
                    str += "sampler2D " + variable->getVariable();
                }
                else
                {
                    str += "\"" + variable->getVariable() + "\": {";
                    //tokenSubstitution(_tokenSubstitutions, str);
                    str += "\"type\": \"" + _syntax->getTypeName(variable->getType()) + "\"";
                  
                    if (variable->getValue()) {
                        str +=  ", \"value\": ";
                        if (variable->getType()->isAggregate())
                            str += "[";
                        str +=  variable->getValue()->getValueString();
                        if (variable->getType()->isAggregate())
                            str += "]";
                    }
                    //std::cout << str << std::endl;
                    result += str + " },\n";
                }
            }
        }
    }

    return result;
}
}
*/

std::string getUniformValues(mx::EsslShaderGenerator& self, mx::TypedElementPtr elem) {
    std::cout << "context generating" << std::endl;
    mx::GenContext context(mx::EsslShaderGenerator::create());
    std::cout << "context generated" << std::endl;
    loadStandardLibraries(context);
    
    /*mx::StringVec renderablePaths;
    std::vector<mx::TypedElementPtr> elems;
    std::vector<mx::NodePtr> materialNodes;
    mx::findRenderableElements(doc, elems);

                MaterialPtr mat = Material::create();
                mat->setDocument(doc);
                mat->setElement(typedElem);
                mat->setMaterialNode(materialNodes[i]);
                newMaterials.push_back(mat);

    elem->getNamePath()*/
    mx::EsslShaderGenerator& esslGenerator = static_cast<mx::EsslShaderGenerator&>(context.getShaderGenerator());
    mx::ShaderPtr shader = self.generate(elem->getNamePath(), elem, context);

    mx::ShaderStage& vs = shader->getStage(mx::Stage::VERTEX);
    mx::ShaderStage& ps = shader->getStage(mx::Stage::PIXEL);
    std::string uniforms = "{";
    uniforms += esslGenerator.getUniformValues(vs);
    uniforms += esslGenerator.getUniformValues(ps);
    uniforms += "}";

    return uniforms;
}

EMSCRIPTEN_BINDINGS(EsslShaderGenerator)
{
    ems::class_<mx::EsslShaderGenerator>("EsslShaderGenerator")
        .constructor<>()
        .function("getUniformValues", &getUniformValues)
        ;
}


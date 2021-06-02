#include "../helpers.h"
#include <MaterialXGenGLES/GLESShaderGenerator.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Shader.h>

#include <iostream>

#include <emscripten.h>
#include <emscripten/bind.h>

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
        stdLib = mx::createDocument();
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

    // Initialize unit management.
    mx::UnitTypeDefPtr distanceTypeDef = stdLib->getUnitTypeDef("distance");
    _distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
    unitRegistry->addUnitConverter(distanceTypeDef, _distanceUnitConverter);
    mx::UnitTypeDefPtr angleTypeDef = stdLib->getUnitTypeDef("angle");
    mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
    unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

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


std::string getUniformValues(mx::GLESShaderGenerator& self, mx::TypedElementPtr elem) {
    mx::GenContext context(mx::GLESShaderGenerator::create());
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
    mx::GLESShaderGenerator& esslGenerator = static_cast<mx::GLESShaderGenerator&>(context.getShaderGenerator());
    mx::ShaderPtr shader = self.generate(elem->getNamePath(), elem, context);

    mx::ShaderStage& vs = shader->getStage(mx::Stage::VERTEX);
    mx::ShaderStage& ps = shader->getStage(mx::Stage::PIXEL);
    std::string uniforms = "{";
    uniforms += esslGenerator.getUniformValues(vs);
    uniforms += esslGenerator.getUniformValues(ps);
    uniforms += "}";

    return uniforms;
}

EMSCRIPTEN_BINDINGS(GLESShaderGenerator)
{
    ems::class_<mx::GLESShaderGenerator>("GLESShaderGenerator")
        .constructor<>()
        .function("getUniformValues", &getUniformValues)
        ;

}


#include "../helpers.h"
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
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

mx::DocumentPtr loadStandardLibraries(mx::GenContext& context)
{
    mx::DocumentPtr stdLib;
    mx::LinearUnitConverterPtr _distanceUnitConverter;
    mx::StringVec _distanceUnitOptions;
    mx::UnitConverterRegistryPtr unitRegistry;
    mx::FilePathVec libraryFolders = { "libraries" };
    mx::FileSearchPath searchPath;
    searchPath.append("/");

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
        return nullptr;
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

    return stdLib;
}

std::string getUniformValues(mx::EsslShaderGenerator& self, mx::TypedElementPtr elem, mx::GenContext& context) {
    mx::XmlReadOptions readOptions;
    readOptions.readXIncludeFunction = [](mx::DocumentPtr doc, const mx::FilePath& filename,
                                          const mx::FileSearchPath& searchPath, const mx::XmlReadOptions* options)
    {
        mx::FilePath resolvedFilename = searchPath.find(filename);
        if (resolvedFilename.exists())
        {
            readFromXmlFile(doc, resolvedFilename, searchPath, options);
        }
        else
        {
            std::cerr << "Include file not found: " << filename.asString() << std::endl;
        }
    };

    mx::ShaderPtr shader = self.generate(elem->getNamePath(), elem, context);

    mx::ShaderStage& vs = shader->getStage(mx::Stage::VERTEX);
    mx::ShaderStage& ps = shader->getStage(mx::Stage::PIXEL);

    std::string uniforms = "{";
    uniforms += self.getUniformValues(vs);
    uniforms += self.getUniformValues(ps);
    uniforms += "}";

    return uniforms;
}

EMSCRIPTEN_BINDINGS(EsslShaderGenerator)
{
    ems::class_<mx::Shader>("Shader")
        .smart_ptr<std::shared_ptr<mx::Shader>>("ShaderPtr")
        .function("getSourceCode", &mx::Shader::getSourceCode);
        ;

    ems::class_<mx::GenContext>("GenContext")
        .constructor<mx::ShaderGeneratorPtr>()
        .smart_ptr<std::shared_ptr<mx::GenContext>>("GenContextPtr")
        ;

    ems::class_<mx::ShaderGenerator>("ShaderGenerator")
        .smart_ptr<std::shared_ptr<mx::ShaderGenerator>>("ShaderGeneratorPtr")
        .function("generate", &mx::ShaderGenerator::generate)
        ;

    ems::class_<mx::EsslShaderGenerator, ems::base<mx::ShaderGenerator>>("EsslShaderGenerator")
        .smart_ptr_constructor("EsslShaderGenerator", &std::make_shared<mx::EsslShaderGenerator>)
        .function("getUniformValues", &getUniformValues)
        ;

    ems::function("loadStandardLibraries", &loadStandardLibraries);
}


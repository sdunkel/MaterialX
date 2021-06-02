#include "../helpers.h"
#include <MaterialXGenGLES/GLESShaderGenerator.h>

#include <emscripten.h>
#include <emscripten/bind.h>

namespace ems = emscripten;
namespace mx = MaterialX;

void initContext() {
    mx::GenContext context(mx::GLESShaderGenerator::create());

    // Initialize search paths.
    for (const mx::FilePath& path : _searchPath)
    {
        context.registerSourceCodeSearchPath(path / "libraries");
    }

    // Initialize color management.
    mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
    cms->loadLibrary(_stdLib);
    context.getShaderGenerator().setColorManagementSystem(cms);

    // Initialize unit management.
    mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
    unitSystem->loadLibrary(_stdLib);
    unitSystem->setUnitConverterRegistry(_unitRegistry);
    context.getShaderGenerator().setUnitSystem(unitSystem);
    context.getOptions().targetDistanceUnit = "meter";
}

void loadStandardLibraries()
{
     mx::DocumentPtr _stdLib;
     mx::LinearUnitConverterPtr _distanceUnitConverter;
     mx::StringVec _distanceUnitOptions;
     mx::UnitConverterRegistryPtr _unitRegistry;
     std::string _libraryFolders = "???";
     std::string _searchPath = "???";

    // Initialize the standard library.
    try
    {
        _stdLib = mx::createDocument();
        _xincludeFiles = mx::loadCoreLibraries(_libraryFolders, _searchPath, _stdLib);
        if (_xincludeFiles.empty())
        {
            std::cerr << "Could not find standard data libraries on the given search path: " << _searchPath.asString() << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
        return;
    }

    // Initialize unit management.
    mx::UnitTypeDefPtr distanceTypeDef = _stdLib->getUnitTypeDef("distance");
    _distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
    _unitRegistry->addUnitConverter(distanceTypeDef, _distanceUnitConverter);
    mx::UnitTypeDefPtr angleTypeDef = _stdLib->getUnitTypeDef("angle");
    mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
    _unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

    // Create the list of supported distance units.
    auto unitScales = _distanceUnitConverter->getUnitScale();
    _distanceUnitOptions.resize(unitScales.size());
    for (auto unitScale : unitScales)
    {
        int location = _distanceUnitConverter->getUnitAsInteger(unitScale.first);
        _distanceUnitOptions[location] = unitScale.first;
    }

    initContext();

}


std::string getUniformValues(mx::GLESShaderGenerator& self) {
    mx::StringVec renderablePaths;
    std::vector<mx::TypedElementPtr> elems;
    std::vector<mx::NodePtr> materialNodes;
    mx::findRenderableElements(doc, elems);

                MaterialPtr mat = Material::create();
                mat->setDocument(doc);
                mat->setElement(typedElem);
                mat->setMaterialNode(materialNodes[i]);
                newMaterials.push_back(mat);

    mx::ShaderPtr shader = self.generate(shaderName, elem, context);

               mx::ShaderStage& vs = shader->getStage(mx::Stage::VERTEX);
                    mx::ShaderStage& ps = shader->getStage(mx::Stage::PIXEL);
                    std::string uniforms = "{";
                    uniforms += esslGenerator.getUniformValues(vs);
                    uniforms += esslGenerator.getUniformValues(ps);
                    uniforms += "}";
}

EMSCRIPTEN_BINDINGS(GLESShaderGenerator)
{
    ems::class_<mx::GLESShaderGenerator>("GLESShaderGenerator")
        .constructor<>()
        .function("getUniformValues", &getUniformValues)
        .property("parentXIncludes", &mx::XmlReadOptions::parentXIncludes);

}


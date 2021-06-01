//
// TM & (c) 2032 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXGenGLES/GLESShaderGenerator.h>
#include <MaterialXGenGLES/GLESSyntax.h>

#include <MaterialXGenShader/Nodes/HwImageNode.h>

namespace MaterialX
{

const string GLESShaderGenerator::TARGET = "gles";
const string GLESShaderGenerator::VERSION = "100"; // Target webgl 1.0
const string GLESShaderGenerator::MATH_INCLUDE_PATH = "pbrlib/" + GLESShaderGenerator::TARGET + "/lib/mx_math.glsl";


GLESShaderGenerator::GLESShaderGenerator()
    : GlslShaderGenerator()
{
    _syntax = GLESSyntax::create();
    // GL ES specific keywords
    const StringSet reservedWords = { "precision", "highp", "mediump", "lowp" };
    _syntax->registerReservedWords(reservedWords);

    // Temporary overwrites for THREE js
    _tokenSubstitutions[HW::T_IN_POSITION] = "position";
    _tokenSubstitutions[HW::T_IN_NORMAL] = "normal";

    // Register override implementations
    // <!-- <image> -->
    registerImplementation("IM_image_float_" + GLESShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_color3_" + GLESShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_color4_" + GLESShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector2_" + GLESShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector3_" + GLESShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector4_" + GLESShaderGenerator::TARGET, HwImageNode::create);
}

void GLESShaderGenerator::emitDirectives(GenContext&, ShaderStage& stage) const
{
    emitLine("#version " + getVersion(), stage, false);
BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
    emitLineBreak(stage);
    emitLine("#extension GL_EXT_shader_texture_lod : enable", stage, false);
    emitLine("#extension GL_OES_standard_derivatives : enable", stage, false);
    emitLineBreak(stage);
    emitLine("precision mediump float", stage);
END_SHADER_STAGE(stage, Stage::PIXEL)
}

void GLESShaderGenerator::emitUniforms(GenContext& context, ShaderStage& stage, HwResourceBindingContextPtr& resourceBindingCtx) const
{
    for (const auto& it : stage.getUniformBlocks())
    {
        const VariableBlock& uniforms = *it.second;

        // Skip light uniforms as they are handled separately
        if (!uniforms.empty() && uniforms.getName() != HW::LIGHT_DATA)
        {
            emitComment("Uniform block: " + uniforms.getName(), stage);
            if (resourceBindingCtx)
            {
                resourceBindingCtx->emitResourceBindings(context, uniforms, stage);
            }
            else
            {
                emitVariableDeclarations(uniforms, _syntax->getUniformQualifier(), Syntax::SEMICOLON, context, stage, false);
                emitLineBreak(stage);
            }
        }
    }
}

void GLESShaderGenerator::emitInputs(GenContext& context, ShaderStage& stage) const
{
BEGIN_SHADER_STAGE(stage, Stage::VERTEX)
    const VariableBlock& vertexInputs = stage.getInputBlock(HW::VERTEX_INPUTS);
    if (!vertexInputs.empty())
    {
        emitComment("Inputs block: " + vertexInputs.getName(), stage);
        emitVariableDeclarations(vertexInputs, std::dynamic_pointer_cast<GLESSyntax>(_syntax)->getAttributeQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::VERTEX)

BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
    const VariableBlock& vertexData = stage.getInputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData, std::dynamic_pointer_cast<GLESSyntax>(_syntax)->getVaryingQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::PIXEL)
}

void GLESShaderGenerator::emitOutpus(GenContext& context, ShaderStage& stage) const
{
BEGIN_SHADER_STAGE(stage, Stage::VERTEX)
    const VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData,  std::dynamic_pointer_cast<GLESSyntax>(_syntax)->getVaryingQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::VERTEX)
}

const string GLESShaderGenerator::getPixelStageOutputVariable(const ShaderGraphOutputSocket&) const
{
    return "gl_FragColor";
}

const string GLESShaderGenerator::getVertexDataPrefix(const VariableBlock&) const
{
    return "";
}

bool GLESShaderGenerator::requiresLighting(const ShaderGraph& graph) const
{
    // Force it to be false for now to not include lighting shader functions.
    return GlslShaderGenerator::requiresLighting(graph) && false;
}

}

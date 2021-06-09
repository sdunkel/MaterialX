//
// TM & (c) 2032 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXGenEssl/EsslShaderGenerator.h>
#include <MaterialXGenEssl/EsslSyntax.h>

#include <MaterialXGenShader/Nodes/HwImageNode.h>
#include <MaterialXGenShader/Util.h>

namespace MaterialX
{

const string EsslShaderGenerator::TARGET = "essl";
const string EsslShaderGenerator::VERSION = "300 es"; // Target webgl 2.0

EsslShaderGenerator::EsslShaderGenerator()
    : GlslShaderGenerator()
{
    _syntax = EsslSyntax::create();
    // GL ES specific keywords
    const StringSet reservedWords = { "precision", "highp", "mediump", "lowp" };
    _syntax->registerReservedWords(reservedWords);

    // Temporary overwrites for THREE js
    _tokenSubstitutions[HW::T_IN_POSITION] = "position";
    _tokenSubstitutions[HW::T_IN_NORMAL] = "normal";
    _tokenSubstitutions[HW::T_IN_TEXCOORD] = "uv";
    _tokenSubstitutions[HW::T_IN_TANGENT] = "tangent";

    // Register override implementations
    // <!-- <image> -->
    registerImplementation("IM_image_float_" + EsslShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_color3_" + EsslShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_color4_" + EsslShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector2_" + EsslShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector3_" + EsslShaderGenerator::TARGET, HwImageNode::create);
    registerImplementation("IM_image_vector4_" + EsslShaderGenerator::TARGET, HwImageNode::create);
}

void EsslShaderGenerator::emitDirectives(GenContext&, ShaderStage& stage) const
{
    emitLine("#version " + getVersion(), stage, false);
BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
    emitLineBreak(stage);
    emitLine("precision mediump float", stage);
END_SHADER_STAGE(stage, Stage::PIXEL)
}

void EsslShaderGenerator::emitUniforms(GenContext& context, ShaderStage& stage, HwResourceBindingContextPtr& resourceBindingCtx) const
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

void EsslShaderGenerator::emitInputs(GenContext& context, ShaderStage& stage) const
{
BEGIN_SHADER_STAGE(stage, Stage::VERTEX)
    const VariableBlock& vertexInputs = stage.getInputBlock(HW::VERTEX_INPUTS);
    if (!vertexInputs.empty())
    {
        emitComment("Inputs block: " + vertexInputs.getName(), stage);
        emitVariableDeclarations(vertexInputs, _syntax->getInputQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::VERTEX)

BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
    const VariableBlock& vertexData = stage.getInputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData, _syntax->getInputQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::PIXEL)
}

void EsslShaderGenerator::emitOutpus(GenContext& context, ShaderStage& stage) const
{
BEGIN_SHADER_STAGE(stage, Stage::VERTEX)
    const VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData,  _syntax->getOutputQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }
END_SHADER_STAGE(stage, Stage::VERTEX)

BEGIN_SHADER_STAGE(stage, Stage::PIXEL)
    emitComment("Pixel shader outputs", stage);
    const VariableBlock& outputs = stage.getOutputBlock(HW::PIXEL_OUTPUTS);
    emitVariableDeclarations(outputs, _syntax->getOutputQualifier(), Syntax::SEMICOLON, context, stage, false);
    emitLineBreak(stage);
END_SHADER_STAGE(stage, Stage::PIXEL)
}

const string EsslShaderGenerator::getVertexDataPrefix(const VariableBlock&) const
{
    return "";
}

string EsslShaderGenerator::getUniformValues(ShaderStage& stage) const {
    string result;
    for (const auto& it : stage.getUniformBlocks())
    {
        const VariableBlock& uniforms = *it.second;
        if (!uniforms.empty())
        {
            for (size_t i=0; i<uniforms.size(); ++i)
            {
                auto variable = uniforms[i];
                string str;
                // A file texture input needs special handling on GLSL
                if (variable->getType() == Type::FILENAME)
                {
                    // Samplers must always be uniforms
                    str += "sampler2D " + variable->getVariable();
                }
                else
                {
                    str += "\"" + variable->getVariable() + "\": {";
                    tokenSubstitution(_tokenSubstitutions, str);
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

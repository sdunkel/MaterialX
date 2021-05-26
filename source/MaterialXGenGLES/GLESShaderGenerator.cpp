//
// TM & (c) 2032 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXGenGLES/GLESShaderGenerator.h>
#include <MaterialXGenGLES/Nodes/SurfaceNodeGLES.h>

#include <MaterialXGenShader/Nodes/HwImageNode.h>

namespace MaterialX
{

const string GLESShaderGenerator::TARGET = "gles";
const string GLESShaderGenerator::VERSION = "300 es";


GLESShaderGenerator::GLESShaderGenerator()
    : GlslShaderGenerator()
{
    // GL ES specific keywords... etc
    const StringSet reservedWords = { "precision", "highp", "lowp" }; 
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

    // <!-- <surface> -->
    registerImplementation("IM_surface_" + GLESShaderGenerator::TARGET, SurfaceNodeGLES::create);
}

void GLESShaderGenerator::emitVertexStage(const ShaderGraph& graph, GenContext& context, ShaderStage& stage) const
{
    HwResourceBindingContextPtr resourceBindingCtx = context.getUserData<HwResourceBindingContext>(HW::USER_DATA_BINDING_CONTEXT);

    // Add directives
    emitLine("#version " + getVersion(), stage, false);
    if (resourceBindingCtx)
    {
        resourceBindingCtx->emitDirectives(context, stage);
    }
    emitLineBreak(stage);

    // Add all constants
    const VariableBlock& constants = stage.getConstantBlock();
    if (!constants.empty())
    {
        emitVariableDeclarations(constants, _syntax->getConstantQualifier(), Syntax::SEMICOLON, context, stage);
        emitLineBreak(stage);
    }

    // Add all uniforms
    for (const auto& it : stage.getUniformBlocks())
    {
        const VariableBlock& uniforms = *it.second;
        if (!uniforms.empty())
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

    // Add vertex inputs
    const VariableBlock& vertexInputs = stage.getInputBlock(HW::VERTEX_INPUTS);
    if (!vertexInputs.empty())
    {
        emitComment("Inputs block: " + vertexInputs.getName(), stage);
        emitVariableDeclarations(vertexInputs, _syntax->getInputQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }

    // Add vertex data outputs block
    const VariableBlock& vertexData = stage.getOutputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData, _syntax->getOutputQualifier(), Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
    }

    emitFunctionDefinitions(graph, context, stage);

    // Add main function
    setFunctionName("main", stage);
    emitLine("void main()", stage, false);
    emitScopeBegin(stage);
    emitLine("vec4 hPositionWorld = " + HW::T_WORLD_MATRIX + " * vec4(" + HW::T_IN_POSITION + ", 1.0)", stage);
    emitLine("gl_Position = " + HW::T_VIEW_PROJECTION_MATRIX + " * hPositionWorld", stage);
    emitFunctionCalls(graph, context, stage);
    emitScopeEnd(stage);
    emitLineBreak(stage);
}

bool GLESShaderGenerator::requiresLighting(const ShaderGraph& graph) const
{
    // Force it to be false for now to not include lighting shader functions.
    return GlslShaderGenerator::requiresLighting(graph) && false;
}

void GLESShaderGenerator::emitPixelStage(const ShaderGraph& graph, GenContext& context, ShaderStage& stage) const
{
    HwResourceBindingContextPtr resourceBindingCtx = context.getUserData<HwResourceBindingContext>(HW::USER_DATA_BINDING_CONTEXT);

    // Add directives
    emitLine("#version " + getVersion(), stage, false);
    emitLineBreak(stage);
    emitLine("precision mediump float", stage);
    if (resourceBindingCtx)
    {
        resourceBindingCtx->emitDirectives(context, stage);
    }
    emitLineBreak(stage);

    // Add global constants and type definitions
    emitInclude("pbrlib/" + GlslShaderGenerator::TARGET + "/lib/mx_defines.glsl", context, stage);

    if (context.getOptions().hwMaxActiveLightSources > 0)
    {
        const unsigned int maxLights = std::max(1u, context.getOptions().hwMaxActiveLightSources);
        emitLine("#define " + HW::LIGHT_DATA_MAX_LIGHT_SOURCES + " " + std::to_string(maxLights), stage, false);
    }
    emitLine("#define DIRECTIONAL_ALBEDO_METHOD " + std::to_string(int(context.getOptions().hwDirectionalAlbedoMethod)), stage, false);
    emitLine("#define " + HW::ENV_RADIANCE_MAX_SAMPLES + " " + std::to_string(context.getOptions().hwMaxRadianceSamples), stage, false);
    emitLineBreak(stage);
    emitTypeDefinitions(context, stage);

    // Add all constants
    const VariableBlock& constants = stage.getConstantBlock();
    if (!constants.empty())
    {
        emitVariableDeclarations(constants, _syntax->getConstantQualifier(), Syntax::SEMICOLON, context, stage);
        emitLineBreak(stage);
    }

    // Add all uniforms
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

    bool lighting = requiresLighting(graph);

    bool shadowing = (lighting && context.getOptions().hwShadowMap) ||
                     context.getOptions().hwWriteDepthMoments;

    // Add light data block if needed
    if (lighting && context.getOptions().hwMaxActiveLightSources > 0)
    {
        const VariableBlock& lightData = stage.getUniformBlock(HW::LIGHT_DATA);
        const string structArraySuffix = "[" + HW::LIGHT_DATA_MAX_LIGHT_SOURCES + "]";
        const string structName        = lightData.getInstance();
        if (resourceBindingCtx)
        {
            resourceBindingCtx->emitStructuredResourceBindings(
                context, lightData, stage, structName, structArraySuffix);
        }
        else
        {
            emitLine("struct " + lightData.getName(), stage, false);
            emitScopeBegin(stage);
            emitVariableDeclarations(lightData, EMPTY_STRING, Syntax::SEMICOLON, context, stage, false);
            emitScopeEnd(stage, true);
            emitLineBreak(stage);
            emitLine("uniform " + lightData.getName() + " " + structName + structArraySuffix, stage);
        }
        emitLineBreak(stage);
    }

    // Add vertex data inputs block
    const VariableBlock& vertexData = stage.getInputBlock(HW::VERTEX_DATA);
    if (!vertexData.empty())
    {
        emitVariableDeclarations(vertexData, _syntax->getInputQualifier() , Syntax::SEMICOLON, context, stage, false);
        emitLineBreak(stage);
        emitLineBreak(stage);
    }

    // Add the pixel shader output. This needs to be a vec4 for rendering
    // and upstream connection will be converted to vec4 if needed in emitFinalOutput()
    emitComment("Pixel shader outputs", stage);
    const VariableBlock& outputs = stage.getOutputBlock(HW::PIXEL_OUTPUTS);
    emitVariableDeclarations(outputs, _syntax->getOutputQualifier(), Syntax::SEMICOLON, context, stage, false);
    emitLineBreak(stage);

    // Emit common math functions
    emitInclude("pbrlib/" + GlslShaderGenerator::TARGET + "/lib/mx_math.glsl", context, stage);
    emitLineBreak(stage);

    // Emit lighting and shadowing code
    if (lighting)
    {
        emitSpecularEnvironment(context, stage);
    }
    if (shadowing)
    {
        emitInclude("pbrlib/" + GlslShaderGenerator::TARGET + "/lib/mx_shadow.glsl", context, stage);
    }

    // Emit directional albedo table code.
    if (context.getOptions().hwDirectionalAlbedoMethod == DIRECTIONAL_ALBEDO_TABLE ||
        context.getOptions().hwWriteAlbedoTable)
    {
        emitInclude("pbrlib/" + GlslShaderGenerator::TARGET + "/lib/mx_table.glsl", context, stage);
        emitLineBreak(stage);
    }

    // Set the include file to use for uv transformations,
    // depending on the vertical flip flag.
    if (context.getOptions().fileTextureVerticalFlip)
    {
        _tokenSubstitutions[ShaderGenerator::T_FILE_TRANSFORM_UV] = "stdlib/" + GlslShaderGenerator::TARGET + "/lib/mx_transform_uv_vflip.glsl";
    }
    else
    {
        _tokenSubstitutions[ShaderGenerator::T_FILE_TRANSFORM_UV] = "stdlib/" + GlslShaderGenerator::TARGET + "/lib/mx_transform_uv.glsl";
    }

    // Emit uv transform code globally if needed.
    if (context.getOptions().hwAmbientOcclusion)
    {
        emitInclude(ShaderGenerator::T_FILE_TRANSFORM_UV, context, stage);
    }

    // Add all functions for node implementations
    emitFunctionDefinitions(graph, context, stage);

    const ShaderGraphOutputSocket* outputSocket = graph.getOutputSocket();

    // Add main function
    setFunctionName("main", stage);
    emitLine("void main()", stage, false);
    emitScopeBegin(stage);

    if (graph.hasClassification(ShaderNode::Classification::CLOSURE))
    {
        // Handle the case where the graph is a direct closure.
        // We don't support rendering closures without attaching 
        // to a surface shader, so just output black.
        emitLine(outputSocket->getVariable() + " = vec4(0.0, 0.0, 0.0, 1.0)", stage);
    }
    else if (context.getOptions().hwWriteDepthMoments)
    {
        emitLine(outputSocket->getVariable() + " = vec4(mx_compute_depth_moments(), 0.0, 1.0)", stage);
    }
    else if (context.getOptions().hwWriteAlbedoTable)
    {
        emitLine(outputSocket->getVariable() + " = vec4(mx_ggx_directional_albedo_generate_table(), 0.0, 1.0)", stage);
    }
    else
    {
        // Add all function calls
        emitFunctionCalls(graph, context, stage);

        // Emit final output
        const ShaderOutput* outputConnection = outputSocket->getConnection();
        if (outputConnection)
        {
            string finalOutput = outputConnection->getVariable();
            const string& channels = outputSocket->getChannels();
            if (!channels.empty())
            {
                finalOutput = _syntax->getSwizzledVariable(finalOutput, outputConnection->getType(), channels, outputSocket->getType());
            }

            if (graph.hasClassification(ShaderNode::Classification::SURFACE))
            {
                if (context.getOptions().hwTransparency)
                {
                    emitLine("float outAlpha = clamp(1.0 - dot(" + finalOutput + ".transparency, vec3(0.3333)), 0.0, 1.0)", stage);
                    emitLine(outputSocket->getVariable() + " = vec4(" + finalOutput + ".color, outAlpha)", stage);
                    emitLine("if (outAlpha < " + HW::T_ALPHA_THRESHOLD + ")", stage, false);
                    emitScopeBegin(stage);
                    emitLine("discard", stage);
                    emitScopeEnd(stage);
                }
                else
                {
                    emitLine(outputSocket->getVariable() + " = vec4(" + finalOutput + ".color, 1.0)", stage);
                }
            }
            else
            {
                if (!outputSocket->getType()->isFloat4())
                {
                    toVec4(outputSocket->getType(), finalOutput);
                }
                emitLine(outputSocket->getVariable() + " = " + finalOutput, stage);
            }
        }
        else
        {
            string outputValue = outputSocket->getValue() ? _syntax->getValue(outputSocket->getType(), *outputSocket->getValue()) : _syntax->getDefaultValue(outputSocket->getType());
            if (!outputSocket->getType()->isFloat4())
            {
                string finalOutput = outputSocket->getVariable() + "_tmp";
                emitLine(_syntax->getTypeName(outputSocket->getType()) + " " + finalOutput + " = " + outputValue, stage);
                toVec4(outputSocket->getType(), finalOutput);
                emitLine(outputSocket->getVariable() + " = " + finalOutput, stage);
            }
            else
            {
                emitLine(outputSocket->getVariable() + " = " + outputValue, stage);
            }
        }
    }

    // End main function
    emitScopeEnd(stage);
    emitLineBreak(stage);
}

}

//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_GLESSHADERGENERATOR_H
#define MATERIALX_GLESSHADERGENERATOR_H

/// @file
/// OpenGL ES shader generator

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

namespace MaterialX
{

using GLESShaderGeneratorPtr  = shared_ptr<class GLESShaderGenerator>;

/// @class ArnoldShaderGenerator 
/// An OpenGL ES generator 
class GLESShaderGenerator : public GlslShaderGenerator
{
  public:
    GLESShaderGenerator();

    static ShaderGeneratorPtr create() { return std::make_shared<GLESShaderGenerator>(); }

    /// Return a unique identifier for the target this generator is for
    const string& getTarget() const override { return TARGET; }

    const string& getVersion() const override { return VERSION; }

    const string& getMathIncludePath() const override { return MATH_INCLUDE_PATH; }

    const string getVertexDataPrefix(const VariableBlock& vertexData) const override;

    /// Unique identifier for this generator target
    static const string TARGET;
    static const string VERSION;
    static const string MATH_INCLUDE_PATH;

   protected:
    void emitDirectives(GenContext& context, ShaderStage& stage) const override;
    void emitUniforms(GenContext& context, ShaderStage& stage, HwResourceBindingContextPtr &resourceBindingCtx) const override;
    void emitInputs(GenContext& context, ShaderStage& stage) const override;
    void emitOutpus(GenContext& context, ShaderStage& stage) const override;

    const string getPixelStageOutputVariable(const ShaderGraphOutputSocket& outputSocket) const override;

    bool requiresLighting(const ShaderGraph& graph) const override;
};

}

#endif

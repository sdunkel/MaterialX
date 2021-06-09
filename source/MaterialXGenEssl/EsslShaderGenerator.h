//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_ESSLSHADERGENERATOR_H
#define MATERIALX_ESSLSHADERGENERATOR_H

/// @file
/// OpenGL ES shader generator

#include <MaterialXGenGlsl/GlslShaderGenerator.h>

namespace MaterialX
{

using EsslShaderGeneratorPtr = shared_ptr<class EsslShaderGenerator>;

/// @class ArnoldShaderGenerator 
/// An OpenGL ES generator 
class EsslShaderGenerator : public GlslShaderGenerator
{
  public:
    EsslShaderGenerator();

    static ShaderGeneratorPtr create() { return std::make_shared<EsslShaderGenerator>(); }

    /// Return a unique identifier for the target this generator is for
    const string& getTarget() const override { return TARGET; }

    const string& getVersion() const override { return VERSION; }

    const string getVertexDataPrefix(const VariableBlock& vertexData) const override;

    string getUniformValues(ShaderStage& stage) const;

    /// Unique identifier for this generator target
    static const string TARGET;
    static const string VERSION;

   protected:
    void emitDirectives(GenContext& context, ShaderStage& stage) const override;
    void emitUniforms(GenContext& context, ShaderStage& stage, HwResourceBindingContextPtr &resourceBindingCtx) const override;
    void emitInputs(GenContext& context, ShaderStage& stage) const override;
    void emitOutpus(GenContext& context, ShaderStage& stage) const override;
};

}

#endif

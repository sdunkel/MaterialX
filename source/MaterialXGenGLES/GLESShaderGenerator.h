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

    /// Unique identifier for this generator target
    static const string TARGET;
    static const string VERSION;

   protected:
    void emitVertexStage(const ShaderGraph& graph, GenContext& context, ShaderStage& stage) const override;
    void emitPixelStage(const ShaderGraph& graph, GenContext& context, ShaderStage& stage) const override;

    bool requiresLighting(const ShaderGraph& graph) const;
};

}

#endif

//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_SURFACENODEGLES_H
#define MATERIALX_SURFACENODEGLES_H

#include <MaterialXGenGlsl/Nodes/SurfaceNodeGlsl.h>

namespace MaterialX
{

/// Surface node implementation for GLSL
class SurfaceNodeGLES : public SurfaceNodeGlsl
{
  public:
    SurfaceNodeGLES();

    static ShaderNodeImplPtr create();

    void emitFunctionCall(const ShaderNode& node, GenContext& context, ShaderStage& stage) const override;

     private:
    /// Closure contexts for calling closure functions.
    HwClosureContextPtr _callReflection;
    HwClosureContextPtr _callTransmission;
    HwClosureContextPtr _callIndirect;
    HwClosureContextPtr _callEmission;
};

} // namespace MaterialX

#endif

//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_GLESSYNTAX_H
#define MATERIALX_GLESSYNTAX_H

/// @file
/// GLES syntax class

#include <MaterialXGenGlsl/GlslSyntax.h>

namespace MaterialX
{

/// Syntax class for GLES
class GLESSyntax : public GlslSyntax
{
public:
    GLESSyntax();

    virtual const string& getAttributeQualifier() const { return ATTRIBUTE_QUALIFIER; }
    virtual const string& getVaryingQualifier() const { return VARYING_QUALIFIER; }

    static SyntaxPtr create() { return std::make_shared<GLESSyntax>(); }

    static const string ATTRIBUTE_QUALIFIER;
    static const string VARYING_QUALIFIER;
};

} // namespace MaterialX

#endif

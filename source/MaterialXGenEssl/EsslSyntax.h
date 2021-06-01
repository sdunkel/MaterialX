//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_ESSLSYNTAX_H
#define MATERIALX_ESSLSYNTAX_H

/// @file
/// OpenGL ES syntax class

#include <MaterialXGenGlsl/GlslSyntax.h>

namespace MaterialX
{

/// Syntax class for ESSL
class EsslSyntax : public GlslSyntax
{
public:
    EsslSyntax();

    virtual const string& getAttributeQualifier() const { return ATTRIBUTE_QUALIFIER; }
    virtual const string& getVaryingQualifier() const { return VARYING_QUALIFIER; }

    static SyntaxPtr create() { return std::make_shared<EsslSyntax>(); }

    static const string ATTRIBUTE_QUALIFIER;
    static const string VARYING_QUALIFIER;
};

} // namespace MaterialX

#endif

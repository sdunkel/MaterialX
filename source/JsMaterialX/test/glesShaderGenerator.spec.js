import { expect } from 'chai';
import { traverse, initMaterialX } from './testHelpers';

describe('Generate GLES Shaders', () => {
    let mx, doc, image2, constant, multiply, contrast, mix, output;
    before(async () => {
        mx = await initMaterialX();
    });

    
    it('Get Shader Uniforms', () => {
        const doc = mx.createDocument();
        mx.readFromXmlString(doc, mtlxStr);
        const shaderGen = new mx.GLESShaderGenerator();
        const uniforms = shaderGen.getUniformValues();
    });
}
#version 100

// Uniform block: PrivateUniforms
uniform mat4 u_worldMatrix;
uniform mat4 u_viewProjectionMatrix;
uniform mat4 u_worldInverseTransposeMatrix;

// Inputs block: VertexInputs
attribute vec3 position;
attribute vec3 normal;

varying vec3 positionWorld;
varying vec3 normalWorld;

void main()
{
    vec4 hPositionWorld = u_worldMatrix * vec4(position, 1.0);
    gl_Position = u_viewProjectionMatrix * hPositionWorld;
    positionWorld = hPositionWorld.xyz;
    normalWorld = normalize((u_worldInverseTransposeMatrix * vec4(normal, 0)).xyz);
}

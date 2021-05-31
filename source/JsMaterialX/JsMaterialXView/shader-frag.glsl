#version 100

#extension GL_EXT_shader_texture_lod : enable
#extension GL_OES_standard_derivatives : enable

precision mediump float;

#define M_PI 3.1415926535897932384626433832795
#define M_PI_INV 1.0/3.1415926535897932384626433832795
#define M_GOLDEN_RATIO 1.6180339887498948482045868343656
#define M_FLOAT_EPS 1e-8
#define DIRECTIONAL_ALBEDO_METHOD 0
#define MAX_ENV_RADIANCE_SAMPLES 1024

#define BSDF vec3
#define EDF vec3
struct VDF { vec3 absorption; vec3 scattering; };
struct surfaceshader { vec3 color; vec3 transparency; };
struct volumeshader { VDF vdf; EDF edf; };
struct displacementshader { vec3 offset; float scale; };
struct lightshader { vec3 intensity; vec3 direction; };
struct thinfilm { float thickness; float ior; };

// Uniform block: PrivateUniforms
uniform vec3 u_viewPosition;

// Uniform block: PublicUniforms
uniform vec3 color1;
uniform float surface_opacity;

varying vec3 positionWorld;
varying vec3 normalWorld;

float mx_square(float x)
{
    return x*x;
}

vec2 mx_square(vec2 x)
{
    return x*x;
}

vec3 mx_square(vec3 x)
{
    return x*x;
}

vec4 mx_square(vec4 x)
{
    return x*x;
}

float mx_pow5(float x)
{
    return mx_square(mx_square(x)) * x;
}

float mx_max_component(vec2 v)
{
    return max(v.x, v.y);
}

float mx_max_component(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

float mx_max_component(vec4 v)
{
    return max(max(max(v.x, v.y), v.z), v.w);
}

bool mx_is_tiny(float v)
{
    return abs(v) < M_FLOAT_EPS;
}

bool mx_is_tiny(vec3 v)
{
    return all(lessThan(abs(v), vec3(M_FLOAT_EPS)));
}

float mx_mix(float v00, float v01, float v10, float v11,
             float x, float y)
{
   float v0_ = mix(v00, v01, x);
   float v1_ = mix(v10, v11, x);
   return mix(v0_, v1_, y);
}

vec3 mx_forward_facing_normal(vec3 N, vec3 V)
{
    if (dot(N, V) < 0.0)
    {
        return -N;
    }
    else
    {
        return N;
    }
}

vec2 mx_latlong_projection(vec3 dir)
{
    float latitude = -asin(dir.y) * M_PI_INV + 0.5;
    float longitude = atan(dir.x, -dir.z) * M_PI_INV * 0.5 + 0.5;
    return vec2(longitude, latitude);
}

vec3 mx_latlong_map_lookup(vec3 dir, mat4 transform, float lod, sampler2D sampler)
{
    vec3 envDir = normalize((transform * vec4(dir,0.0)).xyz);
    vec2 uv = mx_latlong_projection(envDir);
    return texture2DLodEXT(sampler, uv, lod).rgb;
}

void mx_uniform_edf(vec3 N, vec3 L, vec3 color, out EDF result)
{
    result = color;
}

void main()
{
    surfaceshader surface_out = surfaceshader(vec3(0.0),vec3(0.0));
    {
        // Shadow occlusion
        float occlusion = 1.0;

        vec3 N = normalize(normalWorld);
        vec3 V = normalize(u_viewPosition - positionWorld);
        vec3 P = positionWorld;
        // Ambient occlusion
        occlusion = 1.0;

        // Add surface emission
        {
            EDF uniform_edf_out = EDF(0.0);
            mx_uniform_edf(N, V, color1, uniform_edf_out);
            surface_out.color += uniform_edf_out;
        }

        // Add indirect contribution
        {

            surface_out.color += occlusion * BSDF(0.0);
        }

        surface_out.transparency = vec3(0.0);
    }

    gl_FragColor = vec4(surface_out.color, 1.0);
}

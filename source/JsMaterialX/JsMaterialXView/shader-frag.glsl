#version 300 es

precision mediump float;

#define M_PI 3.1415926535897932384626433832795
#define M_PI_INV 1.0/3.1415926535897932384626433832795
#define M_GOLDEN_RATIO 1.6180339887498948482045868343656
#define M_FLOAT_EPS 1e-8
#define MAX_LIGHT_SOURCES 1
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
uniform int u_numActiveLightSources;

// Uniform block: PublicUniforms
uniform float burley_brdf1_weight;
uniform vec3 burley_brdf1_color;
uniform float burley_brdf1_roughness;
uniform float surface1_opacity;

struct LightData
{
    int type;
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform LightData u_lightData[MAX_LIGHT_SOURCES];

in vec3 normalWorld;
in vec3 positionWorld;

// Pixel shader outputs
out vec4 out1;

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
    return textureLod(sampler, uv, lod).rgb;
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

// Standard Schlick Fresnel
float mx_fresnel_schlick(float cosTheta, float F0)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return F0 + (1.0 - F0) * x5;
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return F0 + (1.0 - F0) * x5;
}

// Generalized Schlick Fresnel
float mx_fresnel_schlick(float cosTheta, float F0, float F90)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return mix(F0, F90, x5);
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0, vec3 F90)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return mix(F0, F90, x5);
}

// Generalized Schlick Fresnel with a variable exponent
float mx_fresnel_schlick(float cosTheta, float F0, float F90, float exponent)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    return mix(F0, F90, pow(x, exponent));
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0, vec3 F90, float exponent)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    return mix(F0, F90, pow(x, exponent));
}

// Compute the average of an anisotropic roughness pair
float mx_average_roughness(vec2 roughness)
{
    return sqrt(roughness.x * roughness.y);
}

// https://www.graphics.rwth-aachen.de/publication/2/jgt.pdf
float mx_golden_ratio_sequence(int i)
{
    return fract((float(i) + 1.0) * M_GOLDEN_RATIO);
}

// https://people.irisa.fr/Ricardo.Marques/articles/2013/SF_CGF.pdf
vec2 mx_spherical_fibonacci(int i, int numSamples)
{
    return vec2((float(i) + 0.5) / float(numSamples), mx_golden_ratio_sequence(i));
}

// https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
// Appendix B.2 Equation 13
float mx_ggx_NDF(vec3 X, vec3 Y, vec3 H, float NdotH, float alphaX, float alphaY)
{
    float XdotH = dot(X, H);
    float YdotH = dot(Y, H);
    float denom = mx_square(XdotH / alphaX) + mx_square(YdotH / alphaY) + mx_square(NdotH);
    return 1.0 / (M_PI * alphaX * alphaY * mx_square(denom));
}

// https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
// Appendix B.1 Equation 3
float mx_ggx_PDF(vec3 X, vec3 Y, vec3 H, float NdotH, float LdotH, float alphaX, float alphaY)
{
    return mx_ggx_NDF(X, Y, H, NdotH, alphaX, alphaY) * NdotH / (4.0 * LdotH);
}

// https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
// Appendix B.2 Equation 15
vec3 mx_ggx_importance_sample_NDF(vec2 Xi, vec3 X, vec3 Y, vec3 N, float alphaX, float alphaY)
{
    float phi = 2.0 * M_PI * Xi.x;
    float tanTheta = sqrt(Xi.y / (1.0 - Xi.y));
    vec3 H = X * (tanTheta * alphaX * cos(phi)) +
             Y * (tanTheta * alphaY * sin(phi)) +
             N;
    return normalize(H);
}

// http://jcgt.org/published/0003/02/03/paper.pdf
// Equations 72 and 99
float mx_ggx_smith_G(float NdotL, float NdotV, float alpha)
{
    float alpha2 = mx_square(alpha);
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * mx_square(NdotL));
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * mx_square(NdotV));
    return 2.0 / (lambdaL / NdotL + lambdaV / NdotV);
}

// https://www.unrealengine.com/blog/physically-based-shading-on-mobile
vec3 mx_ggx_directional_albedo_curve_fit(float NdotV, float roughness, vec3 F0, vec3 F90)
{
    const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4( 1,  0.0425,  1.04, -0.04 );
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
    return F0 * AB.x + F90 * AB.y;
}

vec3 mx_ggx_directional_albedo_table_lookup(float NdotV, float roughness, vec3 F0, vec3 F90)
{
#if DIRECTIONAL_ALBEDO_METHOD == 1
    vec2 res = textureSize(u_albedoTable, 0);
    if (res.x > 1)
    {
        vec2 AB = texture(u_albedoTable, vec2(NdotV, roughness)).rg;
        return F0 * AB.x + F90 * AB.y;
    }
#endif
    return vec3(0.0);
}

// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec3 mx_ggx_directional_albedo_importance_sample(float NdotV, float roughness, vec3 F0, vec3 F90)
{
    NdotV = clamp(NdotV, M_FLOAT_EPS, 1.0);
    vec3 V = vec3(sqrt(1.0f - mx_square(NdotV)), 0, NdotV);

    vec2 AB = vec2(0.0);
    const int SAMPLE_COUNT = 64;
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = mx_spherical_fibonacci(i, SAMPLE_COUNT);

        // Compute the half vector and incoming light direction.
        vec3 H = mx_ggx_importance_sample_NDF(Xi, vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), roughness, roughness);
        vec3 L = -reflect(V, H);
        
        // Compute dot products for this sample.
        float NdotL = clamp(L.z, M_FLOAT_EPS, 1.0);
        float NdotH = clamp(H.z, M_FLOAT_EPS, 1.0);
        float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

        // Compute the Fresnel term.
        float Fc = mx_fresnel_schlick(VdotH, 0.0, 1.0);

        // Compute the geometric visibility term.
        float Gvis = mx_ggx_smith_G(NdotL, NdotV, roughness) * VdotH / (NdotH * NdotV);
        
        // Add the contribution of this sample.
        AB += vec2(Gvis * (1.0 - Fc), Gvis * Fc);
    }

    // Normalize integrated terms.
    AB /= float(SAMPLE_COUNT);

    // Return the final directional albedo.
    return F0 * AB.x + F90 * AB.y;
}

vec3 mx_ggx_directional_albedo(float NdotV, float roughness, vec3 F0, vec3 F90)
{
#if DIRECTIONAL_ALBEDO_METHOD == 0
    return mx_ggx_directional_albedo_curve_fit(NdotV, roughness, F0, F90);
#elif DIRECTIONAL_ALBEDO_METHOD == 1
    return mx_ggx_directional_albedo_table_lookup(NdotV, roughness, F0, F90);
#else
    return mx_ggx_directional_albedo_importance_sample(NdotV, roughness, F0, F90);
#endif
}

float mx_ggx_directional_albedo(float NdotV, float roughness, float F0, float F90)
{
    return mx_ggx_directional_albedo(NdotV, roughness, vec3(F0), vec3(F90)).x;
}

// https://blog.selfshadow.com/publications/turquin/ms_comp_final.pdf
// Equations 14 and 16
vec3 mx_ggx_energy_compensation(float NdotV, float roughness, vec3 Fss)
{
    float Ess = mx_ggx_directional_albedo(NdotV, roughness, 1.0, 1.0);
    return 1.0 + Fss * (1.0 - Ess) / Ess;
}

float mx_ggx_energy_compensation(float NdotV, float roughness, float Fss)
{
    return mx_ggx_energy_compensation(NdotV, roughness, vec3(Fss)).x;
}

// Convert a real-valued index of refraction to normal-incidence reflectivity.
float mx_ior_to_f0(float ior)
{
    return mx_square((ior - 1.0) / (ior + 1.0));
}

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
float mx_fresnel_dielectric(float cosTheta, float ior)
{
    if (cosTheta < 0.0)
        return 1.0;

    float g =  ior*ior + cosTheta*cosTheta - 1.0;
    // Check for total internal reflection
    if (g < 0.0)
        return 1.0;

    g = sqrt(g);
    float gmc = g - cosTheta;
    float gpc = g + cosTheta;
    float x = gmc / gpc;
    float y = (gpc * cosTheta - 1.0) / (gmc * cosTheta + 1.0);
    return 0.5 * x * x * (1.0 + y * y);
}

vec3 mx_fresnel_conductor(float cosTheta, vec3 n, vec3 k)
{
   float c2 = cosTheta*cosTheta;
   vec3 n2_k2 = n*n + k*k;
   vec3 nc2 = 2.0 * n * cosTheta;

   vec3 rs_a = n2_k2 + c2;
   vec3 rp_a = n2_k2 * c2 + 1.0;
   vec3 rs = (rs_a - nc2) / (rs_a + nc2);
   vec3 rp = (rp_a - nc2) / (rp_a + nc2);

   return 0.5 * (rs + rp);
}

// Fresnel for dielectric/dielectric interface and polarized light.
void mx_fresnel_dielectric_polarized(float cosTheta, float n1, float n2, out vec2 F, out vec2 phi)
{
    float eta2 = mx_square(n1 / n2);
    float st2 = 1.0 - cosTheta*cosTheta;

    // Check for total internal reflection
    if(eta2*st2 > 1.0)
    {
        F = vec2(1.0);
        float s = sqrt(st2 - 1.0/eta2) / cosTheta;
        phi = 2.0 * atan(vec2(-eta2 * s, -s));
        return;
    }

    float cosTheta_t = sqrt(1.0 - eta2 * st2);
    vec2 r = vec2((n2*cosTheta - n1*cosTheta_t) / (n2*cosTheta + n1*cosTheta_t),
                  (n1*cosTheta - n2*cosTheta_t) / (n1*cosTheta + n2*cosTheta_t));
    F = mx_square(r);
    phi.x = (r.x < 0.0) ? M_PI : 0.0;
    phi.y = (r.y < 0.0) ? M_PI : 0.0;
}

// Fresnel for dielectric/conductor interface and polarized light.
// TODO: Optimize this functions and support wavelength dependent complex refraction index.
void mx_fresnel_conductor_polarized(float cosTheta, float n1, float n2, float k, out vec2 F, out vec2 phi)
{
    if (k == 0.0)
    {
        // Use dielectric formula to avoid numerical issues
        mx_fresnel_dielectric_polarized(cosTheta, n1, n2, F, phi);
        return;
    }

    float A = mx_square(n2) * (1.0 - mx_square(k)) - mx_square(n1) * (1.0 - mx_square(cosTheta));
    float B = sqrt(mx_square(A) + mx_square(2.0 * mx_square(n2) * k));
    float U = sqrt((A+B) / 2.0);
    float V = sqrt((B-A) / 2.0);

    F.y = (mx_square(n1*cosTheta - U) + mx_square(V)) / (mx_square(n1*cosTheta + U) + mx_square(V));
    phi.y = atan(2.0*n1 * V*cosTheta, mx_square(U) + mx_square(V) - mx_square(n1*cosTheta)) + M_PI;

    F.x = (mx_square(mx_square(n2) * (1.0 - mx_square(k)) * cosTheta - n1*U) + mx_square(2.0 * mx_square(n2) * k * cosTheta - n1*V)) /
            (mx_square(mx_square(n2) * (1.0 - mx_square(k)) * cosTheta + n1*U) + mx_square(2.0 * mx_square(n2) * k * cosTheta + n1*V));
    phi.x = atan(2.0 * n1 * mx_square(n2) * cosTheta * (2.0*k*U - (1.0 - mx_square(k)) * V), mx_square(mx_square(n2) * (1.0 + mx_square(k)) * cosTheta) - mx_square(n1) * (mx_square(U) + mx_square(V)));
}

// XYZ to CIE 1931 RGB color space (using neutral E illuminant)
const mat3 XYZ_TO_RGB = mat3(2.3706743, -0.5138850, 0.0052982, -0.9000405, 1.4253036, -0.0146949, -0.4706338, 0.0885814, 1.0093968);

// Depolarization functions for natural light
float mx_depolarize(vec2 v)
{
    return 0.5 * (v.x + v.y);
}
vec3 mx_depolarize(vec3 s, vec3 p)
{
    return 0.5 * (s + p);
}

// Evaluation XYZ sensitivity curves in Fourier space
vec3 mx_eval_sensitivity(float opd, float shift)
{
    // Use Gaussian fits, given by 3 parameters: val, pos and var
    float phase = 2.0*M_PI * opd;
    vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);
    vec3 xyz = val * sqrt(2.0*M_PI * var) * cos(pos * phase + shift) * exp(- var * phase*phase);
    xyz.x   += 9.7470e-14 * sqrt(2.0*M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift) * exp(- 4.5282e+09 * phase*phase);
    return xyz / 1.0685e-7;
}

// A Practical Extension to Microfacet Theory for the Modeling of Varying Iridescence
// https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
vec3 mx_fresnel_airy(float cosTheta, vec3 ior, vec3 extinction, float tf_thickness, float tf_ior)
{
    // Convert nm -> m
    float d = tf_thickness * 1.0e-9;

    // Assume vacuum on the outside
    float eta1 = 1.0;
    float eta2 = tf_ior;

    // Optical path difference
    float cosTheta2 = sqrt(1.0 - mx_square(eta1/eta2) * (1.0 - mx_square(cosTheta)));
    float D = 2.0 * eta2 * d * cosTheta2;

    // First interface
    vec2 R12, phi12;
    mx_fresnel_dielectric_polarized(cosTheta, eta1, eta2, R12, phi12);
    vec2 R21  = R12;
    vec2 T121 = vec2(1.0) - R12;
    vec2 phi21 = vec2(M_PI) - phi12;

    // Second interface
    vec2 R23, phi23;
    mx_fresnel_conductor_polarized(cosTheta2, eta2, ior.x, extinction.x, R23, phi23);

    // Phase shift
    vec2 phi2 = phi21 + phi23;

    // Compound terms
    vec3 R = vec3(0.0);
    vec2 R123 = R12*R23;
    vec2 r123 = sqrt(R123);
    vec2 Rs   = mx_square(T121)*R23 / (1.0-R123);

    // Reflectance term for m=0 (DC term amplitude)
    vec2 C0 = R12 + Rs;
    vec3 S0 = mx_eval_sensitivity(0.0, 0.0);
    R += mx_depolarize(C0) * S0;

    // Reflectance term for m>0 (pairs of diracs)
    vec2 Cm = Rs - T121;
    for (int m=1; m<=3; ++m)
    {
        Cm *= r123;
        vec3 SmS = 2.0 * mx_eval_sensitivity(float(m)*D, float(m)*phi2.x);
        vec3 SmP = 2.0 * mx_eval_sensitivity(float(m)*D, float(m)*phi2.y);
        R += mx_depolarize(Cm.x*SmS, Cm.y*SmP);
    }

    // Convert back to RGB reflectance
    R = clamp(XYZ_TO_RGB * R, vec3(0.0), vec3(1.0));

    return R;
}

// Parameters for Fresnel calculations.
struct FresnelData
{
    vec3 ior;        // In Schlick Fresnel mode these two
    vec3 extinction; // hold F0 and F90 reflectance values
    float exponent;
    float tf_thickness;
    float tf_ior;
    int model;
};

FresnelData mx_init_fresnel_dielectric(float ior)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 0;
    fd.ior = vec3(ior);
    fd.tf_thickness = 0.0f;
    return fd;
}

FresnelData mx_init_fresnel_conductor(vec3 ior, vec3 extinction)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 1;
    fd.ior = ior;
    fd.extinction = extinction;
    fd.tf_thickness = 0.0f;
    return fd;
}

FresnelData mx_init_fresnel_schlick(vec3 F0)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 2;
    fd.ior = F0;
    fd.extinction = vec3(1.0);
    fd.exponent = 5.0f;
    fd.tf_thickness = 0.0f;
    return fd;
}

FresnelData mx_init_fresnel_schlick(vec3 F0, vec3 F90, float exponent)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 2;
    fd.ior = F0;
    fd.extinction = F90;
    fd.exponent = exponent;
    fd.tf_thickness = 0.0f;
    return fd;
}

FresnelData mx_init_fresnel_dielectric_airy(float ior, float tf_thickness, float tf_ior)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 3;
    fd.ior = vec3(ior);
    fd.extinction = vec3(0.0);
    fd.tf_thickness = tf_thickness;
    fd.tf_ior = tf_ior;
    return fd;
}

FresnelData mx_init_fresnel_conductor_airy(vec3 ior, vec3 extinction, float tf_thickness, float tf_ior)
{
    FresnelData fd = FresnelData(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, -1);
    fd.model = 3;
    fd.ior = ior;
    fd.extinction = extinction;
    fd.tf_thickness = tf_thickness;
    fd.tf_ior = tf_ior;
    return fd;
}

vec3 mx_compute_fresnel(float cosTheta, FresnelData fd)
{
    if (fd.model == 0)
        return vec3(mx_fresnel_dielectric(cosTheta, fd.ior.x));
    else if (fd.model == 1)
        return mx_fresnel_conductor(cosTheta, fd.ior, fd.extinction);
    else if (fd.model == 2)
        // ior & extinction holds F0 & F90
        return mx_fresnel_schlick(cosTheta, fd.ior, fd.extinction, fd.exponent);
    else
        return mx_fresnel_airy(cosTheta, fd.ior, fd.extinction, fd.tf_thickness, fd.tf_ior);
}

vec3 mx_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 roughness, int distribution, FresnelData fd)
{
    return vec3(0.0);
}

vec3 mx_environment_irradiance(vec3 N)
{
    return vec3(0.0);
}

void mx_directional_light(LightData light, vec3 position, out lightshader result)
{
    result.direction = -light.direction;
    result.intensity = light.color * light.intensity;
}

int numActiveLightSources()
{
    return min(u_numActiveLightSources, MAX_LIGHT_SOURCES) ;
}

void sampleLightSource(LightData light, vec3 position, out lightshader result)
{
    result.intensity = vec3(0.0);
    result.direction = vec3(0.0);
    if (light.type == 1)
    {
        mx_directional_light(light, position, result);
    }
}


// Based on the OSL implementation of Oren-Nayar diffuse, which is in turn
// based on https://mimosa-pudica.net/improved-oren-nayar.html.
float mx_oren_nayar_diffuse(vec3 L, vec3 V, vec3 N, float NdotL, float roughness)
{
    float LdotV = clamp(dot(L, V), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float s = LdotV - NdotL * NdotV;
    float stinv = (s > 0.0f) ? s / max(NdotL, NdotV) : 0.0;

    float sigma2 = mx_square(roughness * M_PI);
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return A + B * stinv;
}

// https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
// Section 5.3
float mx_burley_diffuse(vec3 L, vec3 V, vec3 N, float NdotL, float roughness)
{
    vec3 H = normalize(L + V);
    float LdotH = clamp(dot(L, H), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    float F90 = 0.5 + (2.0 * roughness * mx_square(LdotH));
    float refL = mx_fresnel_schlick(NdotL, 1.0, F90);
    float refV = mx_fresnel_schlick(NdotV, 1.0, F90);
    return refL * refV;
}

// Compute the directional albedo component of Burley diffuse for the given
// view angle and roughness.  Curve fit provided by Stephen Hill.
float mx_burley_diffuse_directional_albedo(float NdotV, float roughness)
{
    float x = NdotV;
    float fit0 = 0.97619 - 0.488095 * mx_pow5(1.0 - x);
    float fit1 = 1.55754 + (-2.02221 + (2.56283 - 1.06244 * x) * x) * x;
    return mix(fit0, fit1, roughness);
}

// Evaluate the Burley diffusion profile for the given distance and diffusion shape.
// Based on https://graphics.pixar.com/library/ApproxBSSRDF/
vec3 mx_burley_diffusion_profile(float dist, vec3 shape)
{
    vec3 num1 = exp(-shape * dist);
    vec3 num2 = exp(-shape * dist / 3.0);
    float denom = max(dist, M_FLOAT_EPS);
    return (num1 + num2) / denom;
}

// Integrate the Burley diffusion profile over a sphere of the given radius.
// Inspired by Eric Penner's presentation in http://advances.realtimerendering.com/s2011/
vec3 mx_integrate_burley_diffusion(vec3 N, vec3 L, float radius, vec3 mfp)
{
    float theta = acos(dot(N, L));

    // Estimate the Burley diffusion shape from mean free path.
    vec3 shape = vec3(1.0) / max(mfp, 0.1);

    // Integrate the profile over the sphere.
    vec3 sumD = vec3(0.0);
    vec3 sumR = vec3(0.0);
    const int SAMPLE_COUNT = 32;
    const float SAMPLE_WIDTH = (2.0 * M_PI) / float(SAMPLE_COUNT);
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float x = -M_PI + (float(i) + 0.5) * SAMPLE_WIDTH;
        float dist = radius * abs(2.0 * sin(x * 0.5));
        vec3 R = mx_burley_diffusion_profile(dist, shape);
        sumD += R * max(cos(theta + x), 0.0);
        sumR += R;
    }

    return sumD / sumR;
}

vec3 mx_subsurface_scattering_approx(vec3 N, vec3 L, vec3 P, vec3 albedo, vec3 mfp)
{
    float curvature = length(fwidth(N)) / length(fwidth(P));
    float radius = 1.0 / max(curvature, 0.01);
    return albedo * mx_integrate_burley_diffusion(N, L, radius, mfp) / vec3(M_PI);
}

void mx_burley_diffuse_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 color, float roughness, vec3 normal, out BSDF result)
{
    if (weight < M_FLOAT_EPS)
    {
        result = BSDF(0.0);
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    float NdotL = clamp(dot(normal, L), M_FLOAT_EPS, 1.0);

    result = color * occlusion * weight * NdotL * M_PI_INV;
    result *= mx_burley_diffuse(L, V, normal, NdotL, roughness);
    return;
}

void mx_burley_diffuse_bsdf_indirect(vec3 V, float weight, vec3 color, float roughness, vec3 normal, out BSDF result)
{
    if (weight < M_FLOAT_EPS)
    {
        result = BSDF(0.0);
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    float NdotV = clamp(dot(normal, V), M_FLOAT_EPS, 1.0);

    vec3 Li = mx_environment_irradiance(normal) *
              mx_burley_diffuse_directional_albedo(NdotV, roughness);
    result = Li * color * weight;
}

void main()
{
    vec3 geomprop_Nworld_out = normalize(normalWorld);

    surfaceshader surface1_out = surfaceshader(vec3(0.0),vec3(0.0));
    {
        // Shadow occlusion
        float occlusion = 1.0;

        vec3 N = normalize(normalWorld);
        vec3 V = normalize(u_viewPosition - positionWorld);
        vec3 P = positionWorld;
        // Light loop
        int numLights = numActiveLightSources();
        lightshader lightShader;
        for (int activeLightIndex = 0; activeLightIndex < numLights; ++activeLightIndex)
        {
            sampleLightSource(u_lightData[activeLightIndex], positionWorld, lightShader);
            vec3 L = lightShader.direction;

            // Calculate the BSDF response for this light source
            BSDF burley_brdf1_out = BSDF(0.0);
            mx_burley_diffuse_bsdf_reflection(L, V, P, occlusion, burley_brdf1_weight, burley_brdf1_color, burley_brdf1_roughness, geomprop_Nworld_out, burley_brdf1_out);

            // Accumulate the light's contribution
            surface1_out.color += lightShader.intensity * burley_brdf1_out;
        }

        // Ambient occlusion
        occlusion = 1.0;

        // Add surface emission
        {
            surface1_out.color += EDF(0.0);
        }

        // Add indirect contribution
        {
            BSDF burley_brdf1_out = BSDF(0.0);
            mx_burley_diffuse_bsdf_indirect(V, burley_brdf1_weight, burley_brdf1_color, burley_brdf1_roughness, geomprop_Nworld_out, burley_brdf1_out);

            surface1_out.color += occlusion * burley_brdf1_out;
        }

        surface1_out.transparency = vec3(0.0);
    }

    out1 = vec4(surface1_out.color, 1.0);
}

#ifndef __BRDF_HLSL__
#define __BRDF_HLSL__

#include "mathconstants.hlsl"
#include "lights.hlsl"
#include "shadingdata.hlsl"

#define DIFFUSE_BRDF_LAMBERT
//#define DIFFUSE_BRDF_DISNEY
//#define DIFFUSE_BRDF_FROSTBITE

float3 fresnelSchlick(float3 f0, float3 f90, float u)
{
    return f0 + (f90 - f0) * pow(1 - u, 5);
}

/** Disney's diffuse term. Based on https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
*/
float disneyDiffuseFresnel(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float fd90 = 0.5 + 2 * LdotH * LdotH * linearRoughness;
    float fd0 = 1;
    float lightScatter = fresnelSchlick(fd0, fd90, NdotL).r;
    float viewScatter = fresnelSchlick(fd0, fd90, NdotV).r;
    return lightScatter * viewScatter;
}

float3 evalDiffuseDisneyBrdf(ShadingData sd, LightSample ls)
{
    return disneyDiffuseFresnel(sd.NdotV, ls.NdotL, ls.LdotH, sd.linearRoughness) * M_1_PI * sd.diffuse.rgb;
}

/** Lambertian diffuse
*/
float3 evalDiffuseLambertBrdf(ShadingData sd, LightSample ls)
{
    return sd.diffuse.rgb * M_1_PI;
}

/** Frostbites's diffuse term. Based on https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
*/
float3 evalDiffuseFrostbiteBrdf(ShadingData sd, LightSample ls)
{
    float energyBias = lerp(0, 0.5, sd.linearRoughness);
    float energyFactor = lerp(1, 1.0 / 1.51, sd.linearRoughness);

    float fd90 = energyBias + 2 * ls.LdotH * ls.LdotH * sd.linearRoughness;
    float fd0 = 1;
    float lightScatter = fresnelSchlick(fd0, fd90, ls.NdotL).r;
    float viewScatter = fresnelSchlick(fd0, fd90, sd.NdotV).r;
    return (viewScatter * lightScatter * energyFactor * M_1_PI) * sd.diffuse.rgb;
}

float3 evalDiffuseBrdf(ShadingData sd, LightSample ls)
{
#if defined(DIFFUSE_BRDF_LAMBERT)
    return evalDiffuseLambertBrdf(sd, ls);
#elif defined(DIFFUSE_BRDF_DISNEY)
    return evalDiffuseDisneyBrdf(sd, ls);
#elif defined(DIFFUSE_BRDF_FROSTBITE)
    return evalDiffuseFrostbiteBrdf(sd, ls);
#endif
}

float evalGGX(float ggxAlpha, float NdotH)
{
    float a2 = ggxAlpha * ggxAlpha;
    float d = ((NdotH * a2 - NdotH) * NdotH + 1);
    return a2 / (d * d);
}

float evalSmithGGX(float NdotL, float NdotV, float ggxAlpha)
{
    // Optimized version of Smith, already taking into account the division by (4 * NdotV)
    float a2 = ggxAlpha * ggxAlpha;
    // `NdotV *` and `NdotL *` are inversed. It's not a mistake.
    float ggxv = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float ggxl = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    return 0.5f / (ggxv + ggxl);
}

float3 evalSpecularBrdf(ShadingData sd, LightSample ls)
{
    float ggxAlpha = sd.ggxAlpha;

    float D = evalGGX(ggxAlpha, ls.NdotH);
    float G = evalSmithGGX(ls.NdotL, sd.NdotV, ggxAlpha);
    float3 F = fresnelSchlick(sd.specular, 1, saturate(ls.LdotH));
    return D * G * F * M_1_PI;
}

#endif // #define __BRDF_HLSL__

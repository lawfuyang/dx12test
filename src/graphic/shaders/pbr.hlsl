#ifndef __PBR_HLSL__
#define __PBR_HLSL__

// Shlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
float3 Fresnel_Shlick(in float3 f0, in float3 f90, in float x)
{
    return f0 + (f90 - f0) * pow(1.f - x, 5.f);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float DiffuseBurley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2.f * roughness * LdotH * LdotH;
    return Fresnel_Shlick(1, fd90, NdotL).x * Fresnel_Shlick(1, fd90, NdotV).x;
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(in float alpha, in float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1)) + 1;
    return alpha2 / max(M_TOLERANCE, M_PI * lower * lower);
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
// http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html
float G_Shlick_Smith_Hable(float alpha, float LdotH)
{
    return rcp(lerp(LdotH * LdotH, 1, alpha * alpha * 0.25f));
}

// A microfacet based BRDF.
// alpha:         This is roughness * roughness as in the "Disney" PBR model by Burley et al.
// specularColor: The F0 reflectance value - 0.04 for non-metals, or RGB for metals. This follows model used by Unreal Engine 4.
float3 SpecularBRDF(in float alpha, in float3 specularColor, in float NdotV, in float NdotL, in float LdotH, in float NdotH)
{
    // Specular D (microfacet normal distribution) component
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // Specular Fresnel
    float3 specular_F = Fresnel_Shlick(specularColor, 1, LdotH);

    // Specular G (visibility) component
    float specular_G = G_Shlick_Smith_Hable(alpha, LdotH);

    return specular_D * specular_F * specular_G;
}

struct CommonPBRParams
{
    float3 N;
    float3 V;
    float roughness;

    // Blended base colors
    float3 baseDiffuse;
    float3 baseSpecular;
};

struct EvaluateLightPBRParams
{
    CommonPBRParams Common;

    float3 lightColor;
    float3 L;
};

float3 EvaluateLightPBR(in EvaluateLightPBRParams input)
{
    const float NdotV = saturate(dot(input.Common.N, input.Common.V));

    // Burley roughness bias
    const float alpha = input.Common.roughness * input.Common.roughness;

    // Half vector
    const float3 H = normalize(input.L + input.Common.V);

    // products
    const float NdotL = saturate(dot(input.Common.N, input.L));
    const float LdotH = saturate(dot(input.L, H));
    const float NdotH = saturate(dot(input.Common.N, H));

    // Diffuse & specular factors
    float diffuse_factor = DiffuseBurley(NdotL, NdotV, LdotH, input.Common.roughness);
    float3 specular = SpecularBRDF(alpha, input.Common.baseSpecular, NdotV, NdotL, LdotH, NdotH);

    // Output Directional light
    return NdotL * input.lightColor * (input.Common.baseDiffuse * diffuse_factor + specular);
}

struct EvaluateEnvPBRParams
{
    CommonPBRParams Common;

    TextureCube<float3> irradianceTexture;
    TextureCube<float3> radianceTexture;
    uint numRadianceMipLevels;
};

float3 EvaluateEnvPBR(in EvaluateEnvPBRParams input)
{
    // Output color
    float3 finalColor = 0;

    // Add diffuse irradiance
    float3 diffuse_env = input.irradianceTexture.Sample(g_AnisotropicWrapSampler, input.Common.N);
    finalColor += input.Common.baseDiffuse * diffuse_env;

    // Add specular radiance via approximate specular image based lighting by sampling radiance map at lower mips according to roughness, then modulating by Fresnel term. 
    float mip = input.Common.roughness * input.numRadianceMipLevels;
    float3 dir = reflect(-input.Common.V, input.Common.N);
    float3 specular_env = input.radianceTexture.SampleLevel(g_AnisotropicWrapSampler, dir, mip);
    finalColor += input.Common.baseSpecular * specular_env;

    return finalColor;
}

#endif // #define __PBR_HLSL__

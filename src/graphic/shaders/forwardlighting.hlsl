
#include "common.hlsl"
#include "commonvertexformats.hlsl"
#include "brdf.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"

Texture2D<float4> g_DiffuseTexture : register(t0);
Texture2D<float4> g_NormalTexture : register(t1);

struct VS_OUT
{
    float4 m_Position  : SV_POSITION;
    float3 m_Normal    : NORMAL;
    float2 m_TexCoord  : TEXCOORD;
    float3 m_Tangent   : TANGENT;
    float3 m_Bitangent : BINORMAL;

    float3 m_PositionW : POSITIONT;
};

#if defined(VERTEX_SHADER)
VS_OUT VSMain(VS_IN input)
{
    VS_OUT result = (VS_OUT)0;

    float4 position;
#if defined(VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
    position = float4(input.m_Position, 0, 1);
#elif defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    position = float4(input.m_Position, 1);
#endif

    result.m_Position = mul(position, g_ViewProjMatrix);
    result.m_TexCoord = input.m_TexCoord;
    result.m_Bitangent = 0;

#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    result.m_Normal = input.m_Normal;
    result.m_Tangent = input.m_Tangent;
    result.m_Bitangent = cross(input.m_Tangent, input.m_Normal);
#endif
    
    // TODO: model to world transform. DX samples have none
    result.m_PositionW = position.xyz;

    return result;
}
#endif

#if defined(PIXEL_SHADER)

//--------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform.
//--------------------------------------------------------------------------------------
float3 CalcPerPixelNormal(float2 vTexcoord, float3 vVertNormal, float3 vVertTangent, float3 vVertBinormal)
{
    float3x3 mTangentSpaceToWorldSpace = float3x3(vVertTangent, vVertBinormal, vVertNormal);

    // Compute per-pixel normal.
    float3 vBumpNormal = (float3)g_NormalTexture.Sample(g_AnisotropicWrapSampler, vTexcoord);
    vBumpNormal = 2.0f * vBumpNormal - 1.0f;

    return mul(vBumpNormal, mTangentSpaceToWorldSpace);
}

struct ShadingResult
{
    float3 diffuseBrdf;             // The result of the diffuse BRDF
    float3 specularBrdf;            // The result of the specular BRDF
    float3 diffuse;                 // The diffuse component of the result
    float3 specular;                // The specular component of the result
    float3 color;                   // The final color. Alpha holds the opacity valua
};

LightSample InitDirectionalLight(in float3 surfacePosW, ShadingData sd)
{
    LightSample ls;
    ls.diffuse = g_SceneLightIntensity.rgb;
    ls.specular = g_SceneLightIntensity.rgb;
    ls.L = g_SceneLightDir.xyz;

    float3 H = normalize(sd.V + ls.L);
    ls.NdotH = dot(sd.N, H);
    ls.NdotL = dot(sd.N, ls.L);
    ls.LdotH = dot(ls.L, H);

    return ls;
}

ShadingResult EvaluateDirectionLight(ShadingData sd, float shadowFactor)
{
    ShadingResult sr = (ShadingResult)0;
    LightSample ls = InitDirectionalLight(sd.posW, sd);

    // If the light doesn't hit the surface or we are viewing the surface from the back, return
    //if (ls.NdotL <= 0) return sr;

    // Calculate the diffuse term
    sr.diffuseBrdf = EvalDiffuseBrdf(sd, ls);
    sr.diffuse = ls.diffuse * sr.diffuseBrdf * ls.NdotL;
    sr.color = sr.diffuse;

    //// Calculate the specular term
    //sr.specularBrdf = EvalSpecularBrdf(sd, ls);
    //sr.specular = ls.specular * sr.specularBrdf * ls.NdotL;
    //sr.color += sr.specular;

    // Apply the shadow factor
    sr.color *= shadowFactor;

    return sr;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    // TODO: texture input for these
    float occlusion = 0.5f;
    float roughness = 0.5f;
    float metallic = 0.5f;

    float4 baseColor = g_DiffuseTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);

    ShadingData sd = (ShadingData)0;
    sd.posW = input.m_PositionW;
    sd.V = normalize(g_CameraPosition.xyz - input.m_PositionW);
    sd.N = CalcPerPixelNormal(input.m_TexCoord, input.m_Normal, input.m_Tangent, input.m_Bitangent);
    sd.T = input.m_Tangent;
    sd.B = input.m_Bitangent;
    sd.uv = input.m_TexCoord;
    sd.diffuse = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), metallic);
    sd.metallic = metallic;
    sd.linearRoughness = roughness;
    sd.NdotV = saturate(dot(sd.N, sd.V));

#if !defined(DIFFUSE_BRDF_LAMBERT)
    sd.linearRoughness = max(0.08, sd.linearRoughness); // Clamp the roughness so that the BRDF won't explode
#endif

    sd.roughness = sd.linearRoughness * sd.linearRoughness;

    float4 finalColor = float4(0, 0, 0, 1);

    float dirLightShadowFactor = 1.0f; // TODO
    finalColor.rgb += EvaluateDirectionLight(sd, dirLightShadowFactor).color;

    return finalColor;
}

#endif

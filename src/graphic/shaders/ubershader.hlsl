
#include "common.hlsl"
#include "commonvertexformats.hlsl"
#include "brdf.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"

Texture2D<float4> g_DiffuseTexture : register(t0);
Texture2D<float4> g_NormalTexture : register(t1);

#if defined(VERTEX_SHADER)
VS_OUT VSMain(VS_IN input)
{
    VS_OUT result = (VS_OUT)0;

#if defined(VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
    result.m_Position = mul(float4(input.m_Position, 0, 1), g_ViewProjMatrix);
#elif defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    result.m_Position = mul(float4(input.m_Position, 1), g_ViewProjMatrix);
#endif

    result.m_TexCoord = input.m_TexCoord;

#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    result.m_Normal = input.m_Normal;
    result.m_Tangent = input.m_Tangent;
#endif

    return result;
}
#endif

#if defined(PIXEL_SHADER)

//--------------------------------------------------------------------------------------
// Diffuse lighting calculation for main Directional Light
//--------------------------------------------------------------------------------------
float4 CalcDirLightColor(float4 lightDir, float3 normal)
{
    float fNDotL = saturate(dot(lightDir.xyz, normal));
    return fNDotL * g_SceneLightIntensity;
}

//--------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform.
//--------------------------------------------------------------------------------------
float3 CalcPerPixelNormal(float2 vTexcoord, float3 vVertNormal, float3 vVertTangent)
{
    float3 vVertBinormal = cross(vVertTangent, vVertNormal);
    float3x3 mTangentSpaceToWorldSpace = float3x3(vVertTangent, vVertBinormal, vVertNormal);

    // Compute per-pixel normal.
    float3 vBumpNormal = (float3)g_NormalTexture.Sample(g_AnisotropicWrapSampler, vTexcoord);
    vBumpNormal = 2.0f * vBumpNormal - 1.0f;

    return mul(vBumpNormal, mTangentSpaceToWorldSpace);
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float4 diffuse = g_DiffuseTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    float3 normal = CalcPerPixelNormal(input.m_TexCoord, input.m_Normal, input.m_Tangent);
    float4 finalLight = CalcDirLightColor(g_SceneLightDir, normal); // dir light
    finalLight = max(0.25f, finalLight); // some ambient

    float4 color = diffuse * finalLight;
    return color;
}

#endif

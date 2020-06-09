
#include "staticsamplers.hlsl"
#include "commonvertexformats.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"

Texture2D<float4> g_Texture : register(t0);

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

float4 CalcDirLightColor(float3 lightDir, float3 normal)
{
    float fNDotL = saturate(dot(lightDir, normal));
    return fNDotL;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float4 diffuse = g_Texture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    float4 finalLight = CalcDirLightColor(g_SceneLightDir, input.m_Normal); // dir light
    finalLight = max(0.25f, finalLight); // some ambient

    float4 color = diffuse * finalLight;
    return color;
}
#endif

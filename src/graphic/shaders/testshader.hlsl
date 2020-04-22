// ShaderDeclaration:VS_TestTriangle EntryPoint:VSMain
// ShaderDeclaration:PS_TestTriangle EntryPoint:PSMain

cbuffer TestShaderConsts : register(b1)
{
    float4x4 g_ViewProjMatrix;
};
// EndShaderDeclaration

Texture2D<float4> g_Texture : register(t0);

#include "common.hlsl"

struct VS_IN
{
    float4 m_Position : POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
};

struct VS_OUT
{
    float4 m_Position : SV_POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT result;

    result.m_Position = input.m_Position;
    result.m_Position = mul(result.m_Position, g_ViewProjMatrix);

    result.m_Normal = input.m_Normal;
    result.m_TexCoord = input.m_TexCoord;

    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return float4(abs(input.m_Normal), 1.0f) * g_Texture.Sample(g_AnisotropicClampSampler, input.m_TexCoord);
}

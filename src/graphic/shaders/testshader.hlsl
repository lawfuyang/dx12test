#include "common.hlsl"

Texture2D g_Texture : register(t0);

struct VS_IN
{
    float4 m_Position : POSITION;
    float2 m_TexCoord : TEXCOORD;
};

struct VS_OUT
{
    float4 m_Position : SV_POSITION;
    float2 m_TexCoord : TEXCOORD;
};

VS_OUT VSMain(VS_IN input)
{
    PSInput result;

    result.m_Position = input.m_Position;
    result.m_TexCoord = input.m_TexCoord;

    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return g_Texture.Sample(gs_AnisotropicClampSampler, input.m_TexCoord);
}

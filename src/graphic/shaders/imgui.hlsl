
#include "staticsamplers.hlsl"

#include "autogen/hlsl/IMGUICbuffer.h"

Texture2D<float4> g_IMGUITexture : register(t0);

struct VS_IN
{
    float2 m_Position : POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color    : COLOR;
};

struct VS_OUT
{
    float4 m_Position : SV_POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color : COLOR;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT result;

    result.m_Position = mul(float4(input.m_Position, 0, 1), g_ProjMatrix);
    result.m_TexCoord = input.m_TexCoord;
    result.m_Color = input.m_Color;

    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return input.m_Color * g_IMGUITexture.Sample(g_PointClampSampler, input.m_TexCoord);
}

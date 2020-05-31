// ShaderDeclaration:VS_IMGUI EntryPoint:VSMain
// ShaderDeclaration:PS_IMGUI EntryPoint:PSMain

cbuffer IMGUICBuffer : register(b0)
{
    float4x4 g_ProjMatrix;
};
// EndShaderDeclaration

Texture2D<float4> g_IMGUITexture : register(t0);

#include "common.hlsl"

struct VS_IN
{
    float4 m_Position : POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color : COLOR;
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

    result.m_Position = mul(input.m_Position, g_ProjMatrix);
    result.m_TexCoord = input.m_TexCoord;
    result.m_Color = input.m_Color;

    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return input.m_Color * g_IMGUITexture.Sample(g_PointClampSampler, input.m_TexCoord);
}

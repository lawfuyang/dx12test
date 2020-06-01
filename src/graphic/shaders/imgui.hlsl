// ShaderDeclaration:VS_IMGUI EntryPoint:VSMain
// ShaderDeclaration:PS_IMGUI EntryPoint:PSMain

cbuffer IMGUICBuffer : register(b0)
{
    float4x4 g_ProjMatrix;
};
// EndShaderDeclaration

Texture2D<float4> g_IMGUITexture : register(t0);

// Temporary until I get the ShaderCompiler to generate the proper UberShader permutations
#define VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub

#include "common.hlsl"

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

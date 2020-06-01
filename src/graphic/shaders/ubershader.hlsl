// ShaderDeclaration:VS_UberShader EntryPoint:VSMain
// ShaderDeclaration:PS_UberShader EntryPoint:PSMain

cbuffer PerFrameConsts : register(b0)
{
    float4x4 g_ViewProjMatrix;
};
// EndShaderDeclaration

Texture2D<float4> g_Texture : register(t0);

// Temporary until I get the ShaderCompiler to generate the proper UberShader permutations
#define VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f

#include "common.hlsl"

VS_OUT VSMain(VS_IN input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT result;

    result.m_Position = mul(float4(input.m_Position, 1.0f), g_ViewProjMatrix);
    result.m_Normal = input.m_Normal;
    result.m_TexCoord = input.m_TexCoord;
    result.m_Tangent = input.m_Tangent;

    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float4 diffuse = g_Texture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    return diffuse;
}

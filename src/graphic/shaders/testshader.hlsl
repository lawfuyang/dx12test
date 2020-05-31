// ShaderDeclaration:VS_TestTriangle EntryPoint:VSMain
// ShaderDeclaration:PS_TestTriangle EntryPoint:PSMain

cbuffer TestShaderConsts : register(b0)
{
    float4x4 g_ViewProjMatrix;
};
// EndShaderDeclaration

Texture2D<float4> g_Texture : register(t0);

#include "common.hlsl"

struct VS_IN
{
    float3 m_Position : POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
    float3 m_Tangent  : TANGENT;

    uint m_VertexID   : SV_VertexID;
    uint m_InstanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 m_Position : SV_POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
    float3 m_Tangent  : TANGENT;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT result;

    result.m_Position = mul(float4(input.m_Position, 1.0f), g_ViewProjMatrix);
    result.m_Normal = input.m_Normal;
    result.m_TexCoord = input.m_TexCoord;
    result.m_Tangent = input.m_Tangent;

    return result;
}

static const float3 HardCodedDirLight = float3(0.5773f, 0.5773f, 0.5773f);

float4 PSMain(VS_OUT input) : SV_TARGET
{
    //float NdotL = dot(input.m_Normal, HardCodedDirLight);

    float4 diffuse = g_Texture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    return diffuse;
}

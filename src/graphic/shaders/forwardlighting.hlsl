
#include "common.hlsl"
#include "commonvertexformats.hlsl"
#include "pbr.hlsl"

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

float4 PSMain(VS_OUT input) : SV_TARGET
{
    // TODO: texture input for these
    float occlusion = 0.5f;
    float roughness = 0.5f;
    float metallic = 0.5f;

    float4 baseColor = g_DiffuseTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);

    return float4(baseColor.rgb, 1.0f);
}

#endif

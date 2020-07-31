
#include "common.hlsl"
#include "commonvertexformats.hlsl"
#include "pbr.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"
#include "autogen/hlsl/PerInstanceConsts.h"

static const PerFrameConsts g_PerFrameConsts = CreatePerFrameConsts();
static const PerInstanceConsts g_PerInstanceConsts = CreatePerInstanceConsts();

Texture2D<float4> g_DiffuseTexture : register(t0);
Texture2D<float4> g_NormalTexture : register(t1);
Texture2D<float4> g_ORMTexture : register(t2);

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
#elif defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f) || defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    position = float4(input.m_Position, 1);
#endif

    result.m_Position = mul(position, g_PerInstanceConsts.m_WorldMatrix);
    result.m_Position = mul(result.m_Position, g_PerFrameConsts.m_ViewProjMatrix);
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
    // Init per-pixel normal
    float3 localNormal = TwoChannelNormalX2(g_NormalTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord).xy);
    float3 normal = PeturbNormal(localNormal, input.m_PositionW.xyz, input.m_Normal, input.m_TexCoord);

    // View vector
    float3 V = normalize(g_PerFrameConsts.m_CameraPosition.xyz - input.m_PositionW.xyz);

    // Init per-pixel PBR properties
    float ambientOcclusion = 1.0;
    float roughness = 0.75;
    float metallic = 0.1;
#if !defined(USE_PBR_CONSTS)
    // R = Occlusion, G = Roughness, B = Metalness
    float3 ORM = g_ORMTexture.Sample(g_AnisotropicClampSampler, input.m_TexCoord).rgb;
    ambientOcclusion = ORM.r;
    roughness = ORM.g;
    metallic = ORM.b;
#endif

    // blended base diffuse
    float4 baseAlbedo = g_DiffuseTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    float3 baseDiffuse = lerp(baseAlbedo.rgb, float3(0, 0, 0), metallic) * ambientOcclusion;

    // Specular coefficiant - fixed reflectance value for non-metals
    static const float kSpecularCoefficient = 0.04;
    float3 baseSpecular = lerp(kSpecularCoefficient, baseAlbedo.rgb, metallic) * ambientOcclusion;

    CommonPBRParams commonPBRParams;
    commonPBRParams.N = normal;
    commonPBRParams.V = V;
    commonPBRParams.roughness = roughness;
    commonPBRParams.baseDiffuse = baseDiffuse;
    commonPBRParams.baseSpecular = baseSpecular;

    EvaluateLightPBRParams dirLightPBRParams;
    dirLightPBRParams.Common = commonPBRParams;
    dirLightPBRParams.lightColor = g_PerFrameConsts.m_SceneLightIntensity.xyz;
    dirLightPBRParams.L = g_PerFrameConsts.m_SceneLightDir.xyz;

    float3 finalColor = EvaluateLightPBR(dirLightPBRParams);

    return float4(finalColor, 1.0);
}

#endif

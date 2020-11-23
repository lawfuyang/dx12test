
#include "common.hlsl"
#include "commonvertexformats.hlsl"
#include "pbr.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"
#include "autogen/hlsl/PerInstanceConsts.h"

static const float4x4 g_ViewProjMatrix      = PerFrameConsts::GetViewProjMatrix();
static const float4   g_CameraPosition      = PerFrameConsts::GetCameraPosition();
static const float4   g_SceneLightDir       = PerFrameConsts::GetSceneLightDir();
static const float4   g_SceneLightIntensity = PerFrameConsts::GetSceneLightIntensity();
static const float    g_ConstPBRRoughness   = PerFrameConsts::GetConstPBRRoughness();
static const float    g_ConstPBRMetallic    = PerFrameConsts::GetConstPBRMetallic();

static const Texture2D g_DiffuseTexture = PerInstanceConsts::GetDiffuseTexture();
static const Texture2D g_NormalTexture  = PerInstanceConsts::GetNormalTexture();
static const Texture2D g_ORMTexture     = PerInstanceConsts::GetORMTexture();
static const float4x4  g_WorldMatrix    = PerInstanceConsts::GetWorldMatrix();

struct VS_OUT
{
    float4 m_Position  : SV_POSITION;
    float3 m_Normal    : NORMAL;
    float2 m_TexCoord  : TEXCOORD;
    float3 m_Tangent   : TANGENT;
    float3 m_Bitangent : BINORMAL;

    float3 m_PositionW : POSITIONT;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT result = (VS_OUT)0;

    float4 position;
#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f) || defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    position = float4(input.m_Position, 1);
#else
    position = float4(input.m_Position, 0, 1);
#endif

    result.m_Position = mul(position, g_WorldMatrix);
    result.m_Position = mul(result.m_Position, g_ViewProjMatrix);
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

float4 PSMain(VS_OUT input) : SV_TARGET
{
    // Init per-pixel normal
    float3 localNormal = TwoChannelNormalX2(g_NormalTexture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord).xy);
    float3 normal = PeturbNormal(localNormal, input.m_PositionW.xyz, input.m_Normal, input.m_TexCoord);

    // View vector
    float3 V = normalize(g_CameraPosition.xyz - input.m_PositionW.xyz);

    // Init per-pixel PBR properties
    float ambientOcclusion = 1.0;
    float roughness = g_ConstPBRRoughness;
    float metallic = g_ConstPBRMetallic;
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
    dirLightPBRParams.lightColor = g_SceneLightIntensity.xyz;
    dirLightPBRParams.L = g_SceneLightDir.xyz;

    float3 finalColor = EvaluateLightPBR(dirLightPBRParams);

    return float4(finalColor, 1.0f);
}

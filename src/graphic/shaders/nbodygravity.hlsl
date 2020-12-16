
#include "common.hlsl"

#include "autogen/hlsl/NBodyGravityCSConsts.h"
static const uint  g_MaxParticles         = NBodyGravityCSConsts::GetMaxParticles();
static const uint  g_TileSize             = NBodyGravityCSConsts::GetTileSize();
static const float g_DeltaTime            = NBodyGravityCSConsts::GetDeltaTime();
static const float g_ParticlesDamping     = NBodyGravityCSConsts::GetParticlesDamping();
static const float g_ParticleMass         = NBodyGravityCSConsts::GetParticleMass();
static const float g_ParticleSpread       = NBodyGravityCSConsts::GetParticleSpread();
static const float g_ParticleCenterSpread = NBodyGravityCSConsts::GetParticleCenterSpread();

static RWStructuredBuffer<BodyGravityPosVelo> g_PosVeloOut = NBodyGravityCSConsts::GetPosVelo();

[numthreads(64, 1, 1)]
void CS_FillParticlesBuffer(uint threadID : SV_DispatchThreadID)
{
    RNG::GlobalRNGSeed = Hash(threadID);

    float3 delta;
    delta.x = (RandomFloat() * 2 - 1) * g_ParticleSpread;
    delta.y = (RandomFloat() * 2 - 1) * g_ParticleSpread;
    delta.z = (RandomFloat() * 2 - 1) * g_ParticleSpread;

    g_PosVeloOut[threadID].m_Pos.x = g_ParticleCenterSpread + delta.x;
    g_PosVeloOut[threadID].m_Pos.y = g_ParticleCenterSpread + delta.y;
    g_PosVeloOut[threadID].m_Pos.z = g_ParticleCenterSpread + delta.z;
    g_PosVeloOut[threadID].m_Pos.w = 10000.0f * 10000.0f;

    g_PosVeloOut[threadID].m_Velocity = float4(0, 0, -20.0f, 0.0f);
}

// Body to body interaction, acceleration of the particle at position 
// bi is updated.
void bodyBodyInteraction(inout float3 ai, float4 bj, float4 bi, int particles) 
{
    const float softeningSquared = 0.00125f * 0.00125f;

    float3 r = bj.xyz - bi.xyz;
    float distSqr = dot(r, r);

    distSqr += softeningSquared;

    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube =  invDist * invDist * invDist;
    
    float s = g_ParticleMass * invDistCube * particles;

    ai += r * s;
}

groupshared float4 sharedPos[NBodyGravityCSConsts::BlockSize];

[numthreads(NBodyGravityCSConsts::BlockSize, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    // Each thread of the CS updates one of the particles.
    float4 pos = g_PosVeloOut[DTid.x].m_Pos;
    float4 vel = g_PosVeloOut[DTid.x].m_Velocity;
    float3 accel = 0;

    // Update current particle using all other particles.
    [loop]
    for (uint tile = 0; tile < g_TileSize; tile++)
    {
        // Cache a tile of particles unto shared memory to increase IO efficiency.
        sharedPos[GI] = g_PosVeloOut[tile * NBodyGravityCSConsts::BlockSize + GI].m_Pos;
       
        GroupMemoryBarrierWithGroupSync();

        for (uint i = 0; i < NBodyGravityCSConsts::BlockSize; ++i)
        {
            bodyBodyInteraction(accel, sharedPos[i], pos, 1);
        }
        
        GroupMemoryBarrierWithGroupSync();
    }  

    // g_MaxParticles is the number of our particles, however this number might not 
    // be an exact multiple of the tile size. In such cases, out of bound reads 
    // occur in the process above, which means there will be tooManyParticles 
    // "phantom" particles generating false gravity at position (0, 0, 0), so 
    // we have to subtract them here. NOTE, out of bound reads always return 0 in CS.
    const int tooManyParticles = g_TileSize * NBodyGravityCSConsts::BlockSize - g_MaxParticles;
    bodyBodyInteraction(accel, float4(0, 0, 0, 0), pos, -tooManyParticles);

    // Update the velocity and position of current particle using the acceleration computed above.
    vel.xyz += accel.xyz * g_DeltaTime * g_ParticlesDamping; // update velocity via acceleration + damping
    pos.xyz += vel.xyz * g_DeltaTime; // update pos;

    if (DTid.x < g_MaxParticles)
    {
        g_PosVeloOut[DTid.x].m_Pos = pos;
        g_PosVeloOut[DTid.x].m_Velocity = float4(vel.xyz, length(accel));
    }
}

#include "autogen/hlsl/NBodyGravityVSPSConsts.h"
static const float4x4 g_ViewProjMatrix    = NBodyGravityVSPSConsts::GetViewProjMatrix();
static const float4x4 g_InvViewProjMatrix = NBodyGravityVSPSConsts::GetInvViewProjMatrix();
static const float    g_ParticleRadius    = NBodyGravityVSPSConsts::GetParticleRadius();

static StructuredBuffer<BodyGravityPosVelo> g_PosVeloIn = NBodyGravityVSPSConsts::GetPosVelo();

struct VS_OUT
{
    float4 m_Position : SV_POSITION;
    float2 m_TexCoord : TEXCOORD0;
    float4 m_Color    : COLOR;
};

VS_OUT VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT result = (VS_OUT)0;

    RNG::GlobalRNGSeed = Hash(instanceID);

    static const float3 QuadPositions[4] =
    {
        float3(-1, 1, 0),
        float3(1, 1, 0),
        float3(-1, -1, 0),
        float3(1, -1, 0),
    };

    static const float2 QuadTexCoords[4] =
    {
        float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1),
    };

    // VPOS and UV for current vertex ID [-1, 1]
    float4 pos = float4(QuadPositions[vertexID] * g_ParticleRadius, 1.0f);

    pos = mul(pos, g_InvViewProjMatrix);
    pos.xyz += g_PosVeloIn[instanceID].m_Pos.xyz;
    pos = mul(pos, g_ViewProjMatrix);

    float4 color = float4(RandomFloat(), RandomFloat(), RandomFloat(), 1.0f);

    result.m_Position = pos;
    result.m_TexCoord = QuadTexCoords[vertexID];
    result.m_Color = color;

    return result;
}

struct PS_OUT
{
    float4 m_Color : SV_TARGET;
};

PS_OUT PSMain(VS_OUT input)
{
    PS_OUT result = (PS_OUT)0;

    // Use the texture coordinates to generate a radial gradient representing the particle
    float intensity = 0.5f - length(float2(0.5f, 0.5f) - input.m_TexCoord);
    intensity = clamp(intensity, 0.0f, 0.5f) * 2.0f;

    result.m_Color = float4(input.m_Color.xyz, intensity);

    return result;
}

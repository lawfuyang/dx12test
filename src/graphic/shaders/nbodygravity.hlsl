
#include "common.hlsl"

#include "autogen/hlsl/NBodyGravityCSConsts.h"
static const uint  g_MaxParticles     = NBodyGravityCSConsts::GetMaxParticles();
static const uint  g_TileSize         = NBodyGravityCSConsts::GetTileSize();
static const float g_DeltaTime        = NBodyGravityCSConsts::GetDeltaTime();
static const float g_ParticlesDamping = NBodyGravityCSConsts::GetParticlesDamping();
static const float g_ParticleMass     = NBodyGravityCSConsts::GetParticleMass();

static RWStructuredBuffer<BodyGravityPosVelo> g_PosVelo = NBodyGravityCSConsts::GetPosVelo();

groupshared float4 sharedPos[NBodyGravityCSConsts::BlockSize];

// Body to body interaction, acceleration of the particle at position 
// bi is updated.
void bodyBodyInteraction(inout float3 ai, float4 bj, float4 bi, int particles) 
{
    float3 r = bj.xyz - bi.xyz;

    float distSqr = dot(r, r);

    const float softeningSquared = 0.00125f * 0.00125f;
    distSqr += softeningSquared;

    float invDist = rcp(sqrt(distSqr));
    float invDistCube =  invDist * invDist * invDist;
    
    float s = g_ParticleMass * invDistCube * particles;

    ai += r * s;
}

[numthreads(NBodyGravityCSConsts::BlockSize, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    // Each thread of the CS updates one of the particles.
    float4 pos = g_PosVelo[DTid.x].m_Pos;
    float4 vel = g_PosVelo[DTid.x].m_Velocity;
    float3 accel = 0;
    float mass = g_ParticleMass;

    // Update current particle using all other particles.
    [loop]
    for (uint tile = 0; tile < g_TileSize; tile++)
    {
        // Cache a tile of particles unto shared memory to increase IO efficiency.
        sharedPos[GI] = g_PosVelo[tile * NBodyGravityCSConsts::BlockSize + GI].m_Pos;
       
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

    // Update the velocity and position of current particle using the 
    // acceleration computed above.
    vel.xyz += accel.xyz * g_DeltaTime;   //deltaTime;
    vel.xyz *= g_ParticlesDamping;        //damping;
    pos.xyz += vel.xyz * g_DeltaTime;     //deltaTime;

    if (DTid.x < g_MaxParticles)
    {
        g_PosVelo[DTid.x].m_Pos = pos;
        g_PosVelo[DTid.x].m_Velocity = float4(vel.xyz, length(accel));
    }
}

#include "autogen/hlsl/NBodyGravityVSPSConsts.h"
static const float g_ParticleRadius = NBodyGravityVSPSConsts::GetParticleRadius();

struct VS_IN
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 m_Position  : SV_POSITION;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT result = (VS_OUT)0;
    return result;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return 0;
}
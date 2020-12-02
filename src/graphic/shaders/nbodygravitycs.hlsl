
#include "common.hlsl"

#include "autogen/hlsl/NBodyGravityConsts.h"

static const uint g_MaxParticles      = NBodyGravityConsts::GetMaxParticles();
static const uint g_TileSize          = NBodyGravityConsts::GetTileSize();
static const float g_DeltaTime        = NBodyGravityConsts::GetDeltaTime();
static const float g_ParticlesDamping = NBodyGravityConsts::GetParticlesDamping();

static StructuredBuffer<BodyGravityPosVelo> g_OldPosVelo   = NBodyGravityConsts::GetOldPosVelo();
static RWStructuredBuffer<BodyGravityPosVelo> g_NewPosVelo = NBodyGravityConsts::GetNewPosVelo();

static const float softeningSquared    = 0.0012500000f * 0.0012500000f;
static const float g_fG                = 6.67300e-11f * 10000.0f;
static const float g_fParticleMass    = g_fG * 10000.0f * 10000.0f;

#define blocksize 128
groupshared float4 sharedPos[blocksize];

// Body to body interaction, acceleration of the particle at position 
// bi is updated.
void bodyBodyInteraction(inout float3 ai, float4 bj, float4 bi, float mass, int particles) 
{
    float3 r = bj.xyz - bi.xyz;

    float distSqr = dot(r, r);
    distSqr += softeningSquared;

    float invDist = 1.0f / sqrt(distSqr);
    float invDistCube =  invDist * invDist * invDist;
    
    float s = mass * invDistCube * particles;

    ai += r * s;
}

[numthreads(blocksize, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    // Each thread of the CS updates one of the particles.
    float4 pos = g_OldPosVelo[DTid.x].m_Pos;
    float4 vel = g_OldPosVelo[DTid.x].m_Velocity;
    float3 accel = 0;
    float mass = g_fParticleMass;

    // Update current particle using all other particles.
    [loop]
    for (uint tile = 0; tile < g_TileSize; tile++)
    {
        // Cache a tile of particles unto shared memory to increase IO efficiency.
        sharedPos[GI] = g_OldPosVelo[tile * blocksize + GI].m_Pos;
       
        GroupMemoryBarrierWithGroupSync();

        for (uint counter = 0; counter < blocksize; counter += 8 ) 
        {
            bodyBodyInteraction(accel, sharedPos[counter], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+1], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+2], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+3], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+4], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+5], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+6], pos, mass, 1);
            bodyBodyInteraction(accel, sharedPos[counter+7], pos, mass, 1);
        }
        
        GroupMemoryBarrierWithGroupSync();
    }  

    // g_MaxParticles is the number of our particles, however this number might not 
    // be an exact multiple of the tile size. In such cases, out of bound reads 
    // occur in the process above, which means there will be tooManyParticles 
    // "phantom" particles generating false gravity at position (0, 0, 0), so 
    // we have to subtract them here. NOTE, out of bound reads always return 0 in CS.
    const int tooManyParticles = g_TileSize * blocksize - g_MaxParticles;
    bodyBodyInteraction(accel, float4(0, 0, 0, 0), pos, mass, -tooManyParticles);

    // Update the velocity and position of current particle using the 
    // acceleration computed above.
    vel.xyz += accel.xyz * g_DeltaTime;   //deltaTime;
    vel.xyz *= g_ParticlesDamping;        //damping;
    pos.xyz += vel.xyz * g_DeltaTime;     //deltaTime;

    if (DTid.x < g_MaxParticles)
    {
        g_NewPosVelo[DTid.x].m_Pos = pos;
        g_NewPosVelo[DTid.x].m_Velocity = float4(vel.xyz, length(accel));
    }
}

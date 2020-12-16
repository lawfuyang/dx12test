#ifndef __UTILS_HLSL__
#define __UTILS_HLSL__

// full screen helpers
void GetFullScreenTriangleVposAndUVFromVertexID(const uint vertexID, out float2 vpos, out float2 uv)
{
    uint2 v = uint2(vertexID & 1, vertexID >> 1); // ccw

    vpos.x = -1.f + v.x * 4.f;
    vpos.y = -1.f + v.y * 4.f;

    uv.x = v.x * 2.f;
    uv.y = 1.f - v.y * 2.f;
}
void GetFullScreenVposAndUVFromVertexID(const uint vertexID, out float2 vpos, out float2 uv)
{
    // based on triangle strip
    vpos = float2(vertexID & 1, vertexID >> 1);
    uv = (vpos * float2(0.5f, -0.5f) + 0.5f);
}

// Sample normal map, convert to signed, apply tangent-to-world space transform.
float3 CalcPerPixelNormal(in Texture2D<float4> normalTexture, float2 vTexcoord, float3 vVertNormal, float3 vVertBinormal, float3 vVertTangent)
{
    float3x3 mTangentSpaceToWorldSpace = float3x3(vVertTangent, vVertBinormal, vVertNormal);

    // Compute per-pixel normal.
    float3 vBumpNormal = (float3)normalTexture.Sample(g_AnisotropicWrapSampler, vTexcoord);
    vBumpNormal = 2.0f * vBumpNormal - 1.0f;

    return mul(vBumpNormal, mTangentSpaceToWorldSpace);
}

// Christian Schuler, "Normal Mapping without Precomputed Tangents", ShaderX 5, Chapter 2.6, pp. 131-140
// See also follow-up blog post: http://www.thetenthplanet.de/archives/1180
float3x3 CalculateTBN(float3 p, float3 n, float2 tex)
{
    float3 dp1 = ddx(p);
    float3 dp2 = ddy(p);
    float2 duv1 = ddx(tex);
    float2 duv2 = ddy(tex);

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    float3 t = normalize(mul(float2(duv1.x, duv2.x), inverseM));
    float3 b = normalize(mul(float2(duv1.y, duv2.y), inverseM));
    return float3x3(t, b, n);
}

float3 PeturbNormal(float3 localNormal, float3 position, float3 normal, float2 texCoord)
{
    const float3x3 TBN = CalculateTBN(position, normal, texCoord);
    return normalize(mul(localNormal, TBN));
}

float3 TwoChannelNormalX2(float2 normal)
{
    float2 xy = 2.0f * normal - 1.0f;
    float z = sqrt(1 - dot(xy, xy));
    return float3(xy.x, xy.y, z);
}

// sRGB 
// https://en.wikipedia.org/wiki/SRGB

// Apply the (approximate) sRGB curve to linear values
float3 LinearToSRGBEst(float3 color)
{
    return pow(abs(color), 1 / 2.2f);
}

// (Approximate) sRGB to linear
float3 SRGBToLinearEst(float3 srgb)
{
    return pow(abs(srgb), 2.2f);
}

// HDR10 Media Profile
// https://en.wikipedia.org/wiki/High-dynamic-range_video#HDR10

// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
static const float3x3 from709to2020 =
{
    { 0.6274040f, 0.3292820f, 0.0433136f },
    { 0.0690970f, 0.9195400f, 0.0113612f },
    { 0.0163916f, 0.0880132f, 0.8955950f }
};


// Apply the ST.2084 curve to normalized linear values and outputs normalized non-linear values
float3 LinearToST2084(float3 normalizedLinearValue)
{
    return pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), 0.1593017578f)), 78.84375f);
}

// ST.2084 to linear, resulting in a linear normalized value
float3 ST2084ToLinear(float3 ST2084)
{
    return pow(max(pow(abs(ST2084), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f * pow(abs(ST2084), 1.0f / 78.84375f)), 1.0f / 0.1593017578f);
}

// Reinhard tonemap operator
// Reinhard et al. "Photographic tone reproduction for digital images." ACM Transactions on Graphics. 21. 2002.
// http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 ToneMapReinhard(float3 color)
{
    return color / (1.0f + color);
}

// ACES Filmic tonemap operator
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ToneMapACESFilmic(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Ref: http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
namespace RNG
{
    static uint GlobalRNGSeed = 0;

    // Create an initial random number for this thread
    uint SeedThread(uint seed)
    {
        // Thomas Wang hash 
        // Ref: http://www.burtleburtle.net/bob/hash/integer.html
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    // Generate a random 32-bit integer
    uint Random(inout uint state)
    {
        // Xorshift algorithm from George Marsaglia's paper.
        state ^= (state << 13);
        state ^= (state >> 17);
        state ^= (state << 5);
        return state;
    }

    // Generate a random float in the range [0.0f, 1.0f)
    float Random01(inout uint state)
    {
        return asfloat(0x3f800000 | Random(state) >> 9) - 1.0;
    }

    // Generate a random float in the range [0.0f, 1.0f]
    float Random01inclusive(inout uint state)
    {
        return Random(state) / float(0xffffffff);
    }


    // Generate a random integer in the range [lower, upper]
    uint Random(inout uint state, uint lower, uint upper)
    {
        return lower + uint(float(upper - lower + 1) * Random01(state));
    }
}

// Generate a random 32-bit integer
uint RandomUint()
{
    RNG::GlobalRNGSeed ^= (RNG::GlobalRNGSeed << 13);
    RNG::GlobalRNGSeed ^= (RNG::GlobalRNGSeed >> 17);
    RNG::GlobalRNGSeed ^= (RNG::GlobalRNGSeed << 5);
    return RNG::GlobalRNGSeed;
}

// Generate a random float in the range [0.0f, 1.0f]
float RandomFloat()
{
    return RandomUint() / float(0xffffffff);
}

// 3D value noise
// Ref: https://www.shadertoy.com/view/XsXfRH
float Hash(float3 p)
{
    p = frac(p * 0.3183099 + .1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Thomas Wang hash 
// Ref: http://www.burtleburtle.net/bob/hash/integer.html
uint Hash(uint seed)
{
    return RNG::SeedThread(seed);
}

#endif // #define __UTILS_HLSL__

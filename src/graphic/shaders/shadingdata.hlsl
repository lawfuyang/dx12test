
#ifndef __SHADINGDATA_HLSL__
#define __SHADINGDATA_HLSL__

/** This struct describes the geometric data for a specific hit point used for lighting calculations
*/
struct ShadingData
{
    float3  posW;                   ///< Shading hit position in world space
    float3  V;                      ///< Direction to the eye at shading hit
    float3  N;                      ///< Shading normal at shading hit
    float3  T;                      ///< Shading tangent at shading hit
    float3  B;                      ///< Shading bitangent at shading hit
    float2  uv;                     ///< Texture mapping coordinates
    float   NdotV;                  // Unclamped, can be negative.

    // Primitive data
    float3  faceN;                  ///< Face normal in world space, always on the front-facing side.
    bool    frontFacing;            ///< True if primitive seen from the front-facing side.
    bool    doubleSided;            ///< Material double-sided flag, if false only shade front face.

    // Pre-loaded texture data
    uint    materialID;             ///< Material ID at shading location.
    float3  diffuse;                ///< Diffuse albedo.
    float   opacity;
    float3  specular;               ///< Specular albedo.
    float   linearRoughness;        ///< This is the original roughness, before re-mapping.
    float   ggxAlpha;               ///< This is the re-mapped roughness value, which should be used for GGX computations. It equals `roughness^2`
    float3  emissive;
    float4  occlusion;
    float   IoR;                    ///< Index of refraction of the medium "behind" the surface.
    float   metallic;               ///< Metallic parameter, blends between dielectric and conducting BSDFs.
    float   specularTransmission;   ///< Specular transmission, blends between opaque dielectric BRDF and specular transmissive BSDF.
    float   eta;                    ///< Relative index of refraction (incident IoR / transmissive IoR).
};

#endif // #define __SHADINGDATA_HLSL__

#pragma once

#include <graphic/gfxtexturesandbuffers.h>
#include <graphic/gfxmesh.h>

class GfxContext;

class GfxDefaultAssets
{
public:
    DeclareSingletonFunctions(GfxDefaultAssets);

    void PreInitialize(tf::Subflow& sf);
    void Initialize(tf::Subflow& sf);
    void ShutDown();

    static void DrawSquidRoom(GfxContext& context);

    inline static GfxTexture White2D;
    inline static GfxTexture Black2D;
    inline static GfxTexture Checkerboard;

    inline static GfxMesh UnitCube;
    inline static GfxMesh Occcity;

private:
    void PreInitOcccity();
    void PreInitSquidRoom();

    void CreateCheckerboardTexture();
    void CreateSolidColorTexture(GfxTexture&, const bbeColor&, const char* colorName);
    void CreateUnitCubeMesh();
    void CreateOcccityMesh();
    void CreateSquidRoomMesh();
    void CreateSquidRoomTextures(tf::Subflow&);
    void ClearPreloadedSampleAssetsMemory();

    std::vector<std::byte> m_OcccityData;
    std::vector<std::byte> m_SquidRoomData;

    GfxMesh m_SquidRoomMesh;
    std::vector<GfxTexture> m_SquidRoomTextures;
};
#define g_GfxDefaultAssets GfxDefaultAssets::GetInstance()

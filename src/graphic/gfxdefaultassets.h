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

    inline static GfxTexture White2D;
    inline static GfxTexture Black2D;
    inline static GfxTexture Checkerboard;

    inline static GfxMesh UnitCube;
    inline static GfxMesh Occcity;
    inline static GfxMesh SquidRoom;

private:
    void PreInitOcccity();
    void PreInitSquidRoom();

    void InitCheckerboardTexture();
    void InitSolidColor(GfxTexture&, const bbeColor&, const char* colorName);
    void CreateUnitCube();
    void CreateOcccity();
    void CreateSquidRoom();

    std::vector<std::byte> m_OcccityData;
    std::vector<std::byte> m_SquidRoomData;
};
#define g_GfxDefaultAssets GfxDefaultAssets::GetInstance()

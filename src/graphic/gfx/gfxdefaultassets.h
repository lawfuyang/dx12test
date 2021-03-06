#pragma once

#include <graphic/gfx/gfxtexturesandbuffers.h>
#include <graphic/gfx/gfxmesh.h>

class GfxContext;

class GfxDefaultAssets
{
public:
    DeclareSingletonFunctions(GfxDefaultAssets);

    void Initialize(tf::Subflow& sf);

    inline static GfxTexture White2D;
    inline static GfxTexture Black2D;
    inline static GfxTexture Red2D;
    inline static GfxTexture Yellow2D;
    inline static GfxTexture FlatNormal;
    inline static GfxTexture Checkerboard;

    inline static GfxMesh UnitCube;

private:
    void CreateCheckerboardTexture();
    void CreateSolidColorTexture(GfxTexture&, const bbeColor&, const char* colorName);
    void CreateUnitCubeMesh();

    std::vector<std::byte> m_SquidRoomData;
};
#define g_GfxDefaultAssets GfxDefaultAssets::GetInstance()

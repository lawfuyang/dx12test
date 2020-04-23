#pragma once

#include <graphic/gfxtexturesandbuffers.h>
#include <graphic/gfxmesh.h>

class GfxDefaultAssets
{
public:
    DeclareSingletonFunctions(GfxDefaultAssets);

    void Initialize(tf::Subflow& sf);
    void ShutDown();

    inline static GfxTexture White2D;
    inline static GfxTexture Black2D;
    inline static GfxTexture Checkerboard;

    inline static GfxMesh UnitCube;

private:
    void InitCheckerboardTexture();
    void InitSolidColor(GfxTexture&, const bbeColor&, const char* colorName);
    void CreateUnitCube();
};
#define g_GfxDefaultAssets GfxDefaultAssets::GetInstance()

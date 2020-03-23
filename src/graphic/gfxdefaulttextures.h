#pragma once

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxDefaultTextures
{
public:
    DeclareSingletonFunctions(GfxDefaultTextures);

    void Initialize();
    void ShutDown();

    inline static GfxTexture White2D;
    inline static GfxTexture Black2D;
    inline static GfxTexture Checkerboard;

private:
    void InitCheckerboardTexture(GfxTexture&);
    void InitSolidColor(GfxTexture&, const bbeColor&, const char* colorName);
};
#define g_GfxDefaultTextures GfxDefaultTextures::GetInstance()

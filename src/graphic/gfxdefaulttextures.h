#pragma once

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxDefaultTextures
{
public:
    DeclareSingletonFunctions(GfxDefaultTextures);

    void Initialize(GfxContext& initContext);
    void ShutDown();

    inline static GfxTexture White;
    inline static GfxTexture Black;
    inline static GfxTexture Checkerboard;

private:
    void InitCheckerboardTexture(GfxContext& initContext);
};
#define g_GfxDefaultTextures GfxDefaultTextures::GetInstance()

#pragma once

#include "tmp/shaders/shadersenumsautogen.h"

class GfxShaderManager
{
public:
    DeclareSingletonFunctions(GfxShaderManager);

    void Initialize();

private:
    void* m_AllShaders[g_NumShaders];
};

#pragma once

class GfxAssetLoader
{
public:
    DeclareSingletonFunctions(GfxAssetLoader);

private:
};
#define g_GfxAssetLoader GfxAssetLoader::GetInstance()

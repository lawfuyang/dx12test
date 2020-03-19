#include "graphic/gfxdefaulttextures.h"

#include "graphic/dx12utils.h"

void GfxDefaultTextures::Initialize(GfxContext& initContext)
{
    bbeProfileFunction();

    InitCheckerboardTexture(initContext);
}

void GfxDefaultTextures::ShutDown()
{
    this->White.Release();
    this->Black.Release();
    this->Checkerboard.Release();
}

void GfxDefaultTextures::InitCheckerboardTexture(GfxContext& initContext)
{
    bbeProfileFunction();

    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) / 8;    // The number of bytes used to represent a pixel in the texture.

    const uint32_t rowPitch = TextureWidth * TexturePixelSize;
    const uint32_t cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const uint32_t cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const uint32_t textureSize = rowPitch * TextureHeight;

    std::vector<uint8_t> dataVec;
    dataVec.resize(textureSize);
    uint8_t* data = dataVec.data();

    for (uint32_t n = 0; n < textureSize; n += TexturePixelSize)
    {
        uint32_t x = n % rowPitch;
        uint32_t y = n / rowPitch;
        uint32_t i = x / cellPitch;
        uint32_t j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            data[n] = 0x00;        // R
            data[n + 1] = 0x00;    // G
            data[n + 2] = 0x00;    // B
            data[n + 3] = 0xff;    // A
        }
        else
        {
            data[n] = 0xff;        // R
            data[n + 1] = 0xff;    // G
            data[n + 2] = 0xff;    // B
            data[n + 3] = 0xff;    // A
        }
    }

    this->Checkerboard.Initialize(initContext, DXGI_FORMAT_R8G8B8A8_UNORM, TextureWidth, TextureHeight, D3D12_RESOURCE_FLAG_NONE, data, "Default Checkboard Texture");
}

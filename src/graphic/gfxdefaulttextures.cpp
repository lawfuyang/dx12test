#include "graphic/gfxdefaulttextures.h"

#include "graphic/dx12utils.h"

void GfxDefaultTextures::Initialize()
{
    bbeProfileFunction();

    tf::Taskflow tf;

    tf.emplace([&]() { InitCheckerboardTexture(Checkerboard); });
    tf.emplace([&]() { InitSolidColor(White2D, bbeColor{ 1.0f, 1.0f, 1.0f }, "Default White2D"); });
    tf.emplace([&]() { InitSolidColor(Black2D, bbeColor{ 0.0f, 0.0f, 0.0f }, "Default Black2D"); });

    g_TasksExecutor.run(tf).wait();
}

void GfxDefaultTextures::ShutDown()
{
    this->White2D.Release();
    this->Black2D.Release();
    this->Checkerboard.Release();
}

void GfxDefaultTextures::InitCheckerboardTexture(GfxTexture& result)
{
    bbeProfileFunction();

    static const uint32_t TextureWidth = 256;
    static const uint32_t TextureHeight = 256;
    static const uint32_t TexturePixelSize = GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) / 8;    // The number of bytes used to represent a pixel in the texture.

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

    result.Initialize(DXGI_FORMAT_R8G8B8A8_UNORM, TextureWidth, TextureHeight, D3D12_RESOURCE_FLAG_NONE, data, "Default Checkboard Texture");
}

void GfxDefaultTextures::InitSolidColor(GfxTexture& result, const bbeColor& color, const char* colorName)
{
    bbeProfileFunction();

    static const uint32_t TextureWidth = 16;
    static const uint32_t TextureHeight = 16;
    static const uint32_t TexturePixelSize = GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) / 8;    // The number of bytes used to represent a pixel in the texture.

    const uint32_t textureSize = TextureWidth * TextureWidth * TexturePixelSize;

    std::vector<uint8_t> dataVec;
    dataVec.resize(textureSize);

    SIMDMemFill(dataVec.data(), color.ToVector4(), DivideByMultiple(textureSize, 16));

    result.Initialize(DXGI_FORMAT_R8G8B8A8_UNORM, TextureWidth, TextureHeight, D3D12_RESOURCE_FLAG_NONE, dataVec.data(), colorName);
}

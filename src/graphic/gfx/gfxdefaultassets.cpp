#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/pch.h>

#include <tmp/shaders/autogen/cpp/ShaderPermutations/ForwardLighting.h>

void GfxDefaultAssets::Initialize(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    subFlow.emplace([this] { CreateCheckerboardTexture(); });
    subFlow.emplace([this] { CreateSolidColorTexture(White2D, bbeColor{ 1.0f, 1.0f, 1.0f }, "Default White2D Texture"); });
    subFlow.emplace([this] { CreateSolidColorTexture(Black2D, bbeColor{ 0.0f, 0.0f, 0.0f }, "Default Black2D Texture"); });
    subFlow.emplace([this] { CreateSolidColorTexture(Red2D, bbeColor{ 1.0f, 0.0f, 0.0f }, "Default Red2D Texture"); });
    subFlow.emplace([this] { CreateSolidColorTexture(Yellow2D, bbeColor{ 1.0f, 1.0f, 0.0f }, "Default Yellow2D Texture"); });
    subFlow.emplace([this] { CreateSolidColorTexture(FlatNormal, bbeColor{ 0.5f, 0.5f, 0.0f }, "Flat Normal Texture"); });
    subFlow.emplace([this] { CreateUnitCubeMesh(); });
}

void GfxDefaultAssets::CreateCheckerboardTexture()
{
    bbeProfileFunction();

    const uint32_t TextureWidth = 256;
    const uint32_t TextureHeight = 256;
    const uint32_t TexturePixelSize = GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) / 8;    // The number of bytes used to represent a pixel in the texture.

    const uint32_t rowPitch = TextureWidth * TexturePixelSize;
    const uint32_t cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const uint32_t cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const uint32_t textureSize = rowPitch * TextureHeight;

    std::vector<uint8_t> data;
    data.resize(textureSize);

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

    GfxDefaultAssets::Checkerboard.InitializeTexture(CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, TextureWidth, TextureHeight), data.data());
    GfxDefaultAssets::Checkerboard.SetDebugName("Checkerboard");
}

void GfxDefaultAssets::CreateSolidColorTexture(GfxTexture& result, const bbeColor& color, const char* colorName)
{
    bbeProfileFunction();

    static const uint32_t TextureWidth = 1;
    static const uint32_t TextureHeight = 1;

    InplaceArray<uint32_t, TextureWidth* TextureWidth> data;
    data.resize(TextureWidth * TextureWidth);

    std::fill(data.begin(), data.end(), color.RGBA().v);

    result.InitializeTexture(CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, TextureWidth, TextureHeight), data.data());
    result.SetDebugName(StringFormat("SolidColorTexture: %s", colorName));
}

template <typename Vertex>
static void ReverseWinding(std::vector<uint16_t>& indices, std::vector<Vertex>& vertices)
{
    assert((indices.size() % 3) == 0);
    for (auto it = indices.begin(); it != indices.end(); it += 3)
    {
        std::swap(*it, *(it + 2));
    }

    for (Vertex& v : vertices)
    {
        v.m_TexCoord.x = (1.0f - v.m_TexCoord.x);
    }
}

void GfxDefaultAssets::CreateUnitCubeMesh()
{
    bbeProfileFunction();

    // A box has six faces, each one pointing in a different direction.
    const bbeVector3 faceNormals[] =
    {
        {  0,  0,  1 },
        {  0,  0, -1 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
    };

    const bbeVector3 textureCoordinates[] =
    {
        { 1, 0, 0 },
        { 1, 1, 0 },
        { 0, 1, 0 },
        { 0, 0, 0 },
    };

    // Position3f_Normal3f_Texcoord2f
    struct Vertex
    {
        bbeVector3 m_Position;
        bbeVector3 m_Normal;
        bbeVector2 m_TexCoord;
    };

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    // Create each face in turn.
    for (uint32_t i = 0; i < _countof(faceNormals); ++i)
    {
        // Get two vectors perpendicular both to the face normal and to each other.
        const bbeVector3 basis = (i >= 4) ? bbeVector3::UnitZ : bbeVector3::UnitY;

        const bbeVector3 side1 = faceNormals[i].Cross(basis);
        const bbeVector3 side2 = faceNormals[i].Cross(side1);

        // Six indices (two triangles) per face.
        const size_t vbase = vertices.size();
        indices.push_back(vbase + 0);
        indices.push_back(vbase + 1);
        indices.push_back(vbase + 2);

        indices.push_back(vbase + 0);
        indices.push_back(vbase + 2);
        indices.push_back(vbase + 3);

        // Four vertices per face.
        // (faceNormals[i] - side1 - side2) * 0.5f // faceNormals[i] // t0
        vertices.push_back(Vertex{ (faceNormals[i] - side1 - side2) * 0.5f, faceNormals[i], textureCoordinates[0] });

        // (faceNormals[i] - side1 + side2) * 0.5f // faceNormals[i] // t1
        vertices.push_back(Vertex{ (faceNormals[i] - side1 + side2) * 0.5f, faceNormals[i], textureCoordinates[1] });

        // (faceNormals[i] + side1 + side2) * 0.5f // faceNormals[i] // t2
        vertices.push_back(Vertex{ (faceNormals[i] + side1 + side2) * 0.5f, faceNormals[i], textureCoordinates[2] });

        // (faceNormals[i] + side1 - side2) * 0.5f // faceNormals[i] // t3
        vertices.push_back(Vertex{ (faceNormals[i] + side1 - side2) * 0.5f, faceNormals[i], textureCoordinates[3] });
    }

    ReverseWinding(indices, vertices);

    GfxDefaultAssets::UnitCube.m_VertexFormat = &GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f;
    GfxDefaultAssets::UnitCube.m_IndexBuffer.Initialize(indices.size(), indices.data());
    GfxDefaultAssets::UnitCube.m_VertexBuffer.Initialize(vertices.size(), sizeof(Vertex), vertices.data());

    GfxDefaultAssets::UnitCube.m_IndexBuffer.SetDebugName("UnitCube");
    GfxDefaultAssets::UnitCube.m_VertexBuffer.SetDebugName("UnitCube");
}

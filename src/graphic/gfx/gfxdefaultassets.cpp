#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/pch.h>

#include <tmp/shaders/autogen/cpp/ShaderPermutations/ForwardLighting.h>

void GfxDefaultAssets::Initialize(tf::Subflow& sf)
{
    bbeProfileFunction();

    ADD_TF_TASK(sf, CreateCheckerboardTexture());
    ADD_TF_TASK(sf, CreateSolidColorTexture(White2D, bbeColor{ 1.0f, 1.0f, 1.0f }, "Default White2D Texture"));
    ADD_TF_TASK(sf, CreateSolidColorTexture(Black2D, bbeColor{ 0.0f, 0.0f, 0.0f }, "Default Black2D Texture"));
    ADD_TF_TASK(sf, CreateSolidColorTexture(Red2D, bbeColor{ 1.0f, 0.0f, 0.0f }, "Default Red2D Texture"));
    ADD_TF_TASK(sf, CreateSolidColorTexture(Yellow2D, bbeColor{ 1.0f, 1.0f, 0.0f }, "Default Yellow2D Texture"));
    ADD_TF_TASK(sf, CreateSolidColorTexture(FlatNormal, bbeColor{ 0.5f, 0.5f, 0.0f }, "Flat Normal Texture"));
    ADD_TF_TASK(sf, CreateUnitCubeMesh());
}

void GfxDefaultAssets::ShutDown()
{
    GfxDefaultAssets::White2D.Release();
    GfxDefaultAssets::Black2D.Release();
    GfxDefaultAssets::Red2D.Release();
    GfxDefaultAssets::Yellow2D.Release();
    GfxDefaultAssets::FlatNormal.Release();
    GfxDefaultAssets::Checkerboard.Release();
    GfxDefaultAssets::UnitCube.Release();
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

    GfxTexture::InitParams initParams;
    initParams.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    initParams.m_Width = TextureWidth;
    initParams.m_Height = TextureHeight;
    initParams.m_InitData = data.data();
    initParams.m_ResourceName = "Default Checkboard Texture";

    GfxDefaultAssets::Checkerboard.Initialize(initParams);
}

void GfxDefaultAssets::CreateSolidColorTexture(GfxTexture& result, const bbeColor& color, const char* colorName)
{
    bbeProfileFunction();

    static const uint32_t TextureWidth = 1;
    static const uint32_t TextureHeight = 1;

    InplaceArray<uint32_t, TextureWidth* TextureWidth> data;
    data.resize(TextureWidth * TextureWidth);

    std::fill(data.begin(), data.end(), color.RGBA().v);

    GfxTexture::InitParams initParams;
    initParams.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    initParams.m_Width = TextureWidth;
    initParams.m_Height = TextureHeight;
    initParams.m_InitData = data.data();
    initParams.m_ResourceName = colorName;

    result.Initialize(initParams);
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

    GfxMesh::InitParams meshInitParams;
    meshInitParams.MeshName = "UnitCube Mesh";
    meshInitParams.m_VertexFormat = &GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f;

    GfxVertexBuffer::InitParams& VBInitParams = meshInitParams.m_VBInitParams;
    VBInitParams.m_InitData = vertices.data();
    VBInitParams.m_NumVertices = vertices.size();
    VBInitParams.m_VertexSize = sizeof(Vertex);
    VBInitParams.m_ResourceName = "GfxDefaultGeometry::UnitCube Vertex Buffer";

    GfxIndexBuffer::InitParams& IBInitParams = meshInitParams.m_IBInitParams;
    IBInitParams.m_InitData = indices.data();
    IBInitParams.m_NumIndices = indices.size();
    IBInitParams.m_IndexSize = sizeof(uint16_t);
    IBInitParams.m_ResourceName = "GfxDefaultGeometry::UnitCube Index Buffer";

    GfxDefaultAssets::UnitCube.Initialize(meshInitParams);
}

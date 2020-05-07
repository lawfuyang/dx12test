#include <graphic/gfxdefaultassets.h>

#include <graphic/dx12utils.h>
#include <graphic/gfxdevice.h>
#include <graphic/gfxmanager.h>

#include <defaultassets/occcity.h>
#include <defaultassets/SquidRoom.h>

void GfxDefaultAssets::PreInitialize(tf::Subflow& sf)
{
    bbeProfileFunction();

    ADD_TF_TASK(sf, PreInitOcccity());
    ADD_TF_TASK(sf, PreInitSquidRoom());
}

void GfxDefaultAssets::Initialize(tf::Subflow& sf)
{
    bbeProfileFunction();

    InplaceArray<tf::Task, 7> tasks;
    tasks.push_back(ADD_TF_TASK(sf, CreateCheckerboardTexture()));
    tasks.push_back(ADD_TF_TASK(sf, CreateSolidColorTexture(White2D, bbeColor{ 1.0f, 1.0f, 1.0f }, "Default White2D Texture")));
    tasks.push_back(ADD_TF_TASK(sf, CreateSolidColorTexture(Black2D, bbeColor{ 0.0f, 0.0f, 0.0f }, "Default Black2D Texture")));
    tasks.push_back(ADD_TF_TASK(sf, CreateUnitCubeMesh()));
    tasks.push_back(ADD_TF_TASK(sf, CreateOcccityMesh()));
    tasks.push_back(ADD_TF_TASK(sf, CreateSquidRoomMesh()));
    tasks.push_back(ADD_SF_TASK(sf, CreateSquidRoomTextures(sf)));

    tf::Task clearPreloadedSampleAssetsMemoryTask = ADD_TF_TASK(sf, ClearPreloadedSampleAssetsMemory());
    for (tf::Task& t : tasks)
    {
        clearPreloadedSampleAssetsMemoryTask.succeed(t);
    }
}

void GfxDefaultAssets::ShutDown()
{
    this->White2D.Release();
    this->Black2D.Release();
    this->Checkerboard.Release();

    for (GfxTexture& tex : this->SquidRoomTextures)
    {
        tex.Release();
    }

    this->UnitCube.Release();
    this->Occcity.Release();
    this->SquidRoom.Release();
}

void GfxDefaultAssets::PreInitOcccity()
{
    bbeProfileFunction();

    using namespace SampleAssets;

    StaticString<128> dataFileName = "..\\bin\\assets\\";
    dataFileName += Occcity::DataFileName;

    ReadDataFromFile(dataFileName.c_str(), m_OcccityData);
}

void GfxDefaultAssets::PreInitSquidRoom()
{
    bbeProfileFunction();

    using namespace SampleAssets;

    StaticString<128> dataFileName = "..\\bin\\assets\\";
    dataFileName += SquidRoom::DataFileName;

    ReadDataFromFile(dataFileName.c_str(), m_SquidRoomData);
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

    this->Checkerboard.Initialize(initParams);
}

void GfxDefaultAssets::CreateSolidColorTexture(GfxTexture& result, const bbeColor& color, const char* colorName)
{
    bbeProfileFunction();

    static const uint32_t TextureWidth = 16;
    static const uint32_t TextureHeight = 16;
    static const uint32_t TexturePixelSize = GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) / 8;    // The number of bytes used to represent a pixel in the texture.

    const uint32_t textureSize = TextureWidth * TextureWidth * TexturePixelSize;

    std::vector<uint8_t> dataVec;
    dataVec.resize(textureSize);

    SIMDMemFill(dataVec.data(), color.ToVector4(), DivideByMultiple(textureSize, 16));

    GfxTexture::InitParams initParams;
    initParams.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    initParams.m_Width = TextureWidth;
    initParams.m_Height = TextureHeight;
    initParams.m_InitData = dataVec.data();
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

    this->UnitCube.Initialize(meshInitParams);
}

void GfxDefaultAssets::CreateOcccityMesh()
{
    bbeProfileFunction();

    assert(m_OcccityData.size());

    using namespace SampleAssets;

    GfxMesh::InitParams meshInitParams;
    meshInitParams.MeshName = "Occcity Mesh";
    meshInitParams.m_VertexFormat = &GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f_Tangent3f;

    GfxVertexBuffer::InitParams& VBInitParams = meshInitParams.m_VBInitParams;
    VBInitParams.m_ResourceName = "Occcity Mesh Vertex Buffer";
    VBInitParams.m_InitData = m_OcccityData.data() + Occcity::VertexDataOffset;
    VBInitParams.m_NumVertices = Occcity::VertexDataSize / Occcity::StandardVertexStride;
    VBInitParams.m_VertexSize = Occcity::StandardVertexStride;

    GfxIndexBuffer::InitParams& IBInitParams = meshInitParams.m_IBInitParams;
    IBInitParams.m_ResourceName = "Occcity Mesh Index Buffer";
    IBInitParams.m_InitData = m_OcccityData.data() + Occcity::IndexDataOffset;
    IBInitParams.m_NumIndices = Occcity::IndexDataSize / 4; // R32_UINT (SampleAssets::StandardIndexFormat) = 4 bytes each.
    IBInitParams.m_IndexSize = 4;

    this->Occcity.Initialize(meshInitParams);
}

void GfxDefaultAssets::CreateSquidRoomMesh()
{
    bbeProfileFunction();

    assert(m_SquidRoomData.size());

    using namespace SampleAssets;

    GfxMesh::InitParams meshInitParams;
    meshInitParams.MeshName = "SquidRoom Mesh";
    meshInitParams.m_VertexFormat = &GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f_Tangent3f;

    GfxVertexBuffer::InitParams& VBInitParams = meshInitParams.m_VBInitParams;
    VBInitParams.m_ResourceName = "SquidRoom Mesh Vertex Buffer";
    VBInitParams.m_InitData = m_SquidRoomData.data() + SquidRoom::VertexDataOffset;
    VBInitParams.m_NumVertices = SquidRoom::VertexDataSize / SquidRoom::StandardVertexStride;
    VBInitParams.m_VertexSize = SquidRoom::StandardVertexStride;

    GfxIndexBuffer::InitParams& IBInitParams = meshInitParams.m_IBInitParams;
    IBInitParams.m_ResourceName = "SquidRoom Mesh Index Buffer";
    IBInitParams.m_InitData = m_SquidRoomData.data() + SquidRoom::IndexDataOffset;
    IBInitParams.m_NumIndices = SquidRoom::IndexDataSize / 4; // R32_UINT (SampleAssets::StandardIndexFormat) = 4 bytes each.
    IBInitParams.m_IndexSize = 4;

    this->SquidRoom.Initialize(meshInitParams);
}

void GfxDefaultAssets::CreateSquidRoomTextures(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    assert(m_SquidRoomData.size());

    using namespace SampleAssets;

    const uint32_t srvCount = _countof(SquidRoom::Textures);
    this->SquidRoomTextures.resize(srvCount);

    for (uint32_t i = 0; i < srvCount; i++)
    {
        GfxTexture& tex = this->SquidRoomTextures[i];
        const SquidRoom::TextureResource& texResource = SquidRoom::Textures[i];

        subFlow.emplace([&]()
            {
                GfxTexture::InitParams initParams;
                initParams.m_Format = texResource.Format;
                initParams.m_Width = texResource.Width;
                initParams.m_Height = texResource.Height;
                initParams.m_InitData = m_SquidRoomData.data() + texResource.Data->Offset;
                initParams.m_ResourceName = texResource.Name;

                tex.Initialize(initParams);
            });
    }
}

void GfxDefaultAssets::ClearPreloadedSampleAssetsMemory()
{
    m_OcccityData.clear();
    m_OcccityData.shrink_to_fit();

    m_SquidRoomData.clear();
    m_SquidRoomData.shrink_to_fit();
}

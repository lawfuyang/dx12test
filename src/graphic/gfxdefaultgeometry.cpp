#include <graphic/gfxdefaultgeometry.h>

void GfxDefaultGeometry::Initialize(tf::Subflow& sf)
{
    bbeProfileFunction();

    ADD_TF_TASK(sf, CreateUnitCube());
    ADD_TF_TASK(sf, CreateUnitSphere());
    ADD_TF_TASK(sf, CreateTeapot());
}

void GfxDefaultGeometry::Shutdown()
{
    UnitCube.VertexBuffer.Release();
    UnitCube.IndexBuffer.Release();
    UnitSphere.VertexBuffer.Release();
    UnitSphere.IndexBuffer.Release();
    Teapot.VertexBuffer.Release();
    Teapot.IndexBuffer.Release();
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

void GfxDefaultGeometry::CreateUnitCube()
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

    GfxVertexBuffer::InitParams VBInitParams;
    VBInitParams.m_InitData = vertices.data();
    VBInitParams.m_NumVertices = vertices.size();
    VBInitParams.m_VertexSize = sizeof(Vertex);
    VBInitParams.m_ResourceName = "GfxDefaultGeometry::UnitCube Vertex Buffer";

    GfxIndexBuffer::InitParams IBInitParams;
    IBInitParams.m_InitData = indices.data();
    IBInitParams.m_NumIndices = indices.size();
    IBInitParams.m_IndexSize = sizeof(uint16_t);
    IBInitParams.m_ResourceName = "GfxDefaultGeometry::UnitCube Index Buffer";

    GfxDefaultGeometry::UnitCube.VertexBuffer.Initialize(VBInitParams);
    GfxDefaultGeometry::UnitCube.IndexBuffer.Initialize(IBInitParams);
}

void GfxDefaultGeometry::CreateUnitSphere()
{

}

void GfxDefaultGeometry::CreateTeapot()
{

}

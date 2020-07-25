#pragma once

class GfxMesh;
class GfxTexture;

class Visual
{
public:

    ObjectID m_ObjectID = ID_InvalidObject;
    std::string m_Name;

    GfxMesh* m_Mesh = nullptr;
    GfxTexture* m_AlbedoTexture = nullptr;
    GfxTexture* m_NormalTexture = nullptr;
    GfxTexture* m_ORMTexture = nullptr;
};

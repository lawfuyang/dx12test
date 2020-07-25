#pragma once

#include <graphic/gfx/gfxdefaultassets.h>

class Visual
{
public:
    void UpdatePropertiesIMGUI();

    ObjectID m_ObjectID = ID_InvalidObject;
    std::string m_Name = "Unnamed Visual";

    GfxMesh* m_Mesh = &GfxDefaultAssets::UnitCube;
    GfxTexture* m_AlbedoTexture = &GfxDefaultAssets::Checkerboard;
    GfxTexture* m_NormalTexture = &GfxDefaultAssets::FlatNormal;
    GfxTexture* m_ORMTexture = &GfxDefaultAssets::Black2D;

    bbeVector3 m_WorldPosition;
    bbeQuaternion m_Rotation;
};

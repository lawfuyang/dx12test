#pragma once

#include <graphic/gfx/gfxdefaultassets.h>

class Visual
{
public:
    DeclareObjectModelFunctions(Visual);

    void UpdateIMGUI();

    bool IsValid();

    std::string m_Name = "Unnamed Visual";

    GfxMesh* m_Mesh = &GfxDefaultAssets::UnitCube;
    GfxTexture* m_DiffuseTexture = &GfxDefaultAssets::Checkerboard;
    GfxTexture* m_NormalTexture = &GfxDefaultAssets::FlatNormal;
    GfxTexture* m_ORMTexture = &GfxDefaultAssets::Yellow2D;

    bbeVector3 m_Scale = bbeVector3::One;
    bbeVector3 m_WorldPosition;
    bbeQuaternion m_Rotation;
};

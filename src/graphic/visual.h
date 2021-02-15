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

    bool m_UseGlobalPBRConsts = false;
    bbeVector3 m_Scale = bbeVector3::One * 100; // the unit cube is smol, so scale it up first. it will be reset back to '1' after selected new mesh
    bbeVector3 m_WorldPosition;
    bbeQuaternion m_Rotation;
};

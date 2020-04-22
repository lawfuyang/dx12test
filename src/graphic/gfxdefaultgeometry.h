#pragma once

#include <graphic/gfxtexturesandbuffers.h>

struct GfxDrawData
{
    GfxVertexBuffer VertexBuffer;
    GfxIndexBuffer IndexBuffer;
};

class GfxDefaultGeometry
{
public:
    DeclareSingletonFunctions(GfxDefaultGeometry);

    void Initialize(tf::Subflow& sf);
    void Shutdown();

    inline static GfxDrawData UnitCube;
    inline static GfxDrawData UnitSphere;
    inline static GfxDrawData Teapot;

private:
    static void CreateUnitCube();
    static void CreateUnitSphere();
    static void CreateTeapot();
};
#define g_GfxDefaultGeometry GfxDefaultGeometry::GetInstance()

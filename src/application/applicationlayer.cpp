#include <application/cameracontroller.h>
#include <application/scene.h>

void InitializeApplicationLayer(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    subFlow.emplace([] { g_CameraController.Initialize(); });
    subFlow.emplace([] { g_Scene.Initialize(); });
}

void UpdateApplicationLayer(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    subFlow.emplace([] { g_CameraController.Update(); });
    subFlow.emplace([] { g_Scene.Update(); });
}

void ShutdownApplicationLayer()
{
    bbeProfileFunction();
    g_Scene.ShutDown();
}

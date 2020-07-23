#include <application/cameracontroller.h>
#include <application/scene.h>

void InitializeApplicationLayer(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    ADD_TF_TASK(subFlow, g_CameraController.Initialize());
    ADD_TF_TASK(subFlow, g_Scene.Initialize());
}

void UpdateApplicationLayer(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    ADD_TF_TASK(subFlow, g_CameraController.Update());
    ADD_TF_TASK(subFlow, g_Scene.Update());
}

void ShutdownApplicationLayer()
{
    bbeProfileFunction();
}

#include "application/cameracontroller.h"

void InitializeApplicationLayer(tf::Taskflow& tf)
{
    bbeProfileFunction();

    g_CameraController.Initialize();
}

void UpdateApplicationLayer(tf::Taskflow& tf)
{
    bbeProfileFunction();

    g_CameraController.Update();
}

void ShutdownApplicationLayer()
{
    bbeProfileFunction();
}

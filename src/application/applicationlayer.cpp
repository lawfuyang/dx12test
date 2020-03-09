#include "application/cameracontroller.h"

void InitializeApplicationLayer(tf::Taskflow& tf)
{
    g_CameraController.Initialize();
}

void UpdateApplicationLayer(tf::Taskflow& tf)
{
    g_CameraController.Update();
}

void ShutdownApplicationLayer()
{

}

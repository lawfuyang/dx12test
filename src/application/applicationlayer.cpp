#include "application/cameracontroller.h"

void InitializeApplicationLayer()
{
    bbeProfileFunction();

    g_CameraController.Initialize();
}

void UpdateApplicationLayer()
{
    bbeProfileFunction();

    g_CameraController.Update();
}

void ShutdownApplicationLayer()
{
    bbeProfileFunction();
}

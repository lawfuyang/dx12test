#pragma once

static const char* GetGfxDeviceTypeName(GfxDeviceType type)
{
    switch (type)
    {
        bbeSimpleSwitchCaseString(GfxDeviceType::MainDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::DeferredDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::LoadingThreadDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::ComputeDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::CopyDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::CreationDevice);
        bbeSimpleSwitchCaseString(GfxDeviceType::GfxDeviceTypeCount);
        bbeSimpleSwitchCaseString(GfxDeviceType::InvalidDevice);
        default: return "Unknown GfxDeviceType!";
    }
};

static const char* GetGfxQueueTypeName(GfxQueueType type) 
{
    switch (type)
    {
        bbeSimpleSwitchCaseString(GfxQueueType::Graphics);
        bbeSimpleSwitchCaseString(GfxQueueType::Compute);
        bbeSimpleSwitchCaseString(GfxQueueType::Copy);
        default: return "Unknown GfxQueueType!";
    }
}

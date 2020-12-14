#pragma once

class GfxResourceManager
{
public:
    DeclareSingletonFunctions(GfxResourceManager);

    template <typename T>
    using ResourceLoadingFinalizer = std::function<void(T*)>;

    // if it's already loaded, return ptr to resource. If not, the finalizer will be called when its loaded from disk, and nullptr will be returned
    template <typename T>
    T* Get(const std::string& filePath, const ResourceLoadingFinalizer<T>& finalizer);

private:
};
#define g_GfxResourceManager GfxResourceManager::GetInstance()

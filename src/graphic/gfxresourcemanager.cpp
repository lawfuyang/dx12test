#include <graphic/gfxresourcemanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

std::vector<std::function<void()>> gs_AllManagedGfxResourcesReleasers;

template <typename T>
struct ManagedGfxResources
{
    using HashedResourceFilePath = std::size_t;
    std::unordered_map<HashedResourceFilePath, T*> m_ResourceCache;
    std::shared_mutex m_Lock;

    T* m_DefaultResource = nullptr;

    static T* Load(const std::string& filePath);
    void Release();

    static ManagedGfxResources<T>& Get() 
    {
        static ManagedGfxResources<T> ms_Instance;
        return ms_Instance;
    }
};

#define RegisterResource(RESOURCE_TYPE, DEFAULT_RESOURCE)                                                                                     \
template RESOURCE_TYPE* GfxResourceManager::Get(const std::string&, const ResourceLoadingFinalizer<RESOURCE_TYPE>&);                          \
                                                                                                                                              \
struct RESOURCE_TYPE_Register                                                                                                                 \
{                                                                                                                                             \
    RESOURCE_TYPE_Register()                                                                                                                  \
    {                                                                                                                                         \
        gs_AllManagedGfxResourcesReleasers.push_back([]() { ManagedGfxResources<RESOURCE_TYPE>::Get().Release(); });                          \
        ManagedGfxResources<RESOURCE_TYPE>::Get().m_DefaultResource = DEFAULT_RESOURCE;                                                       \
    }                                                                                                                                         \
};                                                                                                                                            \
static RESOURCE_TYPE_Register gs_RESOURCE_TYPE_Register;

RegisterResource(GfxTexture, &GfxDefaultAssets::Checkerboard);

#undef RegisterResource

template <typename T>
T* GfxResourceManager::Get(const std::string& filePath, const ResourceLoadingFinalizer<T>& finalizer)
{
    ManagedGfxResources<T>& resources = ManagedGfxResources<T>::Get();

    // if resource is already loaded in memory, return ptr to resource
    {
        const std::size_t hashedFilePath = std::hash<std::string>{}(filePath);

        std::shared_lock<std::shared_mutex> readLock{ resources.m_Lock };
        if (resources.m_ResourceCache.count(hashedFilePath))
        {
            return resources.m_ResourceCache[hashedFilePath];
        }
    }

    // resource not yet loaded in memory. Load it async in BG thread, and return nullptr
    g_System.AddBGAsyncCommand([&, filePath, finalizer]()
        {
            T* loadedResource = nullptr;

            {
                std::unique_lock<std::shared_mutex> writeLock{ resources.m_Lock };
                loadedResource = ManagedGfxResources<T>::Load(filePath);
            }
            
            // unable to load for some reason. Assign default resource
            if (!loadedResource)
            {
                g_Log.error("Failed to load {}!", filePath.c_str());

                assert(resources.m_DefaultResource);
                loadedResource = resources.m_DefaultResource;
            }
            else
            {
                const std::size_t hashedFilePath = std::hash<std::string>{}(filePath);

                std::unique_lock<std::shared_mutex> writeLock{ resources.m_Lock };
                resources.m_ResourceCache[hashedFilePath] = loadedResource;
            }

            g_GfxManager.AddGraphicCommand([loadedResource, finalizer]() { finalizer(loadedResource); });
        });

    return nullptr;
}

void GfxResourceManager::ShutDown()
{
    for (const auto& releaser : gs_AllManagedGfxResourcesReleasers)
    {
        releaser();
    }
}

template<>
GfxTexture* ManagedGfxResources<GfxTexture>::Load(const std::string& filePath)
{
    bbeProfileFunction();

    return nullptr;
}

template<>
void ManagedGfxResources<GfxTexture>::Release()
{
    for (auto it : m_ResourceCache) { it.second->Release(); }
    m_ResourceCache.clear();
}

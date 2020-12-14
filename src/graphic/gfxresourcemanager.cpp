#include <graphic/gfxresourcemanager.h>
#include <graphic/pch.h>

#include <graphic/WICTextureLoader12.h>
#include <graphic/DDSTextureLoader12.h>

std::vector<std::function<void()>> gs_AllManagedGfxResourcesReleasers;

template <typename T>
struct ManagedGfxResources
{
    using HashedResourceFilePath = std::size_t;
    std::unordered_map<HashedResourceFilePath, T*> m_ResourceCache;
    std::shared_mutex m_CacheLock;

    ObjectPool<T> m_Pool;
    std::mutex m_PoolLock;

    void Load(const std::string& filePath, const GfxResourceManager::ResourceLoadingFinalizer<T>&);
    void Release();

    static ManagedGfxResources<T>& Get() 
    {
        static ManagedGfxResources<T> ms_Instance;
        return ms_Instance;
    }
};

#define RegisterResource(RESOURCE_TYPE)                                                                              \
template RESOURCE_TYPE* GfxResourceManager::Get(const std::string&, const ResourceLoadingFinalizer<RESOURCE_TYPE>&); \
                                                                                                                     \
struct RESOURCE_TYPE_Register                                                                                        \
{                                                                                                                    \
    RESOURCE_TYPE_Register()                                                                                         \
    {                                                                                                                \
        ManagedGfxResources<RESOURCE_TYPE>& resources = ManagedGfxResources<RESOURCE_TYPE>::Get();                   \
        gs_AllManagedGfxResourcesReleasers.push_back([&]()                                                           \
            {                                                                                                        \
                resources.Release();                                                                                 \
                resources.m_ResourceCache.clear();                                                                   \
            });                                                                                                      \
    }                                                                                                                \
};                                                                                                                   \
static RESOURCE_TYPE_Register gs_RESOURCE_TYPE_Register;

RegisterResource(GfxTexture);

#undef RegisterResource

template <typename T>
T* GfxResourceManager::Get(const std::string& filePath, const ResourceLoadingFinalizer<T>& finalizer)
{
    ManagedGfxResources<T>& resources = ManagedGfxResources<T>::Get();

    // if resource is already loaded in memory, return ptr to resource
    const std::size_t hashedFilePath = std::hash<std::string>{}(filePath);

    bbeAutoLockRead(resources.m_CacheLock);
    if (resources.m_ResourceCache.count(hashedFilePath))
    {
        g_Log.info("Retrieved {} from resource cache", filePath.c_str());
        return resources.m_ResourceCache[hashedFilePath];
    }

    // resource not yet loaded in memory. Load it async in BG thread, and return nullptr
    g_System.AddBGAsyncCommand([&, filePath, finalizer]() { resources.Load(filePath, finalizer); });

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
void ManagedGfxResources<GfxTexture>::Load(const std::string& filePath, const GfxResourceManager::ResourceLoadingFinalizer<GfxTexture>& finalizer)
{
    bbeProfileFunction();

    const std::string fileExt = GetFileExtensionFromPath(filePath);
    const std::wstring filePathW = StringUtils::Utf8ToWide(filePath);

    std::vector<std::byte> decodedData;
    D3D12_RESOURCE_DESC texDesc{};
    D3D12_SUBRESOURCE_DATA subResource{};
    if (fileExt == "dds")
    {
        __noop; // not implemented yet
    }
    else
    {
        DirectX::LoadWICTextureFromFileSimple(filePathW.c_str(), decodedData, subResource, texDesc);
    }

    const uint32_t imageBytes = (uint32_t)subResource.SlicePitch;
    if (imageBytes == 0)
    {
        g_Log.error("Failed to load {} from disk!", filePath.c_str());
        return;
    }

    // init and assign gfx texture in the next gfx frame
    auto CreateGfxTexture = [&, texDesc = std::move(texDesc), initData = std::move(decodedData), filePath, finalizer]()
    {
        GfxTexture* newTex = [&]()
        {
            bbeAutoLock(m_PoolLock);
            return m_Pool.construct();
        }();
        newTex->Initialize(CD3DX12_RESOURCE_DESC1::Tex2D(texDesc.Format, texDesc.Width, texDesc.Height), initData.data());
        newTex->SetDebugName(GetFileNameFromPath(filePath).c_str());

        finalizer(newTex);

        g_Log.info("Loaded and initialized {}", filePath.c_str());

        // finally, cache gfx texture
        const std::size_t hashedFilePath = std::hash<std::string>{}(filePath);

        bbeAutoLockWrite(m_CacheLock);
        m_ResourceCache[hashedFilePath] = newTex;
    };
    g_GfxManager.AddGraphicCommand(CreateGfxTexture);
}

template<>
void ManagedGfxResources<GfxTexture>::Release()
{
    for (auto& it : m_ResourceCache) { it.second->Release(); }
}

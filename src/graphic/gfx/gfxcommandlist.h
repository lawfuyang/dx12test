#pragma once

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList5* Dev() const { return m_CommandList.Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE);
    void BeginRecording();
    void EndRecording();

    D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

    void SetName(const std::string& name) { m_Name = name; };

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0;

    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList5> m_CommandList;

    std::string m_Name;

    friend class GfxCommandListsManager;
};

class GfxCommandListsManager
{
    DeclareSingletonFunctions(GfxCommandListsManager);

public:
    static const uint32_t MaxCmdLists = 128;

    void Initialize();
    void ShutDown();

    GfxCommandList* Allocate(D3D12_COMMAND_LIST_TYPE, const std::string&);
    void QueueCommandListToExecute(GfxCommandList&, D3D12_COMMAND_LIST_TYPE);
    void ExecutePendingCommandLists();

    ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) { return GetPoolFromType(type).m_CommandQueue.Get(); }

private:
    struct CommandListPool
    {
        ComPtr<ID3D12CommandQueue> m_CommandQueue;

        ObjectPool<GfxCommandList> m_CommandListsPool;

        std::mutex m_ListsLock;

        CircularBuffer<GfxCommandList*> m_FreeCommandLists;
        InplaceArray<GfxCommandList*, MaxCmdLists> m_PendingExecuteCommandLists;
    };
    CommandListPool& GetPoolFromType(D3D12_COMMAND_LIST_TYPE);

    CommandListPool m_DirectPool;
};
#define g_GfxCommandListsManager GfxCommandListsManager::GetInstance()

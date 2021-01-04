#pragma once

class GfxFence;

class GfxCommandList
{
public:
    D3D12GraphicsCommandList* Dev() const { return m_CommandList.Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE, uint32_t queueIdx);
    void BeginRecording();

    D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;
    uint32_t m_QueueIndex = 0xDEADBEEF;

    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<D3D12GraphicsCommandList> m_CommandList;

    friend class GfxCommandListsManager;
    friend class GfxCommandListQueue;
};

class GfxCommandListQueue
{
public:
    static const uint32_t MaxCmdLists = 128;

    ID3D12CommandQueue* Dev() const { return m_CommandQueue.Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE type, uint32_t queueIdx);
    void ShutDown();
    GfxCommandList* AllocateCommandList(std::string_view name);
    bool HasPendingCommandLists() const { return !m_PendingExecuteCommandLists.empty(); };
    void ExecutePendingCommandLists();
    void StallGPUForFence(GfxFence&) const;

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;

    ComPtr<ID3D12CommandQueue> m_CommandQueue;

    std::mutex m_ListsLock;
    CircularBuffer<GfxCommandList> m_CommandLists;
    InplaceArray<GfxCommandList*, MaxCmdLists> m_PendingExecuteCommandLists;

    friend class GfxCommandListsManager;
};

class GfxCommandListsManager
{
    DeclareSingletonFunctions(GfxCommandListsManager);

public:
    static const uint32_t NbQueues = 3;

    void Initialize(tf::Subflow& sf);
    void ShutDown();

    void QueueCommandListToExecute(GfxCommandList*);
    void ExecuteCommandListImmediate(GfxCommandList*);
    void ExecutePendingCommandLists();

    GfxCommandListQueue& GetMainQueue() { return m_DirectQueue; }
    GfxCommandListQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_DirectQueue;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_AsyncComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
        }

        assert(false);
        return *(GfxCommandListQueue*)0;
    }

private:
    GfxCommandListQueue m_DirectQueue;
    GfxCommandListQueue m_AsyncComputeQueue;
    GfxCommandListQueue m_CopyQueue;

    // convenience array of all queues
    GfxCommandListQueue* m_AllQueues[NbQueues] = { &m_DirectQueue, &m_AsyncComputeQueue, &m_CopyQueue };
};
#define g_GfxCommandListsManager GfxCommandListsManager::GetInstance()

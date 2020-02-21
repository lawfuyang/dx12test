#pragma once

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList5* Dev() const { return m_CommandList[m_RecordingCmdListIdx].Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE);
    void BeginRecording();
    void EndRecording();

    D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0;

    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList5> m_CommandList[2];

    uint32_t m_RecordingCmdListIdx = 1;
};

class GfxCommandListsManager
{
public:
    void Initialize();
    void ShutDown();

    GfxCommandList* Allocate(D3D12_COMMAND_LIST_TYPE);
    void ExecuteAllActiveCommandLists();

    ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) { return GetPoolFromType(type).m_CommandQueue.Get(); }

private:
    struct CommandListPool
    {
        ComPtr<ID3D12CommandQueue> m_CommandQueue;

        AdaptiveLock m_PoolLock;
        boost::object_pool<GfxCommandList>     m_CommandListsPool;

        tbb::concurrent_queue<GfxCommandList*> m_FreeCommandLists;
        tbb::concurrent_queue<GfxCommandList*> m_ActiveCommandLists;
        tbb::concurrent_queue<GfxCommandList*> m_PendingFreeCommandLists;
    };
    CommandListPool& GetPoolFromType(D3D12_COMMAND_LIST_TYPE);

    CommandListPool m_DirectPool;
};

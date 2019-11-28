#pragma once

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList* Dev() const { return m_CommandList[m_RecordingCmdListIdx].Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE);
    void BeginRecording();
    void EndRecording();

    D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0;

    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_CommandList[2];

    uint32_t m_RecordingCmdListIdx = 1;
};

class GfxCommandListsManager
{
public:
    void Initialize();

    GfxCommandList* Allocate(D3D12_COMMAND_LIST_TYPE);
    void ExecuteAllActiveCommandLists();

    ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) { return GetPoolFromType(type).m_CommandQueue.Get(); }

private:
    struct CommandListPool
    {
        ComPtr<ID3D12CommandQueue> m_CommandQueue;

        boost::object_pool<GfxCommandList>      m_CommandListsPool;
        boost::lockfree::stack<GfxCommandList*> m_FreeCommandLists{ 32 };
        boost::lockfree::queue<GfxCommandList*> m_ActiveCommandLists{ 32 };

        SpinLock m_PoolLock;
    };
    CommandListPool& GetPoolFromType(D3D12_COMMAND_LIST_TYPE);

    CommandListPool m_DirectPool;
};

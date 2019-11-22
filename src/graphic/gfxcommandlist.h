#pragma once

#include "extern/boost/pool/object_pool.hpp"
#include "extern/boost/lockfree/stack.hpp"

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

    ID3D12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) { return m_Pools[type].m_CommandQueue.Get(); }

private:
    struct CommandListPool
    {
        ComPtr<ID3D12CommandQueue> m_CommandQueue;

        boost::object_pool<GfxCommandList>      m_CommandListsPool;
        boost::lockfree::stack<GfxCommandList*> m_FreeCommandLists{ 32 };
        boost::lockfree::stack<GfxCommandList*> m_ActiveCommandLists{ 32 };
    };

    std::unordered_map<D3D12_COMMAND_LIST_TYPE, CommandListPool> m_Pools;
};

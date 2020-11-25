#pragma once

#include <graphic/dx12utils.h>
#include <graphic/gfx/gfxfence.h>

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList5* Dev() const { return m_CommandList.Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE);
    void BeginRecording();

    D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;

    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList5> m_CommandList;

    friend class GfxCommandListsManager;
};

class GfxCommandListQueue
{
public:
    static const uint32_t MaxCmdLists = 128;

    ID3D12CommandQueue* Dev() const { return m_CommandQueue.Get(); }

    void Initialize(D3D12_COMMAND_LIST_TYPE type);
    GfxCommandList* AllocateCommandList(const std::string&);
    bool HasPendingCommandLists() const { return !m_PendingExecuteCommandLists.empty(); };
    void ExecutePendingCommandLists();
    void SignalFence() { m_Fence.IncrementAndSignal(Dev()); }
    void StallCPUForFence() const { m_Fence.WaitForSignalFromGPU(); }
    void StallGPUForFence() const { DX12_CALL(m_CommandQueue->Wait(m_Fence.Dev(), m_Fence.GetValue())); }

private:
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;

    GfxFence m_Fence;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    ObjectPool<GfxCommandList> m_CommandListsPool;

    std::mutex m_ListsLock;
    CircularBuffer<GfxCommandList*> m_FreeCommandLists;
    InplaceArray<GfxCommandList*, MaxCmdLists> m_PendingExecuteCommandLists;

    friend class GfxCommandListsManager;
};

class GfxCommandListsManager
{
    DeclareSingletonFunctions(GfxCommandListsManager);

public:
    void Initialize();
    void ShutDown();

    void QueueCommandListToExecute(GfxCommandList*);
    void ExecutePendingCommandLists();

    GfxCommandListQueue& GetMainQueue() { return m_DirectPool; }
    GfxCommandListQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_DirectPool;
        default: assert(false); return m_DirectPool;
        }
    }

private:
    GfxCommandListQueue m_DirectPool;
};
#define g_GfxCommandListsManager GfxCommandListsManager::GetInstance()

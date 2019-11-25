#pragma once

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapPageCommon
{
public:
    virtual ~GfxDescriptorHeapPageCommon() {}

    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }


protected:
    static constexpr uint32_t NbDescHandles = 32;

    virtual void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) = 0;
    void InitializeCommon(D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_FLAGS);

    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    uint32_t m_DescriptorSize = 0;

private:
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NbDescHandles> m_CPUDescriptorHandles;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorNonShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;

private:
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, NbDescHandles> m_GPUDescriptorHandles;
};

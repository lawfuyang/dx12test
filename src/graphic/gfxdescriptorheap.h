#pragma once

#include "extern/boost/pool/object_pool.hpp"
#include "extern/boost/lockfree/stack.hpp"

class GfxDescriptorHeapPool
{
public:

private:

};

static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == 0);
static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == 1);

class GfxDescriptorHeapManager
{
public:
    void Initalize();

private:
    GfxDescriptorHeapPool m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    GfxDescriptorHeapPool m_ShaderVisiblePools[2]; // CBV_SRV_UAV & SAMPLER
    GfxDescriptorHeapPool m_NullPools[2]; // CBV_SRV_UAV & SAMPLER
};

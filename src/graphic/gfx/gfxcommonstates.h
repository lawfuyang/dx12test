#pragma once

class GfxCommonStates
{
public:

    // Blend states.
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendOpaque;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendModulate;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendAlpha;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendAdditive;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendAlphaAdditive;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendDestAlpha;
    static const D3D12_RENDER_TARGET_BLEND_DESC BlendPremultipliedAlpha;

    // Depth stencil states.
    static const D3D12_DEPTH_STENCIL_DESC DepthNone;
    static const D3D12_DEPTH_STENCIL_DESC DepthDefault;
    static const D3D12_DEPTH_STENCIL_DESC DepthRead;

    // Rasterizer states.
    static const D3D12_RASTERIZER_DESC CullNone;
    static const D3D12_RASTERIZER_DESC CullClockwise;
    static const D3D12_RASTERIZER_DESC CullCounterClockwise;
    static const D3D12_RASTERIZER_DESC Wireframe;
};

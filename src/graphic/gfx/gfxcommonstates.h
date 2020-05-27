//--------------------------------------------------------------------------------------
// File: CommonStates.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

class GfxCommonStates
{
public:

    // Blend states.
    static const D3D12_BLEND_DESC Opaque;
    static const D3D12_BLEND_DESC AlphaBlend;
    static const D3D12_BLEND_DESC Additive;
    static const D3D12_BLEND_DESC NonPremultiplied;

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

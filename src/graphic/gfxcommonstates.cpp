//--------------------------------------------------------------------------------------
// File: CommonStates.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include <graphic/gfxcommonstates.h>

// --------------------------------------------------------------------------
// Blend States
// --------------------------------------------------------------------------

const D3D12_BLEND_DESC GfxCommonStates::Opaque =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    { {
        FALSE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_ONE, // SrcBlend
        D3D12_BLEND_ZERO, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_ONE, // SrcBlendAlpha
        D3D12_BLEND_ZERO, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    } }
};

const D3D12_BLEND_DESC GfxCommonStates::AlphaBlend =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    { {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_ONE, // SrcBlend
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_ONE, // SrcBlendAlpha
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    } }
};

const D3D12_BLEND_DESC GfxCommonStates::Additive =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    { {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_SRC_ALPHA, // SrcBlend
        D3D12_BLEND_ONE, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_SRC_ALPHA, // SrcBlendAlpha
        D3D12_BLEND_ONE, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    } }
};

const D3D12_BLEND_DESC GfxCommonStates::NonPremultiplied =
{
    FALSE, // AlphaToCoverageEnable
    FALSE, // IndependentBlendEnable
    { {
        TRUE, // BlendEnable
        FALSE, // LogicOpEnable
        D3D12_BLEND_SRC_ALPHA, // SrcBlend
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlend
        D3D12_BLEND_OP_ADD, // BlendOp
        D3D12_BLEND_SRC_ALPHA, // SrcBlendAlpha
        D3D12_BLEND_INV_SRC_ALPHA, // DestBlendAlpha
        D3D12_BLEND_OP_ADD, // BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    } }
};


// --------------------------------------------------------------------------
// Depth-Stencil States
// --------------------------------------------------------------------------

const D3D12_DEPTH_STENCIL_DESC GfxCommonStates::DepthNone =
{
    FALSE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,
    D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};

const D3D12_DEPTH_STENCIL_DESC GfxCommonStates::DepthDefault =
{
    TRUE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ALL,
    D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};

const D3D12_DEPTH_STENCIL_DESC GfxCommonStates::DepthRead =
{
    TRUE, // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,
    D3D12_COMPARISON_FUNC_LESS_EQUAL, // DepthFunc
    FALSE, // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    }, // FrontFace
    {
        D3D12_STENCIL_OP_KEEP, // StencilFailOp
        D3D12_STENCIL_OP_KEEP, // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP, // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
    } // BackFace
};


// --------------------------------------------------------------------------
// Rasterizer States
// --------------------------------------------------------------------------

const D3D12_RASTERIZER_DESC GfxCommonStates::CullNone =
{
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_NONE,
    FALSE, // FrontCounterClockwise
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

const D3D12_RASTERIZER_DESC GfxCommonStates::CullClockwise =
{
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_FRONT,
    FALSE, // FrontCounterClockwise
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

const D3D12_RASTERIZER_DESC GfxCommonStates::CullCounterClockwise =
{
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_BACK,
    FALSE, // FrontCounterClockwise
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

const D3D12_RASTERIZER_DESC GfxCommonStates::Wireframe =
{
    D3D12_FILL_MODE_WIREFRAME,
    D3D12_CULL_MODE_NONE,
    FALSE, // FrontCounterClockwise
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE, // DepthClipEnable
    TRUE, // MultisampleEnable
    FALSE, // AntialiasedLineEnable
    0, // ForcedSampleCount
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

#pragma once

enum class GfxDeviceType
{
    MainDevice,
    DeferredDevice,
    LoadingThreadDevice,
    ComputeDevice,
    CopyDevice,
    CreationDevice,

    GfxDeviceTypeCount,

    InvalidDevice
};

/** @brief Resource memory access flags. */
enum class GfxResourceAccessFlags
{
    None             = (0 << 0),           // None
    CPURead          = (1 << 0),           // CPU read access allowed.  Cannot be set as input or outputs to the pipeline.
    CPUWrite         = (1 << 1),           // CPU write access allowed.  Cannot be set as outputs to the pipeline.
    GPURead          = (1 << 2),           // GPU read access allowed.
    GPUWrite         = (1 << 3),           // GPU write access allowed.
    GPUReadWrite     = GPURead | GPUWrite, // GPU read write access allowed.
    GPUExecute       = (1 << 4),           // GPU execute access allowed.  Needed for indirect or counter buffer.
    CPUWriteCombined = (1 << 5),           // CPU write combined memory.
    MaxAccessFlags   = (1 << 6),           // Max value for access flags.
};
BBE_DEFINE_ENUM_OPERATOR(GfxResourceAccessFlags);

/** @brief  Render topology. */
enum class GfxTopology : uint8_t
{
    PointList,     // Point list.
    LineList,      // List list.
    LineStrip,     // Line strip.
    TriangleList,  // Triangle list.
    TriangleStrip, // Triangle strip.
    PatchList1CP,  // Patch list one control point.
    PatchList2CP,  // Patch list two control points.
    PatchList3CP,  // Patch list three control points.
    PatchList4CP,  // Patch list four control points.
    PatchList5CP,  // Patch list five control points.
    PatchList6CP,  // Patch list six control points.
    PatchList7CP,  // Patch list seven control points.
    PatchList8CP,  // Patch list eight control points.
    PatchList9CP,  // Patch list nine control points.
    PatchList10CP, // Patch list ten control points.
};

/** @brief Queue type */
enum class GfxQueueType
{
    Graphics,   // Graphics queue
    Compute,    // Compute queue
    Copy,       // Copy queue
    NumQueueType,
};

/** @brief Specifies the shaders that can access the contents of a given layout slot. */
enum class GfxShaderVisibility
{
    All,      // Specifies that all shader stages can access whatever is bound to the layout slot.
    Vertex,   // Specifies that the vertex shader stage can access whatever is bound to the layout slot.
    Hull,     // Specifies that the hull shader stage can access whatever is bound to the layout slot.
    Domain,   // Specifies that the domain shader stage can access whatever is bound to the layout slot.
    Geometry, // Specifies that the geometry shader stage can access whatever is bound to the layout slot.
    Pixel     // Specifies that the pixel shader stage can access whatever is bound to the layout slot.
};

/** @brief Type of descriptor range. */
enum class GfxDescriptorRangeAccess
{
    ReadOnly,  //< Read only descriptors.
    ReadWrite, //< Read write descriptors.
};

/** @brief Type of descriptor range. */
enum class GfxDescriptorRangeType
{
    Texture,          // Texture descriptor range
    Buffer,           // Read write descriptors.
    StructuredBuffer, // Read write descriptors.
    FormattedBuffer,  // Formatted buffer descriptor range
    ConstantBuffer,   // Constant buffer descriptor range
    Sampler,          // Sampler descriptor range
};

/** @brief Shader types */
enum class GfxShaderType
{
    VertexShader,             // Vertex shader.
    PixelShader,              // Pixel shader.
    GeometryShader,           // Geometry shader.
    HullShader,               // Hull shader.
    DomainShader,             // Domain shader.
    ComputeShader,            // Compute shader.
    NumShaderPipelineStages,  // Number of shader pipeline stages.
    Library,                  // Shader library.
    NumShaderType,            // Number of shader types.
};

/** @brief Depth stencil clear values */
struct GfxDepthStencil
{
    float Depth;     // Depth clear value.
    uint8_t Stencil; // Stencil clear value.
};

/** @brief Clear value needed for Clear* API calls */
union GfxClearValue
{
    float Color[4];                // Clear color for render targets.
    GfxDepthStencil DepthStencil;  // Clear depth stencil value for depth stencil.
};

/** @brief Generic compare modes. */
enum class GfxCompareFunction : uint32_t
{
    Never,        // Always fail the test.
    Less,         // Accept the new pixel if its value is less than the value of the current pixel.
    Equal,        // Accept the new pixel if its value is equal to the value of the current pixel.
    LessEqual,    // Accept the new pixel if its value is less than or equal to the value of the current pixel.
    Greater,      // Accept the new pixel if its value is greater than the value of the current pixel.
    NotEqual,     // Accept the new pixel if its value does not equal the value of the current pixel.
    GreaterEqual, // Accept the new pixel if its value is greater than or equal to the value of the current pixel.
    Always,       // Always pass the test.
};

/** @brief Memory type. */
enum class GfxMemoryType
{
    VirtualMemory  = (0 << 0), // Virtual memory.
    PhysicalMemory = (1 << 0), // Physical memory.
    EmbeddedMemory = (1 << 1), // Embedded memory.
    MaxMemoryType  = (1 << 2), // Max value for memory Type.
};

/** @brief Query type. */
enum class GfxQueryType
{
    Occlusion, // Occlusion query.
    Timestamp, // Timestamp query.
};

/** @brief DebugFlags. */
enum class GfxDebugFlags
{
    None                   = (0 << 0), // No debug flags.
    DebugValidation        = (1 << 0), // Enable debug validation.
    EnableGPUCapture       = (1 << 1), // Enable render doc/pix GPU capture layer.
    EnableGPUMarkers       = (1 << 2), // Enable GPU markers support.
    BreakOnErrors          = (1 << 3), // Break on errors.
    BreakOnWarnings        = (1 << 4), // Break on warnings.
    ValidateBarriers       = (1 << 5), // Validate barriers.
    ValidateRootParameters = (1 << 6), // Validate root parameters.
};
BBE_DEFINE_ENUM_OPERATOR(GfxDebugFlags);

/** @brief Texture sampling addressing modes. */
enum class GfxAddressMode : uint32_t
{
    Wrap,      // Tiles the texture at every integer junction.
    Mirror,    // For u values between 0 and 1, for example, the texture is addressed normally; between 1 and 2, the texture is flipped (mirrored); between 2 and 3, the texture is normal again, and so on.
    Clamp,     // Texture coordinates outside the range [0.0, 1.0] are set to the texture color at 0.0 or 1.0, respectively.
    Border,    // Texture coordinates outside the range [0.0, 1.0] are set to the border color.
    MirrorOnce // Similar to Mirror and Clamp. Takes the absolute value of the texture coordinate (thus, mirroring around 0), and then clamps to the maximum value.
};

/** @brief Texture sampling filter types. */
enum class GfxFilterType : uint32_t
{
    Point,      // Point filtering.
    Linear,     // Linear filtering.
    Anisotropic // Anisotropic filtering.
};

/** @brief Color write channel enable mask. */
enum class GfxColorWriteMask
{
    None = (0 << 0), // None.
    R    = (1 << 0), // Red channel only.
    G    = (1 << 1), // Green channel only.
    B    = (1 << 2), // Blue channel only.
    A    = (1 << 3), // Alpha channel only.
    RG   = R | G,    // Red and green channel.
    RGB  = RG | B,   // Red, green and blue channel.
    RGBA = A | RGB,  // Red, green, blue and alpha channel.
    All  = RGBA      // Red, green, blue and alpha channel.
};
BBE_DEFINE_ENUM_OPERATOR(GfxColorWriteMask);

enum class GfxTextureCreationFlags
{
    None         = 0,
    CreateROView = (1 << 0),
    CreateRWView = (1 << 1),
};

/** @brief Texture bind flags. */
enum class GfxTextureBindFlags
{
    None         = (0 << 0), // None
    RenderTarget = (1 << 1), // Can bind as a render target.
    DepthStencil = (1 << 2), // Can bind as a depth stencil.
    CopySrc      = (1 << 3), // Can bind as a copy source.
    CopyDst      = (1 << 4), // Can bind as a copy destination.
    SwapChain    = (1 << 5)  // Can bind as a swap chain
};
BBE_DEFINE_ENUM_OPERATOR(GfxTextureBindFlags);

/** @brief Texture flags. */
enum class GfxTextureFlags
{
    None               = (0 << 0), // None
    CubeMap            = (1 << 1), // Texture is a cube map.
    LinearTiling       = (1 << 2), // Use linear tiling.
    DepthCompression   = (1 << 3), // Enable depth compression.
    StencilCompression = (1 << 4), // Enable stencil compression.
    ColorCompression   = (1 << 5), // Enable color compression.
    DCCCompression     = (1 << 6), // Enable Direct Color Compression
};
BBE_DEFINE_ENUM_OPERATOR(GfxTextureFlags);

/** @brief Texture flags. */
enum class GfxTextureViewFlags
{
    None             = (0 << 0), // None
    ReadOnlyDepth    = (1 << 1), // Read only view to depth surface.
    ReadOnlyStencil  = (1 << 2), // Read only view to stencil surface
    FMask            = (1 << 3), // Creates a FMask view, only valid if the texture is a render target
    CubeMapAs2DArray = (1 << 4)  // Creates a FMask view, only valid if the texture is a render target
};
BBE_DEFINE_ENUM_OPERATOR(GfxTextureViewFlags);

enum class GfxBufferCreationFlags
{
    None         = 0,
    CreateROView = (1 << 0),
    CreateRWView = (1 << 1),
};

/** @brief Buffer bind flags. */
enum class GfxBufferBindFlags
{
    None              = (0 << 0), // None
    VertexBuffer      = (1 << 0), // Can bind as a vertex buffer
    IndexBuffer       = (1 << 1), // Can bind as an index buffer
    StreamOut         = (1 << 2), // Can bind for stream output.
    ConstantBuffer    = (1 << 3), // Can bind as a constant buffer.
    StructuredBuffer  = (1 << 4), // Can bind as a structured buffer.
    ByteAddressBuffer = (1 << 5), // Can bind as a byte address buffer.
    FormattedBuffer   = (1 << 6), // Can bind as a formatted buffer.
    CopySrc           = (1 << 7), // Can bind as a copy source.
    CopyDst           = (1 << 8), // Can bind as a copy destination.
};
BBE_DEFINE_ENUM_OPERATOR(GfxBufferBindFlags);

/** @brief  Descriptor heap type. */
enum class GfxDescriptorHeapType
{
    ShaderViews,  // Constant buffer, read only and read write views.
    Sampler,      // Sampler.
    RenderTarget, // Render target.
    DepthStencil, // Depth stencil.
};

/** @brief  Descriptor heap visibility. */
enum class GfxDescriptorHeapVisibility
{
    CPUVisible, // CPU visible descriptor heap.
    GPUVisible, // GPU visible descriptor heap.
};

/** @brief Use to specify if the layout is going to be use for draw or dispatch calls. */
enum class DescriptorLayoutType
{
    Draw,    // Use for draw calls
    Dispatch // Use for dispatch calls
};

/** @brief DescriptorFeatureFlags. */
enum class GfxDescriptorFeatureFlags
{
    None                    = (0 << 0), // No flags.
    EnableUpdateAfterBind   = (1 << 0), // Enable updating descriptors [unreferenced by shaders] after the descriptor table has been bound.
    EnableArrayRanges       = (1 << 1), // Enable array ranges in a descriptor table (instead of the default individual range per descriptor)
    EnableBindlessRanges    = (1 << 2), // Enable unbound ranges in a descriptor table (on most platforms only the last range can be unbound)
    EnablePartialBind       = (1 << 3), // Enable descriptor tables with uninitialized descriptors (as long they are unreferenced by the shaders)
    EnableOverlappingRanges = (1 << 4), // Makes the ranges in a table all start at the same register on platforms that support this feature
    EnableCustomSpaces      = (1 << 5), // Enable calling the custom space index extension for this table
};
BBE_DEFINE_ENUM_OPERATOR(GfxDescriptorFeatureFlags);

/** @brief Clear depth stencil flags. */
enum class GfxClearDSFlags
{
    None         = (1 << 0), // None
    ClearDepth   = (1 << 1), // Clear depth
    ClearStencil = (1 << 2), // Clear stencil
};

/** @brief Alpha blend operand values. */
enum class GfxBlendValue
{
    Zero,           // The blend factor is (0, 0, 0, 0).
    One,            // The blend factor is (1, 1, 1, 1).
    SrcColor,       // The blend factor is (Rs, Gs, Bs, As), that is color data (RGB) from a pixel shader.
    InvSrcColor,    // The blend factor is (1 - Rs, 1 - Gs, 1 - Bs, 1 - As), that is color data (RGB) from a pixel shader.
    SrcAlpha,       // The blend factor is (As, As, As, As), that is alpha data (A) from a pixel shader..
    InvSrcAlpha,    // The blend factor is ( 1 - A?, 1 - A?, 1 - A?, 1 - A?), that is alpha data (A) from a pixel shader. The pre-blend operation inverts the data, generating 1 - A.
    DestAlpha,      // The blend factor is (Ad Ad Ad Ad), that is alpha data from a render target.
    InvDestAlpha,   // The blend factor is (1 - Ad 1 - Ad 1 - Ad 1 - Ad), that is alpha data from a render target. The pre-blend operation inverts the data, generating 1 - A.
    DestColor,      // The blend factor is (Rd, Gd, Bd, Ad), that is color data from a render target.
    InvDestColor,   // The blend factor is (1 - Rd, 1 - Gd, 1 - Bd, 1 - Ad), that is color data from a render target. The pre-blend operation inverts the data, generating 1 - RGB.
    BlendFactor,    // The blend factor is the specified blend factor by SetBlendFactor.
    InvBlendFactor, // The blend factor is the specified blend factor by SetBlendFactor. The pre-blend operation inverts the blend factor, generating 1 - blend_factor.
    SrcAlphaSat,    // The blend factor is (f, f, f, 1); where f = min(As, 1 - Ad). The pre-blend operation clamps the data to 1 or less.
    Src1Color,      // The blend factor is data sources both as color data output by a pixel shader. This blend factor supports dual-source color blending.
    InvSrc1Color,   // The blend factor is data sources both as color data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - RGB. This blend factor supports dual-source color blending.
    Src1Alpha,      // he blend factor is data sources as alpha data output by a pixel shader. This blend factor supports dual-source color blending.
    InvSrc1Alpha,   // The blend factor is data sources as alpha data output by a pixel shader. The pre-blend operation inverts the data, generating 1 - A. This blend factor supports dual-source color blending.
};

/** @brief Alpha blend operations to be applied to BlendValues. */
enum class GfxBlendOp : uint32_t
{
    Add,         // Add source 1 and source 2.
    Subtract,    // Subtract source 1 from source 2.
    RevSubtract, // Subtract source 2 from source 1.
    Min,         // Find the minimum of source 1 and source 2.
    Max,         // Find the maximum of source 1 and source 2.
};

/** @brief Face culling modes. */
enum class GfxCullMode : uint32_t
{
    None, // None.
    Back, // Back face culling.
    Front // Front face culling.
};

/** @brief Winding modes. */
enum class GfxWindingOrder : uint8_t
{
    CW, // Clock wise.
    CCW // Counter clock wise.
};

/** @brief Stencil operation mode. */
enum class GfxStencilOp : uint32_t
{
    Keep,    // Keep the existing stencil data.
    Zero,    // Set the stencil data to 0.
    Replace, // Set the stencil data to the reference value.
    IncrSat, // Increment the stencil value by 1, and clamp the result.
    DecrSat, // Decrement the stencil value by 1, and clamp the result.
    Invert,  // Invert the stencil data.
    Incr,    // Increment the stencil value by 1, and wrap the result if necessary.
    Decr,    // Decrement the stencil value by 1, and wrap the result if necessary.
};

/** @brief Window modes */
enum class GfxWindowMode
{
    FullScreen, // Full screen.
    Windowed,   // Windowed.
};

enum class GfxBackBufferStatus
{
    Reserved,
    Occluded
};

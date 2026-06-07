-- Tickernel Lua API Documentation and Type Hints
-- Actual implementations are provided by C bindings in tknGfxLua.c
-- This file provides Lua IDE support and documentation

-- ============================================================================
-- Tickernel Constants
-- ============================================================================

--- Vertex binding enum for vertex input layouts
tkn.TKN_VERTEX_BINDING_DESCRIPTION = 0

--- Instance binding enum for vertex input layouts
tkn.TKN_INSTANCE_BINDING_DESCRIPTION = 1

-- ============================================================================
-- Vertex Input Layout Structures
-- ============================================================================
-- Lua representation of C structures for vertex and instance input layouts
-- 
-- C Structure (TknVertexInputLayout):
--   struct TknVertexInputLayout {
--       uint32_t binding;  // TKN_VERTEX_BINDING_DESCRIPTION (0) or TKN_INSTANCE_BINDING_DESCRIPTION (1)
--       uint32_t attributeCount;
--       TknVertexInputAttributeLayout[] attributes;  // Array of attributes
--   }
--
--   struct TknVertexInputAttributeLayout {
--       uint32_t location;    // Shader input location
--       int format;           // VkFormat enum (e.g., VK_FORMAT_R32_SFLOAT)
--       uint32_t offset;      // Offset in bytes within vertex/instance struct
--   }
--
-- Lua Table Format:
--   vertexLayout = {
--       binding = vulkan.TKN_VERTEX_BINDING_DESCRIPTION,  -- or 0
--       {location = 0, format = vulkan.VK_FORMAT_R32_SFLOAT, offset = 0},     -- position (8 bytes)
--       {location = 1, format = vulkan.VK_FORMAT_R32_SFLOAT, offset = 8},     -- texCoord
--   }
--   
--   instanceLayout = {
--       binding = vulkan.TKN_INSTANCE_BINDING_DESCRIPTION,  -- or 1
--       {location = 2, format = vulkan.VK_FORMAT_R32G32B32_SFLOAT, offset = 0},  -- position (12 bytes)
--       {location = 3, format = vulkan.VK_FORMAT_R32_UINT, offset = 12},         -- color
--   }
-- 
-- Notes:
--   - binding must be either TKN_VERTEX_BINDING_DESCRIPTION or TKN_INSTANCE_BINDING_DESCRIPTION
--   - location values must be unique across all attributes
--   - format must be a VkFormat constant (see vulkan.lua)
--   - offset is in bytes and must account for all previous attributes
--   - Vertex layouts typically use binding 0, instance layouts use binding 1

-- ============================================================================
-- Image Functions
-- ============================================================================

---Create a GPU image with specified properties
---@param pGfxContext lightuserdata Graphics context pointer
---@param dimension integer Image dimension (1=1D, 2=2D, 3=3D)
---@param format integer VkFormat enum value
---@param mipLevelCount integer Number of mipmap levels
---@param sampleCount integer Sample count for MSAA
---@param width integer Image width in pixels
---@param height integer Image height in pixels
---@param depth integer Image depth in pixels (1 for 2D)
---@param imageUsageFlags integer VkImageUsageFlags combination
---@return lightuserdata TknImage pointer
function tkn.tknCreateImagePtr(pGfxContext, dimension, format, mipLevelCount, sampleCount, width, height, depth, imageUsageFlags) end

---Destroy a GPU image
---@param pGfxContext lightuserdata Graphics context pointer
---@param pImage lightuserdata TknImage pointer
function tkn.tknDestroyImagePtr(pGfxContext, pImage) end

---Upload image data to GPU with explicit size (format-agnostic)
---@param pGfxContext lightuserdata Graphics context pointer
---@param pImage lightuserdata TknImage pointer
---@param pData string Binary image data (e.g., ASTC, ETC2, uncompressed)
---@param dataSize integer Size of image data in bytes
---@param width integer Image width in pixels
---@param height integer Image height in pixels
---@param depth integer Image depth in pixels
---@param mipLevel integer Target mipmap level
---@param offsetX integer X offset in pixels
---@param offsetY integer Y offset in pixels
---@param offsetZ integer Z offset in pixels
---@return boolean success True if upload succeeded
---@return string error Error message if upload failed
function tkn.tknWriteImagePtr(pGfxContext, pImage, pData, dataSize, width, height, depth, mipLevel, offsetX, offsetY, offsetZ) end

---Create an image view for sampling
---@param pGfxContext lightuserdata Graphics context pointer
---@param baseLayer integer Base layer index
---@param layerCount integer Number of layers
---@param aspectFlags integer VkImageAspectFlags (e.g., VK_IMAGE_ASPECT_COLOR_BIT)
---@param baseMipLevel integer Base mipmap level
---@param mipLevelCount integer Number of mipmap levels
---@param dimension integer Image view dimension
---@param format integer VkFormat enum value
---@param pImage lightuserdata TknImage pointer
---@return lightuserdata TknImageView pointer
function tkn.tknCreateImageView(pGfxContext, baseLayer, layerCount, aspectFlags, baseMipLevel, mipLevelCount, dimension, format, pImage) end

---Destroy an image view
---@param pGfxContext lightuserdata Graphics context pointer
---@param pImageView lightuserdata TknImageView pointer
function tkn.tknDestroyImageView(pGfxContext, pImageView) end

-- ============================================================================
-- Buffer Functions
-- ============================================================================

---Create a GPU buffer with optional initial data
---@param pGfxContext lightuserdata Graphics context pointer
---@param size integer Buffer size in bytes
---@param usage integer VkBufferUsageFlags combination
---@param mappedAtCreation boolean Whether buffer should be host-mappable
---@param pData string Binary buffer data or nil
---@return lightuserdata TknBuffer pointer
function tkn.tknCreateBufferPtr(pGfxContext, size, usage, mappedAtCreation, pData) end

---Destroy a GPU buffer
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBuffer lightuserdata TknBuffer pointer
function tkn.tknDestroyBufferPtr(pGfxContext, pBuffer) end

---Update buffer data
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBuffer lightuserdata TknBuffer pointer
---@param offset integer Offset in bytes
---@param size integer Size of data to write
---@param pData string Binary data or nil
function tkn.tknUpdateBuffer(pGfxContext, pBuffer, offset, size, pData) end

---Create a uniform buffer descriptor
---@param pBuffer lightuserdata TknBuffer pointer
---@param offset integer Offset in bytes
---@param range integer Size of uniform data in bytes
---@return lightuserdata TknUniformBuffer pointer
function tkn.tknCreateUniformBuffer(pBuffer, offset, range) end

---Destroy a uniform buffer descriptor
---@param pUniformBuffer lightuserdata TknUniformBuffer pointer
function tkn.tknDestroyUniformBuffer(pUniformBuffer) end

-- ============================================================================
-- Sampler Functions
-- ============================================================================

---Create a sampler for texture filtering and addressing
---@param pGfxContext lightuserdata Graphics context pointer
---@param magFilter integer VkFilter (NEAREST or LINEAR)
---@param minFilter integer VkFilter (NEAREST or LINEAR)
---@param mipmapMode integer VkSamplerMipmapMode (NEAREST or LINEAR)
---@param addressModeU integer VkSamplerAddressMode
---@param addressModeV integer VkSamplerAddressMode
---@param addressModeW integer VkSamplerAddressMode
---@param mipLodBias number Mipmap LOD bias
---@param anisotropyEnable boolean Enable anisotropic filtering
---@param maxAnisotropy number Maximum anisotropy level
---@param compareEnable boolean Enable comparison
---@param compareOp integer VkCompareOp
---@param minLod number Minimum LOD
---@param maxLod number Maximum LOD
---@param borderColor integer VkBorderColor
---@param unnormalizedCoordinates boolean Use unnormalized texture coordinates
---@return lightuserdata TknSampler pointer
function tkn.tknCreateSampler(pGfxContext, magFilter, minFilter, mipmapMode, addressModeU, addressModeV, addressModeW, mipLodBias, anisotropyEnable, maxAnisotropy, compareEnable, compareOp, minLod, maxLod, borderColor, unnormalizedCoordinates) end

---Destroy a sampler
---@param pGfxContext lightuserdata Graphics context pointer
---@param pSampler lightuserdata TknSampler pointer
function tkn.tknDestroySampler(pGfxContext, pSampler) end

-- ============================================================================
-- Binding Group Functions (Descriptor Sets)
-- ============================================================================

---Create a binding group layout from shader reflection
---@param pGfxContext lightuserdata Graphics context pointer
---@param shaderPaths table Array of compiled shader paths (SPIR-V)
---@param set integer Descriptor set index
---@return lightuserdata TknBindingGroupLayout pointer
function tkn.tknCreateBindingGroupLayout(pGfxContext, shaderPaths, set) end

---Destroy a binding group layout
---@param pGfxContext lightuserdata Graphics context pointer
---@param pLayout lightuserdata TknBindingGroupLayout pointer
function tkn.tknDestroyBindingGroupLayout(pGfxContext, pLayout) end

---Create a binding group (descriptor set) instance
---@param pGfxContext lightuserdata Graphics context pointer
---@param pLayout lightuserdata TknBindingGroupLayout pointer
---@param resourcePtrs table Array of resource pointers (TknBuffer, TknImageView, TknSampler, TknUniformBuffer)
---@return lightuserdata TknBindingGroup pointer
function tkn.tknCreateBindingGroup(pGfxContext, pLayout, resourcePtrs) end

---Destroy a binding group
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBindingGroup lightuserdata TknBindingGroup pointer
function tkn.tknDestroyBindingGroup(pGfxContext, pBindingGroup) end

---Update binding group resource bindings
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBindingGroup lightuserdata TknBindingGroup pointer
---@param indices table Array of binding indices
---@param resourcePtrs table Array of new resource pointers
function tkn.tknUpdateBindingGroup(pGfxContext, pBindingGroup, indices, resourcePtrs) end

-- ============================================================================
-- Pipeline Functions
-- ============================================================================

---Create a graphics rendering pipeline
---@param pGfxContext lightuserdata Graphics context pointer
---@param pRenderPassBindingGroupLayout lightuserdata TknBindingGroupLayout pointer
---@param spvPaths table Array of compiled shader paths (SPIR-V)
---@param pMeshVertexInputLayout table Mesh vertex attributes array ({location, format, offset})
---@param pInstanceVertexInputLayout table Instance vertex attributes array ({location, format, offset})
---@param primitiveState table Primitive state ({topology, polygonMode, cullMode, frontFace, depthBiasEnable, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor, lineWidth})
---@param fragmentState table Fragment state ({pColorAttachmentFormats, colorBlend={attachments, blendConstants}})
---@param multisampleState table Multisample state ({rasterizationSamples, alphaToCoverageEnable})
---@param depthStencilState table Depth/stencil state ({depthTestEnable, depthWriteEnable, depthCompareOp, stencilTestEnable, front, back})
---@param depthAttachmentFormat integer VkFormat for depth attachment
---@return lightuserdata TknPipeline pointer
function tkn.tknCreatePipelinePtr(pGfxContext, pRenderPassBindingGroupLayout, spvPaths, pMeshVertexInputLayout, pInstanceVertexInputLayout, primitiveState, fragmentState, multisampleState, depthStencilState, depthAttachmentFormat) end

---Destroy a graphics pipeline
---@param pGfxContext lightuserdata Graphics context pointer
---@param pPipeline lightuserdata TknPipeline pointer
function tkn.tknDestroyPipelinePtr(pGfxContext, pPipeline) end

-- ============================================================================
-- Command Buffer Functions
-- ============================================================================

---Begin recording render commands
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknBeginCommandBuffer(pGfxContext, width, height) end

---End recording render commands and submit to GPU
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknEndCommandBuffer(pGfxContext) end

-- ============================================================================
-- Render Pass Functions
-- ============================================================================

---Begin a render pass with attachments
---@param pGfxContext lightuserdata Graphics context pointer
---@param colorImageViewPtrs table Array of TknImageView pointers for color attachments
---@param loadOps table Array of VkAttachmentLoadOp values
---@param storeOps table Array of VkAttachmentStoreOp values
---@param colorClearValues table 2D array of clear values (RGBA) for each color attachment
---@param pDepthImageView lightuserdata TknImageView pointer for depth attachment or nil
---@param depthLoadOp integer VkAttachmentLoadOp
---@param depthStoreOp integer VkAttachmentStoreOp
---@param depthClearValue number Clear value for depth
---@param stencilClearValue integer Clear value for stencil
---@param width integer Render pass width
---@param height integer Render pass height
function tkn.tknBeginRenderPass(pGfxContext, colorImageViewPtrs, loadOps, storeOps, colorClearValues, pDepthImageView, depthLoadOp, depthStoreOp, depthClearValue, stencilClearValue, width, height) end

---End current render pass
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknEndRenderPass(pGfxContext) end

-- ============================================================================
-- Pipeline Binding Functions
-- ============================================================================

---Set the active pipeline and descriptor sets
---@param pGfxContext lightuserdata Graphics context pointer
---@param pPipeline lightuserdata TknPipeline pointer
---@param pRenderPassBindingGroup lightuserdata TknBindingGroup pointer for render pass descriptors
---@param pPipelineBindingGroup lightuserdata TknBindingGroup pointer for pipeline-specific descriptors
function tkn.tknSetPipelinePtr(pGfxContext, pPipeline, pRenderPassBindingGroup, pPipelineBindingGroup) end

-- ============================================================================
-- Buffer Binding Functions
-- ============================================================================

---Bind a buffer as vertex data
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBuffer lightuserdata TknBuffer pointer
---@param offset integer Offset in bytes
function tkn.tknBindVertexBuffer(pGfxContext, pBuffer, offset) end

---Bind a buffer as instance data
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBuffer lightuserdata TknBuffer pointer
---@param offset integer Offset in bytes
function tkn.tknBindInstanceBuffer(pGfxContext, pBuffer, offset) end

---Bind a buffer as index data
---@param pGfxContext lightuserdata Graphics context pointer
---@param pBuffer lightuserdata TknBuffer pointer
---@param indexType integer VkIndexType (UINT16 or UINT32)
---@param offset integer Offset in bytes
function tkn.tknBindIndexBuffer(pGfxContext, pBuffer, indexType, offset) end

-- ============================================================================
-- Draw Functions
-- ============================================================================

---Record a non-indexed draw call
---@param pGfxContext lightuserdata Graphics context pointer
---@param vertexCount integer Number of vertices to draw
---@param instanceCount integer Number of instances
---@param firstVertex integer Index of first vertex
---@param firstInstance integer Index of first instance
function tkn.tknDraw(pGfxContext, vertexCount, instanceCount, firstVertex, firstInstance) end

---Record an indexed draw call
---@param pGfxContext lightuserdata Graphics context pointer
---@param indexCount integer Number of indices to draw
---@param instanceCount integer Number of instances
---@param firstIndex integer Offset into index buffer
---@param baseVertex integer Base vertex offset
---@param firstInstance integer Index of first instance
function tkn.tknDrawIndexed(pGfxContext, indexCount, instanceCount, firstIndex, baseVertex, firstInstance) end

-- ============================================================================
-- Font Functions
-- ============================================================================

---Create a font library instance
---@return lightuserdata TknFontLibrary pointer
function tkn.tknCreateTknFontLibraryPtr() end

---Destroy a font library
---@param pTknFontLibrary lightuserdata TknFontLibrary pointer
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknDestroyTknFontLibraryPtr(pTknFontLibrary, pGfxContext) end

---Create a font from one or more font files
---@param pTknFontLibrary lightuserdata TknFontLibrary pointer
---@param pGfxContext lightuserdata Graphics context pointer
---@param fontPathCount integer Number of font files
---@param fontPaths table Array of font file paths (strings)
---@param fontSize integer Font size in pixels
---@param atlasLength integer Atlas texture size (width and height)
---@param boldStrengths table Array of bold strengths (FT_Pos, 26.6 format) or nil for no bold
---@return lightuserdata TknFont pointer
function tkn.tknCreateTknFontPtr(pTknFontLibrary, pGfxContext, fontPathCount, fontPaths, fontSize, atlasLength, boldStrengths) end

---Destroy a font
---@param pTknFontLibrary lightuserdata TknFontLibrary pointer
---@param pTknFont lightuserdata TknFont pointer
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknDestroyTknFontPtr(pTknFontLibrary, pTknFont, pGfxContext) end

---Load a character into font atlas
---@param pTknFont lightuserdata TknFont pointer
---@param unicode integer Unicode codepoint
---@return lightuserdata TknChar pointer
---@return boolean hasLoaded True if character was newly loaded
function tkn.tknLoadTknChar(pTknFont, unicode) end

---Flush dirty characters to GPU
---@param pTknFont lightuserdata TknFont pointer
---@param pGfxContext lightuserdata Graphics context pointer
function tkn.tknFlushTknFontPtr(pTknFont, pGfxContext) end

-- ============================================================================
-- Tickernel Context Functions (tkn module)
-- ============================================================================

---Create Tickernel context integrating graphics and Lua
---@param assetsPath string Path to assets directory (for shaders and Lua scripts)
---@param luaLibraryCount integer Number of Lua libraries to register
---@param luaLibraries table Array of {name, functions} Lua library definitions
---@param extensionCount integer Number of Vulkan extensions
---@param extensions table Array of extension names
---@param pSurface lightuserdata Native surface pointer
---@param width integer Window width
---@param height integer Window height
---@return lightuserdata TknContext pointer
function tknCreateContextPtr(assetsPath, luaLibraryCount, luaLibraries, extensionCount, extensions, pSurface, width, height) end

---Destroy Tickernel context
---@param pTknContext lightuserdata TknContext pointer
function tknDestroyContextPtr(pTknContext) end

---Update Tickernel context (call each frame)
---@param pTknContext lightuserdata TknContext pointer
---@param width integer Window width
---@param height integer Window height
---@param keyCodeStateCount integer Number of key states
---@param keyCodeStates table Array of InputState values for keys
---@param mouseCodeStateCount integer Number of mouse states
---@param mouseCodeStates table Array of InputState values for mouse buttons
---@param scrollingDeltaX number Horizontal scroll delta
---@param scrollingDeltaY number Vertical scroll delta
---@param mousePositionNDCX number Mouse X in NDC (-1 to 1)
---@param mousePositionNDCY number Mouse Y in NDC (-1 to 1)
---@param inputText string Unicode input text
---@param pShouldQuit table Reference to boolean: set to true to quit
---@param pImeEnabled table Reference to boolean: IME enabled state
function tknUpdateContext(pTknContext, width, height, keyCodeStateCount, keyCodeStates, mouseCodeStateCount, mouseCodeStates, scrollingDeltaX, scrollingDeltaY, mousePositionNDCX, mousePositionNDCY, inputText, pShouldQuit, pImeEnabled) end


-- Vertex format type constants
tkn = tkn or {}

return tkn

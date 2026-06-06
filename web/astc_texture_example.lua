-- ASTC Texture Loading Example
-- Demonstrates how to load ASTC compressed textures and upload to GPU using tknWriteImagePtr

-- ============================================================================
-- Helper: Load ASTC file and strip 16-byte header
-- ============================================================================
local function loadASTCTexture(filePath)
    local file = io.open(filePath, "rb")
    if not file then
        error("Failed to open ASTC file: " .. filePath)
    end
    
    -- Read 16-byte ASTC header (magic + metadata)
    local header = file:read(16)
    if not header or #header < 16 then
        error("Invalid ASTC file: header too short")
    end
    
    -- Verify magic number (0x5CA1AB13 in little-endian)
    local magic = string.unpack("<I", header)
    if magic ~= 0x5CA1AB13 then
        error("Invalid ASTC magic number: " .. string.format("0x%x", magic))
    end
    
    -- Parse block size and dimensions from header
    -- Bytes 4-6: block size (X, Y, Z)
    local blockSizeX = string.byte(header, 5)
    local blockSizeY = string.byte(header, 6)
    local blockSizeZ = string.byte(header, 7)
    
    -- Bytes 7-12: image dimensions (width, height, depth)
    local width = string.unpack("<I3", header:sub(8, 10))     -- 24-bit little-endian
    local height = string.unpack("<I3", header:sub(11, 13))
    local depth = string.unpack("<I3", header:sub(14, 16))
    
    print(string.format("ASTC Info: %dx%dx%d, Block: %dx%dx%d", 
                        width, height, depth, blockSizeX, blockSizeY, blockSizeZ))
    
    -- Read compressed texture data (everything after header)
    local textureData = file:read("*a")
    file:close()
    
    if not textureData then
        error("Failed to read ASTC texture data")
    end
    
    return {
        data = textureData,
        width = width,
        height = height,
        depth = depth,
        blockSizeX = blockSizeX,
        blockSizeY = blockSizeY,
        blockSizeZ = blockSizeZ,
    }
end

-- ============================================================================
-- Helper: Calculate ASTC compressed size
-- ============================================================================
local function getASTCCompressedSize(width, height, depth, blockSizeX, blockSizeY, blockSizeZ)
    -- Each ASTC block is always 16 bytes (128 bits)
    -- Calculate number of blocks needed
    local blocksX = math.ceil(width / blockSizeX)
    local blocksY = math.ceil(height / blockSizeY)
    local blocksZ = math.ceil(depth / blockSizeZ)
    
    local blockCount = blocksX * blocksY * blocksZ
    return blockCount * 16  -- Each block is 16 bytes
end

-- ============================================================================
-- Main Example: Load and upload ASTC texture
-- ============================================================================
local function uploadASTCTexture(pGfxContext, astcFilePath)
    print("Loading ASTC texture: " .. astcFilePath)
    
    -- Load ASTC file and strip header
    local astcInfo = loadASTCTexture(astcFilePath)
    
    -- Create GPU image for ASTC texture
    -- Format: VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 162 (Vulkan enum)
    local format = 162  -- VK_FORMAT_ASTC_4x4_UNORM_BLOCK
    local dimension = 1 -- VK_IMAGE_TYPE_2D
    local imageUsage = 0x00000004  -- VK_IMAGE_USAGE_TRANSFER_DST_BIT
    
    print(string.format("Creating GPU image: %dx%d", astcInfo.width, astcInfo.height))
    local pImage = tknGfx.tknCreateImagePtr(
        pGfxContext,
        dimension,
        format,
        1,                 -- mipLevelCount
        1,                 -- sampleCount
        astcInfo.width,
        astcInfo.height,
        1                  -- depth
    )
    
    if not pImage then
        error("Failed to create GPU image")
    end
    
    -- Upload ASTC data to GPU
    -- Note: astcInfo.data already has the 16-byte header removed
    print("Uploading texture data to GPU...")
    tknGfx.tknWriteImagePtr(
        pGfxContext,
        pImage,
        astcInfo.data,     -- Binary string with ASTC compressed data (no header)
        #astcInfo.data,    -- dataSize: actual byte size of compressed data
        astcInfo.width,
        astcInfo.height,
        1,                 -- depth
        0,                 -- mipLevel
        0,                 -- offsetX
        0,                 -- offsetY
        0                  -- offsetZ
    )
    
    print("ASTC texture uploaded successfully!")
    
    return pImage
end

-- ============================================================================
-- Example Usage in Main Program
-- ============================================================================
-- Uncomment and adapt this to your application:
--[[
function setupGraphics()
    -- Create graphics context
    local extensions = {"VK_KHR_swapchain"}
    local pGfxContext = tknGfx.tknCreateGfxContextPtr(
        #extensions,
        extensions,
        nil,           -- pSurface (would be WebGPU surface in real app)
        800,           -- width
        600            -- height
    )
    
    -- Load and upload ASTC texture
    local pASTCImage = uploadASTCTexture(pGfxContext, "assets/textures/sample.astc")
    
    -- Create image view for sampling
    local pImageView = tknGfx.tknCreateImageView(
        pGfxContext,
        0,             -- baseLayer
        1,             -- layerCount
        0x00000001,    -- VK_IMAGE_ASPECT_COLOR_BIT
        0,             -- baseMipLevel
        1,             -- mipLevelCount
        1,             -- VK_IMAGE_VIEW_TYPE_2D
        162,           -- VK_FORMAT_ASTC_4x4_UNORM_BLOCK
        pASTCImage
    )
    
    -- Now use pImageView for rendering...
    
    return pGfxContext, pASTCImage, pImageView
end

-- In cleanup:
function cleanupGraphics(pGfxContext, pImage, pImageView)
    tknGfx.tknDestroyImageView(pGfxContext, pImageView)
    tknGfx.tknDestroyImagePtr(pGfxContext, pImage)
    tknGfx.tknDestroyGfxContextPtr(pGfxContext)
end
--]]

-- ============================================================================
-- Vulkan Format Constants (for reference)
-- ============================================================================
local VulkanFormats = {
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 162,
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 166,
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 170,
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 174,
    VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 178,
}

local VulkanImageTypes = {
    VK_IMAGE_TYPE_1D = 0,
    VK_IMAGE_TYPE_2D = 1,
    VK_IMAGE_TYPE_3D = 2,
}

local VulkanImageUsageFlags = {
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
}

return {
    loadASTCTexture = loadASTCTexture,
    getASTCCompressedSize = getASTCCompressedSize,
    uploadASTCTexture = uploadASTCTexture,
    VulkanFormats = VulkanFormats,
    VulkanImageTypes = VulkanImageTypes,
    VulkanImageUsageFlags = VulkanImageUsageFlags,
}

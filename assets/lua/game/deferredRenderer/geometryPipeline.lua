local vulkan = require("vulkan")
local tkn = require("tkn")
local geometryPipeline = {}
function geometryPipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
    local geometryPipelineSpvPaths = {assetsPath .. "/shaders/opaqueGeometry.vert.spv", assetsPath .. "/shaders/opaqueGeometry.frag.spv"}
    
    -- Geometry pass: 2 color attachments (albedo + normal) + depth
    return tkn.tknCreatePipelinePtr(
        pTknGfxContext,
        2,  -- colorAttachmentCount (albedo, normal)
        {vulkan.VK_FORMAT_R8G8B8A8_UNORM, vulkan.VK_FORMAT_A8B8G8R8_UNORM_PACK32},  -- pColorAttachmentFormats
        vulkan.VK_FORMAT_D32_SFLOAT,  -- depthAttachmentFormat
        nil,  -- pRenderPassBindingGroupLayout
        2,  -- spvPathCount
        geometryPipelineSpvPaths,  -- spvPaths
        nil,  -- pMeshVertexInputLayout
        nil,  -- pInstanceVertexInputLayout
        nil,  -- pVkPipelineInputAssemblyStateCreateInfo
        nil,  -- pVkPipelineViewportStateCreateInfo
        nil,  -- pVkPipelineRasterizationStateCreateInfo
        nil,  -- pVkPipelineMultisampleStateCreateInfo
        nil,  -- pVkPipelineDepthStencilStateCreateInfo
        nil,  -- pVkPipelineColorBlendStateCreateInfo
        nil  -- pVkPipelineDynamicStateCreateInfo
    )
end

function geometryPipeline.destroyPipelinePtr(pTknGfxContext, pTknPipeline)
    tkn.tknDestroyPipelinePtr(pTknGfxContext, pTknPipeline)
end

return geometryPipeline

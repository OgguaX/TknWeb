local vulkan = require("vulkan")
local tkn = require("tkn")
local imagePipeline = {}

function imagePipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
    local imagePipelineSpvPaths = {assetsPath .. "/shaders/ui.vert.spv", assetsPath .. "/shaders/image.frag.spv"}
    
    -- Create with new low-level API signature (16 parameters):
    return tkn.tknCreatePipelinePtr(
        pTknGfxContext,
        1,  -- colorAttachmentCount
        {vulkan.VK_FORMAT_B8G8R8A8_UNORM},  -- pColorAttachmentFormats
        vulkan.VK_FORMAT_D32_SFLOAT,  -- depthAttachmentFormat
        nil,  -- pRenderPassBindingGroupLayout
        2,  -- spvPathCount
        imagePipelineSpvPaths,  -- spvPaths
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

function imagePipeline.destroyPipelinePtr(pTknGfxContext, pTknPipeline)
    tkn.tknDestroyPipelinePtr(pTknGfxContext, pTknPipeline)
end

return imagePipeline

local vulkan = require("vulkan")
local tkn = require("tkn")
local textPipeline = {}

function textPipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
    local textPipelineSpvPaths = {assetsPath .. "/shaders/ui.vert.spv", assetsPath .. "/shaders/text.frag.spv"}
    
    -- Create with new low-level API signature (16 parameters):
    return tkn.tknCreatePipelinePtr(
        pTknGfxContext,
        1,  -- colorAttachmentCount
        {vulkan.VK_FORMAT_B8G8R8A8_UNORM},  -- pColorAttachmentFormats
        vulkan.VK_FORMAT_D32_SFLOAT,  -- depthAttachmentFormat
        nil,  -- pRenderPassBindingGroupLayout
        2,  -- spvPathCount
        textPipelineSpvPaths,  -- spvPaths
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

function textPipeline.destroyPipelinePtr(pTknGfxContext, pTknPipeline)
    tkn.tknDestroyPipelinePtr(pTknGfxContext, pTknPipeline)
end

return textPipeline

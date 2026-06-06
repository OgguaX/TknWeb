local vulkan = require("vulkan")
local tkn = require("tkn")
local lightingPipeline = {}
function lightingPipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
    local lightingPipelineSpvPaths = {assetsPath .. "/shaders/opaqueLighting.vert.spv", assetsPath .. "/shaders/opaqueLighting.frag.spv"}

    -- Lighting pass: 1 color attachment (output), no depth
    return tkn.tknCreatePipelinePtr(
        pTknGfxContext,
        1,  -- colorAttachmentCount
        {vulkan.VK_FORMAT_B8G8R8A8_UNORM},  -- pColorAttachmentFormats
        0,  -- depthAttachmentFormat (0 means no depth)
        nil,  -- pRenderPassBindingGroupLayout
        2,  -- spvPathCount
        lightingPipelineSpvPaths,  -- spvPaths
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

function lightingPipeline.destroyPipelinePtr(pTknGfxContext, pTknPipeline)
    tkn.tknDestroyPipelinePtr(pTknGfxContext, pTknPipeline)
end

return lightingPipeline

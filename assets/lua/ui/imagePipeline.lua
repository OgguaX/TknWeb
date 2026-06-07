local vulkan = require("vulkan")
local tkn = require("tkn")
local imagePipeline = {}

function imagePipeline.createPipelinePtr(pTknGfxContext, meshVertexInputLayout, instanceVertexInputLayout, assetsPath)
    local imagePipelineSpvPaths = {assetsPath .. "/shaders/ui.vert.spv", assetsPath .. "/shaders/image.frag.spv"}

    return tkn.tknCreatePipelinePtr(pTknGfxContext, nil, imagePipelineSpvPaths, meshVertexInputLayout, instanceVertexInputLayout, {
        topology = vulkan.VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        polygonMode = vulkan.VK_POLYGON_MODE_FILL,
        cullMode = vulkan.VK_CULL_MODE_BACK_BIT,
        frontFace = vulkan.VK_FRONT_FACE_COUNTER_CLOCKWISE,
        depthBiasEnable = false,
        depthBiasConstantFactor = 0.0,
        depthBiasClamp = 0.0,
        depthBiasSlopeFactor = 0.0,
        lineWidth = 1.0,
    }, {
        pColorAttachmentFormats = {vulkan.VK_FORMAT_B8G8R8A8_UNORM},
        colorBlend = {
            attachments = {},
            blendConstants = {0.0, 0.0, 0.0, 0.0},
        },
    }, {
        rasterizationSamples = vulkan.VK_SAMPLE_COUNT_1_BIT,
        alphaToCoverageEnable = false,
    }, {
        depthTestEnable = true,
        depthWriteEnable = true,
        depthCompareOp = vulkan.VK_COMPARE_OP_LESS,
        stencilTestEnable = false,
        front = {
            failOp = 0,
            passOp = 0,
            depthFailOp = 0,
            compareOp = 0,
            compareMask = 0,
            writeMask = 0,
            reference = 0,
        },
        back = {
            failOp = 0,
            passOp = 0,
            depthFailOp = 0,
            compareOp = 0,
            compareMask = 0,
            writeMask = 0,
            reference = 0,
        },
    }, vulkan.VK_FORMAT_D32_SFLOAT -- depthAttachmentFormat
    )
end

function imagePipeline.destroyPipelinePtr(pTknGfxContext, pTknPipeline)
    tkn.tknDestroyPipelinePtr(pTknGfxContext, pTknPipeline)
end

return imagePipeline

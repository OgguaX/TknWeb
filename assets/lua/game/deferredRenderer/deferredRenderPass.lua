local tkn = require("tkn")
local vulkan = require("vulkan")
local geometryPipeline = require("game.deferredRenderer.geometryPipeline")
local lightingPipeline = require("game.deferredRenderer.lightingPipeline")
local deferredRenderPass = {}

function deferredRenderPass.setup(pTknGfxContext, assetsPath, renderPassIndex, pDepthStencilAttachment, pSwapchainAttachment)
    -- Vertex format for voxel meshes
    deferredRenderPass.vertexFormat = {{
        name = "position",
        type = tkn.type.float,
        count = 3,
    }, {
        name = "color",
        type = tkn.type.uint32,
        count = 1,
    }, {
        name = "normal",
        type = tkn.type.uint32,
        count = 1,
    }, {
        name = "pbr",
        type = tkn.type.uint32,
        count = 1,
    }}

    -- Instance format for model matrices
    deferredRenderPass.instanceFormat = {{
        name = "model",
        type = tkn.type.float,
        count = 16,
    }}

    -- Create pipelines with new low-level API
    deferredRenderPass.pGeometryPipeline = geometryPipeline.createPipelinePtr(pTknGfxContext, deferredRenderPass.vertexFormat, deferredRenderPass.instanceFormat, assetsPath)
    deferredRenderPass.pLightingPipeline = lightingPipeline.createPipelinePtr(pTknGfxContext, deferredRenderPass.vertexFormat, deferredRenderPass.instanceFormat, assetsPath)
    
    -- Store attachments for render pass
    deferredRenderPass.pSwapchainAttachment = pSwapchainAttachment
    deferredRenderPass.pDepthStencilAttachment = pDepthStencilAttachment
end

function deferredRenderPass.teardown(pTknGfxContext)
    geometryPipeline.destroyPipelinePtr(pTknGfxContext, deferredRenderPass.pGeometryPipeline)
    lightingPipeline.destroyPipelinePtr(pTknGfxContext, deferredRenderPass.pLightingPipeline)
    
    deferredRenderPass.pGeometryPipeline = nil
    deferredRenderPass.pLightingPipeline = nil
    deferredRenderPass.pSwapchainAttachment = nil
    deferredRenderPass.pDepthStencilAttachment = nil
end

return deferredRenderPass

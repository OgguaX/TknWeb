local tkn = require("tkn")
local vulkan = require("vulkan")
local imagePipeline = require("ui.imagePipeline")
local textPipeline = require("ui.textPipeline")
local uiRenderPass = {}

function uiRenderPass.setup(pTknGfxContext, pSwapchainAttachment, pDepthStencilAttachment, assetsPath, vertexFormat, instanceFormat, renderPassIndex)
    -- Store attachments for later use in render pass
    uiRenderPass.pSwapchainAttachment = pSwapchainAttachment
    uiRenderPass.pDepthStencilAttachment = pDepthStencilAttachment
    
    -- Create pipelines (will be adapted for new API)
    uiRenderPass.pImagePipeline = imagePipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
    uiRenderPass.pTextPipeline = textPipeline.createPipelinePtr(pTknGfxContext, vertexFormat, instanceFormat, assetsPath)
end

function uiRenderPass.beginRenderPass(pTknGfxContext, pTknFrame)
    -- Begin render pass with new low-level API
    -- Parameters: context, colorCount, colorViews[], loadOps[], storeOps[], clearValues[], depthView, depthLoadOp, depthStoreOp, depthClear, stencilClear, width, height
    tkn.tknBeginRenderPass(
        pTknGfxContext,
        1,  -- colorAttachmentCount
        {uiRenderPass.pSwapchainAttachment},  -- colorImageViewPtrs
        {vulkan.VK_ATTACHMENT_LOAD_OP_LOAD},  -- loadOps
        {vulkan.VK_ATTACHMENT_STORE_OP_STORE},  -- storeOps
        {{0.0, 0.0, 0.0, 1.0}},  -- colorClearValues (2D array)
        uiRenderPass.pDepthStencilAttachment,  -- pDepthImageView
        vulkan.VK_ATTACHMENT_LOAD_OP_CLEAR,  -- depthLoadOp
        vulkan.VK_ATTACHMENT_STORE_OP_DONT_CARE,  -- depthStoreOp
        1.0,  -- depthClearValue
        0,  -- stencilClearValue
        800,  -- width (placeholder - should be from camera/context)
        600  -- height (placeholder - should be from camera/context)
    )
end

function uiRenderPass.endRenderPass(pTknGfxContext, pTknFrame)
    -- End render pass
    tkn.tknEndRenderPass(pTknGfxContext)
end

function uiRenderPass.teardown(pTknGfxContext)
    textPipeline.destroyPipelinePtr(pTknGfxContext, uiRenderPass.pTextPipeline)
    imagePipeline.destroyPipelinePtr(pTknGfxContext, uiRenderPass.pImagePipeline)
    uiRenderPass.pSwapchainAttachment = nil
    uiRenderPass.pDepthStencilAttachment = nil
    uiRenderPass.pImagePipeline = nil
    uiRenderPass.pTextPipeline = nil
end

return uiRenderPass

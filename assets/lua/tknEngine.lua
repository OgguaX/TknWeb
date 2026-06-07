local tkn = require("tkn")
local vulkan = require("vulkan")
local ui = require("ui.ui")
local input = require("input")
local tknWidgetConfig = require("engine.widgets.tknWidgetConfig")
local editorPanel = require("engine.panels.editorPanel")
local tknScrollViewWidget = require("engine.widgets.tknScrollViewWidget")
local tknInputFieldWidget = require("engine.widgets.tknInputFieldWidget")
local tknEngine = {}

function tknEngine.start(pTknGfxContext, assetsPath)
    tknEngine.frameCount = 0
    tknEngine.assetsPath = assetsPath
    ui.setup(pTknGfxContext, assetsPath, 1)
    tknWidgetConfig.setup(pTknGfxContext, assetsPath)
    tknEngine.editorRootUINode = ui.addNode(pTknGfxContext, ui.rootNode, 1, "Editor", tknWidgetConfig.fullRelativeOrientation, tknWidgetConfig.fullRelativeOrientation, tknWidgetConfig.defaultTransform)
    tknEngine.editorPanel = editorPanel.create(pTknGfxContext, tknEngine.editorRootUINode)
end

function tknEngine.stop(pTknGfxContext)
    tkn.tknWaitRenderFence(pTknGfxContext)
    editorPanel.destroy(pTknGfxContext, tknEngine.editorPanel)
    ui.removeNode(pTknGfxContext, tknEngine.editorRootUINode)
    tknWidgetConfig.teardown(pTknGfxContext)
    ui.teardown(pTknGfxContext)
    tknEngine.frameCount = nil
end

function tknEngine.update(pTknGfxContext, width, height)
    tknEngine.frameCount = tknEngine.frameCount + 1
    tkn.tknWaitRenderFence(pTknGfxContext)
    ui.update(pTknGfxContext, width, height)
    tknScrollViewWidget.update()
    tknInputFieldWidget.update(tknEngine.frameCount)
    return false
end

_G.tknEngine = tknEngine
return tknEngine

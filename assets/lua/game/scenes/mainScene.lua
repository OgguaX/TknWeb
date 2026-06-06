local mainScene = {}
local mainPanel = require("game.panels.mainPanel")
local characterSystem = require("game.characterSystem")
local transformSystem = require("game.transformSystem")
local tkn = require("tkn")

function mainScene.start(pTknGfxContext, game)
    mainScene.mainPanel = mainPanel.create(pTknGfxContext, game, game.rootUINode)
    transformSystem.update()
    characterSystem.updateInstances(pTknGfxContext)
end

function mainScene.stop(game)
end

function mainScene.stopGfx(game, pTknGfxContext)
    mainPanel.destroy(pTknGfxContext, mainScene.mainPanel)
end

function mainScene.update(game)
end

function mainScene.updateGfx(game, pTknGfxContext, width, height)
end

function mainScene.recordFrame(game, pTknGfxContext, pTknFrame)
    -- TODO: Implement new low-level rendering API calls
    -- In new API, would bind vertex buffer, instance buffer, and call tknDraw/tknDrawIndexed
    -- for _, char in ipairs(characterSystem.characters) do
    --     tkn.tknBindVertexBuffer(pTknGfxContext, char.body.pVertexBuffer, 0)
    --     tkn.tknDraw(pTknGfxContext, char.body.vertexCount, 1, 0, 0)
    -- end
end

return mainScene

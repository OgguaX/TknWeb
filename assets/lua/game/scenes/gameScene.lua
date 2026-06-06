local gameScene = {}
local wildMap = require("game.maps.wildMap")
local characterSystem = require("game.characterSystem")
local transformSystem = require("game.transformSystem")
local tkn = require("tkn")

function gameScene.start(pTknGfxContext, game)
    gameScene.map = wildMap.create(pTknGfxContext)
    gameScene.player = characterSystem.add(pTknGfxContext, 1, 1, 0, 42)
    transformSystem.update()
    characterSystem.updateInstances(pTknGfxContext)
end

function gameScene.stop(game)
end

function gameScene.stopGfx(game, pTknGfxContext)
    wildMap.destroy(pTknGfxContext, gameScene.map)
    gameScene.map = nil
end

function gameScene.update(game)
end

function gameScene.updateGfx(game, pTknGfxContext, width, height)
end

function gameScene.recordFrame(game, pTknGfxContext, pTknFrame)
    -- TODO: Implement new low-level rendering API calls
    -- In new API, would bind vertex buffer, instance buffer, and call tknDraw/tknDrawIndexed
    -- if gameScene.map then
    --     tkn.tknBindVertexBuffer(pTknGfxContext, gameScene.map.pVertexBuffer, 0)\n    --     tkn.tknDraw(pTknGfxContext, gameScene.map.vertexCount, 1, 0, 0)
    -- end
end

return gameScene

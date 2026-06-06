local tknMath = require("tknMath")
local tkn = require("tkn")
local deferredRenderPass = require("game.deferredRenderer.deferredRenderPass")
local voxelConfig = require("game.voxelConfig")
local vulkan = require("vulkan")
local groundSystem = {}

function groundSystem.setup(voxelPerMeter)
    groundSystem.ground = {
        snow = 1,
        ice = 2,
        sand = 3,
        grass = 4,
        water = 5,
        lava = 6,
        volcanic = 7,
    }

    groundSystem.groundConfig = {
        [1] = {
            name = "snow",
            temperature = -1,
            humidity = 0,
            temperatureVariance = 0.15,
            humidityVariance = 0.15,
        },
        [2] = {
            name = "ice",
            temperature = -1,
            humidity = 1,
            temperatureVariance = 0.15,
            humidityVariance = 0.12,
        },
        [3] = {
            name = "sand",
            temperature = 0,
            humidity = -1,
            temperatureVariance = 0.18,
            humidityVariance = 0.15,
        },
        [4] = {
            name = "grass",
            temperature = 0,
            humidity = 0,
            temperatureVariance = 0.18,
            humidityVariance = 0.18,
        },
        [5] = {
            name = "water",
            temperature = 0,
            humidity = 1,
            temperatureVariance = 0.15,
            humidityVariance = 0.12,
        },
        [6] = {
            name = "lava",
            temperature = 1,
            humidity = -1,
            temperatureVariance = 0.15,
            humidityVariance = 0.12,
        },
        [7] = {
            name = "volcanic",
            temperature = 1,
            humidity = 0,
            temperatureVariance = 0.15,
            humidityVariance = 0.15,
        },
    }

    groundSystem.temperatureStep = 0.27
    groundSystem.humidityStep = 0.27

    groundSystem.voxelPerMeter = voxelPerMeter
end

function groundSystem.teardown()
    groundSystem.temperatureStep = nil
    groundSystem.humidityStep = nil
    groundSystem.temperatureNoiseScale = nil
    groundSystem.humidityNoiseScale = nil
    groundSystem.voxelPerMeter = nil

    groundSystem.ground = nil
    groundSystem.groundConfig = nil
end

function groundSystem.getGround(temperature, humidity)
    local result
    if temperature < -groundSystem.temperatureStep then
        if humidity < -groundSystem.humidityStep then
            result = groundSystem.ground.snow
        elseif humidity < groundSystem.humidityStep then
            result = groundSystem.ground.snow
        else
            result = groundSystem.ground.ice
        end
    elseif temperature < groundSystem.temperatureStep then
        if humidity < -groundSystem.humidityStep then
            result = groundSystem.ground.sand
        elseif humidity < groundSystem.humidityStep then
            result = groundSystem.ground.grass
        else
            result = groundSystem.ground.water
        end
    else
        if humidity < -groundSystem.humidityStep then
            result = groundSystem.ground.volcanic
        elseif humidity < groundSystem.humidityStep then
            result = groundSystem.ground.lava
        else
            result = groundSystem.ground.lava
        end
    end
    return result
end

function groundSystem.buildVoxelMap(groundMap)
    -- Build voxel map based on height data
    for x = 1, groundMap.length * groundSystem.voxelPerMeter do
        if not groundMap.voxelMap[x] then
            groundMap.voxelMap[x] = {}
        end
        for y = 1, groundMap.width * groundSystem.voxelPerMeter do
            if not groundMap.voxelMap[x][y] then
                groundMap.voxelMap[x][y] = {}
            end
            
            local gx = math.floor((x - 1) / groundSystem.voxelPerMeter) + 1
            local gy = math.floor((y - 1) / groundSystem.voxelPerMeter) + 1
            local ground = groundMap.groundMap[gx][gy]
            
            -- Get voxel height based on ground type
            local height = math.floor(10 + tknMath.perlinNoise2D(groundMap.seed, x * 0.1, y * 0.1) * 5)
            
            for z = 1, height do
                local groundConfig = groundSystem.groundConfig[ground]
                if groundConfig then
                    groundMap.voxelMap[x][y][z] = voxelConfig[groundConfig.name] or voxelConfig.grass
                end
            end
        end
    end
end

local function calculateNormal(voxelMap, x, y, z)
    -- Simplified normal calculation: returns 0-255 based on neighboring voxels
    return 0
end

function groundSystem.createMap(groundLength, groundWidth, seed)
    -- Build ground map
    local groundMap = {
        seed = seed,
        temperatureSeed = seed + 1,
        humiditySeed = seed + 2,
        length = groundLength,
        width = groundWidth,
        groundMap = {},
        voxelMap = {},
    }

    for x = 1, groundLength do
        if not groundMap.groundMap[x] then
            groundMap.groundMap[x] = {}
        end
        for y = 1, groundWidth do
            local temperature = tknMath.perlinNoise2D(groundMap.temperatureSeed, x * groundSystem.temperatureStep, y * groundSystem.temperatureStep)
            local humidity = tknMath.perlinNoise2D(groundMap.humiditySeed, x * groundSystem.humidityStep, y * groundSystem.humidityStep)
            local ground = groundSystem.getGround(temperature, humidity)
            groundMap.groundMap[x][y] = ground
        end
    end

    groundSystem.buildVoxelMap(groundMap)
    return groundMap
end

function groundSystem.destroyMap(groundMap)
    groundMap.seed = nil
    groundMap.temperatureSeed = nil
    groundMap.humiditySeed = nil
    groundMap.length = nil
    groundMap.width = nil
    groundMap.groundMap = nil
    groundMap.voxelMap = nil
end

-- Helper: Pack vertex data to binary format
local function packVertexData(vertices, vertexCount)
    local packed = ""
    for i = 1, vertexCount do
        -- position (3 floats)
        packed = packed .. string.pack("f", vertices.position[(i-1)*3 + 1] or 0)
        packed = packed .. string.pack("f", vertices.position[(i-1)*3 + 2] or 0)
        packed = packed .. string.pack("f", vertices.position[(i-1)*3 + 3] or 0)
        -- color (1 uint32)
        packed = packed .. string.pack("I4", vertices.color[i] or 0)
        -- normal (1 uint32)
        packed = packed .. string.pack("I4", vertices.normal[i] or 0)
        -- pbr (1 uint32)
        packed = packed .. string.pack("I4", vertices.pbr[i] or 0)
    end
    return packed
end

-- Helper: Pack instance data to binary format
local function packInstanceData(scale, offsetX, offsetY, offsetZ)
    local packed = ""
    -- Pack 4x4 model matrix (column-major order)
    -- Column 0: [scale, 0, 0, 0]
    -- Column 1: [0, scale, 0, 0]
    -- Column 2: [0, 0, scale, 0]
    -- Column 3: [offsetX, offsetY, offsetZ, 1]
    local matrix = {
        scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        offsetX, offsetY, offsetZ, 1,
    }
    for _, v in ipairs(matrix) do
        packed = packed .. string.pack("f", v)
    end
    return packed
end

function groundSystem.createMesh(pTknGfxContext, groundMap)
    local vertices = {
        position = {},
        color = {},
        normal = {},
        pbr = {},
    }
    local voxelPerMeter = groundSystem.voxelPerMeter
    for x = 1, groundMap.length * groundSystem.voxelPerMeter do
        for y = 1, groundMap.width * groundSystem.voxelPerMeter do
            for z = 1, #groundMap.voxelMap[x][y], 1 do
                local voxel = groundMap.voxelMap[x][y][z]
                if voxel then
                    table.insert(vertices.position, x)
                    table.insert(vertices.position, y)
                    table.insert(vertices.position, z)
                    table.insert(vertices.color, tknMath.rgbaToAbgr(voxel.color))
                    local normal = calculateNormal(groundMap.voxelMap, x, y, z)
                    table.insert(vertices.normal, normal)
                    local pbr = (voxel.emissive & 0xF) | ((voxel.roughness & 0xF) << 4) | ((voxel.metallic & 0xF) << 8)
                    table.insert(vertices.pbr, pbr)
                end
            end
        end
    end
    
    local vertexCount = #vertices.color
    local packed = packVertexData(vertices, vertexCount)
    
    -- Create vertex buffer (NEW API)
    local pVertexBuffer = tkn.tknCreateBufferPtr(
        pTknGfxContext,
        math.max(#packed, 1),
        vulkan.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        false,
        packed ~= "" and packed or "\0"
    )
    
    local scale = 1.0 / groundSystem.voxelPerMeter
    local instancePacked = packInstanceData(scale, 0.5, 0.5, scale * -4)
    
    -- Create instance buffer (NEW API)
    local pInstanceBuffer = tkn.tknCreateBufferPtr(
        pTknGfxContext,
        #instancePacked,
        vulkan.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        false,
        instancePacked
    )
    
    -- Return mesh data (no DrawCall object in new API)
    return {
        pVertexBuffer = pVertexBuffer,
        vertexCount = vertexCount,
        pInstanceBuffer = pInstanceBuffer,
        instanceCount = 1,
    }
end

function groundSystem.destroyMesh(pTknGfxContext, mesh)
    if mesh and mesh.pVertexBuffer then
        tkn.tknDestroyBufferPtr(pTknGfxContext, mesh.pVertexBuffer)
    end
    if mesh and mesh.pInstanceBuffer then
        tkn.tknDestroyBufferPtr(pTknGfxContext, mesh.pInstanceBuffer)
    end
end

return groundSystem

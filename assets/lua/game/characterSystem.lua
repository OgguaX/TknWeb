local tkn = require("tkn")
local tknVoxel = require("game.tknVoxel")
local tknMath = require("tknMath")
local voxelConfig = require("game.voxelConfig")
local deferredRenderPass = require("game.deferredRenderer.deferredRenderPass")
local transformSystem = require("game.transformSystem")
local vulkan = require("vulkan")
local characterSystem = {}

function characterSystem.setup(assetsPath, voxelPerMeter)
    characterSystem.assetPath = assetsPath
    characterSystem.scale = 1.0 / voxelPerMeter
    characterSystem.characters = {}
    characterSystem.bodyTvox = nil
    characterSystem.maskTvox = nil
    characterSystem.mask = nil
end

function characterSystem.teardown()
    characterSystem.assetPath = nil
    characterSystem.scale = nil
    characterSystem.characters = nil
    characterSystem.bodyTvox = nil
    characterSystem.maskTvox = nil
    characterSystem.mask = nil
end

-- Helper: Pack vertex data to binary format
-- Returns packed binary string
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

local function createBody(pTknGfxContext, seed)
    local tvox = characterSystem.bodyTvox
    local pivotOffsetX = tvox.sizeX * 0.5
    local pivotOffsetY = tvox.sizeY * 0.5
    local pivotOffsetZ = 0

    local darkWood = voxelConfig.darkWood
    local wood = voxelConfig.wood
    local lightWood = voxelConfig.lightWood

    local vertices = {
        position = {},
        color = {},
        normal = {},
        pbr = {},
    }

    for _, record in ipairs(tvox.records) do
        local noise = tknMath.perlinNoise3D(seed, record.x * 0.27, record.y * 0.27, record.z * 3)
        local mat
        if noise < -0.4 then
            mat = darkWood
        elseif noise < 0.4 then
            mat = wood
        else
            mat = lightWood
        end
        local rgba = mat.color
        local r = (rgba >> 24) & 0xFF
        local g = (rgba >> 16) & 0xFF
        local b = (rgba >> 8) & 0xFF
        local a = rgba & 0xFF
        local color = r | (g << 8) | (b << 16) | (a << 24)
        table.insert(vertices.position, record.x - pivotOffsetX)
        table.insert(vertices.position, record.y - pivotOffsetY)
        table.insert(vertices.position, record.z - pivotOffsetZ)
        table.insert(vertices.color, color)
        table.insert(vertices.normal, record.normal)
        local pbr = (mat.emissive & 0xF) | ((mat.roughness & 0xF) << 4) | ((mat.metallic & 0xF) << 8)
        table.insert(vertices.pbr, pbr)
    end

    local vertexCount = #tvox.records
    local packed = packVertexData(vertices, vertexCount)
    
    -- Create vertex buffer (NEW API)
    local pVertexBuffer = tkn.tknCreateBufferPtr(
        pTknGfxContext,
        math.max(#packed, 1),
        vulkan.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        false,
        packed ~= "" and packed or "\0"
    )

    -- Store mesh data (no DrawCall object in new API)
    return {
        pVertexBuffer = pVertexBuffer,
        vertexCount = vertexCount,
        instanceBuffer = nil,
        instanceCount = 1,
    }
end

local function createMask(pTknGfxContext, seed)
    local tvox = characterSystem.maskTvox
    local pivotOffsetX = tvox.sizeX * 0.5
    local pivotOffsetY = 0
    local pivotOffsetZ = 0

    local darkGrass = voxelConfig.darkGrass
    local grass = voxelConfig.grass
    local lightGrass = voxelConfig.lightGrass

    -- Build occupancy set from original records
    local occupied = {}
    local function key(x, y, z) return x .. "," .. y .. "," .. z end
    for _, record in ipairs(tvox.records) do
        occupied[key(record.x, record.y, record.z)] = true
    end

    -- Collect extra voxels by expanding from boundary faces (top, left, right)
    -- Only expand outside the original bounding box to preserve internal holes (eyes)
    local maxX = tvox.sizeX - 1
    local maxZ = tvox.sizeZ - 1
    local extras = {}
    for _, record in ipairs(tvox.records) do
        -- Top expansion (z+): only from top boundary voxels
        if record.z >= maxZ and not occupied[key(record.x, record.y, record.z + 1)] then
            local n = tknMath.perlinNoise3D(seed + 1, record.x * 0.35, record.y * 0.35, record.z * 0.35)
            if n > 0.0 then
                local reach = math.floor(n * 4) + 1
                for dz = 1, reach do
                    local nz = record.z + dz
                    local k = key(record.x, record.y, nz)
                    if not occupied[k] then
                        occupied[k] = true
                        extras[#extras + 1] = { x = record.x, y = record.y, z = nz, normal = record.normal }
                    end
                end
            end
        end
        -- Left expansion (x-): only from left boundary
        if record.x <= 0 and not occupied[key(record.x - 1, record.y, record.z)] then
            local n = tknMath.perlinNoise3D(seed + 2, record.x * 0.35, record.y * 0.35, record.z * 0.35)
            if n > 0.0 then
                local reach = math.floor(n * 4) + 1
                for dx = 1, reach do
                    local nx = record.x - dx
                    local k = key(nx, record.y, record.z)
                    if not occupied[k] then
                        occupied[k] = true
                        extras[#extras + 1] = { x = nx, y = record.y, z = record.z, normal = record.normal }
                    end
                end
            end
        end
        -- Right expansion (x+): only from right boundary
        if record.x >= maxX and not occupied[key(record.x + 1, record.y, record.z)] then
            local n = tknMath.perlinNoise3D(seed + 3, record.x * 0.35, record.y * 0.35, record.z * 0.35)
            if n > 0.0 then
                local reach = math.floor(n * 4) + 1
                for dx = 1, reach do
                    local nx = record.x + dx
                    local k = key(nx, record.y, record.z)
                    if not occupied[k] then
                        occupied[k] = true
                        extras[#extras + 1] = { x = nx, y = record.y, z = record.z, normal = record.normal }
                    end
                end
            end
        end
    end

    local vertices = {
        position = {},
        color = {},
        normal = {},
        pbr = {},
    }

    -- Add original records
    for _, record in ipairs(tvox.records) do
        local noise = tknMath.perlinNoise3D(seed, record.x * 0.27, record.y * 0.27, record.z * 3)
        local mat
        if noise < -0.4 then
            mat = darkGrass
        elseif noise < 0.4 then
            mat = grass
        else
            mat = lightGrass
        end
        local rgba = mat.color
        local r = (rgba >> 24) & 0xFF
        local g = (rgba >> 16) & 0xFF
        local b = (rgba >> 8) & 0xFF
        local a = rgba & 0xFF
        local color = r | (g << 8) | (b << 16) | (a << 24)
        table.insert(vertices.position, record.x - pivotOffsetX)
        table.insert(vertices.position, record.y - pivotOffsetY)
        table.insert(vertices.position, record.z - pivotOffsetZ)
        table.insert(vertices.color, color)
        table.insert(vertices.normal, record.normal)
        local pbr = (mat.emissive & 0xF) | ((mat.roughness & 0xF) << 4) | ((mat.metallic & 0xF) << 8)
        table.insert(vertices.pbr, pbr)
    end

    -- Add extra voxels
    for _, record in ipairs(extras) do
        local noise = tknMath.perlinNoise3D(seed, record.x * 0.27, record.y * 0.27, record.z * 3)
        local mat
        if noise < -0.4 then
            mat = darkGrass
        elseif noise < 0.4 then
            mat = grass
        else
            mat = lightGrass
        end
        local rgba = mat.color
        local r = (rgba >> 24) & 0xFF
        local g = (rgba >> 16) & 0xFF
        local b = (rgba >> 8) & 0xFF
        local a = rgba & 0xFF
        local color = r | (g << 8) | (b << 16) | (a << 24)
        table.insert(vertices.position, record.x - pivotOffsetX)
        table.insert(vertices.position, record.y - pivotOffsetY)
        table.insert(vertices.position, record.z - pivotOffsetZ)
        table.insert(vertices.color, color)
        table.insert(vertices.normal, record.normal)
        local pbr = (mat.emissive & 0xF) | ((mat.roughness & 0xF) << 4) | ((mat.metallic & 0xF) << 8)
        table.insert(vertices.pbr, pbr)
    end

    local vertexCount = #tvox.records + #extras
    local packed = packVertexData(vertices, vertexCount)
    
    -- Create vertex buffer (NEW API)
    local pVertexBuffer = tkn.tknCreateBufferPtr(
        pTknGfxContext,
        math.max(#packed, 1),
        vulkan.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        false,
        packed ~= "" and packed or "\0"
    )

    -- Create instance buffer for mask instances (one per character, will update dynamically)
    -- Start with zeroed instance data
    local maxCharacters = 100
    local instancePacked = ""
    for i = 1, maxCharacters do
        -- model matrix (16 floats) - initialize to identity
        for j = 1, 16 do
            if j == 1 or j == 6 or j == 11 or j == 16 then
                instancePacked = instancePacked .. string.pack("f", 1.0)
            else
                instancePacked = instancePacked .. string.pack("f", 0.0)
            end
        end
    end

    local pInstanceBuffer = tkn.tknCreateBufferPtr(
        pTknGfxContext,
        #instancePacked,
        vulkan.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        true,  -- dynamic (will be updated)
        instancePacked
    )

    return {
        pVertexBuffer = pVertexBuffer,
        vertexCount = vertexCount,
        pInstanceBuffer = pInstanceBuffer,
        maxInstanceCount = maxCharacters,
        instanceCount = 0,
    }
end

function characterSystem.add(pTknGfxContext, x, y, z, seed)
    local rot = {0, 0, 0, 1}
    local characterTransform = transformSystem.add(x, y, z, 0, 0, 1, 0, characterSystem.scale, characterSystem.scale, characterSystem.scale, transformSystem.rootTransform, nil)
    local bodyTransform = transformSystem.add(0, 0, 0, rot[1], rot[2], rot[3], rot[4], 1, 1, 1, characterTransform, nil)
    local maskTransform = transformSystem.add(0, 2, 4, rot[1], rot[2], rot[3], rot[4], 1, 1, 1, bodyTransform, nil)

    if not characterSystem.bodyTvox then
        local meshPath = characterSystem.assetPath .. "/models/body.tvox"
        characterSystem.bodyTvox = tknVoxel.readTvox(meshPath)
    end
    if not characterSystem.maskTvox then
        local meshPath = characterSystem.assetPath .. "/models/mask.tvox"
        characterSystem.maskTvox = tknVoxel.readTvox(meshPath)
    end
    if not characterSystem.mask then
        characterSystem.mask = createMask(pTknGfxContext, seed)
    end

    local body = createBody(pTknGfxContext, seed)

    local char = {
        characterTransform = characterTransform,
        bodyTransform = bodyTransform,
        maskTransform = maskTransform,
        body = body,
    }

    table.insert(characterSystem.characters, char)
    
    -- Update mask instance count
    characterSystem.mask.instanceCount = #characterSystem.characters
    
    return char
end

local function transposeToColumnMajor(m, out, offset)
    out[offset + 1] = m[1];
    out[offset + 2] = m[5];
    out[offset + 3] = m[9];
    out[offset + 4] = m[13]
    out[offset + 5] = m[2];
    out[offset + 6] = m[6];
    out[offset + 7] = m[10];
    out[offset + 8] = m[14]
    out[offset + 9] = m[3];
    out[offset + 10] = m[7];
    out[offset + 11] = m[11];
    out[offset + 12] = m[15]
    out[offset + 13] = m[4];
    out[offset + 14] = m[8];
    out[offset + 15] = m[12];
    out[offset + 16] = m[16]
end

function characterSystem.updateInstances(pTknGfxContext)
    if not characterSystem.mask then
        return
    end
    local list = characterSystem.characters
    local maskModel = {}
    for i, char in ipairs(list) do
        local mm = char.maskTransform.model
        if mm then
            transposeToColumnMajor(mm, maskModel, (i - 1) * 16)
        else
            local base = (i - 1) * 16
            for j = 1, 16 do
                maskModel[base + j] = 0
            end
        end
    end
    
    -- Pack instance data to binary
    local packed = ""
    for i = 1, characterSystem.mask.maxInstanceCount do
        if i <= #maskModel / 16 then
            for j = 1, 16 do
                packed = packed .. string.pack("f", maskModel[(i-1)*16 + j] or 0)
            end
        else
            -- Remaining instances: identity matrix
            for j = 1, 16 do
                if j == 1 or j == 6 or j == 11 or j == 16 then
                    packed = packed .. string.pack("f", 1.0)
                else
                    packed = packed .. string.pack("f", 0.0)
                end
            end
        end
    end
    
    -- Update instance buffer (NEW API)
    tkn.tknUpdateBuffer(pTknGfxContext, characterSystem.mask.pInstanceBuffer, 0, #packed, packed)
end

function characterSystem.remove(char)
    local list = characterSystem.characters
    for i, v in ipairs(list) do
        if v == char then
            table.remove(list, i)
            break
        end
    end
    transformSystem.remove(char.maskTransform)
    transformSystem.remove(char.bodyTransform)
    transformSystem.remove(char.characterTransform)
    char.maskTransform = nil
    char.bodyTransform = nil
    char.characterTransform = nil
    char.body = nil
    
    -- Update mask instance count
    if characterSystem.mask then
        characterSystem.mask.instanceCount = #characterSystem.characters
    end
end

return characterSystem

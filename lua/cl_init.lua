print "Initializing client side"

local StaticBlockType = {}

function StaticBlockType:GetModel()
    return { model=self.model, textures=self.textures }
end

function PreInitialize()
    -- request the engine to load some required assets at startup
    game.RequestAsset("models/base/CommonBlock.json")
    game.RequestAsset("textures/white.png")
    game.RequestAsset("textures/minecraft/programmer_art/stone.png")
    game.RequestAsset("textures/minecraft/programmer_art/dirt.png")
    game.RequestAsset("textures/minecraft/programmer_art/dirt.png")
end

local function Transform(func, array)
    local result = {}
    for k, v in pairs(array) do
        result[k] = func(v)
    end
    return result
end

local function RegisterStaticBlock(name, model, ...)
    local textures = {...}
    local new_block_type = {model=model,textures=textures}
    setmetatable(new_block_type,{__index=StaticBlockType})
    return game.RegisterBlock(name, new_block_type)
end

function Initialize()

    

    StoneBlockType.model = game.GetAsset("models/base/CommonBlock.json")
    StoneBlockType.textures = {
        game.GetAsset("textures/minecraft/programmer_art/stone.png")
    }
end
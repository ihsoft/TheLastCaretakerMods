local PREFIX = "[DonkLiftKeyStateProbe]"

local tick_hook_registered = false
local registration_pending = false
local first_error_reported = false
local previous = { W = false, S = false, A = false, D = false }

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function unwrap(value)
    if value == nil then return nil end
    local ok, result = pcall(function() return value:get() end)
    return ok and result or value
end

local function valid(object)
    if object == nil then return false end
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result
end

local function is_player_controlled(actor)
    local ok, result = pcall(function() return actor:IsPlayerControlled() end)
    return ok and result
end

local function read_key(controller, key_name)
    local ok, result = pcall(function()
        return controller:IsInputKeyDown({ KeyName = key_name })
    end)
    if not ok then
        if not first_error_reported then
            first_error_reported = true
            log(string.format("IsInputKeyDown(%s) failed: %s", key_name, tostring(result)))
        end
        return nil
    end
    return unwrap(result) == true
end

local function probe_tick(context)
    local actor = unwrap(context)
    if not valid(actor) or not is_player_controlled(actor) then return end

    local controller_ok, controller = pcall(function() return actor:GetController() end)
    controller = unwrap(controller)
    if not controller_ok or not valid(controller) then return end

    for _, key_name in ipairs({ "W", "S", "A", "D" }) do
        local down = read_key(controller, key_name)
        if down ~= nil and down ~= previous[key_name] then
            previous[key_name] = down
            log(string.format("%s=%s", key_name, down and "down" or "up"))
        end
    end
end

local tick_path = "/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C:ReceiveTick"
local function try_register_tick_hook()
    if tick_hook_registered or registration_pending then return end
    registration_pending = true
    ExecuteInGameThread(function()
        local ok, pre_id, post_id = pcall(RegisterHook, tick_path, probe_tick)
        registration_pending = false
        if ok then
            tick_hook_registered = true
            log(string.format("read-only key-state hook registered (pre=%s post=%s)", tostring(pre_id), tostring(post_id)))
        end
    end)
end

LoopAsync(500, function()
    if tick_hook_registered then return true end
    try_register_tick_hook()
    return false
end)

log("loaded; read-only W/S/A/D state probe active while the forklift is player-controlled")

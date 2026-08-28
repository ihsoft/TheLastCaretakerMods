local PREFIX = "[DonkLiftInputInjectionProbe]"
local TEST_VALUE = 0.30

local injection_enabled = false
local release_pending = false
local tick_hook_registered = false
local registration_pending = false
local throttle_action = nil
local input_subsystem = nil
local first_success_reported = false
local first_error_reported = false
local reflection_reported = false

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

local function full_name(object)
    if object == nil then return "<nil>" end
    local ok, result = pcall(function() return object:GetFullName() end)
    return ok and result or tostring(object)
end

local function is_player_controlled(actor)
    local ok, result = pcall(function() return actor:IsPlayerControlled() end)
    return ok and result
end

local function find_throttle_action()
    if valid(throttle_action) then return throttle_action end
    pcall(LoadAsset, "/Game/Game/Input/Vehicle/IA_VehicleForward")
    local ok, action = pcall(
        StaticFindObject,
        "/Game/Game/Input/Vehicle/IA_VehicleForward.IA_VehicleForward"
    )
    if ok and valid(action) then
        throttle_action = action
        log("throttle action acquired: " .. full_name(action))
    end
    return throttle_action
end

local function find_input_subsystem()
    if valid(input_subsystem) then return input_subsystem end
    local ok, subsystems = pcall(FindAllOf, "EnhancedInputLocalPlayerSubsystem")
    if not ok or subsystems == nil then return nil end
    for _, subsystem in pairs(subsystems) do
        if valid(subsystem) then
            input_subsystem = subsystem
            log("input subsystem acquired: " .. full_name(subsystem))
            return input_subsystem
        end
    end
    return nil
end

local function report_injection_function()
    if reflection_reported then return end
    reflection_reported = true
    local ok, functions = pcall(FindAllOf, "Function")
    if not ok or functions == nil then return end
    for _, fn in pairs(functions) do
        local name = string.lower(full_name(fn))
        if string.find(name, "injectinputvectorforaction", 1, true) then
            log("injection function found: " .. full_name(fn))
        end
    end
end

local function inject_throttle(value)
    local subsystem = find_input_subsystem()
    local action = find_throttle_action()
    if not valid(subsystem) or not valid(action) then
        if not first_error_reported then
            first_error_reported = true
            log(string.format(
                "injection prerequisites missing: subsystem=%s action=%s",
                full_name(subsystem), full_name(action)
            ))
        end
        return false
    end

    local ok, err = pcall(function()
        subsystem:InjectInputVectorForAction(
            action,
            { X = value, Y = 0, Z = 0 },
            {},
            {}
        )
    end)
    if not ok then
        if not first_error_reported then
            first_error_reported = true
            log("InjectInputVectorForAction failed: " .. tostring(err))
        end
        return false
    end
    if not first_success_reported then
        first_success_reported = true
        log(string.format("native Axis1D injection active at %.2f", value))
    end
    return true
end

local function injection_tick(context)
    local actor = unwrap(context)
    if not valid(actor) or not is_player_controlled(actor) then
        if injection_enabled then
            injection_enabled = false
            release_pending = false
            log("injection stopped because the forklift is no longer player-controlled")
        end
        return
    end

    if injection_enabled then
        inject_throttle(TEST_VALUE)
    elseif release_pending then
        inject_throttle(0)
        release_pending = false
        log("zero Axis1D release injected")
    end
end

local tick_path = "/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C:ReceiveTick"
local function try_register_tick_hook()
    if tick_hook_registered or registration_pending then return end
    registration_pending = true
    ExecuteInGameThread(function()
        local ok, pre_id, post_id = pcall(RegisterHook, tick_path, injection_tick)
        registration_pending = false
        if ok then
            tick_hook_registered = true
            log(string.format("forklift tick hook registered (pre=%s post=%s)", tostring(pre_id), tostring(post_id)))
        end
    end)
end

RegisterKeyBind(Key.F6, function()
    injection_enabled = not injection_enabled
    release_pending = not injection_enabled
    first_success_reported = false
    first_error_reported = false
    ExecuteInGameThread(report_injection_function)
    log(string.format(
        "30%% throttle injection %s",
        injection_enabled and "enabled" or "disabled"
    ))
end)

LoopAsync(500, function()
    if tick_hook_registered then return true end
    try_register_tick_hook()
    return false
end)

log("loaded; enter DonkLift, press F7 to disable the control mod, then F6 to toggle 30% native throttle injection")

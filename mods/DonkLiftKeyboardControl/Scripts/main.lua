local PREFIX = "[DonkLiftKeyboardControl]"

local FIXED_INTERVAL_MS = 50
local MIN_THROTTLE_RATE = 0.45
local MAX_THROTTLE_RATE = 0.80
local STEP_RAMP_TIME_MS = 2000
local MAX_ABS_THROTTLE = 0.9999
local ZERO_OUTPUT_SENTINEL = 0.0001

local throttle_direction = 0
local current_throttle = ZERO_OUTPUT_SENTINEL
local current_throttle_step = MIN_THROTTLE_RATE * FIXED_INTERVAL_MS / 1000
local active_step_direction = 0
local active_direction_time_ms = 0
local last_reported_direction = nil
local active_actor_name = nil
local active_actor = nil

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function unwrap(value)
    if value == nil then return nil end
    local ok, result = pcall(function() return value:get() end)
    return ok and result or value
end

local function valid(object)
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result
end

local function full_name(object)
    local ok, result = pcall(function() return object:GetFullName() end)
    return ok and result or ""
end

local function is_player_controlled(actor)
    local ok, result = pcall(function() return actor:IsPlayerControlled() end)
    return ok and result
end

local function clamp(value, minimum, maximum)
    if value < minimum then return minimum end
    if value > maximum then return maximum end
    return value
end

local function reset_throttle_state(throttle_value)
    throttle_direction = 0
    current_throttle = throttle_value
    if current_throttle == nil then
        current_throttle = ZERO_OUTPUT_SENTINEL
    end
    current_throttle_step = MIN_THROTTLE_RATE * FIXED_INTERVAL_MS / 1000
    active_step_direction = 0
    active_direction_time_ms = 0
    last_reported_direction = nil
end

RegisterKeyBind(Key.X, function()
    if active_actor_name == nil then return end
    reset_throttle_state()
    if valid(active_actor) then
        pcall(function() active_actor.ThrottleInput = current_throttle end)
    end
    log("throttle reset to zero")
end)

local function before_throttle_input_read(context)
    local actor = unwrap(context)
    if not valid(actor) then return end

    local actor_name = full_name(actor)
    if not is_player_controlled(actor) then
        if active_actor_name == actor_name then
            reset_throttle_state(0)
            active_actor_name = nil
            active_actor = nil
            actor.ThrottleInput = 0
            log("forklift left; throttle reset to zero")
        end
        return
    end

    if active_actor_name ~= actor_name then
        reset_throttle_state()
        active_actor_name = actor_name
        active_actor = actor
        log("player-controlled forklift acquired; throttle initialized at zero")
    else
        active_actor = actor
    end

    local ok, written_value = pcall(function() return actor.ThrottleInput end)
    if not ok or type(written_value) ~= "number" then
        log("GetThrottleInput pre-hook could not read ThrottleInput")
        return
    end

    if written_value == -1 then
        throttle_direction = -1
    elseif written_value == 1 then
        throttle_direction = 1
    elseif written_value == 0 then
        throttle_direction = 0
    end

    if throttle_direction ~= last_reported_direction then
        last_reported_direction = throttle_direction
        log(string.format(
            "throttle direction=%d current=%.3f raw=%.3f",
            throttle_direction, current_throttle, written_value
        ))
    end

    actor.ThrottleInput = current_throttle
end


LoopAsync(FIXED_INTERVAL_MS, function()
    if throttle_direction == 0 then
        active_step_direction = 0
        active_direction_time_ms = 0
        current_throttle_step = MIN_THROTTLE_RATE * FIXED_INTERVAL_MS / 1000
    else
        if throttle_direction ~= active_step_direction then
            active_step_direction = throttle_direction
            active_direction_time_ms = 0
        end

        local ramp = clamp(active_direction_time_ms / STEP_RAMP_TIME_MS, 0, 1)
        local rate = MIN_THROTTLE_RATE
            + (MAX_THROTTLE_RATE - MIN_THROTTLE_RATE) * ramp
        current_throttle_step = rate * FIXED_INTERVAL_MS / 1000
        current_throttle = clamp(
            current_throttle + throttle_direction * current_throttle_step,
            -MAX_ABS_THROTTLE,
            MAX_ABS_THROTTLE
        )
        active_direction_time_ms = active_direction_time_ms + FIXED_INTERVAL_MS
    end
    return false
end)

local function register_hook()
    local ok, pre_id, post_id = pcall(
        RegisterHook,
        "/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput",
        before_throttle_input_read
    )

    if ok then
        log(string.format("throttle getter pre-hook registered (pre=%s post=%s)", tostring(pre_id), tostring(post_id)))
    else
        log("throttle getter pre-hook registration failed: " .. tostring(pre_id))
    end
end

ExecuteWithDelay(1000, register_hook)
log("loaded; waiting to register throttle getter pre-hook")

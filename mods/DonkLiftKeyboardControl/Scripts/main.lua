local PREFIX = "[DonkLiftKeyboardControl]"

local FIXED_INTERVAL_MS = 50
local MIN_THROTTLE_RATE = 0.45
local MAX_THROTTLE_RATE = 0.80
local STEP_RAMP_TIME_MS = 2000

local throttle_direction = 0
local current_throttle = 0
local current_throttle_step = MIN_THROTTLE_RATE * FIXED_INTERVAL_MS / 1000
local active_step_direction = 0
local active_direction_time_ms = 0
local last_reported_direction = nil

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

local function clamp(value, minimum, maximum)
    if value < minimum then return minimum end
    if value > maximum then return maximum end
    return value
end

local function before_throttle_input_read(context)
    local actor = unwrap(context)
    if not valid(actor) then return end

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
            -1,
            1
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

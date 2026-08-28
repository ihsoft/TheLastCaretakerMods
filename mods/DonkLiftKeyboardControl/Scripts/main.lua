local PREFIX = "[DonkLiftKeyboardControl]"

local FIXED_INTERVAL_MS = 50
local MIN_THROTTLE_RATE = 0.45
local MAX_THROTTLE_RATE = 0.80
local STEP_RAMP_TIME_MS = 2000
local STEERING_MAXIMUM_SPEED = 1.60
local STEERING_ACCELERATION = 2.50
local STEERING_REVERSAL_BRAKING = 5.00

-- The game's digital axes write exact -1, 0, and 1 into the input fields.
-- Our values must never equal those command markers: 0.9999/0.0001 still
-- render as 100%/0% in the HUD, but remain distinguishable on the next read.
local MAX_ABS_INPUT = 0.9999
local ZERO_OUTPUT_SENTINEL = 0.0001

local throttle_direction = 0
local current_throttle = ZERO_OUTPUT_SENTINEL
local current_throttle_step = MIN_THROTTLE_RATE * FIXED_INTERVAL_MS / 1000
local active_step_direction = 0
local active_direction_time_ms = 0

local steering_direction = 0
local current_steering = ZERO_OUTPUT_SENTINEL
local steering_velocity = 0

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

local function approach(current, target, maximum_change)
    if current < target then return math.min(current + maximum_change, target) end
    if current > target then return math.max(current - maximum_change, target) end
    return current
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
end

local function reset_steering_state(steering_value)
    steering_direction = 0
    current_steering = steering_value
    if current_steering == nil then
        current_steering = ZERO_OUTPUT_SENTINEL
    end
    steering_velocity = 0
end

local function reset_control_state(value)
    reset_throttle_state(value)
    reset_steering_state(value)
end

local function controlled_forklift(context)
    local actor = unwrap(context)
    if not valid(actor) then return nil, nil end

    local actor_name = full_name(actor)
    if not is_player_controlled(actor) then
        if active_actor_name == actor_name then
            -- Sentinels are only for an actively controlled forklift. Leaving
            -- it requires real zeroes so no residual throttle or steering remains.
            reset_control_state(0)
            actor.ThrottleInput = 0
            actor.SteeringInput = 0
            active_actor_name = nil
            active_actor = nil
        end
        return nil, nil
    end

    if active_actor_name ~= actor_name then
        reset_control_state()
        active_actor_name = actor_name
        active_actor = actor
    else
        active_actor = actor
    end

    return actor, actor_name
end

RegisterKeyBind(Key.X, function()
    if active_actor_name == nil then return end
    reset_throttle_state()
    if valid(active_actor) then
        pcall(function() active_actor.ThrottleInput = current_throttle end)
    end
end)

RegisterKeyBind(Key.C, function()
    if active_actor_name == nil then return end
    reset_steering_state()
    if valid(active_actor) then
        pcall(function() active_actor.SteeringInput = current_steering end)
    end
end)

local function before_throttle_input_read(context)
    local actor = controlled_forklift(context)
    if actor == nil then return end

    local ok, written_value = pcall(function() return actor.ThrottleInput end)
    if not ok or type(written_value) ~= "number" then return end

    -- Non-digital values are the previous output of this mod. Preserve the
    -- last real keyboard direction until the game writes a new -1/0/1 command.
    if written_value == -1 then
        throttle_direction = -1
    elseif written_value == 1 then
        throttle_direction = 1
    elseif written_value == 0 then
        throttle_direction = 0
    end

    actor.ThrottleInput = current_throttle
end

local function before_steering_input_read(context)
    local actor = controlled_forklift(context)
    if actor == nil then return end

    local ok, written_value = pcall(function() return actor.SteeringInput end)
    if not ok or type(written_value) ~= "number" then return end

    -- Steering uses the same reserved digital command values as throttle.
    if written_value == -1 then
        steering_direction = -1
    elseif written_value == 1 then
        steering_direction = 1
    elseif written_value == 0 then
        steering_direction = 0
    end

    actor.SteeringInput = current_steering
end


-- Integrate on a fixed clock. Getter frequency varies within a frame and must
-- not affect acceleration, steering speed, or the duration of a key press.
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
            -MAX_ABS_INPUT,
            MAX_ABS_INPUT
        )
        if current_throttle == 0 then current_throttle = ZERO_OUTPUT_SENTINEL end
        active_direction_time_ms = active_direction_time_ms + FIXED_INTERVAL_MS
    end

    -- Steering velocity ramps up for precise taps and brakes faster on reversal.
    local dt = FIXED_INTERVAL_MS / 1000
    if steering_direction == 0 then
        steering_velocity = 0
    else
        local target_velocity = steering_direction * STEERING_MAXIMUM_SPEED
        local acceleration = STEERING_ACCELERATION
        if steering_velocity * steering_direction < 0 then
            acceleration = STEERING_REVERSAL_BRAKING
        end
        steering_velocity = approach(
            steering_velocity,
            target_velocity,
            acceleration * dt
        )
        current_steering = clamp(
            current_steering + steering_velocity * dt,
            -MAX_ABS_INPUT,
            MAX_ABS_INPUT
        )
        if current_steering == 0 then current_steering = ZERO_OUTPUT_SENTINEL end
        if (current_steering <= -MAX_ABS_INPUT and steering_velocity < 0)
            or (current_steering >= MAX_ABS_INPUT and steering_velocity > 0) then
            steering_velocity = 0
        end
    end
    return false
end)

local function register_hook()
    local throttle_ok, throttle_result = pcall(
        RegisterHook,
        "/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput",
        before_throttle_input_read
    )
    local steering_ok, steering_result = pcall(
        RegisterHook,
        "/Script/Voyage.VoyageVehicleForkliftPawn:GetSteeringInput",
        before_steering_input_read
    )

    if not throttle_ok or not steering_ok then
        log(string.format(
            "input getter pre-hook registration failed (throttle=%s steering=%s)",
            tostring(throttle_result), tostring(steering_result)
        ))
    end
end

ExecuteWithDelay(1000, register_hook)

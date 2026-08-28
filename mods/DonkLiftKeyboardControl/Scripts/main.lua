local PREFIX = "[DonkLiftKeyboardControl]"

local FIXED_INTERVAL_MS = 50
local FIXED_DELTA_SECONDS = FIXED_INTERVAL_MS / 1000
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
local active_step_direction = 0
local active_direction_time_ms = 0

local steering_direction = 0
local current_steering = ZERO_OUTPUT_SENTINEL
local steering_velocity = 0

local active_actor = nil
local active_actor_name = nil

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function reset_throttle_state(throttle_value)
    throttle_direction = 0
    current_throttle = throttle_value
    if current_throttle == nil then
        current_throttle = ZERO_OUTPUT_SENTINEL
    end
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

RegisterKeyBind(Key.X, function()
    if active_actor == nil then return end
    reset_throttle_state()
    pcall(function() active_actor.ThrottleInput = current_throttle end)
end)

RegisterKeyBind(Key.C, function()
    if active_actor == nil then return end
    reset_steering_state()
    pcall(function() active_actor.SteeringInput = current_steering end)
end)

local function release_forklift_if_active(actor)
    if active_actor_name == nil or actor:GetFullName() ~= active_actor_name then return end
    reset_control_state(0)
    actor.ThrottleInput = 0
    actor.SteeringInput = 0
    active_actor = nil
    active_actor_name = nil
end

-- context:get() may produce a different Lua wrapper for the same UObject on
-- every call, so wrapper equality cannot identify the cached actor. One direct
-- IsPlayerControlled call is the cheapest reliable filter for these hooks.
-- GetFullName is restricted to the cold acquire/release paths.
local function before_throttle_input_read(context)
    local actor = context:get()
    if not actor:IsPlayerControlled() then
        release_forklift_if_active(actor)
        return
    end
    if active_actor == nil then
        reset_control_state()
        active_actor_name = actor:GetFullName()
    end
    active_actor = actor

    local written_value = actor.ThrottleInput

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
    local actor = context:get()
    if not actor:IsPlayerControlled() then
        release_forklift_if_active(actor)
        return
    end
    if active_actor == nil then
        reset_control_state()
        active_actor_name = actor:GetFullName()
    end
    active_actor = actor

    local written_value = actor.SteeringInput

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
    -- Destruction/exit handling runs once per fixed tick instead of being
    -- repeated independently by both native getter hooks.
    local actor = active_actor
    if actor ~= nil then
        local actor_is_valid = actor:IsValid()
        if not actor_is_valid or not actor:IsPlayerControlled() then
            reset_control_state(0)
            if actor_is_valid then
                actor.ThrottleInput = 0
                actor.SteeringInput = 0
            end
            active_actor = nil
            active_actor_name = nil
        end
    end

    if throttle_direction == 0 then
        active_step_direction = 0
        active_direction_time_ms = 0
    else
        if throttle_direction ~= active_step_direction then
            active_step_direction = throttle_direction
            active_direction_time_ms = 0
        end

        local ramp = active_direction_time_ms / STEP_RAMP_TIME_MS
        if ramp > 1 then ramp = 1 end
        local rate = MIN_THROTTLE_RATE
            + (MAX_THROTTLE_RATE - MIN_THROTTLE_RATE) * ramp
        current_throttle = current_throttle
            + throttle_direction * rate * FIXED_DELTA_SECONDS
        if current_throttle > MAX_ABS_INPUT then
            current_throttle = MAX_ABS_INPUT
        elseif current_throttle < -MAX_ABS_INPUT then
            current_throttle = -MAX_ABS_INPUT
        end
        if current_throttle == 0 then current_throttle = ZERO_OUTPUT_SENTINEL end
        active_direction_time_ms = active_direction_time_ms + FIXED_INTERVAL_MS
    end

    -- Steering velocity ramps up for precise taps and brakes faster on reversal.
    if steering_direction == 0 then
        steering_velocity = 0
    else
        local target_velocity = steering_direction * STEERING_MAXIMUM_SPEED
        local acceleration = STEERING_ACCELERATION
        if steering_velocity * steering_direction < 0 then
            acceleration = STEERING_REVERSAL_BRAKING
        end
        local maximum_change = acceleration * FIXED_DELTA_SECONDS
        if steering_velocity < target_velocity then
            steering_velocity = steering_velocity + maximum_change
            if steering_velocity > target_velocity then
                steering_velocity = target_velocity
            end
        elseif steering_velocity > target_velocity then
            steering_velocity = steering_velocity - maximum_change
            if steering_velocity < target_velocity then
                steering_velocity = target_velocity
            end
        end
        current_steering = current_steering
            + steering_velocity * FIXED_DELTA_SECONDS
        if current_steering > MAX_ABS_INPUT then
            current_steering = MAX_ABS_INPUT
        elseif current_steering < -MAX_ABS_INPUT then
            current_steering = -MAX_ABS_INPUT
        end
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

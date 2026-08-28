local PREFIX = "[DonkLiftKeyboardControl]"
local last_observed_value = nil

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

local function before_throttle_input_read(context)
    local actor = unwrap(context)
    if not valid(actor) then return end

    local ok, written_value = pcall(function() return actor.ThrottleInput end)
    if not ok or type(written_value) ~= "number" then
        log("GetThrottleInput pre-hook could not read ThrottleInput")
        return
    end

    if written_value ~= last_observed_value then
        last_observed_value = written_value
        log(string.format("ThrottleInput before game read: %.3f", written_value))
    end

    if written_value == -1 then
        actor.ThrottleInput = -0.3
    elseif written_value == 1 then
        actor.ThrottleInput = 0.5
    end
end

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

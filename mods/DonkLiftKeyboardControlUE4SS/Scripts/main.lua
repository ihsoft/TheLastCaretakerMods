local PREFIX = "[DonkLiftKeyboardControlUE4SS]"

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

-- HUD hints are intentionally isolated from the control state above. This
-- slower loop only creates display widgets and never writes vehicle input.
local HUD_POLL_INTERVAL_MS = 1000
local HUD_LABELS = {
    en = { brake = "Brake", center = "Center" },
    ru = { brake = "Тормоз", center = "Выровнять" },
}
local HUD_LANGUAGE_BY_ENUM_VALUE = {
    [1] = "en",  -- EVoyageLanguageType::English
    [11] = "ru", -- EVoyageLanguageType::Russian
}

local hud_widget_library = nil
local hud_text_library = nil
local hud_internationalization_library = nil
local hud_last_error = nil
local hud_active_language = nil
local hud_configured_roots = {}

local function hud_valid(object)
    return object ~= nil and object:IsValid()
end

local function resolve_hud_libraries()
    if not hud_valid(hud_widget_library) then
        hud_widget_library = StaticFindObject("/Script/UMG.Default__WidgetBlueprintLibrary")
    end
    if not hud_valid(hud_text_library) then
        hud_text_library = StaticFindObject("/Script/Engine.Default__KismetTextLibrary")
    end
    if not hud_valid(hud_internationalization_library) then
        hud_internationalization_library = StaticFindObject(
            "/Script/Engine.Default__KismetInternationalizationLibrary"
        )
    end
    return hud_valid(hud_widget_library) and hud_valid(hud_text_library)
end

local function find_game_settings()
    local settings_objects = FindAllOf("VoyageGameUserSettings")
    if settings_objects == nil then return nil end
    for _, settings in pairs(settings_objects) do
        if hud_valid(settings) and not settings:GetFullName():find("Default__", 1, true) then
            return settings
        end
    end
    return nil
end

local function resolve_hud_language_once()
    if hud_active_language ~= nil then return true end
    local settings = find_game_settings()
    if settings == nil then return false end

    local value = settings.CustomSettings.LanguageType
    local raw = tostring(value)
    local normalized = raw:lower()
    local language = nil
    if normalized:find("russian", 1, true) then
        language = "ru"
    elseif normalized:find("english", 1, true) then
        language = "en"
    else
        language = HUD_LANGUAGE_BY_ENUM_VALUE[tonumber(value) or tonumber(raw)]
    end

    -- System and untranslated languages fall back to Unreal's language when
    -- it is one we support, then to English.
    if language == nil and hud_valid(hud_internationalization_library) then
        local current = hud_internationalization_library:GetCurrentLanguage()
        if current ~= nil then
            local engine_language = tostring(current):lower():match("^([a-z]+)")
            if HUD_LABELS[engine_language] ~= nil then language = engine_language end
        end
    end

    hud_active_language = language or "en"
    return true
end

local function set_hud_text(text_block, value)
    text_block:SetText(hud_text_library:Conv_StringToText(value))
end

local function create_hud_hint(hud, root, template_indicator)
    local owner = hud:GetOwningPlayer()
    local indicator = hud_widget_library:Create(hud, template_indicator:GetClass(), owner)
    if not hud_valid(indicator) then
        error("WidgetBlueprintLibrary.Create returned no widget")
    end
    root:AddChild(indicator)
end

local function configure_hud_hint(indicator, key, description)
    if not hud_valid(indicator) then return false end
    local container = indicator.ButtonInfoContainer
    if not hud_valid(container) then return false end
    local panel = container.RootPanel
    if not hud_valid(panel) or panel:GetChildrenCount() < 1 then return false end
    local button_info = panel:GetChildAt(0)
    if not hud_valid(button_info)
        or not hud_valid(button_info.PrimaryKeyTB)
        or not hud_valid(button_info.DescriptionTB)
        or not hud_valid(button_info.SecondaryKeyTB)
        or not hud_valid(button_info.DescriptionErrorTB) then
        return false
    end

    -- These copies are display-only. Prevent native key-rebinding refreshes
    -- from restoring the template's E/Interact text.
    container.bAutoUpdateKeyRebindings = false
    set_hud_text(button_info.PrimaryKeyTB, key)
    set_hud_text(button_info.DescriptionTB, description)
    set_hud_text(button_info.SecondaryKeyTB, "")
    set_hud_text(button_info.DescriptionErrorTB, "")
    return true
end

local function add_missing_hud_hints(hud, language, labels)
    local dynamic_widget = hud.BP_DynamicPlayerInputHorizontalWidget_Bottom
    local root = dynamic_widget.ContextInputActionsRoot
    if not hud_valid(root) then return end

    local count = root:GetChildrenCount()
    if count < 2 then return end
    local root_name = root:GetFullName()
    if count < 4 then hud_configured_roots[root_name] = nil end
    local template_indicator = root:GetChildAt(0)
    if not hud_valid(template_indicator) then return end

    if count == 2 then
        create_hud_hint(hud, root, template_indicator)
        count = root:GetChildrenCount()
    end
    if count == 3 then
        create_hud_hint(hud, root, template_indicator)
        count = root:GetChildrenCount()
    end
    if count >= 4 and hud_configured_roots[root_name] ~= language then
        local x_ready = configure_hud_hint(root:GetChildAt(2), "X", labels.brake)
        local c_ready = configure_hud_hint(root:GetChildAt(3), "C", labels.center)
        if x_ready and c_ready then hud_configured_roots[root_name] = language end
    end
end

local function update_forklift_huds()
    if not resolve_hud_libraries() then error("required UMG/text libraries are unavailable") end
    local huds = FindAllOf("BP_VoyageIngameForklift_C")
    local has_valid_hud = false
    if huds ~= nil then
        for _, hud in pairs(huds) do
            if hud_valid(hud) then
                has_valid_hud = true
                break
            end
        end
    end

    -- The language can change only from the menu. Clear the cache when the
    -- forklift HUD is destroyed and read it again on the next game load.
    if not has_valid_hud then
        if hud_active_language ~= nil then
            hud_active_language = nil
            hud_configured_roots = {}
        end
        return
    end

    if not resolve_hud_language_once() then return end
    local labels = HUD_LABELS[hud_active_language]
    for _, hud in pairs(huds) do
        if hud_valid(hud) then
            add_missing_hud_hints(hud, hud_active_language, labels)
        end
    end
end

LoopAsync(HUD_POLL_INTERVAL_MS, function()
    ExecuteInGameThread(function()
        local ok, err = pcall(update_forklift_huds)
        if not ok then
            local message = tostring(err)
            if message ~= hud_last_error then
                hud_last_error = message
                log("HUD update failed: " .. message)
            end
        else
            hud_last_error = nil
        end
    end)
    return false
end)

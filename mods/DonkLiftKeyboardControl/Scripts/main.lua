local PREFIX = "[DonkLiftKeyboardControl]"

local CONFIG = {
    throttle_acceleration_rate = 0.45,
    throttle_braking_rate = 0.80,
    steering_maximum_speed = 0.80,
    steering_acceleration = 1.25,
    steering_reversal_braking = 2.50,
    maximum_delta_time = 0.10,
    input_dead_zone = 0.50,
}

local keyboard_seen = false
local smoothing_enabled = true
local tick_hook_registered = false
local registration_pending = false
local states = {}
local input_getter_hooks_registered = false
local input_getter_registration_pending = false
local bypass_input_getter_override = false
local slider_hook_registered = false
local slider_registration_pending = false
local current_hud_throttle = nil
local current_hud_steering = nil
local reported_sliders = {}
local throttle_indicator = nil
local steering_indicator = nil
local throttle_text_indicator = nil
local steering_text_indicator = nil
local indicator_bindings_cleared = false
local reported_binding_error = false
local indicator_search_cooldown = 0
local reported_indicator_widgets = {}
local reported_delegate_sources = {}
local hud_layout_dumped = false
local hud_post_hooks = {}
local vehicle_hud = nil
local vehicle_hud_search_cooldown = 0
local native_hud_update_reported = false
local native_hud_error_reported = false
local native_hud_hooks_registered = false
local native_hud_hook_registration_pending = false
local native_hud_throttle_intercept_reported = false
local native_hud_steering_intercept_reported = false
local input_setter_hooks_registered = false
local input_setter_hook_registration_pending = false
local raw_commands = {}
local fresh_keyboard_press = { throttle = false, steering = false }
local input_fields_reported = false
local throttle_zero_requested = false
local steering_zero_requested = false
local hint_hud_name = nil
local throttle_zero_hint = nil
local steering_zero_hint = nil
local hint_creation_error_reported = false
local hint_signature_dumped = false

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
    return ok and result or tostring(object)
end

local function clamp(value, minimum, maximum)
    if value < minimum then return minimum end
    if value > maximum then return maximum end
    return value
end

local function direction(value)
    if value > CONFIG.input_dead_zone then return 1 end
    if value < -CONFIG.input_dead_zone then return -1 end
    return 0
end

local function approach(current, target, maximum_change)
    if current < target then return math.min(current + maximum_change, target) end
    if current > target then return math.max(current - maximum_change, target) end
    return current
end

local function approach_with_direction(current, requested_direction, delta_time, normal_rate, opposite_rate)
    if requested_direction == 0 then return current end
    local rate = current * requested_direction < 0 and opposite_rate or normal_rate
    local updated = current + requested_direction * rate * delta_time
    if current * requested_direction < 0 and updated * requested_direction > 0 then return 0 end
    return clamp(updated, -1, 1)
end

local function read_number(actor, property_name)
    local ok, value = pcall(function() return actor[property_name] end)
    value = unwrap(value)
    if ok and type(value) == "number" then return value end
    return nil
end

local function slider_value(slider, normalized)
    local minimum = read_number(slider, "MinValue")
    local maximum = read_number(slider, "MaxValue")
    if minimum == nil or maximum == nil then return normalized end
    local magnitude = math.max(math.abs(minimum), math.abs(maximum))
    if magnitude > 1.01 then return normalized * magnitude end
    return normalized
end

local function describe_delegate(delegate)
    if delegate == nil then return "nil" end
    local object_name = "nil"
    local function_name = "nil"
    pcall(function()
        if valid(delegate.Object) then object_name = full_name(delegate.Object) end
    end)
    pcall(function() function_name = tostring(delegate.FunctionName) end)
    return string.format("object=[%s] function=[%s]", object_name, function_name)
end

local function report_delegate_source(widget, property_name)
    if not valid(widget) then return end
    local key = full_name(widget) .. ":" .. property_name
    if reported_delegate_sources[key] then return end
    reported_delegate_sources[key] = true
    local ok, delegate = pcall(function() return widget[property_name] end)
    if ok then
        log(string.format("HUD %s source: %s", property_name, describe_delegate(delegate)))
    else
        log(string.format("HUD %s source read failed: %s", property_name, tostring(delegate)))
    end
end

local function is_player_controlled(actor)
    local ok, result = pcall(function() return actor:IsPlayerControlled() end)
    return ok and result
end

local function capture_raw_command(context, value, field_name)
    local actor = unwrap(context)
    local number = unwrap(value)
    if not valid(actor) or type(number) ~= "number" then return end
    local actor_name = full_name(actor)
    local command = raw_commands[actor_name]
    if command == nil then
        command = {}
        raw_commands[actor_name] = command
    end
    local is_digital = math.abs(number) < 0.01 or math.abs(math.abs(number) - 1) < 0.01
    if not keyboard_seen then
        command[field_name] = number
    elseif is_digital then
        if math.abs(number) < 0.01 then
            command[field_name] = 0
            fresh_keyboard_press[field_name] = false
        elseif fresh_keyboard_press[field_name] then
            -- A real press is also seen by RegisterKeyBind. The game sometimes
            -- repeats the old non-zero value just after sending release=0;
            -- without a matching key-down event that duplicate is discarded.
            command[field_name] = number
            fresh_keyboard_press[field_name] = false
        end
    end

    -- Preserve the physical digital command above, but keep native input
    -- fields neutral. The mod drives the Blueprint physics fields directly;
    -- feeding the retained value back through Get*Input causes recursion.
    if keyboard_seen and smoothing_enabled then
        local state = states[actor_name]
        if state ~= nil and type(state[field_name]) == "number" then
            value:set(0)
        end
    end
end

local function capture_raw_throttle(context, value)
    capture_raw_command(context, value, "throttle")
end

local function capture_raw_steering(context, value)
    capture_raw_command(context, value, "steering")
end

local function try_register_input_setter_hooks()
    if input_setter_hooks_registered or input_setter_hook_registration_pending then return end
    input_setter_hook_registration_pending = true
    ExecuteInGameThread(function()
        local throttle_ok, throttle_pre, throttle_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageVehicleForkliftPawn:SetThrottleInput",
            capture_raw_throttle
        )
        local steering_ok, steering_pre, steering_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageVehicleForkliftPawn:SetSteeringInput",
            capture_raw_steering
        )
        input_setter_hook_registration_pending = false
        if throttle_ok and steering_ok then
            input_setter_hooks_registered = true
            log(string.format(
                "input setter hooks registered (throttle=%s/%s steering=%s/%s)",
                tostring(throttle_pre), tostring(throttle_post),
                tostring(steering_pre), tostring(steering_post)
            ))
        else
            log("input setter hook registration failed; will retry")
        end
    end)
end

local function smoothed_input_for(context, field_name)
    if bypass_input_getter_override or not smoothing_enabled or not keyboard_seen then return nil end
    local actor = unwrap(context)
    if not valid(actor) or not is_player_controlled(actor) then return nil end
    local state = states[full_name(actor)]
    if state == nil then return nil end
    return state[field_name]
end

local function override_throttle_getter(context)
    if bypass_input_getter_override or not smoothing_enabled or not keyboard_seen then return nil end
    local actor = unwrap(context)
    if not valid(actor) or not is_player_controlled(actor) then return nil end
    if states[full_name(actor)] == nil then return nil end
    return 0
end

local function override_steering_getter(context)
    if bypass_input_getter_override or not smoothing_enabled or not keyboard_seen then return nil end
    local actor = unwrap(context)
    if not valid(actor) or not is_player_controlled(actor) then return nil end
    if states[full_name(actor)] == nil then return nil end
    return 0
end

local function read_raw_vehicle_inputs(actor)
    bypass_input_getter_override = true
    local throttle_ok, raw_throttle = pcall(function() return actor:GetThrottleInput() end)
    local steering_ok, raw_steering = pcall(function() return actor:GetSteeringInput() end)
    bypass_input_getter_override = false
    return throttle_ok, unwrap(raw_throttle), steering_ok, unwrap(raw_steering)
end

local function try_register_input_getter_hooks()
    if input_getter_hooks_registered or input_getter_registration_pending then return end
    input_getter_registration_pending = true
    ExecuteInGameThread(function()
        local noop = function() end
        local throttle_ok, throttle_pre, throttle_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput",
            noop,
            override_throttle_getter
        )
        local steering_ok, steering_pre, steering_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageVehicleForkliftPawn:GetSteeringInput",
            noop,
            override_steering_getter
        )
        input_getter_registration_pending = false
        if throttle_ok and steering_ok then
            input_getter_hooks_registered = true
            log(string.format(
                "input getter post-hooks registered (throttle=%s/%s steering=%s/%s)",
                tostring(throttle_pre), tostring(throttle_post),
                tostring(steering_pre), tostring(steering_post)
            ))
        else
            log("input getter hook registration failed; will retry")
        end
    end)
end

local function override_slider_value(context, new_value)
    local slider = unwrap(context)
    if not valid(slider) then return end
    local name = string.lower(full_name(slider))
    local replacement = nil
    if string.find(name, "throttleslider", 1, true) and current_hud_throttle ~= nil then
        replacement = slider_value(slider, current_hud_throttle)
    end
    if string.find(name, "steeringslider", 1, true) and current_hud_steering ~= nil then
        replacement = slider_value(slider, current_hud_steering)
    end
    if replacement == nil then return end

    if not reported_sliders[name] then
        reported_sliders[name] = true
        log(string.format("HUD SetValue intercepted: %s", full_name(slider)))
    end
    new_value:set(replacement)
end

local function try_register_slider_hook()
    if slider_hook_registered or slider_registration_pending then return end
    slider_registration_pending = true
    ExecuteInGameThread(function()
        local path = "/Script/UMG.Slider:SetValue"
        local ok, pre_id, post_id = pcall(RegisterHook, path, override_slider_value)
        slider_registration_pending = false
        if ok then
            slider_hook_registered = true
            log(string.format("HUD Slider:SetValue pre-hook registered (pre=%s post=%s)", tostring(pre_id), tostring(post_id)))
        end
    end)
end

local function apply_cached_indicators()
    if current_hud_throttle ~= nil and valid(throttle_indicator) then
        pcall(function() throttle_indicator:SetValue(slider_value(throttle_indicator, current_hud_throttle)) end)
    end
    if current_hud_steering ~= nil and valid(steering_indicator) then
        pcall(function() steering_indicator:SetValue(slider_value(steering_indicator, current_hud_steering)) end)
    end
end

local function clear_indicator_bindings()
    if indicator_bindings_cleared then return true end
    if not valid(throttle_indicator) or not valid(steering_indicator) then return false end

    report_delegate_source(throttle_indicator, "ValueDelegate")
    report_delegate_source(steering_indicator, "ValueDelegate")
    log(string.format(
        "HUD slider ranges: throttle min=%s max=%s value=%s; steering min=%s max=%s value=%s",
        tostring(read_number(throttle_indicator, "MinValue")),
        tostring(read_number(throttle_indicator, "MaxValue")),
        tostring(read_number(throttle_indicator, "Value")),
        tostring(read_number(steering_indicator, "MinValue")),
        tostring(read_number(steering_indicator, "MaxValue")),
        tostring(read_number(steering_indicator, "Value"))
    ))

    local ok, err = pcall(function()
        -- Both sliders use UMG property bindings. Slate evaluates these bindings
        -- during painting and otherwise replaces every SetValue call with the
        -- original raw keyboard axis. A single-cast delegate is unbound by
        -- assigning nil in UE4SS.
        throttle_indicator.ValueDelegate = nil
        steering_indicator.ValueDelegate = nil
        if valid(throttle_text_indicator) then throttle_text_indicator.TextDelegate = nil end
        if valid(steering_text_indicator) then steering_text_indicator.TextDelegate = nil end
    end)
    if not ok then
        if not reported_binding_error then
            reported_binding_error = true
            log(string.format("failed to clear HUD ValueDelegate bindings: %s", tostring(err)))
        end
        return false
    end

    indicator_bindings_cleared = true
    local throttle_after_ok, throttle_after = pcall(function() return throttle_indicator.ValueDelegate end)
    local steering_after_ok, steering_after = pcall(function() return steering_indicator.ValueDelegate end)
    log(string.format(
        "HUD ValueDelegate bindings cleared; after throttle=%s steering=%s",
        throttle_after_ok and describe_delegate(throttle_after) or "read-error",
        steering_after_ok and describe_delegate(steering_after) or "read-error"
    ))
    apply_cached_indicators()
    return true
end

local function bind_hud_indicators(hud)
    if not valid(hud) then return false end
    if valid(throttle_indicator) and valid(steering_indicator)
        and valid(throttle_text_indicator) and valid(steering_text_indicator) then
        clear_indicator_bindings()
        return true
    end

    local ok, err = pcall(function()
        throttle_indicator = hud.ThrottleSlider
        steering_indicator = hud.SteeringSlider
        throttle_text_indicator = hud.ThrottleTB
        steering_text_indicator = hud.SteeringTB
    end)
    if not ok then
        log("direct HUD indicator binding failed: " .. tostring(err))
        return false
    end
    indicator_bindings_cleared = false
    log(string.format(
        "direct HUD indicators acquired: throttle=%s steering=%s throttleText=%s steeringText=%s",
        full_name(throttle_indicator), full_name(steering_indicator),
        full_name(throttle_text_indicator), full_name(steering_text_indicator)
    ))
    report_delegate_source(throttle_text_indicator, "TextDelegate")
    report_delegate_source(steering_text_indicator, "TextDelegate")
    return clear_indicator_bindings()
end

local function inspect_and_hook_hud(widget)
    if hud_layout_dumped or not valid(widget) then return end
    local outer_ok, widget_tree = pcall(function() return widget:GetOuter() end)
    if not outer_ok or not valid(widget_tree) then return end
    local root_ok, root = pcall(function() return widget_tree:GetOuter() end)
    if not root_ok or not valid(root) then return end
    local class_ok, class = pcall(function() return root:GetClass() end)
    if not class_ok or not valid(class) then return end

    hud_layout_dumped = true
    log(string.format("HUD root: %s", full_name(root)))
    class:ForEachFunction(function(fn)
        local function_name = full_name(fn)
        log(string.format("HUD function: %s", function_name))
    end)
end

local function find_indicator_widgets()
    if valid(throttle_indicator) and valid(steering_indicator) then
        clear_indicator_bindings()
        return
    end

    -- The HUD is recreated whenever the player enters the vehicle. Do not
    -- retain the unbound state from an old, destroyed WidgetTree.
    throttle_indicator = nil
    steering_indicator = nil
    indicator_bindings_cleared = false
    if indicator_search_cooldown > 0 then
        indicator_search_cooldown = indicator_search_cooldown - 1
        return
    end
    indicator_search_cooldown = 60

    local ok, widgets = pcall(FindAllOf, "Widget")
    if not ok or widgets == nil then return end
    for _, widget in pairs(widgets) do
        if valid(widget) then
            local name = string.lower(full_name(widget))
            if string.find(name, "bp_voyageingameforklift", 1, true)
                and (string.find(name, "throttle", 1, true) or string.find(name, "steer", 1, true)) then
                if not reported_indicator_widgets[name] then
                    reported_indicator_widgets[name] = true
                    log(string.format("HUD indicator candidate: %s", full_name(widget)))
                end
                if string.find(name, "/engine/transient.", 1, true) then
                    if string.find(name, "throttleslider", 1, true) then throttle_indicator = widget end
                    if string.find(name, "steeringslider", 1, true) then steering_indicator = widget end
                    if string.find(name, "throttletb", 1, true) then
                        report_delegate_source(widget, "TextDelegate")
                    end
                    if string.find(name, "steeringtb", 1, true) then
                        report_delegate_source(widget, "TextDelegate")
                    end
                end
            end
        end
    end
    if valid(throttle_indicator) then inspect_and_hook_hud(throttle_indicator) end
    clear_indicator_bindings()
end

local function update_indicator_widgets(throttle, steering)
    find_indicator_widgets()
    apply_cached_indicators()
end

local function mark_keyboard_input()
    if not keyboard_seen then
        keyboard_seen = true
        log("keyboard mode activated")
    end
end

local function mark_throttle_input()
    mark_keyboard_input()
    throttle_zero_requested = false
    fresh_keyboard_press.throttle = true
end

local function mark_steering_input()
    mark_keyboard_input()
    steering_zero_requested = false
    fresh_keyboard_press.steering = true
end

RegisterKeyBind(Key.W, mark_throttle_input)
RegisterKeyBind(Key.S, mark_throttle_input)
RegisterKeyBind(Key.A, mark_steering_input)
RegisterKeyBind(Key.D, mark_steering_input)

RegisterKeyBind(Key.X, function()
    mark_keyboard_input()
    throttle_zero_requested = true
    log("smooth throttle return to zero requested")
end)

RegisterKeyBind(Key.C, function()
    mark_keyboard_input()
    steering_zero_requested = true
    log("smooth steering return to center requested")
end)

RegisterKeyBind(Key.F7, function()
    smoothing_enabled = not smoothing_enabled
    states = {}
    raw_commands = {}
    fresh_keyboard_press = { throttle = false, steering = false }
    throttle_zero_requested = false
    steering_zero_requested = false
    current_hud_throttle = nil
    current_hud_steering = nil
    log(string.format("smoothing %s", smoothing_enabled and "enabled" or "disabled"))
end)

local update_native_hud

local function update_control(context, delta_seconds)
    local actor = unwrap(context)
    if not valid(actor) then return end

    local actor_name = full_name(actor)
    if not is_player_controlled(actor) then
        states[actor_name] = nil
        raw_commands[actor_name] = nil
        fresh_keyboard_press.throttle = false
        fresh_keyboard_press.steering = false
        current_hud_throttle = nil
        current_hud_steering = nil
        return
    end
    if not smoothing_enabled or not keyboard_seen then
        states[actor_name] = nil
        current_hud_throttle = nil
        current_hud_steering = nil
        return
    end

    local throttle_ok, raw_throttle, steering_ok, raw_steering = read_raw_vehicle_inputs(actor)
    if not throttle_ok or not steering_ok
        or type(raw_throttle) ~= "number" or type(raw_steering) ~= "number" then return end

    local command = raw_commands[actor_name]
    if command ~= nil then
        if type(command.throttle) == "number" then raw_throttle = command.throttle end
        if type(command.steering) == "number" then raw_steering = command.steering end
    end

    local dt = unwrap(delta_seconds)
    if type(dt) ~= "number" or dt <= 0 then dt = 1 / 60 end
    dt = math.min(dt, CONFIG.maximum_delta_time)

    local state = states[actor_name]
    if state == nil then
        state = {
            throttle = read_number(actor, "Acceleration") or 0,
            steering = read_number(actor, "Steering") or 0,
            steering_velocity = 0,
            throttle_direction = 0,
            steering_direction = 0,
        }
        states[actor_name] = state
        log(string.format("control state initialized for %s", actor_name))
    end

    local throttle_direction = direction(raw_throttle)
    local steering_direction = direction(raw_steering)
    if throttle_direction ~= state.throttle_direction then
        state.throttle_direction = throttle_direction
        log(string.format("throttle key=%d value=%.3f", throttle_direction, state.throttle))
    end
    if steering_direction ~= state.steering_direction then
        state.steering_direction = steering_direction
        log(string.format("steering key=%d value=%.3f", steering_direction, state.steering))
    end

    if throttle_zero_requested then
        state.throttle = approach(state.throttle, 0, CONFIG.throttle_braking_rate * dt)
        if math.abs(state.throttle) < 0.0001 then
            state.throttle = 0
            throttle_zero_requested = false
            log("throttle returned to zero")
        end
    else
        state.throttle = approach_with_direction(
            state.throttle,
            throttle_direction,
            dt,
            CONFIG.throttle_acceleration_rate,
            CONFIG.throttle_braking_rate
        )
    end

    if steering_zero_requested then
        if math.abs(state.steering) < 0.0001 then
            state.steering = 0
            state.steering_velocity = 0
            steering_zero_requested = false
            log("steering returned to center")
        else
            local centering_direction = state.steering > 0 and -1 or 1
            local target_velocity = centering_direction * CONFIG.steering_maximum_speed
            local acceleration = state.steering_velocity * centering_direction < 0
                and CONFIG.steering_reversal_braking
                or CONFIG.steering_acceleration
            state.steering_velocity = approach(
                state.steering_velocity,
                target_velocity,
                acceleration * dt
            )
            local previous_steering = state.steering
            state.steering = state.steering + state.steering_velocity * dt
            if previous_steering * state.steering <= 0 then
                state.steering = 0
                state.steering_velocity = 0
                steering_zero_requested = false
                log("steering returned to center")
            end
        end
    elseif steering_direction == 0 then
        state.steering_velocity = 0
    else
        local target_velocity = steering_direction * CONFIG.steering_maximum_speed
        local acceleration = CONFIG.steering_acceleration
        if state.steering_velocity * steering_direction < 0 then
            acceleration = CONFIG.steering_reversal_braking
        end
        state.steering_velocity = approach(
            state.steering_velocity,
            target_velocity,
            acceleration * dt
        )
        state.steering = clamp(state.steering + state.steering_velocity * dt, -1, 1)
        if (state.steering <= -1 and state.steering_velocity < 0)
            or (state.steering >= 1 and state.steering_velocity > 0) then
            state.steering_velocity = 0
        end
    end

    actor.Acceleration = state.throttle
    actor.Steering = state.steering
    actor.ThrottleInput = 0
    actor.SteeringInput = 0
    current_hud_throttle = state.throttle
    current_hud_steering = state.steering
    if not input_fields_reported then
        input_fields_reported = true
        log("smoothed forklift control is active")
    end
    update_native_hud()
end

local function find_vehicle_hud()
    if valid(vehicle_hud) then return vehicle_hud end
    vehicle_hud = nil
    if vehicle_hud_search_cooldown > 0 then
        vehicle_hud_search_cooldown = vehicle_hud_search_cooldown - 1
        return nil
    end
    vehicle_hud_search_cooldown = 60

    local ok, widgets = pcall(FindAllOf, "VoyageInGameVehicleWidget")
    if not ok or widgets == nil then return nil end
    for _, widget in pairs(widgets) do
        if valid(widget) then
            local name = string.lower(full_name(widget))
            if string.find(name, "bp_voyageingameforklift_c", 1, true)
                and string.find(name, "/engine/transient.", 1, true) then
                vehicle_hud = widget
                log(string.format("native forklift HUD acquired: %s", full_name(widget)))
                return vehicle_hud
            end
        end
    end
    return nil
end

local function loaded_object(asset_path, object_path)
    pcall(LoadAsset, asset_path)
    local ok, object = pcall(StaticFindObject, object_path)
    return ok and object or nil
end

local function dump_hint_function_signatures()
    if hint_signature_dumped then return end
    hint_signature_dumped = true
    local ok, functions = pcall(FindAllOf, "Function")
    if not ok or functions == nil then return end
    for _, fn in pairs(functions) do
        local name = string.lower(full_name(fn))
        if string.find(name, "voyagebuttoninfocontainerwidget:setinputs", 1, true)
            or string.find(name, "voyagebuttoninfocontainerwidget:setactions", 1, true) then
            log("hint signature: " .. full_name(fn))
            pcall(function()
                fn:ForEachProperty(function(parameter)
                    local class_name = "<unknown>"
                    local flags = "?"
                    pcall(function() class_name = parameter:GetClass():GetFName():ToString() end)
                    pcall(function() flags = tostring(parameter:GetPropertyFlags()) end)
                    log(string.format(
                        "  hint parameter: %s %s flags=%s",
                        class_name, full_name(parameter), flags
                    ))
                end)
            end)
        end
    end
end

local function create_zero_hint(library, player_controller, widget_class, context_asset, action, parent, translation)
    local widget = library:Create(player_controller, widget_class, player_controller)
    if not valid(widget) then return nil end
    widget.bShowActionTexts = false
    widget.bShowBackground = true
    -- SetInputs takes maps with localized FText values, not an action array.
    -- The simpler SetActions API accepts the array we need and avoids creating
    -- FText from Lua (unsupported by this UE4SS build).
    widget.ContextAsset = context_asset
    widget:SetActions({ action })

    local slot = parent:AddChild(widget)
    if valid(slot) then
        pcall(function() slot:SetHorizontalAlignment(1) end)
        pcall(function() slot:SetVerticalAlignment(1) end)
    end
    widget:SetRenderScale({ X = 0.55, Y = 0.55 })
    widget:SetRenderTranslation(translation)
    return widget
end

local function ensure_zero_hints(hud)
    dump_hint_function_signatures()
    local hud_name = full_name(hud)
    if hint_hud_name == hud_name and valid(throttle_zero_hint) and valid(steering_zero_hint) then return end
    if hint_hud_name == hud_name and hint_creation_error_reported then return end
    hint_hud_name = hud_name
    throttle_zero_hint = nil
    steering_zero_hint = nil

    local ok, err = pcall(function()
        local container_class = loaded_object(
            "/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container",
            "/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container.BP_ButtonInfo_Vertical_Container_C"
        )
        local context_asset = loaded_object(
            "/Game/Game/Input/Cameraman/DA_Input_Context_Cameraman",
            "/Game/Game/Input/Cameraman/DA_Input_Context_Cameraman.DA_Input_Context_Cameraman"
        )
        local x_action = loaded_object(
            "/Game/Game/Input/Cameraman/IA_ToggleGizmoAxis",
            "/Game/Game/Input/Cameraman/IA_ToggleGizmoAxis.IA_ToggleGizmoAxis"
        )
        local c_action = loaded_object(
            "/Game/Game/Input/Cameraman/IA_Cameraman_MoveUp",
            "/Game/Game/Input/Cameraman/IA_Cameraman_MoveUp.IA_Cameraman_MoveUp"
        )
        local library = StaticFindObject("/Script/UMG.Default__WidgetBlueprintLibrary")
        local player_controller = hud:GetOwningPlayer()
        local parent = hud.SpeedometerRoot
        if not valid(container_class) or not valid(context_asset)
            or not valid(x_action) or not valid(c_action) or not valid(library)
            or not valid(player_controller) or not valid(parent) then
            error("one or more hint assets/owners were not found")
        end

        throttle_zero_hint = create_zero_hint(
            library, player_controller, container_class, context_asset, x_action, parent,
            { X = 18, Y = 112 }
        )
        steering_zero_hint = create_zero_hint(
            library, player_controller, container_class, context_asset, c_action, parent,
            { X = 124, Y = 188 }
        )
        if not valid(throttle_zero_hint) or not valid(steering_zero_hint) then
            error("the native hint widgets could not be created")
        end
        log("native X/C zero-position hints added to the forklift HUD")
    end)
    if not ok and not hint_creation_error_reported then
        hint_creation_error_reported = true
        log("zero-position hint creation failed: " .. tostring(err))
    end
end

update_native_hud = function()
    if current_hud_throttle == nil or current_hud_steering == nil then return end
    local hud = find_vehicle_hud()
    if not valid(hud) then return end
    ensure_zero_hints(hud)

    local ok, err = pcall(function()
        -- These are the game's native update functions. They update both the
        -- numeric text and the matching slider without constructing FText in Lua.
        hud:OnUpdateThrottle(current_hud_throttle)
        hud:OnUpdateSteering(current_hud_steering)
    end)
    if ok then
        if not native_hud_update_reported then
            native_hud_update_reported = true
            log("native throttle/steering HUD updates active")
        end
    elseif not native_hud_error_reported then
        native_hud_error_reported = true
        log(string.format("native HUD update failed: %s", tostring(err)))
    end
end

local function override_native_hud_throttle(context, value)
    if current_hud_throttle == nil then return end
    value:set(current_hud_throttle)
    if not native_hud_throttle_intercept_reported then
        native_hud_throttle_intercept_reported = true
        log("native OnUpdateThrottle input interception active")
    end
end

local function override_native_hud_steering(context, value)
    if current_hud_steering == nil then return end
    value:set(current_hud_steering)
    if not native_hud_steering_intercept_reported then
        native_hud_steering_intercept_reported = true
        log("native OnUpdateSteering input interception active")
    end
end

local function try_register_native_hud_hooks()
    if native_hud_hooks_registered or native_hud_hook_registration_pending then return end
    native_hud_hook_registration_pending = true
    ExecuteInGameThread(function()
        local throttle_ok, throttle_pre, throttle_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageInGameVehicleWidget:OnUpdateThrottle",
            override_native_hud_throttle
        )
        local steering_ok, steering_pre, steering_post = pcall(
            RegisterHook,
            "/Script/Voyage.VoyageInGameVehicleWidget:OnUpdateSteering",
            override_native_hud_steering
        )
        native_hud_hook_registration_pending = false
        if throttle_ok and steering_ok then
            native_hud_hooks_registered = true
            log(string.format(
                "native HUD input hooks registered (throttle=%s/%s steering=%s/%s)",
                tostring(throttle_pre), tostring(throttle_post),
                tostring(steering_pre), tostring(steering_post)
            ))
        else
            log("native HUD input hook registration failed; will retry")
        end
    end)
end

local tick_path = "/Game/Blueprints/Vehicles/BP_Forklift_Possesable.BP_Forklift_Possesable_C:ReceiveTick"
local function try_register_tick_hook()
    if tick_hook_registered or registration_pending then return end
    registration_pending = true
    ExecuteInGameThread(function()
        local ok, pre_id, post_id = pcall(RegisterHook, tick_path, update_control, update_native_hud)
        registration_pending = false
        if ok then
            tick_hook_registered = true
            log(string.format("post-tick control hook registered (pre=%s post=%s)", tostring(pre_id), tostring(post_id)))
        end
    end)
end

RegisterBeginPlayPostHook(function(context)
    local actor = unwrap(context)
    if valid(actor) and string.find(string.lower(full_name(actor)), "bp_forklift_possesable_c", 1, true) then
        try_register_tick_hook()
    end
end)

LoopAsync(500, function()
    if tick_hook_registered and input_getter_hooks_registered
        and input_setter_hooks_registered and native_hud_hooks_registered then return true end
    try_register_tick_hook()
    try_register_input_getter_hooks()
    try_register_input_setter_hooks()
    try_register_native_hud_hooks()
    return false
end)

log("loaded; waiting for DonkLift Blueprint; F7 toggles smoothing")

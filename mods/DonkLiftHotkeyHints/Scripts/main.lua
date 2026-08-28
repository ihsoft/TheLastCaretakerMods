local PREFIX = "[DonkLiftHotkeyHints]"
local POLL_INTERVAL_MS = 1000
local LABELS = {
    en = { brake = "Brake", center = "Center" },
    ru = { brake = "Тормоз", center = "Выровнять" },
}
local LANGUAGE_BY_ENUM_VALUE = {
    [1] = "en",  -- English
    [11] = "ru", -- Russian
}

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function valid(object)
    return object ~= nil and object:IsValid()
end

local widget_library = nil
local text_library = nil
local internationalization_library = nil
local last_error = nil
local active_language = nil
local configured_roots = {}

local function resolve_libraries()
    if not valid(widget_library) then
        widget_library = StaticFindObject("/Script/UMG.Default__WidgetBlueprintLibrary")
    end
    if not valid(text_library) then
        text_library = StaticFindObject("/Script/Engine.Default__KismetTextLibrary")
    end
    if not valid(internationalization_library) then
        internationalization_library = StaticFindObject(
            "/Script/Engine.Default__KismetInternationalizationLibrary"
        )
    end
    return valid(widget_library) and valid(text_library)
end

local function find_game_settings()
    local settings_objects = FindAllOf("VoyageGameUserSettings")
    if settings_objects == nil then return nil end
    for _, settings in pairs(settings_objects) do
        if valid(settings) and not settings:GetFullName():find("Default__", 1, true) then
            return settings
        end
    end
    return nil
end

local function resolve_language_once()
    if active_language ~= nil then return true end
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
        language = LANGUAGE_BY_ENUM_VALUE[tonumber(value) or tonumber(raw)]
    end

    -- The System value and languages not translated by this mod use the
    -- engine language when possible, then English as the final fallback.
    if language == nil and valid(internationalization_library) then
        local current = internationalization_library:GetCurrentLanguage()
        if current ~= nil then
            local engine_language = tostring(current):lower():match("^([a-z]+)")
            if LABELS[engine_language] ~= nil then language = engine_language end
        end
    end

    active_language = language or "en"
    return true
end

local function set_text(text_block, value)
    text_block:SetText(text_library:Conv_StringToText(value))
end

local function create_hint(hud, root, template_indicator)
    local owner = hud:GetOwningPlayer()
    local indicator = widget_library:Create(hud, template_indicator:GetClass(), owner)
    if not valid(indicator) then error("WidgetBlueprintLibrary.Create returned no widget") end
    root:AddChild(indicator)
end

local function configure_hint(indicator, key, description)
    if not valid(indicator) then return false end
    local container = indicator.ButtonInfoContainer
    if not valid(container) then return false end
    local panel = container.RootPanel
    if not valid(panel) or panel:GetChildrenCount() < 1 then return false end
    local button_info = panel:GetChildAt(0)
    if not valid(button_info)
        or not valid(button_info.PrimaryKeyTB)
        or not valid(button_info.DescriptionTB)
        or not valid(button_info.SecondaryKeyTB)
        or not valid(button_info.DescriptionErrorTB) then
        return false
    end

    -- This copy is display-only. Its template action is IAV_Interact; prevent a
    -- later key-rebinding refresh from restoring that action's E/Interact text.
    container.bAutoUpdateKeyRebindings = false
    set_text(button_info.PrimaryKeyTB, key)
    set_text(button_info.DescriptionTB, description)
    set_text(button_info.SecondaryKeyTB, "")
    set_text(button_info.DescriptionErrorTB, "")
    return true
end

local function add_missing_hints(hud, language, labels)
    local dynamic_widget = hud.BP_DynamicPlayerInputHorizontalWidget_Bottom
    local root = dynamic_widget.ContextInputActionsRoot
    if not valid(root) then return end

    local count = root:GetChildrenCount()
    if count < 2 then return end
    local root_name = root:GetFullName()
    if count < 4 then configured_roots[root_name] = nil end
    local template_indicator = root:GetChildAt(0)
    if not valid(template_indicator) then return end

    if count == 2 then
        create_hint(hud, root, template_indicator)
        count = root:GetChildrenCount()
    end
    if count == 3 then
        create_hint(hud, root, template_indicator)
        count = root:GetChildrenCount()
    end
    if count >= 4 and configured_roots[root_name] ~= language then
        local x_ready = configure_hint(root:GetChildAt(2), "X", labels.brake)
        local c_ready = configure_hint(root:GetChildAt(3), "C", labels.center)
        if x_ready and c_ready then
            configured_roots[root_name] = language
        end
    end
end

local function update_huds()
    if not resolve_libraries() then error("required UMG/text libraries are unavailable") end
    local huds = FindAllOf("BP_VoyageIngameForklift_C")
    local has_valid_hud = false
    if huds ~= nil then
        for _, hud in pairs(huds) do
            if valid(hud) then
                has_valid_hud = true
                break
            end
        end
    end

    -- A return to the menu destroys the forklift HUD. Clear session state so
    -- the language selected in the menu is read again on the next game load.
    if not has_valid_hud then
        if active_language ~= nil then
            active_language = nil
            configured_roots = {}
        end
        return
    end

    if not resolve_language_once() then return end
    local labels = LABELS[active_language]
    for _, hud in pairs(huds) do
        if valid(hud) then add_missing_hints(hud, active_language, labels) end
    end
end

LoopAsync(POLL_INTERVAL_MS, function()
    ExecuteInGameThread(function()
        local ok, err = pcall(update_huds)
        if not ok then
            local message = tostring(err)
            if message ~= last_error then
                last_error = message
                log("HUD update failed: " .. message)
            end
        else
            last_error = nil
        end
    end)
    return false
end)

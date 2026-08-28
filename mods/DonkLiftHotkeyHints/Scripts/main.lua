local PREFIX = "[DonkLiftHotkeyHints]"
local POLL_INTERVAL_MS = 1000

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function valid(object)
    return object ~= nil and object:IsValid()
end

local widget_library = nil
local text_library = nil
local last_error = nil
local configured_roots = {}

local function resolve_libraries()
    if not valid(widget_library) then
        widget_library = StaticFindObject("/Script/UMG.Default__WidgetBlueprintLibrary")
    end
    if not valid(text_library) then
        text_library = StaticFindObject("/Script/Engine.Default__KismetTextLibrary")
    end
    return valid(widget_library) and valid(text_library)
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

local function add_missing_hints(hud)
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
    if count >= 4 and not configured_roots[root_name] then
        local x_ready = configure_hint(root:GetChildAt(2), "X", "Reset throttle")
        local c_ready = configure_hint(root:GetChildAt(3), "C", "Center steering")
        if x_ready and c_ready then
            configured_roots[root_name] = true
            log("added X/C hints to the native bottom input row")
        end
    end
end

local function update_huds()
    if not resolve_libraries() then error("required UMG/text libraries are unavailable") end
    local huds = FindAllOf("BP_VoyageIngameForklift_C")
    if huds == nil then return end
    for _, hud in pairs(huds) do
        if valid(hud) then add_missing_hints(hud) end
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

log("loaded")

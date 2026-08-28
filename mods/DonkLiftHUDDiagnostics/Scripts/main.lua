local PREFIX = "[DonkLiftHUDDiagnostics]"

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
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

local function short_class(object)
    local result = "<unknown>"
    pcall(function() result = object:GetClass():GetFName():ToString() end)
    return result
end

local function field_description(field)
    local class_name, offset, flags, array_dim = "<unknown>", "?", "?", "?"
    pcall(function() class_name = field:GetClass():GetFName():ToString() end)
    pcall(function() offset = string.format("0x%04X", field:GetOffset_Internal()) end)
    pcall(function() flags = tostring(field:GetPropertyFlags()) end)
    pcall(function() array_dim = tostring(field:GetArrayDim()) end)
    return string.format("%s %s offset=%s array=%s flags=%s",
        class_name, full_name(field), offset, array_dim, flags)
end

local function dump_functions(label, class)
    if not valid(class) then
        log(label .. ": class not found")
        return
    end
    log(label .. ": " .. full_name(class))
    local ok, err = pcall(function()
        class:ForEachFunction(function(fn)
            log("  function: " .. full_name(fn))
            fn:ForEachProperty(function(parameter)
                log("    parameter: " .. field_description(parameter))
            end)
        end)
    end)
    if not ok then log("  function reflection failed: " .. tostring(err)) end
end

local function dump_object_array(label, array)
    if array == nil then
        log(label .. ": <nil>")
        return
    end
    local count = 0
    local ok, err = pcall(function()
        count = #array
        for index = 1, count do
            log(string.format("%s[%d]: %s", label, index, full_name(array[index])))
        end
    end)
    if not ok then log(label .. ": read failed: " .. tostring(err)) end
    log(string.format("%s count=%d", label, count))
end

local function text_block_value(widget)
    local result = "<unavailable>"
    pcall(function() result = widget:GetText():ToString() end)
    return result
end

local function dump_button_info(widget, indent)
    pcall(function() log(indent .. "InputAction = " .. full_name(widget.InputAction)) end)
    for _, property_name in ipairs({"PrimaryKeyTB", "SecondaryKeyTB", "DescriptionTB", "DescriptionErrorTB"}) do
        pcall(function()
            local text_block = widget[property_name]
            log(string.format("%s%s = %s (%s)", indent, property_name,
                text_block_value(text_block), full_name(text_block)))
        end)
    end
end

local function dump_panel(label, panel, depth)
    depth = depth or 0
    if not valid(panel) then
        log(label .. ": <nil or invalid>")
        return
    end
    local indent = string.rep("  ", depth)
    local visibility = "?"
    pcall(function() visibility = tostring(panel:GetVisibility()) end)
    log(string.format("%s%s: %s class=%s visibility=%s",
        indent, label, full_name(panel), short_class(panel), visibility))
    dump_button_info(panel, indent .. "  ")
    if depth >= 5 then return end
    local ok, count = pcall(function() return panel:GetChildrenCount() end)
    if not ok then return end
    log(string.format("%s  children=%d", indent, count))
    for index = 0, count - 1 do
        local child_ok, child = pcall(function() return panel:GetChildAt(index) end)
        if child_ok then dump_panel("child[" .. index .. "]", child, depth + 1) end
    end
end


local dump_container

local function dump_indicator_children(label, dynamic_widget)
    local root = dynamic_widget.ContextInputActionsRoot
    if not valid(root) then return end
    local count = root:GetChildrenCount()
    for index = 0, count - 1 do
        local indicator = root:GetChildAt(index)
        log(string.format("%s indicator[%d]: %s", label, index, full_name(indicator)))
        pcall(function() log("  DefaultText = " .. tostring(indicator.DefaultText)) end)
        pcall(function() dump_container("  ButtonInfoContainer", indicator.ButtonInfoContainer, false) end)
    end
end

dump_container = function(label, container, allow_nested)
    if not valid(container) then
        log(label .. ": <nil or invalid>")
        return
    end
    log(label .. ": " .. full_name(container))
    local properties = {
        "ContextAsset", "RootPanel", "ContextInputActionsRoot",
        "ButtonInfoContainer", "ButtonInfoWidgetClass", "InputActionType",
        "bFilterByActionType", "bPreferIndicatorWidgets", "bShowActionTexts",
        "bShowBackground", "ShowPrimaryKey", "ShowSecondaryKey"
    }
    for _, property_name in ipairs(properties) do
        pcall(function()
            log(string.format("  %s = %s", property_name, full_name(container[property_name])))
        end)
    end
    pcall(function() dump_object_array("  InputActions", container.InputActions) end)
    pcall(function() dump_panel("  RootPanel tree", container.RootPanel, 1) end)
    pcall(function() dump_panel("  ContextInputActionsRoot tree", container.ContextInputActionsRoot, 1) end)
    if allow_nested then
        pcall(function() dump_container("  nested ButtonInfoContainer", container.ButtonInfoContainer, false) end)
    end
end

local function inspect_live_hud()
    local ok, widgets = pcall(FindAllOf, "BP_VoyageIngameForklift_C")
    if not ok or widgets == nil then
        log("FindAllOf(BP_VoyageIngameForklift_C) failed")
        return
    end
    local count = 0
    for _, hud in pairs(widgets) do
        if valid(hud) then
            count = count + 1
            log("live forklift HUD: " .. full_name(hud))
            pcall(function() dump_panel("KeybindRoot", hud.KeybindRoot, 0) end)
            pcall(function() dump_container("static action container", hud.BP_ButtonInfoContainer_Action, false) end)
            pcall(function() dump_container("dynamic bottom", hud.BP_DynamicPlayerInputHorizontalWidget_Bottom, true) end)
            pcall(function() dump_container("dynamic center", hud.BP_DynamicPlayerInputHorizontalWidget_Center, true) end)
            pcall(function() dump_indicator_children("dynamic bottom", hud.BP_DynamicPlayerInputHorizontalWidget_Bottom) end)
            pcall(function() dump_indicator_children("dynamic center", hud.BP_DynamicPlayerInputHorizontalWidget_Center) end)
        end
    end
    log(string.format("live forklift HUD count=%d", count))
end

local function inspect()
    log("inspection begin")
    dump_functions("VoyageButtonInfoContainerWidget functions",
        StaticFindObject("/Script/Voyage.VoyageButtonInfoContainerWidget"))
    dump_functions("VoyageButtonInfoWidget functions",
        StaticFindObject("/Script/Voyage.VoyageButtonInfoWidget"))
    dump_functions("VoyageDynamicPlayerInputWidget functions",
        StaticFindObject("/Script/Voyage.VoyageDynamicPlayerInputWidget"))
    inspect_live_hud()
    log("inspection end")
end

RegisterKeyBind(Key.F8, function()
    ExecuteInGameThread(inspect)
end)

log("loaded; enter DonkLift and press F8 once")

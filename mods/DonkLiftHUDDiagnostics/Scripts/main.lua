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

local function field_description(field)
    local class_name = "<unknown>"
    local offset = "?"
    local flags = "?"
    pcall(function() class_name = field:GetClass():GetFName():ToString() end)
    pcall(function() offset = string.format("0x%04X", field:GetOffset_Internal()) end)
    pcall(function() flags = tostring(field:GetPropertyFlags()) end)
    return string.format("%s %s offset=%s flags=%s", class_name, full_name(field), offset, flags)
end

local function dump_struct(label, struct)
    if not valid(struct) then
        log(label .. ": not found")
        return
    end
    log(label .. ": " .. full_name(struct))
    local current = struct
    local depth = 0
    while valid(current) and depth < 12 do
        log(string.format("class[%d]: %s", depth, full_name(current)))
        local ok, err = pcall(function()
            current:ForEachProperty(function(property)
                log("  property: " .. field_description(property))
            end)
            current:ForEachFunction(function(fn)
                log("  function: " .. full_name(fn))
                local param_ok, param_err = pcall(function()
                    fn:ForEachProperty(function(parameter)
                        log("    parameter: " .. field_description(parameter))
                    end)
                end)
                if not param_ok then log("    parameter scan failed: " .. tostring(param_err)) end
            end)
        end)
        if not ok then log("  reflection failed: " .. tostring(err)) end
        local super_ok, super = pcall(function() return current:GetSuperStruct() end)
        if not super_ok then break end
        current = super
        depth = depth + 1
    end
end

local function dump_matching_functions()
    local ok, functions = pcall(FindAllOf, "Function")
    if not ok or functions == nil then
        log("FindAllOf(Function) failed: " .. tostring(functions))
        return
    end
    local count = 0
    for _, fn in pairs(functions) do
        local name = full_name(fn)
        local lower = string.lower(name)
        if string.find(lower, "voyageingamevehiclewidget", 1, true)
            or string.find(lower, "voyageingameforkliftwidget", 1, true) then
            count = count + 1
            log("matching function: " .. name)
            pcall(function()
                fn:ForEachProperty(function(parameter)
                    log("  parameter: " .. field_description(parameter))
                end)
            end)
        end
    end
    log(string.format("matching functions found: %d", count))
end

local function dump_instances()
    for _, class_name in ipairs({"VoyageInGameVehicleWidget", "VoyageIngameForkliftWidget"}) do
        local ok, objects = pcall(FindAllOf, class_name)
        local count = 0
        if ok and objects ~= nil then
            for _, object in pairs(objects) do
                if valid(object) then
                    count = count + 1
                    log(string.format("instance %s: %s", class_name, full_name(object)))
                    local class_ok, class = pcall(function() return object:GetClass() end)
                    if class_ok then dump_struct("live class", class) end
                end
            end
        end
        log(string.format("instances %s: %d", class_name, count))
    end
end

local function inspect()
    log("inspection begin")
    dump_struct("native vehicle HUD class", StaticFindObject("/Script/Voyage.VoyageInGameVehicleWidget"))
    dump_struct("native forklift HUD class", StaticFindObject("/Script/Voyage.VoyageIngameForkliftWidget"))
    dump_matching_functions()
    dump_instances()
    log("inspection end")
end

RegisterKeyBind(Key.F8, function()
    ExecuteInGameThread(inspect)
end)

log("loaded; enter DonkLift and press F8 once")

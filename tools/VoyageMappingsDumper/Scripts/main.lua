local PREFIX = "[VoyageMappingsDumper]"
local dump_started = false

local function log(message)
    print(string.format("%s %s\n", PREFIX, message))
end

local function start_dump()
    if dump_started then return end
    dump_started = true

    ExecuteInGameThread(function()
        if type(DumpUSMAP) ~= "function" then
            log("DumpUSMAP is unavailable in this UE4SS build")
            return
        end

        log("starting one-time Mappings.usmap dump")
        local ok, result = pcall(DumpUSMAP)
        if ok then
            log(string.format("DumpUSMAP returned: %s", tostring(result)))
        else
            log(string.format("DumpUSMAP failed: %s", tostring(result)))
        end
    end)
end

-- Let the game finish loading native and Blueprint reflection data first.
ExecuteWithDelay(12000, start_dump)

log("loaded; mappings dump scheduled in 12 seconds")

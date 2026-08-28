-- Voyage 5.7.4 (UE 5.7): FText(FString&&).
-- The stock UE4SS signature does not match this executable. Keep the direct
-- call displacement in the pattern: this executable contains a second,
-- byte-identical constructor body, and the displacement makes this match
-- unique without guessing between the two addresses.
function Register()
    return "40 53 57 48 83 EC 38 48 89 6C 24 58 48 8B FA 48 89 74 24 60 48 8B D9 33 F6 4C 89 74 24 30 89 74 24 50 83 7A 08 01 7F 34 E8 63 22 00 00 48 8B F0 48 8B 38 48 85 FF 74 12"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end

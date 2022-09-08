local WaitSeconds = 60

print(string.format("Waiting %d seconds for player to settle.", WaitSeconds))
wait(WaitSeconds)

local Player = Game.Players.LocalPlayer
local PlayerID = string.gsub(Player.Name, "Player", "")
print("PlayerID", PlayerID)
if PlayerID % 2 then
	print(string.format("Waiting for %d seconds before adding memory pressure.\n", WaitSeconds))
	wait(WaitSeconds)
	
	settings().Network.ExtraMemoryUsed = 16000 -- MBytes
	print("Memory pressure added.")
else
	print("No memory pressure to be added.")
end

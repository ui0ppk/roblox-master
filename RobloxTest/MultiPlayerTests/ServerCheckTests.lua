local MinMoveDistReq = 1.0
local MaxRunTime = Game.TestService.Timeout - 60
local WaitPeriod = 5

-- Give time for players to get in the game
RBX_MESSAGE("Waiting for clients to enter.")
wait(20)
RBX_MESSAGE("Beginning tests.")

local PlayerPositions = {}
for i = 1,((MaxRunTime/WaitPeriod)+1) do
	for id = 1,Game.TestService.NumberOfPlayers do
		local Player = Game.Players[string.format("Player%d", id)]
		local Character = nil
		local Torso = nil
		
		if Player then
			Character = Player.Character
		end
		if Character then
			Torso = Character.Torso
		end
		if Torso then
			local Pos = Torso.Position
			RBX_MESSAGE(string.format("Player%d position: (%.2f, %.2f, %.2f)", id, Pos.x, Pos.y, Pos.z))
			
			local LastPos = PlayerPositions[Player.Name]
			if LastPos ~= nil then
				local Dist = (LastPos - Pos).magnitude
				success = Dist >= MinMoveDistReq
				RBX_CHECK_MESSAGE(success, string.format("%s has moved at least %f distance", Player.Name, MinMoveDistReq))
			end
			
			PlayerPositions[Player.Name] = Pos
		end
	end

	wait(WaitPeriod)
end

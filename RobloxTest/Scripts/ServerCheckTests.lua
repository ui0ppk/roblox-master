local MinMoveDistReq = 0.01
local StartTime = tick()
local WaitPeriod = 5

-- Give time for players to get in the game
RBX_MESSAGE("Waiting for clients to enter.")
wait(20)
RBX_MESSAGE("Beginning tests.")

local PlayerPositions = {}
while tick() - StartTime < Game.TestService.Timeout - 240 do
	for PlayerID = 1,Game.TestService.NumberOfPlayers do
		local Player = Game.Players:FindFirstChild(string.format("Player%d", PlayerID))
		local Character = nil
		local Torso = nil
		
		if Player then
			Character = Player.Character
		else
			RBX_CHECK_MESSAGE(false, string.format("Player%d is in game.", PlayerID))
		end
		if Character then
			Torso = Character.Torso
		end
		if Torso then
			local Pos = Torso.Position
			if _G.Debug then
				RBX_MESSAGE(string.format("Player%d position: (%.2f, %.2f, %.2f)", PlayerID, Pos.x, Pos.y, Pos.z))
			end
			
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

RBX_MESSAGE("Waiting for 180 seconds for clients to disconnect.")
wait(180)

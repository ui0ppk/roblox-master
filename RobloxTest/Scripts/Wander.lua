-- Our wanderer will always choose a point on a circle of this radius.
local WanderCircleRadius = 4.0

-- Number of seconds to wait before we start checking if there's ground in front of us.
local GroundCheckDelay = 0
-- Downwards stud distance we should check to see if there's ground at our intended destination
local GroundCheckDistance = 50

-- Information related to random spawns.
local SpawnChance = 0.0005 -- 0.05% chance to spawn elsewhere


-- Some helpful variables related to the player.
local Player = Game.Players.LocalPlayer
while Player.Character == nil do wait() end
local Character = Player.Character
local Humanoid = Character:WaitForChild("Humanoid")
local Torso = Humanoid.Torso

local PlayerID = string.gsub(Player.Name, "Player", "")

local Terrain = Game.Workspace.Terrain

-- Create a randomtable with precomputed random numbers.
-- We can't control the fact other people can call PRNG;
-- so, this table creates a reproducibly random set.
local function initrand(seed)
	randomstats = {}
	
	local randomtable = {}
	math.randomseed(seed)
	for i = 1,0xFFFF+1 do
		randomtable[i] = math.random()
	end
	
	randomstats.seed = seed
	randomstats.randomtable = randomtable
	randomstats.randomindex = 0
	randomstats.timescalled = 0
end

-- Choose the next precomputed random value and update our randomstats.
-- Cycle the random values once we reach the end.
local function random()
	randomstats.timescalled = randomstats.timescalled + 1
	randomstats.randomindex = 1 + randomstats.randomindex % #randomstats.randomtable
	return randomstats.randomtable[randomstats.randomindex]
end

-- Given a CurrentIndex and a Table, calculate the next valid index
-- in the Table using a circular indexing mechanism.
local function NextIndex(CurrentIndex, Table)
	return 1 + (CurrentIndex % #Table)
end

local function SetupTeleportLocations()
	TeleportLocations = {}
	TeleportLocationIdx = 0
	
	local Radius = 350
	local i = 1
	local j = 1
	-- Create 10 valid teleport locations in a circle.
	while i <= 10 and j <= 1000 do
		local Angle = random()
		local x = math.cos(Angle) * Radius
		local z = math.sin(Angle) * Radius
		local Origin = Vector3.new(x, 500, z)
		
		local ray = Ray.new(Origin, Vector3.new(0, -500, 0))
		local part, endPoint = Game.Workspace:FindPartOnRay(ray)
		if part ~= nil then
			TeleportLocations[i] = endPoint + Vector3.new(0, 3.0, 0)
			i = i + 1
		end
		j = j + 1
	end
	
	if #TeleportLocations < 1 then
		TeleportLocations[1] = Torso.Position
	end
end

local function TryTeleport(ForceTeleport)
	if ForceTeleport or random() <= SpawnChance then
		TeleportLocationIdx = NextIndex(TeleportLocationIdx, TeleportLocations)
		return TeleportLocations[TeleportLocationIdx]
	end
	
	return nil
end

-- project a circle at the player's location and find a random point on that circle
-- return the theta and the target location
--
-- theta can be passed back into this function if you want another target in
-- the same direction as before 
local function Wander(ChangeDirection, WanderTheta)
	if ChangeDirection then
		WanderTheta = random() * 2 * math.pi
	end
	local circleLocation = Torso.Position
	local rotateX = WanderCircleRadius * math.cos(WanderTheta)
	local rotateY = WanderCircleRadius * math.sin(WanderTheta)
	local circleOffset = Vector3.new(rotateX, 0, rotateY)
	local WanderTarget = circleLocation + circleOffset
	return WanderTheta, WanderTarget
end

-- cast a downward ray directly beneath where we expect to walk
-- to see if there's any ground present
local function GroundInFront(WanderTarget, StartTime)
	if tick() - StartTime < GroundCheckDelay then
		return true
	end
	
	local ray = Ray.new(WanderTarget, Vector3.new(0, -1, 0) * GroundCheckDistance)
	local part, endPoint = Game.Workspace:FindPartOnRay(ray)
	return part ~= nil
end

local function IsTargetBlocked(WanderTarget)
	local CharSize = Character:GetModelSize()
	local UnitVecMult = 5
	
	-- for several rays directed in front of us, see if we can find anything
	for rad = -math.pi / 2.0, math.pi / 2.0, math.pi / 4.0 do
		for yDelta = -CharSize.y / 2.0, CharSize.y / 2.0 + 0.1, 0.1 do
			local direction = (WanderTarget - Torso.Position).unit
			direction = direction + Vector3.new(math.cos(rad), 0, math.sin(rad))
			
			local origin = Torso.Position + Vector3.new(0, yDelta, 0)
			
			local ray = Ray.new(origin, UnitVecMult * direction)
			local part, endPoint = Game.Workspace:FindPartOnRayWithIgnoreList(ray, {Character, Terrain})
			if part ~= nil then
				return true
			end
		end
	end
	
	return false
end

initrand(PlayerID * 1000)

local ScriptStartDelay = 5
print(string.format("Starting Player%d in %d seconds...", PlayerID, ScriptStartDelay))
wait(ScriptStartDelay)

SetupTeleportLocations()
print("Start walking...")

local WanderTheta, WanderTarget = Wander(true, 0.0)

local StartTime = tick()
local PrintHeading = true
while not _G.EndGame do
	-- First, try to teleport somewhere, based on a statistical chance.
	local TeleportLocation = TryTeleport()
	if TeleportLocation then
		local msg = string.format("Teleporting to location (%f, %f, %f)",
			TeleportLocation.x, TeleportLocation.y, TeleportLocation.z)
		print(msg)
		Torso.CFrame = CFrame.new(TeleportLocation.x, TeleportLocation.y, TeleportLocation.z)
		print("Waiting 3 seconds before moving again.")
		wait(3)
	-- Otherwise, find a heading and march towards it.
	else
		WanderTheta, WanderTarget = Wander(false, WanderTheta)
		
		local PrintedBlockedMessage = false
		local BlockedCount = 0
		-- If we get blocked, try to find a new heading and go for that.  If we don't find
		-- a new heading after a few tries, teleport to a new location.  If we got blocked
		-- because no ground was in front of us, mark it as an error.
		while IsTargetBlocked(WanderTarget) or not GroundInFront(WanderTarget, StartTime) do		
			if not PrintedBlockedMessage then
				if not GroundInFront(WanderTarget, StartTime) then
					-- throw some sort of error here
					-- and print our seed and random number iteration number
					-- along with CFrame information
					local pos = WanderTarget
					local rs = randomstats
					local error = string.format("Ground expected at target (%f, %f, %f), "..
						"randomstats: {seed=%d, randomindex=%d, timescalled=%d}",
						pos.x, pos.y, pos.z,
						rs.seed, rs.randomindex, rs.timescalled)
					if _G.TestingError == nil then
						_G.TestingError = error
					end
					print(error)
				end
		
				print("Finding new heading...")
				PrintedBlockedMessage = true
			end
			
			BlockedCount = BlockedCount + 1
			if 20 == BlockedCount then
				local TeleportLocation = TryTeleport(true)
				local msg = string.format("Still blocked.  Teleporting to location (%f, %f, %f)",
					TeleportLocation.x, TeleportLocation.y, TeleportLocation.z)
				print(msg)
				Torso.CFrame = CFrame.new(TeleportLocation.x, TeleportLocation.y, TeleportLocation.z)
				PrintHeading = false
				break
			end
			
			PrintHeading = true
			
			WanderTheta, WanderTarget = Wander(true, WanderTheta)
		end
		
		if PrintHeading then
			PrintHeading = false
			local Pos = Torso.Position
			print(string.format("Heading is %.3f away from (%f, %f, %f)", WanderTheta, Pos.x, Pos.y, Pos.z))
		end
		
		Humanoid:MoveTo(WanderTarget, Terrain)
		
		wait()
	end
end

print("Caught _G.EndGame.")

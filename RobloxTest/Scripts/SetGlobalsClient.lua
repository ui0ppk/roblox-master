local StartTime = tick()

-- Do nothing until 3 minutes prior to the game timing out, and then set the EndGame flag.
while tick() - StartTime < Game.TestService.Timeout - 180 do
	wait(1)
end
_G.EndGame = true

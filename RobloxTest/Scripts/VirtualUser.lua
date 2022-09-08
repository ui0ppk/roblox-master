local VUser = Game:GetService("VirtualUser")

while not _G.EndGame do
	wait(60)
	print("Pressing 'w' key on virtual keyboard.")
	VUser:TypeKey("w")
end

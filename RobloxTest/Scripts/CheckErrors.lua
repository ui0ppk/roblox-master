while not _G.EndGame do
	if _G.TestingError ~= nil then
		RBX_CHECK_MESSAGE(false, _G.TestingError)
		break
	end
	wait(1)
end

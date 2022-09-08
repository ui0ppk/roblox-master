-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false




---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() onClicked(mouse) end)
self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("", "Tree", "trees.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)



-----------------------
--FUNCTION DEFINITIONS-
-----------------------

local Base = Instance.new("Part")
Base.Name = "Trunk"
Base.formFactor = "Custom"
Base.TopSurface = 0
Base.BottomSurface = 0
Base.Anchored = false
Base.BrickColor = BrickColor.new("Reddish brown")
local Leaves = Base:Clone()
Leaves.Name = "Leaves"
Leaves.CanCollide = false
Leaves.BrickColor = BrickColor.new("Dark green")
local leafmesh = Instance.new("SpecialMesh")
leafmesh.MeshType = "FileMesh"
leafmesh.MeshId = "http://www.roblox.com/asset/?id=1290033"
leafmesh.TextureId = "http://www.roblox.com/asset/?id=2861779" 
leafmesh.Parent = Leaves
Instance.new("CylinderMesh",Base)


-- get dot product of yz angles
function dot(c1,c2)
	local m = CFrame.Angles(math.pi/2,0,0)
	return (c1*m).lookVector:Dot((c2*m).lookVector)
end

-- multiplier for various sizes of foliage
local leaf_mult = {
	Vector3.new(1.5,1.5,1.2);
	Vector3.new(1.5,1,1.5);
	Vector3.new(1.2,1.5,1.5);
	Vector3.new(1.5,1.5,1.5);
}

function Welder(p0, p1)
	local w = Instance.new("Weld")
	w.Part0 = p0
	w.Part1 = p1
	local CJ = CFrame.new(p0.Position)
	w.C0 = p0.CFrame:inverse() * CJ
	w.C1 = p1.CFrame:inverse() * CJ
	w.Parent = p0
end

function Branch(base,c)
	if c <= 0 then
		-- if the complexity has run out, generate some foliage
		local leaves = Leaves:Clone()
		local vol = base.Size.x+base.Size.y+base.Size.z -- determine size based on branch size 
		leaves.Mesh.Scale = leaf_mult[math.random(1,#leaf_mult)]*math.random(vol/3*10,vol/3*12)/10
		leaves.Size = leaves.Mesh.Scale*0.75
		leaves.CFrame = base.CFrame * CFrame.new(0,base.Size.y/2,0) -- center foliage at top of branch
		leaves.Parent = base.Parent
		Welder(leaves, base)
	else
		-- otherwise, make some more branches
		local pos = base.CFrame*CFrame.new(0,base.Size/2,0)
		local height = base.Size.y
		local width = base.Size.x
		local nb = math.random(2,2) -- # of possible branches (2 seems to work fine for now)
		local r = math.random(45,135) -- rotation of branches on y axis

		-- branch split angle difference
		-- the less complexity, the greater the possible angle
		-- minimum: 20-75; maximum: 40-80
		local da = math.random(20+55/c,40+40/c)

		-- branch angle (overall angle of all branches)
		local ba = math.random(-da/3,da/3)

		-- ba+da/2 shouldn't be near or greater than 90 degrees

		for i=0,nb-1 do -- for each branch
			local branch = base:Clone()
			branch.Name = "Branch"
			local h = height*math.random(95,115)/100 -- height .95 to 1.15 of original

			-- make new cframe
			-- move new to top of base, then apply branch angle (ba)
			local new = branch.CFrame * CFrame.new(0,height/2,0) * CFrame.Angles(0,0,math.rad(ba))
			-- next, rotate branch so that it faces away from others, also apply overall rotation (r)
			-- also, apply the split angle (da)
			-- finally, move branch upward by half it's size
			new = new * CFrame.Angles(0,i*(math.pi*2/nb)+r,math.rad(da/2)) * CFrame.new(0,h/2,0)

			-- determine width by branch's final angle; greater the angle, smaller the width
			-- also shave off a bit of width for more dramatic change in size
			-- a frustum cone mesh would really help here
			local w = dot(new,branch.CFrame)*width*0.9

			branch.Size = Vector3.new(w,h,w)
			branch.CFrame = new
			branch.Parent = base.Parent
			Welder(branch, base)

			-- create the next set of branches with one less complexity
			Branch(branch,c-1)
		end
	end
	--wait()	-- remove to generate faster
end

-- Main Function ----------------
function GenerateTree(location,complexity,width,height)
--[[
	location:		position tree will "sprout" out of
	complexity:		# of times to branch out
						determines overall size and quality of tree
						produces between m^c+(m^(c+1)-1)/(m-1) parts,
						where c is complexity,
						and m is either the min or max # of possible branches
						(currently, both min and max are 2)
	width:			initial x/z size of tree base
						determines overall thickness of tree
	height:			initial y size of tree base
						contributes to sparseness of tree
]]
	print("TreeGen - complexity: " .. complexity .. ", width: " .. width .. ", height: " .. height) 
	local tree = Instance.new("Model")
	tree.Name = "Tree"
	tree.Parent = workspace
	local base = Base:Clone()
	base.Parent = tree
	base.Size = Vector3.new(width,height,width)
	-- move up by half its height, and rotate randomly
	base.CFrame = CFrame.new(location) * CFrame.new(0,height/2,0) * CFrame.Angles(0,math.rad(math.random(1,360)),0)
	leafmesh.VertexColor = Vector3.new((math.random() * .1) + .9, (math.random() * .4) + .6, 1)
	-- start branching
	Branch(base,complexity)
	return base
end


function onClicked(mouse)
	if on then
		local targetPart = mouse.Target
		local newTrunk = GenerateTree(mouse.Hit.p, math.random(3,6), math.random(2,4), math.random(5,10))

		if targetPart and targetPart:IsA("Terrain") then Welder(newTrunk, targetPart) end   -- make trees stick to terrain
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	on = false
end




------
--GUI-
------

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))




--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Tree Plugin Loaded")
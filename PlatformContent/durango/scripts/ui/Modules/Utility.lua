-- Written by Kip Turner, Copyright ROBLOX 2015
local RunService = game:GetService('RunService')
local GuiService = game:GetService('GuiService')

local Util = {}
do

	function Util.IsFinite(num)
		return num == num and num ~= 1/0 and num ~= -1/0
	end

	function Util.CalculateRelativeDimensions(guiObject, guiDims, mockup_dims)
		local guiResolution = GuiService:GetScreenResolution()
		local absolutePercentSize = (guiDims / mockup_dims)
		if mockup_dims.y > 0 and guiResolution.y > 0 then
			local mockupAspectRatio = mockup_dims.x / mockup_dims.y
			local globalAspectRatio = guiResolution.x / guiResolution.y
			absolutePercentSize = absolutePercentSize * (mockupAspectRatio / globalAspectRatio)
			local parentObject = guiObject.Parent
			if parentObject then
				local parentPercentScreen = parentObject.AbsoluteSize / guiResolution
				local parentSizeInverse = 1 / parentPercentScreen
				if Util.IsFinite(parentSizeInverse.x) and Util.IsFinite(parentSizeInverse.y) then
					return UDim2.new(parentSizeInverse.x * absolutePercentSize.x, 0, parentSizeInverse.y * absolutePercentSize.y, 0)
				end
			end
		end
		return UDim2.new(absolutePercentSize.x, 0, absolutePercentSize.y, 0)
	end


	-- Anchor Graph
	-- 1     2     3
	-- 4     5     6
	-- 7     8     9

	Util.Enum =
	{
		Anchor =
		{
			TopLeft = 1;
			TopMiddle = 2;
			TopRight = 3;
			CenterLeft = 4;
			Center = 5;
			CenterRight = 6;
			BottomLeft = 7;
			BottomMiddle = 8;
			BottomRight = 9;
		};
	};

	function Util.CalculateAnchor(imageObject, position, anchorType)
		-- TODO: make this work with relativeXX, relativeYY
		if anchorType == Util.Enum.Anchor.TopLeft then
			imageObject.Position = position
		elseif anchorType == Util.Enum.Anchor.TopMiddle then
			imageObject.Position = position + UDim2.new(-imageObject.Size.X.Scale / 2, -imageObject.Size.X.Offset / 2,
				                                        0, 0);
		elseif anchorType == Util.Enum.Anchor.TopRight then
			imageObject.Position = position + UDim2.new(imageObject.Size.X.Scale, -imageObject.Size.X.Offset,
				                                        0, 0);
		elseif anchorType == Util.Enum.Anchor.CenterLeft then
			imageObject.Position = position + UDim2.new(0, 0,
				                                        -imageObject.Size.Y.Scale / 2, -imageObject.Size.Y.Offset / 2);
		elseif anchorType == Util.Enum.Anchor.Center then
			imageObject.Position = position + UDim2.new(-imageObject.Size.X.Scale / 2, -imageObject.Size.X.Offset / 2,
				                                        -imageObject.Size.Y.Scale / 2, -imageObject.Size.Y.Offset / 2);
		elseif anchorType == Util.Enum.Anchor.CenterRight then
			imageObject.Position = position + UDim2.new(-imageObject.Size.X.Scale, -imageObject.Size.X.Offset,
				                                        -imageObject.Size.Y.Scale / 2, -imageObject.Size.Y.Offset / 2);
		elseif anchorType == Util.Enum.Anchor.BottomLeft then
			imageObject.Position = position + UDim2.new(0, 0,
				                                        -imageObject.Size.Y.Scale, -imageObject.Size.Y.Offset);
		elseif anchorType == Util.Enum.Anchor.BottomMiddle then
			imageObject.Position = position + UDim2.new(-imageObject.Size.X.Scale / 2, -imageObject.Size.X.Offset / 2,
				                                        -imageObject.Size.Y.Scale, -imageObject.Size.Y.Offset);
		elseif anchorType == Util.Enum.Anchor.BottomRight then
			imageObject.Position = position + UDim2.new(-imageObject.Size.X.Scale, -imageObject.Size.X.Offset,
				                                        -imageObject.Size.Y.Scale, -imageObject.Size.Y.Offset);
		end
	end

	function Util.CalculateFit(containerObject, rawImageSize)
		local absSize = containerObject.AbsoluteSize
		local scalar = absSize / rawImageSize
		local fixedSize = rawImageSize * math.min(scalar.X, scalar.Y)

		return UDim2.new(0, fixedSize.X , 0, fixedSize.Y)
	end

	function Util.CalculateFill(containerObject, rawImageSize)
		local absSize = containerObject.AbsoluteSize
		local scalar = absSize / rawImageSize
		local fixedSize = rawImageSize * math.max(scalar.X, scalar.Y)

		return UDim2.new(0, fixedSize.X , 0, fixedSize.Y)
	end

	function Util.Create(instanceType)
		return function(data)
			local obj = Instance.new(instanceType)
			for k, v in pairs(data) do
				if type(k) == 'number' then
					v.Parent = obj
				else
					obj[k] = v
				end
			end
			return obj
		end
	end

	---- TWEENING ----
	local ActiveTweens = {}

	local function getActiveTween(prop, instance)
		return ActiveTweens[prop] and ActiveTweens[prop][instance]
	end

	local function setActiveTween(prop, instance, newTween)
		if not ActiveTweens[prop] then
			ActiveTweens[prop] = {}
		end
		ActiveTweens[prop][instance] = newTween
	end

	function Util.Linear(t, b, c, d)
		if t >= d then return b + c end

		return c*t/d + b
	end

	function Util.EaseOutQuad(t, b, c, d)
		if t >= d then return b + c end

		t = t/d;
		return -c * t*(t-2) + b
	end

	function Util.EaseInOutQuad(t, b, c, d)
		if t >= d then return b + c end

		t = t / (d/2);
		if (t < 1) then return c/2*t*t + b end;
		t = t - 1;
		return -c/2 * (t*(t-2) - 1) + b;
	end

	function Util.PropertyTweener(instance, prop, start, final, duration, easingFunc, override, callbackFunction)
		easingFunc = easingFunc or Util.Linear
		override = override or false

		local this = {}
		this.StartTime = tick()
		this.EndTime = this.StartTime + duration
		this.Cancelled = false

		local finished = false
		local percentComplete = 0


		local function setValue(newValue)
			if instance then
				instance[prop] = newValue
			end
		end

		local function finalize()
			setValue(easingFunc(1, start, final - start, 1))
			finished = true
			percentComplete = 1

			if getActiveTween(prop, instance) == this then
				setActiveTween(prop, instance, nil)
			end

			if callbackFunction then
				callbackFunction()
			end
		end

		if override or not getActiveTween(prop, instance) then
			if getActiveTween(prop, instance) then
				getActiveTween(prop, instance):Cancel()
			end
			setActiveTween(prop, instance, this)

			-- Initial set
			setValue(easingFunc(0, start, final - start, duration))
			spawn(function()
				local now = tick()
				while now < this.EndTime and instance and not this.Cancelled do
					setValue(easingFunc(now - this.StartTime, start, final - start, duration))
					percentComplete = Util.Clamp(0, 1, (now - this.StartTime) / duration)
					RunService.RenderStepped:wait()
					now = tick()
				end
				if this.Cancelled == false and instance then
					finalize()
				end

				if getActiveTween(prop, instance) == this then
					setActiveTween(prop, instance, nil)
				end
			end)
		else
			finished = true
		end

		function this:GetFinal()
			return final
		end

		function this:GetPercentComplete()
			return percentComplete
		end

		function this:IsFinished()
			return finished
		end

		function this:Finish()
			if not finished then
				self:Cancel()
				finalize()
			end
		end

		function this:Cancel()
			this.Cancelled = true
			finished = true
			if getActiveTween(prop, instance) == this then
				setActiveTween(prop, instance, nil)
			end
		end

		return this
	end
	---------------

	--- EVENTS ----
	function Util.Signal()
		local sig = {}

		local mSignaler = Instance.new('BindableEvent')

		local mArgData = nil
		local mArgDataCount = nil

		function sig:fire(...)
			mArgData = {...}
			mArgDataCount = select('#', ...)
			mSignaler:Fire()
		end

		function sig:connect(f)
			if not f then error("connect(nil)", 2) end
			return mSignaler.Event:connect(function()
				f(unpack(mArgData, 1, mArgDataCount))
			end)
		end

		function sig:wait()
			mSignaler.Event:wait()
			assert(mArgData, "Missing arg data, likely due to :TweenSize/Position corrupting threadrefs.")
			return unpack(mArgData, 1, mArgDataCount)
		end

		return sig
	end

	function Util.DisconnectEvent(conn)
		if conn then
			conn:disconnect()
		end
		return nil
	end
	--------------

	-- MATH --
	function Util.Clamp(low, high, input)
		return math.max(low, math.min(high, input))
	end

	function Util.ClampVector2(low, high, input)
		return Vector2.new(Util.Clamp(low.x, high.x, input.x), Util.Clamp(low.y, high.y, input.y))
	end

	function Util.TweenPositionOrSet(guiObject, ...)
		if guiObject:IsDescendantOf(game) then
			guiObject:TweenPosition(...)
		else
			guiObject.Position = select(1, ...)
		end
	end
	----

	function Util.ClampCanvasPosition(scrollingContainer, position)
		local container = scrollingContainer
		local parentSize = container.Parent and container.Parent.AbsoluteSize or Vector2.new(0,0)
		local absoluteCanvasSize = Vector2.new(container.CanvasSize.X.Scale * parentSize.X + container.CanvasSize.X.Offset,
								container.CanvasSize.Y.Scale * parentSize.Y + container.CanvasSize.Y.Offset)
		local nextX = Util.Clamp(0, absoluteCanvasSize.X - container.AbsoluteWindowSize.X, position.X)
		local nextY = Util.Clamp(0, absoluteCanvasSize.Y - container.AbsoluteWindowSize.Y, position.Y)

		return Vector2.new(nextX, nextY)
	end

	function Util.Round(num, roundToNearest)
		roundToNearest = roundToNearest or 1
		return math.floor((num + roundToNearest/2) / roundToNearest) * roundToNearest
	end
	--------------

	-- FORMATING --
	-- Removed whitespace from the beginning and end of the string
	function Util.ChompString(str)
		return tostring(str):gsub("^%s+" , ""):gsub("%s+$" , "")
	end

	function Util.FormatNumberString(value)
		-- Make sure beginning and end of the string is clipped
		local stringValue = Util.ChompString(value)
		return stringValue:reverse():gsub("%d%d%d", "%1,"):reverse():gsub("^,", "")
	end

	-- PrettyPrint function for formatting data structures into flat strings, usefull for debugging

	local function PrettyPrint(tb)
		if type(tb) == 'table' then
			local str = "{"
			for k, v in pairs(tb) do
				str = ((str == "{") and str or str..", ")
				if type(k) == 'string' then
					str = str..k.." = "
				elseif type(k) == 'number' then
					-- nothing
				else
					str = str.."["..k.."] = "
				end
				str = str..PrettyPrint(v)
			end
			return str.."}"
		elseif type(tb) == 'string' then
			return "'"..tb.."'"
		else
			return tostring(tb)
		end
	end

	Util.PrettyPrint = PrettyPrint


	-- K is a tunable parameter that changes the shape of the S-curve
	-- the larger K is the more straight/linear the curve gets
	local function SCurveTranform(t, k, lowerK)
		k = k or 0.35
		lowerK = lowerK or 0.8
		t = Util.Clamp(-1,1,t)
		if t >= 0 then
			return (k*t) / (k - t + 1)
		end
		return -((lowerK*-t) / (lowerK + t + 1))
	end

	local function toSCurveSpace(t, deadzone)
		deadzone = deadzone or 0.1
		return (1 + deadzone) * (2*math.abs(t) - 1) - deadzone
	end

	local function fromSCurveSpace(t)
		return t/2 + 0.5
	end

	function Util.GamepadLinearToCurve(thumbstickPosition, deadzone, k, lowerK)
		local function onAxis(axisValue)
			local sign = axisValue < 0 and -1 or 1

			local point = fromSCurveSpace(SCurveTranform(toSCurveSpace(math.abs(axisValue), deadzone)), k, lowerK)
			return Util.Clamp(-1,1, point * sign)
		end
		return Vector2.new(onAxis(thumbstickPosition.x), onAxis(thumbstickPosition.y))
	end

	function Util.IsFastFlagEnabled(flagName)
		local success, isFlagEnabled = pcall(function()
			return settings():GetFFlag(flagName)
		end)

		if success and not isFlagEnabled then
			print("Fast Flag:", flagName, "is currently not enabled.")
		elseif not success then
			print("GetFFlag failed for flag:", flagName)
		end

		return success and isFlagEnabled
	end
	function Util.GetFastVariable(variableName)
		local success, value = pcall(function()
			return settings():GetFVariable(variableName)
		end)

		return success and value
	end

	function Util.ExponentialRepeat(loopPredicate, loopBody, repeatCount)
		repeatCount = repeatCount or 6
		local retryCount = 1
		local numRetries = repeatCount

		while retryCount <= numRetries and loopPredicate() do
			local done = loopBody()
			if done then return end
			wait(retryCount ^ 2)
			retryCount = retryCount + 1
		end

	end

	function Util.SplitString(str, sep)
		local result = {}
		if str and sep then
			for word in string.gmatch(str, '([^' .. sep .. ']+)') do
				table.insert(result, word)
			end
		end
		return result
	end


	local function findAssetsHelper(object, result, baseUrl)
		if not object then return end

		if object:IsA('CharacterMesh') then
			table.insert(result, baseUrl .. tostring(object.MeshId))
			table.insert(result, baseUrl .. tostring(object.BaseTextureId))
			table.insert(result, baseUrl .. tostring(object.OverlayTextureId))
		elseif object:IsA('FileMesh') then
			table.insert(result, object.MeshId)
			table.insert(result, object.TextureId)
		elseif object:IsA('Decal') then
			table.insert(result, object.Texture)
		elseif object:IsA('Pants') then
			table.insert(result, object.PantsTemplate)
		elseif object:IsA('Shirt') then
			table.insert(result, object.ShirtTemplate)
		end

		for _, child in pairs(object:GetChildren()) do
			findAssetsHelper(child, result, baseUrl)
		end
	end
	function Util.FindAssetsInModel(object, baseUrl)
		baseUrl = baseUrl or 'http://www.roblox.com/asset/?id='
		local result = {}
		findAssetsHelper(object, result, baseUrl)
		return result
	end

	function Util.ConvertFontSizeEnumToInt(fontSizeEnum)
		local name = fontSizeEnum.Name
		-- TODO: this is sort of gross?
		local result = string.match(name, '%d+')
		return result or 10
	end

end

return Util

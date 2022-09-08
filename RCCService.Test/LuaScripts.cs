using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Roblox.Grid.Rcc;
using Roblox.Grid;
using Roblox.Common;


namespace RCCService.Test
{
    public static class LuaScripts
    {
        private static string baseurl = "http://www.roblox.com/";
        private static System.Collections.Generic.Stack<string> savedUrls = new Stack<string>();

        public class BaseUrlPoper : IDisposable
        {
            public void Dispose()
            {
                popBaseUrl();
            }

        }

        // use this in a "using" clause.
        public static BaseUrlPoper pushBaseUrl(string newurl)
        {
            savedUrls.Push(baseurl);
            baseurl = newurl;
            return new BaseUrlPoper();
        }

        private static void popBaseUrl()
        {
            baseurl = savedUrls.Pop();
        }

        public static string getBaseUrl()
        {
            return baseurl;
        }


        public const string LoadUnload = "local url = ...\n" +
                                      "local objs = game.Workspace:InsertContent(url)\n" +
                                      "for _, instance in ipairs(objs) do\n" +
                                      "	instance:Remove()\n" +
                                      "end\n";

        // The final argument in this list is the timeout
        public static string StartGameServer()
        {
            return "local gameport = ...\n" +
                "loadfile('" + baseurl + "game/gameserver.ashx')(0, gameport, nil, nil, nil, nil, nil, 30)\n";
        }

        public static ScriptExecution MakeLoadPlaceScript(int placeid)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "Load";
            script.script = String.Format("game:Load('" + baseurl + "asset/?id={0}')", placeid);
            script.arguments = Lua.NewArgs("0");
            return script;
        }

        public static ScriptExecution MakeJoinGameScript(string host, int gameport)
        {
            // other params: UserID=0
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeJoinGameScript";
            script.script = String.Format("loadfile('" + baseurl + "game/join.ashx?server={0}&serverPort={1}')()", host, gameport);
            script.arguments = Lua.NewArgs("0");
            return script;
        }

        public const string GetClientCount = "return game:GetService(\"NetworkServer\"):GetClientCount()";

        public static string BusyScript() { return "loadfile('" + baseurl + "admi/games/bot.ashx')()"; }

        public static ScriptExecution MakeBusyScript()
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeBusyScript";
            script.script = BusyScript();
            return script;
        }

        // lifted from Server\Roblox.Thumbs\Asset.cs
        private static readonly string ToolThumbScript =
@"local comment = 'Model thumbnail'
url, ext, h, v, scriptIcon, toolIcon = ...
function tryorelse(tryfunc, failfunc)
   local r
   if(pcall(function () r = tryfunc() end)) then
       return
   else
       return failfunc()
   end
end
t = game:GetService('ThumbnailGenerator')
game:GetService('ScriptContext').ScriptsDisabled = true
for _,i in ipairs(game:GetObjects(url)) do
	if i.className=='Sky' then
       return tryorelse(
           function() return t:ClickTexture(i.SkyboxFt, ext, h, v) end,
           function() return t:Click(ext, h, v, true) end)
	elseif (i.className=='Tool' or i.className=='HopperBin') and i.TextureId~='' then
       return tryorelse(
           function() return t:ClickTexture(i.TextureId, ext, h, v) end,
           function() return t:ClickTexture(toolIcon, ext, h, v) end)
	elseif i.className=='Script' then
		return t:ClickTexture(scriptIcon, ext, h, v)
	elseif i.className=='SpecialMesh' then
		part = Instance:new('Part')
		part.Parent = workspace
		i.Parent = part
		return t:Click(ext, h, v, true)
	else
		i.Parent = workspace
	end
end
return t:Click(ext, h, v, true)";

        public static Roblox.Grid.Rcc.ScriptExecution MakeToolThumbScript(int assetid, int Width, int Height)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeToolThumbScript";

            //string url = Web.ApplicationURL + "/Asset/?versionid=" + model.CurrentVersion.ID;
            string url = baseurl + "Asset/?id=" + assetid.ToString();
            return Lua.NewScript(
                "MakeToolThumbScript",
                ToolThumbScript,
                url,
                "PNG",
                Width,
                Height,
                baseurl + "Thumbs/Script.png",
                baseurl + "Thumbs/Tool.png"
            );
        }

        public static readonly string PlaceThumbScript =
@"local comment = 'Place thumbnail'
url, ext, x, y = ...
game:GetService('ContentProvider'):SetBaseUrl(url)
game:GetService('ThumbnailGenerator').GraphicsMode=4
game:GetService('ScriptContext').ScriptsDisabled=true game:Load(url) return game:GetService('ThumbnailGenerator'):Click(ext, x, y, false)";
        public static readonly string AvatarThumbScript =
@"local comment = 'Avatar thumbnail'
asseturl, url, fileExtension, x, y = ...
game:GetService('ScriptContext').ScriptsDisabled=true
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(url) end)
player = game:GetService('Players'):CreateLocalPlayer(0)
player.CharacterAppearance = asseturl
player:LoadCharacter(false)

if (player.Character ~= nil) then
    local c = player.Character:GetChildren()
    for i=1,#c do
        if (c[i].className == 'Tool') then
            player.Character.Torso['Right Shoulder'].CurrentAngle = 1.57
            break
        end
    end
end

game:GetService('ThumbnailGenerator').GraphicsMode = 4
return game:GetService('ThumbnailGenerator'):Click(fileExtension, x, y, true)";

        public static readonly string BodyPartScript =
@"local comment = 'BodyPart thumbnail'
asseturl, url, ext, h, v, guyurl, customurl = ...
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(url) end)
game:GetService('ScriptContext').ScriptsDisabled = true
local guy = game:GetObjects(guyurl)[1]
guy.Parent = workspace

pcall(function()
    local objects = game:GetObjects(asseturl)
    for key, object in pairs(objects) do
        object.Parent = guy
    end
end)

pcall(function()
    game:GetObjects(customurl)[1].Parent = guy
end)

t = game:GetService('ThumbnailGenerator')
game:GetService('ThumbnailGenerator').GraphicsMode = 4
return t:Click(ext, h, v, true) ";

        public static readonly string ShirtScript =
@"local comment = 'Shirt thumbnail'
asseturl, ext, h, v, url, guyurl = ...
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(url) end)
game:GetService('ScriptContext').ScriptsDisabled = true
local guy = game:GetObjects(guyurl)[1]
guy.Parent = workspace
c = Instance.new('Shirt')
c.ShirtTemplate = game:GetObjects(asseturl)[1].ShirtTemplate
c.Parent = guy
t = game:GetService('ThumbnailGenerator')
game:GetService('ThumbnailGenerator').GraphicsMode = 4
return t:Click(ext, h, v, true)";

        public static readonly string HeadScript =
@"local comment = 'Head thumbnail'
asseturl, url, ext, h, v, guyurl = ... 
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(url) end)
game:GetService('ThumbnailGenerator').GraphicsMode = 4
game:GetService('ScriptContext').ScriptsDisabled = true
local guy = game:GetObjects(guyurl)[1]
guy.Parent = workspace
local mesh = game:GetObjects(asseturl)[1]
guy.Head.BrickColor = BrickColor.Gray()
guy.Head.Mesh:remove()
mesh.Parent = guy.Head
guy.Torso:remove()
guy['Right Arm']:remove()
guy['Left Arm']:remove()
guy['Right Leg']:remove()
guy['Left Leg']:remove()
t = game:GetService('ThumbnailGenerator')
return t:Click(ext, h, v, true)";

        public static Roblox.Grid.Rcc.ScriptExecution MakeHeadScript(int assetId, int Width, int Height)
        {
            return MakeThumbScript(HeadScript, assetId, 0, Width, Height, true, 1785197);
        }

        public static Roblox.Grid.Rcc.ScriptExecution MakeThumbScript(string baseScript, int id, int versionid, int Width, int Height, bool needsBaseURL, int guyId)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeThumbScript";
            string url;
            if (id != 0)
            {
                url = baseurl + "Asset/?id=" + id.ToString();
            }
            else
            {
                url = baseurl + "Asset/?versionid=" + versionid.ToString();
            }

            string guyurl = "";
            if (guyId != -1)
            {
                guyurl = baseurl + "Asset/?id=" + guyId.ToString();
            }

            if (needsBaseURL)
            {
                return Lua.NewScript(
                    "MakeThumbScript",
                    baseScript,
                    url,
                    baseurl,
                    "PNG",
                    Width,
                    Height,
                    guyurl
                );
            }
            else
            {
                return Lua.NewScript(
                    "MakeThumbScript",
                    baseScript,
                    url,
                    "PNG",
                    Width,
                    Height,
                    guyurl
                );
            }
        }

        public static Roblox.Grid.Rcc.ScriptExecution MakeThumbScript(string baseScript, int id, int versionid, int Width, int Height, bool needsBaseURL)
        {
            return MakeThumbScript(baseScript, id, versionid, Width, Height, needsBaseURL, -1);
        }

        public static Roblox.Grid.Rcc.ScriptExecution MakePlaceThumbScript(int id, int versionid, int Width, int Height)
        {
            return MakeThumbScript(PlaceThumbScript, id, versionid, Width, Height, false);
        }

        public static Roblox.Grid.Rcc.ScriptExecution MakeCustomThumbScript(string baseScript, string url, int Width, int Height)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeCustomThumbScript";
            return Lua.NewScript(
                "MakeCustomThumbScript",
                baseScript,
                url,
                "PNG",
                Width,
                Height
            );
        }


        public static Roblox.Grid.Rcc.ScriptExecution MakeCustomThumbScript(string baseScript, params object[] args)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeCustomThumbScript";
            return Lua.NewScript(
                "MakeCustomThumbScript",
                baseScript,
                args
            );
        }

        private static readonly string ModelThumbScript =
@"local comment = 'Model thumbnail' 
url, baseUrl, ext, h, v, scriptIcon, toolIcon = ... 
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(baseUrl) end)
game:GetService('ThumbnailGenerator').GraphicsMode = 4 
t = game:GetService('ThumbnailGenerator') 
game:GetService('ScriptContext').ScriptsDisabled = true 
for _,i in ipairs(game:GetObjects(url)) do 
    if i.className=='Script' then
    	return t:ClickTexture(scriptIcon, ext, h, v)
    elseif i.className=='SpecialMesh' then
    	part = Instance:new('Part') 
    	part.Parent = workspace 
    	i.Parent = part 
    	return t:Click(ext, h, v, true) 
    else
    	i.Parent = workspace 
    end 
end 
return t:Click(ext, h, v, true)";

/*
        private static readonly string ModelThumbScript =
@"local comment = 'Model thumbnail' 
url, ext, h, v, scriptIcon, toolIcon = ... 
game:GetService('ThumbnailGenerator').GraphicsMode = 4 
function tryorelse(tryfunc, failfunc) 
    local r 
    if(pcall(function () r = tryfunc() end)) then 
        return r 
    else 
        return failfunc() 
    end 
end 
t = game:GetService('ThumbnailGenerator') 
game:GetService('ScriptContext').ScriptsDisabled = true 
for _,i in ipairs(game:GetObjects(url)) do 
    if i.className=='Sky' then 
        return tryorelse(
            function() return t:ClickTexture(i.SkyboxFt, ext, h, v) end, 
            function() return t:Click(ext, h, v, true) end) 
    elseif (i.className=='Tool' or i.className=='HopperBin') and i.TextureId~='' then
        return tryorelse(
            function() return t:ClickTexture(i.TextureId, ext, h, v) end, 
            function() return t:ClickTexture(toolIcon, ext, h, v) end) 
    elseif i.className=='Script' then
    	return t:ClickTexture(scriptIcon, ext, h, v)
    elseif i.className=='SpecialMesh' then
    	part = Instance:new('Part') 
    	part.Parent = workspace 
    	i.Parent = part 
    	return t:Click(ext, h, v, true) 
    else
    	i.Parent = workspace 
    end 
end 
return t:Click(ext, h, v, true)";
*/

        public static Roblox.Grid.Rcc.ScriptExecution MakeModelThumbScript(int id, int versionid, int Width, int Height)
        {
            ScriptExecution script = new ScriptExecution();
            script.name = "MakeModelThumbScript";
            string url;
            if (id != 0)
            {
                url = baseurl + "Asset/?id=" + id.ToString();
            }
            else
            {
                url = baseurl + "Asset/?versionid=" + versionid.ToString();
            }
            return Lua.NewScript(
                "MakeModelThumbScript",
                ModelThumbScript,
                url,
                baseurl,
                "PNG",
                Width,
                Height,
                baseurl + "/Thumbs/Script.png",
                baseurl + "/Thumbs/Tool.png"
            );
        }

        private static readonly string GearThumbScript =
@"local comment = 'Gear thumbnail'
url, baseUrl, ext, h, v  = ...
pcall(function() game:GetService('ContentProvider'):SetBaseUrl(baseUrl) end)
game:GetService('ThumbnailGenerator').GraphicsMode = 4
t = game:GetService('ThumbnailGenerator')
game:GetService('ScriptContext').ScriptsDisabled = true
for _,i in ipairs(game:GetObjects(url)) do
	i.Parent = workspace
end
return t:Click(ext, h, v, true)";

        public static Roblox.Grid.Rcc.ScriptExecution MakeGearThumbScript(int id, int versionid, int Width, int Height)
        {
            return MakeThumbScript(GearThumbScript, id, versionid, Width, Height, true);
        }
    }
}

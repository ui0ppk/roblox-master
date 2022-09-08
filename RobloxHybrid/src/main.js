//
// RobloxHybrid
//

//---------------------------------------------------
define('RobloxHybrid', {}, function(require, exports, module) {
    exports.Utils = require("Utils");
    exports.Events = require("Events");
    exports.Bridge = require("Bridge");
    exports.Game = require("Game");
    exports.Social = require("Social");
    exports.Chat = require("Chat");
    exports.Input = require("Input");
});

//---------------------------------------------------
// Define a global roblox scope
if(!window.Roblox) {
    window.Roblox = {};
}
window.Roblox.Hybrid = require('RobloxHybrid');
    
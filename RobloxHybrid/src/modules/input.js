/**
* Input module
* @module roblox.input
*/
define("Input", {autoGenerateNative:true}, function(require, exports, module) {

    var events = require("Events");

    /**
     * Notifies when the keyboard appears
     *
     * @event Input#onKeyboardShow
     * @type {object}
     * @property {object} params - Indicates the keyboard overlapped area size with the webview
     */
    exports.onKeyboardShow = new events.Slot("onKeyboardShow");

    /**
     * Notifies when the keyboard hides
     *
     * @event Input#onKeyboardHide
     * @type {object}
     */
    exports.onKeyboardHide = new events.Slot("onKeyboardHide");
});

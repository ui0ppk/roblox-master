/**
* Chat module
* @module roblox.chat
*/
define("Chat", {autoGenerateNative:true}, function(require, exports, module) {

    exports.events = require("Events");

    /**
     * Notification that the Unread Messages badge should be revealed or updated
     * with the number of unread messages. If numUnreadMessages <= 0, the badge
     * will be removed.
     *
     * @param {int} numUnreadMessages - number of currently unread messages
     * @param {Function} callback
     * 
     * @instance
     * @platforms iOS(7.0)
     * @platforms Android(15)
     */
    exports.newMessageNotification = function(numUnreadMessages, callback) {
        // TODO: Implement for WEB
    };

    /**
    * Returns the height of the top bar, in pixels, from the app.
    *
    * @param {Function} callback
    *
    * @instance
    * @platforms iOS(7.0)
    * @platforms Android(15)
    */
    exports.getTopBarHeight = function(callback) {
         
    }

    /**
    * Returns the height of the keyboard, in pixels, from the app. Will
    * only work when the keyboard is already open.
    *
    * @param {Function} callback
    *
    * @instance
    * @platforms iOS(7.0)
    * @platforms Android(15)
    */
    exports.getKeyboardHeight = function(callback) {

    }
});
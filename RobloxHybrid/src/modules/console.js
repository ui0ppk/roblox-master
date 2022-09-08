/**
* Console module
* @module console
*/
define("Console", {autoGenerateNative:true}, function(require, exports, module) {

    var DEFAULT_TAG = "RobloxHybrid";

    var bridge = require("Bridge");

    /**
     * Log event
     *
     * @param {string} tag - Tag
     * @param {string} message - Message
     * @instance
     */
    exports.log = function(tag, message) {
        if(arguments.length < 2) {
            message = tag;
            tag = DEFAULT_TAG;
        }
        window.console.log(tag + ": " + message);
    };

    /**
     * Error event
     *
     * @param {string} tag - Tag
     * @param {string} message - Message
     * @instance
     */
    exports.error = function(tag, message) {
        if(arguments.length < 2) {
            message = tag;
            tag = DEFAULT_TAG;
        }
        window.console.error(tag, message);
    };

});

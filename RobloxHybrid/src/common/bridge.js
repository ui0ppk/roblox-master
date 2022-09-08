/**
* Native/Browser execution module
* @module bridge
*/
define("Bridge", {}, function(require, exports, module) {

    /**
     * Status values
     * @enum {number}
     * @readonly
     */
    exports.CallbackStatus = Object.freeze({
        SUCCESS: 0,
        FAILURE: 1
    });

    /**
     * Execute a native/browser command
     *
     * @param {string} moduleID - The id of the module
     * @param {string} functionName - 
     * @params {Object} params - 
     * @param {Function} callback - Function to be called when the event is triggered 
     * @returns {string} uuid to reference this slot-callback pair
     * @instance
     */
    exports.execute = function(moduleID, functionName, params, callback) {
        Roblox.Hybrid.Console.log("JS bridge failed to load a native implementation");
    };

    /**
     * Function called from native to fire a single callback
     * It will always be executed on the main thread.
     *
     * @param {string} callbackID - Unique ID of the stored callback
     * @param {CallbackStatus} status - Callback status
     * @param {Object} params - Parameters
     * @instance
     */
    exports.nativeCallback = function(callbackID, status, params) {
        Roblox.Hybrid.Console.log("JS bridge failed to load a native implementation");
    };

    /**
     * Function called from native to fire an event
     * It will always be executed on the main thread.
     *
     * @param {string} moduleID - Unique ID of the module that owns the event
     * @param {string} eventName - Name of the event
     * @param {Object} params - Parameters
     * @instance
     */
    exports.emitEvent = function(moduleID, eventName, params) {
        var targetModule = require(moduleID);
        if(targetModule) {
            var slot = targetModule[eventName];
            if(slot) {
                slot.emit(params);
            } else {
                Roblox.Hybrid.Console.log("Unable to emit event: Slot " + eventName + " not found in module " + moduleID);
            }
        } else {
            Roblox.Hybrid.Console.log("Unable to emit event: Module " + moduleID + " not found");
        }
    };
});

define("Bridge/Native", {}, function(require, exports, module) {

    var utils = require("Utils");

    // List of callbacks
    var callbacks = {};

    // Mark this module as native
    exports.isNative = true;

    // Associate a callback function with a UUID.
    // Returns the UUID or null if the callback function is not valid
    exports.registerCallback = function(callback) {
        if(callback) {
            var callbackID = utils.createUUID();
            callbacks[callbackID] = callback;
            return callbackID;
        } else {
            return null;
        }
    };

    /** @inheritdoc */
    exports.nativeCallback = function(callbackID, status, params) {
        var callback = callbacks[callbackID];
        if(callback !== undefined) {
            delete callbacks[callbackID];
            callback.apply(null, [status, params]);
        }
    };
});

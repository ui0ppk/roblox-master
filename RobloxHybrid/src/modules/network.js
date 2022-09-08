/**
* Camera module
* @module roblox.network
*/
define("Network", {autoGenerateNative:true}, function(require, exports, module) {

    /**
     * Network status
     * @readonly
     * @enum {string}
     * @instance
     */
    exports.status = Object.freeze({
        UNKNOWN: "unknown",
        ETHERNET: "ethernet",
        WIFI: "wifi",
        CELL_2G: "2g",
        CELL_3G: "3g",
        CELL_4G: "4g",
        CELL:"cellular",
        NONE: "none"
    });

    /**
     * Start a game session
     *
     * @param {string} placeID - ID of the place to start
     * @param {Function} successCallback
     * @param {Function} errorCallback
     * @returns {status}
     * @instance
     * @todo NOT IMPLEMENTED
     */
    exports.getStatus = function() {
        //
    };

});

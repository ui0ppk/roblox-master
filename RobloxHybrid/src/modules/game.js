/**
* Camera module
* @module roblox.game
*/
define("Game", {autoGenerateNative:true}, function(require, exports, module) {

    exports.events = require("Events");

    /**
     * Start a game session
     *
     * @param {string} placeID - ID of the place to start
     * @param {Function} callback
     * @instance
     * @platforms iOS(7.0)
     */
    exports.startWithPlaceID = function(placeID, callback) {
        // TODO: Implement for WEB
    };

    /**
     * Launches a game from a party request - only used for members joining, NOT leader joining
     *
     * @param {string} placeId - Asset ID of game to launch
     * @param {Function} callback
     *
     * @instance
     * @platforms iOS(7.0)
     * @platforms Android(15)
    */
    exports.launchPartyForPlaceId = function(placeId, callback) {

    }

    /**
     * Launches a game as a party leader - do NOT use for party member joins
     *
     * @param {string} placeId - Asset ID of game to launch
     * @param {Function} callback
     *
     * @instance
     * @platforms iOS(7.0)
     * @platforms Android(15)
    */
    exports.launchPartyLeaderForPlaceId = function (placeId, callback) {

    }
});
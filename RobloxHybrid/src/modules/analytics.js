/**
* Analytics module
* @module analytics
*/
define("Analytics", {autoGenerateNative:true}, function(require, exports, module) {
    /**
     * Track an event
     *
     * @param {string} eventName - Event name
     * @param {Object} params - Dictionary with the event parameters. Only the first level will be saved. (i.e. no nested objects).
     * @instance
     * @platforms iOS(7.0)
     */
    exports.trackEvent = function(eventName, params) {
        //
    };

});

/**
* Camera module
* @module camera
*/
define("Camera", {autoGenerateNative:true}, function(require, exports, module) {
    /**
     * Gets a picture from source defined by "options.sourceType", and returns the
     * image as defined by the "options.destinationType" option.
     *
     * @param {Function} successCallback
     * @param {Function} errorCallback
     * @param {Object} options
     * @instance
     */
    exports.getPicture = function(successCallback, errorCallback, options) {
        //
    };

    /**
     * Clean up
     * @param {Function} successCallback
     * @param {Function} errorCallback
     * @instance
     */
    exports.cleanup = function(successCallback, errorCallback) {
        //
    };

});

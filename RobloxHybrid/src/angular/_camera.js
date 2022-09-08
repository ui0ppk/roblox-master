/**
* Camera module for Angular
* @module angular/camera
*/
angular.module('roblox.camera', [])

    .factory('$robloxCamera', [function ($q) {

        return {
            /**
            * Capture a picture
            * @instance
            *
            * @arg {Object} options - Options
            */
            getPicture: function (options) {
                return null;
            },

            /**
            * Clean up cached data
            * @instance
            */
            cleanup: function () {
                return null;
            }
        };
    }]);
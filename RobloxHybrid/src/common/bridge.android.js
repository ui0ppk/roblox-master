/**
* Native iOS execution module
* @augments bridge
*/
define("Bridge/Android", {}, function(require, exports, module) {

    /** @inheritdoc */
    exports.execute = function(moduleID, functionName, params, callback) {
        
        var callbackID = exports.registerCallback(callback);

        var query = {
            moduleID: moduleID,
            functionName: functionName,
            params: params,
            callbackID: callbackID
        };

        window.__globalRobloxAndroidBridge__.executeRoblox( JSON.stringify(query) );
    };
});

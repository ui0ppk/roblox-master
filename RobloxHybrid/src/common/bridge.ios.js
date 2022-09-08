/**
* Native iOS execution module
* @augments bridge
*/
define("Bridge/iOS", {}, function(require, exports, module) {

    var execXhr;         // Reusing XMLHttpRequest improves performance.

    var requestId = 0;   // NSURLProtocol issues each command several times
                         // 'requestId' helps prevent duplicates.

    var webViewId;       // Identifier for the current webview
                         // This number is injected in the userAgent.


    var utils = require("Utils");

    function doNativeRequest(query) {
        if(!webViewId) {
            var ua = navigator.userAgent;
            webViewId = ua.match(/Hybrid\((.*)\)/i);
            if(webViewId) {
                webViewId = webViewId[1];
            } else {
                webViewId = utils.createUUID();
            }
        }

        // This prevents sending an XHR when there is already one being sent.
        if (execXhr && execXhr.readyState != 4) {
            execXhr = null;
        }
        // Re-using the XHR improves bridge performance by about 10%.
        execXhr = execXhr || new XMLHttpRequest();

        // Add a timestamp to the query param to prevent caching.
        execXhr.open('HEAD', "rbx_native_exec?" + (+new Date()), true);
        execXhr.setRequestHeader('command', JSON.stringify(query));
        execXhr.setRequestHeader('webViewId', webViewId);
        execXhr.setRequestHeader('requestId', ++requestId);
        execXhr.send(null);				
		
		/*
			This is needed to support Apple's new WebKit javascript callback messaging.
			The value "RobloxWKHybrid" in "window.webkit.messageHandlers.RobloxWKHybrid.postMessage" is very intentional.  It must match the
			script message handler name value in /ClientIntegration/Client/iOS/RobloxUI/Source/Screens/RBMobileWebViewController.mm			
		*/
		if ( window.webkit.messageHandlers )
		{
			window.webkit.messageHandlers.RobloxWKHybrid.postMessage({
				"webViewId":webViewId
				,"command":JSON.stringify(query)
				,"requestId":requestId})			
		}
    }

    /** @inheritdoc */
    exports.execute = function(moduleID, functionName, params, callback) {
        
        var callbackID = exports.registerCallback(callback);

        var query = {
            moduleID: moduleID,
            functionName: functionName,
            params: params,
            callbackID: callbackID
        };

        doNativeRequest(query);
    };

    exports.getWebViewID = function() {
        return webViewId;
    };
});

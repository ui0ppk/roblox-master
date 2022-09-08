/**
* Camera module
* @module roblox.social
*/
define("Social", {autoGenerateNative:true}, function(require, exports, module) {
    
    // var utils = require("Utils");
    // var events = require("Events");

    // // Roblox's Gigya API key
    // var GIGYA_API_KEY = "3_OsvmtBbTg6S_EUbwTPtbbmoihFY5ON6v6hbVrTbuqpBs7SyF_LQaJwtwKJ60sY1p";
    
    /**
     * Enum with the available providers
     * @readonly
     * @enum {string}
     * @instance
     */
    exports.providers = Object.freeze({
        FACEBOOK: "facebook",
        TWITTER: "twitter",
        GOOGLEPLUS: "googleplus"
    });

    /**
     * Event triggered when the module is initialized
     * @event
     */
    // exports.onModuleInitialized = new events.Slot("social_initialized", true);

    /**
     * Initialize module.
     * This function is automatically called by the module loader.
     */
    // exports.init = function() {

    //     if(window.gigya) {
    //         // Gigya's global object is declared
    //         // This module dependencies are already initialized
    //         exports.onModuleInitialized.emit();
    //     } else {
    //         // Initialize Gigya
    //         var scriptURL = "http://cdn.gigya.com/js/gigya.js?apiKey=" + GIGYA_API_KEY;
    //         utils.loadScript(scriptURL, function() {
    //             exports.onModuleInitialized.emit();
    //         });
    //     }
    // };

    /**
    * Present a share dialog
    * @param {string} text - Text to share
    * @param {string} link - Link to embed
    * @param {string} [imageURL=null] - link
    * @param {string} snapToElementID - ID of the DOM element
    * @param {Function} [callback] - Callback
    * @instance
    * @platforms Web
    * @platforms iOS(7.0)
    * @platforms Android(19)
    */
    exports.presentShareDialog = function(text, link, imageURL, callback) {
        // The web team doesn't want to include gigya here

        // exports.onModuleInitialized.subscribe(function() {
        //     var ua = new gigya.socialize.UserAction();
        //     ua.setLinkBack(link);
        //     ua.setTitle(text);
        //     var params = {
        //         userAction: ua,
        //         showEmailButton: false,
        //         operationMode: "simpleShare",
        //         snapToElementID: snapToElementID,
        //         grayedOutScreenOpacity: 50
        //     };
        //     gigya.socialize.showShareUI(params);
        // });
    };


    /**
     * Login to a single social network
     *
     * @param {Object} options - Login options
     * @param {provider} options.privider - The provider that is used for authenticating the user. The following values are currently supported for use with this parameter: facebook, twitter, yahoo, messenger, googleplus, linkedin, aol, foursquare, instagram, renren, qq, sina, kaixin, vkontakte, blogger, wordpress, typepad, paypal, amazon, livejournal, verisign, openid, netlog, signon, orangefrance, mixi, yahoojapan, odnoklassniki, spiceworks, livedoor, skyrock, vznet, xing. Also SAML providers are supported - the format of the provider name is "saml-<name>".
     * @param {boolean} [options.forceAuthentication] - The default value of this parameter is 'false'. If set to 'true', the user will be forced to provide her social network credentials during the login, even if she is already connected to the social network. This parameter is currently supported by Facebook, Twitter, Renren, and LinkedIn. Please note that the behavior of the various social networks may be slightly different: Facebook expects the current user to enter their password, and will not accept a different user name. Other networks prompt the user to re-authorize the application or allow a different user to log in. 
     * @param {Function} callback
     * @see {@link http://developers.gigya.com/020_Client_API/010_Socialize/socialize.login} for full login options
     * @instance
     * @todo NOT IMPLEMENTED
     */
    exports.login = function(options, successCallback, errorCallback) {
        //
    };

    /**
    * Logout from a social network
    * @param {Function} callback
    * @instance
    * @todo NOT IMPLEMENTED
    */
    exports.logout = function(callback) {
        //
    };
});

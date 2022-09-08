/**
* Utils module
* @module utils
*/
define("Utils", {}, function(require, exports, module) {

    /**
     * Returns true if this app is executed in a mobile device, either in a native app or in the browser.
     *
     * @returns {Boolean} true if this app is executed in a mobile device, either in a native app or in the browser.
     * @instance
     */
    exports.isMobile = function() {
        var ua = navigator.userAgent.toLowerCase();
        var keywords = "(iphone;android;midp;240x320;blackberry;netfront;nokia;panasonic;portalmmm;sharp;sie-;sonyericsson;symbian;windows ce;benq;mda;mot-;opera mini;philips;pocket pc;sagem;samsung;htc".split(";");
        var keyword;
        for (keyword in keywords) {
            if (-1 != ua.indexOf(keywords[keyword])) {
                return true;
            }
        }
        return false;
    };

    /**
     * Returns true if this app is executed natively ONLY
     *
     * @returns {Boolean} true if this app is executed in a mobile device, either in a native app or in the browser.
     * @instance
     */
    exports.isMobileApp = function() {
        var ua = navigator.userAgent.toLowerCase();
        return ua.indexOf("hybrid") != -1;
    };

    /**
     * Loads/injects a script
     *
     * @param {string} scriptURL - Script URL to load
     * @param {Function} callback - Callback to be executed when the script is loaded
     * @instance
     */
    exports.loadScript = function(scriptURL, callback) {
        
        var scriptLoaded = false;
        function onScriptLoaded() {
            if(!scriptLoaded) {
                scriptLoaded = true;
                callback();
            }
        }

        var head = document.getElementsByTagName('head')[0];
        var script = document.createElement('script');
        script.type= 'text/javascript';
        script.async = true;
        script.onreadystatechange= function () {
          if (this.readyState == 'complete') onScriptLoaded();
        }
        script.onload= onScriptLoaded;
        script.src= scriptURL;
        
        head.appendChild(script);
    };


    /**
     * Extends a child object from a parent object using classical inheritance
     * pattern.
     *
     * @param {Function} Child - Child class
     * @param {Function} Parent - Parent class
     */
    exports.extend = (function() {
        // proxy used to establish prototype chain
        var F = function() {};
        // extend Child from Parent
        return function(Child, Parent) {
            F.prototype = Parent.prototype;
            Child.prototype = new F();
            Child.__super__ = Parent.prototype;
            Child.prototype.constructor = Child;
        };
    }());

    /**
     * Create a UUID
     *
     * @returns A newly random UUID with the format XXXX-XX-XX-XX-XXXXXX
     */
    exports.createUUID = function() {

        function createPart(length) {
            var uuidpart = "";
            for (var i=0; i<length; i++) {
                var uuidchar = parseInt((Math.random() * 256), 10).toString(16);
                if (uuidchar.length == 1) {
                    uuidchar = "0" + uuidchar;
                }
                uuidpart += uuidchar;
            }
            return uuidpart;
        }

        return createPart(4) + '-' +
            createPart(2) + '-' +
            createPart(2) + '-' +
            createPart(2) + '-' +
            createPart(6);
    };
});

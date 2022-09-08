//
// RobloxHybrid
//
var require, define;

//---------------------------------------------------
(function () {
    // Loaded modules
    var modules = {};

    // Stack of moduleIds currently being built.
    var requireStack = [];
    
    // Map of module ID -> index into requireStack of modules currently being built.
    var inProgressModules = {};

    var nativePrefix = (function() {
        var ua = navigator.userAgent.toLowerCase();
        var isNative = ua.indexOf("hybrid") != -1;
        if(isNative) {
            // iOS
            if(ua.indexOf("ipad") != -1 || ua.indexOf("iphone") != -1) {
                return "iOS";
            }
            
            // Android
            if(ua.indexOf("android") != -1) {
                return "Android";
            }
        }
        return null;
    })();

    function build(module) {
        var factory = module.factory;
        var localRequire = require;

        module.exports = createBaseModule(module);

        delete module.factory;
        factory(localRequire, module.exports, module);

        // Check if there's a native implementation available
        if(nativePrefix) {
            var nativeModuleID = module.id + "/Native";
            var nativeModule = modules[nativeModuleID];

            if( !nativeModule && module.options && module.options.autoGenerateNative ) {
                nativeModule = autoGenerateNative(factory, module.options);
            }

            // If exists, build the native implementation
            if( nativeModule ) {
                nativeModule.factory(localRequire, module.exports, module);
                delete nativeModule.factory;
            }

            // Check if there's a platform specific implementation available
            var platformModuleID = module.id + "/" + nativePrefix;
            var platformModule = modules[platformModuleID];
            if(  platformModule ) {
                platformModule.factory(localRequire, module.exports, module);
                delete platformModule.factory;
            }
        }

        // Initialize the module
        if(module.exports && module.exports.init && typeof module.exports.init === "function") {
            module.exports.init();
        }

        return module.exports;
    }

    // Common members and methods for each module
    function createBaseModule(module) {
        var exports = {};

        // Native modules override this member
        exports.isNative = false;

        // This function returns true in the callback if the
        // function is supported natively by the module
        exports.supports = function(functionName, callback) {
            if(callback) {
                callback(false);
            }
        };

        return exports;
    }

    function autoGenerateNative(nativeModuleID, options) {

        // This factory replaces every function
        var generatedFactory = function(require, exports, module) {
            
            var bridge = require("Bridge");

            // Iterate over all the exports to find and override the functions
            for(var prop in exports) {
                
                (function() {
                    var currentProp = prop;

                    var func = exports[currentProp];
                    if(func && typeof func === "function"){

                        // Extract parameter names from function
                        var STRIP_COMMENTS = /((\/\/.*$)|(\/\*[\s\S]*?\*\/))/mg;
                        var ARGUMENT_NAMES = /([^\s,]+)/g;
                        var fnStr = func.toString().replace(STRIP_COMMENTS, '');
                        var paramNames = fnStr.slice(fnStr.indexOf('(')+1, fnStr.indexOf(')')).match(ARGUMENT_NAMES);
                        if(paramNames === null) {
                            paramNames = [];
                        }

                        // Overrided function
                        exports[currentProp] = function() {

                            var callback = null;
                            var parameters = {};

                            // Find the callback function in the argument list
                            // (It should be the first and only function)
                            for(var i = 0; i < arguments.length; ++i) {
                                var paramName = paramNames[i] || ("" + i);
                                var paramValue = arguments[i];

                                if(typeof paramValue === "function") {
                                    callback = paramValue;
                                }

                                parameters[paramName] = paramValue;
                            }

                            bridge.execute(module.id, currentProp, parameters, callback);
                        };
                    }
                })();
            }
        };

        delete options.autoGenerateNative;

        return define(nativeModuleID, options, generatedFactory);
    }

    require = function (id) {
        if (!modules[id]) {
            throw "module " + id + " not found";
        } else if (id in inProgressModules) {
            var cycle = requireStack.slice(inProgressModules[id]).join('->') + '->' + id;
            throw "Cycle in require graph: " + cycle;
        }
        if (modules[id].factory) {
            try {
                inProgressModules[id] = requireStack.length;
                requireStack.push(id);
                return build(modules[id]);
            } finally {
                delete inProgressModules[id];
                requireStack.pop();
            }
        }
        return modules[id].exports;
    };

    define = function (id, options, factory) {
        if (modules[id]) {
            throw "module " + id + " already defined";
        }

        modules[id] = {
            id: id,
            options: options,
            factory: factory
        };

        return modules[id];
    };

    define.remove = function (id) {
        delete modules[id];
    };

    define.moduleMap = modules;
})();

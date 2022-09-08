/**
* Camera module
* @module roblox.dialogs
*/
define("Dialogs", {autoGenerateNative:true}, function(require, exports, module) {
    /**
     * Present a single button dialog
     *
     * @param {string} text - Content text
     * @param {Function} callback - Callback
     * @param {string} title - Window title
     * @param {string} buttonName - Button name
     * @instance
     * @platforms iOS(7.0)
     */
    exports.alert = function(text, callback, title, buttonName) {
        window.alert(text);
        if(callback) {
            callback();
        }
    };

    /**
     * Present a dialog with multiple options
     *
     * @param {string} text - Content text
     * @param {Function} callback - Callback
     * @param {string} title - Window title
     * @param {string[]} buttonName - Button labels
     * @example
     *   function onConfirm(results) {
     *     // results.input1
     *     // results.buttonIndex
     *   }
     *
     *   Roblox.Hybrid.dialogs.confirm(
     *     'Please enter your name',
     *     onConfirm,
     *     'Window title',
     *     ['Ok', 'Cancel']
     *   );
     * @instance
     * @platforms iOS(7.0)
     */
    exports.confirm = function(text, callback, title, buttonName) {
        var result = window.confirm(text);
        if(callback) {
            callback(result);
        }
    };

    /**
     * Present a dialog with a textbox
     *
     * @param {string} text - Content text
     * @param {Function} callback - Callback
     * @param {string} title - Window title
     * @param {string} buttonName - Button name
     * @param {string} defaultText - Default text
     * @example
     *   function onPrompt(results) {
     *     // results.input1
     *     // results.buttonIndex
     *   }
     *
     *   Roblox.Hybrid.dialogs.prompt(
     *     'Please enter your name',
     *     onPrompt,
     *     'Window title',
     *     ['Ok', 'Cancel'],
     *     'Your name here'
     *   );
     * @instance
     * @platforms iOS(7.0)
     */
    exports.prompt = function(text, callback, title, buttonName, defaultText) {
        var result = window.prompt(text, defaultText);
        if(callback) {
            callback(result);
        }
    };

});

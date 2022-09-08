/**
* Events module
* @module events
*/
define("Events", {}, function(require, exports, module) {

    /**
     * Event declaration
     * 
     * @param name {string} name - Event ID
     * @param params {Object} - Custom event specific information
     */
    function Event(name, params) {
        this.name = name;
        this.params = params;
    };

    /**
     * Slot declaration
     * 
     * @class
     * @param eventName {string} - eventID of the events that 
     * @param sticky {boolean} - If true, the event will fire for observers that subscribe even after the event was fired (example, moduleInitialized)
     */
    exports.Slot = function(eventName, sticky) {
        this.eventName = eventName;
        this.sticky = sticky;

        // Has this event already been emitted?
        var emittedEvent = null;

        // List of all subscribers
        var listeners = [];

        /**
         * Subscribe to an event
         *
         * @param {string} eventName - The name of the event
         * @param {Function} listener - Function to be called when the event is triggered.
         *                              This callback should return true to stop propagation.
         * @instance
         */
        this.subscribe = function(listener) {
            listeners.push(listener);

            // Fire the event if its sticky and was already fired
            if(emittedEvent) {
                listener(emittedEvent);
            }
        };

        /**
         * Unsubscribe from an event
         *
         * @param {Function} listener - Listener to be removed
         * @instance
         */
        this.unsubscribe = function(listener) {
            var listenersCount = listeners.length;
            for(var i = 0; i < listenersCount; i++) {
                var thisListener = listeners[i];
                if( thisListener === listener ) {
                    listeners.splice(i, 1);
                    break;
                }
            }
        };

        /**
         * Emit the event with parameters
         *
         * @param {Object} - Custom user object to pass to the listeners
         * @instance
         */
        this.emit = function(params) {
            var eventInstance = new Event(this.eventName, params);
            if(this.sticky) {
                emittedEvent = eventInstance;
            }

            var listenersCount = listeners.length;
            for(var i = 0; i < listenersCount; i++) {
                var listener = listeners[i];
                if( listener(eventInstance) ) {
                    break;
                }
            }
        };
    };
});

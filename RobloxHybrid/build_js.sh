#!/bin/bash

SRC_FILES="
	src/base.js
	src/common/utils.js
	src/common/events.js
	src/common/bridge.js
	src/common/bridge.ios.js
	src/common/bridge.android.js
	src/modules/social.js
	src/modules/game.js
	src/modules/chat.js
	src/modules/input.js
	src/main.js
	"
	# src/modules/analytics.js
	# src/modules/camera.js
	# src/modules/console.js
	# src/modules/dialogs.js
	# src/modules/network.js

java -jar ./closure-compiler/compiler.jar --compilation_level WHITESPACE_ONLY --js_output_file RobloxHybrid.js --output_wrapper="(function() { %output% })();" $SRC_FILES
#java -jar ./closure-compiler/compiler.jar --js_output_file roblox.js --output_wrapper="(function() { %output% })();" $SRC_FILES

cp RobloxHybrid.js ../Android/NativeShell/assets/html/
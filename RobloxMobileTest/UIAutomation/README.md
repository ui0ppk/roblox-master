Mobile Automation 
ywen@roblox.com 

Prerequisite:
1.install appium frame work.
  see details from appium.io
2.install appium client library from below: 
  https://github.com/appium/python-client


Running test:
/python/runtest.py [requiredarguments]
arguments:
-t device_type : currently surrpoot 'phone' & 'tablet'
-p target platfom : currently surppot 'ios' and 'android'
-id target device id : for ios, use 'instruments -w device', for android, use 'adb devices'
-v app_versio : app version
-in internal_version : internal_version_onoff, accept 'true' or 'false'
-s target_server : target test server: 1.production ,2.test1, 3.test2, 4.stest1, 5.stest2
-src appsrc: src for target app

use --help for display help information

for edit build-in parameters
edit /python/automationutils/constants.py
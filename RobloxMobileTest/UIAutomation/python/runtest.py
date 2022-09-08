__author__ = 'yiwen'

import unittest
from automationutils import  contants
import ios_roma
from subprocess import Popen, PIPE, STDOUT
import argparse
import time
from teamcity import is_running_under_teamcity
from teamcity.unittestpy import TeamcityTestRunner


class TestRunner(unittest.TestCase):
    test = None
    androidtest = None
    parser = argparse.ArgumentParser(description='All arguments needed for running a test')
    parser.add_argument('-t',dest='device_type',action='store',help='target device type: currently surrpoot phone & tablet')
    parser.add_argument('-p', dest='platform',action='store', help='target platfom : currently surppot ios and android')
    parser.add_argument('-id',dest='device_id', action='store', help="target device id : for ios, use 'instruments -w device', for android, use 'adb devices'")
    parser.add_argument('-v',dest='app_version', action='store', help='app version')
    parser.add_argument('-in',dest='internal_version', action='store', help='internal_version_onoff')
    parser.add_argument('-s',dest='target_server',action='store',help='target test server: 1.production ,2.test1, 3.test2, 4.stest1, 5.stest2')
    parser.add_argument('-src',dest='appsrc',action='store',help='src for target app')
    parser.add_argument('-tester',dest='tester',action='store',help='tester email')


    args = parser.parse_args()
    TESTER = args.tester
    if args.device_type == 'phone':
       prefix = 'm.'
       DEVICE_TYPE = 'phone'
    else:
        prefix = 'www.'
        DEVICE_TYPE = 'tablet'

    if 'test' in args.target_server:
            USERNAME = contants.GT_USERNAME
            PASSWORD = contants.GT_PASSWORD
            if args.target_server == 'test1':
                SERVER = 'http://'+prefix+contants.GT1_SERVER+'/'
                TEST_GAME = contants.GAMEFORTESTONGT1
            elif args.target_server == 'test2':
                SERVER = 'http://'+prefix+contants.GT2_SERVER+'/'
                TEST_GAME = contants.GAMEFORTESTONGT2
            elif args.target_server == 'stest1':
                SERVER = 'http://'+prefix+contants.ST1_SERVER+'/'
                TEST_GAME = contants.GAMEFORTESTONST1
            elif args.target_server == 'stest2':
                SERVER = 'http://'+prefix+contants.ST2_SERVER+'/'
                TEST_GAME = contants.GAMEFORTESTONST2
    else:
        USERNAME = contants.PD_USERNAME
        PASSWORD = contants.PD_PASSWORD
        TEST_GAME = contants.PD_USERNAME+"'s Place"
        SERVER = 'http://'+prefix+contants.MAIN_SERVER+'/'

    if args.platform == 'ios':
        iostest = ios_roma.IosTestsRoma
        iostest.APP_SRC = args.appsrc
        if args.internal_version == 'false':
            iostest.internal = False
        else:
            iostest.internal = True
        iostest.DEVICE_NAME = args.device_id
        iostest.APP_VERSION = args.app_version
        iostest.USERNAME = USERNAME
        iostest.PASSWORD = PASSWORD
        iostest.TEST_GAME = TEST_GAME
        iostest.SERVER = SERVER
        iostest.DEVICE_TYPE= DEVICE_TYPE
        iostest.TESTER = TESTER

    elif args.platform == 'android':
            androidtest = android_roma.AndroidTestsRoma
            androidtest.APP_SRC = args.appsrc
            if args.internal_version == 'false':
                androidtest.internal = False
            else:
                androidtest.internal = True
            androidtest.DEVICE_NAME = args.device_id
            androidtest.APP_VERSION = args.app_version
            androidtest.USERNAME = USERNAME
            androidtest.PASSWORD = PASSWORD
            androidtest.TEST_GAME = TEST_GAME
            androidtest.SERVER = SERVER[7:]


if __name__ == '__main__':
    if not TestRunner.iostest and not TestRunner.androidtest:
        print 'lack of arguments, cancel test. plese read  -h before run the test'

    elif TestRunner.iostest:
        print TestRunner.args.device_id
        appiumprocess = Popen(['appium', '-U',TestRunner.args.device_id])
        suite = unittest.TestLoader().loadTestsFromTestCase(TestRunner.iostest)
#       suite = unittest.TestLoader().loadTestsFromName(name='test_gamelaunch',module=TestRunner.iostest)
        print 'Smoke Test '+'iOS'+' '+TestRunner.args.platform+' v '+TestRunner.iostest.APP_VERSION+' On '+TestRunner.iostest.SERVER
        # fp = file('report'+contants.PLATFORM+'.html', 'wb')
        # runner = HTMLTestRunner.HTMLTestRunner(
        #             stream=fp,
        #             title='Smoke Test '+'iOS'+' '+contants.PLATFORM_VERSION+' v '+TestRunner.iostest.APP_VERSION +  '  On  ' +TestRunner.iostest.SERVER,
        #             description='smoke test for '+TestRunner.iostest.APP_VERSION
        #             )
        # run the test
        runner = TeamcityTestRunner()
        runner.run(suite)
        appiumprocess.terminate()

    elif TestRunner.androidtest:
        print TestRunner.args.device_id
        appiumprocess = Popen(['appium', '-U',TestRunner.args.device_id])
        suite = unittest.TestLoader().loadTestsFromTestCase(TestRunner.iostest)
#       suite = unittest.TestLoader().loadTestsFromName(name='test_gamelaunch',module=TestRunner.iostest)
        print 'Smoke Test '+'Android'+' '+TestRunner.args.platform+' v '+TestRunner.androidtest.APP_VERSION+' On '+TestRunner.androidtest.SERVER

        appiumprocess.terminate()



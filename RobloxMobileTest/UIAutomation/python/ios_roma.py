__author__ = 'yiwen'
"""
Simple iOS tests, showing accessing elements and getting/setting text from them.
"""
import unittest
import os
from random import randint
from appium import webdriver
from time import sleep
from automationutils import contants
from automationutils import iostestbase
import time

class IosTestsRoma(iostestbase.IOSTestsBase):
    USERNAME = 'mobiletest'
    PASSWORD = 'hackproof12'
    SERVER = 'http://www.gametest1.robloxlabs.com/'
    TEST_GAME = 'Test Place #1'
    ADS_PLACE = 'Mobile Ad Test'
    LAUNCHGAME_TIMEOUT_= 10
# APP test server
    APPSERVER = 'http://www.sitetest1.robloxlabs.com/'

    TIMEOUT = 8

    def test001_login(self):
        """
        ROBLOX APP LOGIN TEST
        Steps:
        1.Verify the version on the screen
        2.Login in.

        """
        print "running test on version "+self.APP_VERSION
        self.assertTrue(self.verifyWebViewStaticText(textToVerify=self.APP_VERSION),'the version is not as expected')
        self.selectwheel(self.SERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        time.sleep(5)
        self.assertTrue(len(self.driver.find_elements_by_accessibility_id('Login'))==0,'login failed')

    def test002_catalog(self):
        """
        ROBLOX APP CATALOG TEST
        Steps:
        1.Login in.
        2.Click Nav Bar Button 'CATALOG'.
        3.Verify the Page by checking text 'Featured Items on ROBLOX'

        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        if self.DEVICE_TYPE == 'phone':
            self.driver.find_elements_by_accessibility_id('MORE')[0].click()
            time.sleep(3)
            #Inventory page will be the 3rd UIAButton in the grid
            self.driver.find_elements_by_class_name('UIAButton')[1].click()
        else:
            self.driver.find_elements_by_accessibility_id('CATALOG')[0].click()
        time.sleep(8)
        self.verifyCookieConstrains()
        time.sleep(8)
        if self.DEVICE_TYPE == 'phone':
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='All Categories'),'after click catalog bar the page did not going forward')
        else:
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='Featured Items on ROBLOX'),'after click catalog bar the page did not going forward')

    def test003_inventory(self):
        """
        ROBLOX APP INVENTORY TEST
        Steps:
        1.Login in.
        2.Click Nav Bar Button 'INVENTORY'.
        3.Verify the Page by checking text 'Character Customizer'

        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        self.driver.find_elements_by_accessibility_id('MORE')[0].click()
        time.sleep(3)
        if self.DEVICE_TYPE == 'phone':
            #Inventory page will be the 4rd UIAButton in the grid
            self.driver.find_elements_by_class_name('UIAButton')[4].click()
        else:
            #Inventory page will be the 3rd UIAButton in the grid
            self.driver.find_elements_by_class_name('UIAButton')[3].click()
        time.sleep(8)
        self.verifyCookieConstrains()
        time.sleep(8)
        self.assertTrue(self.verifyWebViewStaticText(textToVerify='My Inventory'),'after click inventory bar the page did not going forward')

    def test004_profile(self):
        """
        ROBLOX APP PROFILE TEST
        Steps:
        1.Login in.
        2.Click Nav Bar Button 'PROFILE'.
        3.Verify the Page by checking login username is on the screen

        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        self.driver.find_elements_by_accessibility_id('MORE')[0].click()
        time.sleep(3)
        if self.DEVICE_TYPE == 'phone':
            #Profile page will be the 1st UIAButton in the grid
            self.driver.find_elements_by_class_name('UIAButton')[2].click()
        else:
            #Profile page will be the 1st UIAButton in the grid
            self.driver.find_elements_by_class_name('UIAButton')[1].click()
        time.sleep(8)
        self.verifyCookieConstrains()
        time.sleep(8)
        self.assertTrue(self.verifyWebViewStaticText(textToVerify=self.USERNAME),'after click profile bar the page did not going forward')

    def test005_messages(self):
        """
        ROBLOX APP MESSAGES TEST
        Steps:
        1.Login in.
        2.Click Nav Bar Button 'MESSAGES'.
        3.Verify the Page by checking text 'Inbox'

        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        self.driver.find_elements_by_accessibility_id('MESSAGES')[0].click()
        time.sleep(8)
        self.verifyCookieConstrains()
        time.sleep(8)
        if self.DEVICE_TYPE == 'phone':
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='Sent by'),'after click profile bar the page did not going forward')
        else:
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='Inbox'),'after click profile bar the page did not going forward')

    def test006_group(self):
        """
        ROBLOX APP GROUP TEST
        Steps:
        1.Login in.
        2.Click Nav Bar Button 'MESSAGES'.
        3.Verify the Page by checking text 'Inbox'

        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        self.driver.find_elements_by_accessibility_id('MORE')[0].click()
        time.sleep(3)
        #Group page will be the 5th UIAButton in the grid
        self.driver.find_elements_by_class_name('UIAButton')[5].click()
        time.sleep(8)
        self.verifyCookieConstrains()
        time.sleep(10)
 #       can't find statictext
        if self.DEVICE_TYPE == 'phone':
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='Search for groups'),'after click group bar the page did not going forward')
        else:
            self.assertTrue(self.verifyWebViewStaticText(textToVerify='Create a Group'),'after click group bar the page did not going forward')

    def test007_game_launch(self):
        """
        ROBLOX APP GAME LAUNCH TEST
        Steps:
        1.Login in.
        2.in game page, click any game and join
        3.click chat button. check keyboard to verify the game is succefully launched.
        """
        self.selectwheel(self.SERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        time.sleep(2)
        self.driver.find_elements_by_accessibility_id('GAMES')[0].click()
        #self.verifyCookieConstrains()
        # else:
        #     # self.driver.find_elements_by_accessibility_id('MORE')[0].click()
        #     # time.sleep(3)
        #     # #Profile page will be the 1st UIAButton in the grid
        #     # self.driver.find_elements_by_class_name('UIAButton')[1].click()
        #     # self.verifyCookieConstrains()
        time.sleep(8)
        if self.DEVICE_TYPE is 'phone':
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()[4].tap();')
            time.sleep(5)
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.TEST_GAME+'"].scrollToVisible();')
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.TEST_GAME+'"].tap();')
            time.sleep(20)
            self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='game_launch')
            return
            assert False, 'cannot find the game '+self.TEST_GAME
        else:
            statictext = self.driver.find_elements_by_class_name('UIACollectionCell')
            for text in statictext:
                if  self.TEST_GAME in text.get_attribute('name'):
                    print 'game found.'
                    text.click()
                    time.sleep(3)
                    self.launch_game()
                    time.sleep(20)
                    self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='game_launch')
                    return
            assert False, 'cannot find the game '+self.TEST_GAME

    def test008_ingame_ads(self):
        """
        ROBLOX APP GAME LAUNCH TEST
        Steps:
        1.Login in.
        2.in game page, click 'Mobile Ad Tests' game and join
        3.for a certain time. take screenshot and sent to verifer
        """
        self.selectwheel(self.SERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        time.sleep(2)
        self.driver.find_elements_by_accessibility_id('GAMES')[0].click()
        #self.verifyCookieConstrains()
        time.sleep(8)
        if self.DEVICE_TYPE is 'phone':
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()[4].tap();')
            time.sleep(5)
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.ADS_PLACE+'"].scrollToVisible();')
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.ADS_PLACE+'"].tap();')
            time.sleep(20)
            self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='ingame_ads')
            return
            assert False, 'cannot find the game '+self.ADS_PLACE
        else:
            time.sleep(5)
            statictext2 = self.driver.find_elements_by_class_name('UIACollectionCell')
            for text in statictext2:
                if  self.ADS_PLACE in text.get_attribute('name'):
                    print 'ads play found.'
                    text.click()
                    time.sleep(3)
                    playlink = self.verifyWebLink(linktext='Play')
                    if playlink:
                        playlink.click()
                        time.sleep(20)
                        self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='ingame_ads')
                    return
            assert False, 'cannot find the game '+self.ADS_PLACE

    def test009_game_launch_st1(self):
        """
        ROBLOX APP GAME LAUNCH TEST on SiteTest1
        Steps:
        1.Login in.
        2.in game page, click any game and join
        3.click chat button. check keyboard to verify the game is succefully launched.
        """
        self.selectwheel(self.APPSERVER)
        self.login(username=self.USERNAME,password=self.PASSWORD)
        time.sleep(2)
        self.driver.find_elements_by_accessibility_id('GAMES')[0].click()
        self.verifyCookieConstrains()
        # else:
        #     # self.driver.find_elements_by_accessibility_id('MORE')[0].click()
        #     # time.sleep(3)
        #     # #Profile page will be the 1st UIAButton in the grid
        #     # self.driver.find_elements_by_class_name('UIAButton')[1].click()
        #     # self.verifyCookieConstrains()
        time.sleep(8)
        if self.DEVICE_TYPE is 'phone':
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()[4].tap();')
            time.sleep(5)
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.TEST_GAME+'"].scrollToVisible();')
            self.driver.execute_script('target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["'+self.TEST_GAME+'"].tap();')
            time.sleep(20)
            self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='game_launch')
            return
            assert False, 'cannot find the game '+self.TEST_GAME
        else:
            statictext = self.driver.find_elements_by_class_name('UIACollectionCell')
            for text in statictext:
                if  'Place' in text.get_attribute('name'):
                    print 'game found.'
                    text.click()
                    time.sleep(3)
                    self.launch_game()
                    time.sleep(20)
                    self.emailscreenshot('screenshot.jpg',send_to=[self.TESTER],testtitle='game_launch for sitetest1')
                    return
            assert False, 'cannot find the game '+self.TEST_GAME





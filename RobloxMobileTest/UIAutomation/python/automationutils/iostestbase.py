__author__ = 'yiwen'
import unittest
import os
from random import randint
from appium import webdriver
from time import sleep
import contants
import commonutils
import time
import subprocess as sp

class IOSTestsBase(unittest.TestCase):
    APP_VERSION = '0.0'
    DEVICE_NAME = contants.DEVICE_NAME
    internal = True
    APP_SRC = contants.APP_NAME
    DEVICE_TYPE = 'tablet'
    TESTER = ''

    def setUp(self):
        # set up appium
        sleep(2)
     #   sp.call('ios-deploy -i '+ self.DEVICE_NAME+' -b '+self.APP_SRC + ' -r', shell=True)
        # sleep(2)

        self.driver = webdriver.Remote(
            command_executor=contants.COMMAND_SERVER,
            desired_capabilities={
                'app': self.APP_SRC,
                'platformName': contants.PLATFORM,
                'platformVersion': contants.PLATFORM_VERSION,
                'deviceName': self.DEVICE_NAME,
                'autoAcceptAlerts': True,
            })
        sleep(2)

    def tearDown(self):
        self.driver.quit()

    def selectwheel(self,value):
        if self.internal:
            if self.DEVICE_TYPE == 'phone':
                value = value.replace('www','m')
            sleep(3)
            wheel = self.driver.find_elements_by_class_name('UIAPickerWheel')
            wheel[0].send_keys(value)

    def login(self,username,password):
        buttons = self.driver.find_elements_by_class_name('UIAButton')
        for button in buttons:
            if button.get_attribute('name') == 'Login':
                button.click()
                # clear username first
                self.driver.find_elements_by_class_name('UIATextField')[0].clear()
                self.driver.find_elements_by_class_name('UIATextField')[0].send_keys(username)
                self.driver.find_elements_by_class_name('UIASecureTextField')[0].send_keys(password)
                if self.DEVICE_TYPE == 'phone':
                    buttons = self.driver.find_elements_by_class_name('UIAButton')
                    for button in buttons:
                        if button.get_attribute('name') == 'Log in':
                            button.click()
                else:
                    self.driver.find_elements_by_accessibility_id('Log in')[0].click()
        time.sleep(3)

    def emailscreenshot(self,full_filename,send_to,testtitle):
        self.driver.save_screenshot(full_filename)
        time.sleep(3)
        commonutils.send_mail(send_from='ywen@roblox.com',send_to=send_to,subject=testtitle+' result',text='automation test screen shot for '+testtitle,attfile=full_filename)

    def verifyWebViewStaticText(self,textToVerify,source='webriver'):

        if source == 'ios':
   #         statictext = self.driver.find_elements_by_ios_uiautomation(".scrollViews()[0].webViews()[0].staticTexts()['"+textToVerify+"']")

            statictext = self.driver.find_elements_by_xpath('//UIAApplication[1]/UIAWindow[1]/UIAScrollView[1]/UIAWebView[1]/UIAStaticText')
        else:
            statictext = self.driver.find_elements_by_class_name('UIAStaticText')
        for text in statictext:
            if text.get_attribute('name') == textToVerify:
               return True
        return False


    def verifyCookieConstrains(self,keys='rewards'):
        temp = []
        if self.verifyWebViewStaticText(textToVerify='The site is currently offline for maintenance and upgrades.'):
            self.driver.find_elements_by_class_name('UIASecureTextField')[0].send_keys(keys)
            buttons = self.driver.find_elements_by_class_name('UIAButton')
            for button in buttons:
                if button.get_attribute('name') == 'O':
                    temp.append(button)
            temp[1].click()
        time.sleep(10)



    def verifyKeyboard(self):
        return self.driver.execute_script('target.frontMostApp().keyboard().isValid()')

    def verifyWebLink(self,linktext,source='webriver'):

        if source == 'ios':
   #         statictext = self.driver.find_elements_by_ios_uiautomation(".scrollViews()[0].webViews()[0].staticTexts()['"+textToVerify+"']")

            links = self.driver.find_elements_by_xpath('//UIAApplication[1]/UIAWindow[1]/UIAScrollView[1]/UIAWebView[1]/UIAStaticText')
        else:
            links = self.driver.find_elements_by_class_name('UIALink')
        for link in links:
            if link.get_attribute('name') == linktext:
               return link
        return None

    def launch_game(self):
        playlink = self.verifyWebLink(linktext='Play')
        #   self.assertTrue(playlink is None,'Did not found Play link on Test Place #1 ')
        if playlink:
            playlink.click()
        # wait till game gets loaded
            sleep(10)
            # self.driver.execute_script('target.frontMostApp().mainWindow().images()[0].images()[0].tapWithOptions({tapOffset:{x:0.09, y:0.03}});')
            # self.assertTrue(self.verifyKeyboard() == True ,'keyboard did not launch , game was not launched')
            return
        assert False ,"can't found the Play link on page"
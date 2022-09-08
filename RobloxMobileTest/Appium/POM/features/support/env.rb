# encoding: utf-8
require 'rspec/expectations'
require 'appium_lib'
require 'cucumber/ast'
require 'page_helper'
require 'random_data'
require 'require_all'
require 'simplecov'
require 'simplecov-rcov'
SimpleCov.formatters = [
  SimpleCov::Formatter::HTMLFormatter,
  SimpleCov::Formatter::RcovFormatter
]
SimpleCov.start

require './lib/page_helper'
require './lib/data_helper'
require_all './lib/pages'

class AppiumWorld
end

case ENV['OS']
when 'ios'
  caps = Appium.load_appium_txt file: File.expand_path('../ios/appium.txt', __FILE__), verbose: true
when 'android'
  caps = Appium.load_appium_txt file: File.expand_path('../android/appium.txt', __FILE__), verbose: true
end

WAITTIME = 35

Appium::Driver.new(caps)
Appium.promote_appium_methods AppiumWorld
Appium.promote_singleton_appium_methods [DataHelper, PageHelper]

World do
  AppiumWorld.new
end

World PageHelper
World DataHelper

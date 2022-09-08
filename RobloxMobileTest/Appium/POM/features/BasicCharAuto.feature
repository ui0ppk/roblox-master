@ios
Feature: Basic Character Automation

Scenario: Basic Character Automation
  Given I login using "wei" and "sanmateo"
  And I should see "Hello, wei!" texts displayed
  When I enter profile game
  And I tap chat
  Then I should see "return" enable

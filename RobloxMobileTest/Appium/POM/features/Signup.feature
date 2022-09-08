@android@ios
Feature: Signup

Scenario: Tap the "Sign Up" button to sign up
  Given I logout
  When I tap "Sign Up" button
  Then I should see "SIGNUP" texts
  And I sign up as a "new" user

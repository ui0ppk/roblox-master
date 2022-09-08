Feature: Play Now

@android
Scenario: Tap the "Play Now" button and play games
  Given I logout
  When I play now
  Then I should see "Popular"

@ios
Scenario: Tap the "Play Now" button and play games
  Given I logout
  When I play now
  Then I should see "Popular" texts displayed

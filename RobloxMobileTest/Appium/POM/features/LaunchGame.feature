Feature: Game Launch

@android
Scenario: Game Launch
  Given I logout
  And I login as "usertest01"
  And I should see "Home" texts displayed
  When I enter a game
  And I tap chat
  Then I should see id "com.roblox.client.dev:id/gl_edit_text" enable
  And I hide keyboard
  
@ios
Scenario: Game Launch
  Given I login as "usertest01"
  And I should see "Hello, usertest01!" texts displayed
  When I enter a game
  And I tap chat
  Then I should see "return" enable

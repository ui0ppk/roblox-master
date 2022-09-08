Feature: Login

Scenario Outline: Login
  Given I logout
  When I login using "<Username>" and "<Password>"
  Then I should see "<Result>" texts

@ios@android
Examples:
  | Username   | Password    | Result |
  | usertest01 | b0dPasswd   | Invalid username or password|
  | Badusrname | hackproof   | Invalid username or password|
  | Badusrn0me | badPasswd   | Invalid username or password|
  | usertest01 | hackproof   | Home |

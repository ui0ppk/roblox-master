Then(/^I should see "(.*?)" enable$/) do |data|
  expect(wait { find_element(:accessibility_id, data) }.enabled?).to be true
end

Then(/^I should see id "(.*?)" enable$/) do |data|
  expect(wait { find_element(:id, data) }.enabled?).to be true
end

Then(/^I should see "(.*?)"$/) do |text|
  found_text = wait { find_element(:accessibility_id, text) }
  expect(found_text.enabled? || found_text.displayed?).to be true
end

Then(/^I should see "(.*?)" texts$/) do |texts|
  if ENV['OS'] == 'ios'
    found_text = find_element(:xpath, "//UIAStaticText[@name='" + texts + "']")
    expect(found_text.enabled? || found_text.displayed?).to be true
  else
    wait_true { text(texts) }.displayed?
  end
end

Then(/^I should see "(.*?)" texts displayed$/) do |data|
  wait_true { text(data) }.displayed?
end

Given(/^I login as "(.*?)"$/) do |username|
  on(LoginPage).launch_login username, 'hackproof'
end

Given(/^I login using "(.*?)" and "(.*?)"$/) do |username, password|
  on(LoginPage).launch_login username, password
end

When(/^I tap (\d+),(\d+)$/) do |x, y|
  Appium::TouchAction.new.tap(x: x, y: y).perform
end

When(/^I tap "(.*?)" button$/) do |data|
  wait { button(data) }.click
end

# On iPad mini, have to hardcode tap on play button link since it looks disable in appium inspector
When(/^I enter a game$/) do
  wait { on(GamePage).game_button }.click
  if ENV['OS'] == 'ios'
    if ENV['FF'] == 'phone'
      wait { on(GamePage).play_button }.click
    else
      wait { on(GamePage).play_button }
      step 'I tap 815,315'
    end
  else
    wait { on(GamePage).play_button }.click
  end
end

When(/^I enter profile game$/) do
  on(ProfilePage).profile_game
end

When(/^I tap chat$/) do
  sleep 15 # wait till it loads the game
  if ENV['OS'] == 'ios'
    if ENV['FF'] == 'phone'
      step 'I tap 127,20' # iPhone6
    else
      step 'I tap 132,24' # iPad mini
    end
  else
    step 'I tap 255,51' # Nexus 9  # 259,45 # Nexus 7
  end
end

Then(/^I logout$/) do
  on(LogoutPage).logout
end

Then(/^I play now$/) do
  on(LaunchPage).playnow.click
end

When(/^I sign up as a "(.*?)" user$/) do |data|
  signupname = 'Hello, ' + on(SignupPage).sign_up(data, 'appium123') + '!'
  puts 'AUTO: ' + signupname
  if ENV['OS'] == 'ios'
    wait_true { exists { text signupname } }
  else
    wait_true { exists { text 'Home' } }
  end
end

And(/^I hide keyboard$/) do
  hide_keyboard
end

# When(/^I wait to debug$/) do
#   debug
# end
#
# When(/^I tap "(.*?)"$/) do |data|
#   wait { find_element(:accessibility_id, data) }.click
# end
#
# Then(/^I should not see "(.*?)"$/) do |text|
#   expect(something_present(text)).to be false
# end
#
# When(/^I sleep (\d+)$/) do |second|
#   sleep second.to_i
# end
#
# When(/^I wait for "(.*?)"$/) do |elementname|
#   wait { find_element(:accessibility_id, elementname) }
# end
#
# When(/^I tap xpath "(.*?)"$/) do |data|
#   wait { xpath(data) }.click
# end
#
# When(/^I tap button (\d+)$/) do |number|
#   wait { button(number.to_i) }.click
# end
#
# Then(/^I tap button "(.*?)"$/) do |name|
#   wait { button_exact(name) }.click
# end
#
# When(/^I tap text (\d+)$/) do |number|
#   wait { text_exact(number.to_i) }.click
# end
#
# Then(/^I tap text "(.*?)"$/) do |name|
#   wait { text_exact(name) }.click
# end
#
# Then(/^I should see "(.*?)" disable$/) do |data|
#   expect(wait { find_element(:accessibility_id, data) }.enabled?).to be false
# end
#
# Then(/^I should see Login button$/) do
#   expect(login_button_present).to be true
# end
#
# When(/^I enter "(.*?)"$/) do |something|
#   driver.keyboard.send_keys something
# end

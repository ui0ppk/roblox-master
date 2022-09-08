class SignupPage < GenericBasePage
  include PageHelper
  include DataHelper

  element(:accept_button) { |b| b.button_exact('Accept') }
  element(:signup_button) { |b| b.button_exact('Sign Up') }
  element(:validating_button) { |b| b.button('Validating') }
  case ENV['OS']
  when 'android'
    element(:username) { |b| b.textfield('signupUsername') }
    element(:password) { |b| b.textfield('signupPassword') }
    element(:confirm_password) { |b| b.textfield('signupPasswordConfirm') }
    element(:male_button) { |b| b.button('rbxGenderPickerMale') }
    element(:female_button) { |b| b.button('rbxGenderPickerFemale') }
    element(:month) { |b| b.find_element(:xpath, '//android.widget.RelativeLayout[1]/android.widget.Spinner[1]/android.widget.TextView[1]') }
    element(:day) { |b| b.text('--') }
    element(:year) { |b| b.text('----') }
  when 'ios'
    element(:username) { |b| b.textfield(1) }
    element(:password) { |b| b.textfield(2) }
    element(:confirm_password) { |b| b.textfield(3) }
    element(:male_button) { |b| b.button('Gender Male') }
    element(:female_button) { |b| b.button('Gender Female') }
    element(:birthday_button) { |b| b.button(4) }
    element(:month) { |b| b.first_ele('UIAPickerWheel') }
    element(:day) { |b| b.find_element(:xpath, '//UIAPickerWheel[2]') }
    element(:year) { |b| b.last_ele('UIAPickerWheel') }
  end

  def sign_up(usrname, passwd)
    name = Random.firstname + Random.lastname + Random.number(999).to_s
    usrname = 'Appium' + name[0..13] if usrname == 'new'
    username.send_keys usrname
    password.send_keys passwd
    confirm_password.send_keys passwd
    case ENV['OS']
    when 'android'
      hide_keyboard
      male_button.click
      month.click
      wait { text('Aug.') }.click
      day.click
      wait { text('1') }.click
      year.click
      wait { text('2012') }.click
      # wait { signup_button }.click
      # sleep 20 # wait for processing in GT2 and click twice, maybe capcha issues.
    when 'ios'
      female_button.click
      birthday_button.click
      # workaround on iOS for issue https://groups.google.com/forum/#!topic/appium-discuss/uYdRTdQDvpU
      if ENV['FF'] == 'tablet' # iPad Mini
        swipe(start_x: 306, start_y: 405, end_x: 306, end_y: 329, duration: 500)
        swipe(start_x: 511, start_y: 405, end_x: 511, end_y: 329, duration: 500)
      else
        swipe(start_x: 80, start_y: 270, end_x: 80, end_y: 170, duration: 500) # June
        swipe(start_x: 150, start_y: 270, end_x: 150, end_y: 170, duration: 500) # 11
      end
      year.send_keys '2006'
      accept_button.click
    end
    wait { signup_button }.click
    usrname
  end
end

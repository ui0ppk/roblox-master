class LoginPage < GenericBasePage
  include PageHelper
  include DataHelper

  element(:username) { |b| b.first_textfield }
  element(:password) { |b| b.last_textfield }
  if ENV['OS'] == 'android'
    element(:signin_button) { |b| b.button_exact('Login') }
  else
    element(:signin_button) { |b| b.button_exact('Log in') }
  end

  def login(usrname = 'usertest01', passwd = 'hackproof')
    username.send_keys usrname
    password.send_keys passwd
    hide_keyboard if ENV['OS'] == 'android'
    signin_button.click
  end

  def login_yml(options = {})
    options = options.with_indifferent_access
    options.to_hash.reverse_merge! data_yml_hash.with_indifferent_access
    username.send_keys data_yml_hash['username']
    password.send_keys data_yml_hash['password']
    signin_button.click
  end

  def launch_login(usrname, passwd)
    on(LogoutPage).logout unless login_button_present
    wait { on(LaunchPage).login }.click
    login(usrname, passwd)
  end

  def login_button_present
    exists { on(LaunchPage).login }
  end
end

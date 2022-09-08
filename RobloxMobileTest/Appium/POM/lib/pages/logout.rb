class LogoutPage < GenericBasePage
  include PageHelper

  element(:more) { |b| b.find_element(:accessibility_id, 'MORE') }
  # 9 for tablet, 7 for iphone
  if ENV['FF'] == 'phone'
    element(:settings) { |b| b.button(7) }
  else
    element(:settings) { |b| b.button(9) }
  end
  element(:logoutlink) { |b| b.find_element(:accessibility_id, 'Logout') }
  element(:confirm_logout) { |b| b.button_exact('Log Out') }

  def logout
    if on(LoginPage).login_button_present
      sleep 20 if ENV['OS'] == 'android'
      puts 'AUTO: already logout'
    else
      more.click
      case ENV['OS']
      when 'ios'
        settings.click
        logoutlink.click
      when 'android'
        logoutimage.click
      end
      confirm_logout.click
    end
    wait_true { on(LaunchPage).login }.displayed?
  end
end

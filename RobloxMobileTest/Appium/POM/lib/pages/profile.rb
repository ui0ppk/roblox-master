class ProfilePage < GenericBasePage
  include PageHelper

  element(:more) { |b| b.find_element(:accessibility_id, 'MORE') }
  # 1 for tablet, 2 for iphone
  if ENV['FF'] == 'phone'
    element(:profile) { |b| b.button(2) }
    element(:my_game) { |b| b.find_ele_by_attr('UIALink', 'name', 'Basic Character Automation') }
    element(:play_button) { |b| b.find_ele_by_attr('UIALink', 'name', 'Play') }
  else
    element(:profile) { |b| b.button(1) }
    element(:my_game) { |b| b.ele_index('UIALink', 9) }
    element(:play_button) { |b| b.find_element(:xpath, "//UIAStaticText[@name='Play']") }
  end
  element(:creations) { |b| b.find_ele_by_attr('UIALink', 'name', 'Creations') }
  element(:grid_view) { |b| b.button('Grid View') }

  def profile_game
    more.click
    case ENV['OS']
    when 'ios'
      wait { profile }.click
      wait { creations }.click
      wait { grid_view }.click if ENV['FF'] == 'phone'
      wait { my_game }.click
      if ENV['FF'] == 'phone'
        wait { play_button }.click
      else
        wait { play_button }
        Appium::TouchAction.new.tap(x: 815, y: 315).perform
      end
    when 'android'
    end
  end
end

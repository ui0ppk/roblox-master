class GamePage < GenericBasePage
  if ENV['OS'] == 'ios'
    element(:games_navbar) { |b| b.button_exact('GAMES') }
    element(:game_button) { |b| b.find_ele_by_attr('UIALink', 'name', 'Swimming test') }
    # element(:play_button) { |b| b.ele_index('UIALink', 2) }
    # element(:play_button) { |b| b.find_ele_by_attr('UIALink', 'name', 'Play') }
    # element(:play_button) { |b| b.find_element(:xpath, '//UIAApplication[1]/UIAWindow[1]/UIAScrollView[1]/UIAWebView[1]/UIALink[2]/UIAStaticText[1]') }
    element(:play_button) { |b| b.find_element(:xpath, "//UIAStaticText[@name='Play']") }
  else
    element(:games_navbar) { |b| b.text_exact('Games') }
    element(:game_button) { |b| b.find_element(:accessibility_id, 'Swimming test Swimming test 0 Playing ') }
    element(:play_button) { |b| b.find_element(:accessibility_id, 'Play') }
  end
end

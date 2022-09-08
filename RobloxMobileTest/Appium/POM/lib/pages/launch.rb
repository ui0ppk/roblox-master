class LaunchPage < GenericBasePage
  element(:login) { |b| b.button('Login') }
  element(:signup) { |b| b.button('Sign Up') }
  element(:playnow) { |b| b.button('Play Now') }
  element(:moreinfo) { |b| b.button('More Info') }
  element(:version) { |b| b.last_text }
end

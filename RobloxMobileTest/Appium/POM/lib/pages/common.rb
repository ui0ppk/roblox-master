def something_present(text)
  begin
    @wait = Selenium::WebDriver::Wait.new(timeout: WAITTIME)
    @wait.until { find_element(:accessibility_id, text).displayed? }
    @something_displayed = find_element(:accessibility_id, text).displayed?
  rescue
    @something_displayed = false
  end
  @something_displayed
end

def debug
  require 'pry'
  binding.pry
end

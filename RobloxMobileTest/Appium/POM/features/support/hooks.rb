system 'killall node'
system 'appium --log-level error&'
sleep 15

begin
  at_exit do
    system 'killall node'
  end
rescue
  puts 'AUTO: appium not killed at exit'
end

Before do
  begin
    $driver.start_driver
    @browser = $driver
  rescue
    puts 'AUTO: driver not initialized'
  end
end

After do |scenario|
  begin
    if $driver
      if scenario.failed?
        filename = "FAIL-#{scenario.__id__}.png"
        filename = 'reports/' + filename
        screenshot(filename)
        embed(filename, 'image/png', 'SCREENSHOT')
      end
      $driver.driver_quit
    end
  rescue
    puts 'AUTO: driver not exist'
  end
end

class GenericBasePage
  def initialize(browser, visit = false)
    @browser = browser
    goto if visit
    # respond_to? is a Ruby method for detecting whether the class has a particular method on it
    expected_element if respond_to? :expected_element
    has_expected_title? if respond_to? :has_expected_title?
  end

  def method_missing(sym, *args, &block)
    @browser.send sym, *args, &block
  end

  def self.page_url(url)
    define_method 'goto' do
      @browser.goto url
    end
  end

  # This is a class level method
  class << self
    alias direct_url page_url
  end

  # Even this is a class method. Just another way to specify. So either class << self or self.method are same
  # Class methods are for anything that does not deal with an individual instance of a class
  def self.expected_element(type, identifier, timeout = 30)
    define_method 'expected_element' do
      @browser.send(type.to_s, identifier).wait_until_present timeout
    end
  end

  def self.expected_title(expected_title)
    define_method 'has_expected_title?' do
      has_expected_title = expected_title.is_a?(Regexp) ? expected_title =~ @browser.title : expected_title == @browser.title
      raise "Expected title '#{expected_title}' instead of '#{@browser.title}'" unless has_expected_title
    end
  end

  def self.element(element_name)
    define_method element_name.to_s do
      # yield self enters block, associated with method call, passing current object as argument to the block
      yield self
    end
  end

  class << self
    alias value element
    alias action element
  end
end

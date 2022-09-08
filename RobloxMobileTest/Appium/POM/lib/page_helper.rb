
module PageHelper
  # http://watirmelon.com/2011/06/07/removing-local-page-references-from-cucumber-steps/
  # http://stackoverflow.com/questions/3424782/ruby-on-rails-how-do-i-check-if-a-variable-in-an-instance-of-a-class
  # http://www.wellho.net/mouth/2601_Ruby-is-a-v-instance-of-what-is-the-difference-.html
  def visit(page_class, &block)
    on(page_class, true, &block)
  end

  def on(page_class, visit = false, &block)
    page_class = class_from_string(page_class) if page_class.is_a? String
    page = page_class.new @browser, visit
    yield page if block
    page
  end

  private

  # http://stackoverflow.com/questions/3163641/get-a-class-by-name-in-ruby

  def class_from_string(str)
    str.split('::').inject(Object) do |mod, class_name|
      mod.const_get(class_name)
    end
  end
end

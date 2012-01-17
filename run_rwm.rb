require "~/rwm/rwm"
class RWM::Layout; attr_accessor :keys; end
class DwmStyle < RWM::Layout
  def initialize(m)
    @keys = {}
    @keys[[RWM::Control, "h"]] = Proc.new do
      @width -= 10
      m.arrange
    end
    @keys[[RWM::Control, "i"]] = Proc.new do
      @width += 10
      m.arrange
    end
  end
  def arrange(m)
  if m.windows.size == 1
      m.windows[0].resize 0, 20, m.width, m.height-20
    else
      @width ||= m.width / 2
      m.windows.last.resize 0, 20, @width, m.height-20
      height = (m.height-20) / (m.windows.size-1)
      m.windows[0..(m.windows.size-2)].reverse.each_with_index do |w,i|
        w.resize @width, 20+(i*height), m.width-@width, height
      end
    end
  end
end
class TileHorizontally < RWM::Layout
  def arrange(m)
    width = m.width / m.windows.size
    m.windows.each_with_index do |w,i|
     w.resize i*width, 20, width, m.height-20
    end
  end
end
class TileVertically < RWM::Layout
  def arrange(m)
   height = m.height / m.windows.size
    m.windows.each_with_index do |w,i|
      w.resize 0, 20+(i*height), m.width, height
    end
  end
end
class RWM::Window
  attr_accessor :view
  def set_view(view)
    @view = view
  end
end
class RWM::View
  attr_accessor :name
  def initialize(name)
    @name = name
  end 
end
class RWM::Display
attr_accessor :keys, :windows, :width, :height, :layout
  def initialize
    @keys = {}
    @s_view = 0
    @windows = []
    @last = 0
    @layout = DwmStyle.new self
    @focused_window = nil
  end
  def set_focused_window(window)
    @focused_window = window
  end
  def find_action(str)
   @keys.merge(@layout.keys||{}).find{|action| action[0][1] == str}[1]
  end
  def windows
    @windows.select{|w| w.view == @s_view ? true : w.hide }
  end
  def tab
   if(@last >= windows.size)
     @last = 0
  end
    focus windows[@last]
    @last += 1
  end
  def register_key(modifier, key, action=nil, &action_blk)
    @keys[[modifier, key]] = action || action_blk
  end
  def arrange
    return if windows.size < 1
    @layout.arrange self
    sync
  end
  def switch_to(layout)
    @layout = layout
    @layout.register_keys 0
    arrange
  end
  def draw_tags
    @views.map{|v| write_text " [#{v.name}] ", v == @views[@s_view] ? @sel_bg : @normal_bg, v == @views[@s_view] ? @sel_fg : @normal_fg }
  end
  def draw_title
    write_text @focused_window ? "".gsub(/[^\x20-\x7E]/,'') : "", @sel_bg, @sel_fg
  end
  def update_status
    Time.now.to_s + " w: #{@windows.size}"
  end
  def reload 
    load "~/rwm/config.rb"
    Config.config(self)
    get_config
    register_keys 1
    @layout.register_keys 0
    #create_bar
  end
    [:normal_bg, :normal_fg, :sel_bg, :sel_fg, :normal_border_colour, :sel_border_colour].each do |name|
    define_method name do |colour|
      instance_variable_set "@#{name}", get_colour(colour)
    end
  end
  def border_width(size)
    @border_width = size
  end
  def views(*names)
    @views = names.map{|n| RWM::View.new n}
  end
end
x = RWM::Display.new
x.open_display
x.reload
x.create_bar
x.run

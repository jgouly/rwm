module Config
  include RWM
    def self.config(rwm)
      rwm.instance_eval do 
        normal_bg "#802447" 
        normal_fg "#F050A7"
        sel_bg "#ddddaa"
        sel_fg "#4d4e4f"
        border_width 2
        normal_border_colour "#A04467"
        sel_border_colour "#ffffcc"

        views "one", "two", "three"
        (1..@views.size).each do |v|
          register_key Shift, v.to_s do
              @s_view = v - 1
              arrange
              draw_bar 
              focus windows[0] if windows[0]
            end
        end
 
        register_key Control, "a" do
          system "~/st/st &" 
        end
        register_key Control, "b" do
          system "chromium &"
        end
        register_key Control|Shift, "Tab", "tab"
        register_key Control, "r",   "reload"

        register_key Control, "v" do
          switch_to TileVertically.new
        end
        register_key Control, "h" do
          switch_to TileHorizontally.new
        end
        register_key Control, "d" do
          switch_to DwmStyle.new self
        end
        register_key Control, "q", "quit"
      end
    end
end

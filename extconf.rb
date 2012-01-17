require "mkmf"
RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']
dir_config 'rwm'
have_library 'X11'
create_makefile 'rwm'

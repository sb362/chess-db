project('cdb', ['c', 'cpp'],
  version : '0.1',
  default_options : [
  	#'cpp_std=c++23',
    'c_std=c17',
    'warning_level=3',
    'default_library=static']
)

# https://github.com/hyprwm/Hyprland/blob/main/meson.build#L14
cpp = meson.get_compiler('cpp')
if cpp.has_argument('-std=c++23')
  add_global_arguments('-std=c++23', language: 'cpp')
elif cpp.has_argument('-std=c++2b')
  add_global_arguments('-std=c++2b', language: 'cpp')
else
  error('Could not configure current C++ compiler (' + cpp.get_id() + ' ' + cpp.version() + ') to use C++23')
endif

subdir('src')

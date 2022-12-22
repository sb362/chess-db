
src_inc = include_directories('.')

util_srcs = ['util/logger.cc']
util_hdrs = ['util/logger.hh', 'util/source_location.hh']

install_headers(util_hdrs, preserve_path : true)

util_lib = library('util', sources : util_srcs, include_directories : src_inc, install : true)
util_dep = declare_dependency(include_directories : src_inc, link_with : [util_lib])

main_exe = executable('cdb', 'main.cc', install : true, dependencies : util_dep)

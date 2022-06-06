
db_inc = include_directories('.')

db_src = ['bitstream.cc']

db_lib = static_library('db',
	sources             : db_src,
	include_directories : src_inc,
	dependencies        : [fmt_dep, chess_dep]
)

db_dep = declare_dependency(
	link_with           : db_lib,
#	include_directories : db_inc
)

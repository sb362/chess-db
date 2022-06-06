
chess_inc = include_directories('.')

chess_src = ['bits.c', 'position.c', 'movegen.c', 'notation.c']

chess_lib = static_library('chess',
	sources             : chess_src,
	include_directories : chess_inc
)

chess_dep = declare_dependency(
	link_with           : chess_lib,
	include_directories : chess_inc
)

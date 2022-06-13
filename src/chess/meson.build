
chess_inc = include_directories('.')

chess_src = ['bits.c', 'position.c', 'movegen.c', 'text.c']

chess_lib = static_library('chess',
	sources             : chess_src,
	include_directories : chess_inc
)

chess_dep = declare_dependency(
	link_with           : chess_lib,
	include_directories : chess_inc
)

if build_tests
	chess_tests_exe = executable('perft',
		sources : 'test.c',
		link_with : chess_lib
	)
endif

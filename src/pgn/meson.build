
pgn_inc = include_directories('.')

pgn_src = ['game.cc', 'parser.cc']

pgn_lib = static_library('pgn',
	sources             : pgn_src,
	include_directories : src_inc,
	dependencies        : [fmt_dep, chess_dep]
)

pgn_dep = declare_dependency(
	link_with           : pgn_lib,
#	include_directories : pgn_inc
)

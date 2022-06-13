
src_inc = include_directories('.')

subdir('chess')
subdir('pgn')
subdir('db')

executable(meson.project_name(),
	sources      : ['main.cc'],
	dependencies : [fmt_dep, chess_dep, pgn_dep, db_dep],
	install      : true
)

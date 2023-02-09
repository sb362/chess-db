
src_inc = include_directories('.')

cpp = meson.get_compiler('cpp')
if cpp.get_id() == 'clang'
  add_project_arguments('-fconstexpr-steps=11180420', language : 'cpp')
endif

add_project_arguments('-march=znver3', language : 'cpp')


util_srcs = ['util/logger.cc']
util_hdrs = ['util/logger.hh', 'util/source_location.hh']

install_headers(util_hdrs, preserve_path : true)

util_lib = library('util', sources : util_srcs, include_directories : src_inc, install : true)
util_dep = declare_dependency(include_directories : src_inc, link_with : [util_lib])


chess_srcs = ['chess/bitboard.cc', 'chess/position.cc', 'chess/movegen.cc', 'chess/notation.cc']
chess_hdrs = ['chess/bitboard.hh', 'chess/position.hh', 'chess/movegen.hh', 'chess/notation.hh', 'chess/pgn.hh']

install_headers(util_hdrs, preserve_path : true)

chess_lib = library('chess', sources : chess_srcs, include_directories : src_inc,
                    dependencies : util_dep, install : true)
chess_dep = declare_dependency(include_directories : src_inc, link_with : [chess_lib])


main_exe = executable('cdb', 'main.cc', install : true, dependencies : [util_dep, chess_dep])

perft_exe = executable('perft', 'tests/perft.cc', install : true, dependencies : [util_dep, chess_dep])
test('perft', perft_exe)

san_exe = executable('san', 'tests/san.cc', install : true, dependencies : [util_dep, chess_dep])
test('san', san_exe)

pgn_exe = executable('pgn', 'tests/pgn.cc', install : true, dependencies : [util_dep, chess_dep])
test('pgn', pgn_exe)


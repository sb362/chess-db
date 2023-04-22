
src_inc = include_directories('.')

cpp = meson.get_compiler('cpp')

if cpp.get_id() == 'clang' or cpp.get_id() == 'gcc'
  add_project_arguments('-march=znver3', language : 'cpp')
  add_project_arguments('-D_HAS_CXX23',  language : 'cpp') # hack to get C++23 working

  if cpp.get_id() == 'clang'
    add_project_arguments('-fconstexpr-steps=11180420', language : 'cpp')
  endif

  add_project_arguments('-g3', language : 'cpp')

  if host_machine.system() == 'windows' # hack for colours on windows
    add_project_arguments('-fansi-escape-codes',  language : 'cpp')
  endif
elif cpp.get_id() == 'msvc'
  add_project_arguments('/constexpr:steps21180420', language : 'cpp')
endif

# util
util_srcs = []
util_hdrs = ['util/bits.hh', 'util/bytesize.hh', 'util/source_location.hh', 'util/vector.hh']

install_headers(util_hdrs, preserve_path : true)

util_lib = library('util', sources : util_srcs, include_directories : src_inc, install : true)
util_dep = declare_dependency(include_directories : src_inc)


# core
core_srcs = ['core/io.cc', 'core/logger.cc', 'core/error.cc']
core_hdrs = ['core/io.hh', 'core/logger.hh', 'core/error.hh']

install_headers(core_hdrs, preserve_path : true)

core_lib = library('core', sources : core_srcs, include_directories : src_inc,
                   dependencies : [util_dep], install : true)
core_dep = declare_dependency(include_directories : src_inc, link_with : [core_lib])


# chess
chess_srcs = ['chess/bitboard.cc', 'chess/position.cc', 'chess/movegen.cc', 'chess/notation.cc']
chess_hdrs = ['chess/bitboard.hh', 'chess/position.hh', 'chess/movegen.hh', 'chess/notation.hh', 'chess/pgn.hh']

install_headers(chess_hdrs, preserve_path : true)

chess_lib = library('chess', sources : chess_srcs, include_directories : src_inc,
                    dependencies : [util_dep, core_dep], install : true)
chess_dep = declare_dependency(include_directories : src_inc, link_with : [chess_lib])


# db
db_srcs = ['db/db.cc']
db_hdrs = ['db/codec.hh', 'db/page.hh']

install_headers(db_hdrs, preserve_path : true)

db_lib = library('db', sources : db_srcs, include_directories : src_inc,
                    dependencies : [util_dep, core_dep, chess_dep], install : true)
db_dep = declare_dependency(include_directories : src_inc, link_with : [db_lib])


# main
main_exe = executable('cdb', 'main.cc', install : true, dependencies : [util_dep, core_dep, chess_dep, db_dep])

perft_exe = executable('perft', 'tests/perft.cc', install : true, dependencies : [util_dep, core_dep, chess_dep])
test('perft', perft_exe)

san_exe = executable('san', 'tests/san.cc', install : true, dependencies : [util_dep, core_dep, chess_dep])
test('san', san_exe)

san2_exe = executable('san2', 'tests/san2.cc', install : true, dependencies : [util_dep, core_dep, chess_dep])
test('san2', san2_exe)

pgn_exe = executable('pgn', 'tests/pgn.cc', install : true, dependencies : [util_dep, core_dep, chess_dep])
test('pgn', pgn_exe)


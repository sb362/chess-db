
src_inc = include_directories('.')

cpp = meson.get_compiler('cpp')


if cpp.get_id() == 'clang' or cpp.get_id() == 'gcc'
  add_project_arguments('-march=znver3', language : 'cpp')
  add_project_arguments('-D_HAS_CXX23',  language : 'cpp') # hack to get C++23 working
  add_project_arguments('-fno-omit-frame-pointer',  language : 'cpp')

  add_project_arguments('-Wno-unused-parameter', language : 'cpp')

  if cpp.get_id() == 'clang'
    add_project_arguments('-fconstexpr-steps=11180420', language : 'cpp')

	  #add_project_arguments('-stdlib=libc++', language : 'cpp')
    #if cpp.find_library('libc++', required : false).found()
    #  add_project_arguments('-stdlib=libc++', language : 'cpp')
    #endif
  endif

  if host_machine.system() == 'windows' # hack for colours on windows
    add_project_arguments('-fansi-escape-codes',  language : ['c', 'cpp'])
  endif
elif cpp.get_id() == 'msvc'
  add_project_arguments('/constexpr:steps21180420', language : 'cpp')
endif

# dependencies
doctest_dep = dependency('doctest')
fmt_dep = dependency('fmt')


# util
util_srcs = []
util_hdrs = ['util/bits.hh', 'util/bytesize.hh', 'util/iterator.hh',
             'util/misc.hh', 'util/source_location.hh', 'util/vector.hh']

install_headers(util_hdrs, preserve_path : true)

util_lib = library('util', sources : util_srcs, include_directories : src_inc,
                   dependencies : [fmt_dep], install : true)
util_dep = declare_dependency(include_directories : src_inc)


# core
core_srcs = ['core/io.cc', 'core/logger.cc', 'core/error.cc', 'core/byteio.cc']
core_hdrs = ['core/io.hh', 'core/logger.hh', 'core/error.hh', 'core/byteio.hh']

install_headers(core_hdrs, preserve_path : true)

core_lib = library('core', sources : core_srcs, include_directories : src_inc,
                   dependencies : [util_dep, fmt_dep, doctest_dep], install : true)
core_dep = declare_dependency(include_directories : src_inc, link_with : [core_lib])

core_test = executable('core-tests', sources : ['core/main.cc'] + core_srcs,
                                     dependencies : [util_dep, core_dep, fmt_dep, doctest_dep],
                                     install : true, cpp_args : ['-DENABLE_TESTS'])

# chess
chess_srcs = ['chess/bitboard.cc', 'chess/position.cc', 'chess/movegen.cc', 'chess/notation.cc', 'chess/pgn.cc']
chess_hdrs = ['chess/bitboard.hh', 'chess/position.hh', 'chess/movegen.hh', 'chess/notation.hh', 'chess/pgn.hh']

install_headers(chess_hdrs, preserve_path : true)

chess_lib = library('chess', sources : chess_srcs, include_directories : src_inc,
                    dependencies : [util_dep, core_dep, fmt_dep, doctest_dep], install : true)
chess_dep = declare_dependency(include_directories : src_inc, link_with : [chess_lib])

chess_test = executable('chess-tests', sources : ['chess/main.cc'] + chess_srcs,
                                       dependencies : [util_dep, core_dep, fmt_dep, doctest_dep],
                                       install : true, cpp_args : ['-DENABLE_TESTS'])


# db
db_srcs = ['db/database.cc']
db_hdrs = ['db/database.hh']

install_headers(db_hdrs, preserve_path : true)

db_lib = library('db', sources : db_srcs, include_directories : src_inc,
                 dependencies : [util_dep, core_dep, fmt_dep, doctest_dep], install : true)
db_dep = declare_dependency(include_directories : src_inc, link_with : [db_lib])


main_exe = executable('cdb', 'main.cc', install : true,
                      dependencies : [util_dep, core_dep, chess_dep, db_dep, fmt_dep, doctest_dep],
                      cpp_args : ['-DDOCTEST_CONFIG_DISABLE'])

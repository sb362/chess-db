project('chess-db', ['c', 'cpp'],
	version         : '0.1',
	license         : 'MIT',
	default_options : 
	[
		'cpp_std=c++17',
		'warning_level=2',
		'werror=true',
		'default_library=static',
#		'cpp_eh=none'
	]
)

# Check for 64-bit architecture
cc = meson.get_compiler('c')
if cc.sizeof('void *') != 8
	error('Only 64-bit build is supported')
endif

#############################################################################
## Configuration                                                           ##
#############################################################################

use_intrinsics = get_option('use_intrinsics')

build_tests = get_option('build_tests')

# Disable dumb warnings
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language : 'c')
	add_project_arguments('-Wno-gnu-binary-literal', language : 'c')
	add_project_arguments('-Wno-char-subscripts', language : 'c')
	add_project_arguments('-Wno-unused-parameter', language : 'c')

	add_project_arguments('-Wno-unused-parameter', language : 'cpp')
	add_project_arguments('-Wno-unused-private-field', language : 'cpp')
endif

# Intrinsics
if use_intrinsics
	add_project_arguments('-DUSE_INTRINSICS', language : 'c')
	add_project_arguments('-DUSE_INTRINSICS', language : 'cpp')

	if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
		add_project_arguments('-mpopcnt', language : 'c')
		add_project_arguments('-mbmi2', language : 'c')
		add_project_arguments('-mpopcnt', language : 'cpp')
		add_project_arguments('-mbmi2', language : 'cpp')
	endif
endif

#############################################################################
## Subprojects / dependencies                                              ##
#############################################################################

threads_dep = dependency('threads')

fmt_proj = subproject('fmt')
fmt_dep = fmt_proj.get_variable('fmt_dep')

if build_tests
	catch2_proj = subproject('Catch2')
	catch2_dep = catch2_proj.get_variable('catch2_dep')
endif

subdir('src')

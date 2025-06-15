project('taf', 'c',
  version: '0.0.1',
  default_options: [
    'c_std=gnu2x',
    'warning_level=3',
    'default_library=static',
  ],
)

project_root = meson.project_source_root()
macro_prefix_map_flag = '-fmacro-prefix-map=' + project_root + '=.'
add_project_arguments(macro_prefix_map_flag, language: 'c')

is_macos = host_machine.system() == 'darwin'
is_macos_arm64 = is_macos and host_machine.cpu_family() == 'aarch64'
is_macos_x86_64 = is_macos and host_machine.cpu_family() == 'x86-64'

opt_deps = []

thread_dep = dependency(
  'threads',
  required: true,
  static: true
)
json_dep = dependency(
  'json-c',
  method: 'pkg-config',
  required: true,
  static: true
)
libcurl_dep = dependency(
  'libcurl',
  method: 'pkg-config',
  required: true,
  static: false
)

#--- notcurses ---------------------------------------------------------------
notcurses_dep = dependency(
  'notcurses-core',
  method: 'pkg-config',
  required: true,
  static: true
)
libdeflate_dep = dependency(
  'libdeflate',
  method: 'pkg-config',
  required: true,
  static: true,
)
if is_macos_arm64
  libunistring_dep = declare_dependency(
    link_args : ['/opt/homebrew/lib/libunistring.a'],
    include_directories : include_directories('/opt/homebrew/opt/libunistring/include'),
  )
  libncurses_dep = declare_dependency(
    link_args : ['/opt/homebrew/opt/ncurses/lib/libncurses.a'],
    include_directories : include_directories('/opt/homebrew/opt/ncurses/include'),
  )
  opt_deps += [
    libunistring_dep,
    libncurses_dep,
  ]
endif
#----------------------------------------------------------------------

#--- Lua ---------------------------------------------------------------
# Lua’s pkg‑config file is annoyingly named differently on each distro.
lua_pc_names = [
  'lua5.4',  # Debian/Ubuntu, Arch
  'lua-5.4', # Fedora/RHEL
]

lua_dep = dependency(
  lua_pc_names,
  method   : 'pkg-config',
  required : true,
  static   : true
)
#----------------------------------------------------------------------

#--- libserialport ----------------------------------------------------
libsp_dep = dependency(
  'libserialport',
  method : 'pkg-config',
  required : true,
  static : true
)
# libserialport requires apple frameworks when used statically on macOS
if is_macos
  opt_deps += [
    dependency('CoreFoundation'),
    dependency('IOKit'),
  ]
endif
#----------------------------------------------------------------------

sources = [
  'src/main.c',

  'src/taf_init.c',
  'src/taf_logs.c',
  'src/taf_target.c',
  'src/taf_test.c',
  'src/taf_tui.c',

  'src/cmd_parser.c',
  'src/internal_logging.c',
  'src/project_parser.c',
  'src/test_case.c',
  'src/test_logs.c',

  'src/util/files.c',
  'src/util/lua.c',
  'src/util/os.c',
  'src/util/string.c',
  'src/util/time.c',

  'src/modules/http/taf-http.c',
  'src/modules/json/taf-json.c',
  'src/modules/proc/taf-proc.c',
  'src/modules/serial/taf-serial.c',
  'src/modules/taf/taf.c',
]

include_dir = include_directories('include')

executable(
  'taf',
  sources,
  dependencies: opt_deps + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    libdeflate_dep,
    notcurses_dep,
  ],
  include_directories: [
    include_dir,
  ],
  install: true,
)

#--- lua libraries ----------------------------------------------------
fs = import('fs')
taf_lib_path = fs.expanduser('~/.taf') / meson.project_version()

install_subdir(
  'lib',
  install_dir : taf_lib_path
)

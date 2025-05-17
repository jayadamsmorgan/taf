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

#--- notcurses ---------------------------------------------------------------
notcurses_dep = dependency(
  'notcurses',
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
# libserialport requires apple frameworks when used statically
# on x86-64 macOS
apple_fw_deps = []
if host_machine.system() == 'darwin'
  apple_fw_deps += [
    dependency('CoreFoundation'),
    dependency('IOKit'),
  ]
endif
#----------------------------------------------------------------------

sources = [
  'src/main.c',
  'src/test_case.c',

  'src/util/time.c',

  'src/modules/taf.c',
  'src/modules/taf-serial.c',
]

include_dir = include_directories('include')

executable(
  'taf',
  sources,
  dependencies: [
    thread_dep,
    json_dep,
    lua_dep,
    libsp_dep,
    notcurses_dep,
    libdeflate_dep,
  ] + apple_fw_deps,
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

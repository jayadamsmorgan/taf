project(
  'taf',
  'c',
  version: '2.0.0',
  default_options: [
    'buildtype=release',
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
is_linux = host_machine.system() == 'linux'

fs = import('fs')

ver_full = meson.project_version()
ver_core = ver_full.split('-')[0]
ver_parts = ver_core.split('.')
ver_major = ver_parts[0].to_int()
ver_minor = ver_parts[1].to_int()
ver_patch = ver_parts[2].to_int()

cfg = configuration_data()
cfg.set('TAF_VERSION_MAJOR', ver_major)
cfg.set('TAF_VERSION_MINOR', ver_minor)
cfg.set('TAF_VERSION_PATCH', ver_patch)
cfg.set_quoted('TAF_VERSION', ver_full)

version_h = configure_file(
  input: 'include/version.h.in',
  output: 'version.h',
  configuration: cfg,
)

git_header = custom_target('git-version-header',
  output : 'git_version.h',
  command : [files('scripts/gen_git_header.sh')],
  capture : true,
  build_always_stale : true,
)

opt_deps = []

thread_dep = dependency('threads', required: true, static: true)
json_dep = dependency(
  'json-c',
  method: 'pkg-config',
  required: true,
  static: true,
)
libcurl_dep = dependency(
  'libcurl',
  method: 'pkg-config',
  required: true,
  static: false,
)

picotui_dep = dependency(
 'unibilium',
  method: 'pkg-config',
  required: true,
  static: true,
)
#----------------------------------------------------------------------

#--- Lua ---------------------------------------------------------------
# Lua’s pkg‑config file is annoyingly named differently on each distro.
lua_pc_names = [
  'lua5.4', # Debian/Ubuntu, Arch
  'lua-5.4', # Fedora/RHEL
]

lua_dep = dependency(
  lua_pc_names,
  method: 'pkg-config',
  required: true,
  static: true,
)
#----------------------------------------------------------------------

#--- libserialport ----------------------------------------------------
libsp_dep = dependency(
  'libserialport',
  method: 'pkg-config',
  required: true,
  static: true,
)
# libserialport requires apple frameworks when used statically on macOS
if is_macos
  opt_deps += [dependency('CoreFoundation'), dependency('IOKit')]
endif
#----------------------------------------------------------------------

main_source = 'src/main.c'
sources = [
  version_h,
  git_header,
  'src/taf_hooks.c',
  'src/taf_init.c',
  'src/taf_logs.c',
  'src/picotui.c',
  'src/taf_target.c',
  'src/taf_test.c',
  'src/taf_tui.c',
  'src/taf_vars.c',
  'src/headless.c',
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
  'src/modules/hooks/taf-hooks.c',
  'src/modules/http/taf-http.c',
  'src/modules/json/taf-json.c',
  'src/modules/proc/taf-proc.c',
  'src/modules/serial/taf-serial.c',
  'src/modules/taf/taf.c',
]

include_dir = include_directories('include')

need_setgid = get_option('install_setgid')
if is_linux and need_setgid
  #   rwx r-s r-x   → set-gid + 2755
  #   this is needed to be able to open serial ports
  taf_install_mode = ['rwxr-sr-x', 'root', 'dialout']
else
  taf_install_mode = 'rwxr-xr-x'
endif

taf_dir_path = get_option('taf_dir_path')
add_project_arguments('-DTAF_DIR_PATH="@0@"'.format(taf_dir_path), language: 'c')

if taf_dir_path == ''
  # default: ~/.taf
  taf_dir_path = fs.expanduser('~/.taf')
endif

taf_lib = static_library(
  'taf_core',
  sources,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
  ],
  install: false,
)

taf_dep = declare_dependency(
  link_with: taf_lib,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
  ],
)

executable(
  'taf',
  main_source,
  link_with: taf_lib,
  include_directories: [include_dir],
  dependencies: opt_deps
  + [
    thread_dep,
    json_dep,
    libcurl_dep,
    lua_dep,
    libsp_dep,
    picotui_dep,
  ],
  install: true,
  install_mode: taf_install_mode,
)

install_subdir('lib', install_dir: taf_dir_path)

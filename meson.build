project('herness', 'c',
  version: '0.0.1',
  default_options: [
    'c_std=gnu2x',
    'warning_level=3',
  ],
)

project_root = meson.project_source_root()
macro_prefix_map_flag = '-fmacro-prefix-map=' + project_root + '=.'
add_project_arguments(macro_prefix_map_flag, language: 'c')

subdir('src')

thread_dep = dependency('threads')
json_dep = dependency('json-c')

#--- Lua ---------------------------------------------------------------
# Lua’s pkg‑config file is annoyingly named differently on each distro.
lua_pc_names = [
  'lua5.4',  # Debian/Ubuntu, Arch
  'lua-5.4', # Fedora/RHEL
  'lua5.3',
  'lua-5.3',
  'lua',     # generic; often 5.1
  'luajit'   # if you prefer LuaJIT
]

lua_dep = dependency(lua_pc_names,
  method   : 'pkg-config',
  required : true,          # change to false if you want a fallback
  static   : true
)

# You can inspect what you got while configuring:
message('Found Lua: ' + lua_dep.version())
#----------------------------------------------------------------------

executable(
  'herness',
  sources,
  dependencies: [
    thread_dep,
    json_dep,
    lua_dep,
  ],
  include_directories: [
  ],
  install: true,
)

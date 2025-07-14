project(
    'taf',
    'c',
    version: '1.1.1',
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
is_linux = host_machine.system() == 'linux'

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

#--- notcurses ---------------------------------------------------------------
notcurses_dep = dependency(
    'notcurses-core',
    method: 'pkg-config',
    required: true,
    static: true,
)
libdeflate_dep = dependency(
    'libdeflate',
    method: 'pkg-config',
    required: true,
    static: true,
)
if is_macos_arm64
    libunistring_dep = declare_dependency(
        link_args: [
            '-L/opt/homebrew/opt/libunistring/lib',
            '-lunistring',
        ],
        include_directories: include_directories(
            '/opt/homebrew/opt/libunistring/include',
        ),
    )
    libncurses_dep = declare_dependency(
        link_args: [
            '-L/opt/homebrew/opt/ncurses/lib',
            '-lncurses',
        ],
        include_directories: include_directories(
            '/opt/homebrew/opt/ncurses/include',
        ),
    )
    opt_deps += [libunistring_dep, libncurses_dep]
endif
if is_linux
    libunistring_dep = declare_dependency(
        link_args: [
            '-L/home/linuxbrew/.linuxbrew/opt/libunistring/lib',
            '-lunistring',
        ],
        include_directories: include_directories(
            '/home/linuxbrew/.linuxbrew/opt/libunistring/include',
        ),
    )
    opt_deps += [libunistring_dep]
endif
#----------------------------------------------------------------------

#--- Lua ---------------------------------------------------------------
# Lua’s pkg‑config file is annoyingly named differently on each distro.
lua_pc_names = [
    'lua5.4',  # Debian/Ubuntu, Arch
    'lua-5.4',  # Fedora/RHEL
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
    'src/taf_init.c',
    'src/taf_logs.c',
    'src/taf_target.c',
    'src/taf_test.c',
    'src/taf_tui.c',
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
    fs = import('fs')
    taf_dir_path = fs.expanduser('~/.taf')
endif

taf_lib = static_library(
    'taf_core',
    sources,
    include_directories: [include_dir],
    dependencies: opt_deps + [
        thread_dep,
        json_dep,
        libcurl_dep,
        lua_dep,
        libsp_dep,
        libdeflate_dep,
        notcurses_dep,
    ],
    install: false,
)

taf_dep = declare_dependency(
    link_with: taf_lib,
    include_directories: [include_dir],
    dependencies: opt_deps + [
        thread_dep,
        json_dep,
        libcurl_dep,
        lua_dep,
        libsp_dep,
        libdeflate_dep,
        notcurses_dep,
    ],
)

executable(
    'taf',
    main_source,
    link_with: taf_lib,
    include_directories: [include_dir],
    dependencies: opt_deps + [
        thread_dep,
        json_dep,
        libcurl_dep,
        lua_dep,
        libsp_dep,
        libdeflate_dep,
        notcurses_dep,
    ],
    install: true,
    install_mode: taf_install_mode,
)

install_subdir('lib', install_dir: taf_dir_path)

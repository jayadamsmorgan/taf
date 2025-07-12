# Building from source

## Requirements

- Build tools
    - meson
- Libraries
    - json-c
    - libcurl
    - notcurses
    - lua5.4
    - libserialport
    - libssh2

### Linux

- Fedora/RHEL/Alma:

```
sudo dnf groupinstall "Development Tools"
sudo dnf install meson pkgconf-pkg-config \
                 json-c-devel libcurl-devel \
                 notcurses-devel notcurses-static \
                 lua-devel libserialport-devel libssh2-devel
```

- Debian/Ubuntu:

```
sudo apt update
sudo apt install build-essential meson pkg-config \
                 libjson-c-dev libcurl4-openssl-dev \
                 libnotcurses-dev libnotcurses-core-dev \
                 liblua5.4-dev libserialport-dev libssh2-1-dev
```

- Arch/Manjaro

```
sudo pacman -Sy --needed base-devel meson \
     json-c curl notcurses lua libserialport libssh2
```

### Homebrew (macOS / Linux)

```
brew install meson json-c curl notcurses libunistring lua libserialport libssh2
```

### Windows

#### vcpkg:

```
git clone https://github.com/microsoft/vcpkg; .\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe install json-c:notcurses lua libssh2 curl --triplet x64-windows-static
```
libserialport and notcurses have no official port yet -> build from source.

Then `.\vcpkg\vcpkg.exe integrate install`  (optional)

#### MSYS2 MinGW:

```
pacman -Syu
pacman -S --needed \
  mingw-w64-x86_64-toolchain meson pkgconf \
  mingw-w64-x86_64-json-c mingw-w64-x86_64-curl \
  mingw-w64-x86_64-lua mingw-w64-x86_64-libssh2 \
  mingw-w64-x86_64-libserialport
```

notcurses still not present -> build from source.

## Building & Installing

TAF has to install it's Lua libraries to `~/.taf` folder, that's why `meson install` is required:

```
meson setup build
meson install -C build
```

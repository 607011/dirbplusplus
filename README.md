# Dirb++

**Fast, multithreaded version of the original Dirb**

## Prerequisites

- Git
- OpenSSL libraries ≥ 1.1.1t

### Windows

Get `xxd.exe` from https://sourceforge.net/projects/xxd-for-windows/ and copy it to a location thats in the system's `Path`

Install OpenSSL:

```
winget install OpenSSL
winget install Ninja-build.Ninja
```

If you don't want to use the Ninja build tool, you can omit its installation, but must then replace `Ninja` with `"NMake Makefiles"` in the `cmake` command below.

## Build

### macOS

```bash
git clone https://github.com/607011/dirbplusplus.git dirb++
mkdir -p dirb++/build
cd dirb++/build
git submodule init
git submodule update
cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/opt/homebrew/Cellar/openssl@3/3.1.0 ..
cmake --build .
```

### Linux

```bash
git clone https://github.com/607011/dirbplusplus.git dirb++
mkdir -p dirb++/build
cd dirb++/build
git submodule init
git submodule update
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Windows 11

In Visual Studio Developer Command Prompt:

```bash
git clone https://github.com/607011/dirbplusplus.git dirb++
mkdir -p dirb++/build
cd dirb++/build
git submodule init
git submodule update
cmake -G Ninja -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64" ..
cmake --build . --config Release
```

## License

See [LICENSE](LICENSE).

## Copyright

Copyright (c) 2023 Oliver Lau

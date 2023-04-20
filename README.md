# Dirb++

**Fast, multithreaded version of the original Dirb**

## Prerequisites

- Git
- OpenSSL libraries ≥ 1.1.1t

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


## License

See [LICENSE](LICENSE).

## Copyright

Copyright (c) 2023 Oliver Lau

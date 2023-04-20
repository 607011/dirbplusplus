# Dirb++

**Fast, multithreaded version of the original Dirb**

## Build


### macOS

```bash
git clone https://github.com/607011/dirbplusplus.git dirb++
mkdir -p dirb++/build
cd dirb++/build
cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/opt/homebrew/Cellar/openssl@3/3.1.0 ..
cmake --build .
```



## License

See [LICENSE](LICENSE).

## Copyright

Copyright (c) 2023 Oliver Lau

[![Travis](https://img.shields.io/travis/yandex/tomita-parser/master.svg?style=plastic)](https://travis-ci.org/yandex/tomita-parser)

How to build?

1. Install cmake 2.8, lua 5.2, gcc 4.8.1
2. Create directory "build/" on the same level as "src/"
3. cd build
4. cmake ../src/ -DCMAKE_BUILD_TYPE=Release
5. make
6. copy libmystem-c-binding.so from https://github.com/yandex/tomita-parser/releases/tag/v1.0 to the same folder

# Compiling on Linux

## Ubuntu

1. sudo apt-get install build-essential cmake lua5.2
2. git clone https://github.com/yandex/tomita-parser
3. cd tomita-parser && mkdir build && cd build
4. cmake ../src/ -DCMAKE_BUILD_TYPE=Release
5. make
6. copy libmystem-c-binding.so from https://github.com/yandex/tomita-parser/releases/tag/v1.0 to the same folder

## Fedora (RHEL, CentOS, etc.)

The following packages should be installed before compiling on an `x86_64` machine:

* `gcc`,
* `cmake`,
* `lua`,
* `libstdc++-static`.

Please note that the `glibc-devel.i686` package should be either removed or not installed (see yandex/tomita-parser#6 for details). In case when the requirements are met, everything will work fine. it has been tested and so far has worked successfully on Fedora 20 (x86_64) and Fedora 24 (x86_64).

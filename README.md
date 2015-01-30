How to build?

1. Install cmake, lua, gcc 4.8.1
2. Create directory "build/" on the same level as "src/"
3. cd build
4. cmake ../src/ -DMAKE_ONLY=FactExtract/Parser/tomita-parser -DCMAKE_BUILD_TYPE=Release
5. make
6. copy libmystem-c-binding.so from https://github.com/yandex/tomita-parser/releases/tag/v1.0 to the same folder

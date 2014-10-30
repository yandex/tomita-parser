How to build?

1. Install cmake, lua, gcc 4.8.1
2. Create directory build on the same level as src/
3. cd build
4. cmake ../arcadia/ -DMAKE_ONLY=FactExtract/Parser/tomita-parser -DCMAKE_BUILD_TYPE=Release
5. make

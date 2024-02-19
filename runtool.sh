cd tools/AtlasGenerator/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
ninja
./AtlasGenerator $1 $2
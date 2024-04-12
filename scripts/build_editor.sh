 bash scripts/shaderbuild.sh
cd tools/MaterialEditor
 mkdir -p build && cd build
 cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
 ninja
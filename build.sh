mkdir -p ./build/deps

mkdir -p corrade-build-native
cd corrade-build-native
cmake ../src/extern/corrade -DCMAKE_INSTALL_PREFIX=../build/deps -DBUILD_TESTS=OFF
make -j
make install
cd ..

mkdir -p magnum-build-native
cd magnum-build-native
cmake ../src/extern/magnum -DCMAKE_INSTALL_PREFIX=../build/deps -DWITH_SDL2APPLICATION=ON -DWITH_TGAIMPORTER=ON -DWITH_ANYIMAGEIMPORTER=ON -DWITH_TRADE=ON -DWITH_GL=TRUE
make -j
make install
cd ..

mkdir -p build/Release
cd ./build/Release
cmake ../../src -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -mtune=native" -DCMAKE_PREFIX_PATH="$(pwd)/../deps" -DMagnum_DIR="../deps/share/cmake/Magnum/"
make -j

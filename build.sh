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

mkdir -p imgui-build-native
cd imgui-build-native
cmake ../src/extern/imgui-build-system -DCMAKE_INSTALL_PREFIX=../build/deps
make -j
make install
cd ..

cp src/extern/imgui-build-system/CMakeLists_Magnum.txt src/extern/magnum-integration/src/Magnum/ImGuiIntegration/CMakeLists.txt
mkdir -p magnum-integration-build-native
cd magnum-integration-build-native
cmake ../src/extern/magnum-integration -DCMAKE_INSTALL_PREFIX=../build/deps -DWITH_IMGUI=TRUE -DImGui_INCLUDE_DIR=../build/deps/include -DIMGUI_LIB_DIR=../build/deps/lib -DBUILD_STATIC=False
make -j 
make install
cd ..

mkdir -p build/Release
cd ./build/Release
cmake ../../src -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -mtune=native -Wno-old-style-cast -Wno-reorder -Wno-unused-variable -Wno-double-promotion" -DCMAKE_PREFIX_PATH="$(pwd)/../deps" -DMagnum_DIR="../deps/share/cmake/Magnum/" -DIMGUI_LIB_DIR=../build/deps/lib 
make -j
cd ../..

mkdir -p build/Debug
cd ./build/Debug
cmake ../../src -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O0 -g3 -Wno-old-style-cast -Wno-reorder -Wno-unused-variable -Wno-double-promotion" -DCMAKE_PREFIX_PATH="$(pwd)/../deps" -DMagnum_DIR="../deps/share/cmake/Magnum/" -DIMGUI_LIB_DIR=../build/deps/lib 
make -j

cd ../..
mkdir -p build/Editor
cd ./build/Editor
cmake ../../src -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O0 -mtune=native -Wno-old-style-cast -Wno-reorder -Wno-unused-variable -Wno-double-promotion -DEDITOR" -DCMAKE_PREFIX_PATH="$(pwd)/../deps" -DMagnum_DIR="../deps/share/cmake/Magnum/" -DIMGUI_LIB_DIR=../build/deps/lib 
make -j
cd ../..

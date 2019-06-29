export API_VERSION=29
export TOOLS_VERSION=debian
export ANDROID_NDK="$(pwd)/build/deps_android/android-ndk-r20"
export ARM_ABI=arm64-v8a
export ARM_ARCH=arm64
mkdir -p build/deps_android
HOST_OS=linux	
cd build/deps_android
wget https://dl.google.com/android/repository/android-ndk-r20-$HOST_OS-x86_64.zip
unzip android-ndk-r20-$HOST_OS-x86_64.zip
#patch -p1 < ../../src/extern/ndk_cmath.patch
rm *.zip
patch -p2 < ../../src/extern/ndk_cmath.patch
patch -p2 < ../../src/extern/ndk_math_h.patch
cd ../..
cat src/extern/android.toolchain.cmake.in | sed s/NDK_PATH/\\$\\{CMAKE_CURRENT_LIST_DIR\\}\\/..\\/..\\/build\\/deps_android\\/android-ndk-r20/ > src/extern/android.toolchain.cmake
mkdir -p corrade-build-android
cd corrade-build-android
CORRADE_SRC="$(pwd)/../src/extern/corrade" ../src/extern/corrade_cmake_android.sh
make -j
make install
cd ..
mkdir -p magnum-build-android
cd magnum-build-android
MAGNUM_SRC="$(pwd)/../src/extern/magnum" ../src/extern/magnum_cmake_android.sh
make -j
make install
cd ..
#cat src/extern/android.toolchain.cmake.in | sed s/NDK_PATH/\\$\\{CMAKE_CURRENT_LIST_DIR\\}\\/..\\/..\\/build\\/deps_android\\/android-ndk-r20/ > src/extern/android.toolchain.cmake
mkdir -p build/android
cd build/android
cmake ../../src/ -DCMAKE_SYSTEM_NAME=Android     -DCMAKE_SYSTEM_VERSION=$API_VERSION     -DCMAKE_ANDROID_ARCH_ABI=$ARM_ABI     -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang     -DCMAKE_BUILD_TYPE=Release        -DMAGNUM_INCLUDE_INSTALL_PREFIX=$ANDROID_NDK/sysroot/usr -DCMAKE_ANDROID_NDK=$ANDROID_NDK -DWITH_ANDROIDAPPLICATION=ON -DTARGET_GLES2=OFF -DCMAKE_ANDROID_SDK=$ANDROID_SDK -DCMAKE_CXX_STANDARD=14 -DCMAKE_CXX14_EXTENSION_COMPILE_OPTION="-std=c++14" -DCMAKE_ANDROID_STL_TYPE=c++_shared -DTARGET_GLES=ON -DCMAKE_TOOLCHAIN_FILE=$(pwd)/../../src/extern/android.toolchain.cmake -DANDROID_PLATFORM=android-$API_VERSION -DCMAKE_INSTALL_PREFIX=$ANDROID_NDK/platforms/android-$API_VERSION/arch-$ARM_ARCH/usr -DBUILD_STATIC=OFF -DCMAKE_FIND_ROOT_PATH=$ANDROID_NDK/sysroot/usr  -DCMAKE_INSTALL_RPATH=$ANDROID_NDK/platforms/android-$API_VERSION/arch-$ARM_ARCH/usr -D_CORRADE_MODULE_DIR=$ANDROID_NDK/sysroot/usr/share/cmake/Corrade -DCORRADE_TESTSUITE_ADB_RUNNER=$ANDROID_NDK/sysroot/usr/share/corrade/AdbRunner.sh -DARM_ABI=$ARM_ABI -DANDROID_ARM_NEON=ON -DWITH_TGAIMPORTER=ON -DWITH_ANYIMAGEIMPORTER=ON -DCORRADE_TARGET_ANDROID=ON -DANDROID_BUILD_TOOLS_VERSION=$TOOLS_VERSION -G Ninja -DCMAKE_PREFIX_PATH=$(pwd)/../deps -DANDROID_ABI=$ARM_ABI -DCMAKE_CXX_FLAGS="-Wno-c++11-narrowing"
ninja

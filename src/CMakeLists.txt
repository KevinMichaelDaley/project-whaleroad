
#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
#           Vladimír Vondruš <mosra@centrum.cz>
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
#   this software, either in source code form or as a compiled binary, for any
#   purpose, commercial or non-commercial, and by any means.
#
#   In jurisdictions that recognize copyright laws, the author or authors of
#   this software dedicate any and all copyright interest in the software to
#   the public domain. We make this dedication for the benefit of the public
#   at large and to the detriment of our heirs and successors. We intend this
#   dedication to be an overt act of relinquishment in perpetuity of all
#   present and future rights to this software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.1)

project(islands)

# Add module path in case this is project root
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/" "${PROJECT_SOURCE_DIR}/toolchains/modules/")
if(CORRADE_TARGET_ANDROID AND CMAKE_ANDROID_NDK)
    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/toolchains/modules/")
    include(UseAndroid)
endif()
if(NOT CORRADE_TARGET_ANDROID)
find_package(SDL2 REQUIRED)
find_package(JeMalloc REQUIRED)
find_package(Magnum REQUIRED GL Trade Sdl2Application TgaImporter)
else()
	find_package(Magnum REQUIRED GL Trade AndroidApplication TgaImporter)
endif()
find_package(Corrade REQUIRED Utility)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)
if(NOT CORRADE_TARGET_ANDROID)
include_directories(${PROJECT_SOURCE_DIR}/engine ${JEMALLOC_INCLUDE_DIR})
else()
include_directories(${PROJECT_SOURCE_DIR}/engine)
endif()
FILE(GLOB SOURCE_FILES engine/*/*.cpp main/main.cpp)
corrade_add_resource(Resources resources.conf)
if(NOT CORRADE_TARGET_ANDROID)
	add_executable(islands ${SOURCE_FILES} ${Resources})
	target_link_libraries(islands
			    Magnum::Application
			    Magnum::GL  
			    Magnum::Magnum
			    Magnum::Trade pthread Corrade::Utility ${JEMALLOC_LIBRARIES})
else()
	add_library(islands SHARED ${SOURCE_FILES} ${Resources})
	target_link_libraries(islands
			    Magnum::Application
			    Magnum::GL  
			    Magnum::Magnum
			    Magnum::Trade Corrade::Utility Magnum::TgaImporter)
        android_create_apk(islands main/AndroidManifest.xml)
endif()

# cmake
cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0076 NEW)

# vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
endif()

# project
project(pong C CXX)
add_executable(${PROJECT_NAME} pong.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-s STANDALONE_WASM --no-entry -s ERROR_ON_UNDEFINED_SYMBOLS=0 -O3")
endif()

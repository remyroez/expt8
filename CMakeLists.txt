# cmake
cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0076 NEW)
#set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
endif()

# General options
#set(BUILD_SHARED_LIBS OFF)
#set(BUILD_TESTS OFF)
#set(BUILD_TOOLS OFF)

# project
project(expt8 C CXX)
add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Enable LTO in release builds
#if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.11)
#cmake_minimum_required(VERSION 3.12)
#set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
#if (CMAKE_BUILD_TYPE STREQUAL "Release")
#    include(CheckIPOSupported)
#    check_ipo_supported(RESULT result OUTPUT output)
#    if (result)
#        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#    else()
#       message(WARNING "IPO is not supported: ${output}")
#    endif ()
#endif ()
#endif ()

# dependencies
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)

# wasm3
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/thirdparty/wasm3/source ${CMAKE_BINARY_DIR}/m3)
target_link_libraries(${PROJECT_NAME} PRIVATE m3)

add_custom_target(copy_wasm ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_LIST_DIR}/thirdparty/wasm3/platforms/cpp/wasm
    ${CMAKE_BINARY_DIR}/wasm
)
add_dependencies(${PROJECT_NAME} copy_wasm)

# msvc
if (MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
  add_definitions(/bigobj)
endif()

# src
add_subdirectory(src)
#add_subdirectory(thirdparty)

get_property("TARGET_SOURCE_FILES" TARGET ${PROJECT_NAME} PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

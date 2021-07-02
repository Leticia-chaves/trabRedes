cmake_minimum_required(VERSION 3.10)
project(Redes)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories("${CMAKE_SOURCE_DIR}/includes")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# add_subdirectory(lib1_dir)

add_executable(server server.cpp)
conan_target_link_libraries(server)

add_executable(client client.cpp)
conan_target_link_libraries(client)
# target_link_libraries(executable lib1 lib2)


cmake_minimum_required(VERSION 3.16)
project(Redes)

add_definitions(" -D_WIN32_WINNT=0x0601")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
include_directories("D:/Downloads/boost_1_76_0/boost_1_76_0")
conan_basic_setup()

include_directories("${CMAKE_SOURCE_DIR}/includes")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# add_subdirectory(lib1_dir)

add_executable(server main.cpp)
conan_target_link_libraries(server)
# target_link_libraries(executable lib1 lib2)


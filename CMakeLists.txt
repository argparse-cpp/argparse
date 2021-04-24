cmake_minimum_required(VERSION 3.19)
project(argparse)
include(CTest)

set(CMAKE_CXX_STANDARD 20)

add_executable(test_long tests/test_long.cpp include/argparse.hpp)
target_include_directories(test_long PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME test_long COMMAND test_long --output test_successful)

include(GNUInstallDirs)
install(FILES include/argparse.hpp
        DESTINATION include)
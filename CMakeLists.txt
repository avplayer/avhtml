cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)

project(html5)

find_package(Boost 1.55 REQUIRED COMPONENTS coroutine context timer thread chrono date_time filesystem system program_options regex locale)

link_directories(${Boost_LIBRARY_DIR})

include_directories(${Boost_INCLUDE_DIRS})

include_directories(include)

add_library(html5 STATIC src/html5.cpp)

target_link_libraries(html5 ${Boost_LIBRARIES})

add_executable(html5test test/test.cpp)

target_link_libraries(html5test html5)

enable_testing()

add_test(NAME html5test COMMAND ${CMAKE_BINARY_DIR}/html5test test.html title WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)

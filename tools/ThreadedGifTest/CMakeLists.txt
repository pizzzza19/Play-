cmake_minimum_required(VERSION 3.18)
project(ThreadedGifTest)

add_executable(ThreadedGifTest Main.cpp)
target_include_directories(ThreadedGifTest PRIVATE ../../Source)
target_link_libraries(ThreadedGifTest PRIVATE PlayCore)
enable_testing()
add_test(NAME ThreadedGifTest COMMAND ThreadedGifTest)

add_executable(Win64JitAbiTest Main.cpp ../../Source/JitFunction.cpp)
target_include_directories(Win64JitAbiTest PRIVATE ../../Source)
target_link_libraries(Win64JitAbiTest PRIVATE CodeGen Framework)
add_test(NAME Win64JitAbiTest COMMAND Win64JitAbiTest)

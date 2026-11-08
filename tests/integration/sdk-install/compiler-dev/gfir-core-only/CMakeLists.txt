cmake_minimum_required(VERSION 3.25)
project(PycircuitGfirCoreOnlyConsumer LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS CompilerDev)
add_executable(gfir_core_only main.cpp)
target_compile_features(gfir_core_only PRIVATE cxx_std_20)
target_link_libraries(gfir_core_only PRIVATE pycircuit::GFIRDialect)

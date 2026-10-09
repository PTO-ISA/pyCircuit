cmake_minimum_required(VERSION 3.25)
project(PycircuitCompilerDevConsumer LANGUAGES CXX)

find_package(pycircuit CONFIG REQUIRED COMPONENTS CompilerDev)
add_executable(compiler_dev_consumer main.cpp)
target_compile_features(compiler_dev_consumer PRIVATE cxx_std_20)
target_link_libraries(compiler_dev_consumer PRIVATE
  pycircuit::ACIRDialect
  pycircuit::ACIRSourceCompiler
)

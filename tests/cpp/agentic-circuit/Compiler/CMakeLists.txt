find_package(GTest CONFIG REQUIRED)

add_executable(CompilerTests DriverTest.cpp)
target_link_libraries(CompilerTests PRIVATE
  ACIRCompiler
  GTest::gtest_main
)
add_test(NAME CompilerTests COMMAND CompilerTests)

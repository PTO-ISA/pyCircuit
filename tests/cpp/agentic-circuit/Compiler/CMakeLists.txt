find_package(GTest CONFIG REQUIRED)

set(COMPILER_TEST_SOURCES DriverTest.cpp)
if(TARGET pyc_support)
  list(APPEND COMPILER_TEST_SOURCES PYCDiagnosticsTest.cpp)
endif()
add_executable(CompilerTests ${COMPILER_TEST_SOURCES})
target_link_libraries(CompilerTests PRIVATE
  ACIRCompiler
  GTest::gtest_main
)
if(TARGET pyc_support)
  target_link_libraries(CompilerTests PRIVATE pyc_support)
endif()
add_test(NAME CompilerTests COMMAND CompilerTests)

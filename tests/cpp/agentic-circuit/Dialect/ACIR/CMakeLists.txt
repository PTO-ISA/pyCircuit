find_package(GTest CONFIG REQUIRED)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

add_executable(ACIRSourceContractsTests
  SourceContractsTest.cpp
  SourceIdentityContractsTest.cpp
  SourceStaticMatchingTest.cpp
  SourceTypeContractsTest.cpp
)
target_include_directories(ACIRSourceContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRSourceContractsTests PRIVATE
  ACIRDialect
  MLIRAsmParser
  MLIRIR
  GTest::gtest_main
)
add_test(NAME ACIRSourceContractsTests COMMAND ACIRSourceContractsTests)

add_executable(ACIRSourceUnitTests
  SourceUnitHeaderTest.cpp
  SourceUnitReviewTest.cpp
)
target_include_directories(ACIRSourceUnitTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRSourceUnitTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRSourceUnitTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRSourceUnitTests acir-source-unit-harness)
add_test(NAME ACIRSourceUnitTests COMMAND ACIRSourceUnitTests)

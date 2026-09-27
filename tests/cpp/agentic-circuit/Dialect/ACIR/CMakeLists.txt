find_package(GTest CONFIG REQUIRED)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
set(PYC_UNICODE_GENERATOR_PYTHON "$ENV{PYC_UNICODE_GENERATOR_PYTHON}"
  CACHE FILEPATH "Exact CPython 3.14.6/UCD 16 interpreter for Unicode tooling tests"
)

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

add_executable(ACIRNamespaceContractsTests
  NamespaceContractsTest.cpp
  NamespaceRegistryGraphTest.cpp
  SourceIdentifierTest.cpp
)
target_include_directories(ACIRNamespaceContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRNamespaceContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRNamespaceContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRNamespaceContractsTests acir-source-unit-harness)
add_test(NAME ACIRNamespaceContractsTests COMMAND ACIRNamespaceContractsTests)

add_executable(ACIRSourceModuleContractsTests
  SourceBodySnapshotsTest.cpp
  SourceModuleContractsTest.cpp
)
target_include_directories(ACIRSourceModuleContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRSourceModuleContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRSourceModuleContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRSourceModuleContractsTests acir-source-unit-harness)
add_test(NAME ACIRSourceModuleContractsTests
  COMMAND ACIRSourceModuleContractsTests)

if(PYC_UNICODE_GENERATOR_PYTHON)
  execute_process(
    COMMAND "${PYC_UNICODE_GENERATOR_PYTHON}" -c
      "import sys, unicodedata; print(f'{sys.implementation.name}|{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}|{unicodedata.unidata_version}')"
    RESULT_VARIABLE PYC_UNICODE_RECIPE_STATUS
    OUTPUT_VARIABLE PYC_UNICODE_RECIPE_IDENTITY
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT PYC_UNICODE_RECIPE_STATUS EQUAL 0 OR
     NOT PYC_UNICODE_RECIPE_IDENTITY STREQUAL "cpython|3.14.6|16.0.0")
    message(FATAL_ERROR
      "PYC_UNICODE_GENERATOR_PYTHON must be CPython 3.14.6 with UCD 16.0.0; "
      "found '${PYC_UNICODE_RECIPE_IDENTITY}'"
    )
  endif()

  add_executable(ACIRSourceIdentifierOracleTests
    SourceIdentifierOracleTest.cpp
  )
  target_include_directories(ACIRSourceIdentifierOracleTests PRIVATE
    "${ACIR_SOURCE_DIR}/lib"
  )
  target_compile_definitions(ACIRSourceIdentifierOracleTests PRIVATE
    ACIR_UNICODE_RECIPE_PYTHON="${PYC_UNICODE_GENERATOR_PYTHON}"
  )
  target_link_libraries(ACIRSourceIdentifierOracleTests PRIVATE
    ACIRDialect
    ACIRSourceCompiler
    MLIRSupport
    GTest::gtest_main
  )
  add_test(NAME ACIRSourceIdentifierOracleTests
    COMMAND ACIRSourceIdentifierOracleTests)
endif()

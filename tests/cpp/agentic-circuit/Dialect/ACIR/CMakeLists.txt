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

add_executable(ACIRSourceMathContractsTests
  SourceMathContractsTest.cpp
  NumericProofContractsTest.cpp
  NumericExactProofContractsTest.cpp
  NumericExactAddProofContractsTest.cpp
  NumericExactScalarProofContractsTest.cpp
  NumericMaskProofContractsTest.cpp
  NumericCompositionAdversarialTest.cpp
  NumericCompositionContractsTest.cpp
  NumericNextUseContractsTest.cpp
  NumericRangeProofContractsTest.cpp
  NumericVariableAddProofContractsTest.cpp
  ScalarNumericExactLoweringTest.cpp
  ScalarNumericRangeLoweringTest.cpp
  ScalarNumericLoweringTest.cpp
  ScalarNumericNextUseLoweringTest.cpp
)
target_include_directories(ACIRSourceMathContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRSourceMathContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRSourceMathContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRAsmParser
  MLIRArithDialect
  MLIRIR
  MLIRParser
  MLIRPass
  MLIRSCFDialect
  GTest::gtest_main
)
add_test(NAME ACIRSourceMathContractsTests
  COMMAND ACIRSourceMathContractsTests)
add_dependencies(ACIRSourceMathContractsTests acir-source-unit-harness)

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

add_executable(ACIRSourceLinkAdmissionTests
  SourceLinkAdmissionTest.cpp
)
target_include_directories(ACIRSourceLinkAdmissionTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRSourceLinkAdmissionTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRSourceLinkAdmissionTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRSourceLinkAdmissionTests acir-source-unit-harness)
add_test(NAME ACIRSourceLinkAdmissionTests
  COMMAND ACIRSourceLinkAdmissionTests)

add_executable(ACIRRegContractsTests
  RegContractsTest.cpp
)
target_compile_definitions(ACIRRegContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRRegContractsTests PRIVATE
  ACIRDialect
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRRegContractsTests acir-source-unit-harness)
add_test(NAME ACIRRegContractsTests COMMAND ACIRRegContractsTests)

add_executable(ACIRSourceFactsTests
  SourceFactsTest.cpp
)
target_include_directories(ACIRSourceFactsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRSourceFactsTests PRIVATE
  ACIRDialect
  MLIRArithDialect
  MLIRIR
  GTest::gtest_main
)
add_test(NAME ACIRSourceFactsTests COMMAND ACIRSourceFactsTests)

add_executable(ACIRRuleEffectsTests
  RuleEffectsTest.cpp
)
target_include_directories(ACIRRuleEffectsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRRuleEffectsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRRuleEffectsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRRuleEffectsTests acir-source-unit-harness)
add_test(NAME ACIRRuleEffectsTests COMMAND ACIRRuleEffectsTests)

add_executable(ACIRModuleGraphTests
  ModuleGraphTest.cpp
)
target_include_directories(ACIRModuleGraphTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRModuleGraphTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRModuleGraphTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRModuleGraphTests acir-source-unit-harness)
add_test(NAME ACIRModuleGraphTests COMMAND ACIRModuleGraphTests)

add_executable(ACIRFinalProgramTests
  FinalGenericUsesTest.cpp
  FinalRecordDeclarationsTest.cpp
  FinalScalarDeclarationsTest.cpp
  FinalHardwareClosureTest.cpp
  FinalRealizationClosureTest.cpp
  FinalRegControlClosureTest.cpp
  FinalProgramTest.cpp
  FinalSystemPlanTest.cpp
)
target_include_directories(ACIRFinalProgramTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRFinalProgramTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRFinalProgramTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRFinalProgramTests acir-source-unit-harness)
add_test(NAME ACIRFinalProgramTests COMMAND ACIRFinalProgramTests)

add_executable(ACIRProposalContractsTests
  ProposalContractsTest.cpp
)
target_include_directories(ACIRProposalContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRProposalContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRProposalContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRProposalContractsTests acir-source-unit-harness)
add_test(NAME ACIRProposalContractsTests COMMAND ACIRProposalContractsTests)

add_executable(ACIRRegRuntimeTests
  "${CMAKE_CURRENT_LIST_DIR}/../../Runtime/RegRuntimeTest.cpp"
)
target_include_directories(ACIRRegRuntimeTests PRIVATE
  "${ACIR_REPO_ROOT}/simulator/gfsim/include"
)
target_link_libraries(ACIRRegRuntimeTests PRIVATE
  GTest::gtest_main
)
add_test(NAME ACIRRegRuntimeTests COMMAND ACIRRegRuntimeTests)

add_executable(ACIRSystemLifecycleTests
  "${CMAKE_CURRENT_LIST_DIR}/../../Runtime/SystemLifecycleTest.cpp"
)
target_include_directories(ACIRSystemLifecycleTests PRIVATE
  "${ACIR_REPO_ROOT}/simulator/gfsim/include"
)
target_link_libraries(ACIRSystemLifecycleTests PRIVATE
  GTest::gtest_main
)
add_test(NAME ACIRSystemLifecycleTests COMMAND ACIRSystemLifecycleTests)

add_executable(ACIRSimExecutorTests
  "${CMAKE_CURRENT_LIST_DIR}/../../Runtime/SimExecutorTest.cpp"
  "${ACIR_REPO_ROOT}/simulator/gfsim/model_input.cpp"
  "${ACIR_REPO_ROOT}/simulator/gfsim/sim_executor.cpp"
)
target_include_directories(ACIRSimExecutorTests PRIVATE
  "${ACIR_REPO_ROOT}/simulator/gfsim/include"
)
target_link_libraries(ACIRSimExecutorTests PRIVATE
  GTest::gtest_main
)
add_test(NAME ACIRSimExecutorTests COMMAND ACIRSimExecutorTests)

add_executable(ACIRObservationTests
  "${CMAKE_CURRENT_LIST_DIR}/../../Runtime/ObservationTest.cpp"
)
target_include_directories(ACIRObservationTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
  "${ACIR_REPO_ROOT}/simulator/gfsim/include"
)
target_link_libraries(ACIRObservationTests PRIVATE
  GTest::gtest_main
)
add_test(NAME ACIRObservationTests COMMAND ACIRObservationTests)

add_executable(ACIRObservationContractsTests
  ObservationContractsTest.cpp
)
target_include_directories(ACIRObservationContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRObservationContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRObservationContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_test(NAME ACIRObservationContractsTests
  COMMAND ACIRObservationContractsTests)
add_dependencies(ACIRObservationContractsTests acir-source-unit-harness)

add_executable(ACIRCheckContractsTests
  CheckContractsTest.cpp
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${ACIR_SOURCE_DIR}/lib/Compiler/CheckGraph.h"
)
target_include_directories(ACIRCheckContractsTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_compile_definitions(ACIRCheckContractsTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
)
target_link_libraries(ACIRCheckContractsTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRCheckContractsTests acir-source-unit-harness)
add_test(NAME ACIRCheckContractsTests COMMAND ACIRCheckContractsTests)

add_executable(ACIRBackendClosureTests
  BackendClosureTest.cpp
)
target_compile_definitions(ACIRBackendClosureTests PRIVATE
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
)
target_include_directories(ACIRBackendClosureTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRBackendClosureTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRBackendClosureTests acir-backend-closure-harness)
add_test(NAME ACIRBackendClosureTests COMMAND ACIRBackendClosureTests)
set_tests_properties(ACIRBackendClosureTests PROPERTIES
  ENVIRONMENT
    "ACIR_BACKEND_CLOSURE_HARNESS=$<TARGET_FILE:acir-backend-closure-harness>"
)

find_program(ACIR_W10_TEST_CXX NAMES clang++ c++ REQUIRED)
find_program(ACIR_W10_TEST_IVERILOG NAMES iverilog REQUIRED)
find_program(ACIR_W10_TEST_VVP NAMES vvp REQUIRED)
add_executable(ACIRExecutableBackendClosureTests
  ExecutableBackendClosureTest.cpp
  NumericNextBackendTest.cpp
  MultiInstanceEmitTest.cpp
  MultiInstanceVerilogTest.cpp
  ReusableFamilyVerilogTest.cpp
  ReusableFamilyCppTest.cpp
  HierarchyPathFreezeCppTest.cpp
  ZeroRuleFinalProgramTest.cpp
)
target_compile_definitions(ACIRExecutableBackendClosureTests PRIVATE
  ACIR_TEST_PYTHON="${Python3_EXECUTABLE}"
  ACIR_TEST_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_TEST_SOURCE_UNIT_HARNESS="$<TARGET_FILE:acir-source-unit-harness>"
  ACIR_TEST_CXX="${ACIR_W10_TEST_CXX}"
  ACIR_TEST_IVERILOG="${ACIR_W10_TEST_IVERILOG}"
  ACIR_TEST_VVP="${ACIR_W10_TEST_VVP}"
)
target_include_directories(ACIRExecutableBackendClosureTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRExecutableBackendClosureTests PRIVATE
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRParser
  MLIRSupport
  GTest::gtest_main
)
add_dependencies(ACIRExecutableBackendClosureTests acir-source-unit-harness)
add_test(NAME ACIRExecutableBackendClosureTests
  COMMAND ACIRExecutableBackendClosureTests)

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

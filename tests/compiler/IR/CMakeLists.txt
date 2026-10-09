find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

# Text IR and codegen behavior live in lit. C++ tests cover the value APIs,
# source identifier oracle and runtime state transitions that need direct access.
function(add_acir_api_test target)
  cmake_parse_arguments(API_TEST "" "GUARD;FILTER" "" ${ARGN})
  if(API_TEST_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "API test guard requires a script path")
  endif()
  add_executable(${target} ${API_TEST_UNPARSED_ARGUMENTS})
  target_include_directories(${target} PRIVATE "${PYCIRCUIT_SOURCE_DIR}/lib")
  target_link_libraries(${target} PRIVATE ACIRDialect MLIRParser MLIRFuncDialect GTest::gtest_main)
  if(NOT API_TEST_FILTER)
    set(API_TEST_FILTER "*")
  endif()
  if(API_TEST_GUARD)
    add_test(NAME ${target}
      COMMAND "${Python3_EXECUTABLE}" "${API_TEST_GUARD}"
        --timeout 60 --rss-mib 1536 -- "$<TARGET_FILE:${target}>"
        "--gtest_filter=${API_TEST_FILTER}")
  else()
    add_test(NAME ${target} COMMAND ${target})
  endif()
  set_tests_properties(${target} PROPERTIES LABELS "api;gate;nightly")
endfunction()

add_acir_api_test(ACIRSourceValueSemanticsTests SourceValueSemanticsTest.cpp)
add_acir_api_test(ACIRExactIntegerLoweringTests ExactIntegerLoweringTest.cpp)
target_link_libraries(ACIRExactIntegerLoweringTests PRIVATE ACIRSourceCompiler)
add_acir_api_test(ACIRFixedUnsignedDivRemLoweringTests FixedUnsignedDivRemLoweringTest.cpp)
target_link_libraries(ACIRFixedUnsignedDivRemLoweringTests PRIVATE ACIRSourceCompiler)
add_acir_api_test(ACIRSourceInterfacePassTests SourceInterfacePassTest.cpp
  SourceMemoryBindingsTest.cpp TableQuerySourceTest.cpp)
target_compile_definitions(ACIRSourceInterfacePassTests PRIVATE
  PYCIRCUIT_TEST_PYTHON="${Python3_EXECUTABLE}"
  PYCIRCUIT_TEST_REPO_ROOT="${PYCIRCUIT_REPO_ROOT}")
target_link_libraries(ACIRSourceInterfacePassTests PRIVATE
  ACIRSourceCompiler ACIRTransforms MLIRPass)
add_acir_api_test(ACIRRecordProjectionSimplificationTests RecordProjectionSimplificationTest.cpp)
target_link_libraries(ACIRRecordProjectionSimplificationTests PRIVATE
  ACIRTransforms MLIRPass)
add_acir_api_test(ACIRQueueContractsTests QueueContractsTest.cpp)
target_link_libraries(ACIRQueueContractsTests PRIVATE
  ACIRSourceCompiler ACIRCodegen ACIRTransforms MLIRPass MLIRTransforms)
target_compile_definitions(ACIRQueueContractsTests PRIVATE
  PYCIRCUIT_TEST_PYTHON="${Python3_EXECUTABLE}"
  PYCIRCUIT_TEST_REPO_ROOT="${PYCIRCUIT_REPO_ROOT}"
  PYCIRCUIT_TEST_LINKER="$<TARGET_FILE:pycircuit-link>"
  PYCIRCUIT_TEST_EMITTER="$<TARGET_FILE:pycircuit-emit>")
add_dependencies(ACIRQueueContractsTests pycircuit-link pycircuit-emit)
add_acir_api_test(ACIRTableDependencySummaryTests TableDependencySummaryTest.cpp)
add_acir_api_test(ACIRHardwareSourceChecksTests
  GUARD "${CMAKE_CURRENT_SOURCE_DIR}/run_source_checks_guarded.py"
  FILTER "-HardwareSourceChecksExecution.*"
  HardwareSourceChecksTest.cpp HardwareSourceCheckOccurrencesTest.cpp
  HardwareSourceCheckCapacityTest.cpp HardwareSourceCheckValidationBudgetTest.cpp
  HardwareDependencyModeReuseTest.cpp
  HardwareSourceChecksExecutionTest.cpp)
set_tests_properties(ACIRHardwareSourceChecksTests PROPERTIES TIMEOUT 90)
target_link_libraries(ACIRHardwareSourceChecksTests PRIVATE ACIRCodegen)
target_compile_definitions(ACIRHardwareSourceChecksTests PRIVATE
  PYCIRCUIT_TEST_PYTHON="${Python3_EXECUTABLE}"
  PYCIRCUIT_TEST_REPO_ROOT="${PYCIRCUIT_REPO_ROOT}"
  PYCIRCUIT_TEST_SOURCE_COMPILER="$<TARGET_FILE:pycircuit-source-unit>"
  PYCIRCUIT_TEST_LINKER="$<TARGET_FILE:pycircuit-link>"
  PYCIRCUIT_TEST_CXX="${CMAKE_CXX_COMPILER}"
  PYCIRCUIT_TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(ACIRHardwareSourceChecksTests pycircuit-source-unit pycircuit-link)
add_test(NAME ACIRHardwareSourceChecksExecution
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/run_source_checks_guarded.py"
    --timeout 900 --rss-mib 1536 -- "$<TARGET_FILE:ACIRHardwareSourceChecksTests>"
    --gtest_filter=HardwareSourceChecksExecution.*)
set_tests_properties(ACIRHardwareSourceChecksExecution PROPERTIES
  LABELS "api;nightly" TIMEOUT 930)

add_acir_api_test(ACIREnumHardwareAnalysisTests EnumHardwareAnalysisTest.cpp)
add_acir_api_test(ACIRFinalSourceUnitsTests FinalSourceUnitsTest.cpp)
add_acir_api_test(ACIREnumSourceUnitAuthorityTests EnumSourceUnitAuthorityTest.cpp)
target_link_libraries(ACIREnumSourceUnitAuthorityTests PRIVATE
  ACIRSourceCompiler ACIRTransforms MLIRPass)
target_compile_definitions(ACIREnumSourceUnitAuthorityTests PRIVATE
  PYCIRCUIT_TEST_PYTHON="${Python3_EXECUTABLE}"
  PYCIRCUIT_TEST_REPO_ROOT="${PYCIRCUIT_REPO_ROOT}")
target_link_libraries(ACIRTableDependencySummaryTests PRIVATE
  ACIRTransforms MLIRPass)
add_acir_api_test(ACIRSourceStaticEvaluationTests SourceStaticEvaluationTest.cpp)
add_acir_api_test(ACIRSourceIdentifierTests SourceIdentifierTest.cpp SourceIdentifierOracleTest.cpp)
set(_module_loop_ir "${CMAKE_BINARY_DIR}/tests/compiler/module_loop.ac")
file(GLOB _source_driver_files CONFIGURE_DEPENDS "${PYCIRCUIT_REPO_ROOT}/python/pycircuit/src/pycircuit/*.py")
add_custom_command(OUTPUT "${_module_loop_ir}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_module_loop.py"
    --repo "${PYCIRCUIT_REPO_ROOT}"
    --source-compiler "$<TARGET_FILE:pycircuit-source-unit>"
    --linker "$<TARGET_FILE:pycircuit-link>" --output "${_module_loop_ir}"
  DEPENDS pycircuit-source-unit pycircuit-link prepare_module_loop.py
    "${PYCIRCUIT_REPO_ROOT}/examples/module_loop/child.py"
    "${PYCIRCUIT_REPO_ROOT}/examples/module_loop/module_loop.py"
    ${_source_driver_files} VERBATIM)
add_custom_target(PycircuitModuleLoopIR DEPENDS "${_module_loop_ir}")
add_acir_api_test(ACIRWorkPartitionTests WorkPartitionTest.cpp)
add_dependencies(ACIRWorkPartitionTests PycircuitModuleLoopIR)
set_tests_properties(ACIRWorkPartitionTests PROPERTIES
  ENVIRONMENT "PYCIRCUIT_TEST_MODULE_LOOP_FINAL=${_module_loop_ir}")
target_compile_definitions(ACIRSourceIdentifierTests PRIVATE
  ACIR_UNICODE_RECIPE_PYTHON="${Python3_EXECUTABLE}")

function(add_gfsim_test target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE "${PYCIRCUIT_REPO_ROOT}/include")
  target_link_libraries(${target} PRIVATE GTest::gtest_main Threads::Threads)
  add_test(NAME ${target} COMMAND ${target})
  set_tests_properties(${target} PROPERTIES LABELS "api;gate;nightly")
endfunction()
set(_runtime_tests "${CMAKE_CURRENT_LIST_DIR}/../../runtime")
add_gfsim_test(ACIRRegRuntimeTests "${_runtime_tests}/RegRuntimeTest.cpp"
  "${_runtime_tests}/TypedMemoryTest.cpp")
add_gfsim_test(ACIRCollectionRuntimeTests "${_runtime_tests}/CollectionRuntimeTest.cpp")
add_gfsim_test(ACIRFifoRuntimeTests "${_runtime_tests}/FifoRuntimeTest.cpp")
add_gfsim_test(ACIRSystemLifecycleTests "${_runtime_tests}/SystemLifecycleTest.cpp")
add_gfsim_test(ACIRWorkExecutorTests "${_runtime_tests}/WorkExecutorTest.cpp")
add_gfsim_test(ACIRObservationTests "${_runtime_tests}/ObservationTest.cpp")
add_gfsim_test(ACIRSimExecutorTests "${_runtime_tests}/SimExecutorTest.cpp"
  "${_runtime_tests}/RunnerCallbacksTest.cpp"
  "${_runtime_tests}/RunnerObservationGroupsTest.cpp"
  "${PYCIRCUIT_REPO_ROOT}/simulator/gfsim/system_runner.cpp"
  "${PYCIRCUIT_REPO_ROOT}/simulator/gfsim/model_input.cpp"
  "${PYCIRCUIT_REPO_ROOT}/simulator/gfsim/sim_executor.cpp")

set(LLVM_EXTERNAL_LIT "" CACHE FILEPATH "Path to the executable LLVM lit test runner")
if(NOT LLVM_EXTERNAL_LIT)
  find_program(PYCIRCUIT_LIT_EXECUTABLE NAMES llvm-lit lit HINTS "${LLVM_TOOLS_BINARY_DIR}")
  if(NOT PYCIRCUIT_LIT_EXECUTABLE)
    message(FATAL_ERROR "PYC_BUILD_TESTING requires lit; set LLVM_EXTERNAL_LIT to its executable")
  endif()
  set(LLVM_EXTERNAL_LIT "${PYCIRCUIT_LIT_EXECUTABLE}")
endif()
find_program(PYCIRCUIT_FILECHECK NAMES FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
set(_lit_binary "${CMAKE_BINARY_DIR}/tests/compiler/lit")
file(MAKE_DIRECTORY "${_lit_binary}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../lit/lit.site.cfg.py.in"
  "${_lit_binary}/lit.site.cfg.py" @ONLY)
# Complementary filters preserve the complete lit inventory without duplicates.
# Fast verifier/pass tests plus source diagnostics and publication protection.
set(_lit_gate_filter "(IR/|Transforms/|Source/(source-interface-extraction(-invalid)?|source-checks|source-observation-behavioral)\\.test|CodeGen/(output-preservation|two-leaves)\\.test)")
add_test(NAME ACIRLitGateTests COMMAND "${LLVM_EXTERNAL_LIT}" -sv
  --filter "${_lit_gate_filter}" "${_lit_binary}")
add_test(NAME ACIRLitNightlyTests COMMAND "${LLVM_EXTERNAL_LIT}" -sv
  --filter-out "${_lit_gate_filter}" "${_lit_binary}")
set_tests_properties(ACIRLitGateTests PROPERTIES LABELS "api;gate;nightly" TIMEOUT 300)
set_tests_properties(ACIRLitNightlyTests PROPERTIES LABELS "api;nightly" TIMEOUT 3600)
set(PYC_TEST_TIER "gate" CACHE STRING "API test selection: gate or nightly")
set_property(CACHE PYC_TEST_TIER PROPERTY STRINGS gate nightly)
if(NOT PYC_TEST_TIER MATCHES "^(gate|nightly)$")
  message(FATAL_ERROR "PYC_TEST_TIER must be gate or nightly")
endif()
add_custom_target(check-pycircuit
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure --no-tests=error
    -L "^api$" -L "^${PYC_TEST_TIER}$" --output-junit "api-${PYC_TEST_TIER}.xml"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  DEPENDS pycircuit-opt pycircuit-emit pycircuit-source-unit pycircuit-link
    ACIRSourceValueSemanticsTests ACIRSourceStaticEvaluationTests ACIRSourceIdentifierTests
    ACIRExactIntegerLoweringTests
    ACIRFixedUnsignedDivRemLoweringTests
    ACIRSourceInterfacePassTests
    ACIRRecordProjectionSimplificationTests
    ACIRQueueContractsTests
    ACIRTableDependencySummaryTests
    ACIRHardwareSourceChecksTests
    ACIREnumHardwareAnalysisTests
    ACIRFinalSourceUnitsTests ACIREnumSourceUnitAuthorityTests
    ACIRRegRuntimeTests ACIRCollectionRuntimeTests ACIRFifoRuntimeTests ACIRSystemLifecycleTests ACIRObservationTests ACIRSimExecutorTests
    ACIRWorkExecutorTests
    ACIRWorkPartitionTests
  USES_TERMINAL VERBATIM)
if(TARGET pyc6_runtime)
  # Source gates link generated DUTs against the Runtime archive from this build.
  add_dependencies(check-pycircuit pyc6_runtime)
endif()

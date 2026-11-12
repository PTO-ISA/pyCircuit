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
  # API tests exercise staged behavior; the dedicated layer-containment
  # probes (ACIRCore/ACIROps/ACIRAnalysis/GFIRStandalone) own the link-line
  # checks. component-split: superseded by pr/validation (GFIRValidation/
  # HWIRValidation join the shared link line with the package-verification
  # component).
  target_link_libraries(${target} PRIVATE ACIRDialect ACIRAnalysis MLIRParser MLIRFuncDialect GTest::gtest_main)
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
add_acir_api_test(ACIRSourceStaticEvaluationTests SourceStaticEvaluationTest.cpp)
add_acir_api_test(ACIRSourceIdentifierTests SourceIdentifierTest.cpp SourceIdentifierOracleTest.cpp
  SourceIdentityContractsTest.cpp)
target_link_libraries(ACIRSourceIdentifierTests PRIVATE Threads::Threads)
target_compile_definitions(ACIRSourceIdentifierTests PRIVATE
  ACIR_UNICODE_RECIPE_PYTHON="${Python3_EXECUTABLE}")
add_acir_api_test(ACIRExactIntegerLoweringTests ExactIntegerLoweringTest.cpp)
target_link_libraries(ACIRExactIntegerLoweringTests PRIVATE ACIRSourceCompiler)
add_acir_api_test(ACIRFixedUnsignedDivRemLoweringTests FixedUnsignedDivRemLoweringTest.cpp)
target_link_libraries(ACIRFixedUnsignedDivRemLoweringTests PRIVATE ACIRSourceCompiler)
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
target_link_libraries(ACIRTableDependencySummaryTests PRIVATE
  ACIRTransforms MLIRPass)
add_acir_api_test(ACIRSourceSymbolicBoundaryTests SourceSymbolicBoundaryTest.cpp)
add_acir_api_test(ACIRLogicalStateInterfacesTests LogicalStateInterfacesTest.cpp)
target_link_libraries(ACIRLogicalStateInterfacesTests PRIVATE PycircuitStateInterfaces GFIRDialect ACIRAnalysis)
add_acir_api_test(ACIREnumHardwareAnalysisTests EnumHardwareAnalysisTest.cpp)
target_link_libraries(ACIREnumHardwareAnalysisTests PRIVATE ACIRAnalysis)
add_acir_api_test(ACIRFinalSourceUnitsTests FinalSourceUnitsTest.cpp)
target_link_libraries(ACIRFinalSourceUnitsTests PRIVATE ACIRAnalysis)
add_acir_api_test(ACIREnumSourceUnitAuthorityTests EnumSourceUnitAuthorityTest.cpp)
target_link_libraries(ACIREnumSourceUnitAuthorityTests PRIVATE
  ACIRSourceCompiler ACIRTransforms MLIRPass)
target_compile_definitions(ACIREnumSourceUnitAuthorityTests PRIVATE
  PYCIRCUIT_TEST_PYTHON="${Python3_EXECUTABLE}"
  PYCIRCUIT_TEST_REPO_ROOT="${PYCIRCUIT_REPO_ROOT}")
# component-split: the anchor, interface-pass, transaction-effect, tier,
# source-check and routed-anchor clusters return with the package-verification
# component (pr/validation) and the transaction conversion component
# (pr/conversion) together with their adapted fixtures.

# Containment probes: each links exactly one layer target; no archives
# above that layer may appear on the resolved link line (verify-layering).
add_executable(ACIRGFIRStandaloneTests GFIRStandaloneTest.cpp)
target_link_libraries(ACIRGFIRStandaloneTests PRIVATE
  GFIRDialect GTest::gtest_main)
add_test(NAME ACIRGFIRStandaloneTests COMMAND ACIRGFIRStandaloneTests)
set_tests_properties(ACIRGFIRStandaloneTests PROPERTIES LABELS "api;gate;nightly")

add_executable(ACIROpsStandaloneTests LayerProbesTest.cpp)
target_link_libraries(ACIROpsStandaloneTests PRIVATE
  ACIRDialect GTest::gtest_main)
add_test(NAME ACIROpsStandaloneTests COMMAND ACIROpsStandaloneTests)
set_tests_properties(ACIROpsStandaloneTests PROPERTIES LABELS "api;gate;nightly")

# Analysis-layer containment probe: links exactly the ACIRAnalysis target
# (the ACIR dialect resolves through its own operations dependency, which is
# below the analysis layer); no target above the analysis layer may appear.
add_executable(ACIRAnalysisStandaloneTests LayerProbesTest.cpp)
target_compile_definitions(ACIRAnalysisStandaloneTests PRIVATE
  PYCIRCUIT_ANALYSIS_PROBE=1)
target_include_directories(ACIRAnalysisStandaloneTests PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib")
target_link_libraries(ACIRAnalysisStandaloneTests PRIVATE
  ACIRAnalysis GTest::gtest_main)
add_test(NAME ACIRAnalysisStandaloneTests COMMAND ACIRAnalysisStandaloneTests)
set_tests_properties(ACIRAnalysisStandaloneTests PROPERTIES LABELS "api;gate;nightly")

add_executable(ACIRCoreStandaloneTests LayerProbesTest.cpp)
target_compile_definitions(ACIRCoreStandaloneTests PRIVATE
  PYCIRCUIT_CORE_PROBE=1)
target_include_directories(ACIRCoreStandaloneTests PRIVATE
  "${PYCIRCUIT_SOURCE_DIR}/lib")
target_link_libraries(ACIRCoreStandaloneTests PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,ACIRCore>" GTest::gtest_main)
add_test(NAME ACIRCoreStandaloneTests COMMAND ACIRCoreStandaloneTests)
set_tests_properties(ACIRCoreStandaloneTests PROPERTIES LABELS "api;gate;nightly")

# component-split: superseded by pr/validation (GFIRValidationStandaloneTests
# and HWIRValidationStandaloneTests join with the package-verification
# component).

set(_module_loop_ir "${CMAKE_BINARY_DIR}/tests/compiler/module_loop.ac")
file(GLOB _source_driver_files CONFIGURE_DEPENDS "${PYCIRCUIT_REPO_ROOT}/python/pycircuit/*.py")
add_custom_command(OUTPUT "${_module_loop_ir}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_module_loop.py"
    --repo "${PYCIRCUIT_REPO_ROOT}"
    --source-compiler "$<TARGET_FILE:pycircuit-source-unit>"
    --linker "$<TARGET_FILE:pycircuit-link>" --output "${_module_loop_ir}"
  DEPENDS pycircuit-source-unit pycircuit-link prepare_module_loop.py
    "${PYCIRCUIT_REPO_ROOT}/tests/fixtures/historical_examples/module_loop/child.py"
    "${PYCIRCUIT_REPO_ROOT}/tests/fixtures/historical_examples/module_loop/module_loop.py"
    ${_source_driver_files} VERBATIM)
add_custom_target(PycircuitModuleLoopIR DEPENDS "${_module_loop_ir}")
add_acir_api_test(ACIRWorkPartitionTests WorkPartitionTest.cpp)
add_dependencies(ACIRWorkPartitionTests PycircuitModuleLoopIR)
set_tests_properties(ACIRWorkPartitionTests PROPERTIES
  ENVIRONMENT "PYCIRCUIT_TEST_MODULE_LOOP_FINAL=${_module_loop_ir}")

function(add_gfsim_test target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE "${PYCIRCUIT_REPO_ROOT}/include")
  target_link_libraries(${target} PRIVATE GTest::gtest_main Threads::Threads)
  add_test(NAME ${target} COMMAND ${target})
  set_tests_properties(${target} PROPERTIES LABELS "api;gate;nightly")
endfunction()
set(_runtime_tests "${CMAKE_CURRENT_LIST_DIR}/../../runtime")
add_gfsim_test(ACIRRegRuntimeTests "${_runtime_tests}/RegRuntimeTest.cpp"
  "${_runtime_tests}/SampledClockTest.cpp"
  "${_runtime_tests}/FourStateCompareTest.cpp"
  "${_runtime_tests}/TypedMemoryTest.cpp")
add_gfsim_test(ACIRCollectionRuntimeTests "${_runtime_tests}/CollectionRuntimeTest.cpp")
add_gfsim_test(ACIRFifoRuntimeTests "${_runtime_tests}/FifoRuntimeTest.cpp")
add_gfsim_test(ACIRSystemLifecycleTests "${_runtime_tests}/SystemLifecycleTest.cpp")
add_gfsim_test(ACIRWorkExecutorTests "${_runtime_tests}/WorkExecutorTest.cpp")
add_gfsim_test(ACIRObservationTests "${_runtime_tests}/ObservationTest.cpp")
add_gfsim_test(ACIRSimExecutorTests "${_runtime_tests}/SimExecutorTest.cpp"
  "${_runtime_tests}/RunnerCallbacksTest.cpp"
  "${_runtime_tests}/RunnerObservationGroupsTest.cpp"
  "${PYCIRCUIT_REPO_ROOT}/runtime/system_runner.cpp"
  "${PYCIRCUIT_REPO_ROOT}/runtime/model_input.cpp"
  "${PYCIRCUIT_REPO_ROOT}/runtime/sim_executor.cpp")

# component-split: superseded by pr/conversion (the lit gate returns with the
# component-split IR text expectations and the final tool inventory).
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
    ACIRRecordProjectionSimplificationTests
    ACIRQueueContractsTests
    ACIRTableDependencySummaryTests
    ACIRSourceSymbolicBoundaryTests
    ACIRLogicalStateInterfacesTests
    ACIREnumHardwareAnalysisTests
    ACIRFinalSourceUnitsTests ACIREnumSourceUnitAuthorityTests
    ACIRRegRuntimeTests ACIRCollectionRuntimeTests ACIRFifoRuntimeTests ACIRSystemLifecycleTests ACIRObservationTests ACIRSimExecutorTests
    ACIRWorkExecutorTests
    ACIRWorkPartitionTests
    ACIRGFIRStandaloneTests ACIROpsStandaloneTests ACIRAnalysisStandaloneTests ACIRCoreStandaloneTests
  USES_TERMINAL VERBATIM)
if(TARGET pyc6_runtime)
  # Source gates link generated DUTs against the Runtime archive from this build.
  add_dependencies(check-pycircuit pyc6_runtime)
endif()

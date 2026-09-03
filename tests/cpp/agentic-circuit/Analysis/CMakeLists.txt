find_package(GTest CONFIG REQUIRED)

add_executable(ACIRModelAnalysisTests ModelAnalysisTest.cpp)
target_include_directories(ACIRModelAnalysisTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRModelAnalysisTests PRIVATE
  ACIRAnalysis
  ACIRDialect
  ACIRTransforms
  ACSimDialect
  MLIRAsmParser
  MLIRBytecodeWriter
  MLIRFuncDialect
  MLIRIndexDialect
  MLIRIR
  MLIRParser
  MLIRSCFDialect
  GTest::gtest_main
)
add_test(NAME ACIRModelAnalysisTests COMMAND ACIRModelAnalysisTests)

add_executable(ACIRProcessStatePlanTests
  ProcessStatePlanBasicTest.cpp
  ProcessStatePlanControlFlowTest.cpp
  ProcessStatePlanWakeTest.cpp
  ProcessStatePlanEmissionTest.cpp
  ProcessStatePlanCostConsumer.cpp
  ProcessStatePlanLimitsTest.cpp
  ProcessStatePlanAtomicityTest.cpp
  ProcessStatePlanVerifierTest.cpp
  ProcessStatePlanTask13ConsumerTest.cpp
)
target_include_directories(ACIRProcessStatePlanTests PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
target_link_libraries(ACIRProcessStatePlanTests PRIVATE
  ACIRAnalysis
  ACIRDialect
  ACIRTransforms
  ACSimDialect
  MLIRAsmParser
  MLIRBytecodeWriter
  MLIRIndexDialect
  MLIRIR
  MLIRParser
  MLIRPass
  GTest::gtest_main
)
add_test(NAME ACIRProcessStatePlanTests COMMAND ACIRProcessStatePlanTests)

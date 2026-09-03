set(ACIR_OPT_SOURCES acir-opt.cpp BindingOptions.cpp)
add_executable(acir-opt ${ACIR_OPT_SOURCES})
set(ACIR_OPT_LIBRARIES
    AgenticCircuit::ProjectOptions ACIRAnalysis ACIRTransforms ACIRToACSim
    ACIRCodeGen ACIRDialect ACSimDialect MLIRFuncDialect MLIRIndexDialect
    MLIRSCFDialect MLIRTransforms MLIROptLib)
target_link_libraries(acir-opt PRIVATE ${ACIR_OPT_LIBRARIES})

if(ACIR_BUILD_TESTING)
  add_executable(acir-opt-internal ${ACIR_OPT_SOURCES})
  target_compile_definitions(acir-opt-internal PRIVATE ACIR_INTERNAL_TEST_TOOL=1)
  target_link_libraries(acir-opt-internal PRIVATE ${ACIR_OPT_LIBRARIES})
endif()

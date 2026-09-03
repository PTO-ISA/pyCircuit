add_executable(acir-build acir-build.cpp)
target_link_libraries(acir-build PRIVATE
    AgenticCircuit::ProjectOptions
    ACIRAnalysis
    ACIRTransforms
    ACIRToACSim
    ACIRCodeGen
    ACIRDialect
    ACSimDialect
    MLIRParser
    MLIRPass
    MLIRFuncDialect
    MLIRIndexDialect
    MLIRSCFDialect
    LLVMSupport
)
target_compile_definitions(acir-build PRIVATE
  "ACIR_HOST_CXX=\"${CMAKE_CXX_COMPILER}\""
  "ACIR_GFSIM_INCLUDE=\"${ACIR_GFSIM_SOURCE_DIR}/include\""
  "ACIR_GFSIM_LIBRARY=\"$<TARGET_FILE:gfsim>\""
)
add_dependencies(acir-build gfsim)

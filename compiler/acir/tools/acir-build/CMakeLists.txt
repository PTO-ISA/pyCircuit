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
string(JOIN "|" ACIR_BUILD_LLVM_LINK_FLAGS ${ACIR_LLVM_LINK_FLAGS})
target_compile_definitions(acir-build PRIVATE
  "ACIR_HOST_CXX=\"${CMAKE_CXX_COMPILER}\""
  "ACIR_BINDINGS_LIBRARY=\"$<TARGET_FILE:ACIRBindings>\""
  "ACIR_GFSIM_INCLUDE=\"${ACIR_GFSIM_SOURCE_DIR}/include\""
  "ACIR_GFSIM_LIBRARY=\"$<TARGET_FILE:gfsim>\""
  "ACIR_BUILD_LLVM_LINK_FLAGS=\"${ACIR_BUILD_LLVM_LINK_FLAGS}\""
)
add_dependencies(acir-build gfsim)

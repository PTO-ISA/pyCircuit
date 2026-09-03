add_acir_library(ACIRCompiler SOURCES
  Driver.cpp
  Pipeline.cpp
)
target_link_libraries(ACIRCompiler PUBLIC
  ACIRCodeGen
  ACIRToACSim
  ACIRTransforms
  ACIRDialect
  ACSimDialect
  MLIRParser
  MLIRPass
  MLIRTransforms
  LLVMSupport
)
target_include_directories(ACIRCompiler PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${ACIR_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(ACIRCompiler PRIVATE
  "${ACIR_SOURCE_DIR}/lib"
)
add_library(AgenticCircuit::ACIRCompiler ALIAS ACIRCompiler)

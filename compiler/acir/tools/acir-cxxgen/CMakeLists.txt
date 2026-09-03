add_executable(acir-cxxgen acir-cxxgen.cpp)
target_include_directories(acir-cxxgen PRIVATE
  "${ACIR_SOURCE_DIR}/lib/CodeGen"
)
target_link_libraries(acir-cxxgen PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
  ACSimDialect
  MLIRParser
)

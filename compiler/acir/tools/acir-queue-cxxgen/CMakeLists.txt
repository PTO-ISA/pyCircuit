add_executable(acir-queue-cxxgen acir-queue-cxxgen.cpp)
target_link_libraries(acir-queue-cxxgen PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
  ACIRDialect
  MLIRParser
)

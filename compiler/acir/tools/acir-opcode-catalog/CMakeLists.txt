add_executable(acir-opcode-catalog acir-opcode-catalog.cpp)
target_link_libraries(acir-opcode-catalog PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
)

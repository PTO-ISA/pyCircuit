add_executable(acir-queue-pycgen acir-queue-pycgen.cpp)
target_link_libraries(acir-queue-pycgen PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
  ACIRDialect
  MLIRParser
)

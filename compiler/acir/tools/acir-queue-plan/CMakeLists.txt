add_executable(acir-queue-plan acir-queue-plan.cpp)
target_link_libraries(acir-queue-plan PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
  ACIRDialect
  MLIRParser
)

add_executable(acc acc.cpp)
target_link_libraries(acc PRIVATE
  AgenticCircuit::ProjectOptions
  ACIRCodeGen
  ACIRDialect
  ACIRTransforms
  MLIRParser
)

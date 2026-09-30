find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(ACIR_BACKEND_CXX NAMES clang++ c++ REQUIRED)
find_program(ACIR_BACKEND_IVERILOG NAMES iverilog REQUIRED)
find_program(ACIR_BACKEND_VVP NAMES vvp REQUIRED)

add_executable(acir-backend-closure-harness
  acir-backend-closure-harness.cpp
  BackendPublication.cpp
  IdleExecution.cpp
  ComposedFixture.cpp
  ComposedLifecycle.cpp
  ComposedExecution.cpp
  ComposedObservations.cpp
  GeneratedStorageInventory.cpp
)
target_compile_definitions(acir-backend-closure-harness PRIVATE
  ACIR_BACKEND_PYTHON="${Python3_EXECUTABLE}"
  ACIR_BACKEND_REPO_ROOT="${ACIR_REPO_ROOT}"
  ACIR_BACKEND_CXX="${ACIR_BACKEND_CXX}"
  ACIR_BACKEND_IVERILOG="${ACIR_BACKEND_IVERILOG}"
  ACIR_BACKEND_VVP="${ACIR_BACKEND_VVP}"
)
target_link_libraries(acir-backend-closure-harness PRIVATE
  acir_project_options
  ACIRDialect
  ACIRSourceCompiler
  MLIRArithDialect
  MLIRFuncDialect
  MLIRParser
  MLIRSupport
)

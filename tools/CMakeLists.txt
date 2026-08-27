# Match a no-RTTI LLVM build: acir-opt subclasses MLIR passes and LLVM
# command-line options, whose vtables must not reference typeinfo symbols
# that no-RTTI LLVM libraries do not export.
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

add_subdirectory(acir-cxxgen)
add_subdirectory(acir-build)
add_subdirectory(acir-opcode-catalog)
add_subdirectory(acir-queue-cxxgen)
add_subdirectory(acir-queue-plan)
add_subdirectory(acir-queue-pycgen)
add_subdirectory(acir-opt)

# The PYC-compatible Verilog bridge is intentionally a Python entrypoint: it
# invokes the in-tree ACIR→PYC generator and lowers the canonical textual PYC
# contract to a self-contained RTL artifact.
install(PROGRAMS
  "${CMAKE_CURRENT_SOURCE_DIR}/acir-queue-veriloggen.py"
  DESTINATION bin
)

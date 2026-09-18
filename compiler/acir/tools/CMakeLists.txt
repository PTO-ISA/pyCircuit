# Match a no-RTTI LLVM build: acir-opt subclasses MLIR passes and LLVM
# command-line options, whose vtables must not reference typeinfo symbols
# that no-RTTI LLVM libraries do not export.
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

add_subdirectory(acc)
add_subdirectory(acir-opcode-catalog)
add_subdirectory(acir-queue-cxxgen)
add_subdirectory(acir-queue-plan)
add_subdirectory(acir-queue-pycgen)
add_subdirectory(acir-opt)

set(ACIR_NATIVE_TOOLS
  acc
  acir-opcode-catalog
  acir-opt
  acir-queue-cxxgen
  acir-queue-plan
  acir-queue-pycgen
)
if(APPLE)
  set(ACIR_TOOL_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
else()
  set(ACIR_TOOL_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()
set_target_properties(${ACIR_NATIVE_TOOLS} PROPERTIES
  INSTALL_RPATH "${ACIR_TOOL_INSTALL_RPATH}")

install(TARGETS acc acir-opcode-catalog acir-opt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# The integrated ACIR-to-PYC lane always consumes the pyc6 targets built from
# this checkout.  Keeping the dependency explicit prevents a stale external
# pyCircuit install from becoming an accidental build input.
if(TARGET pycc AND TARGET pyc6_runtime)
  add_custom_target(acir-pyc-toolchain
    DEPENDS acir-queue-pycgen pycc pyc6_runtime
  )
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(FATAL_ERROR
    "Agentic Circuit integration requires repo-local pycc and pyc6_runtime targets")
else()
  message(STATUS
    "Standalone Agentic Circuit build: ACIR-to-PYC toolchain target is disabled")
endif()

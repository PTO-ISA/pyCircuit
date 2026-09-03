add_acir_library(ACIRBindings SOURCES
  Binding.cpp
  Registry.cpp
)
target_link_libraries(ACIRBindings PUBLIC LLVMSupport)
target_include_directories(ACIRBindings PUBLIC
  "$<BUILD_INTERFACE:${ACIR_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
add_library(AgenticCircuit::ACIRBindings ALIAS ACIRBindings)

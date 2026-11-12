add_acir_library(PycircuitStateInterfaces SOURCES
  LogicalStateInterfaces.cpp
  ModuleChildInterfaces.cpp
)
add_dependencies(PycircuitStateInterfaces
  LogicalStateInterfacesIncGen
  ModuleChildInterfacesIncGen
)

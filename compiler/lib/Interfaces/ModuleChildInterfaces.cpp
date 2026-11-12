#include "pycircuit/Interfaces/ModuleChildInterfaces.h"

#include "pycircuit/Interfaces/ModuleChildInterfaces.cpp.inc"

// Explicit interface identity for the grammar-cache host, emitted in the
// shared interfaces target (B2 step-2 design r9 host hardening).
#include "pycircuit/Dialect/ACIR/GrammarCacheHost.h"
MLIR_DEFINE_EXPLICIT_TYPE_ID(::acir::ac::GrammarCacheHost)

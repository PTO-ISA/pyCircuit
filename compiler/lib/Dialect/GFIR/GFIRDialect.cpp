#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/GFIR/GFIROps.cpp.inc"

#include "pycircuit/Dialect/GFIR/GFIRDialect.cpp.inc"

using namespace mlir;

namespace acir::gfir {

void GFIRDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "pycircuit/Dialect/GFIR/GFIROps.cpp.inc"
      >();
}

} // namespace acir::gfir

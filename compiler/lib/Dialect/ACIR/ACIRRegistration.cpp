#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/GrammarCacheHost.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#include "pycircuit/Dialect/ACIR/ACIREnums.cpp.inc"

#define GET_ATTRDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRAttributes.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRTypes.cpp.inc"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/ACIR/ACIROps.cpp.inc"

namespace {
// Model owning the grammar-memo storage; lifetime = dialect = context.
struct ACIRGrammarCacheHost final : acir::ac::GrammarCacheHost {
  explicit ACIRGrammarCacheHost(mlir::Dialect *dialect)
      : acir::ac::GrammarCacheHost(dialect) {}
  acir::ac::GrammarMemoCache &cache() final { return storage; }
  acir::ac::GrammarMemoCache storage;
};
} // namespace

void acir::ac::ACIRDialect::initialize() {
  addInterfaces<ACIRGrammarCacheHost>();
  addAttributes<
#define GET_ATTRDEF_LIST
#include "pycircuit/Dialect/ACIR/ACIRAttributes.cpp.inc"
      >();
  addTypes<
#define GET_TYPEDEF_LIST
#include "pycircuit/Dialect/ACIR/ACIRTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "pycircuit/Dialect/ACIR/ACIROps.cpp.inc"
      >();
}

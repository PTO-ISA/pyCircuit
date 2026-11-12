// Staged queue configuration and instance checks (B2 step-2 design r3
// ACIRQueueOps row): the definitions moved from the operations TU so the
// archive below the analysis layer carries only the retired-op rejections
// and grammar. Reached from InstanceOp payload contracts (until the r9 §5
// instance reroute), the resolved dispatch and the staged plan walks.
#include "ACIRSourceContracts.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"

#include <functional>
#include <limits>
#include <optional>

using namespace mlir;
namespace acir::ac {


LogicalResult SourceQueuePopOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}
LogicalResult SourceQueuePeekOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}
LogicalResult SourceQueuePushOp::verify() {
  return emitOpError()
         << "retired dedicated queue effects are not admitted; use a native "
            "FIFO Method call";
}

LogicalResult QueueOp::verify() {
  return emitOpError()
         << "retired ac.queue is not admitted; use a canonical FIFO instance";
}

} // namespace acir::ac

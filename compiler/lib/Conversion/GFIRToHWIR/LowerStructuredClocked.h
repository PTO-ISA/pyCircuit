#ifndef PYCIRCUIT_CONVERSION_LOWERSTRUCTUREDCLOCKED_H
#define PYCIRCUIT_CONVERSION_LOWERSTRUCTUREDCLOCKED_H

#include "pycircuit/Dialect/GFIR/StructuredProgram.h"

namespace acir::detail {
mlir::LogicalResult
materializeStructuredClocked(const gfir::StructuredDefinition &definition,
                              const gfir::StructuredProgram &program,
                              mlir::ModuleOp package);
}

#endif

#ifndef ACIR_LIB_COMPILER_FINALHARDWAREDESIGN_H
#define ACIR_LIB_COMPILER_FINALHARDWAREDESIGN_H

#include "CheckGraph.h"
#include "ObservationGraph.h"
#include "ProposalGraph.h"

#include <memory>

namespace acir::compiler {

struct FinalHardwareDesignView {
  std::unique_ptr<ModuleGraph> modules;
  std::unique_ptr<CheckGraph> checks;
  std::unique_ptr<ProposalGraph> proposals;
  std::unique_ptr<ObservationGraph> observations;
};

mlir::FailureOr<FinalHardwareDesignView>
rebuildFinalHardwareDesign(mlir::ModuleOp package,
                            ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALHARDWAREDESIGN_H

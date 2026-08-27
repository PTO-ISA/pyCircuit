#ifndef ACIR_OPT_BINDINGOPTIONS_H
#define ACIR_OPT_BINDINGOPTIONS_H

#include "acir/Conversion/ACIRToACSim/ACIRToACSim.h"
#include "acir/Transforms/ResolveBindings.h"

#include "llvm/Support/Error.h"

#include <optional>
#include <string>

namespace acir::opt {

llvm::Expected<std::optional<ResolveBindingsPassOptions>>
loadBindingCommandLineOptions();

llvm::Expected<std::optional<ACIRToACSimPassOptions>>
loadLoweringCommandLineOptions();

std::string selectedBindingProfile();
std::string selectedBindingTarget();

} // namespace acir::opt

#endif // ACIR_OPT_BINDINGOPTIONS_H

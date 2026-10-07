#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEEFFECTS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEEFFECTS_H

#include "mlir/Interfaces/SideEffectInterfaces.h"

namespace acir::ac {

class SourceCheckResource
    : public mlir::SideEffects::Resource::Base<SourceCheckResource> {
public:
  llvm::StringRef getName() override { return "source.check"; }
};

class SourceObservationResource
    : public mlir::SideEffects::Resource::Base<SourceObservationResource> {
public:
  llvm::StringRef getName() override { return "source.observation"; }
};

} // namespace acir::ac

#endif // PYCIRCUIT_DIALECT_ACIR_SOURCEEFFECTS_H

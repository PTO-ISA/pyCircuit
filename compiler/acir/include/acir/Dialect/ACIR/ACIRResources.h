#ifndef ACIR_DIALECT_ACIR_ACIRRESOURCES_H
#define ACIR_DIALECT_ACIR_ACIRRESOURCES_H

#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>

namespace mlir {
class Operation;
}

namespace acir::ac {

inline constexpr uint64_t kMaxTickScale = uint64_t{1} << 32;
/// ACIR capability bound for exact general mixed-interleave relations.
inline constexpr uint64_t kMaxGeneralSelectorIntersectionQueries = 256;

/// Wide half-open address endpoint type. A 64-bit address space needs the
/// representable endpoint 2^64 even though its largest address is 2^64-1.
using WideAddress = unsigned __int128;

struct AddressInterval {
  WideAddress begin;
  WideAddress end;
};

struct AddressMapOrderKey {
  uint64_t base;
  uint64_t size;
  bool hasPriority;
  uint64_t priority;
};

bool checkedAdd(uint64_t left, uint64_t right, uint64_t &result);
bool checkedMultiply(uint64_t left, uint64_t right, uint64_t &result);
bool intervalsOverlap(AddressInterval left, AddressInterval right);
int compareAddressMapOrder(AddressMapOrderKey left, AddressMapOrderKey right);

/// Computes the normative global tick phase + cycle * period. The result is
/// bounded by the signed i64 tick domain used by ACIR attributes.
bool checkedDomainTick(uint64_t phase, uint64_t period, uint64_t cycle,
                       uint64_t &tick);

/// Converts an exact rational duration to an integral number of global ticks.
/// The duration is numerator/denominator and the global quantum is
/// quantumNumerator/quantumDenominator. The reduced conversion scale (the
/// dimensionless multiplier between duration and quantum) is bounded by
/// kMaxTickScale. Returns false for invalid, inexact, or overflowing
/// conversions; no floating-point rounding is permitted.
bool normalizeRationalToTicks(uint64_t numerator, uint64_t denominator,
                              uint64_t quantumNumerator,
                              uint64_t quantumDenominator, uint64_t &ticks);

struct QueueStateResource
    : public mlir::SideEffects::Resource::Base<QueueStateResource> {
  llvm::StringRef getName() final { return "ac.queue.state"; }
};

struct EventQueueStateResource
    : public mlir::SideEffects::Resource::Base<EventQueueStateResource> {
  llvm::StringRef getName() final { return "ac.event_queue.state"; }
};

struct ReservationStateResource
    : public mlir::SideEffects::Resource::Base<ReservationStateResource> {
  llvm::StringRef getName() final { return "ac.resource.reservation"; }
};

struct ModuleStateResource
    : public mlir::SideEffects::Resource::Base<ModuleStateResource> {
  llvm::StringRef getName() final { return "ac.module.state"; }
};

struct StorageStateResource
    : public mlir::SideEffects::Resource::Base<StorageStateResource> {
  llvm::StringRef getName() final { return "ac.storage.state"; }
};

struct ProtocolStateResource
    : public mlir::SideEffects::Resource::Base<ProtocolStateResource> {
  llvm::StringRef getName() final { return "ac.protocol.state"; }
};

struct TracePositionResource
    : public mlir::SideEffects::Resource::Base<TracePositionResource> {
  llvm::StringRef getName() final { return "ac.trace.position"; }
};

struct ExternalIOResource
    : public mlir::SideEffects::Resource::Base<ExternalIOResource> {
  llvm::StringRef getName() final { return "ac.external_io"; }
};

struct StatisticsResource
    : public mlir::SideEffects::Resource::Base<StatisticsResource> {
  llvm::StringRef getName() final { return "ac.statistics"; }
};

/// Resolves all Task7 cross-operation references from the ac.module producer
/// index and verifies address/time parent graphs exactly once per module.
mlir::LogicalResult verifyModuleResourceReferences(
    mlir::Operation *module,
    const llvm::StringMap<mlir::Operation *> &producerIndex);

/// Rewrites address-map set fields and entries into the unique ACIR
/// total order. This is the deterministic normalization phase used by the
/// mandatory public pipeline; operation verifiers remain mutation-free.
void normalizeAddressMaps(mlir::Operation *topLevel);

} // namespace acir::ac

#endif

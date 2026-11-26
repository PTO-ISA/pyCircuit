#ifndef PYCIRCUIT_HARDWARE_CLOCK_SOURCES_H
#define PYCIRCUIT_HARDWARE_CLOCK_SOURCES_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <cstdint>
#include <optional>

namespace acir::ac {
/// IDs index sources in this invocation only. Portable identity is the tagged
/// root input or (instancePath, declaration occurrence), never the numeric ID.
struct HardwareClockSource {
  std::optional<unsigned> rootInput;
  unsigned ownerOccurrence = 0;
  ClockSourceOp declaration;
  std::optional<unsigned> parentSource;
  unsigned waveDepth = 0;
  /// Root setup is known zero; derived startup is the proven initialized bit.
  bool initialLevel = false;
  /// Authenticated storage producer and static packed bit projection.
  mlir::Operation *storage = nullptr;
  unsigned storageOwnerOccurrence = 0;
  uint64_t bitOffset = 0;
  mlir::Value storageClock, initial;
};
struct HardwareClockOccurrence {
  ModuleOp definition;
  HardwareBindings bindings;
  std::optional<unsigned> parent;
  InstanceOp allocation;
  llvm::SmallVector<mlir::DictionaryAttr> instancePath;
  llvm::StringMap<unsigned> domains;
  llvm::DenseMap<mlir::Operation *, unsigned> children;
};
struct HardwareClockSourcePlan {
  ModuleOp root;
  llvm::SmallVector<HardwareClockOccurrence, 0> occurrences;
  llvm::SmallVector<HardwareClockSource, 0> sources;
  /// Parent-before-child source IDs; simultaneous depths share one snapshot.
  llvm::SmallVector<unsigned> eventOrder;
  unsigned maxWaveDepth = 0;
};
mlir::DictionaryAttr
getHardwareClockSourceIdentity(const HardwareClockSourcePlan &plan,
                               unsigned sourceID);
/// Analyze the scalar registered-derived source profile independently of
/// execution admission. Does not claim a runnable package or authorize emit.
mlir::FailureOr<HardwareClockSourcePlan>
getHardwareClockSourcePlan(mlir::ModuleOp package,
                           HardwareSourceCheckLimits limits = {});
/// Resolve dedicated clock wiring in an already authenticated, unchanged plan.
mlir::FailureOr<unsigned> resolveHardwareClockSource(
    const HardwareClockSourcePlan &plan, mlir::Value value, unsigned owner,
    HardwareSourceCheckLimits limits = {}, bool diagnose = true);
/// Canonical tagged identities + parent/storage provenance, recomputed from IR.
mlir::ArrayAttr
serializeHardwareClockSources(const HardwareClockSourcePlan &plan);
} // namespace acir::ac
#endif

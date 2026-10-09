#ifndef PYCIRCUIT_HARDWARE_SOURCE_CHECK_PREFLIGHT_H
#define PYCIRCUIT_HARDWARE_SOURCE_CHECK_PREFLIGHT_H

#include "HardwareSourceChecks.h"
#include <limits>

namespace acir::ac::detail {
/// Invocation-local work accounting. Saturation never permits wraparound to
/// turn an unaffordable closure into an admitted one.
class SourceCheckBudget {
public:
  explicit SourceCheckBudget(HardwareSourceCheckLimits limits)
      : limit(limits.maxWorkUnits) {}
  static uint64_t add(uint64_t a, uint64_t b) {
    return b > std::numeric_limits<uint64_t>::max() - a
               ? std::numeric_limits<uint64_t>::max()
               : a + b;
  }
  static uint64_t multiply(uint64_t a, uint64_t b) {
    return a && b > std::numeric_limits<uint64_t>::max() / a
               ? std::numeric_limits<uint64_t>::max()
               : a * b;
  }
  static uint64_t bytes(uint64_t size) { return size / 8 + (size % 8 != 0); }
  mlir::LogicalResult admit(uint64_t units, mlir::Operation *site) const {
    // Saturation is always a rejection, even with a maximal explicit limit.
    if (units == std::numeric_limits<uint64_t>::max() || units > limit - used)
      return site->emitOpError()
             << "source-check plan exceeds analysis work budget before "
                "occurrence expansion (requested "
             << units << ", used " << used << ", limit " << limit << ", phase "
             << phase << ")";
    return mlir::success();
  }
  mlir::LogicalResult debit(uint64_t units, mlir::Operation *site) {
    if (mlir::failed(admit(units, site)))
      return mlir::failure();
    used += units;
    return mlir::success();
  }
  void setPhase(llvm::StringRef name) { phase = name; }
  static uint64_t bindings(const HardwareBindings &scope) {
    uint64_t size = sizeof(HardwareBindings);
    for (const auto &entry : scope.integers)
      size = add(size, add(sizeof(entry), entry.first().size() + 1));
    for (const auto &entry : scope.types)
      size = add(size, add(sizeof(entry), entry.first().size() + 1));
    return bytes(size);
  }

private:
  uint64_t limit, used = 0;
  llvm::StringRef phase = "initialization";
};
mlir::FailureOr<HardwareSourceCheckPlan>
buildSourceCheckPlan(const HardwareAnalysis &analysis,
                     HardwareSourceCheckLimits limits);
} // namespace acir::ac::detail
#endif

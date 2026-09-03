#ifndef ACIR_LIB_DIALECT_ACIR_ACIROPS_TEST_HOOKS_H
#define ACIR_LIB_DIALECT_ACIR_ACIROPS_TEST_HOOKS_H

#include <cstdint>

namespace acir::ac::detail {

struct ProcessLivenessWork {
  uint64_t summaryOperationVisits = 0;
  uint64_t epochOperationVisits = 0;
  uint64_t livenessOperationVisits = 0;
  uint64_t valueVisits = 0;
  uint64_t useVisits = 0;

  uint64_t total() const {
    return summaryOperationVisits + epochOperationVisits +
           livenessOperationVisits + valueVisits + useVisits;
  }
};

class ScopedProcessLivenessWorkCollector {
public:
  explicit ScopedProcessLivenessWorkCollector(ProcessLivenessWork &work);
  ~ScopedProcessLivenessWorkCollector();

  ScopedProcessLivenessWorkCollector(
      const ScopedProcessLivenessWorkCollector &) = delete;
  ScopedProcessLivenessWorkCollector &
  operator=(const ScopedProcessLivenessWorkCollector &) = delete;

private:
  ProcessLivenessWork *previous;
};

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIROPS_TEST_HOOKS_H

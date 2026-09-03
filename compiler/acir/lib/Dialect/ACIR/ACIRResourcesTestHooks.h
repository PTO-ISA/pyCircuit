#ifndef ACIR_LIB_DIALECT_ACIR_ACIR_RESOURCES_TEST_HOOKS_H
#define ACIR_LIB_DIALECT_ACIR_ACIR_RESOURCES_TEST_HOOKS_H

#include <cstdint>

namespace acir::ac::detail {

struct AddressMapVerificationWork {
  uint64_t entryNormalizationVisits = 0;
  uint64_t concreteEntryVisits = 0;
  uint64_t concreteExpirationVisits = 0;
  uint64_t selectorQueryVisits = 0;
  uint64_t selectorUpdateVisits = 0;
  uint64_t candidateIntersectionChecks = 0;
  uint64_t generalRelationChecks = 0;

  uint64_t total() const {
    return entryNormalizationVisits + concreteEntryVisits +
           concreteExpirationVisits + selectorQueryVisits +
           selectorUpdateVisits + candidateIntersectionChecks +
           generalRelationChecks;
  }
};

class ScopedAddressMapVerificationWorkCollector {
public:
  explicit ScopedAddressMapVerificationWorkCollector(
      AddressMapVerificationWork &work);
  ~ScopedAddressMapVerificationWorkCollector();

  ScopedAddressMapVerificationWorkCollector(
      const ScopedAddressMapVerificationWorkCollector &) = delete;
  ScopedAddressMapVerificationWorkCollector &
  operator=(const ScopedAddressMapVerificationWorkCollector &) = delete;

private:
  AddressMapVerificationWork *previous;
};

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIR_RESOURCES_TEST_HOOKS_H

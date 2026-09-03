#ifndef ACIR_LIB_DIALECT_ACSIM_ACSIMOPSTESTHOOKS_H
#define ACIR_LIB_DIALECT_ACSIM_ACSIMOPSTESTHOOKS_H

#include <cstdint>

namespace acir::acsim::detail {

struct ModelVerificationWork {
  uint64_t preflightOperationVisits = 0;
  uint64_t preorderOperationVisits = 0;
  uint64_t indexOperationVisits = 0;
  uint64_t closureOperationVisits = 0;
  uint64_t orderingOperationVisits = 0;
  uint64_t constructionOperationVisits = 0;
  uint64_t semanticOperationVisits = 0;
  uint64_t runtimeOperationVisits = 0;
  uint64_t edgeVisits = 0;
  uint64_t referenceLookups = 0;
  uint64_t expandedOwnerRows = 0;
  uint64_t expandedRuntimeRows = 0;

  uint64_t total() const {
    return preflightOperationVisits + preorderOperationVisits +
           indexOperationVisits + closureOperationVisits +
           orderingOperationVisits + constructionOperationVisits +
           semanticOperationVisits + runtimeOperationVisits + edgeVisits +
           referenceLookups + expandedOwnerRows + expandedRuntimeRows;
  }
};

class ScopedModelVerificationWorkCollector {
public:
  explicit ScopedModelVerificationWorkCollector(ModelVerificationWork &work);
  ~ScopedModelVerificationWorkCollector();

  ScopedModelVerificationWorkCollector(
      const ScopedModelVerificationWorkCollector &) = delete;
  ScopedModelVerificationWorkCollector &
  operator=(const ScopedModelVerificationWorkCollector &) = delete;

private:
  ModelVerificationWork *previous;
};

struct ModelVerificationLimits {
  uint64_t maxNodes = 1ULL << 20;
  uint64_t maxEdges = 1ULL << 22;
  uint64_t maxRegionDepth = 512;
  uint64_t maxExpandedObjects = 1ULL << 20;
  uint64_t maxAttributeElements = 1ULL << 20;
  uint64_t maxAttributeStringBytes = 1ULL << 24;
  uint64_t maxDependencyNodes = 1ULL << 20;
};

class ScopedModelVerificationLimits {
public:
  explicit ScopedModelVerificationLimits(const ModelVerificationLimits &limits);
  ~ScopedModelVerificationLimits();

  ScopedModelVerificationLimits(const ScopedModelVerificationLimits &) = delete;
  ScopedModelVerificationLimits &
  operator=(const ScopedModelVerificationLimits &) = delete;

private:
  const ModelVerificationLimits *previous;
};

} // namespace acir::acsim::detail

#endif // ACIR_LIB_DIALECT_ACSIM_ACSIMOPSTESTHOOKS_H

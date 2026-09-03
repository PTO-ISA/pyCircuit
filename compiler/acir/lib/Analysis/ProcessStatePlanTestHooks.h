#ifndef ACIR_ANALYSIS_PROCESSSTATEPLANTESTHOOKS_H
#define ACIR_ANALYSIS_PROCESSSTATEPLANTESTHOOKS_H

#include "acir/Analysis/ProcessStatePlan.h"

namespace acir {

enum class ProcessStatePlanCorruptionForTest {
  DuplicateOrdinal,
  NonDenseOrdinal,
  DanglingReference,
  DuplicateIdentity,
  UnsortedCanonicalOrder,
  CostMismatch,
  DefinitionKeyMismatch,
  CalleeSpecializationMismatch,
  ValueTypeSpecializationMismatch,
  EffectMismatch,
  IdKindMismatch,
  WrongTypeKey,
  InvalidFramePhase,
  InvalidEdgeBinding,
  InvalidWakeCallee
};

ProcessStatePlanSet cloneProcessStatePlanWithCorruptionForTest(
    const ProcessStatePlanSet &plan,
    ProcessStatePlanCorruptionForTest corruption);

} // namespace acir

#endif

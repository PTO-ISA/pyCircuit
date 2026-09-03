#ifndef ACIR_CODEGEN_QUEUEBLOCKCONTRACT_H
#define ACIR_CODEGEN_QUEUEBLOCKCONTRACT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <string>
#include <vector>

namespace acir::codegen {

struct QueueBlockContract {
  std::string kind;
  std::string operation;
  std::string category;
  std::string role;
  std::string effect;
  int64_t minimumInputs = 0;
  int64_t maximumInputs = 0;
  int64_t minimumOutputs = 0;
  int64_t maximumOutputs = 0;
  std::string payloadRelation;
  std::vector<std::string> constants;
  bool gfsimAvailable = false;
  std::string gfsimRealization;
  bool pycAvailable = false;
  std::string pycRealization;
  std::vector<std::string> refinementObservations;
};

llvm::ArrayRef<QueueBlockContract> officialQueueBlockContracts();
const QueueBlockContract *findQueueBlockContract(llvm::StringRef kind);
llvm::Expected<std::string> canonicalQueueBlockCatalogJson();

} // namespace acir::codegen

#endif // ACIR_CODEGEN_QUEUEBLOCKCONTRACT_H

#ifndef PYCIRCUIT_REGISTERCONTRACTS_H
#define PYCIRCUIT_REGISTERCONTRACTS_H
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"
#include <optional>
namespace acir::ac {
/// Validated register primitive version: absent marker is legacy v1.
mlir::FailureOr<unsigned> getRegisterContract(mlir::Operation *declaration);

enum class RegisterEdge { Posedge, Negedge };
enum class RegisterResetKind { None, Sync, Async };
enum class RegisterResetPolarity { High, Low };
struct RegisterPolicy {
  RegisterEdge edge = RegisterEdge::Posedge;
  RegisterResetKind resetKind = RegisterResetKind::Sync;
  RegisterResetPolarity resetPolarity = RegisterResetPolarity::High;
  bool hasInitial = true;
  bool operator==(const RegisterPolicy &other) const {
    return edge == other.edge && resetKind == other.resetKind &&
           resetPolarity == other.resetPolarity && hasInitial == other.hasInitial;
  }
  bool operator!=(const RegisterPolicy &other) const { return !(*this == other); }
};
/// Read the validated logical or physical register policy.
mlir::FailureOr<RegisterPolicy> getRegisterPolicy(mlir::Operation *operation);
struct RegisterPrimitiveLayout {
  unsigned contract;
  unsigned clock = 0, reset = 1;
  std::optional<unsigned> enable;
  unsigned data, initial, resetValue;
};
/// v3 has an independent reset-value pin; absent initialization is typed all-X.
mlir::FailureOr<RegisterPrimitiveLayout>
getRegisterPrimitiveLayout(mlir::Operation *declaration);

}
#endif

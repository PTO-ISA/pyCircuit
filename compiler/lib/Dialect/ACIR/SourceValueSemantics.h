#ifndef ACIR_LIB_DIALECT_ACIR_SOURCEVALUESEMANTICS_H
#define ACIR_LIB_DIALECT_ACIR_SOURCEVALUESEMANTICS_H

#include "ACIRSourceContracts.h"
#include "llvm/ADT/ArrayRef.h"

#include <array>
#include <optional>

namespace acir::ac::detail {

enum class ValueOpcode {
  Neg,
  Invert,
  Not,
  ToInt,
  Add,
  Sub,
  Mul,
  FloorDiv,
  Mod,
  AndBits,
  OrBits,
  XorBits,
  Shl,
  Shr,
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  AndBool,
  OrBool
};
enum class ValueKind { Boolean, Integer };
enum class ValueKindConstraint { Boolean, Integer, BooleanOrInteger };
enum class ValueEvaluation { Strict, AndShortCircuit, OrShortCircuit };
struct ValueOpcodeInfo {
  unsigned arity;
  std::array<ValueKindConstraint, 2> operands;
  ValueKind result;
  ValueEvaluation evaluation;
  bool mayCheck;
};

std::optional<ValueOpcode> parseValueOpcode(llvm::StringRef spelling);
ValueOpcodeInfo getValueOpcodeInfo(ValueOpcode opcode);
mlir::FailureOr<mlir::Attribute>
evaluateValue(ValueOpcode opcode, llvm::ArrayRef<mlir::Attribute> operands,
              EmitError error);

} // namespace acir::ac::detail
#endif

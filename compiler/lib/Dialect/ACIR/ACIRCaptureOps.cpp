#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"

using namespace mlir;
namespace acir::ac {
bool isACIRCapture(Operation *operation) {
  auto package = dyn_cast<mlir::ModuleOp>(operation);
  if (!package) package = operation->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  return stage && stage.getValue() == "acir";
}
bool isACIRCaptureOperation(Operation *operation) {
  return isa<CallOp, QueryMapOp, QueryChooseOp, BoundaryOp, IndexCheckOp>(operation);
}
Operation *resolvePackageHelper(Operation *site, FlatSymbolRefAttr callee) {
  auto package = site->getParentOfType<mlir::ModuleOp>();
  if (!package || !callee) return nullptr;
  return SymbolTable::lookupSymbolIn(package, callee);
}
Operation *CallOp::resolveCallable() {
  return resolvePackageHelper(*this, getCalleeAttr());
}
Operation *CallOp::resolveCallableInTable(SymbolTableCollection *) {
  return resolveCallable();
}
LogicalResult CallOp::verify() {
  auto function = dyn_cast_or_null<func::FuncOp>(resolveCallable());
  auto kind = function ? function->getAttrOfType<StringAttr>("ac.helper_kind") : StringAttr();
  if (!function || !kind || kind.getValue() != "value" ||
      !llvm::equal(function.getFunctionType().getInputs(), getOperandTypes()) ||
      !llvm::equal(function.getFunctionType().getResults(), getResultTypes()))
    return emitOpError("requires a package helper with matching argument and result types");
  if (!isACIRCapture(*this))
    return emitOpError("requires the acir stage");
  if (auto lexical = function->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner")) {
    auto module = (*this)->getParentOfType<ModuleOp>();
    auto caller = (*this)->getParentOfType<func::FuncOp>();
    auto owner = module ? FlatSymbolRefAttr::get(getContext(), module.getSymName()) :
        caller ? caller->getAttrOfType<FlatSymbolRefAttr>("ac.lexical_owner") : FlatSymbolRefAttr();
    if (owner != lexical) return emitOpError("private helper call crosses lexical owner");
  }
  return success();
}
LogicalResult QueryMapOp::verify() {
  if (!isACIRCapture(*this) || getTableCount() <= 0 ||
      static_cast<uint64_t>(getTableCount()) > getNumOperands() || !getNumResults())
    return emitOpError("requires ACIR tables, explicit captures and results");
  auto first = dyn_cast<TableType>(getOperand(0).getType());
  if (!first) return emitOpError("requires Table inputs");
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto shape = analysis.getTableShape(first, {}, *this);
  if (failed(shape) || shape->size() != 1 || !shape->front())
    return emitOpError("requires a positive closed rank-one Table");
  for (Value input : getOperands().take_front(getTableCount())) {
    auto type = dyn_cast<TableType>(input.getType());
    if (!type) return emitOpError("requires Table inputs");
    auto other = analysis.getTableShape(type, {}, *this);
    if (failed(other) || *other != *shape) return emitOpError("requires equal Table shapes");
  }
  for (Type raw : getResultTypes()) {
    auto type = dyn_cast<TableType>(raw);
    if (!type) return emitOpError("requires Table results");
    auto other = analysis.getTableShape(type, {}, *this);
    if (failed(other) || *other != *shape) return emitOpError("requires result shape matching inputs");
  }
  return success();
}
LogicalResult QueryMapOp::verifyRegions() {
  if (!getBody().hasOneBlock()) return emitOpError("requires one callback block");
  Block &block = getBody().front();
  SmallVector<Type> args, results;
  for (auto [i, value] : llvm::enumerate(getOperands()))
    args.push_back(i < static_cast<uint64_t>(getTableCount()) ?
        cast<TableType>(value.getType()).getElementType() : value.getType());
  for (Type type : getResultTypes()) results.push_back(cast<TableType>(type).getElementType());
  if (!llvm::equal(args, block.getArgumentTypes()) || block.empty() ||
      !isa<YieldOp>(block.back()) || !llvm::equal(results, block.back().getOperandTypes()))
    return emitOpError("callback arguments and yield must match its typed interface");
  for (Operation &operation : block.without_terminator())
    if (!isACIRCaptureOperation(&operation) && !detail::isCombinationalHardwareOperation(&operation))
      return operation.emitOpError("query callback must be pure");
  return success();
}
LogicalResult QueryChooseOp::verify() {
  if (!isACIRCapture(*this) || (getKind() != "first" && getKind() != "argmin") ||
      getNumOperands() != (getKind() == "first" ? 1u : 2u) || getNumResults() != 2)
    return emitOpError("requires first/argmin operands and index/valid results");
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto predicates = dyn_cast<TableType>(getOperand(0).getType());
  if (!predicates) return emitOpError("requires a predicate Table");
  auto size = analysis.getTableSize(predicates, {}, *this);
  auto width = analysis.getPackedWidth(predicates.getElementType(), {}, *this);
  if (failed(size) || !*size || failed(width) || *width != 1)
    return emitOpError("requires a nonempty one-bit predicate Table");
  if (getNumOperands() == 2) {
    auto keys = dyn_cast<TableType>(getOperand(1).getType());
    if (!keys || !isa<BitsType>(keys.getElementType())) return emitOpError("requires Bits keys");
    auto keySize = analysis.getTableSize(keys, {}, *this);
    if (failed(keySize) || *keySize != *size) return emitOpError("key and predicate sizes differ");
  }
  for (Type type : getResultTypes()) if (!isa<BitsType>(type)) return emitOpError("requires Bits results");
  auto indexWidth = analysis.getPackedWidth(getResult(0).getType(), {}, *this);
  auto validWidth = analysis.getPackedWidth(getResult(1).getType(), {}, *this);
  if (failed(indexWidth) || failed(validWidth) || *validWidth != 1 ||
      *indexWidth != std::max(1u, llvm::APInt(64, *size - 1).getActiveBits()))
    return emitOpError("query result widths do not match extent");
  return success();
}
LogicalResult BoundaryOp::verify() {
  auto kind = getConstraint().getAs<StringAttr>("source_kind");
  auto type = getConstraint().getAs<TypeAttr>("type");
  if (!isACIRCapture(*this) || !type || type.getValue() != getResult().getType() ||
      !kind || (kind.getValue() != "fixed_bits" && kind.getValue() != "integer" && kind.getValue() != "boolean"))
    return emitOpError("requires a typed ACIR scalar boundary contract");
  Operation *owner = (*this)->getParentOfType<ModuleOp>();
  if (!owner) owner = (*this)->getParentOfType<func::FuncOp>();
  if (!owner) return emitOpError("requires a lexical Module or helper");
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  return detail::verifySourceCallHardwareConstraint(getConstraint(),
      getResult().getType(), analysis, owner, *this, "ACIR boundary");
}
LogicalResult IndexCheckOp::verify() {
  if (!isACIRCapture(*this) || getExtent() <= 0 || getInput().getType() != getResult().getType())
    return emitOpError("requires a positive extent and unchanged original index type");
  return success();
}
} // namespace acir::ac

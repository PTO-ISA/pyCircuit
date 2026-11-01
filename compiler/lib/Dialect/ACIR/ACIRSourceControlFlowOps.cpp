#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir::ac {
LogicalResult verifyStaticLoopReferences(Operation *operation) {
  llvm::DenseSet<Attribute> attributes;
  llvm::DenseSet<Type> types;
  bool invalid = false;
  std::function<void(Attribute)> checkAttribute;
  std::function<void(Type)> checkType;
  checkAttribute = [&](Attribute attribute) {
    if (!attribute || !attributes.insert(attribute).second) return;
    if (auto expression = dyn_cast<StaticExprAttr>(attribute)) {
      auto reference = expression.getTree().getAs<DictionaryAttr>("ref");
      auto kind = reference ? reference.getAs<StringAttr>("kind") : StringAttr();
      if (kind && kind.getValue() == "induction") {
        bool found = false;
        for (Operation *at = operation->getParentOp(); at; at = at->getParentOp())
          if (isa<scf::ForOp>(at) && at->getAttr("occurrence") == reference.get("loop"))
            found = true;
        if (!found || isLinkedTransactionalPackage(operation)) invalid = true;
      }
    }
    if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
      for (NamedAttribute field : dictionary)
        if (field.getName() != "origin" && field.getName() != "location" &&
            field.getName() != "ac.origin" && field.getName() != "occurrence")
          checkAttribute(field.getValue());
      return;
    }
    attribute.walkImmediateSubElements(checkAttribute, checkType);
  };
  checkType = [&](Type type) {
    if (type && types.insert(type).second)
      type.walkImmediateSubElements(checkAttribute, checkType);
  };
  for (NamedAttribute field : operation->getAttrs())
    if (field.getName() != "ac.origin" && field.getName() != "occurrence")
      checkAttribute(field.getValue());
  for (Type type : operation->getOperandTypes()) checkType(type);
  for (Type type : operation->getResultTypes()) checkType(type);
  if (invalid) return operation->emitOpError("static induction reference escapes its lexical loop");
  return success();
}

namespace {
LogicalResult verifyStaticBridge(Operation *operation, StaticExprAttr expression) {
  if (failed(StaticExprAttr::verify([&] { return operation->emitOpError(); },
                                    expression.getTree())))
    return failure();
  Operation *owner = operation->getParentOp();
  while (owner && !isa<ModuleOp, func::FuncOp>(owner) &&
         !(isa<TransactionRuleOp>(owner) &&
           !owner->getParentOfType<ModuleOp>()))
    owner = owner->getParentOp();
  if (!owner) return operation->emitOpError("static bridge requires a computation scope");
  return detail::verifyHardwareAttributeScope(expression, owner);
}
}

LogicalResult SourceStaticPredicateOp::verify() {
  if (isLinkedTransactionalPackage(*this))
    return emitOpError("static conditions require Source ACIR");
  for (OpOperand &use : getResult().getUses())
    if (!isa<scf::IfOp>(use.getOwner()) || use.getOperandNumber() != 0)
      return emitOpError("static condition may only control scf.if");
  return verifyStaticBridge(*this, getValue());
}

LogicalResult SourceUnboundOp::verify() {
  if (!isa<InferType>(getResult().getType()) || isLinkedTransactionalPackage(*this))
    return emitOpError("unassigned locals require unresolved Source ACIR");
  return success();
}

LogicalResult SourceStaticIndexOp::verify() {
  return verifyStaticBridge(*this, getValue());
}

LogicalResult SourceIndexOp::verify() {
  if (!isa<BitsType, InferType>(getResult().getType()))
    return emitOpError("source induction requires a payload or pending type");
  if (failed(detail::verifyOccurrence(getOccurrence(),
                                      [&] { return emitOpError(); })) ||
      failed(detail::verifySourceSpan(getSourceLocation(),
                                      [&] { return emitOpError(); })))
    return failure();
  return success();
}

LogicalResult SourcePredicateOp::verify() {
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  // The bridge preserves an original four-state control value in both source
  // SCF and transactional v2 ACIR. Its i1 result is structural, not evidence
  // that the input is known and not an executable two-state conversion.
  HardwareAnalysis analysis(package);
  HardwareBindings bindings;
  bindings.owner = (*this)->getParentOfType<ModuleOp>();
  auto width = analysis.getPackedWidth(getInput().getType(), bindings, *this);
  if (failed(width) || *width != 1)
    return emitOpError() << "source SCF predicate requires width-one Bits";
  for (OpOperand &use : getResult().getUses()) {
    auto name = use.getOwner()->getName().getStringRef();
    if (use.getOperandNumber() != 0 ||
        (name != "scf.if" && name != "scf.condition"))
      return emitOpError()
             << "source SCF predicate may only control scf.if/scf.condition";
  }
  return success();
}
} // namespace acir::ac

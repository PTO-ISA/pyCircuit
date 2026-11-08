#include "ACIRSourceContracts.h"
#include "IR/StateDeclarationContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult moduleOccurrence(Operation *operation, DictionaryAttr identity,
                               StringRef description) {
  auto error = [&] { return operation->emitOpError(); };
  if (failed(detail::verifyOccurrence(identity, error)))
    return failure();
  auto owner = operation->getParentOfType<ModuleOp>();
  auto symbol =
      owner ? owner->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  if (!symbol)
    return error() << description << " requires a named owning module";
  return detail::verifyOriginDefinition(
      identity,
      FlatSymbolRefAttr::get(operation->getContext(), symbol.getValue()),
      description, error);
}

LogicalResult occurrence(Operation *operation) {
  return moduleOccurrence(
      operation, operation->getAttrOfType<DictionaryAttr>("occurrence"),
      "typed source operation");
}

LogicalResult namedDeclaration(Operation *operation, StringAttr name) {
  // Single shared leaf implementation for source and routed declarations.
  return verifyNamedLogicalDeclaration(operation, name);
}

bool equivalentTypes(TypeRange left, TypeRange right) {
  if (left.size() != right.size())
    return false;
  for (auto [lhs, rhs] : llvm::zip(left, right))
    if (!areEquivalentHardwareTypes(lhs, rhs))
      return false;
  return true;
}

LogicalResult transactionPlacement(Operation *operation, bool allowGuard,
                                   bool allowBranch = false) {
  auto *placed = operation;
  auto *parent = placed->getParentOp();
  while (allowBranch && isa_and_nonnull<SourceBranchOp>(parent)) {
    placed = parent;
    parent = placed->getParentOp();
  }
  if (!isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent))
    return operation->emitOpError() << "requires direct method or transaction "
                                       "region placement, or legal branch arms";
  if ((!allowGuard || placed != operation) &&
      placed->getParentRegion() != &parent->getRegion(1))
    return operation->emitOpError()
           << "state writes and method calls are forbidden in guards";
  return occurrence(operation);
}

LogicalResult region(Operation *operation, unsigned ordinal, TypeRange inputs,
                     TypeRange outputs) {
  auto &region = operation->getRegion(ordinal);
  bool guard = ordinal == 0;
  if (!region.hasOneBlock() || region.front().empty() ||
      !equivalentTypes(region.front().getArgumentTypes(), inputs) ||
      !isa<YieldOp>(region.front().back()))
    return operation->emitOpError()
           << "guard/body requires one block, matching arguments and final "
              "ac.yield";
  auto &block = region.front();
  auto &yield = block.back();
  if (guard) {
    if (yield.getNumOperands() != 1)
      return yield.emitOpError() << "guard must yield exactly one predicate";
  } else if (!equivalentTypes(yield.getOperandTypes(), outputs)) {
    return yield.emitOpError() << "body yield types do not match results";
  }
  for (Operation &nested : block.without_terminator()) {
    if (isa<SourceStateReadOp>(nested))
      continue;
    if (!guard &&
        isa<SourceStateWriteOp, SourceFormalWriteOp, SourceMethodCallOp,
            SourceExpectOp, SourceObserveOp, SourceBranchOp>(nested))
      continue;
    if (!detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << (guard ? "guard accepts only closed pure payload operations "
                         "and state reads"
                       : "body accepts only closed payload operations, state "
                         "accesses, method calls, checks and observations");
    // Pure nested collection regions do not open another effect placement.
    auto walk =
        nested.walk([&](Operation *child) {
          if (child == &nested ||
              detail::isCombinationalHardwareOperation(child) ||
              isa<YieldOp>(child))
            return WalkResult::advance();
          child->emitOpError()
              << "nested payload region cannot contain transaction "
                 "effects or declarations";
          return WalkResult::interrupt();
        });
    if (walk.wasInterrupted())
      return failure();
  }
  return success();
}

// Structured arms are the only nested regions that can carry transaction
// effects. Pure collection regions remain closed even when enclosed by a
// branch.
LogicalResult branchPayload(Operation *operation, bool arm, unsigned depth,
                            uint64_t &work) {
  if (depth > 128 || ++work > 1024 * 1024)
    return operation->emitOpError()
           << "source branch verification budget exceeded";
  if (arm && isa<SourceBranchOp>(operation)) {
    for (Region &nested : operation->getRegions())
      for (Block &block : nested)
        for (Operation &child : block)
          if (failed(branchPayload(&child, true, depth + 1, work)))
            return failure();
    return success();
  }
  if (arm && isa<SourceStateReadOp, SourceStateWriteOp>(operation)) {
    auto selectors = operation->getAttrOfType<ArrayAttr>("selectors");
    Value payload = isa<SourceStateReadOp>(operation)
                        ? operation->getResult(0)
                        : operation->getOperand(1);
    if (!selectors ||
        !isa<BitsType, EnumType, StructType, TableType>(payload.getType()))
      return operation->emitOpError()
             << "source branch arms require selected finite state accesses";
    return success();
  }
  if (arm &&
      isa<SourceFormalWriteOp, SourceMethodCallOp, SourceExpectOp,
          SourceObserveOp>(operation))
    return success();
  if (!isa<YieldOp>(operation) &&
      !detail::isCombinationalHardwareOperation(operation))
    return operation->emitOpError()
           << "source branch arms accept only closed pure payload operations, "
              "selected finite state accesses, Method calls, checks, "
              "observations "
              "and nested branches";
  for (Region &nested : operation->getRegions())
    for (Block &block : nested)
      for (Operation &child : block)
        if (failed(branchPayload(&child, false, depth + 1, work)))
          return failure();
  return success();
}

} // namespace

LogicalResult SourceStateOp::verify() {
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                       : StringAttr();
  if (stage && stage.getValue() == "final")
    return emitOpError() << "logical state is forbidden in a final package";
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "state uses instance_name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getInstanceNameAttr())))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  for (Operation &other : owner.getBody().front())
    if (&other != getOperation() &&
        isa<SourceStateOp, InstanceOp, CollectionOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "state instance_name must be unique across state "
                              "and child instance declarations";
  for (auto other : owner.getBody().front().getOps<SourceStateOp>()) {
    if (other == *this)
      continue;
    if (failed(occurrence(other)))
      return failure();
    if (detail::compareClosedSourceStructure(getOccurrence(),
                                             other.getOccurrence()) == 0)
      return emitOpError() << "state declarations require distinct occurrence "
                              "identities within their module";
  }
  // The declaration binding (width-one controls, initial/Q payload
  // equivalence, payload constraint and the closed static initializer) is
  // the staged tier's verifyLogicalStateSourceBinding, owned by the
  // source-check catalog declaration row (B1-boundary.md); the native
  // verifier keeps the declaration grammar.
  return success();
}

LogicalResult SourceStateReadOp::verify() {
  return transactionPlacement(*this, true, true);
}

LogicalResult SourceStateWriteOp::verify() {
  return transactionPlacement(*this, false, true);
}

LogicalResult SourceFormalWriteOp::verify() {
  for (NamedAttribute attr : (*this)->getAttrs())
    if (!llvm::is_contained(ArrayRef<StringRef>{"formal", "selectors",
                                                "operandSegmentSizes",
                                                "occurrence", "ac.location"},
                            attr.getName().getValue()))
      return emitOpError() << "unknown formal write attribute";
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  auto method = (*this)->getParentOfType<SourceMethodOp>();
  if (!method)
    return emitOpError() << "formal write requires a lexical Method formal";
  auto formal = getFormalAttr();
  auto signature = method.getFunctionType();
  auto proposals = method->getAttrOfType<ArrayAttr>("ac.formal_proposals");
  if (!formal.getType().isSignlessInteger(64) || formal.getInt() < 0 ||
      static_cast<uint64_t>(formal.getInt()) >= signature.getNumInputs() ||
      !proposals || !llvm::is_contained(proposals, formal))
    return emitOpError()
           << "formal write must identify a declared writable input formal";
  if (!areEquivalentHardwareTypes(getCandidate().getType(),
                                  signature.getInput(formal.getInt())))
    return emitOpError()
           << "formal write candidate must match the whole input formal type";
  // The selected payload agreement and the control predicate are the staged
  // transaction-surface entry.
  if (bool(getPrevious()) != bool(getSnapshot()))
    return emitOpError() << "forwarded formal previous and snapshot must be present together";
  if (getPrevious() &&
      (!areEquivalentHardwareTypes(getPrevious().getType(), getCandidate().getType()) ||
       !areEquivalentHardwareTypes(getSnapshot().getType(), getCandidate().getType())))
    return emitOpError() << "forwarded formal snapshot must preserve the whole formal type";
  if (getPrevious() && (!getSelectors().empty() || !getIndices().empty()))
    return emitOpError()
           << "forwarded formal proposal must inherit callee write domains";
  auto owner = (*this)->getParentOfType<ModuleOp>();
  if (failed(detail::verifySourceEffectMetadata(
          getOccurrence(), getSourceLocation(), owner,
          [&] { return emitOpError(); })) ||
      failed(detail::verifyHardwareTypeScope(getPayload().getType(), owner)) ||
      failed(detail::verifyHardwareTypeScope(getCandidate().getType(), owner)))
    return failure();
  return success();
}

LogicalResult SourceBranchOp::verify() {
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  auto owner = (*this)->getParentOfType<ModuleOp>();
  if (failed(detail::verifySourceEffectMetadata(getOccurrence(),
                                                getConditionLocation(), owner,
                                                [&] { return emitOpError(); })))
    return failure();
  if (Attribute raw = (*this)->getAttr("ac.loop_continuation")) {
    auto continuation = dyn_cast<BoolAttr>(raw);
    if (!continuation || !continuation.getValue())
      return emitOpError() << "ac.loop_continuation requires BoolAttr true";
  }
  // The control predicate is the staged transaction-surface entry.
  return success();
}

LogicalResult SourceBranchOp::verifyRegions() {
  uint64_t work = 0;
  for (Region *arm : {&getThenRegion(), &getElseRegion()}) {
    if (!arm->hasOneBlock() || arm->front().empty() ||
        arm->front().getNumArguments() || !isa<YieldOp>(arm->front().back()))
      return emitOpError() << "source branch arm requires one block, no "
                              "arguments and final ac.yield";
    auto &yield = arm->front().back();
    if (yield.getNumOperands() != getNumResults())
      return yield.emitOpError()
             << "source branch yield arity must match results";
    // The bounded nominal/Bits result agreement is the staged
    // transaction-surface entry.
    for (Operation &operation : arm->front().without_terminator())
      if (failed(branchPayload(&operation, true, 0, work)))
        return failure();
  }
  return success();
}

LogicalResult SourceMethodOp::verify() {
  if (failed(namedDeclaration(*this, getSymNameAttr())))
    return failure();
  if (Attribute raw = (*this)->getAttr("sym_visibility")) {
    auto visibility = dyn_cast<StringAttr>(raw);
    if (!visibility || visibility.getValue() != "private")
      return emitOpError()
             << "source Method visibility must be absent or private";
  }
  if ((*this)->hasAttr("ac.callable_after") || (*this)->hasAttr("ac.after"))
    return emitOpError() << "source after ordering is retired; independent Rules read current state";
  auto signature = getFunctionType();
  if (getArgumentNames().size() != signature.getNumInputs())
    return emitOpError() << "argument name count must match method signature";
  llvm::StringSet<> names;
  for (Attribute raw : getArgumentNames()) {
    auto name = cast<StringAttr>(raw);
    if (failed(detail::verifyPythonAstIdentifier(
            name.getValue(), true, [&] { return emitOpError(); })))
      return failure();
    if (!names.insert(name.getValue()).second)
      return emitOpError() << "method argument names must be unique";
  }
  auto form = (*this)->getAttrOfType<StringAttr>("ac.return_form");
  auto parameters = (*this)->getAttrOfType<ArrayAttr>("ac.parameters");
  auto results = (*this)->getAttrOfType<ArrayAttr>("ac.result_constraints");
  if (!form || !parameters || !results ||
      parameters.size() != signature.getNumInputs() ||
      results.size() != signature.getNumResults())
    return emitOpError()
           << "method source contract requires ac.parameters, "
              "ac.result_constraints and matching none/single/nonempty tuple "
              "ac.return_form";
  auto lexicalModule = (*this)->getParentOp();
  auto error = [&] { return emitOpError(); };
  auto sourceOwner =
      lexicalModule->getAttrOfType<DictionaryAttr>("source_owner");
  if (failed(detail::verifySourceGuardLocation(
          (*this)->getAttrOfType<DictionaryAttr>("ac.guard_location"),
          sourceOwner, error)))
    return failure();
  for (TypeRange types : {signature.getInputs(), signature.getResults()})
    for (Type type : types) {
      if (!isa<BitsType, StructType, EnumType, TypeParamType, TableType>(type))
        return emitOpError()
               << "method signature requires hardware payload types";
      if (failed(detail::verifyHardwareTypeScope(type, lexicalModule)))
        return failure();
    }
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    if (failed(detail::verifyRequiredSourceParameter(
            raw, cast<StringAttr>(getArgumentNames()[index]), lexicalModule,
            *this, "source method")))
      return failure();
  }
  // The hardware call constraints, the writable-formal proposal contract
  // and the guard-yield predicate are the staged transaction-surface entry.
  SourceMethodView contract{lexicalModule, *this, {}};
  unsigned publicCount = contract.getNumPublicResults();
  if (!((form.getValue() == "none" && publicCount == 0) ||
        (form.getValue() == "single" && publicCount == 1) ||
        (form.getValue() == "tuple" && publicCount != 0)))
    return emitOpError()
           << "method ac.return_form must match its public result count";
  return success();
}

LogicalResult SourceMethodOp::verifyRegions() {
  SmallVector<Type> arguments(getFunctionType().getInputs());
  llvm::append_range(arguments, getCaptures().getTypes());
  if (failed(region(*this, 0, arguments, {})))
    return failure();
  return region(*this, 1, arguments, getFunctionType().getResults());
}

LogicalResult SourceMethodCallOp::verify() {
  if ((*this)->hasAttr("receiver_kind") || (*this)->hasAttr("ancestor_depth")) {
    if ((*this)->hasAttr("ac.reference"))
      return emitOpError() << "relative Method receiver cannot also name a Module reference formal";
    NamedAttrList receiver;
    for (StringRef name : {"receiver_kind", "ancestor_depth", "instance_path"})
      if (Attribute value = (*this)->getAttr(name)) receiver.append(name, value);
    if (failed(verifySourceRelativeReceiver(receiver.getDictionary(getContext()), *this)))
      return failure();
  }
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  if (Attribute raw = (*this)->getAttr("ac.reference")) {
    auto reference = dyn_cast<StringAttr>(raw);
    if (!reference || failed(detail::verifyPythonAstIdentifier(reference.getValue(), true,
                                      [&] { return emitOpError(); })))
      return emitOpError() << "ac.reference requires a Module reference formal name";
  }
  auto callee = getCalleeAttr();
  bool nativeQueue =
      callee.getRootReference().getValue() ==
          "pycircuit.__builtins__.fifo_local" ||
      callee.getRootReference().getValue() ==
          "pycircuit.__builtins__.fifo_downstream_pop" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem_dp" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.byte_mem";
  auto owner = (*this)->getParentOfType<ModuleOp>();
  Attribute rawLocation = (*this)->getAttr("ac.location");
  auto location = dyn_cast_or_null<DictionaryAttr>(rawLocation);
  if (rawLocation && !location)
    return emitOpError() << "method call ac.location requires a SourceSpan";
  if (nativeQueue && !location)
    return emitOpError()
           << "native FIFO calls require ac.location as a SourceSpan";
  if (location &&
      failed(detail::verifySourceEffectMetadata(
          getOccurrence(), location, owner, [&] { return emitOpError(); })))
      return failure();
  // The control predicate is the staged transaction-surface entry.
  if (callee.getNestedReferences().size() != 1)
    return emitOpError() << "method callee must be @Module::@method";
  for (Attribute raw : getInstancePath()) {
    auto name = dyn_cast<StringAttr>(raw);
    if (!name || name.getValue().empty() || name.getValue().contains('\0'))
      return emitOpError() << "method instance path requires nonempty names";
  }
  return success();
}

LogicalResult SourceMethodCallOp::verifySymbolUses(SymbolTableCollection &) {
  if ((*this)->getNumOperands() == 0 || (*this)->getNumRegions() != 0)
    return emitOpError() << "method call requires path operand and no regions";
  if (failed(verifyInvariantsImpl()))
    return failure();
  // The callee/receiver resolution, arity and bound-signature payload
  // agreement, and the formal-owner binding contract are the staged
  // transaction-surface entry.
  return success();
}

LogicalResult SourceTransactionOp::verify() {
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "transaction uses name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getNameAttr())))
    return failure();
  if (failed(
          moduleOccurrence(*this, getDeclaration(), "transaction declaration")))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  auto error = [&] { return emitOpError(); };
  if (failed(detail::verifySourceGuardLocation(
          (*this)->getAttrOfType<DictionaryAttr>("ac.guard_location"),
          owner.getSourceOwner(), error)))
    return failure();
  for (auto other : owner.getBody().front().getOps<SourceTransactionOp>()) {
    if (other == *this)
      continue;
    if (failed(moduleOccurrence(other, other.getDeclaration(),
                                "transaction declaration")))
      return failure();
    if (detail::compareClosedSourceStructure(getOccurrence(),
                                             other.getOccurrence()) == 0)
      return emitOpError() << "transaction registration occurrence must be unique";
  }
  if ((*this)->hasAttr("ac.after") || (*this)->hasAttr("ac.callable_after"))
    return emitOpError() << "source after ordering is retired; independent Rules read current state";
  // The clock/reset control predicates and the guard-yield predicate are
  // the staged transaction-surface entry.
  return success();
}

LogicalResult SourceTransactionOp::verifyRegions() {
  if (failed(region(*this, 0, getCaptures().getTypes(), {})))
    return failure();
  if (failed(region(*this, 1, getCaptures().getTypes(), getResultTypes())))
    return failure();
  // The registration-entry structure (one local transparent Method call)
  // requires the staged Method resolution cluster and runs from the staged
  // transaction-surface row (B2 step-2 design r7 diagnostic-site move).
  return success();
}

Operation *detail::findUnloweredTransaction(Operation *root) {
  SmallVector<Operation *> pending;
  if (root)
    pending.push_back(root);
  while (!pending.empty()) {
    Operation *operation = pending.pop_back_val();
    if (isa<SourceStateOp, SourceStateReadOp, SourceStateWriteOp,
            SourceFormalWriteOp, SourceMethodOp, SourceMethodCallOp, SourceTransactionOp,
            SourceBranchOp, SourceQueuePopOp, SourceQueuePeekOp,
            SourceQueuePushOp>(operation) ||
        (isa<InstanceOp>(operation) &&
         (operation->hasAttr("ac.source_unbound_push") ||
          operation->hasAttr("ac.source_unbound_pop") ||
          operation->hasAttr("ac.payload_constraint") ||
          operation->hasAttr("ac.reference_bindings"))) ||
        (isa<ModuleOp, ModuleImportOp>(operation) &&
         operation->hasAttr("ac.reference_parameters")))
      return operation;
    for (Region &region : llvm::reverse(operation->getRegions()))
      for (Block &block : llvm::reverse(region))
        for (Operation &child : llvm::reverse(block))
          pending.push_back(&child);
  }
  return nullptr;
}

LogicalResult detail::verifyNoUnloweredTransactions(Operation *root) {
  if (Operation *operation = findUnloweredTransaction(root))
    return operation->emitOpError() << "unlowered source transaction operation "
                                       "cannot cross the verified "
                                       "hardware boundary";
  return success();
}

// Relative receiver grammar (declared in SourceMethodAnalysis.h; the native
// SourceMethodCallOp verifier and the staged method analysis share it;
// defined here so the operations archive stays self-sufficient).
LogicalResult verifySourceRelativeReceiver(DictionaryAttr receiver,
                                          Operation *site,
                                          const SourceReceiverProofFees *fees) {
  auto kind = receiver ? receiver.getAs<StringAttr>("receiver_kind") : StringAttr();
  auto depth = receiver ? receiver.getAs<IntegerAttr>("ancestor_depth") : IntegerAttr();
  auto path = receiver ? receiver.getAs<ArrayAttr>("instance_path") : ArrayAttr();
  if (!receiver || receiver.size() != 3 || !kind ||
      kind.getValue() != "reference" || !depth ||
      !depth.getType().isSignlessInteger(64) || depth.getInt() < 0 ||
      depth.getInt() > 128 || !path || path.size() > 128 ||
      path.size() + static_cast<uint64_t>(depth.getInt()) > 128)
    return site->emitOpError() << "relative receiver requires exactly receiver_kind = reference, nonnegative i64 ancestor_depth and a bounded child-name instance_path";
  if (depth.getInt() != 0 || path.size() > 1)
    return site->emitOpError() << "static instance reference cannot bypass lexical scope through ancestors, siblings or descendants";
  if (fees && failed(fees->workStorage(site, 3 + path.size(), 0))) return failure();
  for (Attribute raw : path) {
    auto name = dyn_cast<StringAttr>(raw);
    if (!name)
      return site->emitOpError() << "relative receiver child path requires Python identifiers";
    if (fees && failed(fees->workStorage(site, name.getValue().size(), 0))) return failure();
    if (failed(detail::verifyPythonAstIdentifier(
            name.getValue(), true, [&] { return site->emitOpError(); })))
      return failure();
  }
  return success();
}


} // namespace acir::ac

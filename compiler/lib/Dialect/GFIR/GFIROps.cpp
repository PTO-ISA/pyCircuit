#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Interfaces/ModuleChildInterfaces.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "IR/StateDeclarationContracts.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::gfir {

ParseResult ModuleOp::parse(OpAsmParser &parser, OperationState &result) {
  StringAttr name;
  SmallVector<OpAsmParser::Argument> arguments;
  SmallVector<Type> resultTypes;
  if (parser.parseSymbolName(name, "sym_name", result.attributes) ||
      parser.parseArgumentList(arguments, OpAsmParser::Delimiter::Paren,
                               /*allowType=*/true) ||
      parser.parseArrowTypeList(resultTypes) ||
      parser.parseOptionalAttrDictWithKeyword(result.attributes))
    return failure();
  SmallVector<Type> inputTypes;
  for (const auto &argument : arguments)
    inputTypes.push_back(argument.type);
  result.addAttribute("function_type",
                      TypeAttr::get(parser.getBuilder().getFunctionType(
                          inputTypes, resultTypes)));
  return parser.parseRegion(*result.addRegion(), arguments,
                            /*enableNameShadowing=*/true);
}

void ModuleOp::print(OpAsmPrinter &printer) {
  printer << ' ';
  printer.printSymbolName(getSymName());
  printer << '(';
  llvm::interleaveComma(getBody().front().getArguments(), printer,
                        [&](BlockArgument argument) {
                          printer.printRegionArgument(argument);
                        });
  printer << ") -> (";
  llvm::interleaveComma(getFunctionType().getResults(), printer,
                        [&](Type type) { printer.printType(type); });
  printer << ')';
  printer.printOptionalAttrDictWithKeyword(
      (*this)->getAttrs(), {"sym_name", "function_type"});
  printer << ' ';
  printer.printRegion(getBody(), /*printEntryBlockArgs=*/false,
                      /*printBlockTerminators=*/true);
}

LogicalResult ModuleOp::verify() {
  if (!isa<mlir::ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires package placement");
  if (failed(ac::detail::verifySourceOwner(
          getSourceOwner(), [&] { return emitOpError(); })))
    return failure();
  FunctionType signature = getFunctionType();
  if (getInputNames().size() != signature.getNumInputs() ||
      getOutputNames().size() != signature.getNumResults())
    return emitOpError("port name counts must match the function signature");
  llvm::StringSet<> names;
  for (ArrayAttr list : {getInputNames(), getOutputNames()})
    for (Attribute raw : list) {
      StringRef name = cast<StringAttr>(raw).getValue();
      if (name.empty() || name.contains('\0') || !names.insert(name).second)
        return emitOpError("port names must be nonempty and unique");
    }
  if (!getBody().hasOneBlock())
    return emitOpError("requires one body block");
  if (!llvm::equal(getBody().front().getArgumentTypes(), signature.getInputs()))
    return emitOpError("body arguments must match the function inputs");
  return success();
}

LogicalResult ModuleOp::verifyRegions() {
  Block &body = getBody().front();
  if (body.empty() || !isa<YieldOp>(body.back()))
    return emitOpError("body must end with gfir.yield");
  return success();
}

LogicalResult InstanceOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires direct gfir.module placement");
  return success();
}

LogicalResult CallOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires direct gfir.module placement");
  return success();
}

LogicalResult YieldOp::verify() {
  auto module = dyn_cast_or_null<ModuleOp>((*this)->getParentOp());
  if (!module)
    return emitOpError("requires direct gfir.module placement");
  if (&(*this)->getBlock()->back() != getOperation())
    return emitOpError("must terminate its module body");
  if (!llvm::equal(getValues().getTypes(), module.getFunctionType().getResults()))
    return emitOpError("values must match the module function results");
  return success();
}

LogicalResult RootOp::verify() {
  if (!isa<mlir::ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires package placement");
  if (getKind() != "module" && getKind() != "system")
    return emitOpError("kind must be module or system");
  return success();
}

// GFIR owns concrete state local verification (B1-boundary.md). These native
// verifiers prove local structure, placement and literal-shape facts only.
// The staged package pipeline's bound checks (resolved control widths,
// constraint semantics, initializer closure, pairing inventory) remain
// separate obligations; parsing or mlir::verify alone never authorizes
// publication, refinement or execution.

void StateOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  // Same storage resource identity as the source declaration it replaces.
  effects.emplace_back(MemoryEffects::Allocate::get(),
                       ac::SourceStateResource::get());
  effects.emplace_back(MemoryEffects::Read::get(),
                       ac::SourceStateResource::get());
}

LogicalResult StateOp::verify() {
  if (failed(ac::verifyRoutedLogicalStatePlacement(getOperation())))
    return failure();
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "state uses instance_name; sym_name is unsupported";
  if (failed(ac::verifyNamedLogicalDeclaration(getOperation(),
                                                 getInstanceNameAttr())))
    return failure();
  auto owner = (*this)->getParentOp();
  for (Operation &other : owner->getRegion(0).front())
    if (&other != getOperation() &&
        (isa<ModuleChildOpInterface, LogicalStateOpInterface>(other)) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "state instance_name must be unique across state "
                              "and child instance declarations";
  for (Operation &other : owner->getRegion(0).front()) {
    if (&other == getOperation())
      continue;
    auto peer = dyn_cast<LogicalStateOpInterface>(other);
    if (!peer)
      continue;
    if (ac::detail::compareClosedSourceStructure(
            getOccurrence(), peer.getStateOccurrence()) == 0)
      return emitOpError() << "state declarations require distinct occurrence "
                              "identities within their module";
  }
  if (failed(ac::verifyLiteralControlWidth(getOperation(), getClk(),
                                             "state clock")) ||
      failed(ac::verifyLiteralControlWidth(getOperation(), getRst(),
                                             "state reset")))
    return failure();
  if (!ac::areEquivalentHardwareTypes(getInitial().getType(), getQ().getType()))
    return emitOpError()
           << "initial value and old-Q payload types must match";
  // Test the raw attribute so a present-but-malformed constraint is still
  // rejected downstream; the interface getter alone would silently skip a
  // non-Dictionary attribute. The semantic SymbolicFixedBits and scope
  // checks stay with the bound stage verifier.
  if (Attribute constraint = (*this)->getAttr("ac.payload_constraint"))
    if (failed(ac::verifyPayloadConstraintShape(getOperation(), constraint,
                                                  getQ().getType())))
      return failure();
  // Local same-block pairing: exactly one update binds this Q, deduplicating
  // a Q that also appears as the update's candidate. The package declaration
  // binding independently re-establishes the complete inventory.
  LogicalStateUpdateOpInterface paired;
  for (Operation *user : getQ().getUsers()) {
    auto update = dyn_cast<LogicalStateUpdateOpInterface>(user);
    if (!update || update->getBlock() != (*this)->getBlock() ||
        update->getNumOperands() != 3 ||
        update.getUpdatedStateQ() != getQ())
      continue;
    if (paired && paired != update)
      return emitOpError()
             << "routed logical state requires exactly one same-block update";
    paired = update;
  }
  if (!paired)
    return emitOpError()
           << "routed logical state requires exactly one same-block update";
  return success();
}

void StateUpdateOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Write::get(),
                       ac::SourceStateResource::get());
}

LogicalResult StateUpdateOp::verify() {
  if (failed(ac::verifyRoutedLogicalStatePlacement(getOperation())))
    return failure();
  if (!(*this)->getAttrs().empty())
    return emitOpError() << "state update does not accept attributes";
  // The bound Q must be the direct result of one same-block gfir.state;
  // a source ac.state never satisfies this routed pairing.
  auto state = getState().getDefiningOp<StateOp>();
  if (!state || state->getBlock() != (*this)->getBlock())
    return emitOpError()
           << "state must be the direct result of a same-module gfir.state";
  if (failed(verifyLogicalStateStructure(state.getOperation())))
    return failure();
  if (!ac::areEquivalentHardwareTypes(getState().getType(),
                                      getCandidate().getType()))
    return emitOpError() << "candidate must exactly match the state payload type";
  return ac::verifyLiteralControlWidth(getOperation(), getIntent(),
                                         "state update intent");
}

} // namespace acir::gfir

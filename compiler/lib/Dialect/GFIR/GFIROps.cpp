#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Interfaces/ModuleChildInterfaces.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "IR/StateDeclarationContracts.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::gfir {

namespace {

bool isOrderedOwner(Operation *op) {
  return isa<RuleOp, MethodOp, IfOp>(op);
}

bool isValueOwner(Operation *op) {
  return isa<ModuleOp>(op) || isOrderedOwner(op);
}

ParseResult parseOrderedCallable(OpAsmParser &parser, OperationState &result,
                                 bool bindsCaptures = false) {
  StringAttr name;
  SmallVector<OpAsmParser::Argument> arguments;
  SmallVector<Type> resultTypes;
  if (parser.parseSymbolName(name, "sym_name", result.attributes))
    return failure();
  if (bindsCaptures) {
    if (parser.parseCommaSeparatedList(OpAsmParser::Delimiter::Paren, [&]() -> ParseResult {
          OpAsmParser::Argument argument;
          OpAsmParser::UnresolvedOperand capture;
          if (parser.parseArgument(argument) || parser.parseEqual() ||
              parser.parseOperand(capture) || parser.parseColonType(argument.type) ||
              parser.resolveOperand(capture, argument.type, result.operands))
            return failure();
          arguments.push_back(argument);
          return success();
        }))
      return failure();
  } else if (parser.parseArgumentList(arguments, OpAsmParser::Delimiter::Paren,
                                       /*allowType=*/true)) {
    return failure();
  }
  if (parser.parseArrowTypeList(resultTypes) ||
      parser.parseOptionalAttrDictWithKeyword(result.attributes))
    return failure();
  SmallVector<Type> inputs;
  for (const auto &argument : arguments)
    inputs.push_back(argument.type);
  result.addAttribute("function_type", TypeAttr::get(
      parser.getBuilder().getFunctionType(inputs, resultTypes)));
  return parser.parseRegion(*result.addRegion(), arguments,
                            /*enableNameShadowing=*/true);
}

void printOrderedCallable(OpAsmPrinter &printer, Operation *op,
                          StringRef name, FunctionType type, Region &body,
                          ValueRange captures = {}) {
  printer << ' ';
  printer.printSymbolName(name);
  printer << '(';
  llvm::interleaveComma(body.front().getArguments(), printer,
                        [&](BlockArgument argument) {
                          if (argument.getArgNumber() < captures.size()) {
                            printer.printOperand(argument);
                            printer << " = ";
                            printer.printOperand(captures[argument.getArgNumber()]);
                            printer << " : " << argument.getType();
                          } else {
                            printer.printRegionArgument(argument);
                          }
                        });
  printer << ") -> (";
  llvm::interleaveComma(type.getResults(), printer,
                        [&](Type result) { printer.printType(result); });
  printer << ')';
  printer.printOptionalAttrDictWithKeyword(
      op->getAttrs(), {"sym_name", "function_type"});
  printer << ' ';
  printer.printRegion(body, /*printEntryBlockArgs=*/false,
                      /*printBlockTerminators=*/true);
}

LogicalResult verifyOrderedCallable(Operation *op, FunctionType type,
                                    Region &body) {
  if (!isa<ModuleOp>(op->getParentOp()))
    return op->emitOpError("requires direct gfir.module placement");
  if (!body.hasOneBlock())
    return op->emitOpError("requires one ordered body block");
  if (!llvm::equal(body.front().getArgumentTypes(), type.getInputs()))
    return op->emitOpError("body arguments must match the function inputs");
  return success();
}

LogicalResult verifyOrderedTerminator(Operation *op, Region &body) {
  if (body.front().empty() || !isa<YieldOp>(body.front().back()))
    return op->emitOpError("ordered body must end with gfir.yield");
  return success();
}

} // namespace

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
  if (!isValueOwner((*this)->getParentOp()))
    return emitOpError("requires a module graph or ordered body");
  if (!getReceiverAttr() && !getInterfaceAttr())
    return emitOpError("self call requires a named interface");
  if (getInterfaceAttr() && getInterfaceAttr().getValue().empty())
    return emitOpError("interface name must be nonempty");
  return success();
}

LogicalResult YieldOp::verify() {
  auto *owner = (*this)->getParentOp();
  if (!isValueOwner(owner))
    return emitOpError("requires a module graph or ordered body");
  if (&(*this)->getBlock()->back() != getOperation())
    return emitOpError("must terminate its body");
  if (auto branch = dyn_cast<IfOp>(owner)) {
    if (!llvm::equal(getValues().getTypes(), branch.getResultTypes()))
      return emitOpError("values must match the conditional results");
    return success();
  }
  auto type = cast<FunctionType>(
      owner->getAttrOfType<TypeAttr>("function_type").getValue());
  if (!llvm::equal(getValues().getTypes(), type.getResults()))
    return emitOpError("values must match the function results");
  return success();
}

ParseResult CallOp::parse(OpAsmParser &parser, OperationState &result) {
  if (failed(parser.parseOptionalKeyword("self"))) {
    FlatSymbolRefAttr receiver;
    if (parser.parseAttribute(receiver, "receiver", result.attributes))
      return failure();
  }
  if (succeeded(parser.parseOptionalLSquare())) {
    StringAttr interface;
    if (parser.parseAttribute(interface, "interface", result.attributes) ||
        parser.parseRSquare())
      return failure();
  }
  SmallVector<OpAsmParser::UnresolvedOperand> inputs;
  FunctionType type;
  if (parser.parseOperandList(inputs, OpAsmParser::Delimiter::Paren) ||
      parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseColonType(type) ||
      parser.resolveOperands(inputs, type.getInputs(), parser.getCurrentLocation(),
                              result.operands))
    return failure();
  result.addTypes(type.getResults());
  return success();
}

void CallOp::print(OpAsmPrinter &printer) {
  printer << ' ';
  if (auto receiver = getReceiverAttr())
    printer.printAttributeWithoutType(receiver);
  else
    printer << "self";
  if (auto interface = getInterfaceAttr()) {
    printer << '[';
    printer.printAttributeWithoutType(interface);
    printer << ']';
  }
  printer << '(' << getInputs() << ')';
  printer.printOptionalAttrDict((*this)->getAttrs(), {"receiver", "interface"});
  printer << " : ";
  printer.printFunctionalType(getInputs().getTypes(), getOutputs().getTypes());
}

void CallOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  // A local interface may access state or resources. Closed-package analysis
  // resolves the actual receiver and effects before scheduling or lowering.
  SideEffects::Resource *resources[] = {ac::SourceStateResource::get(),
                                        ac::SourceQueueResource::get()};
  for (auto *resource : resources) {
    effects.emplace_back(MemoryEffects::Read::get(), resource);
    effects.emplace_back(MemoryEffects::Write::get(), resource);
  }
  effects.emplace_back(MemoryEffects::Write::get(),
                       ac::SourceCheckResource::get());
  effects.emplace_back(MemoryEffects::Write::get(),
                       ac::SourceObservationResource::get());
}

LogicalResult PrimitiveOp::verify() {
  if (!isa<mlir::ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires package placement");
  if (getNativeKind().empty() || getVersionAttr().getInt() <= 0)
    return emitOpError("native kind and positive version are required");
  return success();
}

LogicalResult VarOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()))
    return emitOpError("requires direct gfir.module placement");
  return success();
}

void VarOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Allocate::get(),
                       ac::SourceStateResource::get());
}

LogicalResult ReadOp::verify() {
  if (!isValueOwner((*this)->getParentOp()))
    return emitOpError("requires a module graph or ordered body");
  return success();
}

void ReadOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Read::get(),
                       ac::SourceStateResource::get());
}

ParseResult WriteOp::parse(OpAsmParser &parser, OperationState &result) {
  FlatSymbolRefAttr state;
  OpAsmParser::UnresolvedOperand value, index;
  Type valueType, indexType;
  if (parser.parseAttribute(state, "state", result.attributes))
    return failure();
  SmallVector<Attribute> fields;
  bool indexed = false;
  while (succeeded(parser.parseOptionalLSquare())) {
    std::string field;
    if (succeeded(parser.parseOptionalString(&field))) {
      if (field.empty())
        return parser.emitError(parser.getCurrentLocation(), "field name must be nonempty");
      if (parser.parseRSquare())
        return failure();
      fields.push_back(parser.getBuilder().getStringAttr(field));
      continue;
    }
    if (parser.parseOperand(index) || parser.parseColonType(indexType) ||
        parser.parseRSquare())
      return failure();
    indexed = true;
    break;
  }
  if (!fields.empty())
    result.addAttribute("fields", parser.getBuilder().getArrayAttr(fields));
  if (parser.parseComma() || parser.parseOperand(value) ||
      parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseColonType(valueType) ||
      parser.resolveOperand(value, valueType, result.operands))
    return failure();
  if (indexed && parser.resolveOperand(index, indexType, result.operands))
    return failure();
  return success();
}

void WriteOp::print(OpAsmPrinter &printer) {
  printer << ' ';
  printer.printAttributeWithoutType(getStateAttr());
  if (auto fields = getFieldsAttr())
    for (Attribute field : fields) {
      printer << '[';
      printer.printAttributeWithoutType(field);
      printer << ']';
    }
  if (auto index = getIndex())
    printer << '[' << index << " : " << index.getType() << ']';
  printer << ", " << getValue();
  printer.printOptionalAttrDict((*this)->getAttrs(), {"state", "fields"});
  printer << " : " << getValue().getType();
}

LogicalResult WriteOp::verify() {
  if (!isOrderedOwner((*this)->getParentOp()))
    return emitOpError("requires an ordered Rule or Method body");
  if (getIndex() && !isa<ac::BitsType>(getIndex().getType()))
    return emitOpError("dynamic table index must have a Bits type");
  if (auto fields = getFieldsAttr())
    if (fields.empty() || llvm::any_of(fields, [](Attribute field) {
          return cast<StringAttr>(field).getValue().empty();
        }))
      return emitOpError("field path must contain nonempty names");
  return success();
}

void WriteOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Write::get(),
                       ac::SourceStateResource::get());
}

ParseResult RuleOp::parse(OpAsmParser &parser, OperationState &result) {
  return parseOrderedCallable(parser, result, /*bindsCaptures=*/true);
}

void RuleOp::print(OpAsmPrinter &printer) {
  printOrderedCallable(printer, *this, getSymName(), getFunctionType(), getBody(),
                       getCaptures());
}

LogicalResult RuleOp::verify() {
  if (!llvm::equal(getCaptures().getTypes(), getFunctionType().getInputs()))
    return emitOpError("capture types must match the Rule inputs");
  if (llvm::any_of(getCaptures(), [&](Value capture) {
        return capture.getParentBlock() != (*this)->getBlock();
      }))
    return emitOpError("captures must be values in the owning Module graph");
  return verifyOrderedCallable(*this, getFunctionType(), getBody());
}

LogicalResult RuleOp::verifyRegions() {
  return verifyOrderedTerminator(*this, getBody());
}

LogicalResult ResultOp::verify() {
  Operation *owner = (*this)->getParentOp();
  if (!isa<ModuleOp>(owner))
    return emitOpError("requires direct gfir.module placement");
  auto rule = dyn_cast_or_null<RuleOp>(
      SymbolTable::lookupSymbolIn(owner, getRuleAttr()));
  if (!rule)
    return emitOpError("requires a local autonomous Rule");
  int64_t ordinal = getOrdinalAttr().getInt();
  if (ordinal < 0 || uint64_t(ordinal) >= rule.getFunctionType().getNumResults())
    return emitOpError("result ordinal is outside the Rule signature");
  if (getValue().getType() != rule.getFunctionType().getResult(ordinal))
    return emitOpError("result type must match the Rule signature");
  return success();
}

ParseResult MethodOp::parse(OpAsmParser &parser, OperationState &result) {
  return parseOrderedCallable(parser, result);
}

void MethodOp::print(OpAsmPrinter &printer) {
  printOrderedCallable(printer, *this, getSymName(), getFunctionType(), getBody());
}

LogicalResult MethodOp::verify() {
  return verifyOrderedCallable(*this, getFunctionType(), getBody());
}

LogicalResult MethodOp::verifyRegions() {
  return verifyOrderedTerminator(*this, getBody());
}

ParseResult IfOp::parse(OpAsmParser &parser, OperationState &result) {
  OpAsmParser::UnresolvedOperand predicate;
  Type type;
  if (parser.parseOperand(predicate) ||
      parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseColonType(type) || parser.parseArrowTypeList(result.types) ||
      parser.resolveOperand(predicate, type, result.operands) ||
      parser.parseRegion(*result.addRegion()) || parser.parseKeyword("else"))
    return failure();
  return parser.parseRegion(*result.addRegion());
}

void IfOp::print(OpAsmPrinter &printer) {
  printer << ' ' << getPredicate();
  printer.printOptionalAttrDict((*this)->getAttrs());
  printer << " : " << getPredicate().getType() << " -> (";
  llvm::interleaveComma(getResultTypes(), printer);
  printer << ") ";
  printer.printRegion(getThenBody(), false, true);
  printer << " else ";
  printer.printRegion(getElseBody(), false, true);
}

LogicalResult IfOp::verify() {
  if (!isOrderedOwner((*this)->getParentOp()))
    return emitOpError("requires an ordered body");
  if (!isa<ac::BitsType>(getPredicate().getType()))
    return emitOpError("condition requires a Bits type");
  for (Region *arm : {&getThenBody(), &getElseBody()})
    if (!arm->hasOneBlock() || arm->front().getNumArguments() != 0)
      return emitOpError("each arm requires one block without arguments");
  return success();
}

LogicalResult IfOp::verifyRegions() {
  if (failed(verifyOrderedTerminator(*this, getThenBody())))
    return failure();
  return verifyOrderedTerminator(*this, getElseBody());
}

LogicalResult GuardOp::verify() {
  if (!isOrderedOwner((*this)->getParentOp()))
    return emitOpError("requires an ordered body");
  if (!isa<ac::BitsType>(getPredicate().getType()))
    return emitOpError("guard requires a Bits predicate");
  return success();
}

void GuardOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Write::get(), ac::SourceCheckResource::get());
}

ParseResult RootOp::parse(OpAsmParser &parser, OperationState &result) {
  FlatSymbolRefAttr callee;
  StringAttr kind;
  if (parser.parseAttribute(callee, "callee", result.attributes) ||
      parser.parseKeyword("kind") ||
      parser.parseAttribute(kind, "kind", result.attributes) ||
      parser.parseOptionalAttrDict(result.attributes))
    return failure();
  for (auto [qualified, local] : {
           std::pair{"ac.system_samples", "system_samples"},
           std::pair{"ac.execution_policy", "execution_policy"}}) {
    if (Attribute mapped = result.attributes.erase(qualified)) {
      if (result.attributes.get(local))
        return parser.emitError(parser.getCurrentLocation(), "duplicate root attribute ") << local;
      result.attributes.set(local, mapped);
    }
  }
  return success();
}

void RootOp::print(OpAsmPrinter &printer) {
  printer << ' ';
  printer.printAttributeWithoutType(getCalleeAttr());
  printer << " kind ";
  printer.printAttributeWithoutType(getKindAttr());
  NamedAttrList attributes((*this)->getAttrs());
  attributes.erase("callee");
  attributes.erase("kind");
  if (Attribute samples = attributes.erase("system_samples"))
    attributes.set("ac.system_samples", samples);
  printer.printOptionalAttrDict(attributes.getAttrs());
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

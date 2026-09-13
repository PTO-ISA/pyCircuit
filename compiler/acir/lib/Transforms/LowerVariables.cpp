#include "acir/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir {
namespace {

unsigned canonicalIndexWidth(uint64_t extent) {
  return std::max<unsigned>(1, llvm::Log2_64_Ceil(extent));
}

ac::VarConstantOp createZeroIndex(OpBuilder &builder, Location location,
                                  unsigned width = 1) {
  Type indexType = IntegerType::get(builder.getContext(), width);
  OperationState state(location, ac::VarConstantOp::getOperationName());
  state.addTypes(ac::VarType::get(builder.getContext(), indexType));
  state.addAttribute("value", builder.getIntegerAttr(indexType, 0));
  return cast<ac::VarConstantOp>(builder.create(state));
}

FailureOr<uint64_t> flattenedEntries(ArrayRef<int64_t> shape) {
  uint64_t result = 1;
  for (int64_t extent : shape) {
    if (extent <= 0 ||
        result > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) /
                     static_cast<uint64_t>(extent))
      return failure();
    result *= static_cast<uint64_t>(extent);
  }
  return result;
}

std::string canonicalTableSchemaId(Type entryType, ArrayRef<int64_t> shape) {
  std::string printedType;
  llvm::raw_string_ostream typeStream(printedType);
  entryType.print(typeStream);
  typeStream.flush();
  std::string preimage;
  llvm::raw_string_ostream stream(preimage);
  stream << R"({"entry":")" << printedType
         << R"(","layout":"row_major","layout_version":1,"shape":[)";
  llvm::interleave(shape, stream, ",");
  stream << "]}";
  stream.flush();
  llvm::SHA256 sha;
  sha.update(preimage);
  return "sha256:" + llvm::toHex(sha.final(), /*LowerCase=*/true);
}

ac::VarDeclOp resolveVariable(Operation *operation,
                              FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp()) {
    if (ancestor->getNumRegions() != 1 || !ancestor->getRegion(0).hasOneBlock())
      continue;
    for (ac::VarDeclOp variable :
         ancestor->getRegion(0).front().getOps<ac::VarDeclOp>())
      if (variable.getSymName() == reference.getValue())
        return variable;
  }
  return {};
}

FailureOr<ArrayAttr> completeWriteFields(OpBuilder &builder,
                                         ac::VarDeclOp variable) {
  auto structure = dyn_cast<ac::StructType>(variable.getValueType());
  if (!structure)
    return builder.getStrArrayAttr({"$entry"});
  Operation *declaration =
      SymbolTable::lookupNearestSymbolFrom(variable, structure.getName());
  auto fields = declaration ? declaration->getAttrOfType<ArrayAttr>("fields")
                            : ArrayAttr();
  if (!fields)
    return failure();
  SmallVector<StringRef> names;
  for (Attribute rawField : fields) {
    auto field = dyn_cast<DictionaryAttr>(rawField);
    auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
    if (!name)
      return failure();
    names.push_back(name.getValue());
  }
  return builder.getStrArrayAttr(names);
}

void replaceYield(Operation *yield, StringRef operationName) {
  OpBuilder builder(yield);
  OperationState state(yield->getLoc(), operationName);
  state.addOperands(yield->getOperands());
  state.addAttributes(yield->getAttrs());
  builder.create(state);
  yield->erase();
}

LogicalResult lowerVariableState(ModuleOp model) {
  SmallVector<ac::VarReadOp> reads;
  SmallVector<ac::VarReadElementOp> elementReads;
  SmallVector<ac::VarAssignOp> assignments;
  SmallVector<ac::VarAssignElementOp> elementAssignments;
  SmallVector<ac::VarMatchOp> matches;
  SmallVector<ac::VarChooseOp> choices;
  SmallVector<ac::VarDeclOp> declarations;
  model.walk([&](ac::VarReadOp operation) { reads.push_back(operation); });
  model.walk([&](ac::VarReadElementOp operation) {
    elementReads.push_back(operation);
  });
  model.walk(
      [&](ac::VarAssignOp operation) { assignments.push_back(operation); });
  model.walk([&](ac::VarAssignElementOp operation) {
    elementAssignments.push_back(operation);
  });
  model.walk([&](ac::VarMatchOp operation) { matches.push_back(operation); });
  model.walk([&](ac::VarChooseOp operation) { choices.push_back(operation); });
  model.walk(
      [&](ac::VarDeclOp operation) { declarations.push_back(operation); });

  for (ac::VarMatchOp match : matches) {
    OpBuilder builder(match);
    ac::VarDeclOp variable = resolveVariable(match, match.getVariableAttr());
    if (!variable)
      return match.emitOpError("persistent ac.var declaration is missing");
    OperationState state(match.getLoc(), ac::TableMatchOp::getOperationName());
    if (match.getRow()) {
      ArrayRef<int64_t> shape = variable.getShapeAttr().asArrayRef();
      if (shape.size() != 2)
        return match.emitOpError(
            "row projection requires a rank-two shaped ac.var");
      FailureOr<uint64_t> entries = flattenedEntries(shape);
      if (failed(entries))
        return match.emitOpError("row projection shape product overflows");
      ac::VarConstantOp zero = createZeroIndex(
          builder, match.getLoc(), canonicalIndexWidth(shape.back()));
      OperationState indexState(match.getLoc(),
                                ac::TableIndexOp::getOperationName());
      indexState.addOperands({match.getRow(), zero.getResult()});
      indexState.addTypes(
          ac::VarType::get(builder.getContext(),
                           IntegerType::get(builder.getContext(),
                                            canonicalIndexWidth(*entries))));
      indexState.addAttribute("table", match.getVariableAttr());
      Operation *base = builder.create(indexState);
      state.addOperands(base->getResult(0));
      state.addAttribute("domain_axes", builder.getDenseI64ArrayAttr({1}));
      state.addAttribute("domain_shape",
                         builder.getDenseI64ArrayAttr({shape.back()}));
      state.addAttribute("domain_strides", builder.getDenseI64ArrayAttr({1}));
      state.addAttribute("domain_offset", builder.getI64IntegerAttr(0));
    }
    state.addTypes(match.getMask().getType());
    NamedAttrList attributes(match->getAttrs());
    attributes.erase("variable");
    attributes.set("table", match.getVariableAttr());
    state.addAttributes(attributes);
    state.addRegion();
    Operation *replacement = builder.create(state);
    replacement->getRegion(0).takeBody(match.getPredicate());
    replaceYield(replacement->getRegion(0).front().getTerminator(),
                 ac::TableMatchYieldOp::getOperationName());
    match.getMask().replaceAllUsesWith(replacement->getResult(0));
    match.erase();
  }

  for (ac::VarChooseOp choice : choices) {
    OpBuilder builder(choice);
    OperationState state(choice.getLoc(),
                         ac::TableChooseOp::getOperationName());
    state.addOperands(choice.getMask());
    state.addTypes({choice.getIndex().getType(), choice.getValid().getType()});
    NamedAttrList attributes(choice->getAttrs());
    attributes.erase("variable");
    attributes.set("table", choice.getVariableAttr());
    const ac::TableSelectionPolicy policy =
        choice.getPolicy() == "first"
            ? ac::TableSelectionPolicy::First
            : (choice.getPolicy() == "min" ? ac::TableSelectionPolicy::Min
                                            : ac::TableSelectionPolicy::Max);
    attributes.set("policy", ac::TableSelectionPolicyAttr::get(
                                 model.getContext(), policy));
    if (policy == ac::TableSelectionPolicy::Min ||
        policy == ac::TableSelectionPolicy::Max)
      attributes.set("key_ordering", ac::TableKeyOrderingAttr::get(
                                         model.getContext(),
                                         ac::TableKeyOrdering::Unsigned));
    auto query = choice->getAttrOfType<StringAttr>("ac.query");
    if (!query || query.getValue().empty())
      return choice.emitOpError(
          "storage selection requires stable ac.query identity");
    attributes.set(
        "stable_id",
        builder.getStringAttr((choice.getVariable() + "/" + query.getValue())
                                  .str()));
    state.addAttributes(attributes);
    state.addRegion();
    Operation *replacement = builder.create(state);
    replacement->getRegion(0).takeBody(choice.getKey());
    if (!replacement->getRegion(0).empty())
      replaceYield(replacement->getRegion(0).front().getTerminator(),
                   ac::TableChooseYieldOp::getOperationName());
    choice.getIndex().replaceAllUsesWith(replacement->getResult(0));
    choice.getValid().replaceAllUsesWith(replacement->getResult(1));
    choice.erase();
  }

  for (ac::VarReadOp read : reads) {
    OpBuilder builder(read);
    ac::VarConstantOp index = createZeroIndex(builder, read.getLoc());
    OperationState state(read.getLoc(), ac::TableGetOp::getOperationName());
    state.addOperands(index.getResult());
    state.addTypes(read.getResult().getType());
    state.addAttribute("table", read.getVariableAttr());
    Operation *replacement = builder.create(state);
    read.getResult().replaceAllUsesWith(replacement->getResult(0));
    read.erase();
  }

  for (ac::VarReadElementOp read : elementReads) {
    OpBuilder builder(read);
    OperationState state(read.getLoc(), ac::TableGetOp::getOperationName());
    state.addOperands(read.getIndex());
    state.addTypes(read.getResult().getType());
    state.addAttribute("table", read.getVariableAttr());
    Operation *replacement = builder.create(state);
    read.getResult().replaceAllUsesWith(replacement->getResult(0));
    read.erase();
  }

  for (ac::VarAssignOp assignment : assignments) {
    OpBuilder builder(assignment);
    ac::VarDeclOp variable =
        resolveVariable(assignment, assignment.getVariableAttr());
    if (!variable)
      return assignment.emitOpError("persistent ac.var declaration is missing");
    FailureOr<ArrayAttr> writeFields = completeWriteFields(builder, variable);
    if (failed(writeFields))
      return assignment.emitOpError(
          "persistent struct field schema is unresolved");
    ac::VarConstantOp index = createZeroIndex(builder, assignment.getLoc());
    OperationState state(assignment.getLoc(),
                         ac::TableProposeOp::getOperationName());
    state.addOperands({index.getResult(), assignment.getValue()});
    if (assignment.getWhen())
      state.addOperands(assignment.getWhen());
    state.addAttribute("table", assignment.getVariableAttr());
    state.addAttribute("mode", builder.getStringAttr("replace"));
    state.addAttribute("write_fields", *writeFields);
    builder.create(state);
    assignment.erase();
  }

  for (ac::VarAssignElementOp assignment : elementAssignments) {
    OpBuilder builder(assignment);
    ac::VarDeclOp variable =
        resolveVariable(assignment, assignment.getVariableAttr());
    if (!variable)
      return assignment.emitOpError("persistent ac.var declaration is missing");
    FailureOr<ArrayAttr> writeFields = completeWriteFields(builder, variable);
    if (failed(writeFields))
      return assignment.emitOpError(
          "persistent struct field schema is unresolved");
    OperationState state(assignment.getLoc(),
                         ac::TableProposeOp::getOperationName());
    state.addOperands({assignment.getIndex(), assignment.getValue()});
    if (assignment.getWhen())
      state.addOperands(assignment.getWhen());
    state.addAttribute("table", assignment.getVariableAttr());
    state.addAttribute("mode", builder.getStringAttr("replace"));
    state.addAttribute("write_fields", *writeFields);
    builder.create(state);
    assignment.erase();
  }

  for (ac::VarDeclOp declaration : declarations) {
    auto integer = dyn_cast<IntegerAttr>(declaration.getInit());
    if (!integer || !integer.getValue().isZero())
      return declaration.emitOpError(
          "first ac.var storage-selection slice requires integer zero init");
    if (!isa<IntegerType, ac::EnumType, ac::StructType>(
            declaration.getValueType()))
      return declaration.emitOpError("first ac.var storage-selection slice "
                                     "requires scalar, enum, or flat struct");
    int64_t entries = 1;
    SmallVector<int64_t> typedShape;
    if (auto shape = declaration.getShapeAttr()) {
      FailureOr<uint64_t> product = flattenedEntries(shape.asArrayRef());
      if (failed(product))
        return declaration.emitOpError(
            "storage selection ac.var shape product overflows");
      entries = static_cast<int64_t>(*product);
      if (shape.asArrayRef().size() > 1)
        typedShape.append(shape.asArrayRef().begin(), shape.asArrayRef().end());
    }
    OpBuilder builder(declaration);
    OperationState state(declaration.getLoc(), ac::TableOp::getOperationName());
    state.addAttribute(SymbolTable::getSymbolAttrName(),
                       declaration.getSymNameAttr());
    state.addAttribute("entry_type", TypeAttr::get(declaration.getValueType()));
    state.addAttribute("entries", builder.getI64IntegerAttr(entries));
    state.addAttribute("init", builder.getI64IntegerAttr(0));
    state.addAttribute("owner", declaration.getOwnerAttr());
    std::string stableId = "table/";
    if (declaration.getOwner() != "/") {
      stableId.append(declaration.getOwner().drop_front());
      stableId.push_back('/');
    }
    stableId.append(declaration.getSymName());
    state.addAttribute("stable_id", builder.getStringAttr(stableId));
    if (!typedShape.empty()) {
      SmallVector<int64_t> axisWidths;
      for (int64_t extent : typedShape)
        axisWidths.push_back(canonicalIndexWidth(extent));
      state.addAttribute("shape", builder.getDenseI64ArrayAttr(typedShape));
      state.addAttribute("axis_widths",
                         builder.getDenseI64ArrayAttr(axisWidths));
      state.addAttribute("layout", builder.getStringAttr("row_major"));
      state.addAttribute("layout_version", builder.getI64IntegerAttr(1));
      state.addAttribute("schema_id",
                         builder.getStringAttr(canonicalTableSchemaId(
                             declaration.getValueType(), typedShape)));
    }
    builder.create(state);
    declaration.erase();
  }
  return success();
}

#define GEN_PASS_DEF_LOWERVARIABLESTATEPASS
#include "acir/Transforms/Passes.h.inc"

struct LowerVariableStatePass
    : impl::LowerVariableStatePassBase<LowerVariableStatePass> {
  void runOnOperation() override {
    if (failed(lowerVariableState(getOperation())))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> createLowerVariableStatePass() {
  return std::make_unique<LowerVariableStatePass>();
}

} // namespace acir

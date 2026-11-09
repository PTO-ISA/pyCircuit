#include "pycircuit/Dialect/HWIR/HWIRDialect.h"

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSet.h"

#include <limits>
#include <optional>

using namespace mlir;

namespace acir::hwir {
namespace {

bool isHardwareType(Type type) {
  return isa<ac::BitsType, ac::StructType, ac::EnumType, ac::TypeParamType,
             ac::TableType>(type);
}

LogicalResult verifyHardwareTypes(Operation *operation, TypeRange types,
                                  StringRef description) {
  for (Type type : types)
    if (!isHardwareType(type))
      return operation->emitOpError()
             << description << " requires a hardware payload type";
  return success();
}

std::optional<llvm::APSInt> literalInteger(ac::StaticExprAttr expression) {
  if (!expression)
    return std::nullopt;
  DictionaryAttr tree = expression.getTree();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto integer =
      value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !valueKind ||
      valueKind.getValue() != "integer" || !integer)
    return std::nullopt;
  return llvm::APSInt(integer.getCanonicalValue());
}

bool isOneBit(Type type) {
  auto bits = dyn_cast<ac::BitsType>(type);
  if (!bits)
    return false;
  auto width = literalInteger(bits.getWidth());
  return width && !width->isNegative() && width->getLimitedValue() == 1;
}

std::optional<uint64_t> literalWidth(Type type) {
  auto bits = dyn_cast<ac::BitsType>(type);
  if (!bits)
    return std::nullopt;
  auto width = literalInteger(bits.getWidth());
  if (!width || width->isNegative() || width->isZero() ||
      width->getActiveBits() > 64)
    return std::nullopt;
  return width->getZExtValue();
}

std::optional<SmallVector<uint64_t>> literalShape(ArrayAttr shape) {
  SmallVector<uint64_t> result;
  for (Attribute raw : shape) {
    auto value = literalInteger(dyn_cast<ac::StaticExprAttr>(raw));
    if (!value)
      return std::nullopt;
    if (value->isNegative() || value->isZero() ||
        value->getActiveBits() > 63)
      return std::nullopt;
    result.push_back(value->getZExtValue());
  }
  return result;
}

std::optional<uint64_t> literalElementCount(ArrayAttr shape) {
  auto dimensions = literalShape(shape);
  if (!dimensions)
    return std::nullopt;
  uint64_t count = 1;
  for (uint64_t dimension : *dimensions) {
    if (count > uint64_t(std::numeric_limits<int64_t>::max()) / dimension)
      return std::nullopt;
    count *= dimension;
  }
  return count;
}

uint64_t indexWidth(uint64_t count) {
  return std::max<uint64_t>(1, llvm::APInt(64, count - 1).getActiveBits());
}

uint64_t countWidth(uint64_t count) {
  return std::max<uint64_t>(1, llvm::APInt(64, count).getActiveBits());
}

bool hasExactlyKeys(DictionaryAttr dictionary, ArrayRef<StringRef> names) {
  if (!dictionary || dictionary.size() != names.size())
    return false;
  return llvm::all_of(names,
                      [&](StringRef name) { return dictionary.get(name); });
}

bool isValidStaticExpression(Attribute attribute) {
  return isa_and_nonnull<ac::StaticExprAttr>(attribute);
}

LogicalResult verifyStaticVector(Operation *operation, Attribute attribute,
                                 size_t expected, StringRef description,
                                 bool allowDifferentRank = false) {
  auto values = dyn_cast_or_null<ArrayAttr>(attribute);
  if (!values || values.empty() ||
      (!allowDifferentRank && values.size() != expected))
    return operation->emitOpError() << description << " rank mismatch";
  for (Attribute value : values)
    if (!isValidStaticExpression(value))
      return operation->emitOpError()
             << description << " requires StaticExpr values";
  return success();
}

bool isPathPrefix(ArrayAttr left, ArrayAttr right) {
  return left.size() <= right.size() &&
         std::equal(left.begin(), left.end(), right.begin());
}

FailureOr<Type> resolveFieldPath(Operation *operation, Type type,
                                 ArrayAttr path) {
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  ac::HardwareAnalysis analysis(package);
  for (Attribute raw : path) {
    while (auto table = dyn_cast<ac::TableType>(type))
      type = table.getElementType();
    auto name = dyn_cast<StringAttr>(raw);
    auto record = dyn_cast<ac::StructType>(type);
    if (!name || name.getValue().empty() || !record)
      return operation->emitOpError()
             << "field path requires existing struct fields";
    auto declaration = analysis.lookupStruct(record);
    if (!declaration)
      return operation->emitOpError()
             << "field path names an unresolved struct";
    bool found = false;
    for (Attribute rawField : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      if (field.getAs<StringAttr>("name") != name)
        continue;
      type = field.getAs<TypeAttr>("type").getValue();
      found = true;
      break;
    }
    if (!found)
      return operation->emitOpError() << "field path names unknown field '"
                                      << name.getValue() << "'";
  }
  return type;
}

FailureOr<SmallVector<ArrayAttr>> getLeafPaths(Operation *operation, Type type,
                                               ArrayAttr prefix = {}) {
  if (!prefix)
    prefix = ArrayAttr::get(operation->getContext(), {});
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  ac::HardwareAnalysis analysis(package);
  while (auto table = dyn_cast<ac::TableType>(type))
    type = table.getElementType();
  if (auto record = dyn_cast<ac::StructType>(type)) {
    auto declaration = analysis.lookupStruct(record);
    if (!declaration)
      return operation->emitOpError()
             << "dependency path names an unresolved struct";
    SmallVector<ArrayAttr> result;
    for (Attribute rawField : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      SmallVector<Attribute> components;
      if (prefix)
        llvm::append_range(components, prefix);
      components.push_back(field.getAs<StringAttr>("name"));
      auto children = getLeafPaths(
          operation, field.getAs<TypeAttr>("type").getValue(),
          ArrayAttr::get(operation->getContext(), components));
      if (failed(children))
        return failure();
      llvm::append_range(result, *children);
    }
    return result;
  }
  return SmallVector<ArrayAttr>{prefix};
}

struct DependencyEndpoint {
  unsigned port = 0;
  ArrayAttr path;
};

FailureOr<DependencyEndpoint>
parseDependencyEndpoint(Operation *operation, Attribute raw, TypeRange ports,
                        StringRef description) {
  auto endpoint = dyn_cast_or_null<DictionaryAttr>(raw);
  auto port = endpoint ? endpoint.getAs<IntegerAttr>("port") : IntegerAttr();
  auto path = endpoint ? endpoint.getAs<ArrayAttr>("path") : ArrayAttr();
  if (!endpoint || endpoint.size() != 2 || !port || !path ||
      port.getValue().isNegative() || port.getValue().getActiveBits() > 32 ||
      port.getValue().getZExtValue() >= ports.size())
    return operation->emitOpError()
           << description << " requires a valid port and field path";
  for (Attribute component : path) {
    auto name = dyn_cast<StringAttr>(component);
    if (!name || name.getValue().empty())
      return operation->emitOpError()
             << description << " path requires nonempty field names";
  }
  unsigned ordinal = port.getValue().getZExtValue();
  if (failed(resolveFieldPath(operation, ports[ordinal], path)))
    return failure();
  return DependencyEndpoint{ordinal, path};
}

bool sameEndpoint(const DependencyEndpoint &left,
                  const DependencyEndpoint &right) {
  return left.port == right.port && left.path == right.path;
}

LogicalResult verifyOutputDependencies(ModuleOp module) {
  ArrayAttr summary =
      module->getAttrOfType<ArrayAttr>("hwir.output_dependencies");
  if (!summary)
    return module.emitOpError()
           << "HWIR v2 requires output dependency facts";
  FunctionType signature = module.getFunctionType();
  SmallVector<DependencyEndpoint> observed;
  for (Attribute raw : summary) {
    auto row = dyn_cast<DictionaryAttr>(raw);
    auto inputs = row ? row.getAs<ArrayAttr>("inputs") : ArrayAttr();
    if (!row || row.size() != 2 || !inputs)
      return module.emitOpError()
             << "output dependency row requires output and inputs";
    auto output = parseDependencyEndpoint(module, row.get("output"),
                                          signature.getResults(), "output");
    if (failed(output))
      return failure();
    auto leaves = getLeafPaths(module, signature.getResult(output->port));
    if (failed(leaves) || !llvm::is_contained(*leaves, output->path))
      return module.emitOpError()
             << "output dependency must name one output leaf";
    if (llvm::any_of(observed, [&](const DependencyEndpoint &previous) {
          return sameEndpoint(previous, *output);
        }))
      return module.emitOpError()
             << "output dependency repeats an output leaf";
    observed.push_back(*output);
    SmallVector<DependencyEndpoint> parsedInputs;
    for (Attribute inputRaw : inputs) {
      auto input = parseDependencyEndpoint(module, inputRaw,
                                           signature.getInputs(), "input");
      if (failed(input))
        return failure();
      if (llvm::any_of(parsedInputs, [&](const DependencyEndpoint &previous) {
            return sameEndpoint(previous, *input);
          }))
        return module.emitOpError()
               << "output dependency repeats an input endpoint";
      parsedInputs.push_back(*input);
    }
  }
  for (auto [port, type] : llvm::enumerate(signature.getResults())) {
    auto leaves = getLeafPaths(module, type);
    if (failed(leaves))
      return failure();
    for (ArrayAttr path : *leaves)
      if (!llvm::any_of(observed, [&](const DependencyEndpoint &endpoint) {
            return endpoint.port == port && endpoint.path == path;
          }))
        return module.emitOpError()
               << "output dependency facts do not cover every output leaf";
  }
  return success();
}

LogicalResult verifyEquivalentShapes(Operation *operation, ArrayAttr left,
                                     ArrayAttr right,
                                     StringRef diagnostic) {
  if (left.size() != right.size())
    return operation->emitOpError() << diagnostic;
  for (auto [leftRaw, rightRaw] : llvm::zip_equal(left, right)) {
    if (leftRaw == rightRaw)
      continue;
    auto leftValue = literalInteger(dyn_cast<ac::StaticExprAttr>(leftRaw));
    auto rightValue = literalInteger(dyn_cast<ac::StaticExprAttr>(rightRaw));
    if (!leftValue || !rightValue || *leftValue != *rightValue)
      return operation->emitOpError() << diagnostic;
  }
  return success();
}

void printTypeList(OpAsmPrinter &printer, TypeRange types) {
  llvm::interleaveComma(types, printer,
                        [&](Type type) { printer.printType(type); });
}

void printResultTypes(OpAsmPrinter &printer, TypeRange results) {
  if (results.size() == 1) {
    printer.printType(results.front());
    return;
  }
  printer << "(";
  printTypeList(printer, results);
  printer << ")";
}

ParseResult parseParenTypeList(OpAsmParser &parser,
                               SmallVectorImpl<Type> &types) {
  if (parser.parseLParen())
    return failure();
  if (succeeded(parser.parseOptionalRParen()))
    return success();
  if (parser.parseTypeList(types) || parser.parseRParen())
    return failure();
  return success();
}

ParseResult parseResultTypes(OpAsmParser &parser,
                             SmallVectorImpl<Type> &results) {
  if (succeeded(parser.parseOptionalLParen())) {
    if (succeeded(parser.parseOptionalRParen()))
      return success();
    if (parser.parseTypeList(results) || parser.parseRParen())
      return failure();
    return success();
  }
  Type type;
  if (parser.parseType(type))
    return failure();
  results.push_back(type);
  return success();
}

ParseResult parseOptionalAttributes(OpAsmParser &parser,
                                    OperationState &result) {
  if (succeeded(parser.parseOptionalKeyword("attributes")))
    return parser.parseOptionalAttrDict(result.attributes);
  return success();
}

bool hasPrintableAttributes(Operation *operation, ArrayRef<StringRef> elided) {
  for (NamedAttribute attribute : operation->getAttrs())
    if (!llvm::is_contained(elided, attribute.getName().getValue()))
      return true;
  return false;
}

void printAttributes(OpAsmPrinter &printer, Operation *operation,
                     ArrayRef<StringRef> elided) {
  if (!hasPrintableAttributes(operation, elided))
    return;
  printer << " attributes";
  printer.printOptionalAttrDict(operation->getAttrs(), elided);
}

LogicalResult verifyNameList(Operation *operation, ArrayAttr names,
                             unsigned expected, StringRef description) {
  if (!names)
    return success();
  if (names.size() != expected)
    return operation->emitOpError()
           << description << " must name every signature position";
  llvm::StringSet<> seen;
  for (Attribute raw : names) {
    auto name = dyn_cast<StringAttr>(raw);
    if (!name || name.getValue().empty())
      return operation->emitOpError()
             << description << " requires nonempty string names";
    if (!seen.insert(name.getValue()).second)
      return operation->emitOpError()
             << description << " contains duplicate name '" << name.getValue()
             << "'";
  }
  return success();
}

LogicalResult verifyBlockSignature(Operation *operation, Block &block,
                                   TypeRange expected, StringRef description) {
  if (block.getNumArguments() != expected.size())
    return operation->emitOpError()
           << description << " count does not match its block arguments";
  for (auto [index, type] : llvm::enumerate(expected))
    if (!ac::areEquivalentHardwareTypes(block.getArgument(index).getType(),
                                        type))
      return operation->emitOpError()
             << description << " type mismatch at position " << index;
  return success();
}

LogicalResult verifyYieldValues(Operation *operation, YieldOp yield,
                                TypeRange expected) {
  if (yield.getValues().size() != expected.size())
    return operation->emitOpError()
           << "must yield " << expected.size() << " value(s), got "
           << yield.getValues().size();
  for (auto [index, type] : llvm::enumerate(expected))
    if (!ac::areEquivalentHardwareTypes(yield.getValues()[index].getType(),
                                        type))
      return operation->emitOpError() << "yielded value " << index
                                      << " does not match the declared type";
  return success();
}

LogicalResult verifyCallee(Operation *operation, FlatSymbolRefAttr callee,
                           TypeRange inputs, TypeRange outputs,
                           ArrayAttr parameters, ArrayAttr typeArguments,
                           ArrayAttr collectionShape = {}) {
  Operation *target = SymbolTable::lookupNearestSymbolFrom(operation, callee);
  if (!target)
    return operation->emitOpError()
           << "callee '" << callee.getValue() << "' does not resolve";

  FunctionType signature;
  if (auto module = dyn_cast<ModuleOp>(target)) {
    signature = module.getFunctionType();
  } else if (auto imported = dyn_cast<ac::ModuleImportOp>(target)) {
    signature = imported.getFunctionType();
  } else if (isa<ac::ModuleOp>(target)) {
    return operation->emitOpError()
           << "final ACIR modules are not HWIR callees";
  } else {
    return operation->emitOpError()
           << "callee '" << callee.getValue()
           << "' is not an HWIR module or trusted import";
  }

  ArrayAttr declarations = target->getAttrOfType<ArrayAttr>("parameters");
  ArrayAttr typeDeclarations =
      target->getAttrOfType<ArrayAttr>("type_parameters");
  if (!parameters || !typeArguments || !declarations || !typeDeclarations ||
      parameters.size() > declarations.size() ||
      typeArguments.size() != typeDeclarations.size())
    return operation->emitOpError()
           << "static binding arity does not match the callee declaration";

  auto package = operation->getParentOfType<mlir::ModuleOp>();
  ac::HardwareAnalysis analysis(package);
  // Both trusted primitives and ordinary HWIR modules can have static integer
  // or type formals. Resolve the occurrence bindings before comparing wires;
  // otherwise a collection of `callee<N=3>` is incorrectly compared against
  // the callee's unresolved `N` result type.
  auto bound = analysis.bindCallee(target, parameters, typeArguments, {},
                                   operation);
  if (failed(bound))
    return failure();
  std::optional<ac::HardwareBindings> bindings(std::move(*bound));
  ac::HardwareBindings callerBindings;
  auto resolveExpected = [&](Type type) -> FailureOr<Type> {
    if (!bindings)
      return type;
    return analysis.resolveType(type, *bindings, operation);
  };

  if (inputs.size() != signature.getNumInputs())
    return operation->emitOpError()
           << "input count does not match the callee declaration";
  for (auto [index, type] : llvm::enumerate(signature.getInputs())) {
    auto actual = analysis.resolveType(inputs[index], callerBindings,
                                       operation);
    if (failed(actual))
      return failure();
    auto resolved = resolveExpected(type);
    if (failed(resolved))
      return failure();
    Type expected = *resolved;
    if (collectionShape && !isa<ac::TypeParamType>(expected))
      expected = ac::TableType::get(operation->getContext(), collectionShape,
                                    expected);
    if (!isa<ac::TypeParamType>(expected) &&
        !ac::areEquivalentHardwareTypes(*actual, expected))
      return operation->emitOpError()
             << "input " << index << " type does not match the callee";
  }
  if (outputs.size() != signature.getNumResults())
    return operation->emitOpError()
           << "result count does not match the callee declaration";
  for (auto [index, type] : llvm::enumerate(signature.getResults())) {
    auto actual = analysis.resolveType(outputs[index], callerBindings,
                                       operation);
    if (failed(actual))
      return failure();
    auto resolved = resolveExpected(type);
    if (failed(resolved))
      return failure();
    Type expected = *resolved;
    if (collectionShape && !isa<ac::TypeParamType>(expected))
      expected = ac::TableType::get(operation->getContext(), collectionShape,
                                    expected);
    if (!isa<ac::TypeParamType>(expected) &&
        !ac::areEquivalentHardwareTypes(*actual, expected))
      return operation->emitOpError()
             << "result " << index << " type does not match the callee";
  }
  return success();
}

} // namespace

// ---------------------------------------------------------------------------
// hwir.module
// ---------------------------------------------------------------------------

ParseResult ModuleOp::parse(OpAsmParser &parser, OperationState &result) {
  StringAttr name;
  if (parser.parseSymbolName(name, "sym_name", result.attributes))
    return failure();
  SmallVector<Type> inputs, outputs;
  if (parseParenTypeList(parser, inputs))
    return failure();
  if (parser.parseArrow())
    return failure();
  if (parseResultTypes(parser, outputs))
    return failure();
  if (parseOptionalAttributes(parser, result))
    return failure();
  result.addAttribute(
      "function_type",
      TypeAttr::get(FunctionType::get(parser.getContext(), inputs, outputs)));
  Region *body = result.addRegion();
  return parser.parseRegion(*body);
}

void ModuleOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printSymbolName(getSymName());
  printer << "(";
  printTypeList(printer, getFunctionType().getInputs());
  printer << ") -> ";
  printResultTypes(printer, getFunctionType().getResults());
  printAttributes(printer, *this, {"sym_name", "function_type"});
  printer << " ";
  printer.printRegion(getBody());
}

LogicalResult ModuleOp::verify() {
  if (getSymName().empty())
    return emitOpError("requires a nonempty symbol name");
  if (!getParametersAttr() || !getTypeParametersAttr())
    return emitOpError("requires explicit parameter declaration arrays");

  FunctionType signature = getFunctionType();
  if (failed(verifyHardwareTypes(*this, signature.getInputs(), "module input")))
    return failure();
  if (failed(
          verifyHardwareTypes(*this, signature.getResults(), "module output")))
    return failure();
  if (failed(verifyNameList(*this, getInputNamesAttr(),
                            signature.getNumInputs(), "input_names")))
    return failure();
  if (failed(verifyNameList(*this, getOutputNamesAttr(),
                            signature.getNumResults(), "output_names")))
    return failure();

  if (getBody().empty())
    return emitOpError("requires a body region");
  Block &block = getBody().front();
  if (failed(verifyBlockSignature(*this, block, signature.getInputs(),
                                  "module input")))
    return failure();
  auto yield = dyn_cast<YieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("module body must end with hwir.yield");
  if (failed(verifyYieldValues(*this, yield, signature.getResults())))
    return failure();
  if ((*this)->hasAttr("hwir.output_dependencies"))
    return verifyOutputDependencies(*this);
  return success();
}

// ---------------------------------------------------------------------------
// hwir.rule
// ---------------------------------------------------------------------------

ParseResult RuleOp::parse(OpAsmParser &parser, OperationState &result) {
  StringAttr name;
  if (parser.parseAttribute(name, "name", result.attributes))
    return failure();
  SmallVector<OpAsmParser::UnresolvedOperand> captures;
  SmallVector<Type> captureTypes, resultTypes;
  if (parser.parseLParen() || parser.parseOperandList(captures) ||
      parser.parseRParen())
    return failure();
  if (parser.parseColon() || parseParenTypeList(parser, captureTypes))
    return failure();
  if (parser.resolveOperands(captures, captureTypes,
                             parser.getCurrentLocation(), result.operands))
    return failure();
  if (parser.parseArrow() || parseResultTypes(parser, resultTypes))
    return failure();
  result.addTypes(resultTypes);
  if (parseOptionalAttributes(parser, result))
    return failure();
  Region *body = result.addRegion();
  return parser.parseRegion(*body);
}

void RuleOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printAttributeWithoutType(getNameAttr());
  printer << "(";
  printer.printOperands(getCaptures());
  printer << ") : (";
  printTypeList(printer, getCaptures().getTypes());
  printer << ") -> ";
  printResultTypes(printer, getResults().getTypes());
  printAttributes(printer, *this, {"name"});
  printer << " ";
  printer.printRegion(getBody());
}

LogicalResult RuleOp::verify() {
  if (getName().empty())
    return emitOpError("requires a nonempty rule name");
  if (failed(verifyHardwareTypes(*this, getCaptures().getTypes(), "capture")))
    return failure();
  if (failed(verifyHardwareTypes(*this, getResults().getTypes(), "result")))
    return failure();
  if (getBody().empty())
    return emitOpError("requires a body region");
  Block &block = getBody().front();
  if (failed(verifyBlockSignature(*this, block, getCaptures().getTypes(),
                                  "rule capture")))
    return failure();
  auto yield = dyn_cast<YieldOp>(block.getTerminator());
  if (!yield)
    return emitOpError("rule body must end with hwir.yield");
  return verifyYieldValues(*this, yield, getResults().getTypes());
}

// ---------------------------------------------------------------------------
// hwir.instance / hwir.collection
// ---------------------------------------------------------------------------

ParseResult InstanceOp::parse(OpAsmParser &parser, OperationState &result) {
  StringAttr name;
  FlatSymbolRefAttr callee;
  if (parser.parseAttribute(name, "instance_name", result.attributes) ||
      parser.parseAttribute(callee, "callee", result.attributes))
    return failure();
  SmallVector<OpAsmParser::UnresolvedOperand> inputs;
  SmallVector<Type> inputTypes, outputTypes;
  if (parser.parseLParen() || parser.parseOperandList(inputs) ||
      parser.parseRParen())
    return failure();
  if (parser.parseColon() || parseParenTypeList(parser, inputTypes))
    return failure();
  if (parser.resolveOperands(inputs, inputTypes, parser.getCurrentLocation(),
                             result.operands))
    return failure();
  if (parser.parseArrow() || parseResultTypes(parser, outputTypes))
    return failure();
  result.addTypes(outputTypes);
  return parseOptionalAttributes(parser, result);
}

void InstanceOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printAttributeWithoutType(getInstanceNameAttr());
  printer << " ";
  printer.printAttributeWithoutType(getCalleeAttr());
  printer << "(";
  printer.printOperands(getInputs());
  printer << ") : (";
  printTypeList(printer, getInputs().getTypes());
  printer << ") -> ";
  printResultTypes(printer, getResults().getTypes());
  printAttributes(printer, *this, {"instance_name", "callee"});
}

LogicalResult InstanceOp::verify() {
  if (getInstanceName().empty())
    return emitOpError("requires a nonempty instance name");
  if (failed(verifyHardwareTypes(*this, getInputs().getTypes(),
                                 "instance input")) ||
      failed(verifyHardwareTypes(*this, getOutputs().getTypes(),
                                 "instance output")))
    return failure();
  return verifyCallee(*this, getCalleeAttr(), getInputs().getTypes(),
                      getResults().getTypes(), getParametersAttr(),
                      getTypeArgumentsAttr());
}

ParseResult CollectionOp::parse(OpAsmParser &parser, OperationState &result) {
  StringAttr name;
  FlatSymbolRefAttr callee;
  if (parser.parseAttribute(name, "instance_name", result.attributes) ||
      parser.parseAttribute(callee, "callee", result.attributes))
    return failure();
  SmallVector<OpAsmParser::UnresolvedOperand> inputs;
  SmallVector<Type> inputTypes, outputTypes;
  if (parser.parseLParen() || parser.parseOperandList(inputs) ||
      parser.parseRParen())
    return failure();
  if (parser.parseColon() || parseParenTypeList(parser, inputTypes))
    return failure();
  if (parser.resolveOperands(inputs, inputTypes, parser.getCurrentLocation(),
                             result.operands))
    return failure();
  if (parser.parseArrow() || parseResultTypes(parser, outputTypes))
    return failure();
  result.addTypes(outputTypes);
  return parseOptionalAttributes(parser, result);
}

void CollectionOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printAttributeWithoutType(getInstanceNameAttr());
  printer << " ";
  printer.printAttributeWithoutType(getCalleeAttr());
  printer << "(";
  printer.printOperands(getInputs());
  printer << ") : (";
  printTypeList(printer, getInputs().getTypes());
  printer << ") -> ";
  printResultTypes(printer, getResults().getTypes());
  printAttributes(printer, *this, {"instance_name", "callee"});
}

LogicalResult CollectionOp::verify() {
  if (getInstanceName().empty())
    return emitOpError("requires a nonempty instance name");
  if (getShape().size() == 0)
    return emitOpError("requires a nonempty static shape");
  if (failed(verifyHardwareTypes(*this, getInputs().getTypes(),
                                 "collection input")) ||
      failed(verifyHardwareTypes(*this, getOutputs().getTypes(),
                                 "collection output")))
    return failure();
  for (Attribute raw : getShape()) {
    auto dimension = dyn_cast<ac::StaticExprAttr>(raw);
    if (!dimension)
      return emitOpError("requires StaticExpr shape dimensions");
    if (auto literal = literalInteger(dimension);
        literal && (literal->isNegative() || literal->isZero()))
      return emitOpError("requires positive static shape dimensions");
  }
  return verifyCallee(*this, getCalleeAttr(), getInputs().getTypes(),
                      getResults().getTypes(), getParametersAttr(),
                      getTypeArgumentsAttr(), getShape());
}

// ---------------------------------------------------------------------------
// hwir.yield
// ---------------------------------------------------------------------------

ParseResult YieldOp::parse(OpAsmParser &parser, OperationState &result) {
  SmallVector<OpAsmParser::UnresolvedOperand> values;
  if (parser.parseOperandList(values))
    return failure();
  if (values.size() != 0) {
    SmallVector<Type> types;
    if (parser.parseColonTypeList(types))
      return failure();
    if (parser.resolveOperands(values, types, parser.getCurrentLocation(),
                               result.operands))
      return failure();
  }
  return parseOptionalAttributes(parser, result);
}

void YieldOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printOperands(getValues());
  if (getValues().size() != 0) {
    printer << " : ";
    printTypeList(printer, getValues().getTypes());
  }
  printAttributes(printer, *this, {});
}

LogicalResult YieldOp::verify() {
  Operation *parent = (*this)->getParentOp();
  if (auto module = dyn_cast<ModuleOp>(parent))
    return verifyYieldValues(*this, *this,
                             module.getFunctionType().getResults());
  if (auto rule = dyn_cast<RuleOp>(parent))
    return verifyYieldValues(*this, *this, rule.getResults().getTypes());
  if (auto map = dyn_cast<TableMapOp>(parent)) {
    SmallVector<Type> expected;
    for (Value result : map.getResults())
      expected.push_back(
          cast<ac::TableType>(result.getType()).getElementType());
    return verifyYieldValues(*this, *this, expected);
  }
  if (isa<TableMatchOp>(parent)) {
    if (getValues().size() != 1 || !isOneBit(getValues().front().getType()))
      return emitOpError("table.match predicate must yield one bit");
    return success();
  }
  return emitOpError("must terminate an HWIR module, rule or table.map");
}

// ---------------------------------------------------------------------------
// hwir.system
// ---------------------------------------------------------------------------

LogicalResult SystemOp::verify() {
  DictionaryAttr entry = getEntry();
  auto callee = entry.getAs<FlatSymbolRefAttr>("callee");
  auto parameters = entry.getAs<ArrayAttr>("parameters");
  auto typeArguments = entry.getAs<ArrayAttr>("type_arguments");
  if (!callee || !parameters || !typeArguments)
    return emitOpError()
           << "entry requires callee, parameters and type_arguments";
  for (NamedAttribute attribute : entry)
    if (attribute.getName() != "callee" &&
        attribute.getName() != "parameters" &&
        attribute.getName() != "type_arguments")
      return emitOpError() << "entry has unsupported key '"
                           << attribute.getName().getValue() << "'";
  if (getDomain().empty())
    return emitOpError("requires a nonempty domain");
  Operation *target = SymbolTable::lookupNearestSymbolFrom(*this, callee);
  if (!isa_and_nonnull<ModuleOp>(target))
    return emitOpError() << "entry '" << callee.getValue()
                         << "' does not resolve to an HWIR module";
  auto module = cast<ModuleOp>(target);
  ArrayAttr declarations = module.getParametersAttr();
  ArrayAttr typeDeclarations = module.getTypeParametersAttr();
  if (!declarations || !typeDeclarations ||
      parameters.size() > declarations.size() ||
      typeArguments.size() != typeDeclarations.size())
    return emitOpError("entry binding arity does not match its module");
  return success();
}

// ---------------------------------------------------------------------------
// hwir.queue / hwir.table.*
// ---------------------------------------------------------------------------

LogicalResult QueueOp::verify() {
  if (getInstanceName().empty())
    return emitOpError("requires a nonempty instance name");
  if (!isOneBit(getClk().getType()) || !isOneBit(getRst().getType()) ||
      !isOneBit(getInValid().getType()) || !isOneBit(getOutReady().getType()) ||
      !isOneBit(getInReady().getType()) || !isOneBit(getOutValid().getType()))
    return emitOpError("control ports require one-bit hardware wires");
  // Static-expression source provenance is intentionally retained on HWIR
  // types, so equal bit widths need not be pointer-identical MLIR Types.
  if (!ac::areEquivalentHardwareTypes(getInData().getType(),
                                      getOutData().getType()))
    return emitOpError("input and output payload types must match");
  if (auto depth = literalInteger(getDepth());
      depth && (depth->isNegative() || depth->isZero()))
    return emitOpError("depth must be positive");
  if (getReadyPolicy() != "local_occupancy" &&
      getReadyPolicy() != "downstream_pop")
    return emitOpError("unsupported ready policy");
  if (getEmptyFlow() || getReadDuringWrite() != "old" ||
      getResetPolicy() != "sync_high_empty" || getEmptyData() != "zero")
    return emitOpError("unsupported queue behavioral contract");
  return success();
}

LogicalResult TableCreateOp::verify() {
  auto table = getResult().getType();
  if (getInputs().empty())
    return emitOpError("requires at least one element");
  if (auto count = literalElementCount(table.getShape());
      count && *count != getInputs().size())
    return emitOpError("input count must equal the result shape product");
  for (Value input : getInputs())
    if (!ac::areEquivalentHardwareTypes(input.getType(),
                                        table.getElementType()))
      return emitOpError("element type does not match the result table");
  return success();
}

LogicalResult TableSplatOp::verify() {
  // Splat adds the requested outer dimensions around the complete input
  // payload.  When the input is itself a table this deliberately produces a
  // higher-rank table; comparing only against the result element type loses
  // those inner dimensions.
  auto expected = ac::TableType::get(getContext(), getShape(),
                                     getInput().getType());
  if (!ac::areEquivalentHardwareTypes(expected, getResult().getType()))
    return emitOpError("shape and input type do not match the result table");
  return success();
}

LogicalResult TableMapOp::verify() {
  if (getShape().empty() || getBody().empty() || !getBody().hasOneBlock())
    return emitOpError("requires a nonempty shape and one body block");
  for (Value table : getTables())
    if (failed(verifyEquivalentShapes(
            *this, cast<ac::TableType>(table.getType()).getShape(), getShape(),
            "operand table shape does not match map shape")))
      return failure();
  for (Value result : getResults())
    if (failed(verifyEquivalentShapes(
            *this, cast<ac::TableType>(result.getType()).getShape(), getShape(),
            "result table shape does not match map shape")))
      return failure();
  Block &body = getBody().front();
  if (body.empty() || !isa<YieldOp>(body.back()))
    return emitOpError("body must terminate with hwir.yield");
  if (body.getNumArguments() != 1 + getTables().size() + getCaptures().size())
    return emitOpError("body argument count does not match map operands");
  auto ordinal = dyn_cast<ac::BitsType>(body.getArgument(0).getType());
  if (!ordinal)
    return emitOpError("body ordinal argument requires bits");
  if (auto count = literalElementCount(getShape())) {
    auto width = literalWidth(ordinal);
    if (!width || *width != indexWidth(*count))
      return emitOpError("body ordinal width does not match map shape");
  }
  for (auto [index, table] : llvm::enumerate(getTables()))
    if (!ac::areEquivalentHardwareTypes(
            body.getArgument(index + 1).getType(),
            cast<ac::TableType>(table.getType()).getElementType()))
      return emitOpError("body table argument type does not match its element");
  for (auto [index, capture] : llvm::enumerate(getCaptures()))
    if (!ac::areEquivalentHardwareTypes(
            body.getArgument(1 + getTables().size() + index).getType(),
            capture.getType()))
      return emitOpError("body capture argument type does not match operand");
  auto yield = cast<YieldOp>(body.back());
  if (yield.getValues().size() != getResults().size())
    return emitOpError("body yield count does not match map results");
  for (auto [value, result] : llvm::zip_equal(yield.getValues(), getResults()))
    if (!ac::areEquivalentHardwareTypes(
            value.getType(),
            cast<ac::TableType>(result.getType()).getElementType()))
      return emitOpError("body yield type does not match result element");
  return success();
}

LogicalResult TableViewOp::verify() {
  auto input = getInput().getType();
  auto result = getResult().getType();
  if (!ac::areEquivalentHardwareTypes(input.getElementType(), result.getElementType()))
    return emitOpError("input and result element types must match");
  StringRef kind = getKind();
  if (!llvm::is_contained(
          ArrayRef<StringRef>{"reshape", "slice", "transpose", "rotate"},
          kind))
    return emitOpError("unsupported table.view kind");
  if (kind == "reshape") {
    auto shape = getParameters().getAs<ArrayAttr>("shape");
    if (!hasExactlyKeys(getParameters(), {"shape"}) ||
        failed(verifyStaticVector(*this, shape, input.getShape().size(),
                                  "reshape", true)) ||
        failed(verifyEquivalentShapes(
            *this, shape, result.getShape(),
            "reshape shape does not match result")))
      return failure();
    auto sourceCount = literalElementCount(input.getShape());
    auto resultCount = literalElementCount(result.getShape());
    if (sourceCount && resultCount && *sourceCount != *resultCount)
      return emitOpError("reshape must preserve element count");
  } else if (kind == "slice") {
    if (!hasExactlyKeys(getParameters(), {"offsets", "sizes", "strides"}))
      return emitOpError("slice parameter inventory mismatch");
    auto sizes = getParameters().getAs<ArrayAttr>("sizes");
    for (StringRef name : {"offsets", "sizes", "strides"})
      if (failed(verifyStaticVector(*this, getParameters().get(name),
                                    input.getShape().size(), "slice")))
        return failure();
    if (failed(verifyEquivalentShapes(*this, sizes, result.getShape(),
                                       "slice sizes do not match result")))
      return failure();
    auto sourceShape = literalShape(input.getShape());
    auto offsets = literalShape(getParameters().getAs<ArrayAttr>("offsets"));
    auto sliceSizes = literalShape(sizes);
    auto strides = literalShape(getParameters().getAs<ArrayAttr>("strides"));
    if (sourceShape && offsets && sliceSizes && strides)
      for (size_t index = 0; index < sourceShape->size(); ++index) {
        uint64_t offset = (*offsets)[index];
        uint64_t size = (*sliceSizes)[index];
        uint64_t stride = (*strides)[index];
        if (size - 1 > (std::numeric_limits<uint64_t>::max() - offset) / stride ||
            offset + (size - 1) * stride >= (*sourceShape)[index])
          return emitOpError("slice exceeds table extent");
      }
  } else if (kind == "transpose") {
    auto axes = getParameters().getAs<ArrayAttr>("axes");
    if (!hasExactlyKeys(getParameters(), {"axes"}) ||
        failed(verifyStaticVector(*this, axes, input.getShape().size(),
                                  "transpose axes")))
      return failure();
    auto sourceShape = literalShape(input.getShape());
    auto resultShape = literalShape(result.getShape());
    auto axisValues = literalShape(axes);
    if (sourceShape && resultShape && axisValues) {
      SmallVector<bool> seen(sourceShape->size());
      for (auto [index, axis] : llvm::enumerate(*axisValues)) {
        if (axis >= sourceShape->size() || seen[axis])
          return emitOpError("transpose axes must be a rank permutation");
        seen[axis] = true;
        if ((*resultShape)[index] != (*sourceShape)[axis])
          return emitOpError("transpose result shape mismatch");
      }
    }
  } else {
    if (!hasExactlyKeys(getParameters(), {"axis", "offset"}) ||
        !isValidStaticExpression(getParameters().get("axis")) ||
        !isValidStaticExpression(getParameters().get("offset")))
      return emitOpError("rotate requires static axis and offset");
    if (failed(verifyEquivalentShapes(*this, input.getShape(), result.getShape(),
                                       "rotate shape does not match input")))
      return failure();
    if (auto axis = literalInteger(
            getParameters().getAs<ac::StaticExprAttr>("axis"));
        axis && (axis->isNegative() ||
                 axis->getLimitedValue() >= input.getShape().size()))
      return emitOpError("rotate axis out of range");
  }
  return success();
}

LogicalResult TableIndexOp::verify() {
  if (getCoords().size() != getShape().size() || getShape().empty())
    return emitOpError("coordinate rank does not match shape rank");
  if (auto count = literalElementCount(getShape())) {
    auto width = literalWidth(getResult().getType());
    if (!width || *width != countWidth(*count))
      return emitOpError("result width does not match the shape product");
  }
  return success();
}

LogicalResult TableGetOp::verify() {
  if (!ac::areEquivalentHardwareTypes(
          getValue().getType(), getInput().getType().getElementType()))
    return emitOpError("value type does not match the table element type");
  if (!isOneBit(getInRange().getType()))
    return emitOpError("in_range requires a one-bit result");
  return success();
}

LogicalResult TableMatchOp::verify() {
  if (getBody().empty() || !getBody().hasOneBlock())
    return emitOpError("requires one predicate body block");
  Block &body = getBody().front();
  if (body.getNumArguments() != 1 + getCaptures().size() || body.empty() ||
      !isa<YieldOp>(body.back()))
    return emitOpError("predicate body signature is invalid");
  auto yield = cast<YieldOp>(body.back());
  if (yield.getValues().size() != 1 || !isOneBit(yield.getValues()[0].getType()))
    return emitOpError("predicate must yield one bit");
  if (!ac::areEquivalentHardwareTypes(body.getArgument(0).getType(),
                                      getInput().getType().getElementType()))
    return emitOpError("predicate element argument type does not match input");
  for (auto [i, capture] : llvm::enumerate(getCaptures()))
    if (!ac::areEquivalentHardwareTypes(body.getArgument(i + 1).getType(),
                                        capture.getType()))
      return emitOpError("predicate capture type does not match operand");
  if (auto count = literalElementCount(getInput().getType().getShape())) {
    auto width = literalWidth(getMask().getType());
    if (!width || *width != *count)
      return emitOpError("mask width must equal the input element count");
  }
  return success();
}

LogicalResult TableChooseOp::verify() {
  if (getPolicy() != "first" || (getOrder() != "low" && getOrder() != "high"))
    return emitOpError("accepts first policy and low/high order");
  if (getCount() <= 0 || getResults().size() != static_cast<size_t>(getCount()) * 2)
    return emitOpError("requires 2*count results and positive count");
  auto match = getMask().getDefiningOp<TableMatchOp>();
  if (!match || match.getInput() != getInput())
    return emitOpError("mask must come from match of the same table SSA value");
  if (auto count = literalElementCount(getInput().getType().getShape())) {
    if (static_cast<uint64_t>(getCount()) > *count)
      return emitOpError("count exceeds the input element count");
    auto maskWidth = literalWidth(getMask().getType());
    if (!maskWidth || *maskWidth != *count)
      return emitOpError("mask width must equal the input element count");
    for (auto [index, result] : llvm::enumerate(getResults())) {
      auto width = literalWidth(result.getType());
      uint64_t expected =
          index < static_cast<uint64_t>(getCount()) ? indexWidth(*count) : 1;
      if (!width || *width != expected)
        return emitOpError("result widths do not match index/valid contract");
    }
  }
  return success();
}

LogicalResult TableFoldOp::verify() {
  if (!llvm::is_contained(ArrayRef<StringRef>{"add", "mul", "and", "or", "xor", "min", "max"}, getKind()))
    return emitOpError("unsupported closed table.fold kind");
  if (!isa<ac::BitsType>(getInput().getType().getElementType()))
    return emitOpError("requires unsigned bits elements");
  if (!ac::areEquivalentHardwareTypes(getInput().getType().getElementType(),
                                      getResult().getType()))
    return emitOpError("result type must match table element type");
  return success();
}

LogicalResult ValueMergeOp::verify() {
  if (getPaths().size() != getGuards().size() || getPaths().size() != getValues().size())
    return emitOpError("paths, guards and values must have equal lengths");
  if (!ac::areEquivalentHardwareTypes(getBase().getType(), getNext().getType()) || !isOneBit(getEn().getType()))
    return emitOpError("merge result types are invalid");
  llvm::SmallVector<ArrayAttr> paths;
  auto leaves = getLeafPaths(*this, getBase().getType());
  if (failed(leaves))
    return failure();
  std::optional<size_t> previousPosition;
  for (auto [i, raw] : llvm::enumerate(getPaths())) {
    auto path = dyn_cast<ArrayAttr>(raw);
    if (!path)
      return emitOpError("merge requires static field paths");
    for (auto prior : paths)
      if (isPathPrefix(path, prior) || isPathPrefix(prior, path))
        return emitOpError("merge paths overlap");
    if (!isOneBit(getGuards()[i].getType()))
      return emitOpError("merge guards require one-bit values");
    auto target = resolveFieldPath(*this, getBase().getType(), path);
    if (failed(target) || !ac::areEquivalentHardwareTypes(
                              *target, getValues()[i].getType()))
      return emitOpError("merge value type does not match its field path");
    size_t position = 0;
    while (position < leaves->size() &&
           !isPathPrefix(path, (*leaves)[position]))
      ++position;
    if (position == leaves->size())
      return emitOpError("merge path does not name a packed field");
    if (previousPosition && position <= *previousPosition)
      return emitOpError("merge paths must follow declaration field order");
    previousPosition = position;
    paths.push_back(path);
  }
  return success();
}

// ---------------------------------------------------------------------------
// hwir.expect / hwir.observe
// ---------------------------------------------------------------------------

ParseResult ExpectOp::parse(OpAsmParser &parser, OperationState &result) {
  SmallVector<OpAsmParser::UnresolvedOperand> operands;
  SmallVector<Type> types;
  if (parser.parseOperandList(operands))
    return failure();
  if (parser.parseColonTypeList(types))
    return failure();
  if (parser.resolveOperands(operands, types, parser.getCurrentLocation(),
                             result.operands))
    return failure();
  StringAttr kind;
  if (parser.parseKeyword("kind") ||
      parser.parseAttribute(kind, "kind", result.attributes))
    return failure();
  return parseOptionalAttributes(parser, result);
}

void ExpectOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printOperands(getOperation()->getOperands());
  printer << " : ";
  printTypeList(printer, getOperation()->getOperandTypes());
  printer << " kind ";
  printer.printAttributeWithoutType(getKindAttr());
  printAttributes(printer, *this, {"kind"});
}

LogicalResult ExpectOp::verify() {
  if (!isOneBit(getCondition().getType()))
    return emitOpError("condition requires a one-bit hardware wire");
  if (!isOneBit(getPath().getType()))
    return emitOpError("path requires a one-bit hardware wire");
  for (Value sampling : getSampling())
    if (!isOneBit(sampling.getType()))
      return emitOpError("sampling requires one-bit hardware wires");

  StringRef kind = getKind();
  if (kind != "assert" && kind != "range" && kind != "guard_known")
    return emitOpError() << "unknown check kind '" << kind << "'";
  if (kind == "guard_known" && getSampling().size() != 2)
    return emitOpError()
           << "guard_known requires clock then reset sampling operands";
  if (kind != "guard_known" && !getSampling().empty())
    return emitOpError() << kind << " does not accept sampling operands";
  return success();
}

ParseResult ObserveOp::parse(OpAsmParser &parser, OperationState &result) {
  SmallVector<OpAsmParser::UnresolvedOperand> operands;
  SmallVector<Type> types;
  if (parser.parseOperandList(operands))
    return failure();
  if (parser.parseColonTypeList(types))
    return failure();
  if (parser.resolveOperands(operands, types, parser.getCurrentLocation(),
                             result.operands))
    return failure();
  StringAttr kind;
  if (parser.parseKeyword("kind") ||
      parser.parseAttribute(kind, "kind", result.attributes))
    return failure();
  return parseOptionalAttributes(parser, result);
}

void ObserveOp::print(OpAsmPrinter &printer) {
  printer << " ";
  printer.printOperands(getOperation()->getOperands());
  printer << " : ";
  printTypeList(printer, getOperation()->getOperandTypes());
  printer << " kind ";
  printer.printAttributeWithoutType(getKindAttr());
  printAttributes(printer, *this, {"kind"});
}

LogicalResult ObserveOp::verify() {
  if (!isOneBit(getPath().getType()))
    return emitOpError("path requires a one-bit hardware wire");
  if (getKind().empty())
    return emitOpError("requires a nonempty observation kind");
  return verifyHardwareTypes(*this, getValues().getTypes(),
                             "observation value");
}

} // namespace acir::hwir

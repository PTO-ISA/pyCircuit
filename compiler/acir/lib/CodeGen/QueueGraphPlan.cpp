#include "acir/CodeGen/QueueGraphPlan.h"

#include "acir/Analysis/ModelAnalysis.h"
#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Bindings/Binding.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/ACIRTypes.h"
#include "acir/Support/PrimitiveWidths.h"
#include "acir/Transforms/Passes.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <system_error>
#include <tuple>

namespace acir::codegen {

std::string legalizeQueueGraphIdentifier(llvm::StringRef value) {
  auto isAsciiAlphaNumeric = [](unsigned char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9');
  };
  std::string result;
  for (unsigned char character : value.bytes())
    result.push_back(isAsciiAlphaNumeric(character) || character == '_'
                         ? static_cast<char>(character)
                         : '_');
  if (result.empty() || (result.front() >= '0' && result.front() <= '9'))
    result.insert(result.begin(), '_');
  static constexpr llvm::StringLiteral keywords[] = {
      "alignas",       "alignof",     "and",
      "and_eq",        "asm",         "auto",
      "bitand",        "bitor",       "bool",
      "break",         "case",        "catch",
      "char",          "char8_t",     "char16_t",
      "char32_t",      "class",       "compl",
      "concept",       "const",       "consteval",
      "constexpr",     "constinit",   "const_cast",
      "continue",      "co_await",    "co_return",
      "co_yield",      "decltype",    "default",
      "delete",        "do",          "double",
      "dynamic_cast",  "else",        "enum",
      "explicit",      "export",      "extern",
      "false",         "float",       "for",
      "friend",        "goto",        "if",
      "inline",        "int",         "long",
      "mutable",       "namespace",   "new",
      "noexcept",      "not",         "not_eq",
      "nullptr",       "operator",    "or",
      "or_eq",         "private",     "protected",
      "public",        "register",    "reinterpret_cast",
      "requires",      "return",      "short",
      "signed",        "sizeof",      "static",
      "static_assert", "static_cast", "struct",
      "switch",        "template",    "this",
      "thread_local",  "throw",       "true",
      "try",           "typedef",     "typeid",
      "typename",      "union",       "unsigned",
      "using",         "virtual",     "void",
      "volatile",      "wchar_t",     "while",
      "xor",           "xor_eq"};
  if (llvm::is_contained(keywords, llvm::StringRef(result)))
    result.push_back('_');
  return result;
}

llvm::Expected<QueueProvedObligationElisionPlan>
buildProvedObligationElision(mlir::Operation *operation,
                             llvm::StringRef module) {
  auto obligation = mlir::dyn_cast_or_null<ac::ArchitectureObligationOp>(operation);
  if (!obligation || module.empty() ||
      obligation.getStatus() != ac::ArchitectureObligationStatus::Proved ||
      obligation.getKind() != ac::ArchitectureObligationKind::SingleWriter ||
      !obligation.getProofCertificate())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "proved obligation elision requires a closed single-writer record");
  auto certificate = *obligation.getProofCertificate();
  QueueProvedObligationElisionPlan elision;
  elision.module = module.str();
  elision.id = obligation.getId().str();
  elision.kind =
      ac::stringifyArchitectureObligationKind(obligation.getKind()).str();
  elision.reason =
      certificate.getAs<mlir::StringAttr>("kind").getValue().str();
  elision.leftEndpoint = certificate.getAs<mlir::StringAttr>("left_endpoint")
                             .getValue()
                             .str();
  elision.rightEndpoint =
      certificate.getAs<mlir::StringAttr>("right_endpoint").getValue().str();
  elision.ownerPath =
      certificate.getAs<mlir::StringAttr>("owner_path").getValue().str();
  elision.ownerStableId = certificate
                              .getAs<mlir::StringAttr>("owner_stable_id")
                              .getValue()
                              .str();
  {
    llvm::raw_string_ostream stream(elision.sourceProvenance);
    stream << obligation.getSourceProvenance();
  }
  elision.propertyRoot = static_cast<uint64_t>(
      certificate.getAs<mlir::IntegerAttr>("property_root").getInt());
  return elision;
}

namespace {

std::optional<llvm::StringRef> payloadTypeName(llvm::StringRef type);

llvm::Error planError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-QUEUE-PLAN: " + message);
}

llvm::Expected<uint64_t> addBitWidths(uint64_t left, uint64_t right) {
  if (right > std::numeric_limits<uint64_t>::max() - left)
    return planError("value bit width overflows uint64_t");
  return left + right;
}

llvm::Expected<uint64_t> multiplyBitWidths(uint64_t width, uint64_t count) {
  if (count != 0 && width > std::numeric_limits<uint64_t>::max() / count)
    return planError("value bit width overflows uint64_t");
  return width * count;
}

std::string printType(mlir::Type type) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  stream << type;
  return result;
}

mlir::Operation *lookupTypeDeclaration(mlir::Operation *from,
                                       mlir::SymbolRefAttr name);

llvm::Expected<TableInitValuePlan>
extractTableInitValue(mlir::Operation *anchor, mlir::Type type,
                      mlir::Attribute value) {
  TableInitValuePlan result;
  result.type = printType(type);
  if (auto integer = mlir::dyn_cast<mlir::IntegerType>(type)) {
    auto typed = mlir::dyn_cast<mlir::IntegerAttr>(value);
    if (!typed || typed.getType() != integer)
      return planError("typed Table integer initializer is malformed");
    llvm::SmallString<32> literal;
    typed.getValue().toString(literal, 10, /*Signed=*/false);
    result.kind = "integer";
    result.value = literal.str().str();
    return result;
  }
  if (auto enumeration = mlir::dyn_cast<ac::EnumType>(type)) {
    auto enumerant = mlir::dyn_cast<mlir::StringAttr>(value);
    if (!enumerant)
      return planError("typed Table enum initializer is malformed");
    result.kind = "enum";
    result.value = enumerant.getValue().str();
    return result;
  }
  if (auto structure = mlir::dyn_cast<ac::StructType>(type)) {
    auto record = mlir::dyn_cast<mlir::DictionaryAttr>(value);
    auto declaration = mlir::dyn_cast_or_null<ac::StructOp>(
        lookupTypeDeclaration(anchor, structure.getName()));
    if (!record || !declaration)
      return planError("typed Table struct initializer is malformed");
    auto fields = structure.getArguments()
                      ? ac::materializeStructFields(
                            declaration, structure.getArguments(),
                            declaration->getParentOfType<mlir::ModuleOp>())
                      : llvm::Expected<mlir::ArrayAttr>(declaration.getFields());
    if (!fields)
      return fields.takeError();
    result.kind = "struct";
    for (mlir::Attribute rawField : *fields) {
      auto field = mlir::dyn_cast<mlir::DictionaryAttr>(rawField);
      auto name =
          field ? field.getAs<mlir::StringAttr>("name") : mlir::StringAttr();
      auto fieldType =
          field ? field.getAs<mlir::TypeAttr>("type") : mlir::TypeAttr();
      mlir::Attribute member =
          name ? record.get(name.getValue()) : mlir::Attribute();
      if (!name || !fieldType || !member)
        return planError("typed Table struct field initializer is malformed");
      auto element =
          extractTableInitValue(anchor, fieldType.getValue(), member);
      if (!element)
        return element.takeError();
      result.fieldNames.push_back(name.getValue().str());
      result.elements.push_back(std::move(*element));
    }
    return result;
  }
  if (auto tuple = mlir::dyn_cast<mlir::TupleType>(type)) {
    auto elements = mlir::dyn_cast<mlir::ArrayAttr>(value);
    if (!elements || elements.size() != tuple.size())
      return planError("typed Table tuple initializer is malformed");
    result.kind = "tuple";
    for (auto [elementType, element] :
         llvm::zip_equal(tuple.getTypes(), elements)) {
      auto nested = extractTableInitValue(anchor, elementType, element);
      if (!nested)
        return nested.takeError();
      result.elements.push_back(std::move(*nested));
    }
    return result;
  }
  if (auto array = mlir::dyn_cast<ac::ValueArrayType>(type)) {
    auto elements = mlir::dyn_cast<mlir::ArrayAttr>(value);
    if (!elements || static_cast<int64_t>(elements.size()) != array.getLength())
      return planError("typed Table array initializer is malformed");
    result.kind = "array";
    for (mlir::Attribute element : elements) {
      auto nested =
          extractTableInitValue(anchor, array.getElementType(), element);
      if (!nested)
        return nested.takeError();
      result.elements.push_back(std::move(*nested));
    }
    return result;
  }
  return planError("typed Table initializer type is unsupported");
}

template <typename Target, typename Match>
void extractTableDomain(Target &target, Match match) {
  auto copy = [](std::optional<llvm::ArrayRef<int64_t>> values) {
    std::vector<uint64_t> result;
    if (values)
      for (int64_t value : *values)
        result.push_back(static_cast<uint64_t>(value));
    return result;
  };
  target.domainAxes = copy(match.getDomainAxes());
  target.domainShape = copy(match.getDomainShape());
  target.domainStrides = copy(match.getDomainStrides());
  if (auto offset = match.getDomainOffset())
    target.domainOffset = static_cast<uint64_t>(*offset);
  target.hasDomainProjection =
      match.getDomainAxesAttr() || match.getDomainShapeAttr() ||
      match.getDomainStridesAttr() || match.getDomainOffsetAttr();
}

mlir::Operation *lookupTypeDeclaration(mlir::Operation *from,
                                       mlir::SymbolRefAttr name) {
  if (name.getNestedReferences().size() != 1)
    return mlir::SymbolTable::lookupNearestSymbolFrom(from, name);

  mlir::Operation *scope = nullptr;
  if (auto enclosing = from->getParentOfType<ac::TypeScopeOp>();
      enclosing && enclosing.getSymNameAttr() == name.getRootReference())
    scope = enclosing;
  if (!scope) {
    auto root = mlir::FlatSymbolRefAttr::get(name.getRootReference());
    scope = mlir::SymbolTable::lookupNearestSymbolFrom(from, root);
    if (!scope)
      if (auto module = from->getParentOfType<mlir::ModuleOp>())
        scope = mlir::SymbolTable::lookupSymbolIn(module, root);
  }
  if (!mlir::isa_and_nonnull<ac::TypeScopeOp>(scope))
    return nullptr;
  return mlir::SymbolTable::lookupSymbolIn(scope, name.getLeafReference());
}

llvm::Expected<uint64_t>
mlirValueBitWidth(mlir::Operation *from, mlir::Type type,
                  llvm::SmallVectorImpl<mlir::Type> &active) {
  if (auto integer = mlir::dyn_cast<mlir::IntegerType>(type))
    return integer.getWidth();
  if (auto range = mlir::dyn_cast<ac::RangeType>(type)) {
    const uint64_t upper = range.getUpper();
    return upper == std::numeric_limits<uint64_t>::max()
               ? 64
               : std::max<uint64_t>(1, llvm::Log2_64_Ceil(upper + 1));
  }
  if (llvm::is_contained(active, type))
    return planError("recursive value type has no finite bit width");
  active.push_back(type);
  auto finish =
      [&](llvm::Expected<uint64_t> result) -> llvm::Expected<uint64_t> {
    active.pop_back();
    return result;
  };
  if (auto enumeration = mlir::dyn_cast<ac::EnumType>(type)) {
    auto declaration = mlir::dyn_cast_or_null<ac::EnumOp>(
        lookupTypeDeclaration(from, enumeration.getName()));
    if (!declaration)
      return finish(planError("enum type declaration is unresolved"));
    return finish(
        declaration.getEncodingWidthAttr()
            ? *declaration.getEncodingWidth()
            : std::max<uint64_t>(
                  1, llvm::Log2_64_Ceil(declaration.getEnumerants().size())));
  }
  if (auto structure = mlir::dyn_cast<ac::StructType>(type)) {
    auto declaration = mlir::dyn_cast_or_null<ac::StructOp>(
        lookupTypeDeclaration(from, structure.getName()));
    if (!declaration)
      return finish(planError("struct type declaration is unresolved"));
    auto fields = structure.getArguments()
                      ? ac::materializeStructFields(
                            declaration, structure.getArguments(),
                            declaration->getParentOfType<mlir::ModuleOp>())
                      : llvm::Expected<mlir::ArrayAttr>(declaration.getFields());
    if (!fields)
      return finish(fields.takeError());
    uint64_t total = 0;
    for (mlir::Attribute rawField : *fields) {
      auto field = mlir::dyn_cast<mlir::DictionaryAttr>(rawField);
      auto fieldType =
          field ? field.getAs<mlir::TypeAttr>("type") : mlir::TypeAttr();
      if (!fieldType)
        return finish(planError("struct field type is malformed"));
      auto width = mlirValueBitWidth(from, fieldType.getValue(), active);
      if (!width)
        return finish(width.takeError());
      auto next = addBitWidths(total, *width);
      if (!next)
        return finish(next.takeError());
      total = *next;
    }
    return finish(total);
  }
  if (auto tuple = mlir::dyn_cast<mlir::TupleType>(type)) {
    uint64_t total = 0;
    for (mlir::Type element : tuple.getTypes()) {
      auto width = mlirValueBitWidth(from, element, active);
      if (!width)
        return finish(width.takeError());
      auto next = addBitWidths(total, *width);
      if (!next)
        return finish(next.takeError());
      total = *next;
    }
    return finish(total);
  }
  if (auto array = mlir::dyn_cast<ac::ValueArrayType>(type)) {
    auto width = mlirValueBitWidth(from, array.getElementType(), active);
    if (!width)
      return finish(width.takeError());
    return finish(
        multiplyBitWidths(*width, static_cast<uint64_t>(array.getLength())));
  }
  return finish(planError("QueueGraph value type has no bit-width model"));
}

std::optional<std::pair<uint64_t, uint64_t>> rangeBounds(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.range<";
  if (!type.starts_with(prefix) || !type.ends_with('>'))
    return std::nullopt;
  llvm::StringRef bounds = type.drop_front(prefix.size()).drop_back();
  auto [lowerText, upperText] = bounds.split(',');
  uint64_t lower = 0;
  uint64_t upper = 0;
  if (lowerText.trim().getAsInteger(10, lower) ||
      upperText.trim().getAsInteger(10, upper) || lower > upper)
    return std::nullopt;
  return std::pair{lower, upper};
}

std::optional<unsigned> integerWidth(llvm::StringRef type) {
  if (type.consume_front("i")) {
    unsigned width = 0;
    if (type.empty() || type.getAsInteger(10, width) || width == 0)
      return std::nullopt;
    return width;
  }
  auto bounds = rangeBounds(type);
  if (!bounds)
    return std::nullopt;
  return bounds->second == std::numeric_limits<uint64_t>::max()
             ? 64
             : std::max(1u, llvm::Log2_64_Ceil(bounds->second + 1));
}

std::optional<unsigned> bitsWidth(llvm::StringRef type) {
  if (!type.consume_front("i"))
    return std::nullopt;
  unsigned width = 0;
  if (type.empty() || type.getAsInteger(10, width) || width == 0)
    return std::nullopt;
  return width;
}

std::optional<uint64_t> candidateMaskWords(llvm::StringRef type) {
  if (auto width = bitsWidth(type); width && *width <= 64)
    return 1;
  constexpr llvm::StringLiteral prefix = "!ac.value_array<";
  constexpr llvm::StringLiteral suffix = " x i64>";
  if (!type.starts_with(prefix) || !type.ends_with(suffix))
    return std::nullopt;
  uint64_t words = 0;
  llvm::StringRef count =
      type.drop_front(prefix.size()).drop_back(suffix.size());
  if (count.getAsInteger(10, words) || words == 0)
    return std::nullopt;
  return words;
}

bool isCandidateMaskType(llvm::StringRef type, uint64_t entries) {
  if (entries == 0)
    return false;
  if (entries <= 64) {
    auto width = bitsWidth(type);
    return width && *width == entries;
  }
  auto words = candidateMaskWords(type);
  return words && *words == (entries + 63) / 64;
}

std::string exactWidthHex(uint64_t value, unsigned width) {
  std::string result = "0x";
  llvm::raw_string_ostream stream(result);
  stream << llvm::format_hex_no_prefix(value, (width + 3) / 4);
  return result;
}

std::optional<uint64_t> parseExactWidthHex(llvm::StringRef text,
                                           unsigned width) {
  if (!text.starts_with("0x") || text.size() != 2 + (width + 3) / 4)
    return std::nullopt;
  uint64_t value = 0;
  if (text.drop_front(2).getAsInteger(16, value) ||
      exactWidthHex(value, width) != text)
    return std::nullopt;
  return value;
}

ValueConstraint planTypeConstraint(llvm::StringRef type) {
  if (auto bounds = rangeBounds(type))
    return ValueConstraint::closedInterval(bounds->first, bounds->second);
  auto width = integerWidth(type);
  if (!width || *width > 64)
    return ValueConstraint::unknown();
  const uint64_t upper = *width == 64 ? std::numeric_limits<uint64_t>::max()
                                      : (uint64_t{1} << *width) - 1;
  return ValueConstraint::closedInterval(0, upper);
}

std::optional<std::pair<uint64_t, uint64_t>>
constraintBounds(const ValueConstraint &constraint) {
  if (constraint.kind == ValueConstraintKind::Constant)
    return std::pair{constraint.values.front(), constraint.values.front()};
  if (constraint.kind == ValueConstraintKind::FiniteSet &&
      !constraint.values.empty()) {
    auto [lower, upper] =
        std::minmax_element(constraint.values.begin(), constraint.values.end());
    return std::pair{*lower, *upper};
  }
  if (constraint.kind == ValueConstraintKind::ClosedInterval)
    return std::pair{constraint.lower, constraint.upper};
  return std::nullopt;
}

std::optional<uint64_t> planConstantValue(llvm::StringRef literal,
                                          llvm::StringRef type) {
  literal = literal.split(':').first.trim();
  if (literal == "true")
    return 1;
  if (literal == "false")
    return 0;
  auto width = integerWidth(type);
  if (!width || *width > 64)
    return std::nullopt;
  const uint64_t mask = *width == 64 ? std::numeric_limits<uint64_t>::max()
                                     : (uint64_t{1} << *width) - 1;
  if (literal.starts_with('-')) {
    int64_t value = 0;
    if (literal.getAsInteger(10, value))
      return std::nullopt;
    return static_cast<uint64_t>(value) & mask;
  }
  uint64_t value = 0;
  if (literal.getAsInteger(10, value))
    return std::nullopt;
  return value & mask;
}

ValueConstraint
inferPlanConstraint(const QueueExpressionPlan &expression,
                    const llvm::StringMap<ValueConstraint> &constraints,
                    const llvm::StringMap<std::string> &types,
                    const llvm::StringMap<const TablePlan *> &tables) {
  ValueConstraint fallback = planTypeConstraint(expression.type);
  auto operand = [&](size_t index) {
    auto found = index < expression.operands.size()
                     ? constraints.find(expression.operands[index])
                     : constraints.end();
    return found == constraints.end() ? ValueConstraint::unknown()
                                      : found->getValue();
  };
  if (expression.kind == "constant") {
    if (auto value = planConstantValue(expression.literal, expression.type))
      return ValueConstraint::constant(*value);
    return ValueConstraint::unknown();
  }
  if (expression.kind == "value_select") {
    ValueConstraint condition = operand(0);
    if (condition.kind == ValueConstraintKind::Constant)
      return operand(condition.values.front() == 0 ? 2 : 1);
    return ValueConstraint::join(operand(1), operand(2));
  }
  if (expression.kind == "masked_match") {
    auto type = expression.operands.empty()
                    ? types.end()
                    : types.find(expression.operands.front());
    auto width = type == types.end() ? std::optional<unsigned>()
                                     : integerWidth(type->getValue());
    auto mask = width ? parseExactWidthHex(expression.mask, *width)
                      : std::optional<uint64_t>();
    auto expected = width ? parseExactWidthHex(expression.value, *width)
                          : std::optional<uint64_t>();
    if (!mask || !expected)
      return ValueConstraint::unknown();
    if (*mask == 0)
      return ValueConstraint::constant(1);
    ValueConstraint input = operand(0);
    if (input.kind == ValueConstraintKind::Constant)
      return ValueConstraint::constant((input.values.front() & *mask) ==
                                       *expected);
    return ValueConstraint::closedInterval(0, 1);
  }
  if (expression.kind == "cmp" || expression.kind == "range_cmp" ||
      expression.kind == "priority_valid" ||
      expression.kind == "table_choose_valid" ||
      expression.kind == "table_selection_valid_ref")
    return ValueConstraint::closedInterval(0, 1);
  if (expression.kind == "priority_index") {
    auto type = expression.operands.empty()
                    ? types.end()
                    : types.find(expression.operands.front());
    auto width = type == types.end() ? std::optional<unsigned>()
                                     : integerWidth(type->getValue());
    return width ? ValueConstraint::closedInterval(0, *width - 1)
                 : ValueConstraint::unknown();
  }
  if (expression.kind == "table_choose_index" ||
      expression.kind == "table_selection_index_ref") {
    auto table = tables.find(expression.table);
    return table != tables.end() && table->getValue()->entries != 0
               ? ValueConstraint::closedInterval(0,
                                                 table->getValue()->entries - 1)
               : ValueConstraint::unknown();
  }

  ValueConstraint left = operand(0);
  ValueConstraint right = operand(1);
  const bool constantOperands = left.kind == ValueConstraintKind::Constant &&
                                right.kind == ValueConstraintKind::Constant;
  auto resultWidth = integerWidth(expression.type);
  const uint64_t mask = !resultWidth || *resultWidth == 64
                            ? std::numeric_limits<uint64_t>::max()
                            : (uint64_t{1} << *resultWidth) - 1;
  if (constantOperands) {
    uint64_t lhs = left.values.front();
    uint64_t rhs = right.values.front();
    if (expression.kind == "add")
      return ValueConstraint::constant((lhs + rhs) & mask);
    if (expression.kind == "sub")
      return ValueConstraint::constant((lhs - rhs) & mask);
    if (expression.kind == "mul")
      return ValueConstraint::constant((lhs * rhs) & mask);
    if (expression.kind == "udiv")
      return ValueConstraint::constant(rhs == 0 ? 0 : lhs / rhs);
    if (expression.kind == "urem")
      return ValueConstraint::constant(rhs == 0 ? 0 : lhs % rhs);
    if (expression.kind == "and")
      return ValueConstraint::constant(lhs & rhs);
    if (expression.kind == "or")
      return ValueConstraint::constant(lhs | rhs);
    if (expression.kind == "xor")
      return ValueConstraint::constant(lhs ^ rhs);
    if (expression.kind == "shl")
      return ValueConstraint::constant(
          !resultWidth || rhs >= *resultWidth ? 0 : (lhs << rhs) & mask);
    if (expression.kind == "shr")
      return ValueConstraint::constant(
          !resultWidth || rhs >= *resultWidth ? 0 : lhs >> rhs);
  }
  if (expression.kind == "add" || expression.kind == "sub" ||
      expression.kind == "mul") {
    auto leftBounds = constraintBounds(left);
    auto rightBounds = constraintBounds(right);
    if (leftBounds && rightBounds) {
      uint64_t lower = 0;
      uint64_t upper = 0;
      bool safe = true;
      if (expression.kind == "add") {
        safe = rightBounds->first <=
                   std::numeric_limits<uint64_t>::max() - leftBounds->first &&
               rightBounds->second <=
                   std::numeric_limits<uint64_t>::max() - leftBounds->second;
        if (safe) {
          lower = leftBounds->first + rightBounds->first;
          upper = leftBounds->second + rightBounds->second;
        }
      } else if (expression.kind == "sub") {
        safe = leftBounds->first >= rightBounds->second;
        if (safe) {
          lower = leftBounds->first - rightBounds->second;
          upper = leftBounds->second - rightBounds->first;
        }
      } else {
        safe = (leftBounds->first == 0 ||
                rightBounds->first <=
                    std::numeric_limits<uint64_t>::max() / leftBounds->first) &&
               (leftBounds->second == 0 ||
                rightBounds->second <=
                    std::numeric_limits<uint64_t>::max() / leftBounds->second);
        if (safe) {
          lower = leftBounds->first * rightBounds->first;
          upper = leftBounds->second * rightBounds->second;
        }
      }
      if (safe && upper <= mask)
        return ValueConstraint::closedInterval(lower, upper);
    }
  }
  if (expression.kind == "and") {
    if (left.kind == ValueConstraintKind::Constant)
      return ValueConstraint::closedInterval(0, left.values.front() & mask);
    if (right.kind == ValueConstraintKind::Constant)
      return ValueConstraint::closedInterval(0, right.values.front() & mask);
  }
  if (expression.kind == "urem" &&
      right.kind == ValueConstraintKind::Constant && right.values.front() != 0)
    return ValueConstraint::closedInterval(0, right.values.front() - 1);
  if (expression.kind == "not" && left.kind == ValueConstraintKind::Constant)
    return ValueConstraint::constant((~left.values.front()) & mask);
  return fallback;
}

std::string printAttribute(mlir::Attribute attribute) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  stream << attribute;
  return result;
}

std::string runtimeMaterializationSignature(
    const QueueArchitectureObligationPlan &obligation,
    llvm::StringRef target) {
  auto predicate = [](const std::string &rule,
                      std::optional<uint64_t> root) {
    return root ? rule + ":" + std::to_string(*root) : std::string("-");
  };
  return "module=" + obligation.module + ";target=" + target.str() +
         ";firing=" + obligation.firing +
         ";input=" + std::to_string(obligation.inputOrdinal) +
         ";maximum=" + std::to_string(obligation.maximum) +
         ";sampling=" + obligation.sampling + ";condition=" +
         obligation.conditionTable + ":" + obligation.conditionRule + ":" +
         std::to_string(obligation.conditionRoot) +
         ";severity=" + obligation.severity +
         ";active=" + predicate(obligation.activeRule, obligation.activeRoot) +
         ";disable=" +
         predicate(obligation.disableRule, obligation.disableRoot);
}

std::string printRegion(mlir::Region &region) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  region.getParentOp()->print(stream);
  return result;
}

std::string scopePath(llvm::ArrayRef<std::string> scope) {
  std::string result;
  for (llvm::StringRef part : scope) {
    result.push_back('/');
    result.append(part);
  }
  return result.empty() ? "/" : result;
}

llvm::Expected<std::string>
queueName(mlir::Value value,
          const llvm::DenseMap<mlir::Value, std::string> &names) {
  auto found = names.find(value);
  if (found == names.end())
    return planError("Queue operand has no frozen logical identity");
  return found->second;
}

llvm::Expected<std::vector<std::string>>
queueNames(mlir::ValueRange values,
           const llvm::DenseMap<mlir::Value, std::string> &names) {
  std::vector<std::string> result;
  for (mlir::Value value : values) {
    auto name = queueName(value, names);
    if (!name)
      return name.takeError();
    result.push_back(std::move(*name));
  }
  return result;
}

llvm::Expected<std::vector<std::string>> outputNames(mlir::Operation *op,
                                                     size_t count) {
  std::vector<std::string> result;
  if (count == 1)
    if (auto name = op->getAttrOfType<mlir::StringAttr>("ac.name"))
      result.push_back(name.getValue().str());
  if (result.empty())
    if (auto names = op->getAttrOfType<mlir::ArrayAttr>("ac.output_names"))
      for (mlir::Attribute value : names) {
        auto name = mlir::dyn_cast<mlir::StringAttr>(value);
        if (!name)
          return planError("ac.output_names must contain only strings");
        result.push_back(name.getValue().str());
      }
  if (result.size() != count)
    return planError("Queue-producing op requires exact frozen output names");
  return result;
}

using SharedExpression = std::pair<mlir::Value, QueueExpressionPlan>;
using SharedValue = std::pair<mlir::Value, std::string>;

std::string localSourceOriginKey(const QueueSourceOriginPlan &origin) {
  std::string key;
  llvm::raw_string_ostream stream(key);
  for (const QueueSourceFramePlan &frame : origin)
    stream << frame.kind << '\0' << frame.file << '\0' << frame.line << '\0'
           << frame.column << '\0' << frame.symbol << '\0';
  return key;
}

void mergeSourceProvenance(QueueSourceProvenancePlan &target,
                           const QueueSourceProvenancePlan &additional) {
  target.origins.insert(target.origins.end(), additional.origins.begin(),
                        additional.origins.end());
  llvm::sort(target.origins, [](const QueueSourceOriginPlan &left,
                                const QueueSourceOriginPlan &right) {
    return localSourceOriginKey(left) < localSourceOriginKey(right);
  });
  target.origins.erase(
      std::unique(target.origins.begin(), target.origins.end()),
      target.origins.end());
}

std::string legalDisplayIdentity(llvm::StringRef value) {
  return legalizeQueueGraphIdentifier(value);
}

std::string normalizedRelativeSourcePath(llvm::StringRef filename) {
  for (llvm::StringRef marker : {"/examples/", "/tests/", "/python/",
                                 "/compiler/", "/simulator/", "/tools/"})
    if (size_t position = filename.find(marker);
        position != llvm::StringRef::npos)
      return filename.drop_front(position + 1).str();
  if (llvm::sys::path::is_absolute(filename))
    return llvm::sys::path::filename(filename).str();
  return filename.str();
}

bool isValidPythonSourcePath(llvm::StringRef path) {
  if (path.empty() || llvm::sys::path::is_absolute(path) ||
      !path.ends_with(".py") || path.contains('\\') || path.contains("//") ||
      (path.size() >= 2 && llvm::isAlpha(path[0]) && path[1] == ':'))
    return false;
  llvm::SmallVector<llvm::StringRef> segments;
  path.split(segments, '/', /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (llvm::StringRef segment : segments) {
    if (segment.empty() || segment == "." || segment == "..")
      return false;
    for (char character : segment)
      if (!llvm::isAlnum(character) && character != '.' && character != '_' &&
          character != '+' && character != '@' && character != '-')
        return false;
  }
  return true;
}

llvm::Expected<QueueSourceProvenancePlan>
extractSourceProvenance(mlir::Operation *operation) {
  QueueSourceProvenancePlan result;
  mlir::Attribute raw = operation->getAttr("ac.source_provenance");
  if (!raw && mlir::isa<ac::ArchitectureObligationOp>(operation))
    raw = operation->getAttr("source_provenance");
  if (!raw)
    return result;
  auto origins = mlir::dyn_cast<mlir::ArrayAttr>(raw);
  if (!origins || origins.empty())
    return planError("source provenance must be a non-empty origin array");
  for (mlir::Attribute rawOrigin : origins) {
    auto origin = mlir::dyn_cast<mlir::DictionaryAttr>(rawOrigin);
    auto frames =
        origin ? origin.getAs<mlir::ArrayAttr>("frames") : mlir::ArrayAttr();
    if (!origin || origin.size() != 1 || !frames || frames.empty())
      return planError("source provenance origin is malformed");
    QueueSourceOriginPlan plannedOrigin;
    for (mlir::Attribute rawFrame : frames) {
      auto frame = mlir::dyn_cast<mlir::DictionaryAttr>(rawFrame);
      auto file =
          frame ? frame.getAs<mlir::StringAttr>("file") : mlir::StringAttr();
      auto kind =
          frame ? frame.getAs<mlir::StringAttr>("kind") : mlir::StringAttr();
      auto line =
          frame ? frame.getAs<mlir::IntegerAttr>("line") : mlir::IntegerAttr();
      auto column = frame ? frame.getAs<mlir::IntegerAttr>("column")
                          : mlir::IntegerAttr();
      auto symbol =
          frame ? frame.getAs<mlir::StringAttr>("symbol") : mlir::StringAttr();
      if (!frame || (frame.size() != 4 && frame.size() != 5) || !file ||
          !kind || !line || !column ||
          (frame.size() == 5) != static_cast<bool>(symbol))
        return planError("source provenance frame is malformed");
      llvm::StringRef rawFile = file.getValue();
      llvm::StringRef rawKind = kind.getValue();
      const int64_t rawLine = line.getInt();
      const int64_t rawColumn = column.getInt();
      if ((rawKind != "statement" && rawKind != "definition" &&
           rawKind != "inline_callsite" && rawKind != "instance") ||
          !isValidPythonSourcePath(rawFile) || rawLine <= 0 || rawColumn <= 0)
        return planError("source provenance frame is malformed");
      plannedOrigin.push_back(
          {rawKind.str(), rawFile.str(), static_cast<uint64_t>(rawLine),
           static_cast<uint64_t>(rawColumn),
           symbol ? symbol.getValue().str() : std::string()});
    }
    result.origins.push_back(std::move(plannedOrigin));
  }
  return result;
}

llvm::Error extractNdfMetadata(mlir::Operation *operation, llvm::StringRef name,
                               std::vector<std::string> &target) {
  auto values = operation->getAttrOfType<mlir::ArrayAttr>(name);
  if (!values)
    return operation->hasAttr(name)
               ? planError(name + " must be an array of NDF identifiers")
               : llvm::Error::success();
  llvm::StringSet<> unique;
  for (mlir::Attribute value : values) {
    auto identifier = mlir::dyn_cast<mlir::StringAttr>(value);
    if (!identifier || identifier.getValue().empty() ||
        !identifier.getValue().contains('-') ||
        !llvm::all_of(identifier.getValue(),
                      [](char character) {
                        return (character >= 'A' && character <= 'Z') ||
                               (character >= '0' && character <= '9') ||
                               character == '-';
                      }) ||
        !unique.insert(identifier.getValue()).second)
      return planError(name + " contains an invalid NDF identifier");
    target.push_back(identifier.getValue().str());
  }
  return llvm::Error::success();
}

llvm::Error extractDefinitionSource(mlir::Operation *operation,
                                    QueueGraphPlan &plan) {
  auto file = operation->getAttrOfType<mlir::StringAttr>("ac.source_file");
  auto line = operation->getAttrOfType<mlir::IntegerAttr>("ac.source_line");
  auto column = operation->getAttrOfType<mlir::IntegerAttr>("ac.source_column");
  if (!file && !line && !column)
    return llvm::Error::success();
  if (!file || !line || !column || !isValidPythonSourcePath(file.getValue()) ||
      line.getInt() <= 0 || column.getInt() <= 0)
    return planError("module Python source metadata is malformed");
  plan.sourceFile = file.getValue().str();
  plan.sourceLine = static_cast<uint64_t>(line.getInt());
  plan.sourceColumn = static_cast<uint64_t>(column.getInt());
  return llvm::Error::success();
}

llvm::Error extractDisplayProvenance(mlir::Operation *operation,
                                     QueueBlockPlan &plan) {
  if (auto error = extractNdfMetadata(operation, "ac.ndf_ids", plan.ndfIds))
    return error;
  if (auto error =
          extractNdfMetadata(operation, "ac.ndf_requires", plan.ndfRequires))
    return error;
  if (auto name =
          operation->getAttrOfType<mlir::StringAttr>("ac.rule_definition"))
    plan.displayRuleName = name.getValue().str();
  auto sourceFile =
      operation->getAttrOfType<mlir::StringAttr>("ac.source_file");
  auto sourceLine =
      operation->getAttrOfType<mlir::IntegerAttr>("ac.source_line");
  auto sourceColumn =
      operation->getAttrOfType<mlir::IntegerAttr>("ac.source_column");
  if (sourceFile && sourceLine && sourceColumn) {
    llvm::StringRef rawFile = sourceFile.getValue();
    const int64_t rawLine = sourceLine.getInt();
    const int64_t rawColumn = sourceColumn.getInt();
    if (rawFile.ends_with(".py")) {
      if (!isValidPythonSourcePath(rawFile) || rawLine <= 0 || rawColumn <= 0)
        return planError("display source location is malformed");
      plan.sourceFile = rawFile.str();
      plan.sourceLine = static_cast<uint64_t>(rawLine);
      plan.sourceColumn = static_cast<uint64_t>(rawColumn);
    }
  } else if (auto source =
                 mlir::dyn_cast<mlir::FileLineColLoc>(operation->getLoc());
             source && isValidPythonSourcePath(source.getFilename())) {
    plan.sourceFile = source.getFilename().str();
    plan.sourceLine = source.getLine();
    plan.sourceColumn = source.getColumn();
  }
  auto provenance = extractSourceProvenance(operation);
  if (!provenance)
    return provenance.takeError();
  plan.sourceProvenance = std::move(*provenance);
  if (plan.sourceFile.empty() && !plan.sourceProvenance.origins.empty() &&
      !plan.sourceProvenance.origins.front().empty()) {
    const QueueSourceFramePlan &frame =
        plan.sourceProvenance.origins.front().front();
    plan.sourceFile = frame.file;
    plan.sourceLine = frame.line;
    plan.sourceColumn = frame.column;
  }
  return llvm::Error::success();
}

llvm::Error
extractExpressions(mlir::Region &region, QueueBlockPlan &plan,
                   llvm::ArrayRef<SharedExpression> sharedExpressions = {},
                   llvm::ArrayRef<SharedValue> sharedValues = {},
                   llvm::StringRef prefix = "v",
                   llvm::ArrayRef<std::string> argumentNames = {}) {
  mlir::Block &block = region.front();
  llvm::DenseMap<mlir::Value, std::string> values;
  llvm::StringSet<> identities;
  for (const auto &[value, identity] : sharedValues) {
    values[value] = identity;
    identities.insert(identity);
  }
  for (const auto &[value, expression] : sharedExpressions) {
    values[value] = expression.result;
    identities.insert(expression.result);
    if (llvm::none_of(plan.expressions, [&](const QueueExpressionPlan &item) {
          return item.result == expression.result;
        }))
      plan.expressions.push_back(expression);
  }
  if (!argumentNames.empty() && argumentNames.size() != block.getNumArguments())
    return planError("helper argument identity count is malformed");
  for (auto [index, argument] : llvm::enumerate(block.getArguments())) {
    values[argument] = !argumentNames.empty() ? argumentNames[index]
                       : index == 0 ? (prefix == "v" ? "item" : "entry")
                                    : (prefix == "v" ? "item" : "entry") +
                                          std::to_string(index);
    identities.insert(values[argument]);
  }
  auto resultIdentity = [&](mlir::Operation &operation,
                            llvm::StringRef fallback) {
    std::string base;
    if (auto display =
            operation.getAttrOfType<mlir::StringAttr>("ac.display_name"))
      base = legalDisplayIdentity(display.getValue());
    if (base.empty())
      base = fallback.str();
    std::string result = base;
    for (uint64_t suffix = 2; !identities.insert(result).second; ++suffix)
      result = base + "_" + std::to_string(suffix);
    return result;
  };
  auto operandNames = [&](mlir::ValueRange operands)
      -> llvm::Expected<std::vector<std::string>> {
    std::vector<std::string> result;
    for (mlir::Value operand : operands) {
      auto found = values.find(operand);
      if (found == values.end())
        return planError(
            "Var expression operand from '" +
            (operand.getDefiningOp()
                 ? operand.getDefiningOp()->getName().getStringRef().str()
                 : std::string("block argument")) +
            "' with type '" + printType(operand.getType()) +
            "' has no local identity");
      result.push_back(found->second);
    }
    return result;
  };
  QueueSourceProvenancePlan currentExpressionProvenance;
  auto append = [&](mlir::Operation &operation, llvm::StringRef kind,
                    llvm::StringRef field = {}, llvm::StringRef predicate = {},
                    llvm::StringRef literal = {}) -> llvm::Error {
    if (operation.getNumResults() != 1)
      return planError("Var expression must produce exactly one result");
    auto resultType =
        mlir::dyn_cast<ac::VarType>(operation.getResult(0).getType());
    if (!resultType)
      return planError("Var expression result must be ac.var");
    auto operands = operandNames(operation.getOperands());
    if (!operands)
      return operands.takeError();
    std::string result = resultIdentity(
        operation, prefix.str() + std::to_string(plan.expressions.size()));
    values[operation.getResult(0)] = result;
    QueueExpressionPlan expression{std::move(result),
                                   kind.str(),
                                   printType(resultType.getElementType()),
                                   std::move(*operands),
                                   field.str(),
                                   predicate.str(),
                                   literal.str()};
    if (auto target =
            operation.getAttrOfType<mlir::StringAttr>("ac.static_type_target"))
      expression.staticTypeTarget = target.getValue().str();
    expression.sourceProvenance = currentExpressionProvenance;
    plan.expressions.push_back(std::move(expression));
    return llvm::Error::success();
  };

  bool sawStructuredYield = false;

  for (mlir::Operation &operation : block) {
    auto provenance = extractSourceProvenance(&operation);
    if (!provenance)
      return provenance.takeError();
    currentExpressionProvenance = std::move(*provenance);
    if (operation.getName().getStringRef() == "ac.var.invariant")
      return planError(
          "residual ac.var.invariant must be lowered before QueueGraph "
          "planning");
    if (auto call = mlir::dyn_cast<mlir::func::CallOp>(operation)) {
      auto operands = operandNames(call.getOperands());
      if (!operands)
        return operands.takeError();
      if (call.getNumResults() == 0)
        return planError("pure Queue helper call must return a value");
      const std::string callIdentity =
          prefix.str() + "call" + std::to_string(plan.expressions.size());
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(call.getResults())) {
        auto resultType = mlir::dyn_cast<ac::VarType>(resultValue.getType());
        if (!resultType)
          return planError("pure Queue helper result must be ac.var");
        std::string result =
            prefix.str() + std::to_string(plan.expressions.size());
        values[resultValue] = result;
        QueueExpressionPlan expression{result, "helper_call",
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.field = call.getCallee().str();
        expression.literal = callIdentity;
        expression.selectionCount = call.getNumResults();
        expression.laneOrdinal = resultIndex;
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto constant = mlir::dyn_cast<ac::VarConstantOp>(operation)) {
      std::string literal = printAttribute(constant.getValueAttr());
      if (auto integer =
              mlir::dyn_cast<mlir::IntegerAttr>(constant.getValueAttr());
          integer &&
          mlir::cast<mlir::IntegerType>(integer.getType()).getWidth() > 1 &&
          mlir::cast<mlir::IntegerType>(integer.getType()).getWidth() <= 64)
        literal = std::to_string(integer.getValue().getZExtValue()) + " : " +
                  printType(integer.getType());
      if (auto error = append(operation, "constant", {}, {}, literal))
        return error;
      continue;
    }
    if (auto value = mlir::dyn_cast<ac::VarEnumOp>(operation)) {
      auto declaration = mlir::dyn_cast_or_null<ac::EnumOp>(
          lookupTypeDeclaration(value, value.getDeclaration()));
      if (!declaration)
        return planError("enum value declaration is unresolved");
      auto enumerant = llvm::find_if(
          declaration.getEnumerants(), [&](mlir::Attribute candidate) {
            return mlir::cast<mlir::StringAttr>(candidate).getValue() ==
                   value.getEnumerant();
          });
      if (enumerant == declaration.getEnumerants().end())
        return planError("enum value is absent from QueueGraph declaration");
      const size_t ordinal =
          std::distance(declaration.getEnumerants().begin(), enumerant);
      const uint64_t encoded = declaration.getValuesAttr()
                                   ? mlir::cast<mlir::IntegerAttr>(
                                         declaration.getValuesAttr()[ordinal])
                                         .getValue()
                                         .getZExtValue()
                                   : ordinal;
      if (auto error = append(operation, "enum_constant", value.getEnumerant(),
                              {}, std::to_string(encoded)))
        return error;
      continue;
    }
    if (mlir::isa<ac::RecoveryEventOp>(operation)) {
      if (auto error = append(operation, "recovery_event"))
        return error;
      auto event = mlir::cast<ac::RecoveryEventOp>(operation);
      plan.expressions.back().field = event.getDomain().str();
      plan.expressions.back().predicate = event.getCause().str();
      continue;
    }
    if (auto kill = mlir::dyn_cast<ac::KillSetOp>(operation)) {
      if (auto error = append(operation, "kill_set", {}, kill.getPolicy()))
        return error;
      continue;
    }
    if (auto reservation = mlir::dyn_cast<ac::ReservationSetOp>(operation)) {
      if (auto error = append(operation, "reservation_set"))
        return error;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(reservation.getLanes());
      plan.expressions.back().predicate =
          reservation.getCommit() ? "commit" : "preview";
      continue;
    }
    if (auto group = mlir::dyn_cast<ac::TransactionGroupOp>(operation)) {
      if (auto error = append(operation, "transaction_group", {},
                              ac::stringifyTransactionGroupPolicy(
                                  group.getPolicy())))
        return error;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(group.getLanes());
      continue;
    }
    if (auto allocator = mlir::dyn_cast<ac::MultiAllocatorOp>(operation)) {
      auto operands = operandNames(allocator->getOperands());
      if (!operands)
        return operands.takeError();
      constexpr llvm::StringLiteral kinds[] = {
          "multi_allocator_allocation", "multi_allocator_accepted",
          "multi_allocator_next_free"};
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(allocator->getResults())) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result = resultIdentity(
            operation, prefix.str() + std::to_string(plan.expressions.size()));
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kinds[resultIndex].str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.selectionCount =
            static_cast<uint64_t>(allocator.getLanes());
        expression.predicate = ac::stringifySameCycleReusePolicy(
                                   allocator.getReusePolicy())
                                   .str();
        expression.width =
            static_cast<uint64_t>(allocator.getGenerationBits());
        expression.literal = allocator.getGenerationPolicy().str();
        expression.laneOrdinal = resultIndex;
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto select = mlir::dyn_cast<ac::AgeSelectKOp>(operation)) {
      if (auto error = append(operation, "age_select_k", {},
                              select.getOrdering()))
        return error;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(select.getLanes());
      plan.expressions.back().laneOrdinal =
          static_cast<uint64_t>(select.getCount());
      continue;
    }
    if (auto dependency = mlir::dyn_cast<ac::DependencySetOp>(operation)) {
      auto operands = operandNames(dependency->getOperands());
      if (!operands)
        return operands.takeError();
      constexpr llvm::StringLiteral kinds[] = {"dependency_set_next",
                                                "dependency_set_ready"};
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(dependency->getResults())) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result = resultIdentity(
            operation, prefix.str() + std::to_string(plan.expressions.size()));
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kinds[resultIndex].str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.selectionCount =
            static_cast<uint64_t>(dependency.getLanes());
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto terminal = mlir::dyn_cast<ac::TerminalTransactionOp>(operation)) {
      if (auto error = append(operation, "terminal_transaction"))
        return error;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(terminal.getLanes());
      continue;
    }
    if (auto edge = mlir::dyn_cast<ac::MemoryOrderEdgeOp>(operation)) {
      if (auto error = append(operation, "memory_order_edge", {},
                              ac::stringifyMemoryOrderKind(edge.getKind())))
        return error;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(edge.getLanes());
      continue;
    }
    if (auto disposition = mlir::dyn_cast<ac::LoadDispositionOp>(operation)) {
      auto operands = operandNames(disposition->getOperands());
      if (!operands)
        return operands.takeError();
      constexpr llvm::StringLiteral kinds[] = {
          "load_disposition_wait", "load_disposition_bypass",
          "load_disposition_forward", "load_disposition_replay",
          "load_disposition_stale"};
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(disposition->getResults())) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result = resultIdentity(
            operation, prefix.str() + std::to_string(plan.expressions.size()));
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kinds[resultIndex].str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.selectionCount =
            static_cast<uint64_t>(disposition.getLanes());
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto tuple = mlir::dyn_cast<ac::VarTupleOp>(operation)) {
      if (auto error = append(operation, "tuple_create"))
        return error;
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(
          tuple,
          mlir::cast<ac::VarType>(tuple.getResult().getType()).getElementType(),
          active);
      if (!width)
        return width.takeError();
      plan.expressions.back().width = *width;
      continue;
    }
    if (auto array = mlir::dyn_cast<ac::VarArrayOp>(operation)) {
      if (auto error = append(operation, "array_create"))
        return error;
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(
          array,
          mlir::cast<ac::VarType>(array.getResult().getType()).getElementType(),
          active);
      if (!width)
        return width.takeError();
      plan.expressions.back().width = *width;
      continue;
    }
    if (auto record = mlir::dyn_cast<ac::VarRecordOp>(operation)) {
      if (auto error = append(operation, "record_create"))
        return error;
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(
          record,
          mlir::cast<ac::VarType>(record.getResult().getType())
              .getElementType(),
          active);
      if (!width)
        return width.takeError();
      plan.expressions.back().width = *width;
      continue;
    }
    if (auto element = mlir::dyn_cast<ac::VarElementOp>(operation)) {
      if (auto error = append(operation, "aggregate_get"))
        return error;
      mlir::Type aggregate =
          mlir::cast<ac::VarType>(element.getAggregate().getType())
              .getElementType();
      uint64_t lsb = 0;
      if (auto tuple = mlir::dyn_cast<mlir::TupleType>(aggregate)) {
        for (mlir::Type trailing :
             tuple.getTypes().drop_front(element.getIndex() + 1)) {
          llvm::SmallVector<mlir::Type> active;
          auto width = mlirValueBitWidth(element, trailing, active);
          if (!width)
            return width.takeError();
          lsb += *width;
        }
      } else if (auto array = mlir::dyn_cast<ac::ValueArrayType>(aggregate)) {
        llvm::SmallVector<mlir::Type> active;
        auto width = mlirValueBitWidth(element, array.getElementType(), active);
        if (!width)
          return width.takeError();
        lsb = *width * (static_cast<uint64_t>(array.getLength()) -
                        static_cast<uint64_t>(element.getIndex()) - 1);
      }
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(
          element,
          mlir::cast<ac::VarType>(element.getResult().getType())
              .getElementType(),
          active);
      if (!width)
        return width.takeError();
      plan.expressions.back().lsb = lsb;
      plan.expressions.back().width = *width;
      continue;
    }
    if (auto element = mlir::dyn_cast<ac::VarDynamicElementOp>(operation)) {
      if (auto error = append(operation, "array_get_dynamic"))
        return error;
      auto array = mlir::cast<ac::ValueArrayType>(
          mlir::cast<ac::VarType>(element.getAggregate().getType())
              .getElementType());
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(element, array.getElementType(), active);
      if (!width)
        return width.takeError();
      active.clear();
      auto indexWidth = mlirValueBitWidth(
          element,
          mlir::cast<ac::VarType>(element.getIndex().getType())
              .getElementType(),
          active);
      if (!indexWidth)
        return indexWidth.takeError();
      plan.expressions.back().width = *width;
      plan.expressions.back().indexWidth = *indexWidth;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(array.getLength());
      continue;
    }
    if (auto update = mlir::dyn_cast<ac::VarWithElementOp>(operation)) {
      if (auto error = append(operation, "array_update_dynamic"))
        return error;
      auto array = mlir::cast<ac::ValueArrayType>(
          mlir::cast<ac::VarType>(update.getAggregate().getType())
              .getElementType());
      llvm::SmallVector<mlir::Type> active;
      auto width = mlirValueBitWidth(update, array.getElementType(), active);
      if (!width)
        return width.takeError();
      active.clear();
      auto indexWidth = mlirValueBitWidth(
          update,
          mlir::cast<ac::VarType>(update.getIndex().getType()).getElementType(),
          active);
      if (!indexWidth)
        return indexWidth.takeError();
      plan.expressions.back().width = *width;
      plan.expressions.back().indexWidth = *indexWidth;
      plan.expressions.back().selectionCount =
          static_cast<uint64_t>(array.getLength());
      continue;
    }
    if (mlir::isa<ac::VarAddOp>(operation)) {
      if (auto error = append(operation, "add"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarSubOp>(operation)) {
      if (auto error = append(operation, "sub"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarMulOp>(operation)) {
      if (auto error = append(operation, "mul"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarUDivOp>(operation)) {
      if (auto error = append(operation, "udiv"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarURemOp>(operation)) {
      if (auto error = append(operation, "urem"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarAndOp>(operation)) {
      if (auto error = append(operation, "and"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarOrOp>(operation)) {
      if (auto error = append(operation, "or"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarXorOp>(operation)) {
      if (auto error = append(operation, "xor"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarShlOp>(operation)) {
      if (auto error = append(operation, "shl"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarShrOp>(operation)) {
      if (auto error = append(operation, "shr"))
        return error;
      continue;
    }
    if (auto matches = mlir::dyn_cast<ac::VarMatchesOp>(operation)) {
      if (auto error = append(operation, "masked_match"))
        return error;
      unsigned width = mlir::cast<mlir::IntegerType>(
                           mlir::cast<ac::VarType>(matches.getInput().getType())
                               .getElementType())
                           .getWidth();
      plan.expressions.back().mask = exactWidthHex(matches.getMask(), width);
      plan.expressions.back().value = exactWidthHex(matches.getValue(), width);
      continue;
    }
    if (mlir::isa<ac::VarNotOp>(operation)) {
      if (auto error = append(operation, "not"))
        return error;
      continue;
    }
    if (auto priority = mlir::dyn_cast<ac::VarPriorityEncodeOp>(operation)) {
      auto operands = operandNames(priority->getOperands());
      if (!operands)
        return operands.takeError();
      const std::array<std::pair<mlir::Value, llvm::StringRef>, 2> results = {{
          {priority.getIndex(), "priority_index"},
          {priority.getValid(), "priority_valid"},
      }};
      for (auto [resultValue, kind] : results) {
        auto resultType = mlir::dyn_cast<ac::VarType>(resultValue.getType());
        if (!resultType)
          return planError("priority encoder result must be ac.var");
        std::string result =
            prefix.str() + std::to_string(plan.expressions.size());
        values[resultValue] = result;
        QueueExpressionPlan expression{std::move(result),
                                       kind.str(),
                                       printType(resultType.getElementType()),
                                       *operands,
                                       {},
                                       priority.getOrder().str(),
                                       {}};
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (mlir::isa<ac::VarPopcountOp>(operation)) {
      if (auto error = append(operation, "popcount"))
        return error;
      continue;
    }
    if (auto count = mlir::dyn_cast<ac::VarCountZerosOp>(operation)) {
      if (auto error =
              append(operation, "count_zeros", {}, count.getDirection()))
        return error;
      continue;
    }
    if (auto compare = mlir::dyn_cast<ac::VarCmpOp>(operation)) {
      if (auto error = append(operation, "cmp", {}, compare.getPredicate()))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarSelectOp>(operation)) {
      if (auto error = append(operation, "value_select"))
        return error;
      continue;
    }
    if (auto extract = mlir::dyn_cast<ac::VarExtractOp>(operation)) {
      if (auto error = append(operation, "bit_extract"))
        return error;
      plan.expressions.back().lsb = static_cast<uint64_t>(extract.getLsb());
      plan.expressions.back().width = static_cast<uint64_t>(extract.getWidth());
      continue;
    }
    if (mlir::isa<ac::VarConcatOp>(operation)) {
      if (auto error = append(operation, "bit_concat"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarRangeWrapOp>(operation)) {
      if (auto error = append(operation, "range_wrap"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarRangeSaturateOp>(operation)) {
      if (auto error = append(operation, "range_saturate"))
        return error;
      continue;
    }
    if (auto checked = mlir::dyn_cast<ac::VarRangeCheckedOp>(operation)) {
      auto operands = operandNames(checked->getOperands());
      if (!operands)
        return operands.takeError();
      const std::string group = prefix.str() + "range_checked_" +
                                std::to_string(plan.expressions.size());
      const std::array<std::pair<mlir::Value, llvm::StringRef>, 2> results = {{
          {checked.getValue(), "range_checked_value"},
          {checked.getValid(), "range_checked_valid"},
      }};
      for (auto [resultValue, kind] : results) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result =
            prefix.str() + std::to_string(plan.expressions.size());
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kind.str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        expression.field =
            printType(mlir::cast<ac::VarType>(checked.getValue().getType())
                          .getElementType());
        expression.literal = group;
        if (auto target = operation.getAttrOfType<mlir::StringAttr>(
                "ac.static_type_target"))
          expression.staticTypeTarget = target.getValue().str();
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (mlir::isa<ac::VarRangeRefineOp>(operation)) {
      if (auto error = append(operation, "range_refine"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarRangeBitsOp>(operation)) {
      if (auto error = append(operation, "range_bits"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarRangeAddOp>(operation)) {
      if (auto error = append(operation, "range_add"))
        return error;
      continue;
    }
    if (mlir::isa<ac::VarRangeSubOp>(operation)) {
      if (auto error = append(operation, "range_sub"))
        return error;
      continue;
    }
    if (auto compare = mlir::dyn_cast<ac::VarRangeCmpOp>(operation)) {
      if (auto error =
              append(operation, "range_cmp", {}, compare.getPredicate()))
        return error;
      continue;
    }
    if (auto insert = mlir::dyn_cast<ac::VarInsertOp>(operation)) {
      if (auto error = append(operation, "bit_insert"))
        return error;
      plan.expressions.back().lsb = static_cast<uint64_t>(insert.getLsb());
      continue;
    }
    if (auto get = mlir::dyn_cast<ac::VarGetOp>(operation)) {
      if (auto error = append(operation, "get", get.getField()))
        return error;
      continue;
    }
    if (auto with = mlir::dyn_cast<ac::VarWithOp>(operation)) {
      if (auto error = append(operation, "with", with.getField()))
        return error;
      continue;
    }
    if (auto index = mlir::dyn_cast<ac::TableIndexOp>(operation)) {
      if (auto error = append(operation, "table_index"))
        return error;
      plan.expressions.back().table = index.getTable().str();
      continue;
    }
    if (auto get = mlir::dyn_cast<ac::TableGetOp>(operation)) {
      if (auto error = append(operation, "table_get"))
        return error;
      plan.expressions.back().table = get.getTable().str();
      continue;
    }
    if (auto get = mlir::dyn_cast<ac::SlotGetOp>(operation)) {
      const std::string base =
          prefix.str() + std::to_string(plan.expressions.size());
      const std::array<std::pair<mlir::Value, llvm::StringRef>, 2> results = {{
          {get.getValid(), "slot_get_valid"},
          {get.getValue(), "slot_get_value"},
      }};
      for (auto [resultValue, kind] : results) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result =
            base + (kind == "slot_get_valid" ? "_valid" : "_value");
        values[resultValue] = result;
        QueueExpressionPlan expression{
            result, kind.str(), printType(resultType.getElementType()), {}};
        expression.slot = get.getSlot().str();
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto release = mlir::dyn_cast<ac::SlotProposeReleaseOp>(operation)) {
      auto when = values.find(release.getWhen());
      if (when == values.end())
        return planError("slot release guard is not a known firing value");
      plan.slotReleases.push_back({release.getSlot().str(), when->second});
      continue;
    }
    if (auto match = mlir::dyn_cast<ac::TableMatchOp>(operation)) {
      QueueBlockPlan nested;
      llvm::SmallVector<SharedValue> captures;
      for (const auto &entry : values)
        captures.emplace_back(entry.first, entry.second);
      const std::string nestedPrefix =
          prefix.str() + "m" + std::to_string(plan.expressions.size()) + "_";
      if (auto error = extractExpressions(match.getPredicate(), nested, {},
                                          captures, nestedPrefix))
        return error;
      auto resultType = mlir::cast<ac::VarType>(match.getMask().getType());
      std::string result =
          prefix.str() + std::to_string(plan.expressions.size());
      values[match.getMask()] = result;
      QueueExpressionPlan expression{
          result, "table_match", printType(resultType.getElementType()), {}};
      expression.table = match.getTable().str();
      if (match.getDomainBase()) {
        auto found = values.find(match.getDomainBase());
        if (found == values.end())
          return planError("table.match dynamic base is not available");
        expression.domainBase = found->second;
        expression.operands.push_back(expression.domainBase);
      }
      for (const auto &[value, identity] : captures) {
        (void)value;
        const bool used =
            llvm::is_contained(nested.yields, identity) ||
            llvm::any_of(nested.expressions,
                         [&](const QueueExpressionPlan &nestedExpression) {
                           return llvm::is_contained(nestedExpression.operands,
                                                     identity);
                         });
        if (used && !llvm::is_contained(expression.operands, identity))
          expression.operands.push_back(identity);
      }
      expression.nestedExpressions = std::move(nested.expressions);
      expression.nestedYields = std::move(nested.yields);
      extractTableDomain(expression, match);
      expression.sourceProvenance = currentExpressionProvenance;
      plan.expressions.push_back(std::move(expression));
      continue;
    }
    if (auto choose = mlir::dyn_cast<ac::TableChooseOp>(operation)) {
      auto operands = operandNames(choose->getOperands());
      if (!operands)
        return operands.takeError();
      QueueBlockPlan nested;
      llvm::SmallVector<SharedValue> captures;
      if (!choose.getKey().empty()) {
        for (const auto &entry : values)
          captures.emplace_back(entry.first, entry.second);
        const std::string nestedPrefix =
            prefix.str() + "k" + std::to_string(plan.expressions.size()) + "_";
        if (auto error = extractExpressions(choose.getKey(), nested, {},
                                            captures, nestedPrefix))
          return error;
      }
      const uint64_t count = static_cast<uint64_t>(choose.getCount());
      const std::string policy =
          ac::stringifyTableSelectionPolicy(choose.getPolicy()).str();
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(choose.getResults())) {
        const bool indexLane = resultIndex < count;
        const uint64_t lane = indexLane ? resultIndex : resultIndex - count;
        const llvm::StringRef kind =
            indexLane ? "table_choose_index" : "table_choose_valid";
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result =
            prefix.str() + std::to_string(plan.expressions.size());
        values[resultValue] = result;
        QueueExpressionPlan expression{result, kind.str(),
                                       printType(resultType.getElementType()),
                                       *operands};
        for (const auto &[value, identity] : captures) {
          (void)value;
          const bool used =
              llvm::is_contained(nested.yields, identity) ||
              llvm::any_of(nested.expressions,
                           [&](const QueueExpressionPlan &nestedExpression) {
                             return llvm::is_contained(
                                 nestedExpression.operands, identity);
                           });
          if (used && !llvm::is_contained(expression.operands, identity))
            expression.operands.push_back(identity);
        }
        expression.table = choose.getTable().str();
        expression.field = choose.getStableId().str();
        expression.predicate = policy;
        expression.selectionCount = count;
        expression.laneOrdinal = lane;
        if (auto ordering = choose.getKeyOrdering())
          expression.keyOrdering =
              ac::stringifyTableKeyOrdering(*ordering).str();
        expression.initialCursor =
            static_cast<uint64_t>(choose.getInitialCursor());
        expression.nestedExpressions = nested.expressions;
        expression.nestedYields = nested.yields;
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto lookup =
            mlir::dyn_cast<ac::VersionedTableLookupOp>(operation)) {
      auto operands = operandNames(lookup->getOperands());
      if (!operands)
        return operands.takeError();
      for (auto [resultIndex, resultValue] :
           llvm::enumerate(lookup->getResults())) {
        auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
        std::string result = resultIdentity(
            operation, prefix.str() + std::to_string(plan.expressions.size()));
        values[resultValue] = result;
        QueueExpressionPlan expression{
            result,
            resultIndex == 0 ? "versioned_lookup_payload"
                             : "versioned_lookup_valid",
            printType(resultType.getElementType()), *operands};
        expression.table = lookup.getTable().str();
        expression.sourceProvenance = currentExpressionProvenance;
        plan.expressions.push_back(std::move(expression));
      }
      continue;
    }
    if (auto proposal = mlir::dyn_cast<ac::TableProposeOp>(operation)) {
      auto operands = operandNames(proposal->getOperands());
      if (!operands)
        return operands.takeError();
      if (operands->size() != 2 && operands->size() != 3)
        return planError(
            "state proposal must contain index, value, and optional presence");
      StateWritePlan write{proposal.getTable().str(),
                           (*operands)[0],
                           (*operands)[1],
                           operands->size() == 3 ? (*operands)[2] : "",
                           proposal.getMode().str(),
                           {}};
      for (mlir::Attribute field : proposal.getWriteFields())
        write.fields.push_back(
            mlir::cast<mlir::StringAttr>(field).getValue().str());
      plan.stateWrites.push_back(std::move(write));
      if (plan.table.empty()) {
        const StateWritePlan &primary = plan.stateWrites.front();
        plan.table = primary.table;
        plan.tableIndex = primary.index;
        plan.tableValue = primary.value;
        plan.writeMode = primary.mode;
        plan.writeFields = primary.fields;
      }
      continue;
    }
    if (auto proposal =
            mlir::dyn_cast<ac::VersionedTableProposeOp>(operation)) {
      auto operands = operandNames(proposal->getOperands());
      if (!operands)
        return operands.takeError();
      if (operands->size() != (proposal.getRefAttempt() ? 6U : 5U))
        return planError("versioned state proposal identity is malformed");
      StateWritePlan write{proposal.getTable().str(),
                           (*operands)[0],
                           (*operands)[1],
                           (*operands)[2],
                           proposal.getMode().str(),
                           {}};
      for (mlir::Attribute field : proposal.getWriteFields())
        write.fields.push_back(
            mlir::cast<mlir::StringAttr>(field).getValue().str());
      write.versionedAction = proposal.getAction().str();
      write.refGeneration = (*operands)[3];
      write.refEpoch = (*operands)[4];
      if (proposal.getRefAttempt())
        write.refAttempt = (*operands)[5];
      auto scope = proposal->getParentOfType<ac::FiringOp>();
      if (!scope) {
        auto rule = proposal->getParentOfType<ac::RuleOp>();
        if (rule)
          write.staleObligationId =
              "no_stale_update:" + proposal.getTable().str() + ":" +
              rule.getStableId().str();
      } else {
        write.staleObligationId =
            "no_stale_update:" + proposal.getTable().str() + ":" +
            scope.getStableId().str();
      }
      plan.stateWrites.push_back(std::move(write));
      if (plan.table.empty()) {
        const StateWritePlan &primary = plan.stateWrites.front();
        plan.table = primary.table;
        plan.tableIndex = primary.index;
        plan.tableValue = primary.value;
        plan.writeMode = primary.mode;
        plan.writeFields = primary.fields;
      }
      continue;
    }
    if (auto snapshot = mlir::dyn_cast<ac::StateSnapshotOp>(operation)) {
      auto operands = operandNames(snapshot->getOperands());
      if (!operands)
        return operands.takeError();
      const bool indexed = static_cast<bool>(snapshot.getIndex());
      if (operands->size() != (indexed ? 2U : 1U))
        return planError("state snapshot operands are malformed");
      llvm::StringRef indexKind =
          snapshot.getIndexKind() == ac::RuleIndexKind::Static
              ? "static"
              : (snapshot.getIndexKind() == ac::RuleIndexKind::Dynamic
                     ? "dynamic"
                     : "all");
      StateReservationPlan reservation{snapshot.getTable().str(),
                                       indexed ? operands->front() : "",
                                       "",
                                       operands->back(),
                                       indexKind.str(),
                                       {}};
      for (mlir::Attribute field : snapshot.getReadFields())
        reservation.fields.push_back(
            mlir::cast<mlir::StringAttr>(field).getValue().str());
      plan.stateReservations.push_back(std::move(reservation));
      continue;
    }
    if (auto snapshotSet = mlir::dyn_cast<ac::StateSnapshotSetOp>(operation)) {
      auto operands = operandNames(snapshotSet->getOperands());
      if (!operands)
        return operands.takeError();
      if (operands->size() != 2)
        return planError("state snapshot-set operands are malformed");
      StateReservationPlan reservation{snapshotSet.getTable().str(),
                                       "",
                                       operands->front(),
                                       operands->back(),
                                       "set",
                                       {}};
      for (mlir::Attribute field : snapshotSet.getReadFields())
        reservation.fields.push_back(
            mlir::cast<mlir::StringAttr>(field).getValue().str());
      plan.stateReservations.push_back(std::move(reservation));
      continue;
    }
    if (auto output = mlir::dyn_cast<ac::FiringOutputOp>(operation)) {
      auto operands = operandNames(output->getOperands());
      if (!operands)
        return operands.takeError();
      if (operands->size() != 2 || output.getOrdinal() < 0)
        return planError("firing output presence is malformed");
      plan.outputPresence.push_back({static_cast<uint64_t>(output.getOrdinal()),
                                     (*operands)[0], (*operands)[1]});
      continue;
    }
    if (auto condition = mlir::dyn_cast<ac::FiringConditionOp>(operation)) {
      auto operands = operandNames(condition->getOperands());
      if (!operands)
        return operands.takeError();
      if (operands->size() != 1 || !plan.guard.empty())
        return planError("firing must contain one closed functional condition");
      plan.guard = operands->front();
      continue;
    }
    llvm::SmallVector<mlir::Value, 2> yielded;
    if (auto yield = mlir::dyn_cast<ac::TransformYieldOp>(operation))
      yielded.append(yield.getValues().begin(), yield.getValues().end());
    else if (auto yield = mlir::dyn_cast<ac::FiringYieldOp>(operation))
      yielded.append(yield.getValues().begin(), yield.getValues().end());
    else if (auto yield = mlir::dyn_cast<ac::RouteYieldOp>(operation))
      yielded.push_back(yield.getSelector());
    else if (auto yield = mlir::dyn_cast<ac::SelectYieldOp>(operation))
      yielded.push_back(yield.getSelector());
    else if (auto yield = mlir::dyn_cast<ac::ReorderYieldOp>(operation))
      yielded.push_back(yield.getKey());
    else if (auto yield = mlir::dyn_cast<ac::DependencyYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::CreditYieldOp>(operation))
      yielded.push_back(yield.getCost());
    else if (auto yield = mlir::dyn_cast<ac::MemoryYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::SlotYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableMatchYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::TableChooseYieldOp>(operation))
      yielded.push_back(yield.getValue());
    else if (auto yield = mlir::dyn_cast<ac::ExpectYieldOp>(operation))
      yielded.push_back(yield.getCondition());
    else if (auto yield = mlir::dyn_cast<ac::FeedbackYieldOp>(operation)) {
      yielded.push_back(yield.getValue());
      yielded.push_back(yield.getContinueValue());
    } else if (auto returned = mlir::dyn_cast<mlir::func::ReturnOp>(operation))
      yielded.append(returned.getOperands().begin(),
                     returned.getOperands().end());
    else
      return planError("unsupported operation in Queue Var region: " +
                       operation.getName().getStringRef());
    sawStructuredYield = true;
    auto names = operandNames(yielded);
    if (!names)
      return names.takeError();
    plan.yields = std::move(*names);
  }
  if (!sawStructuredYield)
    return planError("Queue Var region has no structured yield");
  llvm::sort(plan.outputPresence, [](const OutputPresencePlan &left,
                                     const OutputPresencePlan &right) {
    return left.ordinal < right.ordinal;
  });
  return llvm::Error::success();
}

void collectHelperCalls(const std::vector<QueueExpressionPlan> &expressions,
                        llvm::StringSet<> &names) {
  for (const QueueExpressionPlan &expression : expressions) {
    if (expression.kind == "helper_call")
      names.insert(expression.field);
    collectHelperCalls(expression.nestedExpressions, names);
  }
}

llvm::Error extractHelperPlans(mlir::ModuleOp module, QueueGraphPlan &plan) {
  llvm::StringSet<> requested;
  for (const QueueBlockPlan &block : plan.blocks)
    collectHelperCalls(block.expressions, requested);
  for (const TableMatchPlan &match : plan.tableMatches)
    collectHelperCalls(match.expressions, requested);
  for (const TableSelectionPlan &selection : plan.tableSelections)
    collectHelperCalls(selection.keyExpressions, requested);
  mlir::SymbolTable symbols(module);
  llvm::StringSet<> extracted;
  while (extracted.size() != requested.size()) {
    std::vector<std::string> pending;
    for (const auto &entry : requested)
      if (!extracted.contains(entry.getKey()))
        pending.push_back(entry.getKey().str());
    llvm::sort(pending);
    for (const std::string &name : pending) {
      auto function = symbols.lookup<mlir::func::FuncOp>(name);
      if (!function || function.isExternal() ||
          !function.getBody().hasOneBlock())
        return planError("Queue helper '@" + name +
                         "' is unresolved or has no single body");
      if (auto marker = function->getAttrOfType<mlir::BoolAttr>("ac.inline");
          marker && marker.getValue())
        return planError("residual ac.inline helper reached QueueGraph plan");
      QueueHelperPlan helper;
      helper.name = name;
      auto helperProvenance = extractSourceProvenance(function);
      if (!helperProvenance)
        return helperProvenance.takeError();
      helper.sourceProvenance = std::move(*helperProvenance);
      for (auto [index, type] : llvm::enumerate(function.getArgumentTypes())) {
        auto variable = mlir::dyn_cast<ac::VarType>(type);
        if (!variable)
          return planError("Queue helper argument must be ac.var");
        helper.inputNames.push_back("arg" + std::to_string(index));
        helper.inputTypes.push_back(printType(variable.getElementType()));
      }
      for (mlir::Type type : function.getResultTypes()) {
        auto variable = mlir::dyn_cast<ac::VarType>(type);
        if (!variable)
          return planError("Queue helper result must be ac.var");
        helper.resultTypes.push_back(printType(variable.getElementType()));
      }
      QueueBlockPlan body;
      if (auto error = extractExpressions(function.getBody(), body, {}, {},
                                          "h_", helper.inputNames))
        return error;
      if (body.yields.size() != helper.resultTypes.size())
        return planError("Queue helper return arity is malformed");
      helper.expressions = std::move(body.expressions);
      helper.yields = std::move(body.yields);
      collectHelperCalls(helper.expressions, requested);
      plan.helpers.push_back(std::move(helper));
      extracted.insert(name);
    }
  }
  llvm::sort(plan.helpers,
             [](const QueueHelperPlan &left, const QueueHelperPlan &right) {
               return left.name < right.name;
             });
  return llvm::Error::success();
}

using ActivationNode = QueueActivationNodePlan;
using ActivationEdge = QueueActivationEdgePlan;

unsigned activationKindOrder(QueueActivationNodeKind kind) {
  return static_cast<unsigned>(kind);
}

llvm::StringRef activationKindName(QueueActivationNodeKind kind) {
  switch (kind) {
  case QueueActivationNodeKind::InterfaceInput:
    return "interface_input";
  case QueueActivationNodeKind::InterfaceOutput:
    return "interface_output";
  case QueueActivationNodeKind::Queue:
    return "queue";
  case QueueActivationNodeKind::Block:
    return "block";
  case QueueActivationNodeKind::Table:
    return "table";
  case QueueActivationNodeKind::Slot:
    return "slot";
  }
  llvm_unreachable("unknown Queue activation node kind");
}

auto activationNodeKey(const ActivationNode &node) {
  return std::pair{activationKindOrder(node.kind), node.index};
}

auto activationEdgeKey(const ActivationEdge &edge) {
  return std::tuple{activationKindOrder(edge.source.kind), edge.source.index,
                    activationKindOrder(edge.target.kind), edge.target.index};
}

std::optional<ActivationNode> queueActivationNode(const QueueGraphPlan &plan,
                                                  llvm::StringRef name) {
  for (auto [index, input] : llvm::enumerate(plan.interfaceInputs))
    if (input.name == name)
      return ActivationNode{QueueActivationNodeKind::InterfaceInput, index};
  for (auto [index, output] : llvm::enumerate(plan.interfaceOutputs))
    if (output.name == name)
      return ActivationNode{QueueActivationNodeKind::InterfaceOutput, index};
  for (auto [index, queue] : llvm::enumerate(plan.queues))
    if (queue.name == name)
      return ActivationNode{QueueActivationNodeKind::Queue, index};
  return std::nullopt;
}

std::optional<ActivationNode> tableActivationNode(const QueueGraphPlan &plan,
                                                  llvm::StringRef name) {
  for (auto [index, table] : llvm::enumerate(plan.tables))
    if (table.name == name)
      return ActivationNode{QueueActivationNodeKind::Table, index};
  return std::nullopt;
}

std::optional<ActivationNode> slotActivationNode(const QueueGraphPlan &plan,
                                                 llvm::StringRef name) {
  for (auto [index, slot] : llvm::enumerate(plan.slots))
    if (slot.name == name)
      return ActivationNode{QueueActivationNodeKind::Slot, index};
  return std::nullopt;
}

void collectExpressionTables(const QueueExpressionPlan &expression,
                             llvm::StringSet<> &tables) {
  if (!expression.table.empty())
    tables.insert(expression.table);
  for (const QueueExpressionPlan &nested : expression.nestedExpressions)
    collectExpressionTables(nested, tables);
}

struct InferredActivation {
  std::vector<ActivationEdge> wakeEdges;
  std::vector<ActivationEdge> workClosureEdges;
  std::vector<ActivationNode> initial;
};

llvm::Expected<InferredActivation> inferActivation(const QueueGraphPlan &plan) {
  std::vector<ActivationEdge> edges;
  std::vector<ActivationEdge> workClosureEdges;
  std::vector<ActivationNode> initial;
  for (auto [blockIndex, block] : llvm::enumerate(plan.blocks)) {
    if (block.kind == "source")
      continue;
    auto resolveRuleResource = [&](const QueueRuleResourcePlan &resource)
        -> llvm::Expected<ActivationNode> {
      if (resource.kind == "input_queue") {
        if (resource.ordinal >= block.inputs.size())
          return planError("activation input Queue ordinal is out of range");
        auto node = queueActivationNode(plan, block.inputs[resource.ordinal]);
        if (!node)
          return planError("activation input Queue is unresolved");
        return *node;
      }
      if (resource.kind == "output_queue") {
        if (resource.ordinal >= block.outputs.size())
          return planError("activation output Queue ordinal is out of range");
        auto node = queueActivationNode(plan, block.outputs[resource.ordinal]);
        if (!node)
          return planError("activation output Queue is unresolved");
        return *node;
      }
      if (resource.kind == "state") {
        auto node = tableActivationNode(plan, resource.resource);
        if (!node)
          return planError("activation Table is unresolved");
        return *node;
      }
      if (resource.kind == "slot") {
        auto node = slotActivationNode(plan, resource.resource);
        if (!node)
          return planError("activation slot is unresolved");
        return *node;
      }
      return planError("activation resource kind is unsupported");
    };
    std::vector<ActivationNode> wakeSources;
    std::vector<ActivationNode> transactionResources;
    if (block.hasActivationEvidence) {
      for (const QueueRuleResourcePlan &resource : block.activationSources) {
        auto node = resolveRuleResource(resource);
        if (!node)
          return node.takeError();
        wakeSources.push_back(*node);
      }
      for (const QueueRuleResourcePlan &resource : block.transactionResources) {
        auto node = resolveRuleResource(resource);
        if (!node)
          return node.takeError();
        transactionResources.push_back(*node);
      }
    } else {
      for (const std::string &queue : block.inputs) {
        auto node = queueActivationNode(plan, queue);
        if (!node)
          return planError("activation input Queue is unresolved");
        wakeSources.push_back(*node);
      }
      for (const std::string &queue : block.outputs) {
        auto node = queueActivationNode(plan, queue);
        if (!node)
          return planError("activation output Queue is unresolved");
        wakeSources.push_back(*node);
      }
      llvm::StringSet<> referencedTables;
      if (!block.table.empty())
        referencedTables.insert(block.table);
      for (const StateWritePlan &write : block.stateWrites)
        referencedTables.insert(write.table);
      for (const QueueExpressionPlan &expression : block.expressions)
        collectExpressionTables(expression, referencedTables);
      for (const auto &entry : referencedTables) {
        auto node = tableActivationNode(plan, entry.getKey());
        if (!node)
          return planError("activation Table is unresolved");
        wakeSources.push_back(*node);
      }
      transactionResources = wakeSources;
    }
    auto canonicalizeNodes = [](std::vector<ActivationNode> &nodes) {
      llvm::sort(nodes,
                 [](const ActivationNode &left, const ActivationNode &right) {
                   return activationNodeKey(left) < activationNodeKey(right);
                 });
      nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    };
    canonicalizeNodes(wakeSources);
    canonicalizeNodes(transactionResources);
    const ActivationNode worker{QueueActivationNodeKind::Block,
                                uint64_t(blockIndex)};
    for (const ActivationNode &source : wakeSources)
      edges.push_back({source, worker});
    for (const ActivationNode &resource : transactionResources)
      workClosureEdges.push_back({worker, resource});
    const bool initiallyActive = block.hasActivationEvidence
                                     ? block.initiallyActive
                                     : block.inputs.empty() &&
                                           block.kind != "sink" &&
                                           block.kind != "observe";
    if (initiallyActive)
      initial.push_back(worker);
  }
  llvm::sort(edges,
             [](const ActivationEdge &left, const ActivationEdge &right) {
               return activationEdgeKey(left) < activationEdgeKey(right);
             });
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  llvm::sort(workClosureEdges,
             [](const ActivationEdge &left, const ActivationEdge &right) {
               return activationEdgeKey(left) < activationEdgeKey(right);
             });
  workClosureEdges.erase(
      std::unique(workClosureEdges.begin(), workClosureEdges.end()),
      workClosureEdges.end());
  llvm::sort(initial,
             [](const ActivationNode &left, const ActivationNode &right) {
               return activationNodeKey(left) < activationNodeKey(right);
             });
  initial.erase(std::unique(initial.begin(), initial.end()), initial.end());
  return InferredActivation{std::move(edges), std::move(workClosureEdges),
                            std::move(initial)};
}

llvm::Error materializeActivation(QueueGraphPlan &plan) {
  auto inferred = inferActivation(plan);
  if (!inferred)
    return inferred.takeError();
  plan.activationEdges = std::move(inferred->wakeEdges);
  plan.workClosureEdges = std::move(inferred->workClosureEdges);
  plan.initialActivation = std::move(inferred->initial);
  return llvm::Error::success();
}

llvm::Expected<std::vector<QueueRuleResourcePlan>>
extractRuleResources(mlir::Operation *operation, llvm::StringRef name) {
  auto resources = operation->getAttrOfType<mlir::ArrayAttr>(name);
  if (!resources)
    return planError("lowered rule activation evidence is missing");
  std::vector<QueueRuleResourcePlan> result;
  result.reserve(resources.size());
  for (mlir::Attribute attribute : resources) {
    auto record = mlir::dyn_cast<mlir::DictionaryAttr>(attribute);
    auto kind = record ? record.getAs<ac::ActivationResourceKindAttr>("kind")
                       : ac::ActivationResourceKindAttr();
    if (!kind)
      return planError("lowered rule activation resource kind is missing");
    QueueRuleResourcePlan resource;
    switch (kind.getValue()) {
    case ac::ActivationResourceKind::InputQueue:
      resource.kind = "input_queue";
      break;
    case ac::ActivationResourceKind::OutputQueue:
      resource.kind = "output_queue";
      break;
    case ac::ActivationResourceKind::State:
      resource.kind = "state";
      break;
    case ac::ActivationResourceKind::Slot:
      resource.kind = "slot";
      break;
    }
    if (kind.getValue() == ac::ActivationResourceKind::State ||
        kind.getValue() == ac::ActivationResourceKind::Slot) {
      auto symbol = record.getAs<mlir::FlatSymbolRefAttr>("resource");
      if (!symbol)
        return planError("state activation resource is missing");
      resource.resource = symbol.getValue().str();
    } else {
      auto ordinal = record.getAs<mlir::IntegerAttr>("ordinal");
      if (!ordinal || ordinal.getInt() < 0)
        return planError("Queue activation ordinal is missing or invalid");
      resource.ordinal = static_cast<uint64_t>(ordinal.getInt());
    }
    result.push_back(std::move(resource));
  }
  return result;
}

llvm::Error extractRuleActivation(mlir::Operation *operation,
                                  QueueBlockPlan &block) {
  auto sources = extractRuleResources(operation, "ac.activation_sources");
  if (!sources)
    return sources.takeError();
  auto transaction =
      extractRuleResources(operation, "ac.transaction_resources");
  if (!transaction)
    return transaction.takeError();
  auto initial =
      operation->getAttrOfType<mlir::BoolAttr>("ac.initially_active");
  if (!initial)
    return planError("lowered rule activation evidence is incomplete");
  block.activationSources = std::move(*sources);
  block.transactionResources = std::move(*transaction);
  block.initiallyActive = initial.getValue();
  block.hasActivationEvidence = true;
  return llvm::Error::success();
}

llvm::Error extractWriterArbitration(mlir::Operation *operation,
                                     QueueBlockPlan &block) {
  auto membership =
      operation->getAttrOfType<mlir::ArrayAttr>("ac.arbitration_membership");
  if (!membership)
    return planError("lowered writer arbitration membership is missing");
  for (mlir::Attribute raw : membership) {
    auto record = mlir::dyn_cast<mlir::DictionaryAttr>(raw);
    auto owner = record ? record.getAs<mlir::FlatSymbolRefAttr>("owner")
                        : mlir::FlatSymbolRefAttr();
    auto endpoint = record
                        ? record.getAs<mlir::StringAttr>("endpoint_stable_id")
                        : mlir::StringAttr();
    auto rank = record ? record.getAs<mlir::IntegerAttr>("declared_rank")
                       : mlir::IntegerAttr();
    auto policy = record
                      ? record.getAs<ac::WriterArbitrationPolicyAttr>("policy")
                      : ac::WriterArbitrationPolicyAttr();
    auto resolution =
        record ? record.getAs<ac::WriterArbitrationResolutionAttr>("resolution")
               : ac::WriterArbitrationResolutionAttr();
    if (!owner || !endpoint || endpoint.getValue().empty() || !rank ||
        rank.getInt() < 0 || !policy ||
        policy.getValue() != ac::WriterArbitrationPolicy::Priority ||
        !resolution ||
        resolution.getValue() !=
            ac::WriterArbitrationResolution::WinnerTakesTransaction)
      return planError("lowered writer arbitration membership is malformed");
    block.arbitrationMembership.push_back(
        {owner.getValue().str(), endpoint.getValue().str(), "priority",
         static_cast<uint64_t>(rank.getInt()), "winner_takes_transaction"});
  }
  return llvm::Error::success();
}

llvm::Error resolveWriterPriorities(QueueGraphPlan &plan) {
  std::vector<size_t> nodes;
  for (auto [index, block] : llvm::enumerate(plan.blocks)) {
    llvm::sort(block.arbitrationMembership,
               [](const QueueWriterArbitrationPlan &left,
                  const QueueWriterArbitrationPlan &right) {
                 return std::tie(left.owner, left.declaredRank,
                                 left.endpointStableId, left.policy,
                                 left.resolution) <
                        std::tie(right.owner, right.declaredRank,
                                 right.endpointStableId, right.policy,
                                 right.resolution);
               });
    if (block.kind == "firing" || block.kind == "table_write" ||
        block.kind == "table_masked_write")
      nodes.push_back(index);
  }
  std::vector<std::vector<bool>> edges(nodes.size(),
                                       std::vector<bool>(nodes.size(), false));
  std::vector<size_t> indegree(nodes.size(), 0);
  for (size_t left = 0; left < nodes.size(); ++left) {
    for (size_t right = left + 1; right < nodes.size(); ++right) {
      const QueueBlockPlan &lhs = plan.blocks[nodes[left]];
      const QueueBlockPlan &rhs = plan.blocks[nodes[right]];
      for (const QueueWriterArbitrationPlan &lhsPolicy :
           lhs.arbitrationMembership) {
        for (const QueueWriterArbitrationPlan &rhsPolicy :
             rhs.arbitrationMembership) {
          if (lhsPolicy.owner != rhsPolicy.owner)
            continue;
          if (lhsPolicy.declaredRank == rhsPolicy.declaredRank)
            return planError(
                "writer arbitration contains an owner-local rank tie");
          const size_t before =
              lhsPolicy.declaredRank < rhsPolicy.declaredRank ? left : right;
          const size_t after = before == left ? right : left;
          if (!edges[before][after]) {
            edges[before][after] = true;
            ++indegree[after];
          }
        }
      }
    }
  }
  auto identity = [&](size_t node) -> llvm::StringRef {
    const QueueBlockPlan &block = plan.blocks[nodes[node]];
    return block.stableId.empty() ? llvm::StringRef(block.name)
                                  : llvm::StringRef(block.stableId);
  };
  auto precedes = [&](size_t left, size_t right) {
    const QueueBlockPlan &lhs = plan.blocks[nodes[left]];
    const QueueBlockPlan &rhs = plan.blocks[nodes[right]];
    return std::tuple{lhs.priority, identity(left)} <
           std::tuple{rhs.priority, identity(right)};
  };
  std::vector<bool> emitted(nodes.size(), false);
  std::vector<size_t> canonicalNodes;
  canonicalNodes.reserve(nodes.size());
  for (uint64_t ordinal = 0; ordinal < nodes.size(); ++ordinal) {
    std::optional<size_t> selected;
    for (size_t node = 0; node < nodes.size(); ++node) {
      if (emitted[node] || indegree[node] != 0)
        continue;
      if (!selected || precedes(node, *selected))
        selected = node;
    }
    if (!selected)
      return planError("writer arbitration precedence contains a cycle");
    emitted[*selected] = true;
    plan.blocks[nodes[*selected]].priority = ordinal;
    canonicalNodes.push_back(nodes[*selected]);
    for (size_t successor = 0; successor < nodes.size(); ++successor)
      if (edges[*selected][successor])
        --indegree[successor];
  }
  std::vector<QueueBlockPlan> orderedWriters;
  orderedWriters.reserve(canonicalNodes.size());
  for (size_t index : canonicalNodes)
    orderedWriters.push_back(std::move(plan.blocks[index]));
  for (auto [slot, writer] : llvm::zip_equal(nodes, orderedWriters))
    plan.blocks[slot] = std::move(writer);
  return llvm::Error::success();
}

llvm::Error groupMultiSelectionReads(QueueGraphPlan &plan) {
  std::vector<bool> remove(plan.blocks.size(), false);
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    if (selection.count <= 1)
      continue;
    struct LaneUse {
      bool index = false;
      bool valid = false;
    };
    std::vector<size_t> consumers;
    std::vector<LaneUse> uses(selection.count);
    auto collectUses = [&](auto &&self, const auto &expressions,
                           bool &used) -> void {
      for (const QueueExpressionPlan &expression : expressions) {
        if ((expression.kind == "table_selection_index_ref" ||
             expression.kind == "table_selection_valid_ref") &&
            expression.field == selection.name) {
          used = true;
          if (expression.laneOrdinal < uses.size()) {
            if (expression.kind == "table_selection_index_ref")
              uses[expression.laneOrdinal].index = true;
            else
              uses[expression.laneOrdinal].valid = true;
          }
        }
        self(self, expression.nestedExpressions, used);
      }
    };
    for (auto [blockIndex, block] : llvm::enumerate(plan.blocks)) {
      bool used = false;
      collectUses(collectUses, block.expressions, used);
      if (used)
        consumers.push_back(blockIndex);
    }
    const bool complete = llvm::all_of(
        uses, [](const LaneUse &use) { return use.index && use.valid; });
    if (!consumers.empty() &&
        (!complete || (consumers.size() != selection.count &&
                       !(consumers.size() == 1 &&
                         plan.blocks[consumers.front()].kind == "firing"))))
      return planError("multi-selection consumers must form one complete "
                       "prefix transaction");
    if (consumers.size() == 1 &&
        plan.blocks[consumers.front()].kind == "firing")
      continue;
    if (llvm::any_of(consumers, [&](size_t index) {
          return plan.blocks[index].kind != "table_read";
        }))
      return planError(
          "multi-selection consumer kind cannot form one commit group");
    std::vector<std::pair<uint64_t, size_t>> lanes;
    for (auto [blockIndex, block] : llvm::enumerate(plan.blocks)) {
      if (block.kind != "table_read" || !block.inputs.empty() ||
          block.table != selection.table || block.yields.size() != 2)
        continue;
      auto index = llvm::find_if(
          block.expressions, [&](const QueueExpressionPlan &expression) {
            return expression.result == block.yields[0] &&
                   expression.kind == "table_selection_index_ref" &&
                   expression.field == selection.name;
          });
      auto valid = llvm::find_if(
          block.expressions, [&](const QueueExpressionPlan &expression) {
            return expression.result == block.yields[1] &&
                   expression.kind == "table_selection_valid_ref" &&
                   expression.field == selection.name;
          });
      if (index == block.expressions.end() || valid == block.expressions.end())
        continue;
      if (index->selectionCount != selection.count ||
          valid->selectionCount != selection.count ||
          index->laneOrdinal != valid->laneOrdinal)
        return planError("multi-selection TableRead lane metadata is invalid");
      lanes.emplace_back(index->laneOrdinal, blockIndex);
    }
    if (lanes.empty())
      continue;
    llvm::sort(lanes);
    if (lanes.size() != selection.count)
      return planError(
          "multi-selection TableRead must consume the complete lane set");
    for (auto [expected, lane] : llvm::enumerate(lanes))
      if (lane.first != expected)
        return planError(
            "multi-selection TableRead lanes must be contiguous from zero");
    QueueBlockPlan grouped = plan.blocks[lanes.front().second];
    grouped.kind = "table_read_group";
    grouped.name = selection.stableId;
    grouped.selection = selection.name;
    grouped.selectionCount = selection.count;
    grouped.outputs.clear();
    grouped.depths.clear();
    grouped.latencies.clear();
    grouped.expressions.clear();
    grouped.yields.clear();
    for (auto [lane, blockIndex] : lanes) {
      const QueueBlockPlan &member = plan.blocks[blockIndex];
      grouped.outputs.push_back(member.outputs.front());
      grouped.depths.push_back(member.depths.front());
      grouped.latencies.push_back(member.latencies.front());
      if (blockIndex != lanes.front().second)
        remove[blockIndex] = true;
    }
    plan.blocks[lanes.front().second] = std::move(grouped);
  }
  std::vector<QueueBlockPlan> blocks;
  blocks.reserve(plan.blocks.size());
  for (auto [index, block] : llvm::enumerate(plan.blocks))
    if (!remove[index])
      blocks.push_back(std::move(block));
  plan.blocks = std::move(blocks);
  return llvm::Error::success();
}

void materializeCaptureOnlySlots(QueueGraphPlan &plan) {
  for (const SlotPlan &slot : plan.slots) {
    if (llvm::any_of(plan.blocks, [&](const QueueBlockPlan &block) {
          return block.kind == "slot" && block.slot == slot.name;
        }))
      continue;
    QueueBlockPlan capture{
        "slot", slot.name + "_capture", slot.scope, {slot.input}, {}};
    capture.lexicalOrder = plan.blocks.size() + plan.moduleInstances.size();
    capture.slot = slot.name;
    capture.yields = {"release_disabled"};
    QueueExpressionPlan disabled{"release_disabled", "constant", "i1", {}};
    disabled.literal = "false";
    capture.expressions.push_back(std::move(disabled));
    capture.sourceProvenance = slot.sourceProvenance;
    plan.blocks.push_back(std::move(capture));
  }
}


struct AvailableCasePlan {
  std::string definition;
  ac::StaticArgumentsAttr arguments;
  const QueueGraphPlan *body = nullptr;
};

class Extractor {
public:
  explicit Extractor(mlir::ModuleOp module) : module(module) {}

  llvm::Error extractModuleFamilies() {
    plan.moduleFamilies.clear();
    for (ac::ModuleOp definition : module.getOps<ac::ModuleOp>()) {
      ModuleFamilyPlan family;
      family.definition = definition.getSymName().str();
      family.source = definition.getSource();
      family.parameters = definition.getSchema().getParameters();
      family.declaredCases = definition.getSchema().getCases();
      family.interface = definition.getSchema().getInterface();
      family.nominalDeclarations =
          definition.getSchema().getNominalDeclarations();
      for (mlir::Attribute rawNominal : family.nominalDeclarations) {
        auto nominal = mlir::cast<mlir::FlatSymbolRefAttr>(rawNominal);
        bool found = false;
        for (ac::TypeScopeOp scope : module.getOps<ac::TypeScopeOp>()) {
          for (ac::EnumOp declaration :
               scope.getBody().front().getOps<ac::EnumOp>()) {
            if (declaration.getSymName() != nominal.getValue())
              continue;
            family.nominalDefinitions.push_back(
                {NominalDefinitionPlan::Kind::Enum,
                 scope.getSymName().str(), declaration.getSymName().str(),
                 scope->getAttr("dlti.dl_spec"),
                 declaration->getAttrOfType<ac::StaticParametersAttr>(
                     "parameters"),
                 declaration.getEnumerantsAttr(), declaration.getValuesAttr(),
                 declaration.getEncodingWidthAttr()});
            found = true;
          }
          for (ac::StructOp declaration :
               scope.getBody().front().getOps<ac::StructOp>()) {
            if (declaration.getSymName() != nominal.getValue())
              continue;
            family.nominalDefinitions.push_back(
                {NominalDefinitionPlan::Kind::Struct,
                 scope.getSymName().str(), declaration.getSymName().str(),
                 scope->getAttr("dlti.dl_spec"),
                 declaration->getAttrOfType<ac::StaticParametersAttr>(
                     "parameters"),
                 declaration.getFieldsAttr(), {}, {}});
            found = true;
          }
        }
        if (!found)
          return planError("nominal family declaration is not defined in a typed scope");
      }
      for (ac::ModuleCaseOp moduleCase :
           definition.getBody().front().getOps<ac::ModuleCaseOp>()) {
        ModuleCasePlan item;
        item.arguments = moduleCase.getArguments();
        item.concreteSignature = moduleCase.getFunctionType();
        item.sourceProvenance = moduleCase.getSourceProvenance();
        auto materialized = ac::materializeModuleInterface(
            family.interface, item.arguments, item.concreteSignature, module);
        if (!materialized)
          return materialized.takeError();
        item.materializedInterface = *materialized;
        auto recordName = [](mlir::Operation *operation,
                             std::vector<std::string> &target) {
          if (auto symbol = mlir::SymbolTable::getSymbolName(operation))
            target.push_back(symbol.getValue().str());
          else if (auto name =
                       operation->getAttrOfType<mlir::StringAttr>("ac.name"))
            target.push_back(name.getValue().str());
        };
        moduleCase.getBody().walk([&](mlir::Operation *operation) {
          if (mlir::isa<ac::QueueOp>(operation))
            recordName(operation, item.queues);
          else if (mlir::isa<ac::TableOp>(operation))
            recordName(operation, item.tables);
          else if (mlir::isa<ac::SlotOp>(operation))
            recordName(operation, item.slots);
          else if (mlir::isa<ac::RuleOp>(operation))
            recordName(operation, item.rules);
          else if (mlir::isa<ac::RequireOp, ac::EnsureOp>(operation))
            recordName(operation, item.proofs);
          else if (mlir::isa<ac::ArchitectureObligationOp>(operation))
            recordName(operation, item.obligations);
          if (mlir::isa<ac::StateOp, ac::QueueOp, ac::TableOp, ac::SlotOp>(
                  operation))
            recordName(operation, item.stateOwners);
        });
        family.cases.push_back(std::move(item));
      }
      plan.moduleFamilies.push_back(std::move(family));
    }
    return llvm::Error::success();
  }

  llvm::Expected<QueueGraphPlan> run() {
    if (mlir::failed(mlir::verify(module)))
      return planError("QueueGraph input failed operation verification");
    auto modelKind = module->getAttrOfType<mlir::StringAttr>("ac.model_kind");
    if (!modelKind || modelKind.getValue() != "queue_graph")
      return planError("module requires ac.model_kind exactly 'queue_graph'");
    if (auto error = extractAggregateTypes())
      return std::move(error);
    if (auto error = extractModuleFamilies())
      return std::move(error);
    if (!module.getOps<ac::SystemOp>().empty())
      return runStructured();
    for (mlir::Operation &operation : module.getBody()->getOperations()) {
      if (mlir::isa<ac::SystemOp, ac::ModuleOp, ac::ModuleExternOp>(operation))
        return planError(
            "structured system/module declaration is not legal in QueueGraph");
    }
    mlir::Operation *unclosed = nullptr;
    module.walk([&](mlir::Operation *operation) {
      if (!unclosed &&
          mlir::isa<ac::RuleOp, ac::TypeConstraintMarkerOp,
                    ac::ValueFactMarkerOp, ac::PendingObligationMarkerOp,
                    ac::VarDeclOp, ac::VarReadOp, ac::VarAssignOp,
                    ac::VarReadElementOp, ac::VarAssignElementOp,
                    ac::RuleConditionOp>(operation))
        unclosed = operation;
    });
    if (unclosed)
      return planError("unresolved rule or typed marker reached QueueGraph");
    mlir::LogicalResult loweredRuleProof = mlir::success();
    module.walk([&](ac::TransformOp transform) {
      if (mlir::failed(loweredRuleProof))
        return;
      loweredRuleProof = ac::verifyLoweredRuleTransformContract(transform);
    });
    if (mlir::failed(loweredRuleProof))
      return planError("lowered-rule proof verification failed");
    if (mlir::failed(acir::verifyFrozenFlatQueueGraph(module)))
      return planError("QueueGraph requires verified topology closure");
    auto system = module->getAttrOfType<mlir::StringAttr>("ac.system");
    if (!system || system.getValue().empty())
      return planError("module requires non-empty ac.system");
    plan.system = system.getValue().str();
    if (auto error = extractDefinitionSource(module, plan))
      return std::move(error);
    if (auto error = extractNdfMetadata(module, "ac.ndf_ids", plan.ndfIds))
      return std::move(error);
    if (auto error =
            extractNdfMetadata(module, "ac.ndf_requires", plan.ndfRequires))
      return std::move(error);
    if (auto error = extractBlock(*module.getBody(), {}))
      return std::move(error);
    if (auto error = extractHelperPlans(module, plan))
      return std::move(error);
    if (auto error = groupMultiSelectionReads(plan))
      return std::move(error);
    materializeCaptureOnlySlots(plan);
    if (auto error = resolveWriterPriorities(plan))
      return std::move(error);
    if (auto error = materializeActivation(plan))
      return std::move(error);
    if (auto error = validateGraph())
      return std::move(error);
    return std::move(plan);
  }

private:
  llvm::Expected<QueueGraphPlan> extractDefinition(
      ac::ModuleOp definition,
      ac::StaticArgumentsAttr caseArguments, llvm::StringRef system,
      llvm::ArrayRef<AvailableCasePlan> available = {}) {
    Extractor nested(module);
    nested.plan.system = system.str();
    nested.plan.definition = definition.getSymName().str();
    auto sourceDefinition =
        definition->getAttrOfType<mlir::StringAttr>("ac.definition_name");
    nested.plan.sourceDefinition =
        sourceDefinition && !sourceDefinition.getValue().empty()
            ? sourceDefinition.getValue().str()
            : definition.getSymName().str();
    if (auto error = extractDefinitionSource(definition, nested.plan))
      return std::move(error);
    if (auto error =
            extractNdfMetadata(definition, "ac.ndf_ids", nested.plan.ndfIds))
      return std::move(error);
    if (auto error = extractNdfMetadata(definition, "ac.ndf_requires",
                                        nested.plan.ndfRequires))
      return std::move(error);
    nested.plan.payloads = plan.payloads;
    nested.plan.enums = plan.enums;
    nested.plan.aggregates = plan.aggregates;
    if (auto error = nested.extractModuleFamilies())
      return std::move(error);
    nested.availableCases.assign(available.begin(), available.end());

    ac::ModuleCaseOp selectedCase;
    for (ac::ModuleCaseOp candidate :
         definition.getBody().front().getOps<ac::ModuleCaseOp>())
      if (candidate.getArguments() == caseArguments) {
        selectedCase = candidate;
        break;
      }
    if (!selectedCase)
      return planError("requested family case body is missing");
    mlir::Block &body = selectedCase.getBody().front();
    auto displayNames = [&](llvm::StringRef attribute, size_t count,
                            llvm::StringRef fallbackPrefix)
        -> llvm::Expected<std::vector<std::string>> {
      std::vector<std::string> result;
      auto values = definition->getAttrOfType<mlir::ArrayAttr>(attribute);
      if (!values)
        values = selectedCase->getAttrOfType<mlir::ArrayAttr>(attribute);
      if (!values) {
        if (definition->hasAttr(attribute))
          return planError(attribute + " must be an array of strings");
        for (size_t index = 0; index < count; ++index)
          result.push_back(fallbackPrefix.str() + "_" + std::to_string(index));
        return result;
      }
      if (values.size() != count)
        return planError(attribute + " must match the module interface arity");
      for (mlir::Attribute value : values) {
        auto name = mlir::dyn_cast<mlir::StringAttr>(value);
        if (!name || name.getValue().empty())
          return planError(attribute + " must contain only non-empty strings");
        result.push_back(name.getValue().str());
      }
      return result;
    };
    auto inputDisplayNames =
        displayNames("ac.input_display_names", body.getNumArguments(), "input");
    if (!inputDisplayNames)
      return inputDisplayNames.takeError();
    for (auto [index, argument] : llvm::enumerate(body.getArguments())) {
      auto queue = mlir::dyn_cast<ac::QueueType>(argument.getType());
      if (!queue)
        return planError("module interface input must be ac.queue");
      std::string name = "input_" + std::to_string(index);
      nested.names[argument] = name;
      nested.plan.interfaceInputs.push_back(
          {name, printType(queue.getElementType()),
           static_cast<uint64_t>(queue.getLanes()),
           static_cast<uint64_t>(queue.getRate()),
           (*inputDisplayNames)[index]});
      nested.plan.queues.push_back(
          {name, printType(queue.getElementType()), "/",
           static_cast<uint64_t>(queue.getRate()), 1,
           static_cast<uint64_t>(queue.getRate()),
           static_cast<uint64_t>(queue.getLanes())});
      for (uint64_t lane = 0;
           lane < static_cast<uint64_t>(queue.getLanes()); ++lane)
        nested.plan.queues.back().laneOrdinals.push_back(lane);
    }
    if (auto error = nested.extractBlock(body, {}))
      return std::move(error);
    if (auto table =
            definition->getAttrOfType<mlir::ArrayAttr>("ac.arch_expression_table")) {
      for (mlir::Attribute rawScope : table) {
        auto scope = mlir::cast<mlir::DictionaryAttr>(rawScope);
        QueueArchitectureExpressionScopePlan plannedScope;
        plannedScope.rule =
            scope.getAs<mlir::StringAttr>("rule").getValue().str();
        if (auto ownerRule = scope.getAs<mlir::StringAttr>("owner_rule"))
          plannedScope.ownerRule = ownerRule.getValue().str();
        for (mlir::Attribute rawNode : scope.getAs<mlir::ArrayAttr>("nodes")) {
          auto node = mlir::cast<mlir::DictionaryAttr>(rawNode);
          auto attributes = node.getAs<mlir::DictionaryAttr>("attributes");
          QueueArchitectureExpressionNodePlan plannedNode;
          plannedNode.opcode =
              ac::stringifyRuleExpressionOpcode(
                  node.getAs<ac::RuleExpressionOpcodeAttr>("opcode").getValue())
                  .str();
          plannedNode.type =
              printType(node.getAs<mlir::TypeAttr>("result_type").getValue());
          for (int64_t operand :
               node.getAs<mlir::DenseI64ArrayAttr>("operands").asArrayRef())
            plannedNode.operands.push_back(static_cast<uint64_t>(operand));
          if (auto value = attributes.getAs<mlir::IntegerAttr>("ordinal")) {
            plannedNode.inputOrdinal = value.getValue().getZExtValue();
            plannedNode.hasInputOrdinal = true;
          }
          if (auto value = attributes.getAs<mlir::IntegerAttr>("value")) {
            plannedNode.literal = value.getValue().getZExtValue();
            plannedNode.hasLiteral = true;
          }
          if (auto value = attributes.getAs<mlir::StringAttr>("operation"))
            plannedNode.operation = value.getValue().str();
          if (auto value = attributes.getAs<mlir::StringAttr>("predicate"))
            plannedNode.predicate = value.getValue().str();
          plannedNode.attributes = printAttribute(attributes);
          plannedScope.nodes.push_back(std::move(plannedNode));
        }
        nested.plan.architectureExpressionScopes.push_back(
            std::move(plannedScope));
      }
    }
    for (ac::ArchitectureObligationOp obligation :
         body.getOps<ac::ArchitectureObligationOp>()) {
      if (obligation.getStatus() ==
          ac::ArchitectureObligationStatus::Proved) {
        auto elision = buildProvedObligationElision(
            obligation, definition.getSymName());
        if (!elision)
          return elision.takeError();
        nested.plan.provedObligationElisions.push_back(std::move(*elision));
        continue;
      }
      QueueArchitectureObligationPlan item;
      item.module = definition.getSymName().str();
      item.symbol = obligation.getSymName().str();
      item.id = obligation.getId().str();
      item.kind =
          ac::stringifyArchitectureObligationKind(obligation.getKind()).str();
      item.severity =
          ac::stringifyArchitectureObligationSeverity(obligation.getSeverity())
              .str();
      item.status =
          ac::stringifyArchitectureObligationStatus(obligation.getStatus())
              .str();
      item.message = obligation.getMessage().str();
      auto obligationProvenance = extractSourceProvenance(obligation);
      if (!obligationProvenance)
        return obligationProvenance.takeError();
      item.sourceProvenance = std::move(*obligationProvenance);
      item.conditionRule =
          obligation.getCondition().getAs<mlir::StringAttr>("rule").getValue().str();
      item.conditionTable = obligation.getCondition()
                                .getAs<mlir::StringAttr>("table")
                                .getValue()
                                .str();
      item.conditionRoot = static_cast<uint64_t>(
          obligation.getCondition().getAs<mlir::IntegerAttr>("node").getInt());
      auto sampling = obligation.getSampling();
      item.sampling = printAttribute(sampling);
      item.samplingKind = ac::stringifyArchitectureSamplingKind(
                              sampling.getAs<ac::ArchitectureSamplingKindAttr>("kind")
                                  .getValue())
                              .str();
      item.samplingEdge = ac::stringifyArchitectureSamplingEdge(
                              sampling.getAs<ac::ArchitectureSamplingEdgeAttr>("edge")
                                  .getValue())
                              .str();
      if (auto anchor = sampling.getAs<mlir::StringAttr>("sample_anchor"))
        item.sampleAnchor = anchor.getValue().str();
      item.monitorOnly = sampling.getAs<mlir::BoolAttr>("monitor_only").getValue();
      if (auto latency = sampling.getAs<mlir::IntegerAttr>("capture_latency"))
        item.captureLatency = latency.getValue().getZExtValue();
      auto readRef = [&](llvm::StringRef name, std::string &rule,
                         std::optional<uint64_t> &root) {
        if (auto ref = sampling.getAs<mlir::DictionaryAttr>(name)) {
          rule = ref.getAs<mlir::StringAttr>("rule").getValue().str();
          root = static_cast<uint64_t>(
              ref.getAs<mlir::IntegerAttr>("node").getInt());
        }
      };
      readRef("active_predicate", item.activeRule, item.activeRoot);
      readRef("reset_recovery_disable", item.disableRule, item.disableRoot);
      for (mlir::Attribute source : obligation.getSourceRules())
        item.sourceRules.push_back(
            mlir::cast<mlir::StringAttr>(source).getValue().str());
      for (mlir::Attribute stateOwner : obligation.getStateOwners()) {
        std::string text;
        llvm::raw_string_ostream stream(text);
        stateOwner.print(stream);
        item.stateOwners.push_back(std::move(text));
      }
      for (mlir::Attribute ndf : obligation.getNdfIds())
        item.ndfIds.push_back(mlir::cast<mlir::StringAttr>(ndf).getValue().str());
      if (obligation.getProofCertificate())
        item.proofCertificate =
            printAttribute(*obligation.getProofCertificate());
      for (mlir::Attribute rawTarget : obligation.getRuntimeTargets())
        item.targets.push_back(
            ac::stringifyArchitectureRuntimeTarget(
                mlir::cast<ac::ArchitectureRuntimeTargetAttr>(rawTarget)
                    .getValue())
                .str());
      if (obligation.getStatus() ==
          ac::ArchitectureObligationStatus::RuntimeChecked) {
        auto materialization = mlir::cast<mlir::DictionaryAttr>(
            obligation.getMaterializations()[0]);
        item.firing =
            materialization.getAs<mlir::StringAttr>("firing").getValue().str();
        item.inputOrdinal = static_cast<uint64_t>(
            materialization.getAs<mlir::IntegerAttr>("input_ordinal").getInt());
        item.maximum = static_cast<uint64_t>(
            materialization.getAs<mlir::IntegerAttr>("maximum").getInt());
        for (llvm::StringRef target : item.targets)
          item.materializations.push_back(
              runtimeMaterializationSignature(item, target));
      }
      nested.plan.architectureObligations.push_back(std::move(item));
    }
    llvm::sort(
        nested.plan.architectureObligations,
        [](const auto &left, const auto &right) { return left.id < right.id; });
    llvm::sort(nested.plan.provedObligationElisions,
               [](const auto &left, const auto &right) {
                 return left.id < right.id;
               });
    if (auto error = extractHelperPlans(module, nested.plan))
      return std::move(error);
    if (auto error = groupMultiSelectionReads(nested.plan))
      return std::move(error);
    materializeCaptureOnlySlots(nested.plan);
    if (auto error = resolveWriterPriorities(nested.plan))
      return std::move(error);
    auto returned = mlir::dyn_cast<ac::ReturnOp>(body.getTerminator());
    if (!returned)
      return planError("module definition must terminate with ac.return");
    auto outputDisplayNames = displayNames("ac.output_display_names",
                                           returned.getNumOperands(), "output");
    if (!outputDisplayNames)
      return outputDisplayNames.takeError();
    for (auto [index, value] : llvm::enumerate(returned.getOperands())) {
      auto name = queueName(value, nested.names);
      if (!name)
        return name.takeError();
      auto queue = mlir::cast<ac::QueueType>(value.getType());
      nested.plan.interfaceOutputs.push_back(
          {*name, printType(queue.getElementType()),
           static_cast<uint64_t>(queue.getLanes()),
           static_cast<uint64_t>(queue.getRate()),
           (*outputDisplayNames)[index]});
    }
    if (auto error = materializeActivation(nested.plan))
      return std::move(error);
    if (available.empty())
      if (auto error = verifyQueueGraphPlan(nested.plan))
        return std::move(error);
    return std::move(nested.plan);
  }

  llvm::Expected<QueueGraphPlan> runStructured() {
    if (mlir::failed(acir::verifyFrozenStructuredQueueGraph(module)))
      return planError(
          "QueueGraph requires verified structured topology closure");
    ac::SystemOp selected;
    for (ac::SystemOp system : module.getOps<ac::SystemOp>())
      if (system.getSelected()) {
        selected = system;
        break;
      }
    if (!selected)
      return planError("structured QueueGraph has no selected system");
    mlir::SymbolTable symbols(module);
    auto root = mlir::dyn_cast_or_null<ac::ModuleOp>(
        symbols.lookup(selected.getRootAttr().getValue()));
    if (!root)
      return planError("structured QueueGraph root module is unresolved");

    for (ac::TypeScopeOp typeScope : module.getOps<ac::TypeScopeOp>())
      if (auto error = extractTypeScope(typeScope))
        return std::move(error);

    struct CaseBuild {
      ac::ModuleOp definition;
      ac::StaticArgumentsAttr arguments;
      enum class State { Pending, Visiting, Complete } state = State::Pending;
      std::shared_ptr<QueueGraphPlan> body;
    };
    std::vector<CaseBuild> cases;
    for (ac::ModuleOp definition : module.getOps<ac::ModuleOp>()) {
      if (definition == root)
        continue;
      for (ac::ModuleCaseOp moduleCase :
           definition.getBody().front().getOps<ac::ModuleCaseOp>())
        cases.push_back({definition, moduleCase.getArguments()});
    }
    auto findCase = [&](llvm::StringRef definition,
                        ac::StaticArgumentsAttr arguments) -> CaseBuild * {
      auto found = llvm::find_if(cases, [&](const CaseBuild &candidate) {
        return candidate.definition->getAttrOfType<mlir::StringAttr>(
                   mlir::SymbolTable::getSymbolAttrName()).getValue() ==
                   definition &&
               candidate.arguments == arguments;
      });
      return found == cases.end() ? nullptr : &*found;
    };
    std::function<llvm::Error(CaseBuild &)> buildCase =
        [&](CaseBuild &item) -> llvm::Error {
      if (item.state == CaseBuild::State::Complete)
        return llvm::Error::success();
      if (item.state == CaseBuild::State::Visiting)
        return planError("typed module family dependency graph is cyclic");
      item.state = CaseBuild::State::Visiting;
      ac::ModuleCaseOp selectedCase;
      for (ac::ModuleCaseOp candidate :
           item.definition.getBody().front().getOps<ac::ModuleCaseOp>())
        if (candidate.getArguments() == item.arguments) {
          selectedCase = candidate;
          break;
        }
      if (!selectedCase)
        return planError("declared family case is missing its body");
      llvm::Error dependencyError = llvm::Error::success();
      selectedCase.walk([&](ac::InstanceOp instance) {
        if (dependencyError)
          return;
        CaseBuild *child =
            findCase(instance.getDefinition(), instance.getStaticArgs());
        if (!child) {
          dependencyError = planError(
              "instance does not select one declared typed family case");
          return;
        }
        dependencyError = buildCase(*child);
      });
      if (dependencyError)
        return dependencyError;
      std::vector<AvailableCasePlan> available;
      for (const CaseBuild &candidate : cases)
        if (candidate.state == CaseBuild::State::Complete)
          available.push_back({candidate.definition->getAttrOfType<mlir::StringAttr>(
                                   mlir::SymbolTable::getSymbolAttrName()).getValue().str(),
                               candidate.arguments, candidate.body.get()});
      auto extracted = extractDefinition(item.definition, item.arguments,
                                         selected.getSymName(), available);
      if (!extracted)
        return extracted.takeError();
      item.body =
          std::make_shared<QueueGraphPlan>(std::move(*extracted));
      item.state = CaseBuild::State::Complete;
      return llvm::Error::success();
    };
    for (CaseBuild &item : cases)
      if (auto error = buildCase(item))
        return std::move(error);

    for (CaseBuild &item : cases)
      for (ModuleFamilyPlan &family : item.body->moduleFamilies)
        for (ModuleCasePlan &moduleCase : family.cases) {
          CaseBuild *body = findCase(family.definition, moduleCase.arguments);
          if (body && body->body != item.body)
            moduleCase.bodyPlan = body->body;
        }

    std::vector<AvailableCasePlan> available;
    for (const CaseBuild &item : cases)
      available.push_back({item.definition->getAttrOfType<mlir::StringAttr>(
                               mlir::SymbolTable::getSymbolAttrName()).getValue().str(), item.arguments,
                           item.body.get()});

    auto rootCases = root.getSchema().getCases().getCases();
    if (rootCases.size() != 1)
      return planError("selected root family requires exactly one concrete case");
    auto rootArguments = mlir::cast<ac::StaticArgumentsAttr>(rootCases[0]);
    auto extractedRoot = extractDefinition(root, rootArguments,
                                           selected.getSymName(), available);
    if (!extractedRoot)
      return extractedRoot.takeError();
    for (ModuleFamilyPlan &family : extractedRoot->moduleFamilies) {
      if (family.definition == root.getSymName())
        continue;
      for (ModuleCasePlan &moduleCase : family.cases) {
        CaseBuild *body = findCase(family.definition, moduleCase.arguments);
        if (!body)
          return planError("declared family case body plan is missing");
        moduleCase.bodyPlan = body->body;
      }
    }
    if (auto error = materializeActivation(*extractedRoot))
      return std::move(error);
    if (auto error = verifyQueueGraphPlan(*extractedRoot))
      return std::move(error);
    return std::move(*extractedRoot);
  }

  llvm::Error validateGraph() { return verifyQueueGraphPlan(plan); }

  llvm::Expected<uint64_t>
  valueWidth(mlir::Operation *from, mlir::Type type,
             llvm::SmallVectorImpl<mlir::Type> &active) {
    return mlirValueBitWidth(from, type, active);
  }

  llvm::Error recordAggregateType(mlir::Operation *from, mlir::Type type) {
    llvm::SmallVector<mlir::Type> active;
    auto width = valueWidth(from, type, active);
    if (!width)
      return width.takeError();
    std::string identity = printType(type);
    if (auto tuple = mlir::dyn_cast<mlir::TupleType>(type)) {
      if (!aggregateIdentities.insert(identity).second)
        return llvm::Error::success();
      QueueAggregatePlan aggregate{identity, "tuple", {}, tuple.size(), *width};
      for (mlir::Type element : tuple.getTypes()) {
        aggregate.elements.push_back(printType(element));
        if (mlir::isa<mlir::TupleType, ac::ValueArrayType>(element))
          if (auto error = recordAggregateType(from, element))
            return error;
      }
      plan.aggregates.push_back(std::move(aggregate));
    } else if (auto array = mlir::dyn_cast<ac::ValueArrayType>(type)) {
      if (!aggregateIdentities.insert(identity).second)
        return llvm::Error::success();
      QueueAggregatePlan aggregate{identity,
                                   "array",
                                   {printType(array.getElementType())},
                                   static_cast<uint64_t>(array.getLength()),
                                   *width};
      if (mlir::isa<mlir::TupleType, ac::ValueArrayType>(
              array.getElementType()))
        if (auto error = recordAggregateType(from, array.getElementType()))
          return error;
      plan.aggregates.push_back(std::move(aggregate));
    }
    return llvm::Error::success();
  }

  llvm::Error extractAggregateTypes() {
    llvm::Error error = llvm::Error::success();
    auto inspect = [&](mlir::Operation *operation, mlir::Type type) {
      if (error)
        return;
      if (auto variable = mlir::dyn_cast<ac::VarType>(type))
        type = variable.getElementType();
      else if (auto queue = mlir::dyn_cast<ac::QueueType>(type))
        type = queue.getElementType();
      if (mlir::isa<mlir::TupleType, ac::ValueArrayType>(type))
        error = recordAggregateType(operation, type);
    };
    module.walk([&](mlir::Operation *operation) {
      if (error)
        return;
      for (mlir::Type type : operation->getOperandTypes())
        inspect(operation, type);
      for (mlir::Type type : operation->getResultTypes())
        inspect(operation, type);
      for (mlir::Region &region : operation->getRegions())
        for (mlir::Block &block : region)
          for (mlir::BlockArgument argument : block.getArguments())
            inspect(operation, argument.getType());
    });
    return error;
  }

  llvm::Error extractTypeScope(ac::TypeScopeOp typeScope) {
    for (mlir::Operation &declaration : typeScope.getBody().front()) {
      if (auto enumeration = mlir::dyn_cast<ac::EnumOp>(declaration)) {
        if (!enumIdentities.insert(enumeration.getSymName()).second)
          return planError("enum identities must be unique");
        QueueEnumPlan planEnum{enumeration.getSymName().str(), {}, {}, 0};
        for (mlir::Attribute value : enumeration.getEnumerants())
          planEnum.enumerants.push_back(
              mlir::cast<mlir::StringAttr>(value).getValue().str());
        if (enumeration.getValuesAttr())
          for (mlir::Attribute value : enumeration.getValuesAttr())
            planEnum.values.push_back(
                mlir::cast<mlir::IntegerAttr>(value).getValue().getZExtValue());
        planEnum.width =
            enumeration.getEncodingWidthAttr()
                ? *enumeration.getEncodingWidth()
                : std::max<uint64_t>(
                      1, llvm::Log2_64_Ceil(planEnum.enumerants.size()));
        plan.enums.push_back(std::move(planEnum));
        continue;
      }
      auto structure = mlir::dyn_cast<ac::StructOp>(declaration);
      if (!structure)
        continue;
      if (auto parameters = structure->getAttrOfType<ac::StaticParametersAttr>(
              "parameters");
          parameters && !parameters.getParameters().empty())
        return planError(
            "dependent struct backend emission requires application-aware "
            "payload plans");
      if (!payloadIdentities.insert(structure.getSymName()).second)
        return planError("payload identities must be unique");
      QueuePayloadPlan payload{structure.getSymName().str(), {}};
      for (mlir::Attribute rawField : structure.getFields()) {
        auto field = mlir::dyn_cast<mlir::DictionaryAttr>(rawField);
        auto name =
            field ? field.getAs<mlir::StringAttr>("name") : mlir::StringAttr();
        auto type =
            field ? field.getAs<mlir::TypeAttr>("type") : mlir::TypeAttr();
        if (!name || !type)
          return planError("struct field requires name and type");
        if (mlir::isa<mlir::TupleType, ac::ValueArrayType>(type.getValue()))
          if (auto error = recordAggregateType(structure, type.getValue()))
            return error;
        llvm::SmallVector<mlir::Type> active;
        auto width = valueWidth(structure, type.getValue(), active);
        if (!width)
          return width.takeError();
        payload.fields.push_back(
            {name.getValue().str(), printType(type.getValue()), *width});
      }
      plan.payloads.push_back(std::move(payload));
    }
    return llvm::Error::success();
  }

  llvm::Error addQueue(mlir::Value value, llvm::StringRef name, uint64_t depth,
                       uint64_t latency, uint64_t rate,
                       llvm::ArrayRef<std::string> scope) {
    if (name.empty() || !queueIdentities.insert(name).second)
      return planError("Queue logical identities must be non-empty and unique");
    auto queue = mlir::dyn_cast<ac::QueueType>(value.getType());
    if (!queue || depth == 0 || latency == 0 || rate == 0 || rate > depth ||
        queue.getLanes() <= 0 || queue.getRate() <= 0 ||
        queue.getRate() > queue.getLanes())
      return planError(
          "Queue plan requires typed lanes/rate matching endpoint metadata");
    names[value] = name.str();
    plan.queues.push_back({name.str(), printType(queue.getElementType()),
                           scopePath(scope), depth, latency,
                           static_cast<uint64_t>(queue.getRate())});
    QueuePlan &planned = plan.queues.back();
    planned.lanes = static_cast<uint64_t>(queue.getLanes());
    for (uint64_t lane = 0; lane < planned.lanes; ++lane)
      planned.laneOrdinals.push_back(lane);
    return llvm::Error::success();
  }

  llvm::Error addOutputs(mlir::Operation *op, mlir::ValueRange outputs,
                         llvm::ArrayRef<int64_t> depths,
                         llvm::ArrayRef<int64_t> latencies,
                         llvm::ArrayRef<std::string> scope,
                         std::vector<std::string> &result) {
    auto frozen = outputNames(op, outputs.size());
    if (!frozen)
      return frozen.takeError();
    if (depths.size() != outputs.size() || latencies.size() != outputs.size())
      return planError("Queue output metadata count mismatch");
    llvm::SmallVector<int64_t> defaultRates(outputs.size(), 1);
    llvm::ArrayRef<int64_t> rates = defaultRates;
    if (auto attribute =
            op->getAttrOfType<mlir::DenseI64ArrayAttr>("ac.output_rates"))
      rates = attribute.asArrayRef();
    if (rates.size() != outputs.size())
      return planError("Queue output rate count must match result count");
    for (size_t index = 0; index < outputs.size(); ++index) {
      if (depths[index] <= 0 || latencies[index] <= 0 || rates[index] <= 0 ||
          rates[index] > depths[index])
        return planError("Queue depth/latency must be positive and rate must "
                         "not exceed depth");
      auto error = addQueue(outputs[index], (*frozen)[index], depths[index],
                            latencies[index], rates[index], scope);
      if (error)
        return error;
      if (auto projections = op->getAttrOfType<mlir::ArrayAttr>(
              "ac.payload_projections_out")) {
        for (mlir::Attribute raw : projections) {
          auto record = mlir::dyn_cast<mlir::DictionaryAttr>(raw);
          auto ordinal = record ? record.getAs<mlir::IntegerAttr>("ordinal")
                                : mlir::IntegerAttr();
          if (!ordinal || ordinal.getInt() != static_cast<int64_t>(index))
            continue;
          auto version = record.getAs<mlir::IntegerAttr>("version");
          auto profile = record.getAs<mlir::StringAttr>("profile");
          auto logical = record.getAs<mlir::TypeAttr>("logical_type");
          auto fields = record.getAs<mlir::ArrayAttr>("kept_fields");
          if (!version || !profile || !logical || !fields)
            return planError("payload projection evidence is malformed");
          QueuePayloadProjectionPlan projection;
          projection.version = version.getValue().getZExtValue();
          projection.profile = profile.getValue().str();
          projection.logicalType = printType(logical.getValue());
          projection.carrierType =
              printType(mlir::cast<ac::QueueType>(outputs[index].getType())
                            .getElementType());
          for (mlir::Attribute field : fields) {
            auto name = mlir::dyn_cast<mlir::StringAttr>(field);
            if (!name)
              return planError("payload projection field is not a string");
            projection.keptFields.push_back(name.getValue().str());
          }
          std::optional<llvm::StringRef> logicalName =
              payloadTypeName(projection.logicalType);
          auto logicalPayload = llvm::find_if(
              plan.payloads, [&](const QueuePayloadPlan &candidate) {
                return logicalName && candidate.name == *logicalName;
              });
          auto carrierAggregate = llvm::find_if(
              plan.aggregates, [&](const QueueAggregatePlan &candidate) {
                return candidate.type == projection.carrierType;
              });
          if (!logicalName || logicalPayload == plan.payloads.end() ||
              carrierAggregate == plan.aggregates.end())
            return planError("payload projection cost types are unresolved");
          projection.logicalBits = llvm::accumulate(
              logicalPayload->fields, uint64_t{0},
              [](uint64_t total, const QueuePayloadFieldPlan &field) {
                return total + field.width;
              });
          projection.carrierBits = carrierAggregate->width;
          if (projection.carrierBits >= projection.logicalBits)
            return planError(
                "payload projection does not reduce logical width");
          projection.removedBits =
              projection.logicalBits - projection.carrierBits;
          plan.queues.back().payloadProjection = std::move(projection);
          break;
        }
      }
    }
    result = std::move(*frozen);
    return llvm::Error::success();
  }

  void appendBlock(QueueBlockPlan block) {
    block.lexicalOrder = nextLexicalOrder++;
    mergeSourceProvenance(block.sourceProvenance, currentSourceProvenance);
    plan.blocks.push_back(std::move(block));
  }

  llvm::Error extractBlock(mlir::Block &block, std::vector<std::string> scope) {
    for (mlir::Operation &operation : block) {
      auto provenance = extractSourceProvenance(&operation);
      if (!provenance)
        return provenance.takeError();
      currentSourceProvenance = std::move(*provenance);
      if (mlir::isa<ac::ArchitectureObligationOp>(operation))
        continue;
      if (mlir::isa<ac::RecoveryDomainOp, ac::TypedIdentityOp,
                    ac::CheckpointOp, ac::RetainedResultOp>(operation))
        continue;
      if (auto typeScope = mlir::dyn_cast<ac::TypeScopeOp>(operation)) {
        if (auto error = extractTypeScope(typeScope))
          return error;
        continue;
      }
      if (auto instance = mlir::dyn_cast<ac::MemoryInstanceOp>(operation)) {
        MemoryInstancePlan instancePlan{
            instance.getSymName().str(),     printType(instance.getDataType()),
            uint64_t(instance.getEntries()), uint64_t(instance.getInit()),
            uint64_t(instance.getLatency()), instance.getStableId().str(),
            instance.getOwner().str()};
        instancePlan.sourceProvenance = currentSourceProvenance;
        plan.memoryInstances.push_back(std::move(instancePlan));
        continue;
      }
      if (auto table = mlir::dyn_cast<ac::TableOp>(operation)) {
        TablePlan tablePlan;
        tablePlan.name = table.getSymName().str();
        tablePlan.entryType = printType(table.getEntryType());
        tablePlan.entries = static_cast<uint64_t>(table.getEntries());
        tablePlan.init = static_cast<uint64_t>(table.getInit());
        if (auto shape = table.getShape())
          for (int64_t extent : *shape)
            tablePlan.shape.push_back(static_cast<uint64_t>(extent));
        else
          tablePlan.shape.push_back(tablePlan.entries);
        if (auto widths = table.getAxisWidths())
          for (int64_t width : *widths)
            tablePlan.axisWidths.push_back(static_cast<uint64_t>(width));
        else
          tablePlan.axisWidths.push_back(
              std::max<uint64_t>(1, llvm::Log2_64_Ceil(tablePlan.entries)));
        tablePlan.layout =
            table.getLayout() ? table.getLayout()->str() : "row_major";
        tablePlan.layoutVersion =
            table.getLayoutVersion()
                ? static_cast<uint64_t>(*table.getLayoutVersion())
                : uint64_t{1};
        if (auto initVersion = table.getInitVersion())
          tablePlan.initVersion = static_cast<uint64_t>(*initVersion);
        if (auto initImage = table.getInitImage()) {
          for (mlir::Attribute value : *initImage) {
            auto initial = extractTableInitValue(table.getOperation(),
                                                 table.getEntryType(), value);
            if (!initial)
              return initial.takeError();
            tablePlan.initImage.push_back(std::move(*initial));
          }
        }
        tablePlan.hasTypedSchema = table.getShapeAttr() != nullptr;
        tablePlan.stableId = table.getStableId().str();
        tablePlan.ownerPath = table.getOwner().str();
        if (auto domain = table.getRecoveryDomainAttr()) {
          tablePlan.versioned = true;
          tablePlan.recoveryDomain = domain.getValue().str();
          tablePlan.identity = table.getIdentityAttr().getValue().str();
          if (auto checkpoint = table.getCheckpointAttr())
            tablePlan.checkpoint = checkpoint.getValue().str();
          if (auto retained = table.getRetainedResultAttr())
            tablePlan.retainedResult = retained.getValue().str();
          tablePlan.generationBits =
              static_cast<uint64_t>(table.getGenerationBitsAttr().getInt());
          tablePlan.epochBits =
              static_cast<uint64_t>(table.getEpochBitsAttr().getInt());
          if (auto bits = table.getAttemptBitsAttr())
            tablePlan.attemptBits = static_cast<uint64_t>(bits.getInt());
          tablePlan.validField = table.getValidFieldAttr().getValue().str();
          tablePlan.generationField =
              table.getGenerationFieldAttr().getValue().str();
          tablePlan.epochField = table.getEpochFieldAttr().getValue().str();
          if (auto field = table.getAttemptFieldAttr())
            tablePlan.attemptField = field.getValue().str();
          tablePlan.payloadField =
              table.getPayloadFieldAttr().getValue().str();
        }
        tablePlan.sourceProvenance = currentSourceProvenance;
        plan.tables.push_back(std::move(tablePlan));
        continue;
      }
      if (auto slot = mlir::dyn_cast<ac::SlotOp>(operation)) {
        auto input = queueName(slot.getInput(), names);
        if (!input)
          return input.takeError();
        SlotPlan slotPlan{
            slot.getSymName().str(),
            printType(mlir::cast<ac::QueueType>(slot.getInput().getType())
                          .getElementType()),
            *input,
            scopePath(scope),
            slot.getStableId().str(),
            slot.getOwner().str()};
        slotPlan.sourceProvenance = currentSourceProvenance;
        plan.slots.push_back(std::move(slotPlan));
        continue;
      }
      if (auto match = mlir::dyn_cast<ac::TableMatchOp>(operation)) {
        if (match.getDomainBase())
          return planError(
              "dynamic table.match cannot become a shared match cache");
        const std::string name =
            "table_match_" + std::to_string(plan.tableMatches.size());
        QueueBlockPlan predicate;
        if (auto error = extractExpressions(match.getPredicate(), predicate))
          return error;
        if (predicate.yields.size() != 1)
          return planError("table.match predicate must yield one value");
        auto resultType = mlir::cast<ac::VarType>(match.getMask().getType());
        TableMatchPlan matchPlan{name,
                                 match.getTable().str(),
                                 scopePath(scope),
                                 printType(resultType.getElementType()),
                                 std::move(predicate.expressions),
                                 predicate.yields.front()};
        extractTableDomain(matchPlan, match);
        matchPlan.sourceProvenance = currentSourceProvenance;
        plan.tableMatches.push_back(std::move(matchPlan));
        QueueExpressionPlan reference{
            "shared_match_" + std::to_string(plan.tableMatches.size() - 1),
            "table_match_ref",
            printType(resultType.getElementType()),
            {}};
        reference.field = name;
        reference.table = match.getTable().str();
        reference.sourceProvenance = currentSourceProvenance;
        sharedExpressions.emplace_back(match.getMask(), std::move(reference));
        continue;
      }
      if (auto choose = mlir::dyn_cast<ac::TableChooseOp>(operation)) {
        auto matchValue = llvm::find_if(
            sharedExpressions, [&](const SharedExpression &candidate) {
              return candidate.first == choose.getMask() &&
                     candidate.second.kind == "table_match_ref";
            });
        if (matchValue == sharedExpressions.end())
          return planError("table.choose requires a shared table.match mask");
        const std::string name =
            "table_selection_" + std::to_string(plan.tableSelections.size());
        QueueBlockPlan key;
        if (choose.getPolicy() != ac::TableSelectionPolicy::First &&
            choose.getPolicy() != ac::TableSelectionPolicy::RoundRobin) {
          if (auto error = extractExpressions(choose.getKey(), key))
            return error;
          if (key.yields.size() != 1)
            return planError("table.choose key must yield one value");
        }
        const uint64_t count = static_cast<uint64_t>(choose.getCount());
        const std::string policy =
            ac::stringifyTableSelectionPolicy(choose.getPolicy()).str();
        auto indexType =
            mlir::cast<ac::VarType>(choose.getResults().front().getType());
        TableSelectionPlan selection{name,
                                     choose.getTable().str(),
                                     scopePath(scope),
                                     matchValue->second.field,
                                     policy,
                                     printType(indexType.getElementType()),
                                     std::move(key.expressions),
                                     key.yields.empty() ? std::string()
                                                        : key.yields.front()};
        selection.count = count;
        if (auto ordering = choose.getKeyOrdering())
          selection.keyOrdering =
              ac::stringifyTableKeyOrdering(*ordering).str();
        selection.stableId = choose.getStableId().str();
        selection.initialCursor =
            static_cast<uint64_t>(choose.getInitialCursor());
        selection.sourceProvenance = currentSourceProvenance;
        plan.tableSelections.push_back(std::move(selection));
        for (auto [resultIndex, resultValue] :
             llvm::enumerate(choose.getResults())) {
          const bool indexLane = resultIndex < count;
          const uint64_t lane = indexLane ? resultIndex : resultIndex - count;
          auto resultType = mlir::cast<ac::VarType>(resultValue.getType());
          QueueExpressionPlan reference{
              "shared_selection_" +
                  std::to_string(plan.tableSelections.size() - 1) +
                  (indexLane ? "_index_" : "_valid_") + std::to_string(lane),
              indexLane ? "table_selection_index_ref"
                        : "table_selection_valid_ref",
              printType(resultType.getElementType()),
              {}};
          reference.field = name;
          reference.table = choose.getTable().str();
          reference.predicate = policy;
          reference.selectionCount = count;
          reference.laneOrdinal = lane;
          reference.sourceProvenance = currentSourceProvenance;
          sharedExpressions.emplace_back(resultValue, std::move(reference));
        }
        continue;
      }
      if (auto source = mlir::dyn_cast<ac::SourceOp>(operation)) {
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                source, source->getResults(), {int64_t(source.getDepth())},
                {int64_t(source.getLatency())}, scope, outputs))
          return error;
        appendBlock({"source",
                     outputs.front(),
                     scopePath(scope),
                     {},
                     outputs,
                     {uint64_t(source.getDepth())},
                     {uint64_t(source.getLatency())}});
        continue;
      }
      if (auto firing = mlir::dyn_cast<ac::FiringOp>(operation)) {
        auto inputs = queueNames(firing.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                firing, firing.getOutputs(),
                firing.getOutputDepthsAttr().asArrayRef(),
                firing.getOutputLatenciesAttr().asArrayRef(), scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"firing",
                                 outputs.empty() ? firing.getStableId().str()
                                                 : outputs.front(),
                                 scopePath(scope), std::move(*inputs), outputs};
        blockPlan.stableId = firing.getStableId().str();
        for (int64_t value : firing.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : firing.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        blockPlan.region = printRegion(firing.getBody());
        if (auto error = extractDisplayProvenance(firing, blockPlan))
          return error;
        auto priority =
            firing->getAttrOfType<mlir::IntegerAttr>("ac.rule_priority");
        if (!priority || priority.getInt() < 0)
          return planError("firing priority is missing or invalid");
        blockPlan.priority = static_cast<uint64_t>(priority.getInt());
        if (auto error = extractExpressions(firing.getBody(), blockPlan))
          return error;
        auto yieldProvenance =
            extractSourceProvenance(firing.getBody().front().getTerminator());
        if (!yieldProvenance)
          return yieldProvenance.takeError();
        mergeSourceProvenance(blockPlan.sourceProvenance, *yieldProvenance);
        if (firing->hasAttr("ac.activation_sources"))
          if (auto error = extractRuleActivation(firing, blockPlan))
            return error;
        if (auto error = extractWriterArbitration(firing, blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto transform = mlir::dyn_cast<ac::TransformOp>(operation)) {
        auto inputs = queueNames(transform.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(transform, transform.getOutputs(),
                           transform.getOutputDepthsAttr().asArrayRef(),
                           transform.getOutputLatenciesAttr().asArrayRef(),
                           scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"transform", outputs.front(), scopePath(scope),
                                 std::move(*inputs), outputs};
        for (int64_t value : transform.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : transform.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        blockPlan.region = printRegion(transform.getBody());
        if (auto error = extractDisplayProvenance(transform, blockPlan))
          return error;
        if (auto error = extractExpressions(transform.getBody(), blockPlan))
          return error;
        auto yieldProvenance = extractSourceProvenance(
            transform.getBody().front().getTerminator());
        if (!yieldProvenance)
          return yieldProvenance.takeError();
        mergeSourceProvenance(blockPlan.sourceProvenance, *yieldProvenance);
        if (transform->hasAttr("ac.activation_sources"))
          if (auto error = extractRuleActivation(transform, blockPlan))
            return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto broadcast = mlir::dyn_cast<ac::BroadcastOp>(operation)) {
        auto input = queueName(broadcast.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(broadcast, broadcast.getOutputs(),
                           broadcast.getOutputDepthsAttr().asArrayRef(),
                           broadcast.getOutputLatenciesAttr().asArrayRef(),
                           scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"broadcast",
                                 "broadcast_" + *input,
                                 scopePath(scope),
                                 {*input},
                                 outputs};
        for (int64_t value : broadcast.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : broadcast.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto fork = mlir::dyn_cast<ac::ForkOp>(operation)) {
        auto input = queueName(fork.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(fork, fork.getOutputs(),
                                    fork.getOutputDepthsAttr().asArrayRef(),
                                    fork.getOutputLatenciesAttr().asArrayRef(),
                                    scope, outputs))
          return error;
        QueueBlockPlan blockPlan{
            "fork", "fork_" + *input, scopePath(scope), {*input}, outputs};
        for (int64_t value : fork.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : fork.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto route = mlir::dyn_cast<ac::RouteOp>(operation)) {
        auto input = queueName(route.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(route, route.getOutputs(),
                                    route.getOutputDepthsAttr().asArrayRef(),
                                    route.getOutputLatenciesAttr().asArrayRef(),
                                    scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"route",
                                 "route_" + outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs};
        for (int64_t value : route.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : route.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        blockPlan.region = printRegion(route.getSelector());
        if (auto error = extractExpressions(route.getSelector(), blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto select = mlir::dyn_cast<ac::SelectOp>(operation)) {
        auto inputs = queueNames(select.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                select, select->getResults(), {int64_t(select.getDepth())},
                {int64_t(select.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"select",
                                 outputs.front(),
                                 scopePath(scope),
                                 std::move(*inputs),
                                 outputs,
                                 {uint64_t(select.getDepth())},
                                 {uint64_t(select.getLatency())}};
        blockPlan.region = printRegion(select.getKey());
        if (auto error = extractExpressions(select.getKey(), blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto merge = mlir::dyn_cast<ac::MergeOp>(operation)) {
        auto inputs = queueNames(merge.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                merge, merge->getResults(), {int64_t(merge.getDepth())},
                {int64_t(merge.getLatency())}, scope, outputs))
          return error;
        appendBlock({"merge",
                     outputs.front(),
                     scopePath(scope),
                     std::move(*inputs),
                     outputs,
                     {uint64_t(merge.getDepth())},
                     {uint64_t(merge.getLatency())},
                     merge.getPolicy().str()});
        continue;
      }
      if (auto barrier = mlir::dyn_cast<ac::BarrierOp>(operation)) {
        auto inputs = queueNames(barrier.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                barrier, barrier.getOutputs(),
                barrier.getOutputDepthsAttr().asArrayRef(),
                barrier.getOutputLatenciesAttr().asArrayRef(), scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"barrier", outputs.front(), scopePath(scope),
                                 std::move(*inputs), outputs};
        for (int64_t value : barrier.getOutputDepths())
          blockPlan.depths.push_back(value);
        for (int64_t value : barrier.getOutputLatencies())
          blockPlan.latencies.push_back(value);
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto reorder = mlir::dyn_cast<ac::ReorderOp>(operation)) {
        auto input = queueName(reorder.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                reorder, reorder->getResults(), {int64_t(reorder.getDepth())},
                {int64_t(reorder.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"reorder",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(reorder.getDepth())},
                                 {uint64_t(reorder.getLatency())}};
        blockPlan.capacity = reorder.getCapacity();
        blockPlan.start = reorder.getStart();
        blockPlan.region = printRegion(reorder.getKey());
        if (auto error = extractExpressions(reorder.getKey(), blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto dependency = mlir::dyn_cast<ac::DependencyOp>(operation)) {
        auto input = queueName(dependency.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(dependency, dependency->getResults(),
                           {int64_t(dependency.getDepth())},
                           {int64_t(dependency.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"dependency",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(dependency.getDepth())},
                                 {uint64_t(dependency.getLatency())}};
        blockPlan.capacity = dependency.getCapacity();
        blockPlan.noDependency = dependency.getNoDependency();
        blockPlan.resources = dependency.getResources();
        if (auto provider = dependency->getAttrOfType<mlir::StringAttr>(
                "ac.schedule_provider"))
          blockPlan.provider = provider.getValue().str();
        blockPlan.region = printRegion(dependency.getKey());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&dependency.getKey(), &dependency.getWaitsFor(),
              &dependency.getResource(), &dependency.getCost()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("dependency policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto credit = mlir::dyn_cast<ac::CreditOp>(operation)) {
        auto input = queueName(credit.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error = addOutputs(
                credit, credit->getResults(), {int64_t(credit.getDepth())},
                {int64_t(credit.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"credit",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(credit.getDepth())},
                                 {uint64_t(credit.getLatency())}};
        blockPlan.credits = credit.getCredits();
        blockPlan.region = printRegion(credit.getCost());
        if (auto error = extractExpressions(credit.getCost(), blockPlan))
          return error;
        if (blockPlan.yields.size() != 1)
          return planError("credit cost must yield one value");
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto memory = mlir::dyn_cast<ac::MemoryRequestOp>(operation)) {
        auto input = queueName(memory.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(memory, memory->getResults(),
                           {int64_t(memory.getDepth())}, {1}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"memory_request",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(memory.getDepth())},
                                 {1}};
        blockPlan.resultField = memory.getResultField().str();
        blockPlan.memoryInstance = memory.getInstance().str();
        blockPlan.endpointOrdinal = memory.getOrdinal();
        blockPlan.region = printRegion(memory.getAddress());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&memory.getAddress(), &memory.getWrite(), &memory.getData()}) {
          if (auto error = extractExpressions(*policy, blockPlan))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("memory policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.memoryRequests.push_back(
            {blockPlan.memoryInstance, blockPlan.name, blockPlan.scope,
             blockPlan.inputs.front(), blockPlan.outputs.front(),
             blockPlan.endpointOrdinal, blockPlan.depths.front(),
             blockPlan.resultField});
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto read = mlir::dyn_cast<ac::TableReadOp>(operation)) {
        std::vector<std::string> inputs;
        if (read.getInput()) {
          auto input = queueName(read.getInput(), names);
          if (!input)
            return input.takeError();
          inputs.push_back(std::move(*input));
        }
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(read, read->getResults(), {int64_t(read.getDepth())},
                           {int64_t(read.getLatency())}, scope, outputs))
          return error;
        auto name = read->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.read requires frozen ac.name");
        QueueBlockPlan blockPlan{"table_read",
                                 name.getValue().str(),
                                 scopePath(scope),
                                 inputs,
                                 outputs,
                                 {uint64_t(read.getDepth())},
                                 {uint64_t(read.getLatency())}};
        blockPlan.table = read.getTable().str();
        std::vector<std::string> policyYields;
        for (mlir::Region *policy : {&read.getAddress(), &read.getWhen()}) {
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("table read policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableReads.push_back(
            {blockPlan.table, blockPlan.name, blockPlan.scope,
             inputs.empty() ? std::string() : inputs.front(), outputs.front(),
             uint64_t(read.getDepth()), uint64_t(read.getLatency())});
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto write = mlir::dyn_cast<ac::TableWriteOp>(operation)) {
        std::vector<std::string> inputs;
        if (write.getInput()) {
          auto input = queueName(write.getInput(), names);
          if (!input)
            return input.takeError();
          inputs.push_back(std::move(*input));
        }
        auto name = write->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.write requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "table_write", name.getValue().str(), scopePath(scope), inputs, {}};
        blockPlan.table = write.getTable().str();
        if (auto endpoint =
                write->getAttrOfType<mlir::StringAttr>("ac.endpoint_id"))
          blockPlan.stableId = endpoint.getValue().str();
        if (auto arbitration = write->getAttrOfType<ac::WriterPriorityAttr>(
                "ac.arbitration")) {
          auto endpoint =
              write->getAttrOfType<mlir::StringAttr>("ac.endpoint_id");
          if (!endpoint || endpoint.getValue().empty())
            return planError(
                "arbitrated table writer stable identity is missing");
          blockPlan.arbitrationMembership.push_back(
              {blockPlan.table, endpoint.getValue().str(), "priority",
               static_cast<uint64_t>(arbitration.getRank()),
               "winner_takes_transaction"});
        }
        blockPlan.writeMode = write.getMode().str();
        for (mlir::Attribute rawField : write.getWriteFields())
          blockPlan.writeFields.push_back(
              mlir::cast<mlir::StringAttr>(rawField).getValue().str());
        std::vector<std::string> policyYields;
        for (mlir::Region *policy :
             {&write.getAddress(), &write.getEnable(), &write.getValue()}) {
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("table write policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableWrites.push_back(
            {blockPlan.table, blockPlan.name, blockPlan.scope,
             inputs.empty() ? std::string() : inputs.front(),
             blockPlan.writeMode, blockPlan.writeFields});
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto write = mlir::dyn_cast<ac::TableMaskedWriteOp>(operation)) {
        auto name = write->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("table.masked_write requires frozen ac.name");
        QueueBlockPlan blockPlan{"table_masked_write",
                                 name.getValue().str(),
                                 scopePath(scope),
                                 {},
                                 {}};
        blockPlan.table = write.getTable().str();
        if (auto endpoint =
                write->getAttrOfType<mlir::StringAttr>("ac.endpoint_id"))
          blockPlan.stableId = endpoint.getValue().str();
        if (auto arbitration = write->getAttrOfType<ac::WriterPriorityAttr>(
                "ac.arbitration")) {
          auto endpoint =
              write->getAttrOfType<mlir::StringAttr>("ac.endpoint_id");
          if (!endpoint || endpoint.getValue().empty())
            return planError(
                "arbitrated masked writer stable identity is missing");
          blockPlan.arbitrationMembership.push_back(
              {blockPlan.table, endpoint.getValue().str(), "priority",
               static_cast<uint64_t>(arbitration.getRank()),
               "winner_takes_transaction"});
        }
        blockPlan.writeMode = write.getMode().str();
        for (mlir::Attribute rawField : write.getWriteFields())
          blockPlan.writeFields.push_back(
              mlir::cast<mlir::StringAttr>(rawField).getValue().str());
        auto matchValue = llvm::find_if(
            sharedExpressions, [&](const SharedExpression &candidate) {
              return candidate.first == write.getMask() &&
                     candidate.second.kind == "table_match_ref";
            });
        if (matchValue == sharedExpressions.end())
          return planError("masked table write requires a shared match mask");
        blockPlan.expressions.push_back(matchValue->second);
        std::vector<std::string> policyYields{matchValue->second.result};
        for (mlir::Region *policy : {&write.getEnable(), &write.getValue()}) {
          if (auto error =
                  extractExpressions(*policy, blockPlan, sharedExpressions))
            return error;
          if (blockPlan.yields.size() != 1)
            return planError("masked table write policy must yield one value");
          policyYields.push_back(blockPlan.yields.front());
        }
        blockPlan.yields = std::move(policyYields);
        plan.tableMaskedWrites.push_back({blockPlan.table, blockPlan.name,
                                          blockPlan.scope, blockPlan.writeMode,
                                          blockPlan.writeFields});
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto release = mlir::dyn_cast<ac::SlotReleaseOp>(operation)) {
        auto slot = llvm::find_if(plan.slots, [&](const SlotPlan &candidate) {
          return candidate.name == release.getSlot();
        });
        if (slot == plan.slots.end())
          return planError("slot.release references unknown slot");
        auto name = release->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("slot.release requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "slot", name.getValue().str(), scopePath(scope), {slot->input}, {}};
        blockPlan.slot = slot->name;
        blockPlan.region = printRegion(release.getWhen());
        if (auto error = extractExpressions(release.getWhen(), blockPlan,
                                            sharedExpressions))
          return error;
        if (blockPlan.yields.size() != 1)
          return planError("slot release policy must yield one value");
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto feedback = mlir::dyn_cast<ac::FeedbackOp>(operation)) {
        auto input = queueName(feedback.getInput(), names);
        if (!input)
          return input.takeError();
        std::vector<std::string> outputs;
        if (auto error =
                addOutputs(feedback, feedback->getResults(),
                           {int64_t(feedback.getDepth())},
                           {int64_t(feedback.getLatency())}, scope, outputs))
          return error;
        QueueBlockPlan blockPlan{"feedback",
                                 outputs.front(),
                                 scopePath(scope),
                                 {*input},
                                 outputs,
                                 {uint64_t(feedback.getDepth())},
                                 {uint64_t(feedback.getLatency())},
                                 "",
                                 uint64_t(feedback.getMaxIterations())};
        blockPlan.region = printRegion(feedback.getBody());
        if (auto error = extractExpressions(feedback.getBody(), blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (auto instance = mlir::dyn_cast<ac::InstanceOp>(operation)) {
        auto selected = llvm::find_if(
            availableCases, [&](const AvailableCasePlan &candidate) {
              return candidate.definition == instance.getDefinition() &&
                     candidate.arguments == instance.getStaticArgs();
            });
        if (selected == availableCases.end())
          return planError(
              "module instance references an unavailable typed family case");
        const QueueGraphPlan *target = selected->body;
        if (!target || target->definition != instance.getDefinition())
          return planError(
              "module instance family definition mismatch");
        auto inputs = queueNames(instance.getInputs(), names);
        if (!inputs)
          return inputs.takeError();
        if (inputs->size() != target->interfaceInputs.size() ||
            instance.getOutputs().size() != target->interfaceOutputs.size())
          return planError("module instance interface arity mismatch");
        for (auto [input, interface] :
             llvm::zip_equal(instance.getInputs(), target->interfaceInputs)) {
          auto queue = mlir::cast<ac::QueueType>(input.getType());
          if (printType(queue.getElementType()) != interface.payloadType ||
              static_cast<uint64_t>(queue.getLanes()) != interface.lanes ||
              static_cast<uint64_t>(queue.getRate()) != interface.rate)
            return planError("module instance input payload mismatch");
        }
        std::vector<std::string> outputs;
        for (auto [index, result] : llvm::enumerate(instance.getOutputs())) {
          const QueueInterfacePlan &interface = target->interfaceOutputs[index];
          auto source =
              llvm::find_if(target->queues, [&](const QueuePlan &queue) {
                return queue.name == interface.name;
              });
          if (source == target->queues.end())
            return planError(
                "module output must be produced by a local Queue operation");
          std::string name = instance.getSymName().str();
          if (instance.getOutputs().size() != 1)
            name += "_" + std::to_string(index);
          if (auto error = addQueue(result, name, source->depth,
                                    source->latency, source->rate, scope))
            return error;
          outputs.push_back(std::move(name));
        }
        QueueModuleInstancePlan plannedInstance{instance.getSymName().str(),
                                                instance.getDefinition().str(),
                                                instance.getStaticArgs(),
                                                scopePath(scope),
                                                std::move(*inputs),
                                                std::move(outputs),
                                                nextLexicalOrder++};
        auto provenance = extractSourceProvenance(instance);
        if (!provenance)
          return provenance.takeError();
        plannedInstance.sourceProvenance = std::move(*provenance);
        plan.moduleInstances.push_back(std::move(plannedInstance));
        continue;
      }
      if (auto nested = mlir::dyn_cast<ac::ScopeOp>(operation)) {
        std::vector<std::string> nestedScope = scope;
        nestedScope.push_back(nested.getSymName().str());
        plan.scopes.push_back(scopePath(nestedScope));
        mlir::Block &body = nested.getBody().front();
        if (body.getNumArguments() != nested.getInputs().size())
          return planError("scope input arity mismatch");
        for (size_t index = 0; index < nested.getInputs().size(); ++index) {
          auto name = queueName(nested.getInputs()[index], names);
          if (!name)
            return name.takeError();
          names[body.getArgument(index)] = std::move(*name);
        }
        if (auto error = extractBlock(body, nestedScope))
          return error;
        auto yield = mlir::dyn_cast<ac::ScopeYieldOp>(body.getTerminator());
        bool invalidYield = !yield;
        if (yield)
          invalidYield = yield.getQueues().size() != nested.getOutputs().size();
        if (invalidYield)
          return planError("scope output arity mismatch");
        for (size_t index = 0; index < nested.getOutputs().size(); ++index) {
          auto name = queueName(yield.getQueues()[index], names);
          if (!name)
            return name.takeError();
          names[nested.getOutputs()[index]] = std::move(*name);
        }
        continue;
      }
      auto sink = mlir::dyn_cast<ac::SinkOp>(operation);
      if (sink) {
        auto input = queueName(sink.getInput(), names);
        if (!input)
          return input.takeError();
        auto name = sink->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("sink requires frozen ac.name");
        appendBlock(
            {"sink", name.getValue().str(), scopePath(scope), {*input}, {}});
        continue;
      }
      auto observe = mlir::dyn_cast<ac::ObserveOp>(operation);
      if (observe) {
        auto input = queueName(observe.getInput(), names);
        if (!input)
          return input.takeError();
        appendBlock({"observe",
                     observe.getName().str(),
                     scopePath(scope),
                     {*input},
                     {}});
        continue;
      }
      auto expect = mlir::dyn_cast<ac::ExpectOp>(operation);
      if (expect) {
        auto input = queueName(expect.getInput(), names);
        if (!input)
          return input.takeError();
        auto name = expect->getAttrOfType<mlir::StringAttr>("ac.name");
        if (!name || name.getValue().empty())
          return planError("expect requires frozen ac.name");
        QueueBlockPlan blockPlan{
            "expect", name.getValue().str(), scopePath(scope), {*input}, {}};
        blockPlan.message = expect.getMessage().str();
        blockPlan.region = printRegion(expect.getPredicate());
        if (auto error = extractExpressions(expect.getPredicate(), blockPlan))
          return error;
        appendBlock(std::move(blockPlan));
        continue;
      }
      if (mlir::isa<ac::ScopeYieldOp>(operation) ||
          operation.hasTrait<mlir::OpTrait::IsTerminator>() ||
          mlir::isa<ac::TypeScopeOp>(operation))
        continue;
      if (operation.getName().getDialectNamespace() == "ac")
        return planError("unsupported ACIR op in QueueGraph plan: " +
                         operation.getName().getStringRef());
    }
    return llvm::Error::success();
  }

  mlir::ModuleOp module;
  QueueGraphPlan plan;
  llvm::DenseMap<mlir::Value, std::string> names;
  std::vector<SharedExpression> sharedExpressions;
  llvm::StringSet<> queueIdentities;
  llvm::StringSet<> payloadIdentities;
  llvm::StringSet<> enumIdentities;
  llvm::StringSet<> aggregateIdentities;
  std::vector<AvailableCasePlan> availableCases;
  QueueSourceProvenancePlan currentSourceProvenance;
  uint64_t nextLexicalOrder = 0;
};

std::optional<llvm::StringRef> payloadTypeName(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.struct<@types::@";
  if (type.starts_with(prefix) && type.ends_with('>'))
    return type.drop_front(prefix.size()).drop_back();
  return std::nullopt;
}

const QueueGraphPlan *findFamilyCaseBody(
    const QueueGraphPlan &plan, llvm::StringRef definition,
    ac::StaticArgumentsAttr arguments) {
  for (const ModuleFamilyPlan &family : plan.moduleFamilies) {
    if (family.definition != definition)
      continue;
    for (const ModuleCasePlan &moduleCase : family.cases)
      if (moduleCase.arguments == arguments)
        return moduleCase.bodyPlan.get();
  }
  return nullptr;
}

std::optional<llvm::StringRef> enumTypeName(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.enum<@types::@";
  if (type.starts_with(prefix) && type.ends_with('>'))
    return type.drop_front(prefix.size()).drop_back();
  return std::nullopt;
}

bool verifyStaticConfigValue(const llvm::json::Value &schema,
                             const llvm::json::Value &value) {
  const llvm::json::Object *schemaObject = schema.getAsObject();
  if (!schemaObject)
    return false;
  auto kind = schemaObject->getString("kind");
  auto version = schemaObject->getInteger("version");
  if (!kind || !version || *version != 1)
    return false;
  if (*kind == "scalar") {
    auto name = schemaObject->getString("name");
    if (!name)
      return false;
    if (*name == "int")
      return value.getAsInteger().has_value();
    if (*name == "bool")
      return value.getAsBoolean().has_value();
    if (*name == "float")
      return value.getAsNumber().has_value();
    if (*name == "str")
      return value.getAsString().has_value();
    return false;
  }
  if (*kind != "config" || !schemaObject->getString("name"))
    return false;
  const llvm::json::Array *fields = schemaObject->getArray("fields");
  const llvm::json::Object *valueObject = value.getAsObject();
  if (!fields || !valueObject || fields->size() != valueObject->size())
    return false;
  llvm::StringSet<> names;
  for (const llvm::json::Value &rawField : *fields) {
    const llvm::json::Object *field = rawField.getAsObject();
    auto name = field ? field->getString("name") : std::nullopt;
    const llvm::json::Value *fieldSchema = field ? field->get("type") : nullptr;
    const llvm::json::Value *fieldValue =
        name ? valueObject->get(*name) : nullptr;
    if (!field || field->size() != 2 || !name || name->empty() ||
        !names.insert(*name).second || !fieldSchema || !fieldValue ||
        !verifyStaticConfigValue(*fieldSchema, *fieldValue))
      return false;
  }
  return true;
}

std::optional<int64_t>
projectStaticConfigInteger(const llvm::json::Value &schema,
                           const llvm::json::Value &value,
                           llvm::StringRef path) {
  const llvm::json::Value *currentSchema = &schema;
  const llvm::json::Value *currentValue = &value;
  llvm::SmallVector<llvm::StringRef> fields;
  path.split(fields, '.');
  if (fields.empty())
    return std::nullopt;
  for (llvm::StringRef name : fields) {
    const llvm::json::Object *schemaObject = currentSchema->getAsObject();
    const llvm::json::Object *valueObject = currentValue->getAsObject();
    const llvm::json::Array *schemaFields =
        schemaObject ? schemaObject->getArray("fields") : nullptr;
    if (!schemaFields || !valueObject)
      return std::nullopt;
    const llvm::json::Value *nextSchema = nullptr;
    for (const llvm::json::Value &rawField : *schemaFields) {
      const llvm::json::Object *field = rawField.getAsObject();
      if (field && field->getString("name") == name) {
        nextSchema = field->get("type");
        break;
      }
    }
    const llvm::json::Value *nextValue = valueObject->get(name);
    if (!nextSchema || !nextValue)
      return std::nullopt;
    currentSchema = nextSchema;
    currentValue = nextValue;
  }
  const llvm::json::Object *leafSchema = currentSchema->getAsObject();
  if (!leafSchema || leafSchema->getString("kind") != "scalar" ||
      leafSchema->getString("name") != "int")
    return std::nullopt;
  return currentValue->getAsInteger();
}


llvm::Error verifyPayloadGraph(const QueueGraphPlan &plan) {
  llvm::StringMap<const QueueEnumPlan *> enums;
  for (const QueueEnumPlan &enumeration : plan.enums) {
    if (enumeration.name.empty() || enumeration.enumerants.empty() ||
        !enums.try_emplace(enumeration.name, &enumeration).second)
      return planError("enum identities must be complete and unique");
    llvm::StringSet<> enumerants;
    for (const std::string &enumerant : enumeration.enumerants)
      if (enumerant.empty() || !enumerants.insert(enumerant).second)
        return planError("enum enumerants must be non-empty and unique");
    const uint64_t expectedWidth = std::max<uint64_t>(
        1, llvm::Log2_64_Ceil(enumeration.enumerants.size()));
    if (enumeration.values.empty() && enumeration.width != expectedWidth)
      return planError("enum encoding width is inconsistent");
    if (!enumeration.values.empty()) {
      if (enumeration.values.size() != enumeration.enumerants.size() ||
          enumeration.width == 0 || enumeration.width > 64)
        return planError("explicit enum encoding shape is inconsistent");
      std::set<uint64_t> values;
      for (uint64_t value : enumeration.values)
        if (!values.insert(value).second ||
            (enumeration.width < 64 &&
             value >= (uint64_t{1} << enumeration.width)))
          return planError("explicit enum encoding values are inconsistent");
    }
  }
  llvm::StringMap<const QueuePayloadPlan *> payloads;
  for (const QueuePayloadPlan &payload : plan.payloads) {
    if (payload.name.empty() ||
        !payloads.try_emplace(payload.name, &payload).second)
      return planError("payload identities must be non-empty and unique");
    llvm::StringSet<> fields;
    for (const QueuePayloadFieldPlan &field : payload.fields) {
      if (field.name.empty() || field.type.empty() ||
          !fields.insert(field.name).second)
        return planError("payload fields must be complete and unique");
      if (std::optional<llvm::StringRef> enumeration = enumTypeName(field.type);
          enumeration && !enums.contains(*enumeration))
        return planError("nested enum payload type is unresolved");
    }
  }

  llvm::StringMap<unsigned> states;
  auto visit = [&](auto &self, llvm::StringRef name) -> llvm::Error {
    const unsigned state = states.lookup(name);
    if (state == 2)
      return llvm::Error::success();
    if (state == 1)
      return planError("nested payload definitions contain a cycle");
    auto payload = payloads.find(name);
    if (payload == payloads.end())
      return planError("nested payload type is unresolved");
    states[name] = 1;
    for (const QueuePayloadFieldPlan &field : payload->getValue()->fields)
      if (std::optional<llvm::StringRef> dependency =
              payloadTypeName(field.type))
        if (auto error = self(self, *dependency))
          return error;
    states[name] = 2;
    return llvm::Error::success();
  };
  for (const QueuePayloadPlan &payload : plan.payloads)
    if (auto error = visit(visit, payload.name))
      return error;

  llvm::StringMap<const QueueAggregatePlan *> aggregates;
  for (const QueueAggregatePlan &aggregate : plan.aggregates)
    if (aggregate.type.empty() || aggregate.width == 0 ||
        aggregate.width > kMaximumPackedValueWidth ||
        !aggregates.try_emplace(aggregate.type, &aggregate).second)
      return planError("aggregate type metadata must be complete and unique");

  llvm::StringSet<> widthActive;
  auto typeWidth = [&](auto &self,
                       llvm::StringRef type) -> llvm::Expected<uint64_t> {
    if (auto width = integerWidth(type))
      return *width;
    if (!widthActive.insert(type).second)
      return planError("recursive aggregate type has no finite width");
    auto finish =
        [&](llvm::Expected<uint64_t> result) -> llvm::Expected<uint64_t> {
      widthActive.erase(type);
      return result;
    };
    if (std::optional<llvm::StringRef> name = enumTypeName(type)) {
      auto found = enums.find(*name);
      return finish(found == enums.end()
                        ? llvm::Expected<uint64_t>(
                              planError("enum type metadata is unresolved"))
                        : llvm::Expected<uint64_t>(found->getValue()->width));
    }
    if (std::optional<llvm::StringRef> name = payloadTypeName(type)) {
      auto found = payloads.find(*name);
      if (found == payloads.end())
        return finish(planError("payload type metadata is unresolved"));
      uint64_t total = 0;
      for (const QueuePayloadFieldPlan &field : found->getValue()->fields) {
        auto width = self(self, field.type);
        if (!width)
          return finish(width.takeError());
        if (*width != field.width)
          return finish(planError("payload field width is inconsistent"));
        auto next = addBitWidths(total, *width);
        if (!next)
          return finish(next.takeError());
        total = *next;
        if (total > kMaximumPackedValueWidth)
          return finish(
              planError("payload width exceeds the backend template domain"));
      }
      return finish(total);
    }
    auto found = aggregates.find(type);
    if (found == aggregates.end())
      return finish(planError("aggregate type metadata is unresolved"));
    const QueueAggregatePlan &aggregate = *found->getValue();
    if (aggregate.kind == "tuple") {
      if (aggregate.length == 0 ||
          aggregate.length != aggregate.elements.size())
        return finish(planError("tuple aggregate metadata is malformed"));
      uint64_t total = 0;
      for (const std::string &element : aggregate.elements) {
        auto width = self(self, element);
        if (!width)
          return finish(width.takeError());
        auto next = addBitWidths(total, *width);
        if (!next)
          return finish(next.takeError());
        total = *next;
      }
      if (total != aggregate.width)
        return finish(planError("tuple aggregate width is inconsistent"));
      return finish(total);
    }
    if (aggregate.kind == "array") {
      if (aggregate.length == 0 || aggregate.elements.size() != 1)
        return finish(planError("value-array metadata is malformed"));
      auto width = self(self, aggregate.elements.front());
      if (!width)
        return finish(width.takeError());
      auto total = multiplyBitWidths(*width, aggregate.length);
      if (!total)
        return finish(total.takeError());
      if (*total != aggregate.width)
        return finish(planError("value-array width is inconsistent"));
      return finish(aggregate.width);
    }
    return finish(planError("aggregate kind is unsupported"));
  };
  for (const QueuePayloadPlan &payload : plan.payloads) {
    auto width =
        typeWidth(typeWidth, "!ac.struct<@types::@" + payload.name + ">");
    if (!width)
      return width.takeError();
  }
  return llvm::Error::success();
}

} // namespace

llvm::Error resolveQueueWriterPriorities(QueueGraphPlan &plan) {
  return resolveWriterPriorities(plan);
}

std::string
inlineTableChoiceContractKey(const QueueExpressionPlan &expression) {
  std::string result;
  auto append = [&](llvm::StringRef value) {
    result.append(std::to_string(value.size())).append(":").append(value.str());
  };
  auto appendExpression = [&](auto &&self,
                              const QueueExpressionPlan &nested) -> void {
    append(nested.result);
    append(nested.kind);
    append(nested.type);
    append(std::to_string(nested.operands.size()));
    for (const std::string &operand : nested.operands)
      append(operand);
    append(nested.field);
    append(nested.predicate);
    append(nested.literal);
    append(nested.table);
    append(nested.slot);
    append(std::to_string(nested.lsb));
    append(std::to_string(nested.width));
    append(std::to_string(nested.indexWidth));
    append(nested.mask);
    append(nested.value);
    append(nested.staticTypeTarget);
    for (uint64_t value : nested.domainAxes)
      append(std::to_string(value));
    for (uint64_t value : nested.domainShape)
      append(std::to_string(value));
    for (uint64_t value : nested.domainStrides)
      append(std::to_string(value));
    append(std::to_string(nested.domainOffset));
    append(nested.domainBase);
    append(nested.hasDomainProjection ? "1" : "0");
    append(std::to_string(nested.nestedYields.size()));
    for (const std::string &yield : nested.nestedYields)
      append(yield);
    append(std::to_string(nested.nestedExpressions.size()));
    for (const QueueExpressionPlan &child : nested.nestedExpressions)
      self(self, child);
  };
  append(expression.table);
  append(expression.field);
  append(std::to_string(expression.operands.size()));
  append(expression.operands.empty() ? llvm::StringRef()
                                     : expression.operands.front());
  append(expression.predicate);
  append(std::to_string(expression.nestedYields.size()));
  for (const std::string &yield : expression.nestedYields)
    append(yield);
  append(std::to_string(expression.nestedExpressions.size()));
  for (const QueueExpressionPlan &nested : expression.nestedExpressions)
    appendExpression(appendExpression, nested);
  for (uint64_t value : expression.domainAxes)
    append(std::to_string(value));
  for (uint64_t value : expression.domainShape)
    append(std::to_string(value));
  for (uint64_t value : expression.domainStrides)
    append(std::to_string(value));
  append(std::to_string(expression.domainOffset));
  append(expression.domainBase);
  append(expression.hasDomainProjection ? "1" : "0");
  append(std::to_string(expression.selectionCount));
  append(expression.keyOrdering);
  append(std::to_string(expression.initialCursor));
  append(expression.staticTypeTarget);
  return result;
}

bool isEffectFreeTableMatchExpression(const QueueExpressionPlan &expression) {
  if (!expression.nestedExpressions.empty() || !expression.nestedYields.empty())
    return false;
  return llvm::StringSwitch<bool>(expression.kind)
      .Cases({"constant", "enum_constant", "get", "masked_match"}, true)
      .Cases({"not", "popcount", "count_zeros", "cmp"}, true)
      .Cases({"value_select", "bit_extract", "aggregate_get", "bit_concat"},
             true)
      .Cases({"tuple_create", "array_create", "record_create", "bit_insert"},
             true)
      .Cases({"with", "add", "sub", "mul", "udiv", "urem"}, true)
      .Cases({"and", "or", "xor", "shl", "shr"}, true)
      .Cases({"priority_index", "priority_valid"}, true)
      .Cases({"range_wrap", "range_saturate", "range_refine", "range_bits",
              "range_add", "range_sub", "range_cmp"},
             true)
      .Cases({"range_checked_value", "range_checked_valid", "array_get_dynamic",
              "array_update_dynamic"},
             true)
      .Default(false);
}

llvm::Expected<QueueGraphPlan> buildQueueGraphPlan(mlir::ModuleOp module) {
  if (mlir::failed(verifyArchitectureObligations(module,
                                                 /*requireClosed=*/true)))
    return planError(
        "architecture-obligation closure failed before QueueGraph extraction");
  return Extractor(module).run();
}

std::string sourceOriginKey(const QueueSourceOriginPlan &origin) {
  std::string key;
  llvm::raw_string_ostream stream(key);
  for (const QueueSourceFramePlan &frame : origin)
    stream << frame.kind << '\0' << frame.file << '\0' << frame.line << '\0'
           << frame.column << '\0' << frame.symbol << '\0';
  return key;
}

llvm::Error
verifySourceProvenancePlan(const QueueSourceProvenancePlan &provenance) {
  std::string previous;
  for (const QueueSourceOriginPlan &origin : provenance.origins) {
    if (origin.empty())
      return planError("source provenance origin must not be empty");
    unsigned previousKindRank = 0;
    bool firstFrame = true;
    for (const QueueSourceFramePlan &frame : origin) {
      unsigned kindRank =
          frame.kind == "statement" || frame.kind == "definition" ? 0
          : frame.kind == "inline_callsite"                       ? 1
          : frame.kind == "instance"                              ? 2
                                                                  : 3;
      if ((frame.kind != "statement" && frame.kind != "definition" &&
           frame.kind != "inline_callsite" && frame.kind != "instance") ||
          !isValidPythonSourcePath(frame.file) || frame.line == 0 ||
          frame.column == 0 || (!firstFrame && kindRank < previousKindRank))
        return planError("source provenance frame is malformed");
      previousKindRank = kindRank;
      firstFrame = false;
    }
    std::string key = sourceOriginKey(origin);
    if (!previous.empty() && previous >= key)
      return planError(
          "source provenance origins must be unique and canonical");
    previous = std::move(key);
  }
  return llvm::Error::success();
}

bool appendProjectionDescriptor(const QueueGraphPlan &plan,
                                llvm::raw_ostream &stream, llvm::StringRef type,
                                llvm::StringSet<> &active) {
  if (auto name = payloadTypeName(type)) {
    auto payload =
        llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
          return candidate.name == *name;
        });
    if (payload == plan.payloads.end() || !active.insert(*name).second)
      return false;
    stream << "struct(@types::@" << *name << "){";
    for (const QueuePayloadFieldPlan &field : payload->fields) {
      stream << field.name.size() << ':' << field.name << '=';
      if (!appendProjectionDescriptor(plan, stream, field.type, active))
        return false;
      stream << ';';
    }
    stream << '}';
    active.erase(*name);
    return true;
  }
  if (auto name = enumTypeName(type)) {
    auto enumeration =
        llvm::find_if(plan.enums, [&](const QueueEnumPlan &candidate) {
          return candidate.name == *name;
        });
    if (enumeration == plan.enums.end())
      return false;
    stream << "enum(@types::@" << *name << ')';
    for (const std::string &member : enumeration->enumerants)
      stream << member << ';';
    for (uint64_t value : enumeration->values)
      stream << value << ';';
    stream << "width="
           << (enumeration->values.empty()
                   ? int64_t{-1}
                   : static_cast<int64_t>(enumeration->width));
    return true;
  }
  auto aggregate =
      llvm::find_if(plan.aggregates, [&](const QueueAggregatePlan &candidate) {
        return candidate.type == type;
      });
  if (aggregate != plan.aggregates.end()) {
    if (aggregate->kind == "tuple") {
      stream << "tuple(";
      for (const std::string &element : aggregate->elements) {
        if (!appendProjectionDescriptor(plan, stream, element, active))
          return false;
        stream << ';';
      }
      stream << ')';
      return true;
    }
    if (aggregate->kind == "array" && aggregate->elements.size() == 1) {
      stream << "array(" << aggregate->length << ',';
      if (!appendProjectionDescriptor(plan, stream, aggregate->elements[0],
                                      active))
        return false;
      stream << ')';
      return true;
    }
    return false;
  }
  stream << type;
  return true;
}

llvm::Error verifyQueueGraphPlan(const QueueGraphPlan &plan) {
  llvm::StringSet<> familyNames;
  bool ownsDefinitionFamily = plan.definition.empty();
  for (const ModuleFamilyPlan &family : plan.moduleFamilies) {
    if (family.definition.empty() ||
        family.definition.find("__") != std::string::npos ||
        !familyNames.insert(family.definition).second || !family.source ||
        !family.parameters || !family.declaredCases || !family.interface ||
        !family.nominalDeclarations)
      return planError("typed module family plan is incomplete or duplicated");
    if (family.nominalDefinitions.size() !=
        family.nominalDeclarations.size())
      return planError(
          "typed module family nominal definition coverage is incomplete");
    for (auto [reference, definition] :
         llvm::zip_equal(family.nominalDeclarations,
                         family.nominalDefinitions)) {
      auto symbol = mlir::cast<mlir::FlatSymbolRefAttr>(reference);
      if (definition.scope.empty() || definition.name != symbol.getValue() ||
          !definition.scopeLayout || !definition.members ||
          (definition.kind == NominalDefinitionPlan::Kind::Struct &&
           (definition.values || definition.encodingWidth)))
        return planError(
            "typed module family nominal definition is malformed or reordered");
    }
    if (family.definition == plan.definition)
      ownsDefinitionFamily = true;
    auto declared = family.declaredCases.getCases();
    if (declared.size() != family.cases.size())
      return planError(
          "typed module family case coverage differs from the source schema");
    llvm::SmallDenseSet<mlir::Attribute> argumentsSeen;
    for (auto [index, moduleCase] : llvm::enumerate(family.cases)) {
      if (!moduleCase.arguments || !moduleCase.concreteSignature ||
          !moduleCase.sourceProvenance || !moduleCase.materializedInterface ||
          moduleCase.arguments != declared[index] ||
          !argumentsSeen.insert(moduleCase.arguments).second)
        return planError(
            "typed module cases must preserve exact declared order and identity");
      size_t materializedInput = 0;
      size_t materializedOutput = 0;
      for (ac::InterfacePortAttr port : moduleCase.materializedInterface
                                           .getPorts()
                                           .getAsRange<ac::InterfacePortAttr>()) {
        auto concrete = mlir::dyn_cast<ac::TypeExprConcreteAttr>(
            port.getLogicalType().getValue());
        if (!concrete)
          return planError(
              "typed module case retains an unmaterialized dependent interface");
        mlir::Type expected;
        if (port.getDirection().getValue() == "input") {
          if (materializedInput >=
              moduleCase.concreteSignature.getNumInputs())
            return planError("typed module case materialized input is excess");
          expected = moduleCase.concreteSignature.getInput(materializedInput++);
        } else {
          if (materializedOutput >=
              moduleCase.concreteSignature.getNumResults())
            return planError("typed module case materialized output is excess");
          expected =
              moduleCase.concreteSignature.getResult(materializedOutput++);
        }
        if (concrete.getType().getValue() != expected)
          return planError(
              "typed module case materialized interface was forged");
      }
      if (materializedInput != moduleCase.concreteSignature.getNumInputs() ||
          materializedOutput != moduleCase.concreteSignature.getNumResults())
        return planError("typed module case materialized interface is incomplete");
      auto validInventory = [](const std::vector<std::string> &values) {
        llvm::StringSet<> seen;
        return llvm::all_of(values, [&](const std::string &value) {
          return !value.empty() && seen.insert(value).second;
        });
      };
      if (!validInventory(moduleCase.queues) ||
          !validInventory(moduleCase.tables) ||
          !validInventory(moduleCase.slots) ||
          !validInventory(moduleCase.rules) ||
          !validInventory(moduleCase.proofs) ||
          !validInventory(moduleCase.obligations) ||
          !validInventory(moduleCase.stateOwners))
        return planError("typed module case local inventory is malformed");
    }
  }
  if (!ownsDefinitionFamily)
    return planError("QueueGraph family case lost its source-owned definition");
  llvm::StringMap<const QueueArchitectureExpressionScopePlan *> archScopes;
  for (const auto &scope : plan.architectureExpressionScopes) {
    if (scope.rule.empty() || !archScopes.try_emplace(scope.rule, &scope).second)
      return planError(
          "architecture expression scopes must be non-empty and unique");
    for (auto [ordinal, node] : llvm::enumerate(scope.nodes)) {
      for (uint64_t operand : node.operands)
        if (operand >= ordinal)
          return planError(
              "architecture expression operand is dangling or cyclic");
      if (node.type.empty() ||
          (node.opcode != "rule_input" && node.opcode != "constant" &&
           node.opcode != "operation"))
        return planError("architecture expression node is malformed");
    }
  }
  auto expression = [&](llvm::StringRef rule, uint64_t root)
      -> const QueueArchitectureExpressionNodePlan * {
    auto scope = archScopes.find(rule);
    if (scope == archScopes.end() || root >= scope->getValue()->nodes.size())
      return nullptr;
    return &scope->getValue()->nodes[root];
  };
  llvm::StringSet<> obligationIds;
  const llvm::StringRef expectedObligationModule =
      plan.definition.empty() ? llvm::StringRef(plan.system)
                              : llvm::StringRef(plan.definition);
  for (const QueueProvedObligationElisionPlan &elision :
       plan.provedObligationElisions) {
    if (expectedObligationModule.empty() ||
        elision.module != expectedObligationModule || elision.id.empty() ||
        !obligationIds.insert(elision.id).second ||
        elision.kind != "single_writer" ||
        elision.reason != "predicate_exclusive" ||
        elision.leftEndpoint.empty() || elision.rightEndpoint.empty() ||
        elision.ownerPath.empty() || elision.ownerStableId.empty() ||
        elision.sourceProvenance.empty())
      return planError("proved obligation elision record is incomplete");
  }
  for (const QueueArchitectureObligationPlan &obligation :
       plan.architectureObligations) {
    if (expectedObligationModule.empty() ||
        obligation.module != expectedObligationModule ||
        obligation.symbol != obligation.id || obligation.id.empty() ||
        !obligationIds.insert(obligation.id).second)
      return planError("architecture obligation ID is empty or duplicated");
    const auto *condition =
        expression(obligation.conditionRule, obligation.conditionRoot);
    const auto conditionScope = archScopes.find(obligation.conditionRule);
    auto uniqueStrings = [](const std::vector<std::string> &values) {
      llvm::StringSet<> seen;
      return llvm::all_of(values, [&](const std::string &value) {
        return !value.empty() && seen.insert(value).second;
      });
    };
    if (obligation.conditionTable != "ac.arch_expression_table" || !condition ||
        condition->type != "!ac.var<i1>")
      return planError(
          "architecture obligation condition is missing or not exact i1");
    if (obligation.sampling.empty() ||
        (obligation.severity != "error" && obligation.severity != "fatal"))
      return planError(
          "architecture obligation sampling or severity is incomplete");
    if (obligation.sourceRules.empty() || obligation.stateOwners.empty() ||
        obligation.sourceProvenance.origins.empty())
      return planError(
          "architecture obligation source or owner linkage is incomplete "
          "(rules=" +
          std::to_string(obligation.sourceRules.size()) + ", owners=" +
          std::to_string(obligation.stateOwners.size()) + ", origins=" +
          std::to_string(obligation.sourceProvenance.origins.size()) + ")");
    if (!uniqueStrings(obligation.sourceRules) ||
        !uniqueStrings(obligation.stateOwners) ||
        !uniqueStrings(obligation.targets) ||
        !uniqueStrings(obligation.ndfIds))
      return planError(
          "architecture obligation typed references are empty or duplicated");
    if (conditionScope == archScopes.end() ||
        (!conditionScope->getValue()->ownerRule.empty() &&
         !llvm::is_contained(obligation.sourceRules,
                             conditionScope->getValue()->ownerRule)))
      return planError(
          "architecture obligation expression scope is not owned by a source "
          "rule");
    if (auto error = verifySourceProvenancePlan(obligation.sourceProvenance))
      return error;
    if (obligation.status == "pending" || obligation.status == "rejected")
      return planError(
          "pending or rejected architecture obligation reached QueueGraph");
    if (obligation.status == "proved")
      return planError(
          "proved obligations must be extracted as closed elision records");
    if (obligation.status != "runtime_checked" || obligation.kind != "range" ||
        !obligation.proofCertificate.empty() ||
        obligation.materializations.size() != 3 ||
        obligation.targets !=
            std::vector<std::string>({"cpp", "gfsim", "sva"}) ||
        obligation.firing.empty())
      return planError(
          "QueueGraph admits only complete cpp/gfsim/sva runtime range "
          "obligations");
    for (auto [index, materialization] :
         llvm::enumerate(obligation.materializations))
      if (materialization != runtimeMaterializationSignature(
                                 obligation, obligation.targets[index]))
        return planError(
            "runtime obligation materialization signature is inconsistent");
    auto firing = llvm::find_if(plan.blocks, [&](const QueueBlockPlan &block) {
      return block.kind == "firing" && block.stableId == obligation.firing;
    });
    if (firing == plan.blocks.end() ||
        obligation.inputOrdinal >= firing->inputs.size())
      return planError("runtime obligation references a missing firing input");
    auto firingInputType = [&](uint64_t ordinal) -> std::optional<std::string> {
      if (ordinal >= firing->inputs.size())
        return std::nullopt;
      const std::string &name = firing->inputs[ordinal];
      auto queue = llvm::find_if(plan.queues, [&](const QueuePlan &candidate) {
        return candidate.name == name;
      });
      if (queue != plan.queues.end())
        return "!ac.var<" + queue->payloadType + ">";
      auto interface = llvm::find_if(
          plan.interfaceInputs, [&](const QueueInterfacePlan &candidate) {
            return candidate.name == name;
          });
      if (interface != plan.interfaceInputs.end())
        return "!ac.var<" + interface->payloadType + ">";
      return std::nullopt;
    };
    auto verifyReachableReference = [&](const std::string &rule,
                                        std::optional<uint64_t> root,
                                        llvm::StringRef label) -> llvm::Error {
      if (!root)
        return llvm::Error::success();
      auto scope = archScopes.find(rule);
      if (scope == archScopes.end() ||
          scope->getValue()->ownerRule != obligation.firing)
        return planError(label.str() +
                         " expression scope is not owned by its firing");
      llvm::SmallDenseSet<uint64_t> visited;
      std::function<llvm::Error(uint64_t)> visit =
          [&](uint64_t ordinal) -> llvm::Error {
        if (ordinal >= scope->getValue()->nodes.size())
          return planError(label.str() + " expression root is dangling");
        if (!visited.insert(ordinal).second)
          return llvm::Error::success();
        const auto &node = scope->getValue()->nodes[ordinal];
        if (node.opcode == "rule_input") {
          auto expected = node.hasInputOrdinal
                              ? firingInputType(node.inputOrdinal)
                              : std::optional<std::string>();
          if (!expected || node.type != *expected)
            return planError(label.str() +
                             " rule_input does not match firing interface");
        }
        for (uint64_t operand : node.operands)
          if (auto error = visit(operand))
            return error;
        return llvm::Error::success();
      };
      return visit(*root);
    };
    if (auto error = verifyReachableReference(
            obligation.conditionRule, obligation.conditionRoot, "condition"))
      return error;
    if (auto error = verifyReachableReference(
            obligation.activeRule, obligation.activeRoot, "active predicate"))
      return error;
    if (auto error = verifyReachableReference(
            obligation.disableRule, obligation.disableRoot,
            "reset/recovery disable"))
      return error;
    if (!llvm::is_contained(obligation.sourceRules, obligation.firing))
      return planError("runtime obligation source-rule linkage is incomplete");
    if (obligation.samplingKind != "pre_publish" ||
        obligation.samplingEdge != "none" ||
        obligation.sampleAnchor != obligation.firing ||
        obligation.captureLatency)
      return planError("runtime obligation sampling is inconsistent");
    auto predicateRefValid = [&](const std::string &rule,
                                 std::optional<uint64_t> root) {
      if (!root)
        return rule.empty();
      const auto *node = expression(rule, *root);
      return node && node->type == "!ac.var<i1>";
    };
    if (!predicateRefValid(obligation.activeRule, obligation.activeRoot) ||
        !predicateRefValid(obligation.disableRule, obligation.disableRoot))
      return planError("runtime obligation predicate reference is invalid");
    if (condition->operation != "ac.var.cmp" ||
        condition->predicate != "ule" || condition->operands.size() != 2)
      return planError("runtime range condition is not exact ule");
    const auto *input =
        expression(obligation.conditionRule, condition->operands[0]);
    const auto *maximum =
        expression(obligation.conditionRule, condition->operands[1]);
    if (!input || !maximum || !input->hasInputOrdinal ||
        input->inputOrdinal != obligation.inputOrdinal ||
        !maximum->hasLiteral || maximum->literal != obligation.maximum)
      return planError(
          "runtime range materialization differs from typed condition");
  }
  auto verifyExpressions = [&](auto &&self,
                               const auto &expressions) -> llvm::Error {
    for (const QueueExpressionPlan &expression : expressions) {
      if (auto error = verifySourceProvenancePlan(expression.sourceProvenance))
        return error;
      if (auto error = self(self, expression.nestedExpressions))
        return error;
    }
    return llvm::Error::success();
  };
  for (const QueueBlockPlan &block : plan.blocks) {
    if (auto error = verifySourceProvenancePlan(block.sourceProvenance))
      return error;
    if (auto error = verifyExpressions(verifyExpressions, block.expressions))
      return error;
  }
  for (const QueueHelperPlan &helper : plan.helpers) {
    if (auto error = verifySourceProvenancePlan(helper.sourceProvenance))
      return error;
    if (auto error = verifyExpressions(verifyExpressions, helper.expressions))
      return error;
  }
  for (const QueueModuleInstancePlan &instance : plan.moduleInstances)
    if (auto error = verifySourceProvenancePlan(instance.sourceProvenance))
      return error;
  for (const MemoryInstancePlan &instance : plan.memoryInstances)
    if (auto error = verifySourceProvenancePlan(instance.sourceProvenance))
      return error;
  for (const TablePlan &table : plan.tables)
    if (auto error = verifySourceProvenancePlan(table.sourceProvenance))
      return error;
  for (const SlotPlan &slot : plan.slots)
    if (auto error = verifySourceProvenancePlan(slot.sourceProvenance))
      return error;
  for (const TableMatchPlan &match : plan.tableMatches)
    if (auto error = verifySourceProvenancePlan(match.sourceProvenance))
      return error;
    else if (auto error =
                 verifyExpressions(verifyExpressions, match.expressions))
      return error;
  for (const TableSelectionPlan &selection : plan.tableSelections)
    if (auto error = verifySourceProvenancePlan(selection.sourceProvenance))
      return error;
    else if (auto error =
                 verifyExpressions(verifyExpressions, selection.keyExpressions))
      return error;
  const bool hasStructure = !plan.definition.empty() || !plan.queues.empty() ||
                            !plan.blocks.empty() ||
                            !plan.moduleInstances.empty();
  if (plan.system.empty() || !hasStructure)
    return planError("QueueGraph plan is incomplete");
  if (auto error = verifyPayloadGraph(plan))
    return error;
  llvm::StringMap<const QueueEnumPlan *> enums;
  for (const QueueEnumPlan &enumeration : plan.enums)
    enums[enumeration.name] = &enumeration;
  auto valueWidth = [&](llvm::StringRef type) -> std::optional<uint64_t> {
    if (auto width = integerWidth(type))
      return *width;
    if (std::optional<llvm::StringRef> name = enumTypeName(type)) {
      auto found = enums.find(*name);
      return found == enums.end()
                 ? std::nullopt
                 : std::optional<uint64_t>(found->getValue()->width);
    }
    if (std::optional<llvm::StringRef> name = payloadTypeName(type)) {
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
            return candidate.name == *name;
          });
      if (payload == plan.payloads.end())
        return std::nullopt;
      return llvm::accumulate(
          payload->fields, uint64_t{0},
          [](uint64_t total, const QueuePayloadFieldPlan &field) {
            return total + field.width;
          });
    }
    auto aggregate = llvm::find_if(plan.aggregates,
                                   [&](const QueueAggregatePlan &candidate) {
                                     return candidate.type == type;
                                   });
    return aggregate == plan.aggregates.end()
               ? std::nullopt
               : std::optional<uint64_t>(aggregate->width);
  };
  for (const QueuePlan &queue : plan.queues) {
    if (!queue.payloadProjection)
      continue;
    const QueuePayloadProjectionPlan &projection = *queue.payloadProjection;
    if (projection.version != 1 ||
        projection.profile != "private_transform_tuple_v1" ||
        projection.carrierType != queue.payloadType ||
        projection.keptFields.empty())
      return planError("Queue payload projection metadata is malformed");
    std::optional<llvm::StringRef> logicalName =
        payloadTypeName(projection.logicalType);
    auto logical =
        llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
          return logicalName && candidate.name == *logicalName;
        });
    auto carrier = llvm::find_if(
        plan.aggregates, [&](const QueueAggregatePlan &candidate) {
          return candidate.type == projection.carrierType;
        });
    if (!logicalName || logical == plan.payloads.end() ||
        carrier == plan.aggregates.end() || carrier->kind != "tuple" ||
        carrier->length != projection.keptFields.size() ||
        carrier->elements.size() != projection.keptFields.size() ||
        projection.keptFields.size() >= logical->fields.size())
      return planError("Queue payload projection types are inconsistent");
    uint64_t logicalBits = llvm::accumulate(
        logical->fields, uint64_t{0},
        [](uint64_t total, const QueuePayloadFieldPlan &field) {
          return total + field.width;
        });
    uint64_t retainedBits = 0;
    if (projection.logicalBits != logicalBits ||
        projection.carrierBits != carrier->width ||
        projection.carrierBits >= projection.logicalBits ||
        projection.removedBits !=
            projection.logicalBits - projection.carrierBits)
      return planError(
          "Queue payload projection cost metadata is inconsistent");
    size_t logicalCursor = 0;
    for (auto [index, fieldName] : llvm::enumerate(projection.keptFields)) {
      while (logicalCursor < logical->fields.size() &&
             logical->fields[logicalCursor].name != fieldName)
        ++logicalCursor;
      if (logicalCursor == logical->fields.size() ||
          logical->fields[logicalCursor].type != carrier->elements[index])
        return planError(
            "Queue payload projection fields are not an exact ordered subset");
      retainedBits += logical->fields[logicalCursor].width;
      ++logicalCursor;
    }
    if (retainedBits != carrier->width)
      return planError("Queue payload projection tuple width is inconsistent");
    size_t producers = 0;
    size_t consumers = 0;
    const QueueBlockPlan *producerBlock = nullptr;
    const QueueBlockPlan *consumerBlock = nullptr;
    for (const QueueBlockPlan &block : plan.blocks) {
      const size_t produced = llvm::count(block.outputs, queue.name);
      const size_t consumed = llvm::count(block.inputs, queue.name);
      producers += produced;
      consumers += consumed;
      if (produced)
        producerBlock = &block;
      if (consumed)
        consumerBlock = &block;
      if ((llvm::is_contained(block.outputs, queue.name) ||
           llvm::is_contained(block.inputs, queue.name)) &&
          block.kind != "transform")
        return planError(
            "projected private Queue may connect only Transform blocks");
    }
    if (producers != 1 || consumers != 1)
      return planError(
          "projected private Queue requires one producer and one consumer");
    if (queue.lanes != 1 || queue.rate != 1 || !producerBlock ||
        !consumerBlock || producerBlock->inputs.size() != 1 ||
        producerBlock->outputs.size() != 1 ||
        consumerBlock->inputs.size() != 1 || consumerBlock->outputs.size() != 1)
      return planError("projected private Queue requires scalar "
                       "single-input/output transforms");
    auto verifyCarrierUses = [&](auto &&self, const auto &expressions) -> bool {
      for (const QueueExpressionPlan &expression : expressions) {
        if (llvm::is_contained(expression.operands, "item") &&
            (expression.kind != "aggregate_get" ||
             expression.operands.size() != 1))
          return false;
        if (!self(self, expression.nestedExpressions))
          return false;
        if (llvm::is_contained(expression.nestedYields, "item"))
          return false;
      }
      return true;
    };
    const bool rootEscape =
        llvm::is_contained(consumerBlock->yields, "item") ||
        consumerBlock->guard == "item" ||
        llvm::any_of(consumerBlock->outputPresence,
                     [](const OutputPresencePlan &output) {
                       return output.value == "item" ||
                              output.present == "item";
                     }) ||
        llvm::any_of(consumerBlock->stateWrites,
                     [](const StateWritePlan &write) {
                       return write.index == "item" || write.value == "item" ||
                              write.present == "item";
                     }) ||
        llvm::any_of(consumerBlock->stateReservations,
                     [](const StateReservationPlan &reservation) {
                       return reservation.index == "item" ||
                              reservation.source == "item" ||
                              reservation.predicate == "item";
                     });
    if (rootEscape ||
        !verifyCarrierUses(verifyCarrierUses, consumerBlock->expressions))
      return planError(
          "projected private Queue carrier has a whole-value escape");
  }
  const bool structured = !plan.definition.empty();
  if (structured && llvm::any_of(plan.queues, [](const QueuePlan &queue) {
        return queue.payloadProjection.has_value();
      }))
    return planError(
        "private Queue payload projections are forbidden in structured plans");
  if (!structured &&
      (!plan.interfaceInputs.empty() || !plan.interfaceOutputs.empty() ||
       !plan.moduleInstances.empty() || !plan.moduleFamilies.empty()))
    return planError("flat QueueGraph cannot carry structured module metadata");
  if (structured) {
    llvm::DenseSet<uint64_t> lexicalOrders;
    for (const QueueBlockPlan &block : plan.blocks)
      lexicalOrders.insert(block.lexicalOrder);
    for (const QueueModuleInstancePlan &instance : plan.moduleInstances)
      lexicalOrders.insert(instance.lexicalOrder);
    const size_t expected = plan.blocks.size() + plan.moduleInstances.size();
    if (lexicalOrders.size() != expected)
      return planError("structured QueueGraph lexical orders must be unique");
    for (uint64_t order = 0; order < expected; ++order)
      if (!lexicalOrders.contains(order))
        return planError(
            "structured QueueGraph lexical orders must be dense from zero");
  }

  llvm::StringSet<> queueNames;
  llvm::StringMap<const MemoryInstancePlan *> memoryInstances;
  llvm::StringMap<const TablePlan *> tables;
  for (const MemoryInstancePlan &instance : plan.memoryInstances) {
    if (instance.name.empty() ||
        !memoryInstances.try_emplace(instance.name, &instance).second)
      return planError(
          "memory instance identities must be non-empty and unique");
    if (instance.dataType.empty() || instance.entries == 0 ||
        instance.init != 0 || instance.latency == 0 ||
        instance.stableId.empty() || instance.ownerPath.empty())
      return planError("memory instance metadata is incomplete");
  }
  llvm::StringMap<llvm::DenseSet<uint64_t>> endpointOrdinals;
  for (const MemoryRequestPlan &request : plan.memoryRequests) {
    if (!memoryInstances.contains(request.instance))
      return planError("memory request references unknown instance '" +
                       request.instance + "'");
    if (!endpointOrdinals[request.instance].insert(request.ordinal).second)
      return planError("memory request endpoint ordinals must be unique");
  }
  for (const auto &entry : memoryInstances)
    if (!endpointOrdinals.contains(entry.getKey()))
      return planError("memory instance '" + entry.getKey() +
                       "' has no request endpoints");
  for (const auto &entry : endpointOrdinals)
    for (uint64_t ordinal = 0; ordinal < entry.getValue().size(); ++ordinal)
      if (!entry.getValue().contains(ordinal))
        return planError("memory request endpoint ordinals must be contiguous "
                         "from zero");
  auto tableTypeSupportsZero = [&](auto &&self, llvm::StringRef type,
                                   llvm::StringSet<> &active) -> bool {
    if (bitsWidth(type) || enumTypeName(type))
      return true;
    if (auto bounds = rangeBounds(type))
      return bounds->first == 0;
    if (!active.insert(type).second)
      return false;
    bool supported = false;
    if (auto name = payloadTypeName(type)) {
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
            return candidate.name == *name;
          });
      supported = payload != plan.payloads.end() &&
                  llvm::all_of(payload->fields,
                               [&](const QueuePayloadFieldPlan &field) {
                                 return self(self, field.type, active);
                               });
    } else {
      auto aggregate = llvm::find_if(plan.aggregates,
                                     [&](const QueueAggregatePlan &candidate) {
                                       return candidate.type == type;
                                     });
      supported =
          aggregate != plan.aggregates.end() &&
          llvm::all_of(aggregate->elements, [&](llvm::StringRef element) {
            return self(self, element, active);
          });
    }
    active.erase(type);
    return supported;
  };
  auto tableTypeContainsRange = [&](auto &&self, llvm::StringRef type,
                                    llvm::StringSet<> &active) -> bool {
    if (rangeBounds(type))
      return true;
    if (!active.insert(type).second)
      return false;
    bool found = false;
    if (auto name = payloadTypeName(type)) {
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
            return candidate.name == *name;
          });
      found = payload != plan.payloads.end() &&
              llvm::any_of(payload->fields,
                           [&](const QueuePayloadFieldPlan &field) {
                             return self(self, field.type, active);
                           });
    } else {
      auto aggregate = llvm::find_if(plan.aggregates,
                                     [&](const QueueAggregatePlan &candidate) {
                                       return candidate.type == type;
                                     });
      found = aggregate != plan.aggregates.end() &&
              llvm::any_of(aggregate->elements, [&](llvm::StringRef element) {
                return self(self, element, active);
              });
    }
    active.erase(type);
    return found;
  };
  for (const TablePlan &table : plan.tables) {
    if (table.name.empty() || !tables.try_emplace(table.name, &table).second)
      return planError("table identities must be non-empty and unique");
    if (table.entryType.empty() || table.entries == 0 ||
        table.stableId.empty() || table.ownerPath.empty())
      return planError("table metadata is incomplete");
    llvm::StringSet<> zeroActive;
    llvm::StringSet<> rangeActive;
    const auto scalarBounds =
        constraintBounds(planTypeConstraint(table.entryType));
    if ((table.initImage.empty() && table.init == 0 &&
         !tableTypeSupportsZero(tableTypeSupportsZero, table.entryType,
                                zeroActive)) ||
        (table.initImage.empty() && table.init != 0 &&
         (!scalarBounds || table.init < scalarBounds->first ||
          table.init > scalarBounds->second)) ||
        (!table.initImage.empty() &&
         tableTypeContainsRange(tableTypeContainsRange, table.entryType,
                                rangeActive)))
      return planError("bounded Table initializer contract is unsupported");
    const bool legacyShape =
        !table.hasTypedSchema && table.initVersion == 0 &&
        table.initImage.empty() &&
        (table.shape.empty() ||
         (table.shape.size() == 1 && table.shape.front() == table.entries));
    const llvm::ArrayRef<uint64_t> shape =
        legacyShape ? llvm::ArrayRef<uint64_t>(&table.entries, 1)
                    : llvm::ArrayRef<uint64_t>(table.shape);
    const uint64_t legacyWidth =
        std::max<uint64_t>(1, llvm::Log2_64_Ceil(table.entries));
    const llvm::ArrayRef<uint64_t> axisWidths =
        legacyShape ? llvm::ArrayRef<uint64_t>(&legacyWidth, 1)
                    : llvm::ArrayRef<uint64_t>(table.axisWidths);
    if ((!legacyShape &&
         (table.axisWidths.size() != table.shape.size() ||
          table.layout != "row_major" || table.layoutVersion != 1)))
      return planError("table shape/layout metadata is incomplete");
    uint64_t flattened = 1;
    for (auto [extent, width] : llvm::zip_equal(shape, axisWidths)) {
      if (extent == 0 ||
          flattened > std::numeric_limits<uint64_t>::max() / extent ||
          width != std::max<uint64_t>(1, llvm::Log2_64_Ceil(extent)))
        return planError("table shape/axis metadata is invalid");
      flattened *= extent;
    }
    if (flattened != table.entries ||
        (table.initImage.empty() ? table.initVersion != 0
                                 : table.initVersion != 1 ||
                                       table.initImage.size() != table.entries))
      return planError("table flattened shape or typed init image is invalid");
    if (table.versioned) {
      if (table.entries > 256 || table.recoveryDomain.empty() ||
          table.identity.empty() || table.generationBits == 0 ||
          table.generationBits > 64 || table.epochBits == 0 ||
          table.epochBits > 64 || table.validField.empty() ||
          table.generationField.empty() || table.epochField.empty() ||
          table.payloadField.empty() ||
          ((table.attemptBits == 0) != table.attemptField.empty()))
        return planError("versioned Table metadata is incomplete");
      auto payloadName = payloadTypeName(table.entryType);
      auto payload = llvm::find_if(
          plan.payloads, [&](const QueuePayloadPlan &candidate) {
            return payloadName && candidate.name == *payloadName;
          });
      if (!payloadName || payload == plan.payloads.end())
        return planError("versioned Table entry payload is unresolved");
      auto fieldWidth = [&](llvm::StringRef name) -> std::optional<uint64_t> {
        auto field = llvm::find_if(
            payload->fields, [&](const QueuePayloadFieldPlan &candidate) {
              return candidate.name == name;
            });
        return field == payload->fields.end()
                   ? std::nullopt
                   : std::optional<uint64_t>(field->width);
      };
      if (fieldWidth(table.validField) != 1 ||
          fieldWidth(table.generationField) != table.generationBits ||
          fieldWidth(table.epochField) != table.epochBits ||
          !fieldWidth(table.payloadField) ||
          (table.attemptBits != 0 &&
           fieldWidth(table.attemptField) != table.attemptBits))
        return planError("versioned Table field layout is inconsistent");
    } else if (!table.recoveryDomain.empty() || !table.identity.empty() ||
               !table.checkpoint.empty() || !table.retainedResult.empty() ||
               table.generationBits != 0 || table.epochBits != 0 ||
               table.attemptBits != 0 || !table.validField.empty() ||
               !table.generationField.empty() || !table.epochField.empty() ||
               !table.attemptField.empty() || !table.payloadField.empty()) {
      return planError("plain Table carries partial versioned metadata");
    }
  }
  auto projectionSize = [](const auto &domain,
                           const TablePlan &table) -> std::optional<uint64_t> {
    if (!domain.hasDomainProjection)
      return table.entries;
    if (domain.domainAxes.size() != domain.domainShape.size() ||
        domain.domainShape.size() != domain.domainStrides.size())
      return std::nullopt;
    uint64_t entries = 1;
    uint64_t maximum = domain.domainOffset;
    llvm::DenseSet<uint64_t> axes;
    for (auto [axis, extent, stride] : llvm::zip_equal(
             domain.domainAxes, domain.domainShape, domain.domainStrides)) {
      const size_t rank = table.shape.empty() ? 1 : table.shape.size();
      if (axis >= rank)
        return std::nullopt;
      const uint64_t tableExtent =
          table.shape.empty() ? table.entries : table.shape[axis];
      if (!axes.insert(axis).second || extent != tableExtent || extent == 0 ||
          stride == 0 ||
          entries > std::numeric_limits<uint64_t>::max() / extent ||
          (extent - 1) >
              (std::numeric_limits<uint64_t>::max() - maximum) / stride)
        return std::nullopt;
      entries *= extent;
      maximum += (extent - 1) * stride;
    }
    if (maximum >= table.entries)
      return std::nullopt;
    return entries;
  };
  llvm::StringMap<const TableMatchPlan *> tableMatches;
  for (const TableMatchPlan &match : plan.tableMatches) {
    const TablePlan *table = tables.lookup(match.table);
    std::optional<uint64_t> domainEntries =
        table ? projectionSize(match, *table) : std::nullopt;
    if (match.name.empty() || !table || !domainEntries ||
        !isCandidateMaskType(match.resultType, *domainEntries) ||
        match.resultType.empty() || match.yield.empty() ||
        !match.domainBase.empty() ||
        !tableMatches.try_emplace(match.name, &match).second)
      return planError("table match metadata is incomplete or duplicated");
  }
  llvm::StringMap<const TableSelectionPlan *> tableSelections;
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    const TablePlan *table = tables.lookup(selection.table);
    const TableMatchPlan *match = tableMatches.lookup(selection.match);
    std::optional<uint64_t> domainEntries =
        table && match ? projectionSize(*match, *table) : std::nullopt;
    auto indexWidth = bitsWidth(selection.indexType);
    const unsigned expectedIndexWidth =
        table ? std::max<unsigned>(1, llvm::Log2_64_Ceil(table->entries)) : 0;
    if (selection.name.empty() || !table || !match || !indexWidth ||
        *indexWidth != expectedIndexWidth || match->table != selection.table ||
        selection.indexType.empty() ||
        (selection.policy != "first" && selection.policy != "min" &&
         selection.policy != "max" && selection.policy != "round_robin") ||
        !domainEntries || selection.count == 0 ||
        selection.count > *domainEntries ||
        ((selection.policy == "first" || selection.policy == "round_robin") &&
         (!selection.keyExpressions.empty() || !selection.keyYield.empty())) ||
        ((selection.policy == "min" || selection.policy == "max") &&
         (selection.keyYield.empty() ||
          (selection.keyOrdering != "signed" &&
           selection.keyOrdering != "unsigned"))) ||
        (selection.policy == "round_robin" &&
         (selection.stableId.empty() ||
          selection.initialCursor >= *domainEntries)) ||
        !tableSelections.try_emplace(selection.name, &selection).second)
      return planError("table selection metadata is incomplete, duplicated, or "
                       "inconsistent");
  }
  auto verifySharedExpression =
      [&](auto &&self, const QueueExpressionPlan &expression) -> llvm::Error {
    if (expression.kind == "table_match_ref") {
      const TableMatchPlan *match = tableMatches.lookup(expression.field);
      if (!match)
        return planError("table_match_ref references unknown match target");
      if (expression.table != match->table)
        return planError("table_match_ref Table provenance is inconsistent");
      if (expression.type != match->resultType)
        return planError("table_match_ref field type is inconsistent");
    } else if (expression.kind == "table_selection_index_ref" ||
               expression.kind == "table_selection_valid_ref") {
      const TableSelectionPlan *selection =
          tableSelections.lookup(expression.field);
      if (!selection)
        return planError(
            "table_selection_ref references unknown selection target");
      if (expression.table != selection->table)
        return planError(
            "table_selection_ref Table provenance is inconsistent");
      const llvm::StringRef expected =
          expression.kind == "table_selection_index_ref"
              ? llvm::StringRef(selection->indexType)
              : llvm::StringRef("i1");
      if (expression.type != expected)
        return planError("table_selection_ref field type is inconsistent");
      if (expression.selectionCount != selection->count ||
          expression.laneOrdinal >= selection->count)
        return planError("table_selection_ref lane metadata is inconsistent");
    }
    for (const QueueExpressionPlan &nested : expression.nestedExpressions)
      if (auto error = self(self, nested))
        return error;
    return llvm::Error::success();
  };
  auto verifySharedExpressions = [&](const auto &expressions) -> llvm::Error {
    for (const QueueExpressionPlan &expression : expressions)
      if (auto error =
              verifySharedExpression(verifySharedExpression, expression))
        return error;
    return llvm::Error::success();
  };
  for (const TableMatchPlan &match : plan.tableMatches)
    if (auto error = verifySharedExpressions(match.expressions))
      return error;
  for (const TableSelectionPlan &selection : plan.tableSelections)
    if (auto error = verifySharedExpressions(selection.keyExpressions))
      return error;
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    if (selection.count <= 1)
      continue;
    std::vector<unsigned> indexUses(selection.count, 0);
    std::vector<unsigned> validUses(selection.count, 0);
    size_t firingConsumers = 0;
    auto collectSelectionUses = [&](auto &&self, const auto &expressions,
                                    bool &used) -> llvm::Error {
      for (const QueueExpressionPlan &expression : expressions) {
        if ((expression.kind == "table_selection_index_ref" ||
             expression.kind == "table_selection_valid_ref") &&
            expression.field == selection.name) {
          used = true;
          if (expression.laneOrdinal >= selection.count)
            return planError(
                "multi-selection consumer lane ordinal is out of range");
          auto &uses = expression.kind == "table_selection_index_ref"
                           ? indexUses
                           : validUses;
          ++uses[expression.laneOrdinal];
        }
        if (auto error = self(self, expression.nestedExpressions, used))
          return error;
      }
      return llvm::Error::success();
    };
    for (const QueueBlockPlan &block : plan.blocks) {
      bool used = false;
      if (auto error = collectSelectionUses(collectSelectionUses,
                                            block.expressions, used))
        return error;
      if (!used)
        continue;
      if (block.kind != "firing")
        return planError(
            "multi-selection references must belong to one firing or one "
            "TableRead prefix group");
      ++firingConsumers;
    }
    const size_t readGroups =
        llvm::count_if(plan.blocks, [&](const QueueBlockPlan &block) {
          return block.kind == "table_read_group" &&
                 block.selection == selection.name;
        });
    if (firingConsumers + readGroups != 1)
      return planError(
          "multi-selection must have exactly one atomic prefix consumer");
    if (firingConsumers == 1)
      for (size_t lane = 0; lane < selection.count; ++lane)
        if (indexUses[lane] != 1 || validUses[lane] != 1)
          return planError(
              "multi-selection firing must consume every lane exactly once");
  }
  llvm::StringMap<unsigned> tableReaders;
  llvm::StringMap<llvm::StringSet<>> tableWriterFields;
  llvm::StringSet<> tableReplaceWriters;
  llvm::StringMap<unsigned> tableFirings;
  llvm::DenseSet<uint64_t> firingPriorities;
  auto verifyWriteFields = [&](llvm::StringRef tableName, llvm::StringRef mode,
                               const std::vector<std::string> &writeFields,
                               bool reserveOwnership = true,
                               bool requireDeclarationOrder = true) {
    const TablePlan *table = tables.lookup(tableName);
    if (!table || writeFields.empty())
      return false;
    llvm::StringSet<> allowed;
    llvm::StringMap<unsigned> ordinals;
    if (llvm::StringRef(table->entryType).starts_with("!ac.struct<")) {
      size_t marker = table->entryType.rfind('@');
      size_t end = table->entryType.rfind('>');
      if (marker == std::string::npos || end == std::string::npos ||
          marker >= end)
        return false;
      llvm::StringRef payloadName(table->entryType.data() + marker + 1,
                                  end - marker - 1);
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &item) {
            return item.name == payloadName;
          });
      if (payload == plan.payloads.end())
        return false;
      for (auto [ordinal, field] : llvm::enumerate(payload->fields)) {
        allowed.insert(field.name);
        ordinals[field.name] = ordinal;
      }
    } else {
      allowed.insert("$entry");
      ordinals["$entry"] = 0;
    }
    llvm::StringSet<> local;
    std::optional<unsigned> previousOrdinal;
    for (const std::string &field : writeFields) {
      if (field.empty() || !allowed.contains(field) ||
          !local.insert(field).second)
        return false;
      unsigned ordinal = ordinals.lookup(field);
      if (requireDeclarationOrder && previousOrdinal &&
          ordinal <= *previousOrdinal)
        return false;
      previousOrdinal = ordinal;
    }
    if (mode != "field" && mode != "replace")
      return false;
    if (mode == "replace")
      return writeFields.size() == allowed.size() &&
             (!reserveOwnership ||
              tableReplaceWriters.insert(tableName).second);
    if (reserveOwnership)
      for (const std::string &field : writeFields)
        if (!tableWriterFields[tableName].insert(field).second)
          return false;
    return true;
  };
  auto tableFieldCount = [&](const TablePlan &table) -> std::optional<size_t> {
    if (!llvm::StringRef(table.entryType).starts_with("!ac.struct<"))
      return size_t{1};
    size_t marker = table.entryType.rfind('@');
    size_t end = table.entryType.rfind('>');
    if (marker == std::string::npos || end == std::string::npos ||
        marker >= end)
      return std::nullopt;
    llvm::StringRef payloadName(table.entryType.data() + marker + 1,
                                end - marker - 1);
    auto payload =
        llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &item) {
          return item.name == payloadName;
        });
    if (payload == plan.payloads.end())
      return std::nullopt;
    return payload->fields.size();
  };
  for (const TableReadPlan &read : plan.tableReads) {
    if (!tables.contains(read.table) || read.name.empty() ||
        read.output.empty() || read.depth == 0 || read.latency == 0)
      return planError("table read endpoint metadata is incomplete");
    ++tableReaders[read.table];
  }
  for (const TableWritePlan &write : plan.tableWrites) {
    if (!tables.contains(write.table) || write.name.empty())
      return planError("table write endpoint metadata is incomplete");
    const bool arbitrated =
        llvm::any_of(plan.blocks, [&](const QueueBlockPlan &block) {
          return block.kind == "table_write" && block.name == write.name &&
                 block.table == write.table &&
                 !block.arbitrationMembership.empty();
        });
    if (!verifyWriteFields(write.table, write.mode, write.writeFields,
                           !arbitrated))
      return planError(
          "table write_fields are invalid or overlap another writer");
  }
  for (const TableMaskedWritePlan &write : plan.tableMaskedWrites) {
    if (!tables.contains(write.table) || write.name.empty())
      return planError("masked table write endpoint metadata is incomplete");
    const bool arbitrated =
        llvm::any_of(plan.blocks, [&](const QueueBlockPlan &block) {
          return block.kind == "table_masked_write" &&
                 block.name == write.name && block.table == write.table &&
                 !block.arbitrationMembership.empty();
        });
    if (write.mode != "field" ||
        !verifyWriteFields(write.table, write.mode, write.writeFields,
                           !arbitrated))
      return planError(
          "table write_fields are invalid or overlap another writer");
  }
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind != "firing")
      continue;
    if (!firingPriorities.insert(block.priority).second)
      return planError("firing arbitration priorities must be unique");
    if (block.stateWrites.empty() && block.slotReleases.empty() &&
        block.outputs.empty())
      return planError("outputless firing must update state");
    if (block.guard.empty() || block.yields.size() != block.outputs.size() ||
        block.depths.size() != block.outputs.size() ||
        block.latencies.size() != block.outputs.size() ||
        (block.inputs.empty() && block.outputs.empty() &&
         block.slotReleases.empty()))
      return planError(
          "table firing metadata is incomplete or conflicting for '" +
          block.name + "' (writes=" + std::to_string(block.stateWrites.size()) +
          ", reservations=" + std::to_string(block.stateReservations.size()) +
          ", inputs=" + std::to_string(block.inputs.size()) +
          ", outputs=" + std::to_string(block.outputs.size()) +
          ", yields=" + std::to_string(block.yields.size()) + ")");
    const bool hasPresence =
        !block.outputPresence.empty() ||
        llvm::any_of(block.stateWrites,
                     [](const auto &write) { return !write.present.empty(); });
    if (hasPresence) {
      if (block.outputPresence.size() != block.outputs.size() ||
          llvm::any_of(block.stateWrites,
                       [](const auto &write) { return write.present.empty(); }))
        return planError("state firing SSA presence metadata is incomplete");
      llvm::SmallVector<uint8_t, 4> seen(block.outputs.size(), 0);
      std::optional<uint64_t> previousOrdinal;
      for (const OutputPresencePlan &output : block.outputPresence) {
        if (output.ordinal >= block.outputs.size() || seen[output.ordinal])
          return planError(
              "state firing output presence ordinals must cover each output "
              "exactly once");
        if (previousOrdinal && output.ordinal <= *previousOrdinal)
          return planError(
              "state firing output presence ordinals must be sorted");
        seen[output.ordinal] = true;
        previousOrdinal = output.ordinal;
        if (output.value != block.yields[output.ordinal] ||
            output.present.empty())
          return planError("state firing output presence is not canonical");
      }
      if (llvm::is_contained(seen, uint8_t{0}))
        return planError(
            "state firing output presence ordinals must cover each output "
            "exactly once");
    }
    for (const StateWritePlan &write : block.stateWrites) {
      auto table = tables.find(write.table);
      if (table == tables.end())
        return planError("state write references unknown Table");
      const bool versioned = table->getValue()->versioned;
      if (write.versionedAction.empty()) {
        if (versioned)
          return planError(
              "versioned Table write lacks transaction qualification");
        if (!write.refGeneration.empty() || !write.refEpoch.empty() ||
            !write.refAttempt.empty() || !write.staleObligationId.empty())
          return planError("plain Table write carries versioned metadata");
        continue;
      }
      if (!versioned ||
          (write.versionedAction != "allocate" &&
           write.versionedAction != "qualified_update" &&
           write.versionedAction != "invalidate" &&
           write.versionedAction != "retain" &&
           write.versionedAction != "consume") ||
          write.refGeneration.empty() || write.refEpoch.empty() ||
          write.staleObligationId.empty() ||
          ((table->getValue()->attemptBits == 0) !=
           write.refAttempt.empty()))
        return planError("versioned state write metadata is malformed");
    }
    llvm::StringSet<> ownerWrites;
    for (const StateWritePlan &write : block.stateWrites) {
      if (!tables.contains(write.table) || write.index.empty() ||
          write.value.empty() ||
          llvm::any_of(plan.tableWrites,
                       [&](const TableWritePlan &endpoint) {
                         return endpoint.table == write.table;
                       }) ||
          llvm::any_of(plan.tableMaskedWrites,
                       [&](const TableMaskedWritePlan &endpoint) {
                         return endpoint.table == write.table;
                       }) ||
          !verifyWriteFields(write.table, write.mode, write.fields, false))
        return planError("state firing write metadata is invalid");
      if (ownerWrites.insert(write.table).second)
        ++tableFirings[write.table];
    }
    llvm::StringSet<> reservationOwners;
    for (const StateReservationPlan &reservation : block.stateReservations)
      if (reservationOwners.insert(reservation.table).second)
        ++tableReaders[reservation.table];
  }
  for (const auto &entry : tables)
    if (tableReaders[entry.getKey()] +
            (tableWriterFields.contains(entry.getKey()) ? 1U : 0U) +
            (tableReplaceWriters.contains(entry.getKey()) ? 1U : 0U) +
            tableFirings[entry.getKey()] ==
        0)
      return planError("table '" + entry.getKey() + "' has no endpoints");
  llvm::StringSet<> slotNames;
  for (const SlotPlan &slot : plan.slots)
    if (slot.name.empty() || !slotNames.insert(slot.name).second ||
        slot.payloadType.empty() || slot.input.empty() || slot.scope.empty() ||
        slot.stableId.empty() || slot.ownerPath.empty())
      return planError("slot metadata is incomplete or duplicated");
  llvm::StringMap<unsigned> producers;
  llvm::StringMap<unsigned> consumers;
  llvm::StringMap<unsigned> indegree;
  llvm::StringMap<std::vector<std::string>> successors;
  llvm::StringMap<std::string> queueTypes;
  llvm::StringMap<std::pair<uint64_t, uint64_t>> queueLanesAndRates;
  for (const QueueInterfacePlan &input : plan.interfaceInputs) {
    if (input.name.empty() || input.payloadType.empty() || input.lanes == 0 ||
        input.rate == 0 || input.rate > input.lanes ||
        !queueNames.insert(input.name).second)
      return planError("module input identities must be typed and unique");
    indegree[input.name] = 0;
    queueTypes[input.name] = input.payloadType;
    queueLanesAndRates[input.name] = {input.lanes, input.rate};
    producers[input.name] = 1;
  }
  for (const QueuePlan &queue : plan.queues) {
    const bool interfaceInput = queueTypes.contains(queue.name);
    if (queue.name.empty() ||
        (!queueNames.insert(queue.name).second && !interfaceInput))
      return planError("Queue logical identities must be non-empty and unique");
    if (interfaceInput &&
        (queueTypes.lookup(queue.name) != queue.payloadType ||
         queueLanesAndRates.lookup(queue.name) !=
             std::make_pair(queue.lanes, queue.rate)))
      return planError("module input Queue storage disagrees with its interface");
    if (queue.payloadType.empty() || queue.depth == 0 || queue.latency == 0 ||
        queue.rate == 0 || queue.rate > queue.depth || queue.lanes == 0 ||
        (!queue.laneOrdinals.empty() &&
         (queue.rate > queue.lanes ||
          queue.laneOrdinals.size() != queue.lanes ||
          llvm::any_of(llvm::enumerate(queue.laneOrdinals), [](auto item) {
            return item.value() != item.index();
          }))))
      return planError(
          "Queue plan requires typed positive depth/latency and rate <= depth");
    indegree[queue.name] = 0;
    queueTypes[queue.name] = queue.payloadType;
    queueLanesAndRates[queue.name] = {queue.lanes, queue.rate};
  }
  for (const QueueInterfacePlan &output : plan.interfaceOutputs) {
    if (output.name.empty() || output.payloadType.empty() ||
        output.lanes == 0 || output.rate == 0 || output.rate > output.lanes ||
        !queueNames.contains(output.name) ||
        queueTypes.lookup(output.name) != output.payloadType ||
        queueLanesAndRates.lookup(output.name) !=
            std::pair<uint64_t, uint64_t>{output.lanes, output.rate})
      return planError(
          "module output must reference an exact typed local Queue");
    ++consumers[output.name];
  }

  auto containsDeclaredRange = [&](auto &&self, llvm::StringRef type,
                                   llvm::StringSet<> &active) -> bool {
    if (rangeBounds(type))
      return true;
    if (!active.insert(type).second)
      return false;
    bool found = false;
    if (std::optional<llvm::StringRef> name = payloadTypeName(type)) {
      auto payload =
          llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
            return candidate.name == *name;
          });
      if (payload != plan.payloads.end())
        found = llvm::any_of(payload->fields, [&](const auto &field) {
          return self(self, field.type, active);
        });
    } else {
      auto aggregate = llvm::find_if(plan.aggregates,
                                     [&](const QueueAggregatePlan &candidate) {
                                       return candidate.type == type;
                                     });
      if (aggregate != plan.aggregates.end())
        found = llvm::any_of(aggregate->elements, [&](llvm::StringRef element) {
          return self(self, element, active);
        });
    }
    active.erase(type);
    return found;
  };

  llvm::StringSet<> instanceNames;
  for (const QueueModuleInstancePlan &instance : plan.moduleInstances) {
    const QueueGraphPlan *target = findFamilyCaseBody(
        plan, instance.definition, instance.staticArguments);
    if (instance.name.empty() || !instanceNames.insert(instance.name).second ||
        instance.definition.empty() || !target ||
        target->definition != instance.definition || instance.scope.empty() ||
        instance.inputs.size() != target->interfaceInputs.size() ||
        instance.outputs.size() != target->interfaceOutputs.size())
      return planError(
          "module instance metadata is incomplete or inconsistent");
    for (auto [input, interface] :
         llvm::zip_equal(instance.inputs, target->interfaceInputs)) {
      if (!queueNames.contains(input) ||
          queueTypes.lookup(input) != interface.payloadType ||
          queueLanesAndRates.lookup(input) !=
              std::pair<uint64_t, uint64_t>{interface.lanes, interface.rate})
        return planError("module instance input Queue type is inconsistent");
      ++consumers[input];
    }
    for (auto [output, interface] :
         llvm::zip_equal(instance.outputs, target->interfaceOutputs)) {
      if (!queueNames.contains(output) ||
          queueTypes.lookup(output) != interface.payloadType ||
          queueLanesAndRates.lookup(output) !=
              std::pair<uint64_t, uint64_t>{interface.lanes, interface.rate})
        return planError("module instance output Queue type is inconsistent");
      ++producers[output];
    }
    for (const std::string &input : instance.inputs)
      for (const std::string &output : instance.outputs) {
        successors[input].push_back(output);
        ++indegree[output];
      }
  }

  llvm::StringMap<const QueueHelperPlan *> helpers;
  if (plan.helpers.size() > acir::kMaxPureCallFunctions)
    return planError("Queue helper count exceeds the ACIR capability limit");
  for (const QueueHelperPlan &helper : plan.helpers)
    if (helper.name.empty() ||
        helper.inputNames.size() != helper.inputTypes.size() ||
        helper.resultTypes.size() != helper.yields.size() ||
        !helpers.try_emplace(helper.name, &helper).second)
      return planError("Queue helper metadata is incomplete or duplicated");
  llvm::StringMap<std::vector<std::string>> helperEdges;
  for (const QueueHelperPlan &helper : plan.helpers) {
    llvm::StringSet<> dependencies;
    collectHelperCalls(helper.expressions, dependencies);
    for (const auto &dependency : dependencies) {
      if (!helpers.contains(dependency.getKey()))
        return planError("Queue helper references an unknown helper");
      helperEdges[helper.name].push_back(dependency.getKey().str());
    }
    llvm::sort(helperEdges[helper.name]);
  }
  enum class HelperState : uint8_t { Unvisited, Active, Complete };
  llvm::StringMap<HelperState> helperStates;
  struct HelperFrame {
    std::string name;
    size_t next = 0;
  };
  uint64_t helperEdgeCount = 0;
  for (const QueueHelperPlan &helper : plan.helpers) {
    if (helperStates.lookup(helper.name) != HelperState::Unvisited)
      continue;
    std::vector<HelperFrame> stack{{helper.name, 0}};
    helperStates[helper.name] = HelperState::Active;
    while (!stack.empty()) {
      HelperFrame &frame = stack.back();
      auto &edges = helperEdges[frame.name];
      if (frame.next == edges.size()) {
        helperStates[frame.name] = HelperState::Complete;
        stack.pop_back();
        continue;
      }
      if (++helperEdgeCount > acir::kMaxPureCallEdges)
        return planError("Queue helper graph exceeds the ACIR edge limit");
      const std::string &target = edges[frame.next++];
      if (helperStates.lookup(target) == HelperState::Active)
        return planError("Queue helper graph contains a recursive cycle");
      if (helperStates.lookup(target) == HelperState::Unvisited) {
        if (stack.size() >= acir::kMaxPureCallDepth)
          return planError("Queue helper graph exceeds the ACIR depth limit");
        helperStates[target] = HelperState::Active;
        stack.push_back({target, 0});
      }
    }
  }

  auto verifyExpressionList =
      [&](auto &&self, const auto &expressions,
          llvm::ArrayRef<std::string> rootTypes, llvm::StringRef rootPrefix,
          const llvm::StringMap<std::string> &inheritedTypes) -> llvm::Error {
    llvm::StringMap<std::string> valueTypes;
    llvm::StringMap<const QueueExpressionPlan *> valueDefinitions;
    llvm::StringMap<std::pair<unsigned, unsigned>> inlineChoiceKinds;
    struct RangeCheckedPair {
      const QueueExpressionPlan *value = nullptr;
      const QueueExpressionPlan *valid = nullptr;
      unsigned valueCount = 0;
      unsigned validCount = 0;
    };
    llvm::StringMap<RangeCheckedPair> rangeCheckedPairs;
    for (const auto &entry : inheritedTypes)
      valueTypes[entry.getKey()] = entry.getValue();
    for (auto [index, type] : llvm::enumerate(rootTypes))
      valueTypes[index == 0 ? rootPrefix.str()
                            : rootPrefix.str() + std::to_string(index)] = type;
    for (const QueueExpressionPlan &expression : expressions) {
      if (expression.result.empty() || expression.type.empty() ||
          valueTypes.contains(expression.result))
        return planError(
            "expression identities and result types must be closed");
      if (expression.kind == "invariant")
        return planError(
            "residual ac.var.invariant must be lowered before QueueGraph "
            "planning");
      if (expression.kind == "constant") {
        if (!expression.operands.empty() || expression.literal.empty() ||
            !integerWidth(expression.type))
          return planError("constant expression contract is malformed");
        auto value = planConstantValue(expression.literal, expression.type);
        if (!value)
          return planError("constant expression value is malformed");
        if (auto bounds = rangeBounds(expression.type);
            bounds && (*value < bounds->first || *value > bounds->second))
          return planError("range constant is outside declared bounds");
      } else if (expression.kind == "helper_call") {
        const QueueHelperPlan *helper = helpers.lookup(expression.field);
        if (!helper || expression.literal.empty() ||
            expression.selectionCount != helper->resultTypes.size() ||
            expression.laneOrdinal >= helper->resultTypes.size() ||
            expression.operands.size() != helper->inputTypes.size() ||
            expression.type != helper->resultTypes[expression.laneOrdinal])
          return planError("helper_call expression contract is malformed");
        for (auto [operandName, expected] :
             llvm::zip_equal(expression.operands, helper->inputTypes)) {
          auto operand = valueTypes.find(operandName);
          if (operand == valueTypes.end() || operand->getValue() != expected)
            return planError("helper_call operand type is inconsistent");
        }
      } else if (expression.kind == "table_index") {
        const TablePlan *table = tables.lookup(expression.table);
        const size_t rank = table && table->shape.empty()
                                ? 1
                                : (table ? table->shape.size() : 0);
        if (!table || expression.operands.size() != rank)
          return planError("Table index expression contract is malformed");
        for (auto [axis, operandName] : llvm::enumerate(expression.operands)) {
          auto operand = valueTypes.find(operandName);
          const uint64_t extent =
              table->shape.empty() ? table->entries : table->shape[axis];
          const uint64_t expectedWidth =
              table->axisWidths.empty()
                  ? std::max<uint64_t>(1, llvm::Log2_64_Ceil(extent))
                  : table->axisWidths[axis];
          const bool bounded =
              operand != valueTypes.end() &&
              rangeBounds(operand->getValue()).has_value() &&
              rangeBounds(operand->getValue())->second < extent;
          if (operand == valueTypes.end() ||
              (!bounded &&
               operand->getValue() != "i" + std::to_string(expectedWidth)))
            return planError("Table coordinate type is inconsistent");
        }
        const std::string expectedType =
            "i" + std::to_string(std::max<uint64_t>(
                      1, llvm::Log2_64_Ceil(table->entries)));
        if (expression.type != expectedType)
          return planError("flattened Table index type is inconsistent");
      } else if (expression.kind == "table_match") {
        const TablePlan *table = tables.lookup(expression.table);
        std::optional<uint64_t> domainEntries =
            table ? projectionSize(expression, *table) : std::nullopt;
        if (!table || !domainEntries ||
            !isCandidateMaskType(expression.type, *domainEntries) ||
            expression.nestedYields.size() != 1)
          return planError(
              "inline table match mask width or metadata is inconsistent");
        if (!expression.domainBase.empty()) {
          if (expression.domainOffset != 0 || table->shape.size() != 2 ||
              expression.domainAxes != std::vector<uint64_t>{1} ||
              expression.domainShape !=
                  std::vector<uint64_t>{table->shape.back()} ||
              expression.domainStrides != std::vector<uint64_t>{1} ||
              !llvm::is_contained(expression.operands, expression.domainBase))
            return planError(
                "dynamic inline table match projection is not canonical");
          auto base = valueDefinitions.find(expression.domainBase);
          const std::string expectedType =
              "i" + std::to_string(std::max<uint64_t>(
                        1, llvm::Log2_64_Ceil(table->entries)));
          if (base == valueDefinitions.end() ||
              base->getValue()->kind != "table_index" ||
              base->getValue()->table != expression.table ||
              base->getValue()->type != expectedType ||
              base->getValue()->operands.size() != 2)
            return planError(
                "dynamic inline table match base must be a same-Table index");
          auto suffix =
              valueDefinitions.find(base->getValue()->operands.back());
          llvm::StringRef suffixLiteral =
              suffix == valueDefinitions.end()
                  ? llvm::StringRef()
                  : llvm::StringRef(suffix->getValue()->literal)
                        .split(':')
                        .first.trim();
          if (suffix == valueDefinitions.end() ||
              suffix->getValue()->kind != "constant" ||
              (suffixLiteral != "0" && suffixLiteral != "false"))
            return planError(
                "dynamic inline table match base suffix must be constant zero");
        }
      } else if (expression.kind == "enum_constant") {
        if (!expression.operands.empty() || expression.field.empty() ||
            expression.literal.empty())
          return planError("enum constant expression contract is malformed");
        std::optional<llvm::StringRef> name = enumTypeName(expression.type);
        auto enumeration = name ? enums.find(*name) : enums.end();
        uint64_t encoded = 0;
        if (!name || enumeration == enums.end() ||
            llvm::StringRef(expression.literal).getAsInteger(10, encoded))
          return planError("enum constant expression is inconsistent");
        const QueueEnumPlan &planEnum = *enumeration->getValue();
        auto member = llvm::find(planEnum.enumerants, expression.field);
        if (member == planEnum.enumerants.end())
          return planError("enum constant expression is inconsistent");
        const size_t ordinal =
            std::distance(planEnum.enumerants.begin(), member);
        const uint64_t expected =
            planEnum.values.empty() ? ordinal : planEnum.values[ordinal];
        if (encoded != expected)
          return planError("enum constant expression is inconsistent");
      } else if (expression.kind == "tuple_create" ||
                 expression.kind == "array_create") {
        if (expression.operands.empty() || expression.width == 0)
          return planError("aggregate create expression is malformed");
        auto aggregate = llvm::find_if(
            plan.aggregates, [&](const QueueAggregatePlan &candidate) {
              return candidate.type == expression.type;
            });
        const llvm::StringRef expectedKind =
            expression.kind == "tuple_create" ? "tuple" : "array";
        if (aggregate == plan.aggregates.end() ||
            aggregate->kind != expectedKind)
          return planError("aggregate create result metadata is inconsistent");
        const size_t expectedArity = expression.kind == "tuple_create"
                                         ? aggregate->elements.size()
                                         : aggregate->length;
        if (expression.operands.size() != expectedArity)
          return planError("aggregate create operand arity is inconsistent");
        uint64_t total = 0;
        for (auto [index, operandName] : llvm::enumerate(expression.operands)) {
          auto operand = valueTypes.find(operandName);
          const llvm::StringRef expectedType =
              expression.kind == "tuple_create"
                  ? llvm::StringRef(aggregate->elements[index])
                  : llvm::StringRef(aggregate->elements.front());
          if (operand == valueTypes.end() ||
              operand->getValue() != expectedType)
            return planError("aggregate create operand type is inconsistent");
          auto width = operand == valueTypes.end()
                           ? std::optional<uint64_t>()
                           : valueWidth(operand->getValue());
          if (!width)
            return planError("aggregate operand type has no width");
          total += *width;
        }
        auto resultWidth = valueWidth(expression.type);
        if (!resultWidth || total != expression.width ||
            aggregate->width != expression.width ||
            *resultWidth != expression.width)
          return planError(
              "aggregate create expression widths are inconsistent");
      } else if (expression.kind == "record_create") {
        std::optional<llvm::StringRef> name = payloadTypeName(expression.type);
        auto payload =
            name ? llvm::find_if(plan.payloads,
                                 [&](const QueuePayloadPlan &candidate) {
                                   return candidate.name == *name;
                                 })
                 : plan.payloads.end();
        if (!name || payload == plan.payloads.end() ||
            expression.operands.empty() ||
            payload->fields.size() != expression.operands.size())
          return planError("record create expression is malformed");
        uint64_t total = 0;
        for (auto [operandName, field] :
             llvm::zip_equal(expression.operands, payload->fields)) {
          auto operand = valueTypes.find(operandName);
          if (operand == valueTypes.end() || operand->getValue() != field.type)
            return planError("record create operand type is inconsistent");
          auto width = valueWidth(field.type);
          if (!width)
            return planError("record field type has no width");
          total += *width;
        }
        auto resultWidth = valueWidth(expression.type);
        if (!resultWidth || total != expression.width ||
            *resultWidth != expression.width)
          return planError("record create expression widths are inconsistent");
      } else if (expression.kind == "get") {
        auto source = expression.operands.size() == 1
                          ? valueTypes.find(expression.operands.front())
                          : valueTypes.end();
        auto name = source == valueTypes.end()
                        ? std::optional<llvm::StringRef>()
                        : payloadTypeName(source->getValue());
        auto payload =
            name ? llvm::find_if(plan.payloads,
                                 [&](const QueuePayloadPlan &candidate) {
                                   return candidate.name == *name;
                                 })
                 : plan.payloads.end();
        const QueuePayloadFieldPlan *field = nullptr;
        if (payload != plan.payloads.end()) {
          auto found = llvm::find_if(payload->fields, [&](const auto &item) {
            return item.name == expression.field;
          });
          if (found != payload->fields.end())
            field = &*found;
        }
        if (!field || expression.type != field->type)
          return planError("record get expression type is inconsistent");
      } else if (expression.kind == "with") {
        auto base = expression.operands.size() == 2
                        ? valueTypes.find(expression.operands[0])
                        : valueTypes.end();
        auto replacement = expression.operands.size() == 2
                               ? valueTypes.find(expression.operands[1])
                               : valueTypes.end();
        auto name = base == valueTypes.end()
                        ? std::optional<llvm::StringRef>()
                        : payloadTypeName(base->getValue());
        auto payload =
            name ? llvm::find_if(plan.payloads,
                                 [&](const QueuePayloadPlan &candidate) {
                                   return candidate.name == *name;
                                 })
                 : plan.payloads.end();
        const QueuePayloadFieldPlan *field = nullptr;
        if (payload != plan.payloads.end()) {
          auto found = llvm::find_if(payload->fields, [&](const auto &item) {
            return item.name == expression.field;
          });
          if (found != payload->fields.end())
            field = &*found;
        }
        if (!field || replacement == valueTypes.end() ||
            replacement->getValue() != field->type ||
            expression.type != base->getValue())
          return planError("record with expression type is inconsistent");
      } else if (expression.kind == "table_get") {
        const TablePlan *table = tables.lookup(expression.table);
        if (!table || expression.operands.size() != 1 ||
            expression.type != table->entryType)
          return planError("table get expression type is inconsistent");
      } else if (expression.kind == "slot_get_valid" ||
                 expression.kind == "slot_get_value") {
        auto slot = llvm::find_if(plan.slots, [&](const SlotPlan &candidate) {
          return candidate.name == expression.slot;
        });
        const llvm::StringRef expected =
            expression.kind == "slot_get_valid" ? llvm::StringRef("i1")
            : slot == plan.slots.end()          ? llvm::StringRef()
                                       : llvm::StringRef(slot->payloadType);
        if (!expression.operands.empty() || slot == plan.slots.end() ||
            expression.type != expected)
          return planError("slot get expression type is inconsistent");
      } else if (expression.kind == "array_get_dynamic") {
        if (expression.operands.size() != 2 || expression.selectionCount == 0 ||
            expression.width == 0)
          return planError("dynamic value_array access is malformed");
        auto source = valueTypes.find(expression.operands[0]);
        auto index = valueTypes.find(expression.operands[1]);
        auto aggregate =
            source == valueTypes.end()
                ? plan.aggregates.end()
                : llvm::find_if(plan.aggregates,
                                [&](const QueueAggregatePlan &candidate) {
                                  return candidate.type == source->getValue();
                                });
        auto elementWidth = valueWidth(expression.type);
        auto indexWidth = index == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(index->getValue());
        if (aggregate == plan.aggregates.end() || aggregate->kind != "array" ||
            aggregate->length != expression.selectionCount ||
            aggregate->elements.size() != 1 ||
            aggregate->elements.front() != expression.type || !elementWidth ||
            expression.width != *elementWidth || !indexWidth ||
            *indexWidth > 64 || expression.indexWidth != *indexWidth)
          return planError("dynamic value_array access types are inconsistent");
      } else if (expression.kind == "array_update_dynamic") {
        if (expression.operands.size() != 3 || expression.selectionCount == 0 ||
            expression.width == 0)
          return planError("dynamic value_array update is malformed");
        auto source = valueTypes.find(expression.operands[0]);
        auto index = valueTypes.find(expression.operands[1]);
        auto replacement = valueTypes.find(expression.operands[2]);
        auto aggregate =
            source == valueTypes.end()
                ? plan.aggregates.end()
                : llvm::find_if(plan.aggregates,
                                [&](const QueueAggregatePlan &candidate) {
                                  return candidate.type == source->getValue();
                                });
        auto elementWidth =
            aggregate == plan.aggregates.end() || aggregate->elements.empty()
                ? std::optional<uint64_t>()
                : valueWidth(aggregate->elements.front());
        auto indexWidth = index == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(index->getValue());
        if (aggregate == plan.aggregates.end() || aggregate->kind != "array" ||
            aggregate->length != expression.selectionCount ||
            aggregate->elements.size() != 1 || source == valueTypes.end() ||
            expression.type != source->getValue() ||
            replacement == valueTypes.end() ||
            replacement->getValue() != aggregate->elements.front() ||
            !elementWidth || expression.width != *elementWidth || !indexWidth ||
            *indexWidth > 64 || expression.indexWidth != *indexWidth)
          return planError("dynamic value_array update types are inconsistent");
      } else if (expression.kind == "aggregate_get") {
        if (expression.operands.size() != 1 || expression.width == 0)
          return planError("aggregate get expression is malformed");
        auto source = valueTypes.find(expression.operands.front());
        auto aggregate =
            source == valueTypes.end()
                ? plan.aggregates.end()
                : llvm::find_if(plan.aggregates,
                                [&](const QueueAggregatePlan &candidate) {
                                  return candidate.type == source->getValue();
                                });
        auto resultWidth = valueWidth(expression.type);
        if (aggregate == plan.aggregates.end() || !resultWidth ||
            expression.width != *resultWidth)
          return planError("aggregate get expression widths are inconsistent");
        bool exactElement = false;
        if (aggregate->kind == "tuple") {
          uint64_t offset = aggregate->width;
          for (const std::string &element : aggregate->elements) {
            auto width = valueWidth(element);
            if (!width)
              return planError("aggregate element type has no width");
            offset -= *width;
            exactElement |= expression.lsb == offset &&
                            expression.width == *width &&
                            expression.type == element;
          }
        } else if (aggregate->kind == "array") {
          auto width = valueWidth(aggregate->elements.front());
          if (!width)
            return planError("aggregate element type has no width");
          for (uint64_t index = 0; index < aggregate->length; ++index) {
            const uint64_t offset = (aggregate->length - index - 1) * *width;
            exactElement |= expression.lsb == offset &&
                            expression.width == *width &&
                            expression.type == aggregate->elements.front();
          }
        }
        if (!exactElement)
          return planError(
              "aggregate get must select one exact declared element");
      } else if (expression.kind == "add" || expression.kind == "sub" ||
                 expression.kind == "mul" || expression.kind == "and" ||
                 expression.kind == "or" || expression.kind == "xor" ||
                 expression.kind == "shl" || expression.kind == "shr") {
        auto left = expression.operands.size() == 2
                        ? valueTypes.find(expression.operands[0])
                        : valueTypes.end();
        auto right = expression.operands.size() == 2
                         ? valueTypes.find(expression.operands[1])
                         : valueTypes.end();
        auto width = bitsWidth(expression.type);
        if (!width || *width > 64 || left == valueTypes.end() ||
            right == valueTypes.end() || left->getValue() != expression.type ||
            right->getValue() != expression.type)
          return planError("bits arithmetic operands and result must share one "
                           "i1..i64 type");
      } else if (expression.kind == "not") {
        auto operand = expression.operands.size() == 1
                           ? valueTypes.find(expression.operands.front())
                           : valueTypes.end();
        auto width = bitsWidth(expression.type);
        if (!width || *width > 64 || operand == valueTypes.end() ||
            operand->getValue() != expression.type)
          return planError(
              "bits not operand and result must share one i1..i64 type");
      } else if (expression.kind == "cmp") {
        if (expression.operands.size() != 2 || expression.type != "i1")
          return planError("comparison expression contract is malformed");
        auto left = valueTypes.find(expression.operands[0]);
        auto right = valueTypes.find(expression.operands[1]);
        if (left == valueTypes.end())
          return planError(llvm::Twine("comparison '") + expression.result +
                           "' left operand '" + expression.operands[0] +
                           "' must reference a typed value");
        if (right == valueTypes.end())
          return planError(llvm::Twine("comparison '") + expression.result +
                           "' right operand '" + expression.operands[1] +
                           "' must reference a typed value");
        if (left->getValue() != right->getValue())
          return planError(
              llvm::Twine("comparison operand types must match for '") +
              expression.result + "': " + expression.operands[0] + " is " +
              left->getValue() + ", " + expression.operands[1] + " is " +
              right->getValue());
        const bool integer = bitsWidth(left->getValue()).has_value();
        std::optional<llvm::StringRef> enumName =
            enumTypeName(left->getValue());
        const bool enumeration = enumName && enums.contains(*enumName);
        const bool equality =
            expression.predicate == "eq" || expression.predicate == "ne";
        const bool ordered =
            llvm::StringSwitch<bool>(expression.predicate)
                .Cases({"slt", "sle", "sgt", "sge", "ult", "ule", "ugt", "uge"},
                       true)
                .Default(false);
        if ((!integer && !enumeration) || (!equality && !integer) ||
            (!equality && !ordered))
          return planError(
              "residual aggregate comparison must be lowered to scalar leaf "
              "comparisons before QueueGraph planning");
      } else if (expression.kind == "udiv" || expression.kind == "urem") {
        if (expression.operands.size() != 2)
          return planError("unsigned div/rem expression contract is malformed");
        auto left = valueTypes.find(expression.operands[0]);
        auto right = valueTypes.find(expression.operands[1]);
        auto resultWidth = bitsWidth(expression.type);
        if (left == valueTypes.end() || right == valueTypes.end() ||
            left->getValue() != expression.type ||
            right->getValue() != expression.type || !resultWidth ||
            !acir::isPrimitiveInputWidth(*resultWidth))
          return planError("unsigned div/rem operands and result must share "
                           "one i1..i64 type");
      } else if (expression.kind == "masked_match") {
        if (expression.operands.size() != 1 || expression.type != "i1")
          return planError("masked_match expression contract is malformed");
        auto operand = valueTypes.find(expression.operands.front());
        auto inputWidth = operand == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(operand->getValue());
        if (!inputWidth || *inputWidth > 64)
          return planError("masked_match input must be an i1..i64 value");
        auto mask = parseExactWidthHex(expression.mask, *inputWidth);
        auto value = parseExactWidthHex(expression.value, *inputWidth);
        if (!mask || !value || (*value & ~*mask) != 0)
          return planError("masked_match mask/value metadata is inconsistent");
      } else if (expression.kind == "priority_index" ||
                 expression.kind == "priority_valid") {
        if (expression.operands.size() != 1 ||
            (expression.predicate != "low" && expression.predicate != "high"))
          return planError("priority expression contract is malformed");
        auto operand = valueTypes.find(expression.operands.front());
        auto inputWidth = operand == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(operand->getValue());
        if (!inputWidth || !acir::isPrimitiveInputWidth(*inputWidth))
          return planError(
              "priority expression input must be an i1..i64 value");
        const std::string expected =
            expression.kind == "priority_valid"
                ? "i1"
                : "i" + std::to_string(
                            acir::primitivePriorityIndexWidth(*inputWidth));
        if (expression.type != expected)
          return planError("priority expression result type is inconsistent");
      } else if (expression.kind == "table_choose_index" ||
                 expression.kind == "table_choose_valid") {
        if (expression.operands.size() != 1)
          return planError("inline table choose requires one candidate mask");
        if (!expression.literal.empty() || !expression.slot.empty() ||
            !expression.mask.empty() || !expression.value.empty() ||
            expression.lsb != 0 || expression.width != 0)
          return planError("inline table choose metadata is not canonical");
        const TablePlan *table = tables.lookup(expression.table);
        auto producer = valueDefinitions.find(expression.operands.front());
        if (!table || producer == valueDefinitions.end() ||
            (producer->getValue()->kind != "table_match" &&
             producer->getValue()->kind != "table_match_ref") ||
            producer->getValue()->table != expression.table)
          return planError(
              "inline table choose mask must come from the same Table match");
        auto maskEntries = projectionSize(*producer->getValue(), *table);
        if (!maskEntries ||
            !isCandidateMaskType(producer->getValue()->type, *maskEntries))
          return planError(
              "inline table choose mask width must equal Table entries");
        if (!producer->getValue()->domainBase.empty() &&
            (expression.predicate != "first" || expression.selectionCount != 1))
          return planError(
              "dynamic row projection currently requires first/count=1");
        const unsigned expectedIndexWidth =
            std::max<unsigned>(1, llvm::Log2_64_Ceil(table->entries));
        const std::string expectedType =
            expression.kind == "table_choose_index"
                ? "i" + std::to_string(expectedIndexWidth)
                : "i1";
        if (expression.type != expectedType)
          return planError("inline table choose result type is inconsistent");
        if (expression.selectionCount == 0 ||
            expression.selectionCount > *maskEntries ||
            expression.laneOrdinal >= expression.selectionCount ||
            ((expression.selectionCount > 1 ||
              expression.predicate == "round_robin") &&
             expression.field.empty()))
          return planError("inline table choose lane metadata is malformed");
        if (expression.predicate != "first" && expression.predicate != "min" &&
            expression.predicate != "max" &&
            expression.predicate != "round_robin")
          return planError("inline table choose policy is unsupported");
        if (((expression.predicate == "first" ||
              expression.predicate == "round_robin") &&
             (!expression.nestedExpressions.empty() ||
              !expression.nestedYields.empty())) ||
            ((expression.predicate == "min" || expression.predicate == "max") &&
             (expression.nestedYields.size() != 1 ||
              (!expression.field.empty() &&
               expression.keyOrdering != "signed" &&
               expression.keyOrdering != "unsigned"))))
          return planError("inline table choose key shape is inconsistent");

        auto &kinds =
            inlineChoiceKinds[inlineTableChoiceContractKey(expression)];
        if (expression.field.empty()) {
          if (expression.kind == "table_choose_index") {
            if (kinds.first != kinds.second)
              return planError(
                  "inline table choose requires index before valid");
            ++kinds.first;
          } else {
            if (kinds.first != kinds.second + 1)
              return planError(
                  "inline table choose requires index before valid");
            ++kinds.second;
          }
        } else if (expression.kind == "table_choose_index") {
          if (kinds.second != 0 || expression.laneOrdinal != kinds.first)
            return planError(
                "inline table choose indices must form the first segment");
          ++kinds.first;
        } else {
          if (kinds.first != expression.selectionCount ||
              expression.laneOrdinal != kinds.second)
            return planError(
                "inline table choose valids must form the second segment");
          ++kinds.second;
        }
      } else if (expression.kind == "popcount") {
        if (expression.operands.size() != 1)
          return planError("popcount expression contract is malformed");
        auto operand = valueTypes.find(expression.operands.front());
        auto inputWidth = operand == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(operand->getValue());
        if (!inputWidth || !acir::isPrimitiveInputWidth(*inputWidth))
          return planError("popcount input must be an i1..i64 value");
        const unsigned resultWidth = acir::primitiveCountWidth(*inputWidth);
        if (expression.type != "i" + std::to_string(resultWidth))
          return planError("popcount expression result type is inconsistent");
      } else if (expression.kind == "count_zeros") {
        if (expression.operands.size() != 1)
          return planError("count_zeros expression contract is malformed");
        if (expression.predicate != "leading" &&
            expression.predicate != "trailing")
          return planError("count_zeros direction is malformed");
        auto operand = valueTypes.find(expression.operands.front());
        auto inputWidth = operand == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(operand->getValue());
        if (!inputWidth || !acir::isPrimitiveInputWidth(*inputWidth))
          return planError("count_zeros input must be an i1..i64 value");
        const unsigned resultWidth = acir::primitiveCountWidth(*inputWidth);
        if (expression.type != "i" + std::to_string(resultWidth))
          return planError(
              "count_zeros expression result type is inconsistent");
      } else if (expression.kind == "value_select") {
        if (expression.operands.size() != 3)
          return planError("value_select expression contract is malformed");
        auto condition = valueTypes.find(expression.operands[0]);
        auto trueValue = valueTypes.find(expression.operands[1]);
        auto falseValue = valueTypes.find(expression.operands[2]);
        if (condition == valueTypes.end() || condition->getValue() != "i1" ||
            trueValue == valueTypes.end() || falseValue == valueTypes.end() ||
            trueValue->getValue() != expression.type ||
            falseValue->getValue() != expression.type)
          return planError("value_select expression types are inconsistent");
      } else if (expression.kind == "range_add" ||
                 expression.kind == "range_sub") {
        auto left = expression.operands.size() == 2
                        ? valueTypes.find(expression.operands[0])
                        : valueTypes.end();
        auto right = expression.operands.size() == 2
                         ? valueTypes.find(expression.operands[1])
                         : valueTypes.end();
        auto leftBounds = left == valueTypes.end()
                              ? std::optional<std::pair<uint64_t, uint64_t>>()
                              : rangeBounds(left->getValue());
        auto rightBounds = right == valueTypes.end()
                               ? std::optional<std::pair<uint64_t, uint64_t>>()
                               : rangeBounds(right->getValue());
        auto resultBounds = rangeBounds(expression.type);
        uint64_t lower = 0;
        uint64_t upper = 0;
        bool valid = leftBounds && rightBounds && resultBounds;
        if (valid && expression.kind == "range_add") {
          valid = rightBounds->first <= std::numeric_limits<uint64_t>::max() -
                                            leftBounds->first &&
                  rightBounds->second <=
                      std::numeric_limits<uint64_t>::max() - leftBounds->second;
          if (valid) {
            lower = leftBounds->first + rightBounds->first;
            upper = leftBounds->second + rightBounds->second;
          }
        } else if (valid) {
          valid = leftBounds->first >= rightBounds->second;
          if (valid) {
            lower = leftBounds->first - rightBounds->second;
            upper = leftBounds->second - rightBounds->first;
          }
        }
        if (!valid || *resultBounds != std::pair{lower, upper})
          return planError("bounded arithmetic result range is inconsistent");
      } else if (expression.kind == "range_cmp") {
        auto left = expression.operands.size() == 2
                        ? valueTypes.find(expression.operands[0])
                        : valueTypes.end();
        auto right = expression.operands.size() == 2
                         ? valueTypes.find(expression.operands[1])
                         : valueTypes.end();
        if (left == valueTypes.end() || right == valueTypes.end() ||
            !rangeBounds(left->getValue()) || !rangeBounds(right->getValue()) ||
            expression.type != "i1" ||
            !llvm::is_contained(llvm::ArrayRef<llvm::StringRef>{"eq", "ne",
                                                                "ult", "ule",
                                                                "ugt", "uge"},
                                expression.predicate))
          return planError("bounded comparison contract is malformed");
      } else if (expression.kind == "range_wrap" ||
                 expression.kind == "range_saturate") {
        auto target = rangeBounds(expression.type);
        auto input = expression.operands.size() == 1
                         ? valueTypes.find(expression.operands.front())
                         : valueTypes.end();
        auto inputWidth = input == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(input->getValue());
        if (!target || !inputWidth || *inputWidth > 64)
          return planError("range conversion contract is malformed");
      } else if (expression.kind == "range_checked_value" ||
                 expression.kind == "range_checked_valid") {
        auto target = rangeBounds(expression.field);
        auto input = expression.operands.size() == 1
                         ? valueTypes.find(expression.operands.front())
                         : valueTypes.end();
        auto inputWidth = input == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(input->getValue());
        if (!target || !inputWidth || *inputWidth > 64 ||
            expression.literal.empty() ||
            (expression.kind == "range_checked_value" &&
             expression.type != expression.field) ||
            (expression.kind == "range_checked_valid" &&
             expression.type != "i1"))
          return planError("checked range conversion contract is malformed");
        auto &pair = rangeCheckedPairs[expression.literal];
        if (expression.kind == "range_checked_value") {
          pair.value = &expression;
          ++pair.valueCount;
        } else {
          pair.valid = &expression;
          ++pair.validCount;
        }
      } else if (expression.kind == "range_refine") {
        auto target = rangeBounds(expression.type);
        auto input = expression.operands.size() == 1
                         ? valueTypes.find(expression.operands.front())
                         : valueTypes.end();
        auto inputWidth = input == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(input->getValue());
        if (!target || !inputWidth || *inputWidth > 64)
          return planError("strict range refinement contract is malformed");
      } else if (expression.kind == "range_bits") {
        auto input = expression.operands.size() == 1
                         ? valueTypes.find(expression.operands.front())
                         : valueTypes.end();
        auto bounds = input == valueTypes.end()
                          ? std::optional<std::pair<uint64_t, uint64_t>>()
                          : rangeBounds(input->getValue());
        auto inputWidth = input == valueTypes.end()
                              ? std::optional<unsigned>()
                              : integerWidth(input->getValue());
        auto resultWidth = bitsWidth(expression.type);
        if (!bounds || !inputWidth || !resultWidth ||
            *resultWidth != *inputWidth)
          return planError("range_bits conversion contract is malformed");
      } else if (expression.kind == "bit_extract") {
        if (expression.operands.size() != 1 || expression.width == 0)
          return planError("bit_extract expression contract is malformed");
        auto input = valueTypes.find(expression.operands.front());
        auto inputWidth = input == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(input->getValue());
        auto resultWidth = bitsWidth(expression.type);
        if (!inputWidth || !resultWidth || *inputWidth > 64 ||
            expression.lsb + expression.width > *inputWidth ||
            expression.width != *resultWidth)
          return planError("bit_extract expression widths are inconsistent");
      } else if (expression.kind == "bit_concat") {
        if (expression.operands.empty())
          return planError("bit_concat requires at least one operand");
        uint64_t totalWidth = 0;
        for (const std::string &operandName : expression.operands) {
          auto operand = valueTypes.find(operandName);
          auto width = operand == valueTypes.end()
                           ? std::optional<unsigned>()
                           : bitsWidth(operand->getValue());
          if (!width || *width == 0 || *width > 64)
            return planError("bit_concat operand width is invalid");
          totalWidth += *width;
        }
        auto resultWidth = bitsWidth(expression.type);
        if (!resultWidth || totalWidth == 0 || totalWidth > 64 ||
            totalWidth != *resultWidth)
          return planError("bit_concat result width is inconsistent");
      } else if (expression.kind == "bit_insert") {
        if (expression.operands.size() != 2)
          return planError("bit_insert expression contract is malformed");
        auto base = valueTypes.find(expression.operands[0]);
        auto value = valueTypes.find(expression.operands[1]);
        auto baseWidth = base == valueTypes.end() ? std::optional<unsigned>()
                                                  : bitsWidth(base->getValue());
        auto valueWidth = value == valueTypes.end()
                              ? std::optional<unsigned>()
                              : bitsWidth(value->getValue());
        auto resultWidth = bitsWidth(expression.type);
        if (!baseWidth || !valueWidth || !resultWidth || *baseWidth > 64 ||
            expression.lsb + *valueWidth > *baseWidth ||
            *resultWidth != *baseWidth)
          return planError("bit_insert expression widths are inconsistent");
      } else if (expression.kind == "recovery_event") {
        if (expression.operands.size() != 4 || expression.type != "i1" ||
            expression.field.empty() || expression.predicate.empty())
          return planError("recovery-event expression contract is malformed");
      } else if (expression.kind == "kill_set") {
        if (expression.operands.size() != 5 || expression.type != "i1" ||
            expression.predicate != "epoch_mismatch_or_younger")
          return planError("kill-set expression contract is malformed");
      } else if (expression.kind == "reservation_set" ||
                 expression.kind == "transaction_group" ||
                 expression.kind.starts_with("multi_allocator_") ||
                 expression.kind == "age_select_k" ||
                 expression.kind.starts_with("dependency_set_") ||
                 expression.kind == "terminal_transaction" ||
                 expression.kind == "memory_order_edge" ||
                 expression.kind.starts_with("load_disposition_")) {
        const uint64_t lanes = expression.selectionCount;
        auto resultWidth = bitsWidth(expression.type);
        auto masksHaveWidth = [&](size_t count) {
          if (expression.operands.size() < count)
            return false;
          for (llvm::StringRef operandName :
               llvm::ArrayRef(expression.operands).take_front(count)) {
            auto operand = valueTypes.find(operandName);
            auto width = operand == valueTypes.end()
                             ? std::optional<unsigned>()
                             : bitsWidth(operand->getValue());
            if (!width || *width != lanes)
              return false;
          }
          return true;
        };
        if (lanes == 0 || lanes > 64)
          return planError("transaction algebra lane count is invalid");
        if (expression.kind == "reservation_set") {
          if (expression.operands.empty() || !resultWidth ||
              *resultWidth != lanes ||
              !masksHaveWidth(expression.operands.size()) ||
              (expression.predicate != "preview" &&
               expression.predicate != "commit") ||
              (expression.predicate == "commit" &&
               (expression.operands.size() < 2 ||
                llvm::any_of(llvm::ArrayRef(expression.operands).drop_front(),
                             [&](llvm::StringRef operand) {
                               return operand != expression.operands.front();
                             }))))
            return planError("reservation-set expression is malformed");
        } else if (expression.kind == "transaction_group") {
          if (expression.operands.size() != 2 || !resultWidth ||
              *resultWidth != lanes || !masksHaveWidth(2) ||
              (expression.predicate != "all_or_none" &&
               expression.predicate != "valid_prefix" &&
               expression.predicate != "independent"))
            return planError("transaction-group expression is malformed");
        } else if (expression.kind.starts_with("multi_allocator_")) {
          if (expression.operands.size() != 3 || !resultWidth ||
              *resultWidth != lanes || !masksHaveWidth(3) ||
              expression.laneOrdinal > 2 || expression.width == 0 ||
              expression.width > 64 ||
              (expression.predicate != "allow" &&
               expression.predicate != "forbid") ||
              expression.literal != "increment_on_allocate")
            return planError("multi-allocator expression is malformed");
        } else if (expression.kind == "age_select_k") {
          if (expression.operands.size() != lanes + 1 || !resultWidth ||
              *resultWidth != lanes || !masksHaveWidth(1) ||
              expression.laneOrdinal == 0 ||
              expression.laneOrdinal > lanes ||
              expression.predicate != "oldest_first")
            return planError("age-select-k expression is malformed");
        } else if (expression.kind.starts_with("dependency_set_")) {
          const bool ready = expression.kind == "dependency_set_ready";
          if (expression.operands.size() != 5 || !masksHaveWidth(5) ||
              !resultWidth || *resultWidth != (ready ? 1 : lanes))
            return planError("dependency-set expression is malformed");
        } else if (expression.kind == "terminal_transaction") {
          if (expression.operands.size() != 3 || !resultWidth ||
              *resultWidth != lanes || !masksHaveWidth(3))
            return planError("terminal-transaction expression is malformed");
        } else if (expression.kind == "memory_order_edge") {
          const bool closedKind =
              expression.predicate == "older_than" ||
              expression.predicate == "must_wait" ||
              expression.predicate == "may_bypass" ||
              expression.predicate == "must_forward" ||
              expression.predicate == "must_replay_if" ||
              expression.predicate == "visibility_before";
          if (!closedKind || expression.operands.size() < 2 ||
              expression.operands.size() > 3 || !resultWidth ||
              *resultWidth != lanes ||
              !masksHaveWidth(expression.operands.size()))
            return planError("memory-order-edge expression is malformed");
        } else {
          const bool closedKind =
              expression.kind == "load_disposition_wait" ||
              expression.kind == "load_disposition_bypass" ||
              expression.kind == "load_disposition_forward" ||
              expression.kind == "load_disposition_replay" ||
              expression.kind == "load_disposition_stale";
          if (!closedKind || expression.operands.size() != 7 ||
              !resultWidth || *resultWidth != lanes || !masksHaveWidth(7))
            return planError("load-disposition expression is malformed");
        }
      } else if (expression.kind == "versioned_lookup_payload" ||
                 expression.kind == "versioned_lookup_valid") {
        const TablePlan *table = tables.lookup(expression.table);
        if (!table || !table->versioned ||
            (expression.operands.size() != 3 &&
             expression.operands.size() != 4) ||
            (table->attemptBits == 0) !=
                (expression.operands.size() == 3) ||
            (expression.kind == "versioned_lookup_valid" &&
             expression.type != "i1"))
          return planError("versioned lookup expression contract is malformed");
      } else if (expression.kind == "snapshot_set") {
        if (!expression.operands.empty() ||
            expression.type != "state_reservation" ||
            expression.field.empty() || !tables.contains(expression.table) ||
            (expression.predicate != "complete" &&
             expression.predicate != "fields"))
          return planError("snapshot-set expression contract is malformed");
      } else if (expression.kind != "table_match_ref" &&
                 expression.kind != "table_selection_index_ref" &&
                 expression.kind != "table_selection_valid_ref") {
        return planError("unsupported QueueGraph expression kind");
      }
      valueTypes[expression.result] = expression.type;
      valueDefinitions[expression.result] = &expression;
      if (!expression.nestedExpressions.empty()) {
        const TablePlan *table = tables.lookup(expression.table);
        llvm::SmallVector<std::string> nestedRoots;
        if (table)
          nestedRoots.push_back(table->entryType);
        llvm::StringMap<std::string> nestedInheritedTypes;
        for (const std::string &operand : expression.operands)
          if (auto found = valueTypes.find(operand); found != valueTypes.end())
            nestedInheritedTypes[operand] = found->getValue();
        if (auto error = self(self, expression.nestedExpressions, nestedRoots,
                              "entry", nestedInheritedTypes))
          return error;
      }
    }
    for (const auto &entry : inlineChoiceKinds)
      if (entry.getValue().first == 0 ||
          entry.getValue().first != entry.getValue().second)
        return planError(
            "inline table choose requires balanced index/valid result pairs");
    for (const auto &entry : rangeCheckedPairs) {
      const auto &pair = entry.getValue();
      const auto *value = pair.value;
      const auto *valid = pair.valid;
      if (pair.valueCount != 1 || pair.validCount != 1 || !value || !valid ||
          value->field != valid->field || value->operands != valid->operands ||
          value->staticTypeTarget != valid->staticTypeTarget)
        return planError(
            "checked range conversion requires one value/valid pair");
    }
    return llvm::Error::success();
  };
  for (const QueueHelperPlan &helper : plan.helpers) {
    llvm::StringMap<std::string> inputTypes;
    for (auto [name, type] :
         llvm::zip_equal(helper.inputNames, helper.inputTypes))
      inputTypes[name] = type;
    if (auto error = verifyExpressionList(
            verifyExpressionList, helper.expressions, {}, "", inputTypes))
      return error;
    llvm::StringMap<std::string> valueTypes;
    for (auto [name, type] :
         llvm::zip_equal(helper.inputNames, helper.inputTypes))
      valueTypes[name] = type;
    for (const QueueExpressionPlan &expression : helper.expressions)
      valueTypes[expression.result] = expression.type;
    for (auto [yield, expected] :
         llvm::zip_equal(helper.yields, helper.resultTypes))
      if (valueTypes.lookup(yield) != expected)
        return planError("Queue helper return type is inconsistent");
  }
  auto verifyTableGetConstraints =
      [&](auto &&self, const std::vector<QueueExpressionPlan> &expressions,
          llvm::ArrayRef<std::string> rootTypes,
          const llvm::StringMap<std::string> &inheritedTypes) -> llvm::Error {
    llvm::StringMap<std::string> types;
    llvm::StringMap<ValueConstraint> constraints;
    for (const auto &entry : inheritedTypes) {
      types[entry.getKey()] = entry.getValue();
      constraints[entry.getKey()] = planTypeConstraint(entry.getValue());
    }
    for (auto [index, rootType] : llvm::enumerate(rootTypes)) {
      std::string identity =
          index == 0 ? "item" : "item" + std::to_string(index);
      types[identity] = rootType;
      constraints[identity] = planTypeConstraint(rootType);
    }
    for (const QueueExpressionPlan &expression : expressions) {
      types[expression.result] = expression.type;
      constraints[expression.result] =
          inferPlanConstraint(expression, constraints, types, tables);
      if (expression.kind == "table_get") {
        const TablePlan *table = tables.lookup(expression.table);
        auto index = expression.operands.size() == 1
                         ? constraints.find(expression.operands.front())
                         : constraints.end();
        if (!table || index == constraints.end() || table->entries == 0 ||
            !index->getValue().provesWithin(0, table->entries - 1))
          return planError("Table observation index is not statically safe");
      }
      if (expression.kind == "array_get_dynamic") {
        auto index = expression.operands.size() == 2
                         ? constraints.find(expression.operands[1])
                         : constraints.end();
        if (index == constraints.end() || expression.selectionCount == 0 ||
            !index->getValue().provesWithin(0, expression.selectionCount - 1))
          return planError("value_array index is not statically safe");
      }
      if (expression.kind == "array_update_dynamic") {
        auto index = expression.operands.size() == 3
                         ? constraints.find(expression.operands[1])
                         : constraints.end();
        if (index == constraints.end() || expression.selectionCount == 0 ||
            !index->getValue().provesWithin(0, expression.selectionCount - 1))
          return planError("value_array update index is not statically safe");
      }
      if (expression.kind == "range_refine") {
        auto target = rangeBounds(expression.type);
        auto input = expression.operands.size() == 1
                         ? constraints.find(expression.operands.front())
                         : constraints.end();
        if (!target || input == constraints.end() ||
            !input->getValue().provesWithin(target->first, target->second))
          return planError("strict range refinement is not statically safe");
      }
      if (!expression.nestedExpressions.empty()) {
        const TablePlan *table = tables.lookup(expression.table);
        llvm::SmallVector<std::string> nestedRoots;
        if (table)
          nestedRoots.push_back(table->entryType);
        llvm::StringMap<std::string> noInheritedTypes;
        if (auto error = self(self, expression.nestedExpressions, nestedRoots,
                              noInheritedTypes))
          return error;
      }
    }
    return llvm::Error::success();
  };
  for (const QueueHelperPlan &helper : plan.helpers) {
    llvm::StringMap<std::string> inputTypes;
    for (auto [name, type] :
         llvm::zip_equal(helper.inputNames, helper.inputTypes))
      inputTypes[name] = type;
    if (auto error = verifyTableGetConstraints(
            verifyTableGetConstraints, helper.expressions, {}, inputTypes))
      return error;
  }
  for (const TableMatchPlan &match : plan.tableMatches) {
    const TablePlan *table = tables.lookup(match.table);
    llvm::SmallVector<std::string> roots;
    if (table)
      roots.push_back(table->entryType);
    llvm::StringMap<std::string> noInheritedTypes;
    if (auto error =
            verifyExpressionList(verifyExpressionList, match.expressions, roots,
                                 "item", noInheritedTypes))
      return error;
    if (auto error = verifyTableGetConstraints(verifyTableGetConstraints,
                                               match.expressions, roots,
                                               noInheritedTypes))
      return error;
  }
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    const TablePlan *table = tables.lookup(selection.table);
    llvm::SmallVector<std::string> roots;
    if (table)
      roots.push_back(table->entryType);
    llvm::StringMap<std::string> noInheritedTypes;
    if (auto error =
            verifyExpressionList(verifyExpressionList, selection.keyExpressions,
                                 roots, "item", noInheritedTypes))
      return error;
    if (auto error = verifyTableGetConstraints(verifyTableGetConstraints,
                                               selection.keyExpressions, roots,
                                               noInheritedTypes))
      return error;
  }

  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "source" &&
        llvm::any_of(block.outputs, [&](llvm::StringRef output) {
          llvm::StringSet<> active;
          return containsDeclaredRange(containsDeclaredRange,
                                       queueTypes.lookup(output), active);
        }))
      return planError(
          "external source cannot carry an undecoded bounded range");
    llvm::SmallVector<std::string> roots;
    for (const std::string &input : block.inputs)
      if (auto found = queueTypes.find(input); found != queueTypes.end())
        roots.push_back(found->getValue());
    if (block.kind == "table_masked_write")
      if (const TablePlan *table = tables.lookup(block.table))
        roots.push_back(table->entryType);
    llvm::StringMap<std::string> noInheritedTypes;
    if (auto error =
            verifyExpressionList(verifyExpressionList, block.expressions, roots,
                                 "item", noInheritedTypes))
      return error;
    if (auto error = verifySharedExpressions(block.expressions))
      return error;
    if (auto error = verifyTableGetConstraints(verifyTableGetConstraints,
                                               block.expressions, roots,
                                               noInheritedTypes))
      return error;
    llvm::StringMap<std::string> identities;
    llvm::StringMap<ValueConstraint> constraints;
    for (auto [index, rootType] : llvm::enumerate(roots)) {
      std::string identity =
          index == 0 ? "item" : "item" + std::to_string(index);
      identities[identity] = rootType;
      constraints[identity] = planTypeConstraint(rootType);
    }
    for (const QueueExpressionPlan &expression : block.expressions) {
      identities[expression.result] = expression.type;
      constraints[expression.result] =
          inferPlanConstraint(expression, constraints, identities, tables);
    }
    if (!block.provider.empty()) {
      if (block.kind != "dependency" || block.provider != "v2" ||
          block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.capacity == 0 || block.resources == 0 ||
          block.yields.size() != 4)
        return planError("schedule provider metadata is unsupported");
      auto key = identities.find(block.yields[0]);
      auto predecessor = identities.find(block.yields[1]);
      auto resource = identities.find(block.yields[2]);
      auto cost = identities.find(block.yields[3]);
      std::optional<unsigned> keyWidth =
          key == identities.end() ? std::nullopt : bitsWidth(key->getValue());
      std::optional<unsigned> resourceWidth =
          resource == identities.end() ? std::nullopt
                                       : bitsWidth(resource->getValue());
      std::optional<unsigned> costWidth =
          cost == identities.end() ? std::nullopt : bitsWidth(cost->getValue());
      if (!keyWidth || *keyWidth == 0 || *keyWidth > 16 ||
          predecessor == identities.end() ||
          predecessor->getValue() != key->getValue() || !resourceWidth ||
          *resourceWidth == 0 || *resourceWidth > 64 || !costWidth ||
          *costWidth == 0 || *costWidth > 64 ||
          block.noDependency != (uint64_t{1} << *keyWidth) - 1)
        return planError(
            "schedule v2 requires exact integer policy types, matching key "
            "and predecessor, and an all-ones sentinel with key width <= 16");
    }
    auto verifySafeIndex = [&](llvm::StringRef identity,
                               const TablePlan &table) -> bool {
      auto type = identities.find(identity);
      auto width = type == identities.end() ? std::optional<unsigned>()
                                            : integerWidth(type->getValue());
      if (!width || *width == 0 || *width > 64)
        return false;
      auto flattened = llvm::find_if(
          block.expressions, [&](const QueueExpressionPlan &expression) {
            return expression.result == identity &&
                   expression.kind == "table_index" &&
                   expression.table == table.name;
          });
      if (flattened != block.expressions.end())
        return true;
      auto constraint = constraints.find(identity);
      return constraint != constraints.end() && table.entries != 0 &&
             constraint->getValue().provesWithin(0, table.entries - 1);
    };
    if (block.kind == "firing") {
      if (!identities.contains(block.guard) ||
          llvm::any_of(block.yields,
                       [&](const std::string &yield) {
                         return !identities.contains(yield);
                       }) ||
          llvm::any_of(block.stateWrites,
                       [&](const StateWritePlan &write) {
                         return !identities.contains(write.index) ||
                                !identities.contains(write.value) ||
                                (!write.present.empty() &&
                                 !identities.contains(write.present)) ||
                                (!write.refGeneration.empty() &&
                                 !identities.contains(write.refGeneration)) ||
                                (!write.refEpoch.empty() &&
                                 !identities.contains(write.refEpoch)) ||
                                (!write.refAttempt.empty() &&
                                 !identities.contains(write.refAttempt));
                       }) ||
          llvm::any_of(block.outputPresence,
                       [&](const OutputPresencePlan &output) {
                         return !identities.contains(output.value) ||
                                !identities.contains(output.present);
                       }))
        return planError("state firing value identities are not closed");
      if (identities.lookup(block.guard) != "i1")
        return planError("state firing functional condition must be i1");
      for (auto [output, yield] : llvm::zip_equal(block.outputs, block.yields))
        if (!queueTypes.contains(output) ||
            queueTypes.lookup(output) != identities.lookup(yield))
          return planError(
              "state firing output Queue and yielded value types must match");
      const bool candidateAlways = llvm::any_of(
          block.expressions, [&](const QueueExpressionPlan &expression) {
            return expression.result == block.guard &&
                   expression.kind == "constant" &&
                   expression.literal == "true";
          });
      auto verifyEffectPresence = [&](llvm::StringRef present) {
        if (identities.lookup(present) != "i1")
          return false;
        if (present == block.guard)
          return true;
        return candidateAlways;
      };
      for (const StateWritePlan &write : block.stateWrites) {
        const TablePlan *table = tables.lookup(write.table);
        if (!table || identities.lookup(write.value) != table->entryType ||
            !verifySafeIndex(write.index, *table))
          return planError(
              "state firing proposal type/index is not statically safe");
        if (!write.present.empty() && !verifyEffectPresence(write.present))
          return planError(
              "state firing proposal presence must imply its candidate");
      }
      llvm::StringMap<llvm::SmallVector<const StateWritePlan *, 4>>
          writesByOwner;
      for (const StateWritePlan &write : block.stateWrites)
        writesByOwner[write.table].push_back(&write);

      llvm::StringMap<uint64_t> canonicalMemo;
      llvm::StringMap<uint64_t> canonicalExpressions;
      uint64_t nextCanonicalIdentity = 1;
      auto structurallyPure = [](const QueueExpressionPlan &expression) {
        if (!expression.nestedExpressions.empty() ||
            !expression.nestedYields.empty())
          return false;
        return llvm::StringSwitch<bool>(expression.kind)
            .Cases({"constant", "enum_constant", "get", "value_select"}, true)
            .Cases({"add", "sub", "mul", "udiv", "urem", "and", "or", "xor",
                    "not"},
                   true)
            .Cases({"shl", "shr", "extract", "insert", "concat"}, true)
            .Cases({"popcount", "count_zeros", "cmp", "masked_match"}, true)
            .Cases({"range_wrap", "range_saturate", "range_refine",
                    "range_bits", "range_add", "range_sub", "range_cmp"},
                   true)
            .Cases({"range_checked_value", "range_checked_valid",
                    "array_get_dynamic", "array_update_dynamic"},
                   true)
            .Default(false);
      };
      // Canonical identity is computed bottom-up over structurally pure
      // expressions. An explicit stack with a post-visit marker keeps deep
      // chains off the C++ stack; operands are pushed in reverse so the
      // pre-order identity assignment and the interned keys are unchanged.
      auto canonicalExpression = [&](llvm::StringRef root) -> uint64_t {
        struct Frame {
          std::string identity;
          size_t expressionIndex = 0;
          bool expanded = false;
        };
        llvm::SmallVector<Frame, 16> pending;
        pending.push_back({root.str(), 0, false});
        while (!pending.empty()) {
          Frame frame = std::move(pending.back());
          pending.pop_back();
          if (!frame.expanded) {
            if (canonicalMemo.find(frame.identity) != canonicalMemo.end())
              continue;
            size_t index = block.expressions.size();
            for (size_t candidate = 0; candidate < block.expressions.size();
                 ++candidate)
              if (block.expressions[candidate].result == frame.identity) {
                index = candidate;
                break;
              }
            canonicalMemo[frame.identity] = nextCanonicalIdentity++;
            if (index == block.expressions.size() ||
                !structurallyPure(block.expressions[index]))
              continue;
            pending.push_back({frame.identity, index, true});
            const std::vector<std::string> &names =
                block.expressions[index].operands;
            for (auto it = names.rbegin(); it != names.rend(); ++it)
              if (canonicalMemo.find(*it) == canonicalMemo.end())
                pending.push_back({*it, 0, false});
            continue;
          }
          const QueueExpressionPlan &expression =
              block.expressions[frame.expressionIndex];
          std::vector<uint64_t> operands;
          for (const std::string &operand : expression.operands)
            operands.push_back(canonicalMemo.lookup(operand));
          if (expression.kind == "mul" || expression.kind == "and" ||
              expression.kind == "or" || expression.kind == "xor" ||
              (expression.kind == "cmp" &&
               (expression.predicate == "eq" || expression.predicate == "ne")))
            llvm::sort(operands);
          std::string key;
          llvm::raw_string_ostream stream(key);
          auto writeString = [&](llvm::StringRef value) {
            stream << value.size() << ':' << value;
          };
          for (llvm::StringRef value : {llvm::StringRef(expression.kind),
                                        llvm::StringRef(expression.type),
                                        llvm::StringRef(expression.field),
                                        llvm::StringRef(expression.literal),
                                        llvm::StringRef(expression.predicate),
                                        llvm::StringRef(expression.table),
                                        llvm::StringRef(expression.slot),
                                        llvm::StringRef(expression.mask),
                                        llvm::StringRef(expression.value)})
            writeString(value);
          stream << expression.lsb << ':' << expression.width << ':';
          for (uint64_t operand : operands)
            stream << operand << ',';
          const uint64_t result =
              canonicalExpressions
                  .try_emplace(key, canonicalMemo.lookup(frame.identity))
                  .first->getValue();
          canonicalMemo[frame.identity] = result;
        }
        return canonicalMemo.lookup(root);
      };
      struct Literal {
        uint64_t atom = 0;
        bool negated = false;
      };
      auto isFalse = [&](llvm::StringRef identity) {
        auto expression = llvm::find_if(
            block.expressions, [&](const QueueExpressionPlan &candidate) {
              return candidate.result == identity;
            });
        return expression != block.expressions.end() &&
               expression->kind == "constant" && expression->literal == "false";
      };
      auto collectConjuncts =
          [&](llvm::StringRef root, bool rootNegated,
              llvm::DenseSet<std::pair<uint64_t, uint8_t>> &visited,
              llvm::SmallVectorImpl<Literal> &literals) {
            // Same walk as the analysis-side collector: an explicit LIFO
            // worklist keeps deep `and`/`mul` chains off the C++ stack, with
            // operands pushed in reverse so literals keep their order.
            llvm::SmallVector<std::pair<std::string, bool>, 16> pending;
            pending.push_back({root.str(), rootNegated});
            while (!pending.empty()) {
              auto [identity, negated] = std::move(pending.back());
              pending.pop_back();
              const uint64_t atom = canonicalExpression(identity);
              if (!visited.insert({atom, static_cast<uint8_t>(negated)}).second)
                continue;
              size_t index = block.expressions.size();
              for (size_t candidate = 0; candidate < block.expressions.size();
                   ++candidate)
                if (block.expressions[candidate].result == identity) {
                  index = candidate;
                  break;
                }
              const bool found = index < block.expressions.size();
              if (found && !negated && block.expressions[index].type == "i1" &&
                  (block.expressions[index].kind == "mul" ||
                   block.expressions[index].kind == "and") &&
                  block.expressions[index].operands.size() == 2) {
                literals.push_back({atom, false});
                pending.push_back(
                    {block.expressions[index].operands[1], false});
                pending.push_back(
                    {block.expressions[index].operands[0], false});
                continue;
              }
              if (found && block.expressions[index].kind == "cmp" &&
                  block.expressions[index].predicate == "eq" &&
                  block.expressions[index].operands.size() == 2) {
                if (isFalse(block.expressions[index].operands[0])) {
                  pending.push_back(
                      {block.expressions[index].operands[1], !negated});
                  continue;
                }
                if (isFalse(block.expressions[index].operands[1])) {
                  pending.push_back(
                      {block.expressions[index].operands[0], !negated});
                  continue;
                }
              }
              literals.push_back({atom, negated});
            }
          };
      auto mutuallyExclusive = [&](llvm::StringRef left,
                                   llvm::StringRef right) {
        llvm::SmallVector<Literal> leftLiterals;
        llvm::SmallVector<Literal> rightLiterals;
        llvm::DenseSet<std::pair<uint64_t, uint8_t>> leftVisited;
        llvm::DenseSet<std::pair<uint64_t, uint8_t>> rightVisited;
        collectConjuncts(left, false, leftVisited, leftLiterals);
        collectConjuncts(right, false, rightVisited, rightLiterals);
        return llvm::any_of(leftLiterals, [&](const Literal &lhs) {
          return llvm::any_of(rightLiterals, [&](const Literal &rhs) {
            return lhs.atom == rhs.atom && lhs.negated != rhs.negated;
          });
        });
      };
      for (auto &owner : writesByOwner) {
        auto &writes = owner.getValue();
        for (size_t right = 1; right < writes.size(); ++right)
          for (size_t left = 0; left < right; ++left) {
            auto leftConstraint = constraints.find(writes[left]->index);
            auto rightConstraint = constraints.find(writes[right]->index);
            const bool disjoint = leftConstraint != constraints.end() &&
                                  rightConstraint != constraints.end() &&
                                  leftConstraint->getValue().provesDisjoint(
                                      rightConstraint->getValue());
            const bool exclusive = !writes[left]->present.empty() &&
                                   !writes[right]->present.empty() &&
                                   mutuallyExclusive(writes[left]->present,
                                                     writes[right]->present);
            const bool disjointFields =
                writes[left]->mode == "field" &&
                writes[right]->mode == "field" &&
                llvm::none_of(
                    writes[left]->fields, [&](const std::string &field) {
                      return llvm::is_contained(writes[right]->fields, field);
                    });
            if (!disjoint && !exclusive && !disjointFields)
              return planError(
                  "same-owner firing writes have an unresolved index/field "
                  "overlap");
          }
      }
      for (const OutputPresencePlan &output : block.outputPresence)
        if (!verifyEffectPresence(output.present))
          return planError(
              "state firing output presence must imply its candidate");
      for (const StateReservationPlan &reservation : block.stateReservations) {
        const TablePlan *table = tables.lookup(reservation.table);
        if (!table || identities.lookup(reservation.predicate) != "i1")
          return planError("state snapshot reservation is malformed");
        if (!verifyWriteFields(reservation.table, "field", reservation.fields,
                               false, false))
          return planError("state snapshot reservation fields are invalid");
        std::optional<size_t> fieldCount = tableFieldCount(*table);
        if (!fieldCount || *fieldCount == 0 || *fieldCount > 64)
          return planError(
              "field-qualified state reservation exceeds relation capacity");
        if (reservation.indexKind == "set") {
          if (!reservation.index.empty() || reservation.source.empty())
            return planError("snapshot-set reservation is malformed");
          auto source = llvm::find_if(
              block.expressions, [&](const QueueExpressionPlan &expression) {
                return expression.result == reservation.source &&
                       (expression.kind == "table_match" ||
                        expression.kind == "table_choose_index");
              });
          if (source == block.expressions.end() ||
              !llvm::any_of(source->nestedExpressions,
                            [&](const QueueExpressionPlan &expression) {
                              return expression.kind == "table_get" &&
                                     expression.table == reservation.table;
                            }))
            return planError(
                "snapshot-set source does not read its target table");
        } else if (reservation.indexKind == "all") {
          if (!reservation.index.empty() || !reservation.source.empty())
            return planError(
                "all-entry state snapshot must not carry an index");
        } else if ((reservation.indexKind != "static" &&
                    reservation.indexKind != "dynamic") ||
                   reservation.index.empty() || !reservation.source.empty() ||
                   !verifySafeIndex(reservation.index, *table)) {
          return planError("state snapshot reservation index is not safe");
        }
        auto containsState = [&](const auto &resources) {
          return llvm::any_of(resources, [&](const auto &resource) {
            return resource.kind == "state" &&
                   resource.resource == reservation.table;
          });
        };
        if (block.hasActivationEvidence &&
            !containsState(block.activationSources))
          return planError("state snapshot owner must be an activation source");
        const bool writable =
            llvm::any_of(block.stateWrites, [&](const StateWritePlan &write) {
              return write.table == reservation.table;
            });
        if (!writable && containsState(block.transactionResources))
          return planError(
              "reservation-only state must not be a transaction resource");
      }
    }
    if (block.kind == "memory_request" &&
        !memoryInstances.contains(block.memoryInstance))
      return planError("memory request block references unknown instance");
    if ((block.kind == "table_read" || block.kind == "table_read_group" ||
         block.kind == "table_write" || block.kind == "table_masked_write") &&
        !tables.contains(block.table))
      return planError("table endpoint block references unknown table");
    if (block.kind == "table_read_group") {
      const TableSelectionPlan *selection =
          tableSelections.lookup(block.selection);
      if (!selection || selection->table != block.table ||
          block.selectionCount != selection->count ||
          block.inputs.size() != 0 ||
          block.outputs.size() != block.selectionCount ||
          block.depths.size() != block.selectionCount ||
          block.latencies.size() != block.selectionCount)
        return planError("selection TableRead group metadata is inconsistent");
    }
    if (block.kind == "table_read" || block.kind == "table_write") {
      const TablePlan *table = tables.lookup(block.table);
      const size_t expectedYields = block.kind == "table_read" ? 2 : 3;
      if (!table || block.yields.size() != expectedYields ||
          !verifySafeIndex(block.yields.front(), *table))
        return planError("Table endpoint address is not statically safe");
    }
    if (block.kind == "table_write") {
      auto endpoint =
          llvm::find_if(plan.tableWrites, [&](const TableWritePlan &write) {
            return write.name == block.name && write.table == block.table &&
                   write.scope == block.scope;
          });
      if (endpoint == plan.tableWrites.end() ||
          endpoint->mode != block.writeMode ||
          endpoint->writeFields != block.writeFields)
        return planError("table write block mode/fields are inconsistent");
    }
    if (block.kind == "table_masked_write") {
      auto endpoint = llvm::find_if(
          plan.tableMaskedWrites, [&](const TableMaskedWritePlan &write) {
            return write.name == block.name && write.table == block.table &&
                   write.scope == block.scope;
          });
      if (endpoint == plan.tableMaskedWrites.end() ||
          endpoint->mode != block.writeMode ||
          endpoint->writeFields != block.writeFields)
        return planError(
            "masked table write block mode/fields are inconsistent");
    }
    if (block.kind == "slot" && !slotNames.contains(block.slot))
      return planError("slot block references unknown slot");
    for (const SlotReleaseEffectPlan &release : block.slotReleases)
      if (!slotNames.contains(release.slot) || release.when.empty())
        return planError("firing slot release metadata is invalid");
    for (const QueueExpressionPlan &expression : block.expressions)
      if (expression.kind == "table_get" && !tables.contains(expression.table))
        return planError("table.get expression references unknown table");
    for (const std::string &input : block.inputs)
      if (!queueNames.contains(input))
        return planError("block input references unknown Queue '" + input +
                         "'");
    for (const std::string &output : block.outputs) {
      if (!queueNames.contains(output))
        return planError("block output references unknown Queue '" + output +
                         "'");
      ++producers[output];
    }
    if (block.kind != "observe" && block.kind != "expect")
      for (const std::string &input : block.inputs)
        ++consumers[input];
    for (const std::string &input : block.inputs)
      for (const std::string &output : block.outputs) {
        successors[input].push_back(output);
        ++indegree[output];
      }
  }

  for (const QueuePlan &queue : plan.queues) {
    if (producers[queue.name] != 1)
      return planError("Queue '" + queue.name +
                       "' must have exactly one producer");
    if (consumers[queue.name] == 0)
      return planError("Queue '" + queue.name +
                       "' has no consuming block; connect ac.sink");
    if (consumers[queue.name] > 1)
      return planError("Queue '" + queue.name +
                       "' has multiple consuming blocks; insert ac.broadcast");
  }
  for (const QueueInterfacePlan &input : plan.interfaceInputs) {
    if (consumers[input.name] == 0)
      return planError("module input Queue '" + input.name +
                       "' has no consuming block");
    if (consumers[input.name] > 1)
      return planError("module input Queue '" + input.name +
                       "' has multiple consuming blocks");
  }

  std::vector<std::string> ready;
  for (const auto &queue : queueNames)
    if (indegree[queue.getKey()] == 0)
      ready.push_back(queue.getKey().str());
  size_t visited = 0;
  for (size_t cursor = 0; cursor < ready.size(); ++cursor) {
    ++visited;
    auto found = successors.find(ready[cursor]);
    if (found == successors.end())
      continue;
    for (const std::string &successor : found->getValue())
      if (--indegree[successor] == 0)
        ready.push_back(successor);
  }
  if (visited != queueNames.size())
    return planError("QueueGraph contains a cycle; represent stateful loops "
                     "with ac.feedback");
  if (!plan.activationEdges.empty() || !plan.workClosureEdges.empty() ||
      !plan.initialActivation.empty()) {
    auto inferredActivation = inferActivation(plan);
    if (!inferredActivation)
      return inferredActivation.takeError();
    if (plan.activationEdges != inferredActivation->wakeEdges ||
        plan.workClosureEdges != inferredActivation->workClosureEdges ||
        plan.initialActivation != inferredActivation->initial)
      return planError("activation, Work closure, and initial frontier must "
                       "equal compiler inference");
  }
  return llvm::Error::success();
}

namespace {

/// Render one typed static value as its canonical attribute text.
///
/// The generated ODS printer overload takes an ``AsmPrinter&`` and hides
/// ``Attribute::print(raw_ostream&)``, so the value is converted explicitly.
std::string renderStaticValueText(mlir::Attribute value) {
  std::string rendered;
  llvm::raw_string_ostream stream(rendered);
  value.print(stream);
  return stream.str();
}

} // namespace

llvm::Expected<std::string> QueueGraphPlan::canonicalJson() const {
  auto provenanceJson = [](const QueueSourceProvenancePlan &provenance) {
    llvm::json::Array origins;
    for (const QueueSourceOriginPlan &origin : provenance.origins) {
      llvm::json::Array frames;
      for (const QueueSourceFramePlan &frame : origin) {
        llvm::json::Object value{{"column", frame.column},
                                 {"file", frame.file},
                                 {"kind", frame.kind},
                                 {"line", frame.line}};
        if (!frame.symbol.empty())
          value["symbol"] = frame.symbol;
        frames.push_back(std::move(value));
      }
      origins.push_back(llvm::json::Object{{"frames", std::move(frames)}});
    }
    return llvm::json::Object{{"origins", std::move(origins)}};
  };
  auto expressionJson =
      [&](auto &&self,
          const QueueExpressionPlan &expression) -> llvm::json::Object {
    llvm::json::Array operands;
    for (const std::string &operand : expression.operands)
      operands.push_back(operand);
    llvm::json::Array nested;
    for (const QueueExpressionPlan &item : expression.nestedExpressions)
      nested.push_back(self(self, item));
    llvm::json::Array nestedYields;
    for (const std::string &yield : expression.nestedYields)
      nestedYields.push_back(yield);
    llvm::json::Object result{{"field", expression.field},
                              {"kind", expression.kind},
                              {"literal", expression.literal},
                              {"nested_expressions", std::move(nested)},
                              {"nested_yields", std::move(nestedYields)},
                              {"operands", std::move(operands)},
                              {"predicate", expression.predicate},
                              {"result", expression.result},
                              {"slot", expression.slot},
                              {"table", expression.table},
                              {"type", expression.type}};
    if (expression.kind == "bit_extract" || expression.kind == "bit_insert" ||
        expression.kind == "aggregate_get")
      result["lsb"] = expression.lsb;
    if (expression.kind == "bit_extract" ||
        expression.kind == "aggregate_get" ||
        expression.kind == "array_get_dynamic" ||
        expression.kind == "array_update_dynamic" ||
        expression.kind == "tuple_create" ||
        expression.kind == "array_create" || expression.kind == "record_create")
      result["width"] = expression.width;
    if (!expression.staticTypeTarget.empty())
      result["static_type_target"] = expression.staticTypeTarget;
    if (expression.kind == "array_get_dynamic" ||
        expression.kind == "array_update_dynamic") {
      const uint64_t count = expression.selectionCount;
      const uint64_t ordinalWidth =
          std::max<uint64_t>(1, llvm::Log2_64_Ceil(count));
      const bool widensIndex = expression.indexWidth < ordinalWidth;
      const uint64_t selectionDepth =
          expression.kind == "array_get_dynamic"
              ? static_cast<uint64_t>(llvm::Log2_64_Ceil(count))
              : uint64_t{1};
      result["cost_model"] = "queuegraph_array_expansion_v1";
      result["node_accounting"] = "emitted_pyc_ops_before_dce";
      result["expansion_factor"] = count;
      result["index_width"] = expression.indexWidth;
      result["selection_tree_depth"] = selectionDepth;
      result["logic_depth_model"] = "pyc_check_logic_depth_unit_cost";
      result["logic_depth"] =
          expression.kind == "array_get_dynamic"
              ? (count == 1 ? uint64_t{1}
                            : selectionDepth + (widensIndex ? 2 : 1))
              : (widensIndex ? uint64_t{4} : uint64_t{3});
      result["expanded_nodes"] =
          (expression.kind == "array_get_dynamic" ? 5 * count - 2
                                                  : 4 * count + 1) +
          (widensIndex ? 2 : 0);
    }
    if (expression.kind == "masked_match") {
      result["mask"] = expression.mask;
      result["value"] = expression.value;
    }
    if (expression.kind == "table_match") {
      llvm::json::Array axes;
      llvm::json::Array shape;
      llvm::json::Array strides;
      for (uint64_t value : expression.domainAxes)
        axes.push_back(value);
      for (uint64_t value : expression.domainShape)
        shape.push_back(value);
      for (uint64_t value : expression.domainStrides)
        strides.push_back(value);
      result["domain_axes"] = std::move(axes);
      result["domain_offset"] = expression.domainOffset;
      result["domain_base"] = expression.domainBase;
      result["domain_shape"] = std::move(shape);
      result["domain_strides"] = std::move(strides);
      result["has_domain_projection"] = expression.hasDomainProjection;
      if (expression.hasDomainProjection) {
        uint64_t scanBound = 1;
        for (uint64_t extent : expression.domainShape)
          scanBound *= extent;
        result["scan_bound"] = scanBound;
      }
    }
    if (expression.kind == "table_choose_index" ||
        expression.kind == "table_choose_valid" ||
        expression.kind == "table_selection_index_ref" ||
        expression.kind == "table_selection_valid_ref" ||
        expression.kind == "helper_call") {
      result["lane_ordinal"] = expression.laneOrdinal;
      result["selection_count"] = expression.selectionCount;
      result["key_ordering"] = expression.keyOrdering;
      result["initial_cursor"] = expression.initialCursor;
    }
    if (!expression.sourceProvenance.origins.empty())
      result["source_provenance"] = provenanceJson(expression.sourceProvenance);
    return result;
  };
  auto initValueJson =
      [&](auto &&self, const TableInitValuePlan &value) -> llvm::json::Object {
    llvm::json::Array elements;
    for (const TableInitValuePlan &element : value.elements)
      elements.push_back(self(self, element));
    llvm::json::Array fieldNames;
    for (const std::string &field : value.fieldNames)
      fieldNames.push_back(field);
    return llvm::json::Object{{"elements", std::move(elements)},
                              {"field_names", std::move(fieldNames)},
                              {"kind", value.kind},
                              {"type", value.type},
                              {"value", value.value}};
  };
  llvm::json::Array payloadValues;
  for (const QueuePayloadPlan &payload : payloads) {
    llvm::json::Array fields;
    for (const QueuePayloadFieldPlan &field : payload.fields)
      fields.push_back(llvm::json::Object{
          {"name", field.name}, {"type", field.type}, {"width", field.width}});
    payloadValues.push_back(llvm::json::Object{{"fields", std::move(fields)},
                                               {"name", payload.name}});
  }
  llvm::json::Array enumValues;
  for (const QueueEnumPlan &enumeration : enums) {
    llvm::json::Array enumerants;
    for (const std::string &enumerant : enumeration.enumerants)
      enumerants.push_back(enumerant);
    llvm::json::Object value{{"enumerants", std::move(enumerants)},
                             {"name", enumeration.name},
                             {"width", enumeration.width}};
    if (!enumeration.values.empty()) {
      llvm::json::Array values;
      for (uint64_t encoded : enumeration.values)
        values.push_back(llvm::formatv("{0:x}", encoded).str());
      value["values"] = std::move(values);
      value["value_format"] = "unsigned_hex";
    }
    enumValues.push_back(std::move(value));
  }
  llvm::json::Array aggregateValues;
  for (const QueueAggregatePlan &aggregate : aggregates) {
    llvm::json::Array elements;
    for (const std::string &element : aggregate.elements)
      elements.push_back(element);
    aggregateValues.push_back(
        llvm::json::Object{{"elements", std::move(elements)},
                           {"kind", aggregate.kind},
                           {"length", aggregate.length},
                           {"type", aggregate.type},
                           {"width", aggregate.width}});
  }
  llvm::json::Array helperValues;
  for (const QueueHelperPlan &helper : helpers) {
    llvm::json::Array inputNames;
    llvm::json::Array inputTypes;
    llvm::json::Array resultTypes;
    llvm::json::Array expressions;
    llvm::json::Array yields;
    for (const std::string &name : helper.inputNames)
      inputNames.push_back(name);
    for (const std::string &type : helper.inputTypes)
      inputTypes.push_back(type);
    for (const std::string &type : helper.resultTypes)
      resultTypes.push_back(type);
    for (const QueueExpressionPlan &expression : helper.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    for (const std::string &yield : helper.yields)
      yields.push_back(yield);
    llvm::json::Object helperValue{{"expressions", std::move(expressions)},
                                   {"input_names", std::move(inputNames)},
                                   {"input_types", std::move(inputTypes)},
                                   {"name", helper.name},
                                   {"result_types", std::move(resultTypes)},
                                   {"yields", std::move(yields)}};
    if (!helper.sourceProvenance.origins.empty())
      helperValue["source_provenance"] =
          provenanceJson(helper.sourceProvenance);
    helperValues.push_back(std::move(helperValue));
  }
  llvm::json::Array scopeValues;
  for (const std::string &scope : scopes)
    scopeValues.push_back(scope);
  llvm::json::Array queueValues;
  for (const QueuePlan &queue : queues) {
    llvm::json::Array laneOrdinals;
    for (uint64_t lane : queue.laneOrdinals)
      laneOrdinals.push_back(lane);
    llvm::json::Object value{
        {"depth", queue.depth}, {"lane_ordinals", std::move(laneOrdinals)},
        {"lanes", queue.lanes}, {"latency", queue.latency},
        {"name", queue.name},   {"payload_type", queue.payloadType},
        {"rate", queue.rate},   {"scope", queue.scope}};
    if (queue.payloadProjection) {
      llvm::json::Array fields;
      for (const std::string &field : queue.payloadProjection->keptFields)
        fields.push_back(field);
      value["payload_projection"] = llvm::json::Object{
          {"carrier_bits", queue.payloadProjection->carrierBits},
          {"carrier_type", queue.payloadProjection->carrierType},
          {"kept_fields", std::move(fields)},
          {"logical_type", queue.payloadProjection->logicalType},
          {"logical_bits", queue.payloadProjection->logicalBits},
          {"profile", queue.payloadProjection->profile},
          {"removed_bits", queue.payloadProjection->removedBits},
          {"version", queue.payloadProjection->version}};
    }
    queueValues.push_back(std::move(value));
  }
  llvm::json::Array blockValues;
  for (const QueueBlockPlan &block : blocks) {
    auto resourceJson = [](const QueueRuleResourcePlan &resource) {
      return llvm::json::Object{{"kind", resource.kind},
                                {"ordinal", resource.ordinal},
                                {"resource", resource.resource}};
    };
    llvm::json::Array activationSources;
    for (const QueueRuleResourcePlan &resource : block.activationSources)
      activationSources.push_back(resourceJson(resource));
    llvm::json::Array transactionResources;
    for (const QueueRuleResourcePlan &resource : block.transactionResources)
      transactionResources.push_back(resourceJson(resource));
    llvm::json::Array arbitrationMembership;
    for (const QueueWriterArbitrationPlan &membership :
         block.arbitrationMembership)
      arbitrationMembership.push_back(llvm::json::Object{
          {"declared_rank", membership.declaredRank},
          {"endpoint_stable_id", membership.endpointStableId},
          {"owner", membership.owner},
          {"policy", membership.policy},
          {"resolution", membership.resolution}});
    llvm::json::Array inputs;
    for (const std::string &input : block.inputs)
      inputs.push_back(input);
    llvm::json::Array outputs;
    for (const std::string &output : block.outputs)
      outputs.push_back(output);
    llvm::json::Array depths;
    for (uint64_t depth : block.depths)
      depths.push_back(depth);
    llvm::json::Array latencies;
    for (uint64_t latency : block.latencies)
      latencies.push_back(latency);
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : block.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    llvm::json::Array yields;
    for (const std::string &yield : block.yields)
      yields.push_back(yield);
    llvm::json::Array writeFields;
    for (const std::string &field : block.writeFields)
      writeFields.push_back(field);
    llvm::json::Array stateWrites;
    for (const StateWritePlan &write : block.stateWrites) {
      llvm::json::Array fields;
      for (const std::string &field : write.fields)
        fields.push_back(field);
      llvm::json::Object writeValue{{"fields", std::move(fields)},
                                    {"index", write.index},
                                    {"mode", write.mode},
                                    {"present", write.present},
                                    {"table", write.table},
                                    {"value", write.value}};
      if (!write.versionedAction.empty()) {
        writeValue["ref_attempt"] = write.refAttempt;
        writeValue["ref_epoch"] = write.refEpoch;
        writeValue["ref_generation"] = write.refGeneration;
        writeValue["stale_obligation_id"] = write.staleObligationId;
        writeValue["versioned_action"] = write.versionedAction;
      }
      stateWrites.push_back(std::move(writeValue));
    }
    llvm::json::Array stateReservations;
    for (const StateReservationPlan &reservation : block.stateReservations) {
      llvm::json::Array fields;
      for (const std::string &field : reservation.fields)
        fields.push_back(field);
      stateReservations.push_back(
          llvm::json::Object{{"fields", std::move(fields)},
                             {"index", reservation.index},
                             {"index_kind", reservation.indexKind},
                             {"predicate", reservation.predicate},
                             {"source", reservation.source},
                             {"table", reservation.table}});
    }
    llvm::json::Array slotReleases;
    for (const SlotReleaseEffectPlan &release : block.slotReleases)
      slotReleases.push_back(
          llvm::json::Object{{"slot", release.slot}, {"when", release.when}});
    llvm::json::Array outputPresence;
    for (const OutputPresencePlan &output : block.outputPresence)
      outputPresence.push_back(llvm::json::Object{{"ordinal", output.ordinal},
                                                  {"present", output.present},
                                                  {"value", output.value}});
    llvm::json::Object blockValue{
        {"activation_sources", std::move(activationSources)},
        {"arbitration_membership", std::move(arbitrationMembership)},
        {"capacity", block.capacity},
        {"credits", block.credits},
        {"depths", std::move(depths)},
        {"display_rule_name", block.displayRuleName},
        {"entries", block.entries},
        {"expressions", std::move(expressions)},
        {"inputs", std::move(inputs)},
        {"kind", block.kind},
        {"latencies", std::move(latencies)},
        {"lexical_order", block.lexicalOrder},
        {"max_iterations", block.maxIterations},
        {"message", block.message},
        {"memory_instance", block.memoryInstance},
        {"write_mode", block.writeMode},
        {"table", block.table},
        {"table_index", block.tableIndex},
        {"table_value", block.tableValue},
        {"slot", block.slot},
        {"slot_releases", std::move(slotReleases)},
        {"name", block.name},
        {"no_dependency", block.noDependency},
        {"endpoint_ordinal", block.endpointOrdinal},
        {"outputs", std::move(outputs)},
        {"output_presence", std::move(outputPresence)},
        {"policy", block.policy},
        {"priority", block.priority},
        {"provider", block.provider},
        {"guard", block.guard},
        {"has_activation_evidence", block.hasActivationEvidence},
        {"initially_active", block.initiallyActive},
        {"region", block.region},
        {"result_field", block.resultField},
        {"resources", block.resources},
        {"scope", block.scope},
        {"selection", block.selection},
        {"selection_count", block.selectionCount},
        {"source_column", block.sourceColumn},
        {"source_file", block.sourceFile},
        {"source_line", block.sourceLine},
        {"start", block.start},
        {"state_reservations", std::move(stateReservations)},
        {"state_writes", std::move(stateWrites)},
        {"stable_id", block.stableId},
        {"transaction_resources", std::move(transactionResources)},
        {"init", block.init},
        {"write_fields", std::move(writeFields)},
        {"yields", std::move(yields)}};
    if (!block.ndfIds.empty()) {
      llvm::json::Array values;
      for (const std::string &identifier : block.ndfIds)
        values.push_back(identifier);
      blockValue["ndf_ids"] = std::move(values);
    }
    if (!block.ndfRequires.empty()) {
      llvm::json::Array values;
      for (const std::string &identifier : block.ndfRequires)
        values.push_back(identifier);
      blockValue["ndf_requires"] = std::move(values);
    }
    if (!block.sourceProvenance.origins.empty())
      blockValue["source_provenance"] = provenanceJson(block.sourceProvenance);
    blockValues.push_back(std::move(blockValue));
  }
  llvm::json::Array memoryInstanceValues;
  for (const MemoryInstancePlan &instance : memoryInstances) {
    llvm::json::Object value{
        {"data_type", instance.dataType}, {"entries", instance.entries},
        {"init", instance.init},          {"latency", instance.latency},
        {"name", instance.name},          {"owner_path", instance.ownerPath},
        {"stable_id", instance.stableId}};
    if (!instance.sourceProvenance.origins.empty())
      value["source_provenance"] = provenanceJson(instance.sourceProvenance);
    memoryInstanceValues.push_back(std::move(value));
  }
  llvm::json::Array memoryRequestValues;
  for (const MemoryRequestPlan &request : memoryRequests)
    memoryRequestValues.push_back(
        llvm::json::Object{{"depth", request.depth},
                           {"input", request.input},
                           {"instance", request.instance},
                           {"name", request.name},
                           {"ordinal", request.ordinal},
                           {"output", request.output},
                           {"result_field", request.resultField},
                           {"scope", request.scope}});
  llvm::json::Array tableValues;
  for (const TablePlan &table : tables) {
    llvm::json::Array shape;
    llvm::json::Array axisWidths;
    llvm::json::Array initImage;
    for (uint64_t value : table.shape)
      shape.push_back(value);
    for (uint64_t value : table.axisWidths)
      axisWidths.push_back(value);
    for (const TableInitValuePlan &value : table.initImage)
      initImage.push_back(initValueJson(initValueJson, value));
    llvm::json::Object value{{"axis_widths", std::move(axisWidths)},
                             {"entries", table.entries},
                             {"entry_type", table.entryType},
                             {"init", table.init},
                             {"init_image", std::move(initImage)},
                             {"init_version", table.initVersion},
                             {"has_typed_schema", table.hasTypedSchema},
                             {"layout", table.layout},
                             {"layout_version", table.layoutVersion},
                             {"name", table.name},
                             {"owner_path", table.ownerPath},
                             {"shape", std::move(shape)},
                             {"stable_id", table.stableId}};
    if (table.versioned) {
      value["attempt_bits"] = table.attemptBits;
      value["attempt_field"] = table.attemptField;
      value["checkpoint"] = table.checkpoint;
      value["epoch_bits"] = table.epochBits;
      value["epoch_field"] = table.epochField;
      value["generation_bits"] = table.generationBits;
      value["generation_field"] = table.generationField;
      value["identity"] = table.identity;
      value["payload_field"] = table.payloadField;
      value["recovery_domain"] = table.recoveryDomain;
      value["retained_result"] = table.retainedResult;
      value["valid_field"] = table.validField;
      value["versioned"] = true;
    }
    if (!table.sourceProvenance.origins.empty())
      value["source_provenance"] = provenanceJson(table.sourceProvenance);
    tableValues.push_back(std::move(value));
  }
  llvm::json::Array tableMatchValues;
  for (const TableMatchPlan &match : tableMatches) {
    llvm::json::Array expressions;
    llvm::json::Array domainAxes;
    llvm::json::Array domainShape;
    llvm::json::Array domainStrides;
    for (const QueueExpressionPlan &expression : match.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    for (uint64_t value : match.domainAxes)
      domainAxes.push_back(value);
    for (uint64_t value : match.domainShape)
      domainShape.push_back(value);
    for (uint64_t value : match.domainStrides)
      domainStrides.push_back(value);
    llvm::json::Object matchValue{
        {"domain_axes", std::move(domainAxes)},
        {"domain_offset", match.domainOffset},
        {"domain_shape", std::move(domainShape)},
        {"domain_strides", std::move(domainStrides)},
        {"has_domain_projection", match.hasDomainProjection},
        {"expressions", std::move(expressions)},
        {"name", match.name},
        {"result_type", match.resultType},
        {"scope", match.scope},
        {"table", match.table},
        {"yield", match.yield}};
    if (match.hasDomainProjection) {
      uint64_t scanBound = 1;
      for (uint64_t extent : match.domainShape)
        scanBound *= extent;
      matchValue["scan_bound"] = scanBound;
    }
    if (!match.sourceProvenance.origins.empty())
      matchValue["source_provenance"] = provenanceJson(match.sourceProvenance);
    tableMatchValues.push_back(std::move(matchValue));
  }
  llvm::json::Array tableSelectionValues;
  for (const TableSelectionPlan &selection : tableSelections) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : selection.keyExpressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    llvm::json::Object selectionValue{
        {"count", selection.count},
        {"index_type", selection.indexType},
        {"initial_cursor", selection.initialCursor},
        {"key_ordering", selection.keyOrdering},
        {"key_expressions", std::move(expressions)},
        {"key_yield", selection.keyYield},
        {"match", selection.match},
        {"name", selection.name},
        {"policy", selection.policy},
        {"scope", selection.scope},
        {"stable_id", selection.stableId},
        {"table", selection.table}};
    if (!selection.sourceProvenance.origins.empty())
      selectionValue["source_provenance"] =
          provenanceJson(selection.sourceProvenance);
    tableSelectionValues.push_back(std::move(selectionValue));
  }
  llvm::json::Array tableReadValues;
  for (const TableReadPlan &read : tableReads)
    tableReadValues.push_back(llvm::json::Object{{"depth", read.depth},
                                                 {"input", read.input},
                                                 {"latency", read.latency},
                                                 {"name", read.name},
                                                 {"output", read.output},
                                                 {"scope", read.scope},
                                                 {"table", read.table}});
  llvm::json::Array tableWriteValues;
  for (const TableWritePlan &write : tableWrites) {
    llvm::json::Array writeFields;
    for (const std::string &field : write.writeFields)
      writeFields.push_back(field);
    tableWriteValues.push_back(
        llvm::json::Object{{"input", write.input},
                           {"mode", write.mode},
                           {"name", write.name},
                           {"scope", write.scope},
                           {"table", write.table},
                           {"write_fields", std::move(writeFields)}});
  }
  llvm::json::Array tableMaskedWriteValues;
  for (const TableMaskedWritePlan &write : tableMaskedWrites) {
    llvm::json::Array writeFields;
    for (const std::string &field : write.writeFields)
      writeFields.push_back(field);
    tableMaskedWriteValues.push_back(
        llvm::json::Object{{"name", write.name},
                           {"mode", write.mode},
                           {"scope", write.scope},
                           {"table", write.table},
                           {"write_fields", std::move(writeFields)}});
  }
  llvm::json::Array slotValues;
  for (const SlotPlan &slot : slots) {
    llvm::json::Object value{
        {"input", slot.input},          {"name", slot.name},
        {"owner_path", slot.ownerPath}, {"payload_type", slot.payloadType},
        {"scope", slot.scope},          {"stable_id", slot.stableId}};
    if (!slot.sourceProvenance.origins.empty())
      value["source_provenance"] = provenanceJson(slot.sourceProvenance);
    slotValues.push_back(std::move(value));
  }
  llvm::json::Array interfaceInputValues;
  for (const QueueInterfacePlan &input : interfaceInputs)
    interfaceInputValues.push_back(
        llvm::json::Object{{"display_name", input.displayName},
                           {"lanes", input.lanes},
                           {"name", input.name},
                           {"payload_type", input.payloadType},
                           {"rate", input.rate}});
  llvm::json::Array interfaceOutputValues;
  for (const QueueInterfacePlan &output : interfaceOutputs)
    interfaceOutputValues.push_back(
        llvm::json::Object{{"display_name", output.displayName},
                           {"lanes", output.lanes},
                           {"name", output.name},
                           {"payload_type", output.payloadType},
                           {"rate", output.rate}});
  llvm::json::Array moduleInstanceValues;
  for (const QueueModuleInstancePlan &instance : moduleInstances) {
    llvm::json::Array inputs;
    for (const std::string &input : instance.inputs)
      inputs.push_back(input);
    llvm::json::Array outputs;
    for (const std::string &output : instance.outputs)
      outputs.push_back(output);
    llvm::json::Array staticArguments;
    for (auto argument :
         instance.staticArguments.getArguments()
             .getAsRange<acir::ac::StaticArgumentAttr>()) {
      staticArguments.push_back(llvm::json::Object{
          {"name", argument.getName().getValue().str()},
          {"value", renderStaticValueText(argument.getValue())}});
    }
    llvm::json::Object instanceValue{
        {"definition", instance.definition},
        {"inputs", std::move(inputs)},
        {"lexical_order", instance.lexicalOrder},
        {"name", instance.name},
        {"outputs", std::move(outputs)},
        {"scope", instance.scope},
        {"static_arguments", std::move(staticArguments)}};
    if (!instance.sourceProvenance.origins.empty())
      instanceValue["source_provenance"] =
          provenanceJson(instance.sourceProvenance);
    moduleInstanceValues.push_back(std::move(instanceValue));
  }
  auto activationNodeJson = [](const QueueActivationNodePlan &node) {
    return llvm::json::Object{{"index", node.index},
                              {"kind", activationKindName(node.kind)}};
  };
  llvm::json::Array activationEdgeValues;
  for (const QueueActivationEdgePlan &edge : activationEdges)
    activationEdgeValues.push_back(
        llvm::json::Object{{"source", activationNodeJson(edge.source)},
                           {"target", activationNodeJson(edge.target)}});
  llvm::json::Array workClosureEdgeValues;
  for (const QueueActivationEdgePlan &edge : workClosureEdges)
    workClosureEdgeValues.push_back(
        llvm::json::Object{{"source", activationNodeJson(edge.source)},
                           {"target", activationNodeJson(edge.target)}});
  llvm::json::Array initialActivationValues;
  for (const QueueActivationNodePlan &node : initialActivation)
    initialActivationValues.push_back(activationNodeJson(node));
  llvm::json::Array obligationValues;
  std::vector<const QueueArchitectureObligationPlan *> sortedObligations;
  for (const QueueArchitectureObligationPlan &obligation : architectureObligations)
    sortedObligations.push_back(&obligation);
  llvm::sort(sortedObligations, [](const auto *left, const auto *right) {
    return std::tie(left->module, left->id) < std::tie(right->module, right->id);
  });
  for (const QueueArchitectureObligationPlan *obligationPointer :
       sortedObligations) {
    const QueueArchitectureObligationPlan &obligation = *obligationPointer;
    llvm::json::Array targets;
    for (const std::string &target : obligation.targets)
      targets.push_back(target);
    llvm::json::Array sourceRules;
    for (const std::string &rule : obligation.sourceRules)
      sourceRules.push_back(rule);
    llvm::json::Array stateOwners;
    for (const std::string &owner : obligation.stateOwners)
      stateOwners.push_back(owner);
    llvm::json::Array ndfIds;
    for (const std::string &identifier : obligation.ndfIds)
      ndfIds.push_back(identifier);
    llvm::json::Array materializations;
    for (const std::string &materialization : obligation.materializations)
      materializations.push_back(materialization);
    obligationValues.push_back(
        llvm::json::Object{{"active_root", obligation.activeRoot
                                               ? llvm::json::Value(*obligation.activeRoot)
                                               : llvm::json::Value(nullptr)},
                           {"condition_root", obligation.conditionRoot},
                           {"condition_rule", obligation.conditionRule},
                           {"condition_table", obligation.conditionTable},
                           {"disable_root", obligation.disableRoot
                                                ? llvm::json::Value(*obligation.disableRoot)
                                                : llvm::json::Value(nullptr)},
                           {"firing", obligation.firing},
                           {"id", obligation.id},
                           {"input_ordinal", obligation.inputOrdinal},
                           {"kind", obligation.kind},
                           {"maximum", obligation.maximum},
                           {"message", obligation.message},
                           {"module", obligation.module},
                           {"symbol", obligation.symbol},
                           {"monitor_only", obligation.monitorOnly},
                           {"capture_latency", obligation.captureLatency
                                                   ? llvm::json::Value(*obligation.captureLatency)
                                                   : llvm::json::Value(nullptr)},
                           {"active_rule", obligation.activeRule},
                           {"disable_rule", obligation.disableRule},
                           {"ndf_ids", std::move(ndfIds)},
                           {"proof_certificate", obligation.proofCertificate},
                           {"materializations", std::move(materializations)},
                           {"sampling", obligation.sampling},
                           {"source_provenance",
                            provenanceJson(obligation.sourceProvenance)},
                           {"sample_anchor", obligation.sampleAnchor},
                           {"sampling_edge", obligation.samplingEdge},
                           {"sampling_kind", obligation.samplingKind},
                           {"severity", obligation.severity},
                           {"source_rules", std::move(sourceRules)},
                           {"status", obligation.status},
                           {"state_owners", std::move(stateOwners)},
                           {"targets", std::move(targets)}});
  }
  llvm::json::Array architectureExpressionValues;
  std::vector<const QueueArchitectureExpressionScopePlan *> sortedScopes;
  for (const auto &scope : architectureExpressionScopes)
    sortedScopes.push_back(&scope);
  llvm::sort(sortedScopes, [](const auto *left, const auto *right) {
    return left->rule < right->rule;
  });
  for (const auto *scopePointer : sortedScopes) {
    const auto &scope = *scopePointer;
    llvm::json::Array nodes;
    for (const auto &node : scope.nodes) {
      llvm::json::Array operands;
      for (uint64_t operand : node.operands)
        operands.push_back(operand);
      nodes.push_back(llvm::json::Object{{"input_ordinal", node.inputOrdinal},
                                         {"literal", node.literal},
                                         {"opcode", node.opcode},
                                         {"operands", std::move(operands)},
                                         {"operation", node.operation},
                                         {"predicate", node.predicate},
                                         {"attributes", node.attributes},
                                         {"type", node.type}});
    }
    architectureExpressionValues.push_back(
        llvm::json::Object{{"nodes", std::move(nodes)},
                           {"owner_rule", scope.ownerRule},
                           {"rule", scope.rule}});
  }
  llvm::json::Array provedElisionValues;
  std::vector<const QueueProvedObligationElisionPlan *> sortedElisions;
  for (const auto &elision : provedObligationElisions)
    sortedElisions.push_back(&elision);
  llvm::sort(sortedElisions, [](const auto *left, const auto *right) {
    return std::tie(left->module, left->id) <
           std::tie(right->module, right->id);
  });
  for (const auto *elision : sortedElisions)
    provedElisionValues.push_back(llvm::json::Object{
        {"id", elision->id},
        {"kind", elision->kind},
        {"left_endpoint", elision->leftEndpoint},
        {"module", elision->module},
        {"owner_path", elision->ownerPath},
        {"owner_stable_id", elision->ownerStableId},
        {"property_root", elision->propertyRoot},
        {"reason", elision->reason},
        {"right_endpoint", elision->rightEndpoint},
        {"source_provenance", elision->sourceProvenance}});
  llvm::json::Object root{
      {"activation_edges", std::move(activationEdgeValues)},
      {"aggregates", std::move(aggregateValues)},
      {"architecture_obligations", std::move(obligationValues)},
      {"architecture_expression_scopes",
       std::move(architectureExpressionValues)},
      {"proved_obligation_elisions", std::move(provedElisionValues)},
      {"blocks", std::move(blockValues)},
      {"definition", definition.empty() ? llvm::json::Value(nullptr)
                                        : llvm::json::Value(definition)},
      {"enums", std::move(enumValues)},
      {"interface_inputs", std::move(interfaceInputValues)},
      {"interface_outputs", std::move(interfaceOutputValues)},
      {"helpers", std::move(helperValues)},
      {"initial_activation", std::move(initialActivationValues)},
      {"memory_instances", std::move(memoryInstanceValues)},
      {"memory_requests", std::move(memoryRequestValues)},
      {"module_instances", std::move(moduleInstanceValues)},
      {"payloads", std::move(payloadValues)},
      {"queues", std::move(queueValues)},
      {"schema", "agentic-circuit-queue-graph-plan"},
      {"scopes", std::move(scopeValues)},
      {"slots", std::move(slotValues)},
      {"table_reads", std::move(tableReadValues)},
      {"table_matches", std::move(tableMatchValues)},
      {"table_masked_writes", std::move(tableMaskedWriteValues)},
      {"table_selections", std::move(tableSelectionValues)},
      {"table_writes", std::move(tableWriteValues)},
      {"tables", std::move(tableValues)},
      {"system", system},
      {"version", "0.5"}};
  if (!ndfIds.empty()) {
    llvm::json::Array values;
    for (const std::string &identifier : ndfIds)
      values.push_back(identifier);
    root["ndf_ids"] = std::move(values);
  }
  if (!ndfRequires.empty()) {
    llvm::json::Array values;
    for (const std::string &identifier : ndfRequires)
      values.push_back(identifier);
    root["ndf_requires"] = std::move(values);
  }
  if (!sourceFile.empty()) {
    root["source_file"] = sourceFile;
    root["source_line"] = sourceLine;
    root["source_column"] = sourceColumn;
  }
  root["work_closure_edges"] = std::move(workClosureEdgeValues);
  return bindings::canonicalizeJson(llvm::json::Value(std::move(root)));
}

llvm::Expected<std::string> QueueGraphPlan::sourceMapJson() const {
  auto provenanceJson = [](const QueueSourceProvenancePlan &provenance) {
    llvm::json::Array origins;
    for (const QueueSourceOriginPlan &origin : provenance.origins) {
      llvm::json::Array frames;
      for (const QueueSourceFramePlan &frame : origin) {
        llvm::json::Object value{{"column", frame.column},
                                 {"file", frame.file},
                                 {"kind", frame.kind},
                                 {"line", frame.line}};
        if (!frame.symbol.empty())
          value["symbol"] = frame.symbol;
        frames.push_back(std::move(value));
      }
      origins.push_back(llvm::json::Object{{"frames", std::move(frames)}});
    }
    return llvm::json::Object{{"origins", std::move(origins)}};
  };
  auto expressionJson =
      [&](auto &&self,
          const QueueExpressionPlan &expression) -> llvm::json::Object {
    llvm::json::Array nested;
    for (const QueueExpressionPlan &child : expression.nestedExpressions)
      nested.push_back(self(self, child));
    return llvm::json::Object{
        {"kind", expression.kind},
        {"nested", std::move(nested)},
        {"result", expression.result},
        {"source_provenance", provenanceJson(expression.sourceProvenance)},
    };
  };

  llvm::json::Array blockValues;
  for (auto [index, block] : llvm::enumerate(blocks)) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : block.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    blockValues.push_back(llvm::json::Object{
        {"expressions", std::move(expressions)},
        {"index", index},
        {"kind", block.kind},
        {"name", block.name},
        {"source_provenance", provenanceJson(block.sourceProvenance)},
        {"stable_id", block.stableId},
    });
  }
  llvm::json::Array helperValues;
  for (const QueueHelperPlan &helper : helpers) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : helper.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    helperValues.push_back(llvm::json::Object{
        {"expressions", std::move(expressions)},
        {"name", helper.name},
        {"source_provenance", provenanceJson(helper.sourceProvenance)},
    });
  }
  auto staticArgumentsJson =
      [](const acir::ac::StaticArgumentsAttr &arguments) {
        llvm::json::Array values;
        for (auto argument :
             arguments.getArguments()
                 .getAsRange<acir::ac::StaticArgumentAttr>()) {
          values.push_back(llvm::json::Object{
              {"name", argument.getName().getValue().str()},
              {"value", renderStaticValueText(argument.getValue())}});
        }
        return values;
      };
  llvm::json::Array instanceValues;
  for (const QueueModuleInstancePlan &instance : moduleInstances)
    instanceValues.push_back(llvm::json::Object{
        {"definition", instance.definition},
        {"name", instance.name},
        {"scope", instance.scope},
        {"source_provenance", provenanceJson(instance.sourceProvenance)},
        {"static_arguments", staticArgumentsJson(instance.staticArguments)},
    });
  llvm::json::Array tableMatchValues;
  for (const TableMatchPlan &match : tableMatches) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : match.expressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    tableMatchValues.push_back(llvm::json::Object{
        {"expressions", std::move(expressions)},
        {"name", match.name},
        {"source_provenance", provenanceJson(match.sourceProvenance)},
        {"table", match.table},
    });
  }
  llvm::json::Array tableSelectionValues;
  for (const TableSelectionPlan &selection : tableSelections) {
    llvm::json::Array expressions;
    for (const QueueExpressionPlan &expression : selection.keyExpressions)
      expressions.push_back(expressionJson(expressionJson, expression));
    tableSelectionValues.push_back(llvm::json::Object{
        {"key_expressions", std::move(expressions)},
        {"name", selection.name},
        {"source_provenance", provenanceJson(selection.sourceProvenance)},
        {"table", selection.table},
    });
  }
  llvm::json::Array stateOwnerValues;
  for (const MemoryInstancePlan &instance : memoryInstances)
    stateOwnerValues.push_back(llvm::json::Object{
        {"kind", "memory"},
        {"name", instance.name},
        {"source_provenance", provenanceJson(instance.sourceProvenance)},
    });
  for (const TablePlan &table : tables)
    stateOwnerValues.push_back(llvm::json::Object{
        {"kind", "table"},
        {"name", table.name},
        {"source_provenance", provenanceJson(table.sourceProvenance)},
    });
  for (const SlotPlan &slot : slots)
    stateOwnerValues.push_back(llvm::json::Object{
        {"kind", "slot"},
        {"name", slot.name},
        {"source_provenance", provenanceJson(slot.sourceProvenance)},
    });
  llvm::json::Object root{
      {"blocks", std::move(blockValues)},
      {"definition", definition.empty() ? llvm::json::Value(nullptr)
                                        : llvm::json::Value(definition)},
      {"helpers", std::move(helperValues)},
      {"module_instances", std::move(instanceValues)},
      {"schema", "agentic-circuit-source-map"},
      {"state_owners", std::move(stateOwnerValues)},
      {"system", system},
      {"table_matches", std::move(tableMatchValues)},
      {"table_selections", std::move(tableSelectionValues)},
      {"version", "0.1"},
  };
  return bindings::canonicalizeJson(llvm::json::Value(std::move(root)));
}

llvm::Expected<std::string> QueueGraphPlan::moduleManifestJson() const {
  auto argumentsJson = [](acir::ac::StaticArgumentsAttr arguments) {
    llvm::json::Array values;
    if (!arguments)
      return values;
    for (auto argument :
         arguments.getArguments()
             .getAsRange<acir::ac::StaticArgumentAttr>()) {
      values.push_back(llvm::json::Object{
          {"name", argument.getName().getValue().str()},
          {"value", renderStaticValueText(argument.getValue())}});
    }
    return values;
  };
  auto provenanceJson = [](const QueueSourceProvenancePlan &provenance) {
    llvm::json::Array origins;
    for (const QueueSourceOriginPlan &origin : provenance.origins) {
      llvm::json::Array frames;
      for (const QueueSourceFramePlan &frame : origin) {
        llvm::json::Object value{{"column", frame.column},
                                 {"file", frame.file},
                                 {"kind", frame.kind},
                                 {"line", frame.line}};
        if (!frame.symbol.empty())
          value["symbol"] = frame.symbol;
        frames.push_back(std::move(value));
      }
      origins.push_back(llvm::json::Object{{"frames", std::move(frames)}});
    }
    return llvm::json::Object{{"origins", std::move(origins)}};
  };
  auto portsJson = [](acir::ac::ModuleInterfaceAttr interface) {
    llvm::json::Array values;
    if (!interface)
      return values;
    for (auto port :
         interface.getPorts().getAsRange<acir::ac::InterfacePortAttr>()) {
      values.push_back(llvm::json::Object{
          {"direction", port.getDirection().getValue().str()},
          {"logical_type", renderStaticValueText(port.getLogicalType())},
          {"name", port.getName().getValue().str()}});
    }
    return values;
  };
  auto parametersJson = [](acir::ac::StaticParametersAttr parameters) {
    llvm::json::Array values;
    if (!parameters)
      return values;
    for (auto parameter :
         parameters.getParameters()
             .getAsRange<acir::ac::StaticParameterAttr>()) {
      values.push_back(llvm::json::Object{
          {"name", parameter.getName().getValue().str()},
          {"required", parameter.getRequired()},
          {"type", renderStaticValueText(parameter.getType())}});
    }
    return values;
  };

  llvm::json::Array moduleValues;
  for (const ModuleFamilyPlan &family : moduleFamilies) {
    llvm::json::Array declaredCases;
    if (family.declaredCases)
      for (auto arguments :
           family.declaredCases.getCases()
               .getAsRange<acir::ac::StaticArgumentsAttr>())
        declaredCases.push_back(llvm::json::Object{
            {"static_arguments", argumentsJson(arguments)}});
    llvm::json::Array caseValues;
    for (const ModuleCasePlan &moduleCase : family.cases) {
      std::string signature;
      llvm::raw_string_ostream stream(signature);
      if (moduleCase.concreteSignature)
        moduleCase.concreteSignature.print(stream);
      caseValues.push_back(llvm::json::Object{
          {"signature", stream.str()},
          {"static_arguments", argumentsJson(moduleCase.arguments)}});
    }
    moduleValues.push_back(llvm::json::Object{
        {"cases", std::move(caseValues)},
        {"declaration",
         family.source ? family.source.getDeclaration().getValue().str()
                       : std::string()},
        {"declared_cases", std::move(declaredCases)},
        {"interface", portsJson(family.interface)},
        {"owner",
         family.source ? family.source.getImplementation().getValue().str()
                       : std::string()},
        {"parameters", parametersJson(family.parameters)},
        {"symbol", family.definition}});
  }
  llvm::json::Array instanceValues;
  for (const QueueModuleInstancePlan &instance : moduleInstances)
    instanceValues.push_back(llvm::json::Object{
        {"definition", instance.definition},
        {"name", instance.name},
        {"scope", instance.scope},
        {"source_provenance", provenanceJson(instance.sourceProvenance)},
        {"static_arguments", argumentsJson(instance.staticArguments)}});
  llvm::json::Object root{
      {"definition", definition.empty() ? llvm::json::Value(nullptr)
                                        : llvm::json::Value(definition)},
      {"instances", std::move(instanceValues)},
      {"modules", std::move(moduleValues)},
      {"schema", "agentic-circuit-module-manifest"},
      {"system", system},
      {"version", "0.1"},
  };
  return bindings::canonicalizeJson(llvm::json::Value(std::move(root)));
}

} // namespace acir::codegen

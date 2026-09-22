#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/Bindings/Binding.h"
#include "acir/CodeGen/QueueBlockContract.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <system_error>
#include <tuple>
#include <utility>

namespace acir::codegen {
namespace {

llvm::Error generatorError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-QUEUE-CXX: " + message);
}

template <typename... Values>
void appendInitializer(std::vector<std::string> &initializers,
                       const Values &...values) {
  std::string initializer;
  llvm::raw_string_ostream output(initializer);
  (output << ... << values);
  output.flush();
  initializers.push_back(std::move(initializer));
}

std::string identifier(llvm::StringRef value) {
  std::string legalized = legalizeQueueGraphIdentifier(value);
  std::string result;
  bool underscore = false;
  for (char character : legalized) {
    if (character == '_') {
      if (underscore || result.empty())
        continue;
      underscore = true;
    } else {
      underscore = false;
    }
    result.push_back(character);
  }
  while (!result.empty() && result.back() == '_')
    result.pop_back();
  if (result.empty())
    return "ac_value";
  if (std::isdigit(static_cast<unsigned char>(result.front())))
    result.insert(0, "ac_");
  return result;
}

std::string uniqueIdentifier(llvm::StringRef preferred,
                             llvm::StringSet<> &used) {
  const std::string base = identifier(preferred);
  std::string result = base;
  for (uint64_t suffix = 2; !used.insert(result).second; ++suffix)
    result = base + "_" + std::to_string(suffix);
  return result;
}

llvm::StringMap<std::string>
interfaceParameterNames(const QueueGraphPlan &specialization,
                        uint64_t reservedObjectIds = 0) {
  llvm::StringSet<> used;
  used.insert("name");
  used.insert("parent");
  used.insert("epoch");
  used.insert("table_ref");
  used.insert("table_refs");
  for (uint64_t index = 0; index < reservedObjectIds; ++index)
    used.insert("object_" + std::to_string(index) + "_id");
  for (size_t index = 0; index < specialization.blocks.size(); ++index)
    used.insert("block_" + std::to_string(index) + "_id");
  for (size_t index = 0; index < specialization.tables.size(); ++index)
    used.insert("table_" + std::to_string(index) + "_id");

  llvm::StringMap<std::string> result;
  auto add = [&](const QueueInterfacePlan &port, llvm::StringRef fallback) {
    llvm::StringRef display = port.displayName.empty()
                                  ? llvm::StringRef(port.name)
                                  : llvm::StringRef(port.displayName);
    result[port.name] =
        uniqueIdentifier(display.empty() ? fallback : display, used);
  };
  for (auto [index, input] : llvm::enumerate(specialization.interfaceInputs))
    add(input, ("input_" + std::to_string(index)));
  for (auto [index, output] : llvm::enumerate(specialization.interfaceOutputs))
    add(output, ("output_" + std::to_string(index)));
  return result;
}

void emitNdfComments(std::ostringstream &output,
                     llvm::ArrayRef<std::string> ids,
                     llvm::ArrayRef<std::string> requiredIds) {
  auto emit = [&](llvm::StringRef label, llvm::ArrayRef<std::string> values) {
    if (values.empty())
      return;
    output << "// " << label.str() << ": ";
    for (auto [index, value] : llvm::enumerate(values)) {
      if (index)
        output << ", ";
      output << value;
    }
    output << "\n";
  };
  emit("ndf", ids);
  emit("ndf-requires", requiredIds);
}

void emitDefinitionProvenance(std::ostringstream &output,
                              const QueueGraphPlan &plan) {
  if (!plan.definition.empty())
    output << "// definition: " << plan.definition << "\n";
  emitNdfComments(output, plan.ndfIds, plan.ndfRequires);
  if (!plan.sourceFile.empty())
    output << "// source: " << plan.sourceFile << ':' << plan.sourceLine << ':'
           << plan.sourceColumn << "\n";
}

void emitRuleProvenance(std::ostringstream &output,
                        const QueueBlockPlan &block) {
  output << "// rule: "
         << (block.displayRuleName.empty() ? block.name : block.displayRuleName)
         << "; stable_id: " << block.stableId << "\n";
  emitNdfComments(output, block.ndfIds, block.ndfRequires);
  if (!block.sourceFile.empty())
    output << "// source: " << block.sourceFile << ':' << block.sourceLine
           << ':' << block.sourceColumn << "\n";
  for (auto [originIndex, origin] :
       llvm::enumerate(block.sourceProvenance.origins)) {
    output << "// source-origin[" << originIndex << "]: ";
    for (auto [frameIndex, frame] : llvm::enumerate(origin)) {
      if (frameIndex)
        output << " <- ";
      output << frame.kind << ' ' << frame.file << ':' << frame.line << ':'
             << frame.column;
      if (!frame.symbol.empty())
        output << " (" << frame.symbol << ')';
    }
    output << "\n";
  }
  output << "// nullopt means this rule performs no transition. A valid plan "
            "may have no state writes while consuming inputs or producing "
            "outputs; Queue backpressure, reservations, and atomic commit "
            "remain runtime-owned.\n";
  for (const StateWritePlan &write : block.stateWrites) {
    output << "// state write: " << write.table << " mode=" << write.mode
           << " fields=";
    for (auto [index, field] : llvm::enumerate(write.fields)) {
      if (index)
        output << ',';
      output << field;
    }
    output << " (local record edits do not imply field-level writes)\n";
  }
  for (const StateReservationPlan &reservation : block.stateReservations)
    output << "// reservation: " << reservation.table
           << " index_kind=" << reservation.indexKind
           << " (field masks name reserved owner fields)\n";
}

llvm::Error emitArchitectureObligationChecks(std::ostringstream &output,
                                             const QueueGraphPlan &plan,
                                             const QueueBlockPlan &firing,
                                             llvm::StringRef indent) {
  for (const QueueArchitectureObligationPlan &obligation :
       plan.architectureObligations) {
    if (obligation.status != "runtime_checked" ||
        obligation.firing != firing.stableId)
      continue;
    auto scope = llvm::find_if(
        plan.architectureExpressionScopes, [&](const auto &candidate) {
          return candidate.rule == obligation.conditionRule;
        });
    auto render = [&](auto &&self, const QueueArchitectureExpressionScopePlan &s,
                      uint64_t root) -> llvm::Expected<std::string> {
      const auto &node = s.nodes[root];
      if (node.opcode == "rule_input")
        return node.inputOrdinal == 0
                   ? std::string("item")
                   : "item" + std::to_string(node.inputOrdinal);
      if (node.opcode == "constant")
        return std::to_string(node.literal);
      std::vector<std::string> operands;
      for (uint64_t operand : node.operands) {
        auto rendered = self(self, s, operand);
        if (!rendered)
          return rendered.takeError();
        operands.push_back(std::move(*rendered));
      }
      if (node.operation == "ac.var.not" && operands.size() == 1)
        return "!(" + operands[0] + ")";
      if (node.operation == "ac.var.and" && operands.size() == 2)
        return "(" + operands[0] + " && " + operands[1] + ")";
      if (node.operation == "ac.var.cmp" && node.predicate == "ule" &&
          operands.size() == 2)
        return "(" + operands[0] + " <= " + operands[1] + ")";
      return generatorError("unsupported reachable architecture expression");
    };
    auto renderRef = [&](const std::string &rule,
                         std::optional<uint64_t> root,
                         llvm::StringRef fallback) -> llvm::Expected<std::string> {
      if (!root)
        return fallback.str();
      auto found = llvm::find_if(
          plan.architectureExpressionScopes,
          [&](const auto &candidate) { return candidate.rule == rule; });
      return render(render, *found, *root);
    };
    auto condition = render(render, *scope, obligation.conditionRoot);
    if (!condition)
      return condition.takeError();
    auto active =
        renderRef(obligation.activeRule, obligation.activeRoot, "true");
    auto disabled =
        renderRef(obligation.disableRule, obligation.disableRoot, "false");
    if (!active)
      return active.takeError();
    if (!disabled)
      return disabled.takeError();
    std::string source = "unknown";
    if (!obligation.sourceProvenance.origins.empty() &&
        !obligation.sourceProvenance.origins.front().empty()) {
      const auto &frame = obligation.sourceProvenance.origins.front().front();
      source = frame.file + ":" + std::to_string(frame.line) + ":" +
               std::to_string(frame.column);
    }
    output << indent.str() << "if ((" << *active << ") && !(" << *disabled
           << ") && !(" << *condition << "))\n"
           << indent.str()
           << "  throw gfsim::ArchitectureObligationViolation{\""
           << obligation.id << "\", \"" << obligation.severity << "\", \""
           << source << "\", \"" << obligation.module << "\"};\n";
  }
  return llvm::Error::success();
}

void emitExpressionSourceDirective(std::ostringstream &output,
                                   const QueueExpressionPlan &expression) {
  if (expression.sourceProvenance.origins.empty() ||
      expression.sourceProvenance.origins.front().empty())
    return;
  const QueueSourceFramePlan &frame =
      expression.sourceProvenance.origins.front().front();
  std::string escapedFile;
  for (char character : frame.file) {
    if (character == '\\' || character == '"')
      escapedFile.push_back('\\');
    escapedFile.push_back(character);
  }
  output << "#line " << frame.line << " \"" << escapedFile << "\"\n";
}

std::string className(llvm::StringRef value) {
  std::string result;
  bool capitalize = true;
  for (char character : value) {
    if (!std::isalnum(static_cast<unsigned char>(character))) {
      capitalize = true;
      continue;
    }
    result.push_back(capitalize ? static_cast<char>(std::toupper(
                                      static_cast<unsigned char>(character)))
                                : character);
    capitalize = false;
  }
  if (result.empty() ||
      std::isdigit(static_cast<unsigned char>(result.front())))
    result.insert(result.begin(), '_');
  return result;
}

std::string sourceStem(const QueueGraphPlan &plan) {
  llvm::StringRef source = plan.sourceFile;
  if (source.empty() || source.starts_with('<'))
    source = plan.sourceDefinition.empty() ? llvm::StringRef(plan.system)
                                           : llvm::StringRef(plan.sourceDefinition);
  if (source.contains('/'))
    source = source.rsplit('/').second;
  if (source.ends_with(".py") || source.ends_with(".ac"))
    source = source.drop_back(3);
  return identifier(source);
}

std::string cppStringLiteral(llvm::StringRef value) {
  std::string result = "\"";
  for (char character : value) {
    switch (character) {
    case '\\':
      result.append("\\\\");
      break;
    case '"':
      result.append("\\\"");
      break;
    case '\n':
      result.append("\\n");
      break;
    case '\r':
      result.append("\\r");
      break;
    case '\t':
      result.append("\\t");
      break;
    default:
      result.push_back(character);
      break;
    }
  }
  result.push_back('"');
  return result;
}

std::optional<llvm::StringRef> structTypeName(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.struct<@types::@";
  if (type.starts_with(prefix) && type.ends_with('>'))
    return type.drop_front(prefix.size()).drop_back();
  return std::nullopt;
}

std::optional<llvm::StringRef> enumTypeName(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.enum<@types::@";
  if (type.starts_with(prefix) && type.ends_with('>'))
    return type.drop_front(prefix.size()).drop_back();
  return std::nullopt;
}

std::optional<std::pair<uint64_t, uint64_t>> rangeBounds(llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.range<";
  if (!type.starts_with(prefix) || !type.ends_with('>'))
    return std::nullopt;
  auto [lower, upper] = type.drop_front(prefix.size()).drop_back().split(',');
  uint64_t lowerValue = 0;
  uint64_t upperValue = 0;
  if (lower.trim().getAsInteger(10, lowerValue) ||
      upper.trim().getAsInteger(10, upperValue) || lowerValue > upperValue)
    return std::nullopt;
  return std::pair{lowerValue, upperValue};
}

unsigned rangeStorageWidth(uint64_t upper) {
  return upper == std::numeric_limits<uint64_t>::max()
             ? 64
             : std::max(1u, llvm::Log2_64_Ceil(upper + 1));
}

std::optional<uint64_t> candidateMaskWords(llvm::StringRef type) {
  if (type.starts_with('i')) {
    unsigned width = 0;
    if (!type.drop_front().getAsInteger(10, width) && width > 0 && width <= 64)
      return 1;
    return std::nullopt;
  }
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

llvm::Expected<std::vector<const QueuePayloadPlan *>>
payloadEmissionOrder(const QueueGraphPlan &plan) {
  llvm::StringMap<const QueuePayloadPlan *> byName;
  for (const QueuePayloadPlan &payload : plan.payloads)
    if (!byName.try_emplace(payload.name, &payload).second)
      return generatorError("payload identities are duplicated");

  std::vector<const QueuePayloadPlan *> result;
  llvm::StringSet<> emitted;
  while (result.size() != plan.payloads.size()) {
    const size_t before = result.size();
    for (const QueuePayloadPlan &payload : plan.payloads) {
      if (emitted.contains(payload.name))
        continue;
      bool blocked = false;
      for (const QueuePayloadFieldPlan &field : payload.fields) {
        std::optional<llvm::StringRef> dependency = structTypeName(field.type);
        if (!dependency)
          continue;
        if (!byName.contains(*dependency))
          return generatorError("nested payload type is unresolved");
        blocked |= !emitted.contains(*dependency);
      }
      if (blocked)
        continue;
      emitted.insert(payload.name);
      result.push_back(&payload);
    }
    if (result.size() == before)
      return generatorError("nested payload definitions contain a cycle");
  }
  return result;
}

llvm::StringRef enumStorage(uint64_t width) {
  if (width <= 8)
    return "std::uint8_t";
  if (width <= 16)
    return "std::uint16_t";
  if (width <= 32)
    return "std::uint32_t";
  return "std::uint64_t";
}

llvm::Expected<std::string> cppType(llvm::StringRef type) {
  if (type.starts_with('i')) {
    unsigned width = 0;
    if (!type.drop_front().getAsInteger(10, width) && width > 0) {
      if (width <= 64)
        return "gfsim::UInt<" + std::to_string(width) + ">";
    }
  }
  if (auto bounds = rangeBounds(type))
    return "gfsim::UInt<" + std::to_string(rangeStorageWidth(bounds->second)) +
           ">";
  if (std::optional<llvm::StringRef> name = structTypeName(type))
    return name->str();
  if (std::optional<llvm::StringRef> name = enumTypeName(type))
    return name->str();
  return generatorError("no C++ storage realization for ACIR type '" + type +
                        "'");
}

llvm::Expected<std::string>
cppPayloadFieldType(const QueueGraphPlan &plan,
                    const QueuePayloadFieldPlan &field) {
  auto aggregate =
      llvm::find_if(plan.aggregates, [&](const QueueAggregatePlan &candidate) {
        return candidate.type == field.type;
      });
  if (aggregate != plan.aggregates.end()) {
    if (field.width == 0 || field.width != aggregate->width)
      return generatorError("aggregate payload field width is unsupported");
    return "gfsim::UInt<" + std::to_string(field.width) + ">";
  }
  return cppType(field.type);
}

const QueuePayloadPlan *findPayloadType(const QueueGraphPlan &plan,
                                        llvm::StringRef type) {
  std::optional<llvm::StringRef> name = structTypeName(type);
  if (!name)
    return nullptr;
  auto found =
      llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &payload) {
        return payload.name == *name;
      });
  return found == plan.payloads.end() ? nullptr : &*found;
}

const QueueEnumPlan *findEnumType(const QueueGraphPlan &plan,
                                  llvm::StringRef type) {
  std::optional<llvm::StringRef> name = enumTypeName(type);
  if (!name)
    return nullptr;
  auto found = llvm::find_if(plan.enums, [&](const QueueEnumPlan &enumeration) {
    return enumeration.name == *name;
  });
  return found == plan.enums.end() ? nullptr : &*found;
}

const QueueAggregatePlan *findAggregateType(const QueueGraphPlan &plan,
                                            llvm::StringRef type) {
  auto found =
      llvm::find_if(plan.aggregates, [&](const QueueAggregatePlan &aggregate) {
        return aggregate.type == type;
      });
  return found == plan.aggregates.end() ? nullptr : &*found;
}

llvm::Expected<std::string> cppValueType(const QueueGraphPlan &plan,
                                         llvm::StringRef type) {
  if (const QueueAggregatePlan *aggregate = findAggregateType(plan, type))
    return "gfsim::UInt<" + std::to_string(aggregate->width) + ">";
  return cppType(type);
}

llvm::Expected<uint64_t> generatedTypeWidth(const QueueGraphPlan &plan,
                                            llvm::StringRef type);

llvm::Expected<bool> usesSharedQueueStorage(const QueueGraphPlan &plan,
                                            llvm::StringRef type) {
  if (!findPayloadType(plan, type))
    return false;
  auto width = generatedTypeWidth(plan, type);
  if (!width)
    return width.takeError();
  return *width > 64;
}

llvm::Expected<std::string> cppQueueStorageType(const QueueGraphPlan &plan,
                                                llvm::StringRef type) {
  auto valueType = cppValueType(plan, type);
  if (!valueType)
    return valueType.takeError();
  auto shared = usesSharedQueueStorage(plan, type);
  if (!shared)
    return shared.takeError();
  return *shared ? "std::shared_ptr<const " + *valueType + ">"
                 : std::move(*valueType);
}

llvm::Expected<std::string> cppQueueType(const QueueGraphPlan &plan,
                                         const QueuePlan &queue) {
  if (!queue.payloadProjection)
    return plan.definition.empty() ? cppType(queue.payloadType)
                                   : cppQueueStorageType(plan, queue.payloadType);
  if (queue.payloadProjection->profile != "private_transform_tuple_v1" ||
      queue.payloadProjection->carrierType != queue.payloadType)
    return generatorError(
        "private Queue payload projection is malformed for '" + queue.name +
        "' (profile='" + queue.payloadProjection->profile + "', carrier='" +
        queue.payloadProjection->carrierType + "', payload='" +
        queue.payloadType + "')");
  const QueueAggregatePlan *aggregate =
      findAggregateType(plan, queue.payloadType);
  if (!aggregate || aggregate->width == 0)
    return generatorError(
        "private Queue payload projection has no aggregate storage layout");
  return "gfsim::UInt<" + std::to_string(aggregate->width) + ">";
}

llvm::Expected<std::string> cppQueueType(const QueueGraphPlan &plan,
                                         const QueueInterfacePlan &interface) {
  return cppQueueStorageType(plan, interface.payloadType);
}

const QueueGraphPlan *findFamilyCaseBody(
    const QueueGraphPlan &plan, llvm::StringRef definition,
    acir::ac::StaticArgumentsAttr arguments) {
  for (const ModuleFamilyPlan &family : plan.moduleFamilies) {
    if (family.definition != definition)
      continue;
    for (const ModuleCasePlan &moduleCase : family.cases)
      if (moduleCase.arguments == arguments)
        return moduleCase.bodyPlan.get();
  }
  return nullptr;
}

std::vector<const QueueGraphPlan *>
orderedFamilyCaseBodies(const QueueGraphPlan &plan) {
  std::vector<const QueueGraphPlan *> result;
  llvm::DenseSet<const QueueGraphPlan *> visited;
  std::function<void(const QueueGraphPlan *)> visit =
      [&](const QueueGraphPlan *body) {
        if (!body || !visited.insert(body).second)
          return;
        for (const QueueModuleInstancePlan &instance : body->moduleInstances)
          visit(findFamilyCaseBody(plan, instance.definition,
                                   instance.staticArguments));
        result.push_back(body);
      };
  for (const ModuleFamilyPlan &family : plan.moduleFamilies)
    for (const ModuleCasePlan &moduleCase : family.cases)
      visit(moduleCase.bodyPlan.get());
  return result;
}

const QueueHelperPlan *findHelper(const QueueGraphPlan &plan,
                                  llvm::StringRef name) {
  auto found = llvm::find_if(plan.helpers, [&](const QueueHelperPlan &helper) {
    return helper.name == name;
  });
  return found == plan.helpers.end() ? nullptr : &*found;
}

bool isAggregateValueType(const QueueGraphPlan &plan, llvm::StringRef type) {
  return findPayloadType(plan, type) || findAggregateType(plan, type);
}

const TablePlan *findTable(const QueueGraphPlan &plan, llvm::StringRef name);
const QueuePlan *findQueue(const QueueGraphPlan &plan, llvm::StringRef name);

llvm::Expected<uint64_t> generatedTypeWidth(const QueueGraphPlan &plan,
                                            llvm::StringRef type) {
  if (type.starts_with('i')) {
    uint64_t width = 0;
    if (!type.drop_front().getAsInteger(10, width) && width > 0 && width <= 64)
      return width;
  }
  if (auto bounds = rangeBounds(type))
    return rangeStorageWidth(bounds->second);
  if (const QueueEnumPlan *enumeration = findEnumType(plan, type)) {
    if (enumeration->width > kMaximumPackedValueWidth)
      return generatorError("enum width exceeds the backend template domain");
    return enumeration->width;
  }
  if (const QueueAggregatePlan *aggregate = findAggregateType(plan, type)) {
    if (aggregate->width > kMaximumPackedValueWidth)
      return generatorError(
          "aggregate width exceeds the backend template domain");
    return aggregate->width;
  }
  if (const QueuePayloadPlan *payload = findPayloadType(plan, type)) {
    uint64_t width = 0;
    for (const QueuePayloadFieldPlan &field : payload->fields) {
      auto fieldWidth = generatedTypeWidth(plan, field.type);
      if (!fieldWidth)
        return fieldWidth.takeError();
      if (field.width != 0 && field.width != *fieldWidth)
        return generatorError("payload field width disagrees with its type");
      if (*fieldWidth > kMaximumPackedValueWidth - width)
        return generatorError(
            "payload width exceeds the backend template domain");
      width += *fieldWidth;
    }
    if (width > 0)
      return width;
  }
  return generatorError("no packed width for ACIR type '" + type + "'");
}

llvm::Expected<std::string> emitPackedValueImpl(const QueueGraphPlan &plan,
                                                llvm::StringRef type,
                                                llvm::StringRef value,
                                                llvm::StringSet<> &active) {
  if (type.starts_with('i') || rangeBounds(type) ||
      findAggregateType(plan, type))
    return value.str();
  if (const QueueEnumPlan *enumeration = findEnumType(plan, type))
    return "gfsim::UInt<" + std::to_string(enumeration->width) +
           ">{static_cast<std::uint64_t>(" + value.str() + ")}";
  const QueuePayloadPlan *payload = findPayloadType(plan, type);
  if (!payload)
    return generatorError("cannot pack ACIR type '" + type + "'");
  if (!active.insert(payload->name).second)
    return generatorError("cannot pack recursive struct type '" + type + "'");
  std::vector<std::string> fields;
  fields.reserve(payload->fields.size());
  for (const QueuePayloadFieldPlan &field : payload->fields) {
    auto packed = emitPackedValueImpl(
        plan, field.type, "(" + value.str() + ")." + identifier(field.name),
        active);
    if (!packed) {
      active.erase(payload->name);
      return packed.takeError();
    }
    fields.push_back(std::move(*packed));
  }
  active.erase(payload->name);
  if (fields.empty())
    return generatorError("cannot pack an empty struct type '" + type + "'");
  if (fields.size() == 1)
    return fields.front();
  std::string result = "gfsim::bitConcat(";
  for (auto [index, field] : llvm::enumerate(fields)) {
    if (index)
      result.append(", ");
    result.append(field);
  }
  result.push_back(')');
  return result;
}

llvm::Expected<std::string> emitPackedValue(const QueueGraphPlan &plan,
                                            llvm::StringRef type,
                                            llvm::StringRef value) {
  llvm::StringSet<> active;
  return emitPackedValueImpl(plan, type, value, active);
}

llvm::Expected<std::string> emitUnpackedValueImpl(const QueueGraphPlan &plan,
                                                  llvm::StringRef type,
                                                  llvm::StringRef value,
                                                  llvm::StringSet<> &active) {
  if (type.starts_with('i') || rangeBounds(type) ||
      findAggregateType(plan, type))
    return value.str();
  if (const QueueEnumPlan *enumeration = findEnumType(plan, type))
    return "static_cast<" + enumeration->name +
           ">(static_cast<std::uint64_t>((" + value.str() + ").value()))";
  const QueuePayloadPlan *payload = findPayloadType(plan, type);
  if (!payload)
    return generatorError("cannot unpack ACIR type '" + type + "'");
  if (!active.insert(payload->name).second)
    return generatorError("cannot unpack recursive struct type '" + type + "'");
  auto totalWidth = generatedTypeWidth(plan, type);
  if (!totalWidth) {
    active.erase(payload->name);
    return totalWidth.takeError();
  }
  uint64_t cursor = *totalWidth;
  std::string result = "[&]() { " + payload->name + " unpacked{}; ";
  for (const QueuePayloadFieldPlan &field : payload->fields) {
    auto fieldWidth = generatedTypeWidth(plan, field.type);
    if (!fieldWidth) {
      active.erase(payload->name);
      return fieldWidth.takeError();
    }
    cursor -= *fieldWidth;
    const std::string extracted =
        "gfsim::bitExtract<" + std::to_string(*fieldWidth) + ">(" +
        value.str() + ", " + std::to_string(cursor) + ")";
    auto unpacked = emitUnpackedValueImpl(plan, field.type, extracted, active);
    if (!unpacked) {
      active.erase(payload->name);
      return unpacked.takeError();
    }
    result.append("unpacked.")
        .append(identifier(field.name))
        .append(" = ")
        .append(*unpacked)
        .append("; ");
  }
  active.erase(payload->name);
  result.append("return unpacked; }()");
  return result;
}

llvm::Expected<std::string> emitUnpackedValue(const QueueGraphPlan &plan,
                                              llvm::StringRef type,
                                              llvm::StringRef value) {
  llvm::StringSet<> active;
  return emitUnpackedValueImpl(plan, type, value, active);
}

llvm::Expected<std::string>
emitTableInitValue(const QueueGraphPlan &plan,
                   const TableInitValuePlan &value) {
  if (value.kind == "integer") {
    auto type = cppType(value.type);
    if (!type)
      return type.takeError();
    return *type + "{" + value.value + "}";
  }
  if (value.kind == "enum") {
    auto name = enumTypeName(value.type);
    if (!name || value.value.empty())
      return generatorError("typed Table enum initializer is malformed");
    return name->str() + "::" + identifier(value.value);
  }
  if (value.kind == "struct") {
    const QueuePayloadPlan *payload = findPayloadType(plan, value.type);
    if (!payload || payload->fields.size() != value.elements.size() ||
        value.fieldNames.size() != value.elements.size())
      return generatorError("typed Table struct initializer is malformed");
    std::string result = payload->name + "{";
    for (auto [index, element] : llvm::enumerate(value.elements)) {
      if (payload->fields[index].name != value.fieldNames[index])
        return generatorError(
            "typed Table struct initializer field order is inconsistent");
      auto emitted = emitTableInitValue(plan, element);
      if (!emitted)
        return emitted.takeError();
      if (index)
        result.append(", ");
      result.append(*emitted);
    }
    result.push_back('}');
    return result;
  }
  if (value.kind == "tuple" || value.kind == "array") {
    const QueueAggregatePlan *aggregate = findAggregateType(plan, value.type);
    if (!aggregate || value.elements.empty())
      return generatorError("typed Table aggregate initializer is malformed");
    std::vector<std::string> packed;
    packed.reserve(value.elements.size());
    for (const TableInitValuePlan &element : value.elements) {
      auto emitted = emitTableInitValue(plan, element);
      if (!emitted)
        return emitted.takeError();
      auto encoded = emitPackedValue(plan, element.type, *emitted);
      if (!encoded)
        return encoded.takeError();
      packed.push_back(std::move(*encoded));
    }
    if (packed.size() == 1)
      return packed.front();
    std::string result = "gfsim::bitConcat(";
    for (auto [index, element] : llvm::enumerate(packed)) {
      if (index)
        result.append(", ");
      result.append(element);
    }
    result.push_back(')');
    return result;
  }
  return generatorError("typed Table initializer kind is unsupported");
}

llvm::Expected<std::string> tableStorageArgument(const QueueGraphPlan &plan,
                                                 const TablePlan &table) {
  auto type = cppType(table.entryType);
  if (!type)
    return type.takeError();
  if (!table.initImage.empty()) {
    std::string result = "std::vector<" + *type + ">{";
    for (auto [index, value] : llvm::enumerate(table.initImage)) {
      auto emitted = emitTableInitValue(plan, value);
      if (!emitted)
        return emitted.takeError();
      if (index)
        result.append(", ");
      result.append(*emitted);
    }
    result.push_back('}');
    return result;
  }
  if (table.init == 0)
    return std::to_string(table.entries);
  return "std::vector<" + *type + ">(" + std::to_string(table.entries) + ", " +
         *type + "{std::uint64_t{" + std::to_string(table.init) + "ULL}})";
}

template <typename Domain>
std::string tableProjectionArgument(const TablePlan &table,
                                    const Domain &match) {
  if (!match.hasDomainProjection)
    return "gfsim::TableDomainProjection(" + std::to_string(table.entries) +
           ")";
  auto array = [](llvm::ArrayRef<uint64_t> values) {
    std::string result =
        "std::array<std::size_t, " + std::to_string(values.size()) + ">{";
    for (auto [index, value] : llvm::enumerate(values)) {
      if (index)
        result.append(", ");
      result.append(std::to_string(value));
    }
    result.push_back('}');
    return result;
  };
  if (!match.domainBase.empty()) {
    uint64_t domainEntries = 1;
    for (uint64_t extent : match.domainShape)
      domainEntries *= extent;
    return "gfsim::TableDomainProjection(" + std::to_string(table.entries) +
           ", " + std::to_string(domainEntries) +
           ", static_cast<std::size_t>(" + match.domainBase + "))";
  }
  return "gfsim::TableDomainProjection(" + std::to_string(table.entries) +
         ", " + array(match.domainShape) + ", " + array(match.domainStrides) +
         ", " + std::to_string(match.domainOffset) + ")";
}

std::vector<std::string> pathParts(llvm::StringRef path) {
  std::vector<std::string> result;
  while (!path.empty()) {
    path = path.ltrim('/');
    if (path.empty())
      break;
    auto split = path.split('/');
    result.push_back(split.first.str());
    path = split.second;
  }
  return result;
}

std::string commonPath(llvm::StringRef left, llvm::StringRef right) {
  std::vector<std::string> lhs = pathParts(left);
  std::vector<std::string> rhs = pathParts(right);
  std::string result;
  for (size_t index = 0; index < std::min(lhs.size(), rhs.size()); ++index) {
    if (lhs[index] != rhs[index])
      break;
    result.push_back('/');
    result.append(lhs[index]);
  }
  return result.empty() ? "/" : result;
}

std::string matchExpressionValueKey(const QueueExpressionPlan &expression) {
  std::string result;
  auto append = [&](llvm::StringRef value) {
    result.append(std::to_string(value.size())).append(":").append(value.str());
  };
  append(expression.kind);
  append(expression.type);
  append(std::to_string(expression.operands.size()));
  for (const std::string &operand : expression.operands)
    append(operand);
  append(expression.field);
  append(expression.predicate);
  append(expression.literal);
  append(expression.table);
  append(expression.slot);
  append(std::to_string(expression.lsb));
  append(std::to_string(expression.width));
  append(expression.mask);
  append(expression.value);
  for (uint64_t value : expression.domainAxes)
    append(std::to_string(value));
  for (uint64_t value : expression.domainShape)
    append(std::to_string(value));
  for (uint64_t value : expression.domainStrides)
    append(std::to_string(value));
  append(std::to_string(expression.domainOffset));
  append(expression.domainBase);
  append(expression.hasDomainProjection ? "1" : "0");
  return result;
}

llvm::Expected<std::string>
emitExpressionBody(const QueueGraphPlan &plan, const QueueBlockPlan &block,
                   llvm::StringRef yield, unsigned indent,
                   bool qualifyTables = false, bool checkedTableAccess = true,
                   llvm::ArrayRef<std::string> additionalNeeded = {},
                   llvm::StringRef returnExpression = {}) {
  std::ostringstream output;
  std::string padding(indent, ' ');
  llvm::StringMap<std::string> priorityEncodings;
  llvm::StringMap<std::string> helperCallValues;
  llvm::StringMap<std::pair<std::string, std::string>> tableChoices;
  llvm::StringMap<std::string> rangeCheckedValues;
  llvm::StringMap<std::vector<std::string>> priorChoiceIndices;
  llvm::StringMap<std::pair<unsigned, unsigned>> tableChoicePairCounts;
  llvm::StringMap<unsigned> tableChoicePairOrdinals;
  llvm::StringSet<> sharedValues;
  for (auto [index, inputName] : llvm::enumerate(block.inputs)) {
    if (plan.definition.empty())
      break;
    const QueuePlan *input = findQueue(plan, inputName);
    if (!input)
      continue;
    auto shared = usesSharedQueueStorage(plan, input->payloadType);
    if (!shared)
      return shared.takeError();
    if (*shared)
      sharedValues.insert(index == 0 ? "item" : "item" + std::to_string(index));
  }
  for (const QueueExpressionPlan &expression : block.expressions) {
    if (expression.kind != "table_choose_index" &&
        expression.kind != "table_choose_valid")
      continue;
    const std::string contract = inlineTableChoiceContractKey(expression);
    auto &counts = tableChoicePairCounts[contract];
    unsigned &ordinal =
        expression.kind == "table_choose_index" ? counts.first : counts.second;
    tableChoicePairOrdinals[expression.result] = ordinal++;
  }
  llvm::StringSet<> needed;
  needed.insert(yield);
  for (const std::string &name : additionalNeeded)
    needed.insert(name);
  for (const QueueExpressionPlan &expression : llvm::reverse(block.expressions))
    if (needed.contains(expression.result)) {
      for (const std::string &operand : expression.operands)
        needed.insert(operand);
      if (expression.kind == "snapshot_set")
        needed.insert(expression.field);
    }
  for (const QueueExpressionPlan &expression : block.expressions) {
    if (!needed.contains(expression.result) ||
        (expression.kind != "table_choose_index" &&
         expression.kind != "table_choose_valid"))
      continue;
    const std::string contract = inlineTableChoiceContractKey(expression);
    for (const QueueExpressionPlan &candidate : block.expressions)
      if (candidate.kind == "table_choose_index" &&
          candidate.laneOrdinal <= expression.laneOrdinal &&
          inlineTableChoiceContractKey(candidate) == contract)
        needed.insert(candidate.result);
  }
  llvm::StringMap<size_t> expressionPositions;
  for (auto [index, expression] : llvm::enumerate(block.expressions))
    expressionPositions[expression.result] = index;
  llvm::StringSet<> emittedTableMatches;
  for (auto [expressionIndex, expression] :
       llvm::enumerate(block.expressions)) {
    if (!needed.contains(expression.result))
      continue;
    emitExpressionSourceDirective(output, expression);
    auto operand = [&](size_t index) -> llvm::Expected<llvm::StringRef> {
      if (index >= expression.operands.size())
        return generatorError("expression operand arity mismatch");
      return llvm::StringRef(expression.operands[index]);
    };
    if (expression.kind == "helper_call") {
      const QueueHelperPlan *helper = findHelper(plan, expression.field);
      if (!helper || expression.literal.empty() ||
          expression.selectionCount != helper->resultTypes.size() ||
          expression.laneOrdinal >= helper->resultTypes.size())
        return generatorError("helper_call expression is malformed");
      auto emitted = helperCallValues.find(expression.literal);
      if (emitted == helperCallValues.end()) {
        const std::string callValue = identifier(expression.literal);
        output << padding << "auto " << callValue << " = helper_"
               << identifier(helper->name) << '(';
        for (auto [index, argument] : llvm::enumerate(expression.operands)) {
          if (index)
            output << ", ";
          output << argument;
        }
        output << ");\n";
        helperCallValues[expression.literal] = callValue;
        emitted = helperCallValues.find(expression.literal);
      }
      if (helper->resultTypes.size() == 1)
        output << padding << "auto " << expression.result << " = "
               << emitted->getValue() << ";\n";
      else
        output << padding << "auto " << expression.result << " = std::get<"
               << expression.laneOrdinal << ">(" << emitted->getValue()
               << ");\n";
      continue;
    }
    if (expression.kind == "enum_constant") {
      std::optional<llvm::StringRef> type = enumTypeName(expression.type);
      if (!type || expression.field.empty())
        return generatorError("enum constant expression is malformed");
      output << padding << "auto " << expression.result << " = " << type->str()
             << "::" << expression.field << ";\n";
      continue;
    }
    if (expression.kind == "constant") {
      llvm::StringRef literal = expression.literal;
      auto type = cppType(expression.type);
      if (!type)
        return type.takeError();
      output << padding << "auto " << expression.result << " = " << *type << "{"
             << literal.split(" : ").first.str() << "};\n";
      continue;
    }
    if (expression.kind == "range_wrap" ||
        expression.kind == "range_saturate") {
      auto input = operand(0);
      auto bounds = rangeBounds(expression.type);
      if (!input)
        return input.takeError();
      if (!bounds)
        return generatorError("range conversion target is malformed");
      const unsigned width = rangeStorageWidth(bounds->second);
      output << padding << "auto " << expression.result << " = gfsim::range"
             << (expression.kind == "range_wrap" ? "Wrap" : "Saturate") << '<'
             << width << ", " << bounds->first << "ULL, " << bounds->second
             << "ULL>(" << input->str() << ");\n";
      continue;
    }
    if (expression.kind == "range_checked_value" ||
        expression.kind == "range_checked_valid") {
      auto input = operand(0);
      auto bounds = rangeBounds(expression.field);
      if (!input)
        return input.takeError();
      if (!bounds || expression.literal.empty())
        return generatorError("checked range conversion is malformed");
      const unsigned width = rangeStorageWidth(bounds->second);
      auto [entry, inserted] = rangeCheckedValues.try_emplace(
          expression.literal, "checked_" + identifier(expression.literal));
      if (inserted)
        output << padding << "auto " << entry->getValue()
               << " = gfsim::rangeChecked<" << width << ", " << bounds->first
               << "ULL, " << bounds->second << "ULL>(" << input->str()
               << ");\n";
      output << padding << "auto " << expression.result << " = "
             << entry->getValue() << '.'
             << (expression.kind == "range_checked_value" ? "value" : "valid")
             << ";\n";
      continue;
    }
    if (expression.kind == "range_refine") {
      auto input = operand(0);
      auto type = cppType(expression.type);
      if (!input)
        return input.takeError();
      if (!type)
        return type.takeError();
      output << padding << "auto " << expression.result << " = " << *type << "{"
             << input->str() << ".value()};\n";
      continue;
    }
    if (expression.kind == "range_bits") {
      auto input = operand(0);
      if (!input)
        return input.takeError();
      auto type = cppType(expression.type);
      if (!type)
        return type.takeError();
      output << padding << "auto " << expression.result << " = " << *type << "{"
             << input->str() << ".value()};\n";
      continue;
    }
    if (expression.kind == "range_add" || expression.kind == "range_sub") {
      auto left = operand(0);
      auto right = operand(1);
      if (!left)
        return left.takeError();
      if (!right)
        return right.takeError();
      auto type = cppType(expression.type);
      if (!type)
        return type.takeError();
      output << padding << "auto " << expression.result << " = " << *type << "{"
             << left->str() << ".value() "
             << (expression.kind == "range_add" ? '+' : '-') << ' '
             << right->str() << ".value()};\n";
      continue;
    }
    if (expression.kind == "range_cmp") {
      auto left = operand(0);
      auto right = operand(1);
      if (!left)
        return left.takeError();
      if (!right)
        return right.takeError();
      llvm::StringRef comparison =
          llvm::StringSwitch<llvm::StringRef>(expression.predicate)
              .Case("eq", "==")
              .Case("ne", "!=")
              .Case("ult", "<")
              .Case("ule", "<=")
              .Case("ugt", ">")
              .Case("uge", ">=")
              .Default("");
      if (comparison.empty())
        return generatorError("bounded comparison predicate is malformed");
      output << padding << "auto " << expression.result << " = gfsim::UInt<1>{"
             << left->str() << ".value() " << comparison.str() << ' '
             << right->str() << ".value()};\n";
      continue;
    }
    if (expression.kind == "slot_get_valid") {
      output << padding << "auto " << expression.result << " = slot_"
             << identifier(expression.slot) << "->valid;\n";
      continue;
    }
    if (expression.kind == "slot_get_value") {
      output << padding << "auto " << expression.result << " = slot_"
             << identifier(expression.slot) << "->value;\n";
      continue;
    }
    if (expression.kind == "table_match_ref") {
      auto maskWords = candidateMaskWords(expression.type);
      if (!maskWords)
        return generatorError("table.match reference type is unsupported");
      if (*maskWords == 1) {
        auto type = cppType(expression.type);
        if (!type)
          return type.takeError();
        output << padding << "auto " << expression.result << " = " << *type
               << "{static_cast<std::uint64_t>(" << identifier(expression.field)
               << "->get(epoch))};\n";
      } else {
        output << padding << "const auto &" << expression.result << " = "
               << identifier(expression.field) << "->get(epoch);\n";
      }
      continue;
    }
    if (expression.kind == "table_selection_index_ref" ||
        expression.kind == "table_selection_valid_ref") {
      output << padding << "auto " << expression.result << " = "
             << identifier(expression.field) << "->get(epoch).";
      if (expression.selectionCount == 1 &&
          expression.predicate != "round_robin")
        output << (expression.kind == "table_selection_index_ref" ? "index"
                                                                  : "valid");
      else
        output << (expression.kind == "table_selection_index_ref" ? "indices["
                                                                  : "valid[")
               << expression.laneOrdinal << ']';
      output << ";\n";
      continue;
    }
    if (expression.kind == "table_match") {
      if (emittedTableMatches.contains(expression.result))
        continue;
      if (expression.nestedYields.size() != 1)
        return generatorError("table.match predicate yield is missing");
      auto hasSnapshotSet = [&](llvm::StringRef result) {
        return llvm::any_of(block.expressions,
                            [&](const QueueExpressionPlan &candidate) {
                              return candidate.kind == "snapshot_set" &&
                                     candidate.field == result;
                            });
      };
      auto canFuse = [&](const QueueExpressionPlan &candidate) {
        if (candidate.kind != "table_match" ||
            candidate.table != expression.table ||
            candidate.type != expression.type ||
            candidate.operands != expression.operands ||
            candidate.domainAxes != expression.domainAxes ||
            candidate.domainShape != expression.domainShape ||
            candidate.domainStrides != expression.domainStrides ||
            candidate.domainOffset != expression.domainOffset ||
            candidate.domainBase != expression.domainBase ||
            candidate.hasDomainProjection != expression.hasDomainProjection ||
            candidate.nestedYields.size() != 1 ||
            !needed.contains(candidate.result) ||
            hasSnapshotSet(candidate.result) ||
            !llvm::all_of(candidate.nestedExpressions,
                          isEffectFreeTableMatchExpression))
          return false;
        return llvm::all_of(candidate.operands,
                            [&](const std::string &operand) {
                              auto position = expressionPositions.find(operand);
                              return position == expressionPositions.end() ||
                                     position->getValue() < expressionIndex;
                            });
      };
      std::vector<const QueueExpressionPlan *> fusedMatches;
      if (!hasSnapshotSet(expression.result) &&
          llvm::all_of(expression.nestedExpressions,
                       isEffectFreeTableMatchExpression)) {
        for (size_t index = expressionIndex; index < block.expressions.size();
             ++index) {
          const QueueExpressionPlan &candidate = block.expressions[index];
          if (canFuse(candidate))
            fusedMatches.push_back(&candidate);
        }
      }
      if (fusedMatches.size() > 1) {
        QueueBlockPlan combined;
        llvm::StringMap<std::string> commonValues;
        llvm::StringSet<> occupiedNames;
        for (const QueueExpressionPlan *match : fusedMatches) {
          for (const std::string &operand : match->operands)
            occupiedNames.insert(operand);
          for (const QueueExpressionPlan &nested : match->nestedExpressions)
            occupiedNames.insert(nested.result);
        }
        unsigned nextFusedValue = 0;
        auto freshFusedValue = [&] {
          std::string name;
          do {
            name = "fused_value_" + std::to_string(nextFusedValue++);
          } while (occupiedNames.contains(name));
          occupiedNames.insert(name);
          return name;
        };
        std::vector<std::string> predicates;
        predicates.reserve(fusedMatches.size());
        for (const QueueExpressionPlan *match : fusedMatches) {
          llvm::StringMap<std::string> renamed;
          for (const QueueExpressionPlan &nested : match->nestedExpressions) {
            QueueExpressionPlan canonical = nested;
            for (std::string &operandName : canonical.operands)
              if (auto found = renamed.find(operandName);
                  found != renamed.end())
                operandName = found->getValue();
            const std::string key = matchExpressionValueKey(canonical);
            auto found = commonValues.find(key);
            if (found != commonValues.end()) {
              renamed[nested.result] = found->getValue();
              continue;
            }
            canonical.result = freshFusedValue();
            renamed[nested.result] = canonical.result;
            commonValues[key] = canonical.result;
            combined.expressions.push_back(std::move(canonical));
          }
          auto predicate = renamed.find(match->nestedYields.front());
          predicates.push_back(predicate == renamed.end()
                                   ? match->nestedYields.front()
                                   : predicate->getValue());
        }
        combined.yields = predicates;
        std::string tuple = "std::tuple{";
        for (auto [index, predicate] : llvm::enumerate(predicates)) {
          if (index)
            tuple.append(", ");
          tuple.append(predicate);
        }
        tuple.push_back('}');
        auto predicateBody = emitExpressionBody(
            plan, combined, predicates.front(), indent + 6, qualifyTables,
            checkedTableAccess, predicates, tuple);
        if (!predicateBody)
          return predicateBody.takeError();
        const std::string table = qualifyTables
                                      ? "table_" + identifier(expression.table)
                                      : std::string("table");
        for (const QueueExpressionPlan *match : fusedMatches) {
          auto maskWords = candidateMaskWords(match->type);
          if (!maskWords)
            return generatorError(
                "table.match candidate mask type is unsupported");
          if (*maskWords == 1)
            output << padding << "std::uint64_t " << match->result << " = 0;\n";
          else
            output << padding << "std::array<std::uint64_t, " << *maskWords
                   << "> " << match->result << "{};\n";
        }
        const TablePlan *tablePlan = findTable(plan, expression.table);
        if (!tablePlan)
          return generatorError("table.match Table is missing");
        output << padding << "const auto projection_" << expression.result
               << " = " << tableProjectionArgument(*tablePlan, expression)
               << ";\n"
               << padding << "for (std::size_t index = 0; index < "
               << "projection_" << expression.result << ".size(); "
               << "++index) {\n"
               << padding << "  const std::size_t global_index = *projection_"
               << expression.result << ".globalIndex(index);\n"
               << padding << "  const auto &entry = " << table
               << "->at(global_index);\n"
               << padding << "  auto [";
        for (auto [index, match] : llvm::enumerate(fusedMatches)) {
          if (index)
            output << ", ";
          output << "fused_match_" << identifier(match->result);
        }
        output << "] = [&]() {\n" << *predicateBody << padding << "  }();\n";
        for (const QueueExpressionPlan *match : fusedMatches) {
          auto maskWords = candidateMaskWords(match->type);
          output << padding << "  if (fused_match_" << identifier(match->result)
                 << ")\n"
                 << padding << "    ";
          if (*maskWords == 1)
            output << match->result << " |= (std::uint64_t{1} << index);\n";
          else
            output << match->result
                   << "[index / 64] |= (std::uint64_t{1} << "
                      "(index % 64));\n";
          emittedTableMatches.insert(match->result);
        }
        output << padding << "}\n";
        continue;
      }
      QueueBlockPlan nested;
      nested.expressions = expression.nestedExpressions;
      nested.yields = expression.nestedYields;
      auto predicate =
          emitExpressionBody(plan, nested, nested.yields.front(), indent + 8,
                             qualifyTables, checkedTableAccess);
      if (!predicate)
        return predicate.takeError();
      std::vector<const QueueExpressionPlan *> snapshotSets;
      for (const QueueExpressionPlan &candidate : block.expressions)
        if (candidate.kind == "snapshot_set" &&
            candidate.field == expression.result)
          snapshotSets.push_back(&candidate);
      const std::string table =
          qualifyTables ? "table_" + identifier(expression.table) : "table";
      auto maskWords = candidateMaskWords(expression.type);
      if (!maskWords)
        return generatorError("table.match candidate mask type is unsupported");
      if (*maskWords == 1)
        output << padding << "std::uint64_t " << expression.result << " = 0;\n";
      else
        output << padding << "std::array<std::uint64_t, " << *maskWords << "> "
               << expression.result << "{};\n";
      for (const QueueExpressionPlan *snapshotSet : snapshotSets)
        output << padding << "gfsim::StateReservation " << snapshotSet->result
               << "{};\n";
      const TablePlan *tablePlan = findTable(plan, expression.table);
      if (!tablePlan)
        return generatorError("table.match Table is missing");
      output << padding << "const auto projection_" << expression.result
             << " = " << tableProjectionArgument(*tablePlan, expression)
             << ";\n"
             << padding << "for (std::size_t index = 0; index < "
             << "projection_" << expression.result << ".size(); "
             << "++index) {\n"
             << padding << "  const std::size_t global_index = *projection_"
             << expression.result << ".globalIndex(index);\n"
             << padding << "  const auto &entry = " << table
             << "->at(global_index);\n";
      for (auto [setIndex, snapshotSet] : llvm::enumerate(snapshotSets)) {
        std::vector<const QueueExpressionPlan *> reads;
        for (const QueueExpressionPlan &candidate :
             expression.nestedExpressions)
          if (candidate.kind == "table_get" &&
              candidate.table == snapshotSet->table)
            reads.push_back(&candidate);
        if (reads.empty())
          return generatorError("snapshot-set target read is missing");
        for (auto [readIndex, read] : llvm::enumerate(reads)) {
          if (read->operands.size() != 1)
            return generatorError("snapshot-set TableGet index is malformed");
          QueueBlockPlan indexExpression;
          indexExpression.expressions = expression.nestedExpressions;
          indexExpression.yields = {read->operands.front()};
          auto indexBody =
              emitExpressionBody(plan, indexExpression, read->operands.front(),
                                 indent + 4, qualifyTables, checkedTableAccess);
          if (!indexBody)
            return indexBody.takeError();
          output << padding << "  const auto snapshot_index_" << setIndex << '_'
                 << readIndex << " = [&]() {\n"
                 << *indexBody << padding << "  }();\n"
                 << padding << "  " << snapshotSet->result << " = "
                 << snapshotSet->result << " | gfsim::StateReservation::"
                 << (snapshotSet->predicate == "complete" ? "forEntry("
                                                          : "forFieldsAt(")
                 << "static_cast<std::size_t>(snapshot_index_" << setIndex
                 << '_' << readIndex << ")";
          if (snapshotSet->predicate == "complete")
            output << ");\n";
          else
            output << ", std::uint64_t{" << snapshotSet->mask << "}, "
                   << snapshotSet->width << ");\n";
        }
      }
      output << padding << "  if ([&]() {\n"
             << *predicate << padding << "  }())\n"
             << padding << "    ";
      if (*maskWords == 1)
        output << expression.result << " |= (std::uint64_t{1} << index);\n";
      else
        output << expression.result
               << "[index / 64] |= (std::uint64_t{1} << "
                  "(index % 64));\n";
      output << padding << "}\n";
      continue;
    }
    if (expression.kind == "snapshot_set") {
      auto source = llvm::find_if(
          block.expressions, [&](const QueueExpressionPlan &candidate) {
            return candidate.result == expression.field &&
                   (candidate.kind == "table_match" ||
                    candidate.kind == "table_choose_index");
          });
      if (source == block.expressions.end())
        return generatorError("snapshot-set source evaluation is missing");
      continue;
    }
    if (expression.kind == "table_index") {
      const TablePlan *table = findTable(plan, expression.table);
      if (!table || expression.operands.size() != table->shape.size())
        return generatorError("Table index expression is malformed");
      std::vector<uint64_t> strides(table->shape.size(), 1);
      for (size_t axis = table->shape.size(); axis > 1; --axis)
        strides[axis - 2] = strides[axis - 1] * table->shape[axis - 1];
      const std::string tableName =
          qualifyTables ? "table_" + identifier(expression.table)
                        : std::string("table");
      output << padding << "const std::optional<std::size_t> "
             << expression.result << "_checked = [&]() -> "
             << "std::optional<std::size_t> {\n";
      for (auto [axis, coordinate] : llvm::enumerate(expression.operands))
        output << padding << "  const std::size_t coordinate_" << axis
               << " = static_cast<std::size_t>(" << coordinate << ");\n"
               << padding << "  if (coordinate_" << axis
               << " >= " << table->shape[axis] << ") return std::nullopt;\n";
      output << padding << "  return ";
      for (size_t axis = 0; axis < table->shape.size(); ++axis) {
        if (axis)
          output << " + ";
        output << "coordinate_" << axis << " * " << strides[axis];
      }
      output << ";\n"
             << padding << "}();\n"
             << padding << "const std::size_t " << expression.result << " = "
             << expression.result << "_checked.value_or(" << tableName
             << "->size());\n";
      continue;
    }
    auto first = operand(0);
    if (!first)
      return first.takeError();
    if (expression.kind == "recovery_event") {
      if (expression.operands.size() != 4)
        return generatorError("recovery_event expression arity mismatch");
      output << padding << "auto " << expression.result << " = "
             << first->str() << ";\n";
      continue;
    }
    if (expression.kind == "kill_set") {
      if (expression.operands.size() != 5 ||
          expression.predicate != "epoch_mismatch_or_younger")
        return generatorError("kill_set expression contract is malformed");
      auto transactionEpoch = operand(1);
      auto nextEpoch = operand(2);
      auto transactionSlot = operand(3);
      auto boundary = operand(4);
      if (!transactionEpoch || !nextEpoch || !transactionSlot || !boundary)
        return generatorError("kill_set operand is unavailable");
      output << padding << "auto " << expression.result
             << " = static_cast<bool>(" << first->str() << ") && (("
             << transactionEpoch->str() << " != " << nextEpoch->str()
             << ") || (" << boundary->str() << " < "
             << transactionSlot->str() << "));\n";
      continue;
    }
    if (expression.kind == "reservation_set" ||
        expression.kind == "memory_order_edge") {
      output << padding << "auto " << expression.result << " = "
             << first->str();
      for (size_t index = 1; index < expression.operands.size(); ++index) {
        auto current = operand(index);
        if (!current)
          return current.takeError();
        output << " & " << current->str();
      }
      output << ";\n";
      continue;
    }
    if (expression.kind.starts_with("load_disposition_")) {
      if (expression.operands.size() != 7)
        return generatorError("load-disposition operand count mismatch");
      const uint64_t lanes = expression.selectionCount;
      std::array<std::string, 7> names;
      for (size_t index = 0; index < names.size(); ++index) {
        auto current = operand(index);
        if (!current)
          return current.takeError();
        names[index] = current->str();
      }
      const std::string &pending = names[0];
      const std::string &alias = names[1];
      const std::string &disjoint = names[2];
      const std::string &ready = names[3];
      const std::string &executed = names[4];
      const std::string &identity = names[5];
      const std::string &killed = names[6];
      output << padding << "auto " << expression.result << " = [&]() {\n"
             << padding << "  const std::uint64_t pending = " << pending
             << ".value();\n"
             << padding << "  const std::uint64_t alias = " << alias
             << ".value();\n"
             << padding << "  const std::uint64_t disjoint = " << disjoint
             << ".value();\n"
             << padding << "  const std::uint64_t ready = " << ready
             << ".value();\n"
             << padding << "  const std::uint64_t executed = " << executed
             << ".value();\n"
             << padding << "  const std::uint64_t identity = " << identity
             << ".value();\n"
             << padding << "  const std::uint64_t killed = " << killed
             << ".value();\n"
             << padding
             << "  const std::uint64_t stale = pending & (~identity | "
                "killed);\n"
             << padding
             << "  const std::uint64_t qualified = pending & ~stale;\n"
             << padding
             << "  const std::uint64_t forward = qualified & alias & ready & "
                "~executed;\n"
             << padding
             << "  const std::uint64_t replay = qualified & alias & executed;\n"
             << padding
             << "  const std::uint64_t bypass = qualified & disjoint & "
                "~alias;\n"
             << padding << "  const std::uint64_t wait = qualified & "
                "~(forward | replay | bypass);\n"
             << padding << "  return gfsim::UInt<" << lanes << ">{"
             << (expression.kind == "load_disposition_wait"
                     ? "wait"
                 : expression.kind == "load_disposition_bypass"
                     ? "bypass"
                 : expression.kind == "load_disposition_forward"
                     ? "forward"
                 : expression.kind == "load_disposition_replay" ? "replay"
                                                                 : "stale")
             << "};\n"
             << padding << "}();\n";
      continue;
    }
    if (expression.kind == "transaction_group") {
      auto reserved = operand(1);
      if (!reserved)
        return reserved.takeError();
      const uint64_t lanes = expression.selectionCount;
      if (expression.predicate == "independent") {
        output << padding << "auto " << expression.result << " = "
               << first->str() << " & " << reserved->str() << ";\n";
      } else if (expression.predicate == "all_or_none") {
        output << padding << "auto " << expression.result
               << " = ((" << first->str() << " & " << reserved->str()
               << ") == " << first->str() << ") ? " << first->str()
               << " : gfsim::UInt<" << lanes << ">{0};\n";
      } else {
        output << padding << "auto " << expression.result << " = [&]() {\n"
               << padding << "  const std::uint64_t valid = " << first->str()
               << ".value();\n"
               << padding << "  const std::uint64_t reserved = "
               << reserved->str() << ".value();\n"
               << padding << "  std::uint64_t accepted = 0;\n"
               << padding << "  bool prefix = true;\n"
               << padding << "  for (std::size_t lane = 0; lane < " << lanes
               << "; ++lane) {\n"
               << padding
               << "    const bool take = prefix && ((valid >> lane) & 1u) && "
                  "((reserved >> lane) & 1u);\n"
               << padding << "    if (take) accepted |= std::uint64_t{1} << lane;\n"
               << padding << "    prefix = take;\n"
               << padding << "  }\n"
               << padding << "  return gfsim::UInt<" << lanes
               << ">{accepted};\n"
               << padding << "}();\n";
      }
      continue;
    }
    if (expression.kind.starts_with("multi_allocator_")) {
      auto requests = operand(1);
      auto release = operand(2);
      if (!requests || !release)
        return generatorError("multi-allocator operand is unavailable");
      const uint64_t lanes = expression.selectionCount;
      const unsigned resultOrdinal =
          expression.kind == "multi_allocator_allocation" ? 0
          : expression.kind == "multi_allocator_accepted" ? 1
                                                            : 2;
      output << padding << "auto " << expression.result << " = [&]() {\n"
             << padding << "  const std::uint64_t free_mask = " << first->str()
             << ".value();\n"
             << padding << "  const std::uint64_t request_mask = "
             << requests->str() << ".value();\n"
             << padding << "  const std::uint64_t release_mask = "
             << release->str() << ".value();\n"
             << padding << "  const std::uint64_t candidate_free = "
             << (expression.predicate == "allow"
                     ? "free_mask | release_mask"
                     : "free_mask")
             << ";\n"
             << padding << "  unsigned free_count = 0;\n"
             << padding << "  for (std::size_t slot = 0; slot < " << lanes
             << "; ++slot) free_count += (candidate_free >> slot) & 1u;\n"
             << padding << "  unsigned accepted_count = 0;\n"
             << padding << "  std::uint64_t accepted = 0;\n"
             << padding << "  bool prefix = true;\n"
             << padding << "  for (std::size_t lane = 0; lane < " << lanes
             << "; ++lane) {\n"
             << padding
             << "    const bool take = prefix && ((request_mask >> lane) & 1u) "
                "&& accepted_count < free_count;\n"
             << padding << "    if (take) { accepted |= std::uint64_t{1} << lane; ++accepted_count; }\n"
             << padding << "    prefix = take;\n"
             << padding << "  }\n"
             << padding << "  unsigned allocated_count = 0;\n"
             << padding << "  std::uint64_t allocation = 0;\n"
             << padding << "  for (std::size_t slot = 0; slot < " << lanes
             << "; ++slot) {\n"
             << padding
             << "    if (((candidate_free >> slot) & 1u) && allocated_count < "
                "accepted_count) { allocation |= std::uint64_t{1} << slot; "
                "++allocated_count; }\n"
             << padding << "  }\n"
             << padding << "  const std::uint64_t remaining = candidate_free & ~allocation;\n"
             << padding << "  const std::uint64_t next_free = "
             << (expression.predicate == "allow"
                     ? "remaining"
                     : "remaining | release_mask")
             << ";\n"
             << padding << "  return gfsim::UInt<" << lanes << ">{"
             << (resultOrdinal == 0 ? "allocation"
                 : resultOrdinal == 1 ? "accepted"
                                      : "next_free")
             << "};\n"
             << padding << "}();\n";
      continue;
    }
    if (expression.kind == "age_select_k") {
      const uint64_t lanes = expression.selectionCount;
      output << padding << "auto " << expression.result << " = [&]() {\n"
             << padding << "  std::uint64_t remaining = " << first->str()
             << ".value();\n"
             << padding << "  std::uint64_t winners = 0;\n"
             << padding << "  const std::array<std::uint64_t, " << lanes
             << "> ages{";
      for (size_t index = 1; index < expression.operands.size(); ++index) {
        auto age = operand(index);
        if (!age)
          return age.takeError();
        if (index != 1)
          output << ", ";
        output << age->str() << ".value()";
      }
      output << "};\n"
             << padding << "  for (std::size_t pick = 0; pick < "
             << expression.laneOrdinal << "; ++pick) {\n"
             << padding << "    std::size_t winner = " << lanes << ";\n"
             << padding << "    for (std::size_t lane = 0; lane < " << lanes
             << "; ++lane) {\n"
             << padding << "      if (!((remaining >> lane) & 1u)) continue;\n"
             << padding << "      if (winner == " << lanes
             << " || ages[lane] < ages[winner]) winner = lane;\n"
             << padding << "    }\n"
             << padding << "    if (winner == " << lanes << ") break;\n"
             << padding << "    winners |= std::uint64_t{1} << winner;\n"
             << padding << "    remaining &= ~(std::uint64_t{1} << winner);\n"
             << padding << "  }\n"
             << padding << "  return gfsim::UInt<" << lanes
             << ">{winners};\n"
             << padding << "}();\n";
      continue;
    }
    if (expression.kind.starts_with("dependency_set_")) {
      llvm::SmallVector<std::string> values;
      for (size_t index = 0; index < expression.operands.size(); ++index) {
        auto current = operand(index);
        if (!current)
          return current.takeError();
        values.push_back(current->str());
      }
      const std::string next = "((" + values[0] + " | " + values[1] +
                               ") & ~(" + values[2] + " & " + values[4] +
                               ") & ~(" + values[3] + " & " + values[4] +
                               "))";
      output << padding << "auto " << expression.result << " = "
             << (expression.kind == "dependency_set_next"
                     ? next
                     : next + " == gfsim::UInt<" +
                           std::to_string(expression.selectionCount) + ">{0}")
             << ";\n";
      continue;
    }
    if (expression.kind == "terminal_transaction") {
      auto effects = operand(1);
      auto terminal = operand(2);
      if (!effects || !terminal)
        return generatorError("terminal transaction operand is unavailable");
      output << padding << "auto " << expression.result << " = "
             << first->str() << " & " << effects->str() << " & "
             << terminal->str() << ";\n";
      continue;
    }
    if (expression.kind == "versioned_lookup_payload" ||
        expression.kind == "versioned_lookup_valid") {
      const TablePlan *tablePlan = findTable(plan, expression.table);
      if (!tablePlan || !tablePlan->versioned ||
          (expression.operands.size() != 3 &&
           expression.operands.size() != 4))
        return generatorError("versioned lookup contract is malformed");
      const std::string table =
          qualifyTables ? "table_" + identifier(expression.table) : "table";
      const std::string entry = expression.result + "_entry";
      output << padding << "const auto &" << entry << " = " << table
             << (checkedTableAccess ? "->checkedAt(static_cast<size_t>("
                                    : "->at(static_cast<size_t>(")
             << first->str() << "));\n";
      if (expression.kind == "versioned_lookup_payload") {
        output << padding << "auto " << expression.result << " = " << entry
               << "." << identifier(tablePlan->payloadField) << ";\n";
      } else {
        auto refGeneration = operand(1);
        auto refEpoch = operand(2);
        if (!refGeneration || !refEpoch)
          return generatorError("versioned lookup refs are unavailable");
        output << padding << "auto " << expression.result
               << " = static_cast<bool>(" << entry << "."
               << identifier(tablePlan->validField) << ") && (" << entry
               << "." << identifier(tablePlan->generationField) << " == "
               << refGeneration->str() << ") && (" << entry << "."
               << identifier(tablePlan->epochField) << " == "
               << refEpoch->str() << ")";
        if (expression.operands.size() == 4) {
          auto refAttempt = operand(3);
          if (!refAttempt)
            return generatorError("versioned lookup attempt is unavailable");
          output << " && (" << entry << "."
                 << identifier(tablePlan->attemptField) << " == "
                 << refAttempt->str() << ")";
        }
        output << ";\n";
      }
      continue;
    }
    if (expression.kind == "masked_match") {
      output << padding << "auto " << expression.result << " = ("
             << first->str() << " & std::uint64_t{" << expression.mask
             << "}) == std::uint64_t{" << expression.value << "};\n";
      continue;
    }
    if (expression.kind == "get") {
      output << padding
             << (isAggregateValueType(plan, expression.type) ? "const auto &"
                                                             : "auto ")
             << expression.result << " = " << first->str()
             << (sharedValues.contains(*first) ? "->" : ".")
             << identifier(expression.field) << ";\n";
      continue;
    }
    if (expression.kind == "table_get") {
      const std::string table =
          qualifyTables ? "table_" + identifier(expression.table) : "table";
      output << padding
             << (isAggregateValueType(plan, expression.type) ? "const auto &"
                                                             : "auto ")
             << expression.result << " = " << table
             << (checkedTableAccess ? "->checkedAt(static_cast<size_t>("
                                    : "->at(static_cast<size_t>(")
             << first->str() << "));\n";
      continue;
    }
    if (expression.kind == "popcount") {
      output << padding << "auto " << expression.result
             << " = gfsim::populationCount(" << first->str() << ");\n";
      continue;
    }
    if (expression.kind == "count_zeros") {
      output << padding << "auto " << expression.result
             << (expression.predicate == "trailing"
                     ? " = gfsim::countTrailingZeros("
                     : " = gfsim::countLeadingZeros(")
             << first->str() << ");\n";
      continue;
    }
    if (expression.kind == "table_choose_index" ||
        expression.kind == "table_choose_valid") {
      QueueBlockPlan nested;
      nested.expressions = expression.nestedExpressions;
      nested.yields = expression.nestedYields;
      std::string choiceKey =
          inlineTableChoiceContractKey(expression) + "#" +
          std::to_string(tableChoicePairOrdinals.lookup(expression.result));
      const std::string choiceContract =
          inlineTableChoiceContractKey(expression);
      if (auto cached = tableChoices.find(choiceKey);
          cached != tableChoices.end()) {
        auto resultType = cppType(expression.type);
        if (!resultType)
          return resultType.takeError();
        output << padding << "auto " << expression.result << " = "
               << *resultType << "{"
               << (expression.kind == "table_choose_index"
                       ? cached->second.first
                       : cached->second.second)
               << "};\n";
        continue;
      }
      const std::string choice = "choice_" + identifier(expression.result);
      const std::string choiceIndex = choice + "_index";
      const std::string choiceValid = choice + "_valid";
      const std::string choiceBest = choice + "_best";
      auto maskExpression = llvm::find_if(
          block.expressions, [&](const QueueExpressionPlan &candidate) {
            return candidate.result == first->str();
          });
      auto maskWords = maskExpression == block.expressions.end()
                           ? std::optional<uint64_t>()
                           : candidateMaskWords(maskExpression->type);
      if (!maskWords)
        return generatorError(
            "table.choose candidate mask type is unsupported");
      std::vector<const QueueExpressionPlan *> snapshotSets;
      for (const QueueExpressionPlan &candidate : block.expressions)
        if (candidate.kind == "snapshot_set" &&
            candidate.field == expression.result)
          snapshotSets.push_back(&candidate);
      auto choiceTable =
          llvm::find_if(plan.tables, [&](const TablePlan &table) {
            return table.name == expression.table;
          });
      auto scalarMaskWidth =
          choiceTable != plan.tables.end() && choiceTable->entries <= 64 &&
                  maskExpression != block.expressions.end() &&
                  !maskExpression->hasDomainProjection
              ? std::optional<unsigned>(choiceTable->entries)
              : std::nullopt;
      if (expression.predicate == "first" && expression.selectionCount == 1 &&
          scalarMaskWidth && nested.expressions.empty() &&
          nested.yields.empty() && snapshotSets.empty()) {
        auto resultType = cppType(expression.type);
        if (!resultType)
          return resultType.takeError();
        output << padding << "auto " << choice
               << " = gfsim::priorityEncode(gfsim::UInt<" << *scalarMaskWidth
               << ">{static_cast<std::uint64_t>(" << first->str()
               << ")}, true);\n"
               << padding << "auto " << expression.result << " = "
               << *resultType << "{"
               << (expression.kind == "table_choose_index" ? choice + ".index"
                                                           : choice + ".valid")
               << "};\n";
        tableChoices[choiceKey] = {choice + ".index", choice + ".valid"};
        continue;
      }
      for (const QueueExpressionPlan *snapshotSet : snapshotSets)
        output << padding << "gfsim::StateReservation " << snapshotSet->result
               << "{};\n";
      output << padding << "std::uint64_t " << choiceIndex << " = 0;\n"
             << padding << "bool " << choiceValid << " = false;\n"
             << padding
             << (expression.keyOrdering == "signed" ? "std::int64_t "
                                                    : "std::uint64_t ")
             << choiceBest << " = 0;\n";
      if (choiceTable == plan.tables.end() ||
          maskExpression == block.expressions.end())
        return generatorError("table.choose projection metadata is missing");
      output << padding << "const auto projection_" << choice << " = "
             << tableProjectionArgument(*choiceTable, *maskExpression) << ";\n"
             << padding << "for (std::size_t index = 0; index < projection_"
             << choice << ".size(); ++index) {\n"
             << padding << "  if ((";
      if (*maskWords == 1)
        output << "static_cast<std::uint64_t>(" << first->str()
               << ") & (std::uint64_t{1} << index)";
      else
        output << first->str()
               << "[index / 64] & (std::uint64_t{1} << (index % 64))";
      output << ") == 0) continue;\n";
      output << padding << "  const std::size_t global_index = *projection_"
             << choice << ".globalIndex(index);\n";
      for (const std::string &prior : priorChoiceIndices[choiceContract])
        output << padding << "  if (global_index == " << prior
               << ") continue;\n";
      if (expression.predicate != "first")
        output << padding << "  const auto &entry = "
               << (qualifyTables ? "table_" + identifier(expression.table)
                                 : std::string("table"))
               << "->at(global_index);\n";
      for (auto [setIndex, snapshotSet] : llvm::enumerate(snapshotSets)) {
        std::vector<const QueueExpressionPlan *> reads;
        for (const QueueExpressionPlan &candidate : nested.expressions)
          if (candidate.kind == "table_get" &&
              candidate.table == snapshotSet->table)
            reads.push_back(&candidate);
        if (reads.empty())
          return generatorError(
              "snapshot-set choose-key target read is missing");
        for (auto [readIndex, read] : llvm::enumerate(reads)) {
          if (read->operands.size() != 1)
            return generatorError(
                "snapshot-set choose-key TableGet index is malformed");
          QueueBlockPlan indexExpression;
          indexExpression.expressions = nested.expressions;
          indexExpression.yields = {read->operands.front()};
          auto indexBody =
              emitExpressionBody(plan, indexExpression, read->operands.front(),
                                 indent + 4, qualifyTables, checkedTableAccess);
          if (!indexBody)
            return indexBody.takeError();
          output << padding << "  const auto snapshot_index_" << setIndex << '_'
                 << readIndex << " = [&]() {\n"
                 << *indexBody << padding << "  }();\n"
                 << padding << "  " << snapshotSet->result << " = "
                 << snapshotSet->result << " | gfsim::StateReservation::"
                 << (snapshotSet->predicate == "complete" ? "forEntry("
                                                          : "forFieldsAt(")
                 << "static_cast<std::size_t>(snapshot_index_" << setIndex
                 << '_' << readIndex << ")";
          if (snapshotSet->predicate == "complete")
            output << ");\n";
          else
            output << ", std::uint64_t{" << snapshotSet->mask << "}, "
                   << snapshotSet->width << ");\n";
        }
      }
      if (expression.predicate == "first") {
        output << padding << "  " << choiceIndex << " = global_index;\n"
               << padding << "  " << choiceValid << " = true;\n"
               << padding << "  break;\n";
      } else {
        if (nested.yields.size() != 1)
          return generatorError("table.choose key yield is missing");
        auto key =
            emitExpressionBody(plan, nested, nested.yields.front(), indent + 6,
                               qualifyTables, checkedTableAccess);
        if (!key)
          return key.takeError();
        const char *comparison = expression.predicate == "min" ? "<" : ">";
        const std::string normalizedKey =
            expression.keyOrdering == "signed"
                ? "gfsim::signedValue(key)"
                : "static_cast<std::uint64_t>(key)";
        output << padding << "  auto key = [&]() {\n"
               << *key << padding << "  }();\n"
               << padding << "  if (!" << choiceValid << " || " << normalizedKey
               << ' ' << comparison << " " << choiceBest << ") {\n"
               << padding << "    " << choiceIndex << " = global_index;\n"
               << padding << "    " << choiceValid << " = true;\n"
               << padding << "    " << choiceBest << " = " << normalizedKey
               << ";\n"
               << padding << "  }\n";
      }
      auto resultType = cppType(expression.type);
      if (!resultType)
        return resultType.takeError();
      output << padding << "}\n"
             << padding << "auto " << expression.result << " = " << *resultType
             << "{"
             << (expression.kind == "table_choose_index" ? choiceIndex
                                                         : choiceValid)
             << "};\n";
      tableChoices[choiceKey] = {choiceIndex, choiceValid};
      if (expression.kind == "table_choose_index")
        priorChoiceIndices[choiceContract].push_back(choiceIndex);
      continue;
    }
    if (expression.kind == "not") {
      output << padding << "auto " << expression.result << " = ~"
             << first->str() << ";\n";
      continue;
    }
    if (expression.kind == "priority_index" ||
        expression.kind == "priority_valid") {
      std::string key = first->str() + "#" + expression.predicate;
      auto [entry, inserted] = priorityEncodings.try_emplace(
          key, "priority_" + identifier(expression.result));
      if (inserted)
        output << padding << "auto " << entry->getValue()
               << " = gfsim::priorityEncode(" << first->str() << ", "
               << (expression.predicate == "low" ? "true" : "false") << ");\n";
      output << padding << "auto " << expression.result << " = "
             << entry->getValue() << "."
             << (expression.kind == "priority_index" ? "index" : "valid")
             << ";\n";
      continue;
    }
    if (expression.kind == "bit_extract") {
      output << padding << "auto " << expression.result
             << " = gfsim::bitExtract<" << expression.width << ">("
             << first->str() << ", " << expression.lsb << ");\n";
      continue;
    }
    if (expression.kind == "array_get_dynamic") {
      auto array = operand(0);
      auto index = operand(1);
      if (!array)
        return array.takeError();
      if (!index)
        return index.takeError();
      auto resultType = cppType(expression.type);
      if (!resultType)
        return resultType.takeError();
      output << padding << "auto " << expression.result << " = [&]() -> "
             << *resultType << " {\n"
             << padding << "  switch (static_cast<std::uint64_t>("
             << index->str() << ")) {\n";
      for (uint64_t element = 0; element < expression.selectionCount;
           ++element) {
        const uint64_t lsb =
            expression.width * (expression.selectionCount - element - 1);
        const std::string extracted =
            "gfsim::bitExtract<" + std::to_string(expression.width) + ">(" +
            array->str() + ", " + std::to_string(lsb) + ")";
        auto unpacked = emitUnpackedValue(plan, expression.type, extracted);
        if (!unpacked)
          return unpacked.takeError();
        output << padding << "  case " << element << ": return " << *unpacked
               << ";\n";
      }
      output << padding << "  default: return " << *resultType << "{};\n"
             << padding << "  }\n"
             << padding << "}();\n";
      continue;
    }
    if (expression.kind == "array_update_dynamic") {
      auto array = operand(0);
      auto index = operand(1);
      auto replacement = operand(2);
      if (!array)
        return array.takeError();
      if (!index)
        return index.takeError();
      if (!replacement)
        return replacement.takeError();
      const QueueAggregatePlan *aggregate =
          findAggregateType(plan, expression.type);
      if (!aggregate || aggregate->kind != "array" ||
          aggregate->elements.size() != 1)
        return generatorError("dynamic value_array update type is unresolved");
      auto packed = emitPackedValue(plan, aggregate->elements.front(),
                                    replacement->str());
      if (!packed)
        return packed.takeError();
      output << padding << "auto " << expression.result
             << " = gfsim::arrayUpdate<" << expression.selectionCount << ", "
             << expression.width << ">(" << array->str() << ", " << index->str()
             << ", " << *packed << ");\n";
      continue;
    }
    if (expression.kind == "aggregate_get") {
      const std::string extracted =
          "gfsim::bitExtract<" + std::to_string(expression.width) + ">(" +
          first->str() + ", " + std::to_string(expression.lsb) + ")";
      auto unpacked = emitUnpackedValue(plan, expression.type, extracted);
      if (!unpacked)
        return unpacked.takeError();
      output << padding << "auto " << expression.result << " = " << *unpacked
             << ";\n";
      continue;
    }
    if (expression.kind == "bit_concat") {
      output << padding << "auto " << expression.result
             << " = gfsim::bitConcat(";
      for (auto [index, value] : llvm::enumerate(expression.operands)) {
        if (index)
          output << ", ";
        output << value;
      }
      output << ");\n";
      continue;
    }
    if (expression.kind == "tuple_create" ||
        expression.kind == "array_create") {
      const QueueAggregatePlan *aggregate =
          findAggregateType(plan, expression.type);
      if (!aggregate)
        return generatorError("aggregate create type is unresolved");
      const bool tuple = expression.kind == "tuple_create";
      if ((tuple && aggregate->kind != "tuple") ||
          (!tuple && aggregate->kind != "array"))
        return generatorError("aggregate create kind disagrees with its type");
      if ((tuple && aggregate->elements.size() != expression.operands.size()) ||
          (!tuple && (aggregate->elements.size() != 1 ||
                      aggregate->length != expression.operands.size())))
        return generatorError("aggregate create operand arity mismatch");
      output << padding << "auto " << expression.result
             << " = gfsim::bitConcat(";
      for (auto [index, value] : llvm::enumerate(expression.operands)) {
        auto packed = emitPackedValue(
            plan, tuple ? aggregate->elements[index] : aggregate->elements[0],
            value);
        if (!packed)
          return packed.takeError();
        if (index)
          output << ", ";
        output << *packed;
      }
      output << ");\n";
      continue;
    }
    if (expression.kind == "record_create") {
      const QueuePayloadPlan *payload = findPayloadType(plan, expression.type);
      if (!payload)
        return generatorError("record create type is unresolved");
      if (payload->fields.size() != expression.operands.size())
        return generatorError("record create operand arity mismatch");
      auto type = cppType(expression.type);
      if (!type)
        return type.takeError();
      output << padding << "auto " << expression.result << " = " << *type
             << "{";
      for (auto [index, value] : llvm::enumerate(expression.operands)) {
        if (index)
          output << ", ";
        output << value;
      }
      output << "};\n";
      continue;
    }
    if (expression.kind == "bit_insert") {
      auto value = operand(1);
      if (!value)
        return value.takeError();
      output << padding << "auto " << expression.result
             << " = gfsim::bitInsert(" << first->str() << ", " << value->str()
             << ", " << expression.lsb << ");\n";
      continue;
    }
    if (expression.kind == "value_select") {
      auto trueValue = operand(1);
      auto falseValue = operand(2);
      if (!trueValue)
        return trueValue.takeError();
      if (!falseValue)
        return falseValue.takeError();
      output << padding << "auto " << expression.result << " = " << first->str()
             << " ? " << trueValue->str() << " : " << falseValue->str()
             << ";\n";
      if (sharedValues.contains(*trueValue) &&
          sharedValues.contains(*falseValue))
        sharedValues.insert(expression.result);
      continue;
    }
    auto second = operand(1);
    if (!second)
      return second.takeError();
    if (expression.kind == "with") {
      output << padding << "auto " << expression.result << " = "
             << (sharedValues.contains(*first) ? "*" : "") << first->str()
             << ";\n";
      output << padding << expression.result << '.'
             << identifier(expression.field) << " = " << second->str() << ";\n";
      continue;
    }
    if (expression.type == "i1" &&
        (expression.kind == "mul" || expression.kind == "and" ||
         expression.kind == "or")) {
      output << padding << "auto " << expression.result
             << " = gfsim::UInt<1>{static_cast<bool>(" << first->str() << ") "
             << (expression.kind == "or" ? "||" : "&&") << " static_cast<bool>("
             << second->str() << ")};\n";
      continue;
    }
    llvm::StringRef operation;
    if (expression.kind == "add")
      operation = "+";
    else if (expression.kind == "sub")
      operation = "-";
    else if (expression.kind == "mul")
      operation = "*";
    else if (expression.kind == "udiv")
      operation = "/";
    else if (expression.kind == "urem")
      operation = "%";
    else if (expression.kind == "and")
      operation = "&";
    else if (expression.kind == "or")
      operation = "|";
    else if (expression.kind == "xor")
      operation = "^";
    else if (expression.kind == "shl")
      operation = "<<";
    else if (expression.kind == "shr")
      operation = ">>";
    else if (expression.kind == "cmp") {
      operation = llvm::StringSwitch<llvm::StringRef>(expression.predicate)
                      .Case("eq", "==")
                      .Case("ne", "!=")
                      .Case("slt", "<")
                      .Case("sle", "<=")
                      .Case("sgt", ">")
                      .Case("sge", ">=")
                      .Case("ult", "<")
                      .Case("ule", "<=")
                      .Case("ugt", ">")
                      .Case("uge", ">=")
                      .Default("");
    }
    if (operation.empty())
      return generatorError("unsupported Var expression kind '" +
                            expression.kind + "'");
    output << padding << "auto " << expression.result << " = ";
    if (expression.kind == "cmp") {
      output << "gfsim::UInt<1>{";
      if (expression.predicate.starts_with("s"))
        output << "gfsim::signedValue(" << first->str() << ") "
               << operation.str() << " gfsim::signedValue(" << second->str()
               << ")";
      else
        output << first->str() << ' ' << operation.str() << ' '
               << second->str();
      output << "}";
    } else {
      output << first->str() << ' ' << operation.str() << ' ' << second->str();
    }
    output << ";\n";
  }
  output << "#line 1 \"generated/agentic-circuit.cpp\"\n";
  llvm::StringRef returned = returnExpression.empty() ? yield : returnExpression;
  const QueuePlan *outputQueue = nullptr;
  if (returnExpression.empty())
    for (auto [index, candidate] : llvm::enumerate(block.yields))
      if (candidate == yield && index < block.outputs.size()) {
        outputQueue = findQueue(plan, block.outputs[index]);
        break;
      }
  bool wrapShared = false;
  std::string sharedValueType;
  if (outputQueue && !plan.definition.empty()) {
    auto shared = usesSharedQueueStorage(plan, outputQueue->payloadType);
    if (!shared)
      return shared.takeError();
    wrapShared = *shared && !sharedValues.contains(returned);
    if (wrapShared) {
      auto type = cppValueType(plan, outputQueue->payloadType);
      if (!type)
        return type.takeError();
      sharedValueType = std::move(*type);
    }
  }
  output << padding << "return ";
  if (wrapShared)
    output << "std::make_shared<const " << sharedValueType << ">(";
  output << returned.str();
  if (wrapShared)
    output << ')';
  output << ";\n";
  return output.str();
}

enum class HelperEmission { CombinedStatic, Declarations, Definitions };

llvm::Error emitHelperDefinitions(
    std::ostringstream &output, const QueueGraphPlan &plan,
    llvm::StringSet<> *emitted = nullptr,
    HelperEmission emission = HelperEmission::CombinedStatic) {
  auto emitResultType = [&](const QueueHelperPlan &helper) -> llvm::Error {
    if (helper.resultTypes.size() == 1) {
      auto type = cppValueType(plan, helper.resultTypes.front());
      if (!type)
        return type.takeError();
      output << *type;
    } else {
      output << "std::tuple<";
      for (auto [index, resultType] : llvm::enumerate(helper.resultTypes)) {
        auto type = cppValueType(plan, resultType);
        if (!type)
          return type.takeError();
        if (index)
          output << ", ";
        output << *type;
      }
      output << '>';
    }
    return llvm::Error::success();
  };
  auto emitArguments = [&](const QueueHelperPlan &helper) -> llvm::Error {
    for (auto [index, inputType] : llvm::enumerate(helper.inputTypes)) {
      auto type = cppValueType(plan, inputType);
      if (!type)
        return type.takeError();
      if (index)
        output << ", ";
      output << *type << ' ' << identifier(helper.inputNames[index]);
    }
    return llvm::Error::success();
  };
  if (emission != HelperEmission::Definitions) {
    for (const QueueHelperPlan &helper : plan.helpers) {
      if (emitted && emitted->contains(helper.name))
        continue;
      if (emission == HelperEmission::CombinedStatic)
        output << "static ";
      if (auto error = emitResultType(helper))
        return error;
      output << " helper_" << identifier(helper.name) << '(';
      if (auto error = emitArguments(helper))
        return error;
      output << ");\n";
    }
    if (!plan.helpers.empty())
      output << '\n';
    if (emission == HelperEmission::Declarations) {
      if (emitted)
        for (const QueueHelperPlan &helper : plan.helpers)
          emitted->insert(helper.name);
      return llvm::Error::success();
    }
  }
  for (const QueueHelperPlan &helper : plan.helpers) {
    if (emitted && !emitted->insert(helper.name).second)
      continue;
    if (emission == HelperEmission::CombinedStatic)
      output << "static ";
    if (auto error = emitResultType(helper))
      return error;
    output << " helper_" << identifier(helper.name) << '(';
    if (auto error = emitArguments(helper))
      return error;
    output << ") {\n";
    QueueBlockPlan body;
    body.expressions = helper.expressions;
    body.yields = helper.yields;
    std::string returned;
    if (helper.yields.size() > 1) {
      returned = "std::tuple{";
      for (auto [index, yield] : llvm::enumerate(helper.yields)) {
        if (index)
          returned += ", ";
        returned += yield;
      }
      returned += "}";
    }
    auto generated = emitExpressionBody(plan, body, helper.yields.front(), 2,
                                        false, true, {}, returned);
    if (!generated)
      return generated.takeError();
    output << *generated << "}\n\n";
  }
  return llvm::Error::success();
}

bool referencesTable(const std::vector<QueueExpressionPlan> &expressions,
                     llvm::StringRef table) {
  for (const QueueExpressionPlan &expression : expressions) {
    if (expression.table == table ||
        referencesTable(expression.nestedExpressions, table))
      return true;
  }
  return false;
}

bool referencesSelection(const std::vector<QueueExpressionPlan> &expressions,
                         llvm::StringRef selection) {
  for (const QueueExpressionPlan &expression : expressions)
    if (((expression.kind == "table_selection_index_ref" ||
          expression.kind == "table_selection_valid_ref") &&
         expression.field == selection) ||
        referencesSelection(expression.nestedExpressions, selection))
      return true;
  return false;
}

const QueuePlan *findQueue(const QueueGraphPlan &plan, llvm::StringRef name) {
  auto found =
      std::find_if(plan.queues.begin(), plan.queues.end(),
                   [&](const QueuePlan &queue) { return queue.name == name; });
  return found == plan.queues.end() ? nullptr : &*found;
}

const TablePlan *findTable(const QueueGraphPlan &plan, llvm::StringRef name) {
  auto found =
      std::find_if(plan.tables.begin(), plan.tables.end(),
                   [&](const TablePlan &table) { return table.name == name; });
  return found == plan.tables.end() ? nullptr : &*found;
}

struct ReservationFieldEncoding {
  uint64_t mask = 0;
  unsigned count = 0;
  bool complete = false;
};

llvm::Expected<ReservationFieldEncoding>
reservationFieldMask(const QueueGraphPlan &plan, const TablePlan &table,
                     llvm::ArrayRef<std::string> fields) {
  if (fields.empty())
    return generatorError("state reservation fields are empty");
  if (!llvm::StringRef(table.entryType).starts_with("!ac.struct<")) {
    if (fields.size() != 1 || fields.front() != "$entry")
      return generatorError("scalar state reservation requires $entry");
    return ReservationFieldEncoding{1, 1, true};
  }
  size_t marker = table.entryType.rfind('@');
  size_t end = table.entryType.rfind('>');
  if (marker == std::string::npos || end == std::string::npos || marker >= end)
    return generatorError("state reservation Entry type is malformed");
  llvm::StringRef payloadName(table.entryType.data() + marker + 1,
                              end - marker - 1);
  auto payload =
      llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
        return candidate.name == payloadName;
      });
  if (payload == plan.payloads.end() || payload->fields.size() > 64)
    return generatorError("state reservation Entry requires at most 64 fields");
  uint64_t mask = 0;
  for (const std::string &field : fields) {
    auto declared = llvm::find_if(payload->fields,
                                  [&](const QueuePayloadFieldPlan &candidate) {
                                    return candidate.name == field;
                                  });
    if (declared == payload->fields.end())
      return generatorError("state reservation field is not declared");
    mask |= uint64_t{1} << static_cast<unsigned>(
                std::distance(payload->fields.begin(), declared));
  }
  const unsigned count = static_cast<unsigned>(payload->fields.size());
  const uint64_t completeMask =
      count == 64 ? ~uint64_t{0} : (uint64_t{1} << count) - 1;
  return ReservationFieldEncoding{mask, count, mask == completeMask};
}

const StateWritePlan *findStateWrite(const QueueBlockPlan &block,
                                     llvm::StringRef table) {
  auto found =
      llvm::find_if(block.stateWrites, [&](const StateWritePlan &write) {
        return write.table == table;
      });
  return found == block.stateWrites.end() ? nullptr : &*found;
}

std::vector<size_t> findStateWriteOrdinals(const QueueBlockPlan &block,
                                           llvm::StringRef table) {
  std::vector<size_t> result;
  for (auto [ordinal, write] : llvm::enumerate(block.stateWrites))
    if (write.table == table)
      result.push_back(ordinal);
  return result;
}

std::string stateWriteStem(const QueueBlockPlan &block, size_t writeIndex) {
  const StateWritePlan &write = block.stateWrites[writeIndex];
  size_t occurrence = 0;
  for (size_t index = 0; index < writeIndex; ++index)
    occurrence += block.stateWrites[index].table == write.table;
  std::string result = "state_" + identifier(write.table);
  if (occurrence)
    result += "_" + std::to_string(occurrence + 1);
  return result;
}

std::string stateWriteIndexName(const QueueBlockPlan &block,
                                size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_index";
}

std::string stateWriteValueName(const QueueBlockPlan &block,
                                size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_next";
}

std::string stateWritePresentName(const QueueBlockPlan &block,
                                  size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_write_present";
}

std::string stateWriteRefGenerationName(const QueueBlockPlan &block,
                                        size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_ref_generation";
}

std::string stateWriteRefEpochName(const QueueBlockPlan &block,
                                   size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_ref_epoch";
}

std::string stateWriteRefAttemptName(const QueueBlockPlan &block,
                                     size_t writeIndex) {
  return stateWriteStem(block, writeIndex) + "_ref_attempt";
}

std::string stateWriteBatchName(llvm::StringRef table) {
  return "state_" + identifier(table) + "_writes";
}

std::string outputValueName(const QueueBlockPlan &block, size_t outputIndex) {
  const std::string suffix = outputIndex < block.outputs.size()
                                 ? identifier(block.outputs[outputIndex])
                                 : std::to_string(outputIndex);
  return "output_" + suffix;
}

std::string outputPresentName(const QueueBlockPlan &block, size_t outputIndex) {
  return outputValueName(block, outputIndex) + "_present";
}

std::string reservationBindingName(llvm::StringRef table,
                                   const StateReservationPlan &reservation,
                                   size_t reservationIndex) {
  return "state_" + identifier(table) + "_reservation_" +
         (reservation.indexKind == "set" ? "set_" : "index_") +
         std::to_string(reservationIndex);
}

void emitStateWriteBatch(std::ostringstream &output,
                         const QueueBlockPlan &block, llvm::StringRef table,
                         const QueueGraphPlan &plan, llvm::StringRef entryType,
                         size_t ownerIndex, llvm::StringRef padding) {
  (void)ownerIndex;
  const std::string batch = stateWriteBatchName(table);
  output << padding.str() << "gfsim::OwnerWriteBatch<" << entryType.str()
         << "> " << batch << ";\n";
  auto payload =
      llvm::find_if(plan.payloads, [&](const QueuePayloadPlan &candidate) {
        return candidate.name == entryType;
      });
  for (size_t writeIndex : findStateWriteOrdinals(block, table)) {
    const StateWritePlan &write = block.stateWrites[writeIndex];
    uint64_t fieldMask = 0;
    if (write.fields == std::vector<std::string>{"$entry"}) {
      fieldMask = 1;
    } else if (payload != plan.payloads.end()) {
      for (const std::string &field : write.fields) {
        auto declared = llvm::find_if(
            payload->fields, [&](const QueuePayloadFieldPlan &candidate) {
              return candidate.name == field;
            });
        if (declared != payload->fields.end())
          fieldMask |= uint64_t{1} << static_cast<unsigned>(
                           std::distance(payload->fields.begin(), declared));
      }
    }
    output << padding.str() << "if ("
           << stateWritePresentName(block, writeIndex) << ")\n"
           << padding.str() << "  " << batch
           << ".emplace_back(gfsim::TableWriteRecord<" << entryType.str()
           << ">{static_cast<size_t>(" << stateWriteIndexName(block, writeIndex)
           << "), " << stateWriteValueName(block, writeIndex) << ", "
           << (write.mode == "replace" ? "gfsim::TableWriteMode::Replace"
                                       : "gfsim::TableWriteMode::FieldMerge")
           << ", std::uint64_t{" << fieldMask << "}});\n";
  }
}

void emitVersionedWriteQualification(std::ostringstream &output,
                                     const QueueBlockPlan &block,
                                     const QueueGraphPlan &plan,
                                     llvm::StringRef padding) {
  for (auto [writeIndex, write] : llvm::enumerate(block.stateWrites)) {
    if (write.versionedAction.empty() ||
        write.versionedAction == "allocate")
      continue;
    auto table = llvm::find_if(plan.tables, [&](const TablePlan &candidate) {
      return candidate.name == write.table;
    });
    if (table == plan.tables.end())
      continue;
    const std::string stem = stateWriteStem(block, writeIndex);
    output << padding.str() << "const auto &" << stem << "_committed = table_"
           << identifier(table->name) << "->at(static_cast<size_t>("
           << stateWriteIndexName(block, writeIndex) << "));\n"
           << padding.str() << "const bool " << stem << "_requested = "
           << "static_cast<bool>(" << stateWritePresentName(block, writeIndex)
           << ");\n"
           << padding.str() << "const bool " << stem
           << "_identity_match = ";
    if (write.versionedAction == "retain") {
      output << "!static_cast<bool>(" << stem << "_committed."
             << table->validField << ")";
    } else {
      output << "static_cast<bool>(" << stem << "_committed."
             << table->validField << ") && (" << stem << "_committed."
             << table->generationField << " == "
             << stateWriteRefGenerationName(block, writeIndex) << ") && ("
             << stem << "_committed." << table->epochField << " == "
             << stateWriteRefEpochName(block, writeIndex) << ")";
      if (!write.refAttempt.empty())
        output << " && (" << stem << "_committed." << table->attemptField
               << " == " << stateWriteRefAttemptName(block, writeIndex)
               << ")";
    }
    output << ";\n"
           << padding.str() << "const bool " << stem << "_stale = " << stem
           << "_requested && !" << stem << "_identity_match;\n"
           << padding.str() << stateWritePresentName(block, writeIndex)
           << " = " << stem << "_requested && " << stem
           << "_identity_match;\n"
           << padding.str() << "(void)" << stem << "_stale; // "
           << write.staleObligationId << "\n";
  }
}

std::vector<const StateReservationPlan *>
findStateReservations(const QueueBlockPlan &block, llvm::StringRef table) {
  std::vector<const StateReservationPlan *> result;
  for (const StateReservationPlan &reservation : block.stateReservations)
    if (reservation.table == table)
      result.push_back(&reservation);
  return result;
}

std::vector<const TablePlan *> stateOwnerTables(const QueueGraphPlan &plan,
                                                const QueueBlockPlan &block) {
  std::vector<const TablePlan *> result;
  for (const TablePlan &table : plan.tables)
    if (findStateWrite(block, table.name) ||
        !findStateReservations(block, table.name).empty())
      result.push_back(&table);
  return result;
}

std::vector<const TablePlan *> readOnlyTables(const QueueGraphPlan &plan,
                                              const QueueBlockPlan &block) {
  llvm::StringSet<> owned;
  for (const TablePlan *table : stateOwnerTables(plan, block))
    owned.insert(table->name);
  std::vector<const TablePlan *> result;
  for (const TablePlan &table : plan.tables)
    if (!owned.contains(table.name) &&
        referencesTable(block.expressions, table.name))
      result.push_back(&table);
  return result;
}

const SlotPlan *findSlot(const QueueGraphPlan &plan, llvm::StringRef name) {
  auto found =
      std::find_if(plan.slots.begin(), plan.slots.end(),
                   [&](const SlotPlan &slot) { return slot.name == name; });
  return found == plan.slots.end() ? nullptr : &*found;
}

bool isRuntimeBlock(const QueueBlockPlan &block) {
  return block.kind != "source";
}

bool isWriterBlock(const QueueBlockPlan &block) {
  return block.kind == "firing" || block.kind == "table_write" ||
         block.kind == "table_masked_write";
}

std::vector<const QueueBlockPlan *>
arbitrationDispatchOrder(llvm::ArrayRef<const QueueBlockPlan *> blocks) {
  std::vector<const QueueBlockPlan *> ordered(blocks.begin(), blocks.end());
  std::vector<const QueueBlockPlan *> writers;
  for (const QueueBlockPlan *block : blocks)
    if (isWriterBlock(*block))
      writers.push_back(block);
  llvm::sort(writers,
             [](const QueueBlockPlan *left, const QueueBlockPlan *right) {
               return std::tie(left->priority, left->stableId) <
                      std::tie(right->priority, right->stableId);
             });
  size_t nextWriter = 0;
  for (const QueueBlockPlan *&block : ordered)
    if (isWriterBlock(*block))
      block = writers[nextWriter++];
  return ordered;
}

llvm::Error emitStructuredMergePolicy(std::ostringstream &output,
                                      const QueueGraphPlan &specialization,
                                      llvm::StringRef policyName,
                                      llvm::StringRef entryType,
                                      llvm::ArrayRef<std::string> fields) {
  auto payload = llvm::find_if(specialization.payloads,
                               [&](const QueuePayloadPlan &candidate) {
                                 return candidate.name == entryType;
                               });
  const bool scalar = fields.size() == 1 && fields.front() == "$entry";
  const size_t fieldCount = scalar ? 1
                                   : (payload == specialization.payloads.end()
                                          ? 0
                                          : payload->fields.size());
  if (fieldCount == 0 || fieldCount > 64)
    return generatorError("structured specialization Entry fields missing");
  output << "struct " << policyName.str()
         << " {\n  static constexpr size_t fieldCount = " << fieldCount
         << ";\n  static constexpr std::array<size_t, " << fields.size()
         << "> fields{";
  for (auto [fieldIndex, field] : llvm::enumerate(fields)) {
    if (fieldIndex)
      output << ", ";
    if (field == "$entry") {
      output << 0;
      continue;
    }
    if (payload == specialization.payloads.end())
      return generatorError("structured specialization Entry payload missing");
    auto declared = llvm::find_if(payload->fields,
                                  [&](const QueuePayloadFieldPlan &candidate) {
                                    return candidate.name == field;
                                  });
    if (declared == payload->fields.end())
      return generatorError("structured specialization write field missing");
    output << std::distance(payload->fields.begin(), declared);
  }
  output << "};\n  void operator()(" << entryType.str() << " &target, const "
         << entryType.str() << " &value) const {\n";
  for (const std::string &field : fields)
    if (field == "$entry")
      output << "    target = value;\n";
    else
      output << "    target." << identifier(field) << " = value."
             << identifier(field) << ";\n";
  output << "  }\n  void operator()(" << entryType.str() << " &target, const "
         << entryType.str() << " &value, std::uint64_t mask) const {\n";
  if (scalar) {
    output << "    if (mask & std::uint64_t{1}) target = value;\n";
  } else {
    for (auto [fieldIndex, field] : llvm::enumerate(payload->fields))
      output << "    if (mask & (std::uint64_t{1} << " << fieldIndex
             << ")) target." << identifier(field.name) << " = value."
             << identifier(field.name) << ";\n";
  }
  output << "  }\n};\n\n";
  return llvm::Error::success();
}

struct StructuredQueueGraphCpp {
  std::string concatenated;
  struct TypeUnit {
    std::string name;
    std::string definition;
    std::vector<std::string> dependencies;
  };
  std::vector<TypeUnit> types;
  std::string helperDeclarations;
  std::string helperDefinitions;
  struct ModuleUnit {
    std::string fileStem;
    std::string className;
    std::vector<std::string> childFileStems;
    std::string provenance;
    std::string header;
    std::string source;
  };
  std::vector<ModuleUnit> modules;
  std::string rootClass;
};

struct LexicalState {
  bool lineComment = false;
  bool blockComment = false;
  bool stringLiteral = false;
  bool characterLiteral = false;
  bool escaped = false;
};

void advanceLexicalState(llvm::StringRef text, size_t index,
                         LexicalState &state) {
  const char current = text[index];
  const char next = index + 1 < text.size() ? text[index + 1] : '\0';
  if (state.lineComment) {
    if (current == '\n')
      state.lineComment = false;
    return;
  }
  if (state.blockComment) {
    if (current == '*' && next == '/')
      state.blockComment = false;
    return;
  }
  if (state.stringLiteral || state.characterLiteral) {
    if (state.escaped) {
      state.escaped = false;
      return;
    }
    if (current == '\\') {
      state.escaped = true;
      return;
    }
    if ((state.stringLiteral && current == '"') ||
        (state.characterLiteral && current == '\'')) {
      state.stringLiteral = false;
      state.characterLiteral = false;
    }
    return;
  }
  if (current == '/' && next == '/') {
    state.lineComment = true;
    return;
  }
  if (current == '/' && next == '*') {
    state.blockComment = true;
    return;
  }
  if (current == '"')
    state.stringLiteral = true;
  else if (current == '\'')
    state.characterLiteral = true;
}

bool isLexicallyActive(const LexicalState &state) {
  return !state.lineComment && !state.blockComment && !state.stringLiteral &&
         !state.characterLiteral;
}

llvm::Expected<size_t> matchingBrace(llvm::StringRef text, size_t opening) {
  if (opening >= text.size() || text[opening] != '{')
    return generatorError("generated C++ brace scan started out of range");
  LexicalState state;
  unsigned depth = 0;
  for (size_t index = opening; index < text.size(); ++index) {
    const bool active = isLexicallyActive(state);
    if (active && text[index] == '{')
      ++depth;
    else if (active && text[index] == '}' && --depth == 0)
      return index;
    advanceLexicalState(text, index, state);
    if (state.blockComment && text[index] == '*' && index + 1 < text.size() &&
        text[index + 1] == '/') {
      ++index;
      state.blockComment = false;
    } else if (state.lineComment && text[index] == '/' &&
               index + 1 < text.size() && text[index + 1] == '/') {
      ++index;
    }
  }
  return generatorError("generated C++ contains an unbalanced class body");
}

size_t methodSignatureStart(llvm::StringRef prefix) {
  size_t result = 0;
  for (llvm::StringLiteral access :
       {llvm::StringLiteral("public:"), llvm::StringLiteral("private:"),
        llvm::StringLiteral("protected:")}) {
    const size_t found = prefix.rfind(access);
    if (found != llvm::StringRef::npos)
      result = std::max(result, found + access.size());
  }
  return result;
}

std::string qualifyMethodSignature(llvm::StringRef signature,
                                   llvm::StringRef className) {
  std::string result = signature.trim().str();
  const size_t constructor = result.find(className.str() + "(");
  if (constructor != std::string::npos &&
      result.substr(0, constructor).find_first_not_of(" \t\n") ==
          std::string::npos) {
    result.insert(constructor + className.size(), "::" + className.str());
    return result;
  }
  size_t method = result.find("operator");
  if (method == std::string::npos) {
    const size_t open = result.find('(');
    if (open == std::string::npos)
      return result;
    method = open;
    while (method > 0 &&
           std::isspace(static_cast<unsigned char>(result[method - 1])))
      --method;
    while (method > 0 &&
           (std::isalnum(static_cast<unsigned char>(result[method - 1])) ||
            result[method - 1] == '_'))
      --method;
  }
  result.insert(method, className.str() + "::");
  return result;
}

std::string declarationSignature(llvm::StringRef signature,
                                 llvm::StringRef className) {
  llvm::StringRef trimmed = signature.trim();
  if (!trimmed.starts_with(className))
    return trimmed.str();
  const size_t open = trimmed.find('(');
  if (open == llvm::StringRef::npos)
    return trimmed.str();
  unsigned depth = 0;
  for (size_t index = open; index < trimmed.size(); ++index) {
    if (trimmed[index] == '(')
      ++depth;
    else if (trimmed[index] == ')' && --depth == 0)
      return trimmed.take_front(index + 1).str();
  }
  return trimmed.str();
}

llvm::Expected<std::pair<std::string, std::string>>
outlineClass(llvm::StringRef declaration, llvm::StringRef className) {
  const size_t opening = declaration.find('{');
  auto closing = matchingBrace(declaration, opening);
  if (!closing)
    return closing.takeError();
  llvm::StringRef body = declaration.slice(opening + 1, *closing);
  std::string header = declaration.take_front(opening + 1).str();
  std::string source;
  size_t copied = 0;
  size_t memberStart = 0;
  LexicalState state;
  for (size_t index = 0; index < body.size(); ++index) {
    const bool active = isLexicallyActive(state);
    if (active && body[index] == ';') {
      memberStart = index + 1;
    } else if (active && body[index] == '{') {
      llvm::StringRef prefix = body.slice(memberStart, index);
      const size_t signatureOffset = methodSignatureStart(prefix);
      llvm::StringRef signature = prefix.drop_front(signatureOffset);
      auto methodEnd = matchingBrace(body, index);
      if (!methodEnd)
        return methodEnd.takeError();
      size_t following = *methodEnd + 1;
      while (following < body.size() &&
             std::isspace(static_cast<unsigned char>(body[following])))
        ++following;
      const bool initializerBrace =
          following < body.size() &&
          (body[following] == ',' || body[following] == ')' ||
           body[following] == ';');
      if (signature.contains('(') && !initializerBrace) {
        header.append(body.slice(copied, memberStart + signatureOffset).str());
        header.append(declarationSignature(signature, className));
        header.append(";");
        source.append(qualifyMethodSignature(signature, className));
        source.push_back(' ');
        source.append(body.slice(index, *methodEnd + 1).str());
        source.append("\n\n");
        copied = *methodEnd + 1;
        memberStart = copied;
        index = *methodEnd;
        state = {};
        continue;
      }
      index = *methodEnd;
      state = {};
      continue;
    }
    advanceLexicalState(body, index, state);
    if (state.blockComment && body[index] == '*' && index + 1 < body.size() &&
        body[index + 1] == '/') {
      ++index;
      state.blockComment = false;
    } else if (state.lineComment && body[index] == '/' &&
               index + 1 < body.size() && body[index + 1] == '/') {
      ++index;
    }
  }
  header.append(body.drop_front(copied).str());
  header.append(declaration.drop_front(*closing).str());
  return std::pair{std::move(header), std::move(source)};
}

llvm::Expected<std::pair<std::string, std::string>>
outlineModuleBody(llvm::StringRef body) {
  std::string header;
  std::string source;
  size_t cursor = 0;
  while (cursor < body.size()) {
    size_t classPos = body.find("class ", cursor);
    size_t structPos = body.find("struct ", cursor);
    size_t declarationPos = std::min(classPos, structPos);
    if (declarationPos == llvm::StringRef::npos)
      break;
    const size_t nameStart =
        declarationPos + (declarationPos == classPos ? 6 : 7);
    size_t nameEnd = nameStart;
    while (nameEnd < body.size() &&
           (std::isalnum(static_cast<unsigned char>(body[nameEnd])) ||
            body[nameEnd] == '_'))
      ++nameEnd;
    const size_t opening = body.find('{', nameEnd);
    if (opening == llvm::StringRef::npos)
      return generatorError("generated module declaration has no body");
    auto closing = matchingBrace(body, opening);
    if (!closing)
      return closing.takeError();
    size_t declarationEnd = *closing + 1;
    if (declarationEnd < body.size() && body[declarationEnd] == ';')
      ++declarationEnd;
    header.append(body.slice(cursor, declarationPos).str());
    auto outlined = outlineClass(body.slice(declarationPos, declarationEnd),
                                 body.slice(nameStart, nameEnd));
    if (!outlined)
      return outlined.takeError();
    header.append(outlined->first);
    source.append(outlined->second);
    cursor = declarationEnd;
  }
  header.append(body.drop_front(cursor).str());
  return std::pair{std::move(header), std::move(source)};
}

llvm::Expected<StructuredQueueGraphCpp>
generateStructuredQueueGraphCpp(const QueueGraphPlan &plan) {
  if (auto error = verifyQueueGraphPlan(plan))
    return std::move(error);
  std::vector<const QueueGraphPlan *> emissionOrder =
      orderedFamilyCaseBodies(plan);
  if (plan.definition.empty() || emissionOrder.empty() ||
      plan.moduleInstances.empty())
    return generatorError("structured QueueGraph plan is incomplete");

  auto verifyRuntimeInstanceNamespace =
      [](const QueueGraphPlan &candidate) -> llvm::Error {
    for (const QueuePlan &queue : candidate.queues)
      if (llvm::StringRef(queue.name).starts_with("compiler_instance_"))
        return generatorError(
            "Queue name uses the compiler-reserved runtime instance prefix");
    for (const std::string &scope : candidate.scopes) {
      const std::vector<std::string> parts = pathParts(scope);
      if (!parts.empty() &&
          llvm::StringRef(parts.back()).starts_with("compiler_instance_"))
        return generatorError(
            "scope name uses the compiler-reserved runtime instance prefix");
    }
    return llvm::Error::success();
  };
  if (auto error = verifyRuntimeInstanceNamespace(plan))
    return std::move(error);
  for (const QueueGraphPlan *specialization : emissionOrder)
    if (auto error = verifyRuntimeInstanceNamespace(*specialization))
      return std::move(error);

  // A stateless local firing block has no Table writes, reservations, or slot
  // releases, and carries exactly one condition and presence predicate per
  // output. Both the mixed local shapes and the emitter rely on this shape so a
  // stateful firing block is rejected instead of flattened.
  auto isStatelessFiringBlock = [](const QueueBlockPlan &block) {
    return block.kind == "firing" && block.stateWrites.empty() &&
           block.stateReservations.empty() && block.slotReleases.empty() &&
           block.yields.size() == block.outputs.size() &&
           block.outputPresence.size() == block.outputs.size();
  };
  // A parent may own any number of local block subgraphs across its own scopes,
  // either alongside child instances or on its own for a rule-backed body with
  // no child call. Every block has to live in one of the parent's scopes, and
  // the Queues between the local blocks and the children stay parent-owned.
  // This is the generalization of mixedNested from exactly one transform to the
  // whole "local blocks plus child instances" shape. Only single-pass local
  // transforms, fanout broadcasts, selective merges, and stateless firing
  // blocks are representable here: a feedback, select, stateful firing, or
  // table block would otherwise be silently flattened into a one-shot
  // transform.
  //
  // A selective merge (`ac.merge` -> `gfsim::QueueMerge`) is admitted only with
  // the arity its runtime primitive requires: at least two inputs and exactly
  // one output. Any other arity stays rejected so the mixed emitter can never
  // emit a half-formed `QueueMerge`.
  auto isWideMixedLocalShape = [&](const QueueGraphPlan &specialization) {
    return !specialization.blocks.empty() && specialization.tables.empty() &&
           !specialization.scopes.empty() &&
           llvm::all_of(specialization.blocks,
                        [&](const QueueBlockPlan &block) {
                          if (!llvm::is_contained(specialization.scopes,
                                                  block.scope))
                            return false;
                          if (block.kind == "broadcast")
                            return block.inputs.size() == 1 &&
                                   block.outputs.size() >= 2;
                          if (block.kind == "merge")
                            return block.inputs.size() >= 2 &&
                                   block.outputs.size() == 1;
                          if (block.kind == "firing")
                            return isStatelessFiringBlock(block);
                          return block.kind == "transform";
                        });
  };

  for (const QueueGraphPlan *specialization : emissionOrder) {
    const bool pureTransform =
        specialization && specialization->blocks.size() == 1 &&
        specialization->blocks.front().kind == "transform" &&
        specialization->tables.empty() &&
        !specialization->blocks.front().inputs.empty() &&
        !specialization->blocks.front().outputs.empty() &&
        specialization->blocks.front().outputs.size() ==
            specialization->blocks.front().yields.size();
    const bool firingModule =
        specialization && !specialization->blocks.empty() &&
        llvm::all_of(specialization->blocks, [&](const QueueBlockPlan &block) {
          if (block.kind == "slot")
            return block.inputs.size() == 1 && block.outputs.empty() &&
                   block.yields.size() == 1 && !block.slot.empty();
          return block.kind == "firing" &&
                 llvm::all_of(block.stateWrites,
                              [&](const StateWritePlan &write) {
                                return findTable(*specialization,
                                                 write.table) != nullptr;
                              }) &&
                 llvm::all_of(block.stateReservations,
                              [&](const StateReservationPlan &reservation) {
                                return findTable(*specialization,
                                                 reservation.table) != nullptr;
                              }) &&
                 block.yields.size() == block.outputs.size();
        });
    const bool conditionalTransform =
        specialization && specialization->blocks.size() == 1 &&
        specialization->blocks.front().kind == "firing" &&
        specialization->tables.empty() &&
        specialization->interfaceInputs.size() == 1 &&
        specialization->interfaceOutputs.size() == 1 &&
        specialization->blocks.front().inputs.size() == 1 &&
        specialization->blocks.front().outputs.size() == 1 &&
        specialization->blocks.front().stateWrites.empty() &&
        specialization->blocks.front().stateReservations.empty() &&
        specialization->blocks.front().yields.size() == 1;
    const bool emptyModule = specialization && specialization->queues.empty() &&
                             specialization->interfaceInputs.empty() &&
                             specialization->interfaceOutputs.empty() &&
                             specialization->blocks.empty() &&
                             specialization->tables.empty() &&
                             specialization->memoryInstances.empty() &&
                             specialization->moduleInstances.empty() &&
                             specialization->scopes.empty();
    const bool nestedWrapper = specialization &&
                               specialization->blocks.empty() &&
                               specialization->tables.empty() &&
                               !specialization->moduleInstances.empty() &&
                               specialization->scopes.empty();
    const bool mixedNested =
        specialization && specialization->blocks.size() == 1 &&
        specialization->blocks.front().kind == "transform" &&
        specialization->tables.empty() &&
        !specialization->moduleInstances.empty() &&
        specialization->scopes.size() == 1 &&
        specialization->blocks.front().scope == specialization->scopes.front();
    const bool nestedAssembly =
        specialization && !specialization->moduleInstances.empty() &&
        !specialization->blocks.empty() && specialization->tables.empty() &&
        specialization->memoryInstances.empty() &&
        llvm::all_of(specialization->blocks, [](const QueueBlockPlan &block) {
          return block.kind == "broadcast";
        });
    const bool wideMixedLocal =
        specialization && isWideMixedLocalShape(*specialization);
    const bool wideMixedNested =
        wideMixedLocal && !specialization->moduleInstances.empty();
    // A childless rule-backed body may own the same multi-block local shape. A
    // body with a single local block keeps the older dedicated emitters so its
    // generated output stays byte-identical, and an all-firing body keeps the
    // direct-interface stateful emitter.
    const bool multiBlockLocal =
        wideMixedLocal && specialization->moduleInstances.empty() &&
        specialization->blocks.size() > 1 &&
        llvm::any_of(specialization->blocks, [](const QueueBlockPlan &block) {
          return block.kind != "firing" && block.kind != "slot";
        });
    const bool localShape =
        emptyModule || nestedWrapper || mixedNested || nestedAssembly ||
        wideMixedNested || multiBlockLocal ||
        (specialization && specialization->moduleInstances.empty() &&
         specialization->scopes.size() == 1 &&
         llvm::none_of(specialization->blocks,
                       [&](const QueueBlockPlan &block) {
                         return block.scope != specialization->scopes.front();
                       }));
    // The local block graph is not representable by any admitted shape, so
    // report the specific unsupported block instead of silently flattening it
    // into a one-shot transform.
    const bool localBlockGraph =
        specialization && !specialization->blocks.empty() &&
        !specialization->scopes.empty() &&
        llvm::all_of(specialization->blocks,
                     [&](const QueueBlockPlan &block) {
                       return llvm::is_contained(specialization->scopes,
                                                 block.scope);
                     });
    if (localBlockGraph && !wideMixedLocal && !nestedWrapper &&
        !nestedAssembly && !mixedNested && !firingModule && !pureTransform &&
        !conditionalTransform &&
        (!specialization->moduleInstances.empty() ||
         specialization->blocks.size() > 1)) {
      for (const QueueBlockPlan &block : specialization->blocks) {
        if (block.kind == "firing" && !isStatelessFiringBlock(block))
          return generatorError(
              "mixed nested module supports only stateless local firing "
              "blocks; firing block '" +
              block.name + "' owns Table state");
        if (block.kind == "merge") {
          // A merge with the supported arity is not the offending block here;
          // some sibling shape is. Only a malformed merge is reported.
          if (block.inputs.size() >= 2 && block.outputs.size() == 1)
            continue;
          return generatorError(
              "mixed nested module merge block requires at least two input "
              "Queues and exactly one output Queue; block '" +
              block.name + "' has " + std::to_string(block.inputs.size()) +
              " input(s) and " + std::to_string(block.outputs.size()) +
              " output(s)");
        }
        if (block.kind != "transform" && block.kind != "broadcast" &&
            block.kind != "firing")
          return generatorError(
              "mixed nested module supports only local transform, fanout "
              "broadcast, selective merge, and stateless firing blocks; "
              "block '" +
              block.name + "' has kind '" + block.kind + "'");
      }
    }
    if (!specialization ||
        (!emptyModule && !pureTransform && !conditionalTransform &&
         !firingModule && !nestedWrapper && !mixedNested && !nestedAssembly &&
         !wideMixedNested && !multiBlockLocal) ||
        !localShape || !specialization->memoryInstances.empty())
      return generatorError(
          "structured QueueGraph specialization requires a pure transform, "
          "direct-interface firing module, or "
          "direct nested wrapper");
    llvm::StringSet<> interfaceQueues;
    for (const QueueInterfacePlan &input : specialization->interfaceInputs)
      interfaceQueues.insert(input.name);
    for (const QueueInterfacePlan &output : specialization->interfaceOutputs)
      interfaceQueues.insert(output.name);
    if (mixedNested || nestedAssembly || wideMixedLocal)
      for (const QueuePlan &queue : specialization->queues)
        interfaceQueues.insert(queue.name);
    for (const QueueBlockPlan &block : specialization->blocks)
      if (llvm::any_of(block.inputs,
                       [&](const std::string &name) {
                         return !interfaceQueues.contains(name);
                       }) ||
          llvm::any_of(block.outputs, [&](const std::string &name) {
            return !interfaceQueues.contains(name);
          }))
        return generatorError(
            "first multi-rule specialization slice requires direct interface "
            "Queue bindings");
  }

  for (const QueueBlockPlan &block : plan.blocks)
    if (block.kind != "source" && block.kind != "broadcast" &&
        block.kind != "sink" && block.kind != "observe")
      return generatorError(
          "first structured QueueGraph root supports source, broadcast, "
          "sink, and observe blocks");

  llvm::StringSet<> resolvedClassNames;
  for (const QueueGraphPlan *specialization : emissionOrder) {
    std::string resolved = className(specialization->sourceDefinition);
    if (!resolvedClassNames.insert(resolved).second)
      return generatorError(
          "unsupported C++ parameter family shape: one readable class name "
          "would denote incompatible family cases");
  }
  auto specializationClassName =
      [&](const QueueGraphPlan &specialization) -> std::string {
    return className(specialization.sourceDefinition);
  };
  llvm::StringMap<std::string> portableFileDefinitions;
  for (const QueueGraphPlan *specialization : emissionOrder) {
    const std::string fileStem = sourceStem(*specialization);
    std::string portableKey = fileStem;
    for (char &character : portableKey)
      if (character >= 'A' && character <= 'Z')
        character = static_cast<char>(character - 'A' + 'a');
    auto [entry, inserted] = portableFileDefinitions.try_emplace(
        portableKey, specialization->sourceFile);
    if (!inserted && entry->getValue() != specialization->sourceFile)
      return generatorError(
          "portable source-stem collision: rename one implementation source");
  }
  auto specializationFileStem =
      [&](const QueueGraphPlan &specialization) -> std::string {
    return sourceStem(specialization);
  };
  llvm::DenseMap<const QueueGraphPlan *, uint64_t> specializationObjectCounts;
  std::function<llvm::Expected<uint64_t>(const QueueGraphPlan &)>
      computeObjectCount = [&](const QueueGraphPlan &specialization)
      -> llvm::Expected<uint64_t> {
    if (auto found = specializationObjectCounts.find(&specialization);
        found != specializationObjectCounts.end())
      return found->second;
    llvm::StringSet<> interfaceQueues;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      interfaceQueues.insert(input.name);
    for (const QueueInterfacePlan &output : specialization.interfaceOutputs)
      interfaceQueues.insert(output.name);
    uint64_t count =
        specialization.blocks.size() + specialization.tables.size() +
        llvm::count_if(specialization.queues, [&](const QueuePlan &queue) {
          return !interfaceQueues.contains(queue.name);
        });
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances) {
      const QueueGraphPlan *childPlan = findFamilyCaseBody(
          plan, instance.definition, instance.staticArguments);
      if (!childPlan)
        return generatorError(
            "nested family case object count dependency is unavailable");
      auto child = computeObjectCount(*childPlan);
      if (!child)
        return child.takeError();
      count += *child;
    }
    specializationObjectCounts[&specialization] = count;
    return count;
  };
  for (const QueueGraphPlan *specialization : emissionOrder) {
    auto count = computeObjectCount(*specialization);
    if (!count)
      return count.takeError();
  }
  auto specializationObjectCount = [&](const QueueGraphPlan &specialization) {
    return specializationObjectCounts.lookup(&specialization);
  };
  auto specializationInternalQueueCount =
      [](const QueueGraphPlan &specialization) -> uint64_t {
    llvm::StringSet<> interfaceQueues;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      interfaceQueues.insert(input.name);
    for (const QueueInterfacePlan &output : specialization.interfaceOutputs)
      interfaceQueues.insert(output.name);
    return llvm::count_if(specialization.queues, [&](const QueuePlan &queue) {
      return !interfaceQueues.contains(queue.name);
    });
  };
  auto queueRuntimeNames = [](const QueueGraphPlan &specialization) {
    llvm::StringSet<> occupied;
    for (const QueuePlan &queue : specialization.queues)
      occupied.insert(queue.name);
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances)
      occupied.insert(identifier(instance.name));
    llvm::StringMap<std::string> names;
    for (const QueuePlan &queue : specialization.queues) {
      std::string candidate = queue.name + "_queue";
      while (occupied.contains(candidate))
        candidate += "_queue";
      occupied.insert(candidate);
      names[queue.name] = std::move(candidate);
    }
    return names;
  };

  llvm::StringMap<std::string> queueMembers;
  llvm::StringMap<std::string> queueOwners;
  llvm::StringMap<uint64_t> queueIds;
  for (auto [index, queue] : llvm::enumerate(plan.queues)) {
    queueMembers[queue.name] = identifier(queue.name) + "_";
    queueOwners[queue.name] = queue.scope;
    queueIds[queue.name] = index;
  }
  for (const QueueBlockPlan &block : plan.blocks)
    for (const std::string &input : block.inputs)
      queueOwners[input] = commonPath(queueOwners[input], block.scope);
  for (const QueueModuleInstancePlan &instance : plan.moduleInstances)
    for (const std::string &input : instance.inputs)
      queueOwners[input] = commonPath(queueOwners[input], instance.scope);

  llvm::StringMap<std::string> scopeMembers;
  for (auto [index, scope] : llvm::enumerate(plan.scopes))
    scopeMembers[scope] = "scope_" + std::to_string(index) + "_";
  auto modulePointer =
      [&](llvm::StringRef path) -> llvm::Expected<std::string> {
    if (path == "/")
      return std::string("this");
    auto found = scopeMembers.find(path);
    if (found == scopeMembers.end())
      return generatorError("unknown structured scope path '" + path + "'");
    return "&" + found->getValue();
  };
  auto attach = [&](llvm::StringRef path,
                    llvm::StringRef member) -> llvm::Expected<std::string> {
    if (path == "/")
      return "    attachChild(" + member.str() + ");";
    auto found = scopeMembers.find(path);
    if (found == scopeMembers.end())
      return generatorError("unknown structured attachment scope '" + path +
                            "'");
    return "    " + found->getValue() + ".attachChild(" + member.str() + ");";
  };

  std::vector<const QueueBlockPlan *> runtimeBlocks;
  for (const QueueBlockPlan &block : plan.blocks)
    if (isRuntimeBlock(block))
      runtimeBlocks.push_back(&block);
  struct DispatchItem {
    uint64_t lexicalOrder = 0;
    const QueueBlockPlan *block = nullptr;
    size_t instanceIndex = 0;
  };
  std::vector<DispatchItem> dispatchItems;
  for (const QueueBlockPlan *block : runtimeBlocks)
    dispatchItems.push_back({block->lexicalOrder, block, 0});
  for (auto [index, instance] : llvm::enumerate(plan.moduleInstances))
    dispatchItems.push_back({instance.lexicalOrder, nullptr, index});
  llvm::sort(dispatchItems,
             [](const DispatchItem &left, const DispatchItem &right) {
               return left.lexicalOrder < right.lexicalOrder;
             });
  uint64_t nextId = plan.queues.size();
  llvm::DenseMap<const QueueBlockPlan *, uint64_t> blockIds;
  std::vector<std::vector<uint64_t>> instanceObjectIds(
      plan.moduleInstances.size());
  for (const DispatchItem &item : dispatchItems) {
    if (item.block)
      blockIds[item.block] = nextId++;
    else {
      const QueueModuleInstancePlan &instance =
          plan.moduleInstances[item.instanceIndex];
      const QueueGraphPlan *specialization =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      for (uint64_t index = 0;
           index < specializationObjectCount(*specialization); ++index)
        instanceObjectIds[item.instanceIndex].push_back(nextId++);
    }
  }

  auto resolveRootActivation =
      [&](const QueueActivationNodePlan &node) -> llvm::Expected<uint64_t> {
    if (node.kind == QueueActivationNodeKind::Queue) {
      if (node.index >= plan.queues.size())
        return generatorError("root activation Queue index is out of range");
      return queueIds.lookup(plan.queues[node.index].name);
    }
    if (node.kind == QueueActivationNodeKind::Block) {
      if (node.index >= plan.blocks.size())
        return generatorError("root activation block index is out of range");
      auto found = blockIds.find(&plan.blocks[node.index]);
      if (found == blockIds.end())
        return generatorError("root activation block has no runtime object");
      return found->second;
    }
    if (node.kind == QueueActivationNodeKind::Slot) {
      if (node.index >= plan.slots.size())
        return generatorError("root activation slot index is out of range");
      const std::string &name = plan.slots[node.index].name;
      auto block =
          llvm::find_if(plan.blocks, [&](const QueueBlockPlan &candidate) {
            return candidate.kind == "slot" && candidate.slot == name;
          });
      if (block == plan.blocks.end())
        return generatorError("root activation slot has no runtime object");
      auto found = blockIds.find(&*block);
      if (found == blockIds.end())
        return generatorError("root activation slot block is not dispatched");
      return found->second;
    }
    return generatorError(
        "structured root activation contains an unsupported node kind");
  };
  std::set<std::pair<uint64_t, uint64_t>> activationEdges;
  std::set<std::pair<uint64_t, uint64_t>> physicalWorkClosureEdges;
  std::set<uint64_t> initialActivation;
  const bool activationComplete = true;
  for (const QueueActivationEdgePlan &edge : plan.activationEdges) {
    auto source = resolveRootActivation(edge.source);
    auto target = resolveRootActivation(edge.target);
    if (!source)
      return source.takeError();
    if (!target)
      return target.takeError();
    activationEdges.emplace(*source, *target);
  }
  for (const QueueActivationNodePlan &node : plan.initialActivation) {
    auto resolved = resolveRootActivation(node);
    if (!resolved)
      return resolved.takeError();
    initialActivation.insert(*resolved);
  }
  for (const QueueActivationEdgePlan &edge : plan.workClosureEdges) {
    auto source = resolveRootActivation(edge.source);
    auto target = resolveRootActivation(edge.target);
    if (!source)
      return source.takeError();
    if (!target)
      return target.takeError();
    physicalWorkClosureEdges.emplace(*source, *target);
  }
  auto instantiateActivation =
      [&](auto &&self, const QueueGraphPlan &specialization,
          llvm::ArrayRef<uint64_t> objectIds,
          const llvm::StringMap<uint64_t> &interfaceBindings) -> llvm::Error {
    llvm::StringSet<> exported;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      exported.insert(input.name);
    for (const QueueInterfacePlan &output : specialization.interfaceOutputs)
      exported.insert(output.name);
    std::vector<const QueuePlan *> internalQueues;
    llvm::StringMap<uint64_t> internalQueueIndices;
    for (const QueuePlan &queue : specialization.queues) {
      if (exported.contains(queue.name))
        continue;
      internalQueueIndices[queue.name] = internalQueues.size();
      internalQueues.push_back(&queue);
    }
    const uint64_t blockOffset = internalQueues.size();
    const uint64_t tableOffset = blockOffset + specialization.blocks.size();
    uint64_t childOffset = tableOffset + specialization.tables.size();
    if (childOffset > objectIds.size())
      return generatorError(
          "activation specialization object layout is incomplete");
    auto resolveQueue = [&](llvm::StringRef name) -> llvm::Expected<uint64_t> {
      auto interface = interfaceBindings.find(name);
      if (interface != interfaceBindings.end())
        return interface->getValue();
      auto internal = internalQueueIndices.find(name);
      if (internal == internalQueueIndices.end() ||
          internal->getValue() >= objectIds.size())
        return generatorError("activation Queue binding is unresolved");
      return objectIds[internal->getValue()];
    };
    auto resolveNode =
        [&](const QueueActivationNodePlan &node) -> llvm::Expected<uint64_t> {
      if (node.kind == QueueActivationNodeKind::InterfaceInput) {
        if (node.index >= specialization.interfaceInputs.size())
          return generatorError(
              "activation interface input index is out of range");
        return resolveQueue(specialization.interfaceInputs[node.index].name);
      }
      if (node.kind == QueueActivationNodeKind::InterfaceOutput) {
        if (node.index >= specialization.interfaceOutputs.size())
          return generatorError(
              "activation interface output index is out of range");
        return resolveQueue(specialization.interfaceOutputs[node.index].name);
      }
      if (node.kind == QueueActivationNodeKind::Queue) {
        if (node.index >= specialization.queues.size())
          return generatorError("activation Queue index is out of range");
        return resolveQueue(specialization.queues[node.index].name);
      }
      if (node.kind == QueueActivationNodeKind::Block) {
        const uint64_t local = blockOffset + node.index;
        if (node.index >= specialization.blocks.size() ||
            local >= objectIds.size())
          return generatorError("activation block index is out of range");
        return objectIds[local];
      }
      if (node.kind == QueueActivationNodeKind::Table) {
        const uint64_t local = tableOffset + node.index;
        if (node.index >= specialization.tables.size() ||
            local >= objectIds.size())
          return generatorError("activation Table index is out of range");
        return objectIds[local];
      }
      if (node.kind == QueueActivationNodeKind::Slot) {
        if (node.index >= specialization.slots.size())
          return generatorError("activation slot index is out of range");
        const std::string &name = specialization.slots[node.index].name;
        auto block = llvm::find_if(
            specialization.blocks, [&](const QueueBlockPlan &candidate) {
              return candidate.kind == "slot" && candidate.slot == name;
            });
        if (block == specialization.blocks.end())
          return generatorError("activation slot block is unresolved");
        const uint64_t ordinal =
            static_cast<uint64_t>(block - specialization.blocks.begin());
        const uint64_t local = blockOffset + ordinal;
        if (local >= objectIds.size())
          return generatorError("activation slot object is out of range");
        return objectIds[local];
      }
      return generatorError("activation node kind is unsupported");
    };
    for (const QueueActivationEdgePlan &edge : specialization.activationEdges) {
      auto source = resolveNode(edge.source);
      auto target = resolveNode(edge.target);
      if (!source)
        return source.takeError();
      if (!target)
        return target.takeError();
      activationEdges.emplace(*source, *target);
    }
    for (const QueueActivationEdgePlan &edge :
         specialization.workClosureEdges) {
      auto source = resolveNode(edge.source);
      auto target = resolveNode(edge.target);
      if (!source)
        return source.takeError();
      if (!target)
        return target.takeError();
      physicalWorkClosureEdges.emplace(*source, *target);
    }
    for (const QueueActivationNodePlan &node :
         specialization.initialActivation) {
      auto resolved = resolveNode(node);
      if (!resolved)
        return resolved.takeError();
      initialActivation.insert(*resolved);
    }
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        return generatorError("nested activation specialization is missing");
      const uint64_t childCount = specializationObjectCount(*child);
      if (childOffset + childCount > objectIds.size())
        return generatorError("nested activation ID partition is incomplete");
      llvm::StringMap<uint64_t> childBindings;
      for (auto [index, input] : llvm::enumerate(instance.inputs)) {
        auto resolved = resolveQueue(input);
        if (!resolved)
          return resolved.takeError();
        childBindings[child->interfaceInputs[index].name] = *resolved;
      }
      for (auto [index, outputName] : llvm::enumerate(instance.outputs)) {
        auto resolved = resolveQueue(outputName);
        if (!resolved)
          return resolved.takeError();
        childBindings[child->interfaceOutputs[index].name] = *resolved;
      }
      if (auto error =
              self(self, *child, objectIds.slice(childOffset, childCount),
                   childBindings))
        return error;
      childOffset += childCount;
    }
    if (childOffset != objectIds.size())
      return generatorError("activation ID partition has unused objects");
    return llvm::Error::success();
  };
  for (auto [instanceIndex, instance] : llvm::enumerate(plan.moduleInstances)) {
    const QueueGraphPlan *specialization =
        findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
    if (!specialization)
      return generatorError("activation specialization is missing");
    llvm::StringMap<uint64_t> bindings;
    for (auto [index, input] : llvm::enumerate(instance.inputs))
      bindings[specialization->interfaceInputs[index].name] =
          queueIds.lookup(input);
    for (auto [index, outputName] : llvm::enumerate(instance.outputs))
      bindings[specialization->interfaceOutputs[index].name] =
          queueIds.lookup(outputName);
    if (auto error = instantiateActivation(
            instantiateActivation, *specialization,
            llvm::ArrayRef(instanceObjectIds[instanceIndex]), bindings))
      return std::move(error);
  }
  std::vector<uint32_t> activationOffsets(nextId + 1, 0);
  std::vector<uint64_t> activationTargets;
  activationTargets.reserve(activationEdges.size());
  for (auto [source, target] : activationEdges) {
    if (source >= nextId || target >= nextId)
      return generatorError("activation endpoint is outside dispatch rows");
    ++activationOffsets[source + 1];
    activationTargets.push_back(target);
  }
  for (size_t index = 1; index < activationOffsets.size(); ++index)
    activationOffsets[index] += activationOffsets[index - 1];
  std::vector<uint32_t> workClosureOffsets(nextId + 1, 0);
  std::vector<uint64_t> workClosureTargets;
  workClosureTargets.reserve(physicalWorkClosureEdges.size());
  for (auto [worker, resource] : physicalWorkClosureEdges) {
    if (worker >= nextId || resource >= nextId)
      return generatorError("Work closure endpoint is outside dispatch rows");
    ++workClosureOffsets[worker + 1];
    workClosureTargets.push_back(resource);
  }
  for (size_t index = 1; index < workClosureOffsets.size(); ++index)
    workClosureOffsets[index] += workClosureOffsets[index - 1];

  std::ostringstream output;
  output << "// Generated from hierarchy-preserving verified ACIR QueueGraph "
            "plan; do not edit.\n"
            "#include \"gfsim/bits.h\"\n"
            "#include \"gfsim/dispatch.h\"\n"
            "#include \"gfsim/object.h\"\n"
            "#include \"gfsim/priority_encode.h\"\n"
            "#include \"gfsim/queue.h\"\n"
            "#include \"gfsim/queue_blocks.h\"\n\n"
            "#include <array>\n#include <cstdint>\n#include <limits>\n"
            "#include <memory>\n#include <optional>\n#include <string>\n#include <tuple>\n"
            "#include <stdexcept>\n#include <string_view>\n#include <utility>\n"
            "#include <vector>\n\n"
            "namespace ac_generated {\n\n"
            "template <typename T>\n"
            "gfsim::SimQueue<T> &require_queue_port(gfsim::SimQueue<T> *queue, "
            "std::string_view name) {\n"
            "  if (!queue) throw std::invalid_argument(std::string{name} + "
            "\" Queue port is null\");\n"
            "  return *queue;\n"
            "}\n\n";
  std::vector<StructuredQueueGraphCpp::TypeUnit> typeUnits;
  llvm::StringSet<> typePaths;
  llvm::StringMap<std::string> typeHeaderNames;
  auto registerType = [&](llvm::StringRef name) -> llvm::Error {
    const std::string path = identifier(name);
    if (!typePaths.insert(path).second)
      return generatorError("nominal type header paths collide after "
                            "identifier sanitization");
    typeHeaderNames[name] = path;
    return llvm::Error::success();
  };
  for (const QueueEnumPlan &enumeration : plan.enums)
    if (auto error = registerType(enumeration.name))
      return std::move(error);
  for (const QueuePayloadPlan &payload : plan.payloads)
    if (auto error = registerType(payload.name))
      return std::move(error);
  for (const QueueEnumPlan &enumeration : plan.enums) {
    const std::string typeName = typeHeaderNames.lookup(enumeration.name);
    std::ostringstream definition;
    definition << "enum class " << enumeration.name << " : "
               << enumStorage(enumeration.width).str() << " {\n";
    for (auto [index, enumerant] : llvm::enumerate(enumeration.enumerants))
      definition << "  " << enumerant << " = "
                 << (enumeration.values.empty() ? index
                                                : enumeration.values[index])
                 << ",\n";
    definition << "};\n\n";
    output << definition.str();
    typeUnits.push_back({typeName, definition.str(), {}});
  }
  auto payloadOrder = payloadEmissionOrder(plan);
  if (!payloadOrder)
    return payloadOrder.takeError();
  for (const QueuePayloadPlan *payload : *payloadOrder) {
    const std::string typeName = typeHeaderNames.lookup(payload->name);
    std::ostringstream definition;
    std::vector<std::string> dependencies;
    llvm::StringSet<> seenDependencies;
    definition << "struct " << payload->name << " {\n";
    for (const QueuePayloadFieldPlan &field : payload->fields) {
      auto type = cppPayloadFieldType(plan, field);
      if (!type)
        return type.takeError();
      std::optional<llvm::StringRef> dependency = structTypeName(field.type);
      if (!dependency)
        dependency = enumTypeName(field.type);
      if (dependency) {
        const std::string header = typeHeaderNames.lookup(*dependency);
        if (header.empty())
          return generatorError("nominal payload field type has no generated "
                                "header");
        if (seenDependencies.insert(header).second)
          dependencies.push_back(header);
      }
      definition << "  " << *type << ' ' << identifier(field.name) << "{};\n";
    }
    definition << "  bool operator==(const " << payload->name
               << " &) const = default;\n};\n\n";
    output << definition.str();
    typeUnits.push_back({typeName, definition.str(), std::move(dependencies)});
  }

  std::ostringstream helperOutput;
  std::ostringstream helperDeclarations;
  std::ostringstream helperDefinitions;
  helperDeclarations
      << "template <typename T>\n"
         "gfsim::SimQueue<T> &require_queue_port(gfsim::SimQueue<T> *queue, "
         "std::string_view name) {\n"
         "  if (!queue) throw std::invalid_argument(std::string{name} + "
         "\" Queue port is null\");\n"
         "  return *queue;\n"
         "}\n\n";
  llvm::StringSet<> emittedHelpers;
  llvm::StringSet<> declaredHelpers;
  llvm::StringSet<> definedHelpers;
  for (const QueueGraphPlan *specialization : emissionOrder)
    if (auto error = emitHelperDefinitions(helperOutput, *specialization,
                                           &emittedHelpers))
      return std::move(error);
    else if (auto error = emitHelperDefinitions(
                 helperDeclarations, *specialization, &declaredHelpers,
                 HelperEmission::Declarations))
      return std::move(error);
    else if (auto error = emitHelperDefinitions(
                 helperDefinitions, *specialization, &definedHelpers,
                 HelperEmission::Definitions))
      return std::move(error);
  if (auto error = emitHelperDefinitions(helperOutput, plan, &emittedHelpers))
    return std::move(error);
  if (auto error =
          emitHelperDefinitions(helperDeclarations, plan, &declaredHelpers,
                                HelperEmission::Declarations))
    return std::move(error);
  if (auto error =
          emitHelperDefinitions(helperDefinitions, plan, &definedHelpers,
                                HelperEmission::Definitions))
    return std::move(error);
  output << helperOutput.str();
  struct ModuleSpan {
    std::string fileStem;
    std::string className;
    std::vector<std::string> childFileStems;
    std::string provenance;
    std::size_t begin = 0;
    std::size_t end = 0;
  };
  std::vector<ModuleSpan> moduleSpans;

  auto emitStatefulSpecialization =
      [&](const QueueGraphPlan &specialization,
          const std::string &implementation) -> llvm::Error {
    const std::string localScope =
        pathParts(specialization.scopes.front()).back();
    std::vector<std::string> tableTypes;
    std::vector<std::string> tableMembers;
    llvm::StringSet<> usedTableMembers;
    llvm::StringMap<size_t> tableIndices;
    for (auto [index, table] : llvm::enumerate(specialization.tables)) {
      if (table.ownerPath != specialization.scopes.front())
        return generatorError(
            "stateful specialization Tables must share the firing scope");
      auto type = cppType(table.entryType);
      if (!type)
        return type.takeError();
      tableIndices[table.name] = index;
      tableTypes.push_back(std::move(*type));
      tableMembers.push_back(
          uniqueIdentifier("state_" + table.name, usedTableMembers) + "_");
    }
    std::vector<std::string> slotTypes;
    llvm::StringMap<size_t> slotIndices;
    for (auto [index, slot] : llvm::enumerate(specialization.slots)) {
      auto type = cppType(slot.payloadType);
      if (!type)
        return type.takeError();
      slotIndices[slot.name] = index;
      slotTypes.push_back(std::move(*type));
    }
    llvm::StringMap<std::string> portTypes;
    llvm::StringMap<std::string> portParameters =
        interfaceParameterNames(specialization);
    for (const QueueInterfacePlan &input : specialization.interfaceInputs) {
      auto type = cppQueueType(plan, input);
      if (!type)
        return type.takeError();
      portTypes[input.name] = *type;
    }
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs) {
      auto type = cppQueueType(plan, result);
      if (!type)
        return type.takeError();
      portTypes[result.name] = *type;
    }
    std::vector<std::string> firingSymbols;
    llvm::StringSet<> usedFiringSymbols;
    for (const QueueBlockPlan &firing : specialization.blocks) {
      llvm::StringRef display = firing.displayRuleName.empty()
                                    ? llvm::StringRef(firing.name)
                                    : llvm::StringRef(firing.displayRuleName);
      firingSymbols.push_back(
          uniqueIdentifier(("rule_" + display).str(), usedFiringSymbols));
    }
    auto policyName = [&](size_t blockIndex) {
      return implementation + "_" + firingSymbols[blockIndex] + "_policy";
    };
    auto mergeName = [&](size_t blockIndex, size_t writeIndex) {
      return implementation + "_" + firingSymbols[blockIndex] +
             "_merge_policy_" + std::to_string(writeIndex);
    };
    auto queueTypes = [&](llvm::ArrayRef<std::string> queues) {
      std::vector<std::string> result;
      for (const std::string &queue : queues)
        result.push_back(portTypes.lookup(queue));
      return result;
    };
    auto tableBindings = [&](const QueueBlockPlan &firing) {
      std::vector<size_t> result;
      for (const TablePlan *table : stateOwnerTables(specialization, firing))
        result.push_back(tableIndices.lookup(table->name));
      return result;
    };
    auto readOnlyTableBindings = [&](const QueueBlockPlan &firing) {
      std::vector<size_t> result;
      for (const TablePlan *table : readOnlyTables(specialization, firing))
        result.push_back(tableIndices.lookup(table->name));
      return result;
    };
    auto tupleType = [](llvm::ArrayRef<std::string> types) {
      std::string result = "std::tuple<";
      for (auto [index, type] : llvm::enumerate(types)) {
        if (index)
          result.append(", ");
        result.append(type);
      }
      result.push_back('>');
      return result;
    };

    for (auto [blockIndex, firing] : llvm::enumerate(specialization.blocks)) {
      if (firing.kind == "slot") {
        if (firing.yields.size() != 1)
          return generatorError("structured slot release policy is malformed");
        output << "struct " << policyName(blockIndex) << " {\n";
        for (auto [slotIndex, slot] : llvm::enumerate(specialization.slots))
          output << "  const gfsim::SlotState<" << slotTypes[slotIndex]
                 << "> *slot_" << identifier(slot.name) << "{};\n";
        output << "  bool operator()(gfsim::Epoch epoch) const {\n";
        auto body = emitExpressionBody(specialization, firing,
                                       firing.yields.front(), 4, true);
        if (!body)
          return body.takeError();
        output << *body << "  }\n};\n\n";
        continue;
      }
      const std::vector<size_t> tables = tableBindings(firing);
      const std::vector<size_t> readOnlyTables = readOnlyTableBindings(firing);
      std::vector<std::string> writeTypes;
      for (size_t table : tables)
        writeTypes.push_back(tableTypes[table]);
      const std::vector<std::string> inputTypes = queueTypes(firing.inputs);
      const std::vector<std::string> outputTypes = queueTypes(firing.outputs);
      const bool oneOwner = tables.size() == 1 && firing.slotReleases.empty();

      QueueBlockPlan evaluation = firing;
      std::vector<std::string> additional{firing.guard};
      std::string tupleResult = "std::tuple{";
      bool tupleHasValue = false;
      for (auto [writeIndex, write] : llvm::enumerate(firing.stateWrites)) {
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(write.index)
            .append(", ")
            .append(write.value)
            .append(", ")
            .append(write.present);
        tupleHasValue = true;
        additional.push_back(write.index);
        additional.push_back(write.value);
        additional.push_back(write.present);
        if (!write.versionedAction.empty()) {
          tupleResult.append(", ").append(write.refGeneration)
              .append(", ")
              .append(write.refEpoch);
          additional.push_back(write.refGeneration);
          additional.push_back(write.refEpoch);
          if (!write.refAttempt.empty()) {
            tupleResult.append(", ").append(write.refAttempt);
            additional.push_back(write.refAttempt);
          }
        }
      }
      for (auto [outputIndex, yield] : llvm::enumerate(firing.yields)) {
        const std::string &present = firing.outputPresence[outputIndex].present;
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(yield).append(", ").append(present);
        tupleHasValue = true;
        additional.push_back(yield);
        additional.push_back(present);
      }
      for (auto [ownerIndex, tableIndex] : llvm::enumerate(tables)) {
        const TablePlan &table = specialization.tables[tableIndex];
        size_t reservationIndex = 0;
        for (const StateReservationPlan *reservation :
             findStateReservations(firing, table.name)) {
          if (reservation->indexKind == "all") {
            ++reservationIndex;
            continue;
          }
          if (reservation->indexKind == "set") {
            const std::string result = "snapshot_set_" +
                                       std::to_string(ownerIndex) + "_" +
                                       std::to_string(reservationIndex++);
            auto fieldMask = reservationFieldMask(specialization, table,
                                                  reservation->fields);
            if (!fieldMask)
              return fieldMask.takeError();
            QueueExpressionPlan expression{
                result, "snapshot_set", "state_reservation", {}};
            expression.field = reservation->source;
            expression.table = reservation->table;
            expression.predicate = fieldMask->complete ? "complete" : "fields";
            expression.mask = std::to_string(fieldMask->mask);
            expression.width = fieldMask->count;
            evaluation.expressions.push_back(std::move(expression));
            tupleResult.append(", ").append(result);
            additional.push_back(result);
            continue;
          }
          ++reservationIndex;
          if (tupleHasValue)
            tupleResult.append(", ");
          tupleResult.append(reservation->index);
          tupleHasValue = true;
          additional.push_back(reservation->index);
        }
      }
      for (const SlotReleaseEffectPlan &release : firing.slotReleases) {
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(release.when);
        tupleHasValue = true;
        additional.push_back(release.when);
      }
      tupleResult.append(", ").append(firing.guard).push_back('}');
      const std::string &primaryValue =
          !firing.stateWrites.empty() ? firing.stateWrites.front().index
          : !firing.yields.empty()    ? firing.yields.front()
                                      : firing.guard;
      auto body = emitExpressionBody(specialization, evaluation, primaryValue,
                                     6, true, false, additional, tupleResult);
      if (!body)
        return body.takeError();

      std::string planType;
      if (oneOwner) {
        planType = "gfsim::TableTransitionPlan<" + writeTypes.front();
        for (const std::string &type : outputTypes)
          planType.append(", ").append(type);
        planType.push_back('>');
      } else {
        planType = "gfsim::StateTransitionPlan<" + tupleType(writeTypes) +
                   ", " + tupleType(outputTypes) + ">";
      }
      emitRuleProvenance(output, firing);
      output << "struct " << policyName(blockIndex) << " {\n";
      for (size_t table : readOnlyTables)
        output << "  const gfsim::SimTable<" << tableTypes[table]
               << "> *read_table_"
               << identifier(specialization.tables[table].name) << "{};\n";
      for (auto [slotIndex, slot] : llvm::enumerate(specialization.slots))
        output << "  const gfsim::SlotState<" << slotTypes[slotIndex]
               << "> *slot_" << identifier(slot.name) << "{};\n";
      output << "  std::optional<" << planType
             << "> operator()(gfsim::Epoch epoch, ";
      if (oneOwner) {
        output << "const gfsim::SimTable<" << writeTypes.front()
               << "> &table_ref";
      } else {
        output << "std::tuple<";
        for (auto [index, type] : llvm::enumerate(writeTypes)) {
          if (index)
            output << ", ";
          output << "const gfsim::SimTable<" << type << "> *";
        }
        output << "> table_refs";
      }
      for (auto [inputIndex, type] : llvm::enumerate(inputTypes)) {
        output << ", const " << type << " &item";
        if (inputIndex)
          output << inputIndex;
      }
      output << ") const {\n";
      if (oneOwner) {
        output << "    const auto *table_"
               << identifier(specialization.tables[tables.front()].name)
               << " = &table_ref;\n";
      } else {
        for (auto [ownerIndex, tableIndex] : llvm::enumerate(tables))
          output << "    const auto *table_"
                 << identifier(specialization.tables[tableIndex].name)
                 << " = std::get<" << ownerIndex << ">(table_refs);\n";
      }
      for (size_t table : readOnlyTables)
        output << "    const auto *table_"
               << identifier(specialization.tables[table].name)
               << " = read_table_"
               << identifier(specialization.tables[table].name) << ";\n";
      output << "    auto [";
      bool bindingHasValue = false;
      for (size_t writeIndex = 0; writeIndex < firing.stateWrites.size();
           ++writeIndex) {
        if (bindingHasValue)
          output << ", ";
        output << stateWriteIndexName(firing, writeIndex) << ", "
               << stateWriteValueName(firing, writeIndex) << ", "
               << stateWritePresentName(firing, writeIndex);
        const StateWritePlan &write = firing.stateWrites[writeIndex];
        if (!write.versionedAction.empty()) {
          output << ", " << stateWriteRefGenerationName(firing, writeIndex)
                 << ", " << stateWriteRefEpochName(firing, writeIndex);
          if (!write.refAttempt.empty())
            output << ", " << stateWriteRefAttemptName(firing, writeIndex);
        }
        bindingHasValue = true;
      }
      for (size_t outputIndex = 0; outputIndex < outputTypes.size();
           ++outputIndex) {
        if (bindingHasValue)
          output << ", ";
        output << outputValueName(firing, outputIndex) << ", "
               << outputPresentName(firing, outputIndex);
        bindingHasValue = true;
      }
      for (auto [ownerIndex, tableIndex] : llvm::enumerate(tables)) {
        size_t reservationIndex = 0;
        for (const StateReservationPlan *reservation : findStateReservations(
                 firing, specialization.tables[tableIndex].name)) {
          if (reservation->indexKind == "all") {
            ++reservationIndex;
            continue;
          }
          if (bindingHasValue)
            output << ", ";
          output << reservationBindingName(
              specialization.tables[tableIndex].name, *reservation,
              reservationIndex++);
          bindingHasValue = true;
        }
      }
      for (size_t releaseIndex = 0; releaseIndex < firing.slotReleases.size();
           ++releaseIndex) {
        if (bindingHasValue)
          output << ", ";
        output << "slot_release_" << releaseIndex;
        bindingHasValue = true;
      }
      output << ", rule_condition] = [&]() {\n"
             << *body << "    }();\n"
             << "    if (!rule_condition)\n      return std::nullopt;\n";
      emitVersionedWriteQualification(output, firing, specialization, "    ");
      if (auto error = emitArchitectureObligationChecks(
              output, specialization, firing, "    "))
        return error;
      for (auto [ownerIndex, tableIndex] : llvm::enumerate(tables))
        emitStateWriteBatch(
            output, firing, specialization.tables[tableIndex].name,
            specialization, writeTypes[ownerIndex], ownerIndex, "    ");
      output << "    return " << planType;
      if (oneOwner) {
        output << "{std::move("
               << stateWriteBatchName(
                      specialization.tables[tables.front()].name)
               << "), {";
      } else {
        output << "{{";
        for (size_t ownerIndex = 0; ownerIndex < writeTypes.size();
             ++ownerIndex) {
          if (ownerIndex)
            output << ", ";
          output << "std::move("
                 << stateWriteBatchName(
                        specialization.tables[tables[ownerIndex]].name)
                 << ")";
        }
        output << "}, {";
      }
      for (auto [outputIndex, type] : llvm::enumerate(outputTypes)) {
        if (outputIndex)
          output << ", ";
        output << outputPresentName(firing, outputIndex) << " ? std::optional<"
               << type << ">{" << outputValueName(firing, outputIndex)
               << "} : std::optional<" << type << ">{}";
      }
      output << "}, {";
      for (size_t ownerIndex = 0; ownerIndex < writeTypes.size();
           ++ownerIndex) {
        if (ownerIndex)
          output << ", ";
        const TablePlan &table = specialization.tables[tables[ownerIndex]];
        output << "gfsim::StateReservation{}";
        size_t reservationIndex = 0;
        for (const StateReservationPlan *reservation :
             findStateReservations(firing, table.name)) {
          auto fieldMask =
              reservationFieldMask(plan, table, reservation->fields);
          if (!fieldMask)
            return fieldMask.takeError();
          if (reservation->indexKind == "all") {
            output << " | "
                   << (fieldMask->complete
                           ? "gfsim::StateReservation::all()"
                           : "gfsim::StateReservation::forAllFields("
                             "std::uint64_t{" +
                                 std::to_string(fieldMask->mask) + "}, " +
                                 std::to_string(fieldMask->count) + ")");
            ++reservationIndex;
          } else if (reservation->indexKind == "set") {
            output << " | "
                   << reservationBindingName(table.name, *reservation,
                                             reservationIndex++);
          } else {
            output << " | "
                   << (fieldMask->complete
                           ? "gfsim::StateReservation::forEntry("
                           : "gfsim::StateReservation::forFieldsAt(")
                   << "static_cast<std::size_t>("
                   << reservationBindingName(table.name, *reservation,
                                             reservationIndex++)
                   << ")";
            if (fieldMask->complete)
              output << ")";
            else
              output << ", std::uint64_t{" << fieldMask->mask << "}, "
                     << fieldMask->count << ")";
          }
        }
      }
      if (!oneOwner) {
        output << "}, {";
        for (size_t releaseIndex = 0; releaseIndex < firing.slotReleases.size();
             ++releaseIndex) {
          if (releaseIndex)
            output << ", ";
          output << "slot_release_" << releaseIndex;
        }
      }
      output << "}};\n  }\n};\n\n";
      for (auto [ownerIndex, tableIndex] : llvm::enumerate(tables)) {
        const TablePlan &table = specialization.tables[tableIndex];
        const StateWritePlan *write = findStateWrite(firing, table.name);
        const std::vector<std::string> fields =
            write ? write->fields : std::vector<std::string>{"$entry"};
        if (auto error = emitStructuredMergePolicy(
                output, specialization, mergeName(blockIndex, ownerIndex),
                writeTypes[ownerIndex], fields))
          return error;
      }
    }
    auto emitQueueTuple = [&](llvm::ArrayRef<std::string> queues) {
      output << "std::tuple{";
      for (auto [index, queue] : llvm::enumerate(queues)) {
        if (index)
          output << ", ";
        output << portParameters.lookup(queue);
      }
      output << '}';
    };
    output << "class " << implementation
           << " final : public gfsim::Module {\npublic:\n  " << implementation
           << "(std::string name";
    for (size_t index = 0; index < specialization.blocks.size(); ++index)
      output << ", gfsim::ObjectId block_" << index << "_id";
    for (size_t index = 0; index < specialization.tables.size(); ++index)
      output << ", gfsim::ObjectId table_" << index << "_id";
    output << ", gfsim::SimObject *parent";
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      output << ", gfsim::SimQueue<" << portTypes.lookup(input.name) << "> *"
             << portParameters.lookup(input.name);
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs)
      output << ", gfsim::SimQueue<" << portTypes.lookup(result.name) << "> *"
             << portParameters.lookup(result.name);
    output << ")\n      : gfsim::Module(std::move(name), "
              "gfsim::kInvalidObjectId, parent),\n        scope_(\""
           << localScope << "\", gfsim::kInvalidObjectId, this)";
    for (auto [index, table] : llvm::enumerate(specialization.tables)) {
      auto storage = tableStorageArgument(specialization, table);
      if (!storage)
        return storage.takeError();
      output << ",\n        " << tableMembers[index] << "(\"" << table.name
             << "\", table_" << index << "_id, &scope_, " << *storage << ")";
    }
    for (auto [blockIndex, firing] : llvm::enumerate(specialization.blocks)) {
      if (firing.kind == "slot") {
        auto slotIndex = slotIndices.find(firing.slot);
        if (slotIndex == slotIndices.end() || firing.inputs.size() != 1)
          return generatorError("structured slot declaration is missing");
        std::string policy = policyName(blockIndex) + "{";
        for (size_t index = 0; index < specialization.slots.size(); ++index) {
          if (index)
            policy.append(", ");
          policy.append("&slot_state_")
              .append(std::to_string(index))
              .append("_");
        }
        policy.push_back('}');
        output << ",\n        " << firingSymbols[blockIndex] << "_(\"slot_"
               << firing.name << "\", block_" << blockIndex << "_id, &scope_, "
               << "require_queue_port("
               << portParameters.lookup(firing.inputs.front()) << ", \""
               << firing.inputs.front() << "\")"
               << ", slot_state_" << slotIndex->getValue() << "_, " << policy
               << ")";
        continue;
      }
      const std::vector<size_t> tables = tableBindings(firing);
      const std::vector<size_t> readOnlyTables = readOnlyTableBindings(firing);
      std::string policy = policyName(blockIndex) + "{";
      bool hasPolicyMember = false;
      for (auto [index, table] : llvm::enumerate(readOnlyTables)) {
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&").append(tableMembers[table]);
        hasPolicyMember = true;
      }
      for (size_t slotIndex = 0; slotIndex < specialization.slots.size();
           ++slotIndex) {
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&slot_state_")
            .append(std::to_string(slotIndex))
            .append("_");
        hasPolicyMember = true;
      }
      policy.push_back('}');
      output << ",\n        " << firingSymbols[blockIndex] << "_(\"firing_"
             << firing.name << "\", block_" << blockIndex << "_id, &scope_, ";
      if (tables.size() == 1 && firing.slotReleases.empty()) {
        output << tableMembers[tables.front()] << ", ";
      } else {
        output << "std::tuple{";
        for (auto [index, table] : llvm::enumerate(tables)) {
          if (index)
            output << ", ";
          output << '&' << tableMembers[table];
        }
        output << "}, ";
      }
      emitQueueTuple(firing.inputs);
      output << ", ";
      emitQueueTuple(firing.outputs);
      if (tables.size() == 1) {
        const StateWritePlan *write =
            findStateWrite(firing, specialization.tables[tables.front()].name);
        output << ", gfsim::TableWriteMode::"
               << (!write || write->mode == "replace" ? "Replace"
                                                      : "FieldMerge")
               << ", " << policy << ", " << mergeName(blockIndex, 0) << "{})";
      } else {
        output << (tables.empty() ? ", std::array<gfsim::TableWriteMode, 0>{"
                                  : ", std::array{");
        for (auto [index, tableIndex] : llvm::enumerate(tables)) {
          if (index)
            output << ", ";
          const StateWritePlan *write =
              findStateWrite(firing, specialization.tables[tableIndex].name);
          output << "gfsim::TableWriteMode::"
                 << (!write || write->mode == "replace" ? "Replace"
                                                        : "FieldMerge");
        }
        output << "}, " << policy << ", std::tuple{";
        for (size_t index = 0; index < tables.size(); ++index) {
          if (index)
            output << ", ";
          output << mergeName(blockIndex, index) << "{}";
        }
        output << "}, nullptr, std::vector<gfsim::SlotReleaseResource *>{";
        for (auto [releaseIndex, release] :
             llvm::enumerate(firing.slotReleases)) {
          auto slotIndex = slotIndices.find(release.slot);
          if (slotIndex == slotIndices.end())
            return generatorError("structured slot release is missing");
          if (releaseIndex)
            output << ", ";
          output << "&slot_state_" << slotIndex->getValue() << "_";
        }
        output << "})";
      }
    }
    output << " {\n    attachChild(scope_);\n";
    for (size_t index = 0; index < specialization.tables.size(); ++index)
      output << "    scope_.attachChild(" << tableMembers[index] << ");\n";
    for (size_t index = 0; index < specialization.blocks.size(); ++index)
      output << "    scope_.attachChild(" << firingSymbols[index] << "_);\n";
    output << "  }\n\n  gfsim::DispatchRow dispatch_row(size_t index) {\n"
           << "    switch (index) {\n";
    for (size_t index = 0; index < specialization.blocks.size(); ++index)
      output << "    case " << index << ": return gfsim::makeDispatchRow(&"
             << firingSymbols[index] << "_);\n";
    for (size_t index = 0; index < specialization.tables.size(); ++index)
      output << "    case " << specialization.blocks.size() + index
             << ": return gfsim::makeDispatchRow(&" << tableMembers[index]
             << ");\n";
    output << "    default: return {};\n    }\n  }\n\nprivate:\n"
           << "  gfsim::Module scope_;\n";
    for (auto [index, type] : llvm::enumerate(tableTypes))
      output << "  gfsim::SimTable<" << type << "> " << tableMembers[index]
             << ";\n";
    for (auto [index, type] : llvm::enumerate(slotTypes))
      output << "  gfsim::SlotState<" << type << "> slot_state_" << index
             << "_;\n";
    for (auto [blockIndex, firing] : llvm::enumerate(specialization.blocks)) {
      if (firing.kind == "slot") {
        auto slotIndex = slotIndices.find(firing.slot);
        if (slotIndex == slotIndices.end())
          return generatorError("structured slot member is missing");
        output << "  gfsim::QueueSlot<" << slotTypes[slotIndex->getValue()]
               << ", " << policyName(blockIndex) << "> "
               << firingSymbols[blockIndex] << "_;\n";
        continue;
      }
      const std::vector<size_t> tables = tableBindings(firing);
      const std::vector<std::string> inputTypes = queueTypes(firing.inputs);
      const std::vector<std::string> outputTypes = queueTypes(firing.outputs);
      if (tables.size() == 1 && firing.slotReleases.empty()) {
        output << "  gfsim::QueueTableTransition<" << policyName(blockIndex)
               << ", " << tableTypes[tables.front()] << ", "
               << tupleType(inputTypes) << ", " << tupleType(outputTypes)
               << ", " << mergeName(blockIndex, 0) << "> "
               << firingSymbols[blockIndex] << "_;\n";
      } else {
        std::vector<std::string> writeTypes;
        for (size_t table : tables)
          writeTypes.push_back(tableTypes[table]);
        output << "  gfsim::QueueStateTransition<" << policyName(blockIndex)
               << ", " << tupleType(writeTypes) << ", " << tupleType(inputTypes)
               << ", " << tupleType(outputTypes) << ", std::tuple<";
        for (size_t index = 0; index < tables.size(); ++index) {
          if (index)
            output << ", ";
          output << mergeName(blockIndex, index);
        }
        output << ">> " << firingSymbols[blockIndex] << "_;\n";
      }
    }
    output << "};\n\n";
    return llvm::Error::success();
  };

  auto emitNestedWrapper =
      [&](const QueueGraphPlan &specialization,
          const std::string &implementation) -> llvm::Error {
    const auto runtimeNames = queueRuntimeNames(specialization);
    llvm::StringMap<std::string> portTypes;
    llvm::StringMap<std::string> queueExpressions;
    const uint64_t objectCount = specializationObjectCount(specialization);
    llvm::StringMap<std::string> portParameters =
        interfaceParameterNames(specialization, objectCount);
    for (const QueueInterfacePlan &input : specialization.interfaceInputs) {
      auto type = cppQueueType(plan, input);
      if (!type)
        return type.takeError();
      portTypes[input.name] = *type;
      queueExpressions[input.name] = portParameters.lookup(input.name);
    }
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs) {
      auto type = cppQueueType(plan, result);
      if (!type)
        return type.takeError();
      portTypes[result.name] = *type;
      queueExpressions[result.name] = portParameters.lookup(result.name);
    }
    llvm::StringSet<> interfaceQueues;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      interfaceQueues.insert(input.name);
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs)
      interfaceQueues.insert(result.name);
    std::vector<const QueuePlan *> internalQueues;
    for (const QueuePlan &queue : specialization.queues) {
      if (interfaceQueues.contains(queue.name))
        continue;
      auto type = cppQueueType(plan, queue);
      if (!type)
        return type.takeError();
      const size_t index = internalQueues.size();
      internalQueues.push_back(&queue);
      portTypes[queue.name] = *type;
      queueExpressions[queue.name] = "&queue_" + std::to_string(index) + "_";
    }
    output << "class " << implementation
           << " final : public gfsim::Module {\npublic:\n  " << implementation
           << "(std::string name";
    for (uint64_t index = 0; index < objectCount; ++index)
      output << ", gfsim::ObjectId object_" << index << "_id";
    output << ", gfsim::SimObject *parent";
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      output << ", gfsim::SimQueue<" << portTypes.lookup(input.name) << "> *"
             << portParameters.lookup(input.name);
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs)
      output << ", gfsim::SimQueue<" << portTypes.lookup(result.name) << "> *"
             << portParameters.lookup(result.name);
    output << ")\n      : gfsim::Module(std::move(name), "
              "gfsim::kInvalidObjectId, parent)";
    for (auto [index, queue] : llvm::enumerate(internalQueues))
      output << ",\n        queue_" << index << "_(\""
             << runtimeNames.lookup(queue->name)
             << "\", object_" << index << "_id, this, " << queue->depth
             << ", std::numeric_limits<size_t>::max(), nullptr, "
             << queue->latency << ", " << queue->rate << ", " << queue->lanes
             << ")";
    uint64_t objectOffset = internalQueues.size();
    for (const QueueModuleInstancePlan &instance : specialization.moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        return generatorError("nested wrapper child specialization is missing");
      objectOffset += specializationObjectCount(*child);
    }
    if (objectOffset != objectCount)
      return generatorError("nested wrapper object ID partition is incomplete");
    output << " {\n";
    for (size_t index = 0; index < internalQueues.size(); ++index)
      output << "    attachChild(queue_" << index << "_);\n";
    objectOffset = internalQueues.size();
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    child_" << instanceIndex << "_ = std::make_unique<"
             << specializationClassName(*child) << ">(\""
             << identifier(instance.name) << "\"";
      for (uint64_t index = 0; index < childCount; ++index)
        output << ", object_" << objectOffset + index << "_id";
      output << ", this";
      for (const std::string &input : instance.inputs)
        output << ", " << queueExpressions.lookup(input);
      for (const std::string &result : instance.outputs)
        output << ", " << queueExpressions.lookup(result);
      output << ");\n    attachChild(*child_" << instanceIndex << "_);\n";
      objectOffset += childCount;
    }
    output << "  }\n\n  gfsim::DispatchRow dispatch_row(size_t index) {\n";
    for (size_t queueIndex = 0; queueIndex < internalQueues.size();
         ++queueIndex)
      output << "    if (index == " << queueIndex
             << ") return gfsim::makeDispatchRow(&queue_" << queueIndex
             << "_);\n";
    objectOffset = internalQueues.size();
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    if (index < " << objectOffset + childCount
             << ") return child_" << instanceIndex << "_->dispatch_row(index - "
             << objectOffset << ");\n";
      objectOffset += childCount;
    }
    output << "    return {};\n  }\n\nprivate:\n";
    for (auto [index, queue] : llvm::enumerate(internalQueues))
      output << "  gfsim::SimQueue<" << portTypes.lookup(queue->name)
             << "> queue_" << index << "_;\n";
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      output << "  std::unique_ptr<" << specializationClassName(*child)
             << "> child_" << instanceIndex << "_;\n";
    }
    output << "};\n\n";
    return llvm::Error::success();
  };

  auto emitNestedAssembly =
      [&](const QueueGraphPlan &specialization,
          const std::string &implementation) -> llvm::Error {
    const auto runtimeNames = queueRuntimeNames(specialization);
    llvm::StringMap<std::string> queueTypes;
    llvm::StringMap<std::string> queueExpressions;
    const uint64_t objectCount = specializationObjectCount(specialization);
    llvm::StringMap<std::string> portParameters =
        interfaceParameterNames(specialization, objectCount);
    llvm::StringSet<> interfaceQueues;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs) {
      auto type = cppQueueType(plan, input);
      if (!type)
        return type.takeError();
      interfaceQueues.insert(input.name);
      queueTypes[input.name] = *type;
      queueExpressions[input.name] = portParameters.lookup(input.name);
    }
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs) {
      auto type = cppQueueType(plan, result);
      if (!type)
        return type.takeError();
      interfaceQueues.insert(result.name);
      queueTypes[result.name] = *type;
      queueExpressions[result.name] = portParameters.lookup(result.name);
    }
    std::vector<const QueuePlan *> internalQueues;
    for (const QueuePlan &queue : specialization.queues) {
      if (interfaceQueues.contains(queue.name))
        continue;
      auto type = cppQueueType(plan, queue);
      if (!type)
        return type.takeError();
      const size_t index = internalQueues.size();
      internalQueues.push_back(&queue);
      queueTypes[queue.name] = *type;
      queueExpressions[queue.name] = "&queue_" + std::to_string(index) + "_";
    }
    llvm::StringMap<std::string> scopeMembers;
    for (auto [index, scope] : llvm::enumerate(specialization.scopes))
      scopeMembers[scope] = "scope_" + std::to_string(index) + "_";
    auto modulePointer =
        [&](llvm::StringRef path) -> llvm::Expected<std::string> {
      if (path == "/")
        return std::string("this");
      auto found = scopeMembers.find(path);
      if (found == scopeMembers.end())
        return generatorError("unknown nested assembly scope '" + path + "'");
      return "&" + found->getValue();
    };
    auto attach = [&](llvm::StringRef path,
                      llvm::StringRef member) -> llvm::Expected<std::string> {
      if (path == "/")
        return "    attachChild(" + member.str() + ");";
      auto found = scopeMembers.find(path);
      if (found == scopeMembers.end())
        return generatorError("unknown nested assembly attachment scope '" +
                              path + "'");
      return "    " + found->getValue() + ".attachChild(" + member.str() + ");";
    };

    output << "class " << implementation
           << " final : public gfsim::Module {\npublic:\n  " << implementation
           << "(std::string name";
    for (uint64_t index = 0; index < objectCount; ++index)
      output << ", gfsim::ObjectId object_" << index << "_id";
    output << ", gfsim::SimObject *parent";
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      output << ", gfsim::SimQueue<" << queueTypes.lookup(input.name) << "> *"
             << portParameters.lookup(input.name);
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs)
      output << ", gfsim::SimQueue<" << queueTypes.lookup(result.name) << "> *"
             << portParameters.lookup(result.name);
    output << ")\n      : gfsim::Module(std::move(name), "
              "gfsim::kInvalidObjectId, parent)";
    for (const std::string &scope : specialization.scopes) {
      llvm::StringRef parentPath = llvm::StringRef(scope).rsplit('/').first;
      if (parentPath.empty())
        parentPath = "/";
      auto parentPointer = modulePointer(parentPath);
      if (!parentPointer)
        return parentPointer.takeError();
      output << ",\n        " << scopeMembers.lookup(scope) << "(\""
             << pathParts(scope).back() << "\", gfsim::kInvalidObjectId, "
             << *parentPointer << ")";
    }
    for (auto [index, queue] : llvm::enumerate(internalQueues)) {
      auto parentPointer = modulePointer(queue->scope);
      if (!parentPointer)
        return parentPointer.takeError();
      output << ",\n        queue_" << index << "_(\""
             << runtimeNames.lookup(queue->name)
             << "\", object_" << index << "_id, " << *parentPointer << ", "
             << queue->depth
             << ", std::numeric_limits<size_t>::max(), nullptr, "
             << queue->latency << ", " << queue->rate << ", " << queue->lanes
             << ")";
    }
    uint64_t childOffset = internalQueues.size() + specialization.blocks.size();
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        return generatorError(
            "nested assembly child specialization is missing");
      const uint64_t childCount = specializationObjectCount(*child);
      childOffset += childCount;
    }
    const uint64_t blockOffset = internalQueues.size();
    for (auto [blockIndex, block] : llvm::enumerate(specialization.blocks)) {
      if (block.kind != "broadcast" || block.inputs.size() != 1 ||
          block.outputs.empty())
        return generatorError("nested assembly supports broadcast blocks only");
      auto parentPointer = modulePointer(block.scope);
      if (!parentPointer)
        return parentPointer.takeError();
      const std::string type = queueTypes.lookup(block.inputs.front());
      if (type.empty())
        return generatorError(
            "nested assembly broadcast input type is missing");
      output << ",\n        block_" << blockIndex << "_(\"broadcast_"
             << block.name << "\", object_" << blockOffset + blockIndex
             << "_id, " << *parentPointer << ", "
             << "require_queue_port("
             << queueExpressions.lookup(block.inputs.front()) << ", \""
             << block.inputs.front() << "\")"
             << ", std::array<gfsim::SimQueue<" << type << "> *, "
             << block.outputs.size() << ">{";
      for (auto [outputIndex, outputName] : llvm::enumerate(block.outputs)) {
        if (outputIndex)
          output << ", ";
        output << queueExpressions.lookup(outputName);
      }
      output << "})";
    }
    if (childOffset != objectCount)
      return generatorError(
          "nested assembly object ID partition is incomplete");

    output << " {\n";
    for (const std::string &scope : specialization.scopes) {
      llvm::StringRef parentPath = llvm::StringRef(scope).rsplit('/').first;
      if (parentPath.empty())
        parentPath = "/";
      auto line = attach(parentPath, scopeMembers.lookup(scope));
      if (!line)
        return line.takeError();
      output << *line << '\n';
    }
    for (auto [index, queue] : llvm::enumerate(internalQueues)) {
      auto line = attach(queue->scope, "queue_" + std::to_string(index) + "_");
      if (!line)
        return line.takeError();
      output << *line << '\n';
    }
    childOffset = internalQueues.size() + specialization.blocks.size();
    for (auto [index, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      auto parentPointer = modulePointer(instance.scope);
      if (!parentPointer)
        return parentPointer.takeError();
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    child_" << index << "_ = std::make_unique<"
             << specializationClassName(*child) << ">(\""
             << identifier(instance.name) << "\"";
      for (uint64_t local = 0; local < childCount; ++local)
        output << ", object_" << childOffset + local << "_id";
      output << ", " << *parentPointer;
      for (const std::string &input : instance.inputs)
        output << ", " << queueExpressions.lookup(input);
      for (const std::string &result : instance.outputs)
        output << ", " << queueExpressions.lookup(result);
      output << ");\n";
      auto line = attach(instance.scope,
                         "*child_" + std::to_string(index) + "_");
      if (!line)
        return line.takeError();
      output << *line << '\n';
      childOffset += childCount;
    }
    for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
      auto line = attach(block.scope, "block_" + std::to_string(index) + "_");
      if (!line)
        return line.takeError();
      output << *line << '\n';
    }
    output << "  }\n\n  gfsim::DispatchRow dispatch_row(size_t index) {\n";
    for (size_t index = 0; index < internalQueues.size(); ++index)
      output << "    if (index == " << index
             << ") return gfsim::makeDispatchRow(&queue_" << index << "_);\n";
    for (size_t index = 0; index < specialization.blocks.size(); ++index)
      output << "    if (index == " << blockOffset + index
             << ") return gfsim::makeDispatchRow(&block_" << index << "_);\n";
    childOffset = internalQueues.size() + specialization.blocks.size();
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    if (index < " << childOffset + childCount
             << ") return child_" << instanceIndex << "_->dispatch_row(index - "
             << childOffset << ");\n";
      childOffset += childCount;
    }
    output << "    return {};\n  }\n\nprivate:\n";
    for (auto [index, scope] : llvm::enumerate(specialization.scopes))
      output << "  gfsim::Module scope_" << index << "_;\n";
    for (auto [index, queue] : llvm::enumerate(internalQueues))
      output << "  gfsim::SimQueue<" << queueTypes.lookup(queue->name)
             << "> queue_" << index << "_;\n";
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      output << "  std::unique_ptr<" << specializationClassName(*child)
             << "> child_" << instanceIndex << "_;\n";
    }
    for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
      const std::string type = queueTypes.lookup(block.inputs.front());
      output << "  gfsim::QueueBroadcast<" << type << ", "
             << block.outputs.size() << "> block_" << index << "_;\n";
    }
    output << "};\n\n";
    return llvm::Error::success();
  };

  auto emitMixedNested = [&](const QueueGraphPlan &specialization,
                             const std::string &implementation) -> llvm::Error {
    const auto runtimeNames = queueRuntimeNames(specialization);
    llvm::StringMap<std::string> queueTypes;
    llvm::StringMap<std::string> queueExpressions;
    for (auto [index, input] :
         llvm::enumerate(specialization.interfaceInputs)) {
      auto type = cppQueueType(plan, input);
      if (!type)
        return type.takeError();
      queueTypes[input.name] = *type;
      queueExpressions[input.name] = "input_" + std::to_string(index);
    }
    for (auto [index, result] :
         llvm::enumerate(specialization.interfaceOutputs)) {
      auto type = cppQueueType(plan, result);
      if (!type)
        return type.takeError();
      queueTypes[result.name] = *type;
      queueExpressions[result.name] = "output_" + std::to_string(index);
    }
    llvm::StringSet<> exported;
    for (const QueueInterfacePlan &input : specialization.interfaceInputs)
      exported.insert(input.name);
    for (const QueueInterfacePlan &result : specialization.interfaceOutputs)
      exported.insert(result.name);
    std::vector<const QueuePlan *> internalQueues;
    for (const QueuePlan &queue : specialization.queues) {
      if (exported.contains(queue.name))
        continue;
      auto type = cppQueueType(plan, queue);
      if (!type)
        return type.takeError();
      const size_t index = internalQueues.size();
      internalQueues.push_back(&queue);
      queueTypes[queue.name] = *type;
      queueExpressions[queue.name] = "&queue_" + std::to_string(index) + "_";
    }
    if (specialization.blocks.empty() || internalQueues.empty())
      return generatorError("mixed nested module requires local blocks and an "
                            "internal Queue");
    // A parent owns one scope object per distinct local block scope. The single
    // block shape keeps the original scope_/block_ spelling so its generated
    // output stays byte-identical.
    llvm::SmallVector<std::string> scopeNames;
    for (const QueueBlockPlan &candidate : specialization.blocks) {
      const std::string scope = pathParts(candidate.scope).back();
      if (!llvm::is_contained(scopeNames, scope))
        scopeNames.push_back(scope);
    }
    const bool singleLocalBlock = specialization.blocks.size() == 1;
    auto scopeMember = [&](size_t index) -> std::string {
      return singleLocalBlock ? std::string("scope_")
                              : "scope_" + std::to_string(index) + "_";
    };
    auto blockMember = [&](size_t index) -> std::string {
      return singleLocalBlock ? std::string("block_")
                              : "block_" + std::to_string(index) + "_";
    };
    auto blockPolicy = [&](size_t index) -> std::string {
      return singleLocalBlock
                 ? implementation + "_local_policy"
                 : implementation + "_local_policy_" + std::to_string(index);
    };
    // A stateless firing block reuses the direct-interface stateful emitter's
    // policy shape: one closed condition plus one presence predicate per output,
    // returned as a StateTransitionPlan over an empty Table tuple. That keeps
    // the conditional `ac.firing.output ... when ...` presence instead of
    // flattening it into an unconditional single output.
    llvm::SmallVector<std::string> blockFiringPolicies(
        specialization.blocks.size());
    {
      llvm::StringSet<> usedFiringSymbols;
      for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
        if (block.kind != "firing")
          continue;
        llvm::StringRef display = block.displayRuleName.empty()
                                      ? llvm::StringRef(block.name)
                                      : llvm::StringRef(block.displayRuleName);
        blockFiringPolicies[index] =
            implementation + "_" +
            uniqueIdentifier(("rule_" + display).str(), usedFiringSymbols) +
            "_policy";
      }
    }
    auto emitStatelessFiringPolicy =
        [&](const QueueBlockPlan &block, const std::string &policy,
            llvm::ArrayRef<std::string> inputTypes,
            llvm::ArrayRef<std::string> outputTypes) -> llvm::Error {
      if (!isStatelessFiringBlock(block))
        return generatorError(
            "mixed nested module supports only stateless local firing blocks; "
            "firing block '" +
            block.name + "' owns Table state");
      if (block.outputs.empty() || block.yields.size() != block.outputs.size())
        return generatorError(
            "mixed nested firing yield count does not match its output Queue "
            "count");
      std::string planType = "gfsim::StateTransitionPlan<std::tuple<>, "
                             "std::tuple<";
      for (auto [typeIndex, type] : llvm::enumerate(outputTypes)) {
        if (typeIndex)
          planType.append(", ");
        planType.append(type);
      }
      planType.append(">>");
      QueueBlockPlan evaluation = block;
      std::vector<std::string> additional{block.guard};
      std::string tupleResult = "std::tuple{";
      bool tupleHasValue = false;
      for (auto [outputIndex, yield] : llvm::enumerate(block.yields)) {
        const std::string &present = block.outputPresence[outputIndex].present;
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(yield).append(", ").append(present);
        tupleHasValue = true;
        additional.push_back(yield);
        additional.push_back(present);
      }
      if (tupleHasValue)
        tupleResult.append(", ");
      tupleResult.append(block.guard).push_back('}');
      auto body = emitExpressionBody(specialization, evaluation,
                                     block.yields.front(), 6, true, false,
                                     additional, tupleResult);
      if (!body)
        return body.takeError();
      emitRuleProvenance(output, block);
      output << "struct " << policy << " {\n";
      output << "  std::optional<" << planType
             << "> operator()(gfsim::Epoch epoch, std::tuple<> table_refs";
      for (auto [inputIndex, type] : llvm::enumerate(inputTypes)) {
        output << ", const " << type << " &item";
        if (inputIndex)
          output << inputIndex;
      }
      output << ") const {\n    auto [";
      bool bindingHasValue = false;
      for (size_t outputIndex = 0; outputIndex < outputTypes.size();
           ++outputIndex) {
        if (bindingHasValue)
          output << ", ";
        output << outputValueName(block, outputIndex) << ", "
               << outputPresentName(block, outputIndex);
        bindingHasValue = true;
      }
      if (bindingHasValue)
        output << ", ";
      output << "rule_condition] = [&]() {\n"
             << *body << "    }();\n"
             << "    if (!rule_condition)\n      return std::nullopt;\n"
             << "    return " << planType << "{{}, {";
      for (auto [outputIndex, type] : llvm::enumerate(outputTypes)) {
        if (outputIndex)
          output << ", ";
        output << outputPresentName(block, outputIndex) << " ? std::optional<"
               << type << ">{" << outputValueName(block, outputIndex)
               << "} : std::optional<" << type << ">{}";
      }
      output << "}, {}, {}};\n  }\n};\n\n";
      return llvm::Error::success();
    };
    auto scopeIndexOf = [&](const std::string &scope) -> size_t {
      const auto found = std::find(scopeNames.begin(), scopeNames.end(),
                                   pathParts(scope).back());
      return static_cast<size_t>(std::distance(scopeNames.begin(), found));
    };
    llvm::SmallVector<std::vector<std::string>> blockInputTypes;
    llvm::SmallVector<std::vector<std::string>> blockOutputTypes;
    llvm::SmallVector<std::string> blockPolicies;
    llvm::SmallVector<uint64_t> blockRates;
    for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
      if (block.kind == "broadcast") {
        if (block.inputs.size() != 1 || block.outputs.size() < 2)
          return generatorError("mixed nested broadcast requires one input and "
                                "at least two output Queues");
        const std::string inputType = queueTypes.lookup(block.inputs.front());
        if (inputType.empty())
          return generatorError("mixed nested broadcast input Queue is missing");
        blockInputTypes.push_back({inputType});
        blockOutputTypes.push_back({});
        blockPolicies.push_back("");
        blockRates.push_back(0);
        continue;
      }
      if (block.kind == "merge") {
        // A selective merge reuses the runtime `gfsim::QueueMerge` primitive
        // that the generic `kind == "merge"` emitter already binds; the mixed
        // path only proves the arity, payload equality, and policy the
        // primitive requires before registering the block.
        if (block.inputs.size() < 2 || block.outputs.size() != 1)
          return generatorError(
              "mixed nested merge requires at least two input Queues and "
              "exactly one output Queue");
        if (block.policy != "priority" && block.policy != "round_robin")
          return generatorError(
              "mixed nested merge policy must be priority or round_robin");
        const std::string outputType = queueTypes.lookup(block.outputs.front());
        if (outputType.empty())
          return generatorError(
              "mixed nested merge output Queue type is missing");
        std::vector<std::string> inputTypes;
        for (const std::string &name : block.inputs) {
          const std::string type = queueTypes.lookup(name);
          if (type.empty())
            return generatorError(
                "mixed nested merge input Queue type is missing");
          if (type != outputType)
            return generatorError(
                "mixed nested merge requires every input Queue payload type "
                "to match its output Queue payload type");
          inputTypes.push_back(type);
        }
        blockInputTypes.push_back(std::move(inputTypes));
        blockOutputTypes.push_back({outputType});
        blockPolicies.push_back(block.policy == "priority"
                                    ? "gfsim::QueueMergePolicy::Priority"
                                    : "gfsim::QueueMergePolicy::RoundRobin");
        blockRates.push_back(1);
        continue;
      }
      if (block.kind == "firing") {
        if (block.inputs.empty() || block.outputs.empty())
          return generatorError(
              "mixed nested firing requires input and output Queues");
        std::vector<std::string> inputTypes;
        for (const std::string &name : block.inputs) {
          const std::string type = queueTypes.lookup(name);
          if (type.empty())
            return generatorError(
                "mixed nested firing input Queue type is missing");
          inputTypes.push_back(type);
        }
        std::vector<std::string> outputTypes;
        for (const std::string &name : block.outputs) {
          const std::string type = queueTypes.lookup(name);
          if (type.empty())
            return generatorError(
                "mixed nested firing output Queue type is missing");
          outputTypes.push_back(type);
        }
        blockInputTypes.push_back(std::move(inputTypes));
        blockOutputTypes.push_back(std::move(outputTypes));
        blockPolicies.push_back(blockFiringPolicies[index]);
        blockRates.push_back(1);
        if (auto error = emitStatelessFiringPolicy(
                block, blockPolicies.back(), blockInputTypes.back(),
                blockOutputTypes.back()))
          return error;
        continue;
      }
      if (block.kind != "transform")
        return generatorError(
            "mixed nested module supports local transform, fanout broadcast, "
            "selective merge, and stateless firing blocks only");
      if (block.inputs.empty() || block.outputs.empty())
        return generatorError(
            "mixed nested transform requires input and output Queues");
      if (block.yields.size() != block.outputs.size())
        return generatorError("mixed nested transform yield count does not "
                              "match its output Queue count");
      std::vector<std::string> inputTypes;
      for (const std::string &name : block.inputs) {
        const std::string type = queueTypes.lookup(name);
        if (type.empty())
          return generatorError(
              "mixed nested transform input Queue type is missing");
        inputTypes.push_back(type);
      }
      std::vector<std::string> outputTypes;
      for (const std::string &name : block.outputs) {
        const std::string type = queueTypes.lookup(name);
        if (type.empty())
          return generatorError(
              "mixed nested transform output Queue type is missing");
        outputTypes.push_back(type);
      }
      const QueuePlan *outputQueue =
          findQueue(specialization, block.outputs.front());
      if (!outputQueue)
        return generatorError("mixed nested transform output Queue is missing");
      const bool oneByOne = inputTypes.size() == 1 && outputTypes.size() == 1;
      if (!oneByOne) {
        // The variadic atomic transform shares one policy invocation across all
        // inputs and outputs, so every port has to be a scalar Queue.
        for (const std::string &name : block.inputs) {
          const QueuePlan *queue = findQueue(specialization, name);
          if (queue && (queue->lanes != 1 || queue->rate != 1))
            return generatorError(
                "mixed nested atomic transform requires scalar rate and lanes");
        }
        for (const std::string &name : block.outputs) {
          const QueuePlan *queue = findQueue(specialization, name);
          if (queue && (queue->lanes != 1 || queue->rate != 1))
            return generatorError(
                "mixed nested atomic transform requires scalar rate and lanes");
        }
      }
      blockInputTypes.push_back(std::move(inputTypes));
      blockOutputTypes.push_back(std::move(outputTypes));
      blockRates.push_back(outputQueue->rate);
      blockPolicies.push_back(blockPolicy(index));
      output << "struct " << blockPolicies.back() << " {\n  ";
      if (oneByOne) {
        output << blockOutputTypes.back().front();
      } else {
        output << "std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(blockOutputTypes.back())) {
          if (typeIndex)
            output << ", ";
          output << type;
        }
        output << ">";
      }
      output << " operator()(";
      for (auto [inputIndex, type] : llvm::enumerate(blockInputTypes.back())) {
        if (inputIndex)
          output << ", ";
        output << "const " << type << " &item";
        if (inputIndex)
          output << inputIndex;
      }
      output << ") const {\n";
      if (oneByOne) {
        auto body =
            emitExpressionBody(specialization, block, block.yields.front(), 4);
        if (!body)
          return body.takeError();
        output << *body;
      } else {
        output << "    return {\n";
        for (auto [yieldIndex, yield] : llvm::enumerate(block.yields)) {
          output << "      [&]() -> " << blockOutputTypes.back()[yieldIndex]
                 << " {\n";
          auto body = emitExpressionBody(specialization, block, yield, 8);
          if (!body)
            return body.takeError();
          output << *body << "      }()"
                 << (yieldIndex + 1 == block.yields.size() ? "\n" : ",\n");
        }
        output << "    };\n";
      }
      output << "  }\n};\n\n";
    }
    output << "class " << implementation
           << " final : public gfsim::Module {\npublic:\n  " << implementation
           << "(std::string name";
    const uint64_t objectCount = specializationObjectCount(specialization);
    for (uint64_t index = 0; index < objectCount; ++index)
      output << ", gfsim::ObjectId object_" << index << "_id";
    output << ", gfsim::SimObject *parent";
    for (auto [index, input] : llvm::enumerate(specialization.interfaceInputs))
      output << ", gfsim::SimQueue<" << queueTypes.lookup(input.name)
             << "> *input_" << index;
    for (auto [index, result] :
         llvm::enumerate(specialization.interfaceOutputs))
      output << ", gfsim::SimQueue<" << queueTypes.lookup(result.name)
             << "> *output_" << index;
    output << ")\n      : gfsim::Module(std::move(name), "
              "gfsim::kInvalidObjectId, parent)";
    for (size_t index = 0; index < scopeNames.size(); ++index)
      output << ",\n        " << scopeMember(index) << "(\""
             << scopeNames[index] << "\", gfsim::kInvalidObjectId, this)";
    for (auto [index, queue] : llvm::enumerate(internalQueues))
      output << ",\n        queue_" << index << "_(\""
             << runtimeNames.lookup(queue->name)
             << "\", object_" << index << "_id, this, " << queue->depth
             << ", std::numeric_limits<size_t>::max(), nullptr, "
             << queue->latency << ", " << queue->rate << ", " << queue->lanes
             << ")";
    const uint64_t blockId = internalQueues.size();
    for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
      output << ",\n        " << blockMember(index) << "(\"";
      if (block.kind == "broadcast") {
        output << "broadcast_" << block.name << "\", object_" << blockId + index
               << "_id, &" << scopeMember(scopeIndexOf(block.scope)) << ", "
               << "require_queue_port("
               << queueExpressions.lookup(block.inputs.front()) << ", \""
               << block.inputs.front() << "\"), "
               << "std::array<gfsim::SimQueue<" << blockInputTypes[index].front()
               << "> *, " << block.outputs.size() << ">{";
        for (auto [outputIndex, name] : llvm::enumerate(block.outputs)) {
          if (outputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "})";
        continue;
      }
      if (block.kind == "merge") {
        output << "merge_" << block.name << "\", object_" << blockId + index
               << "_id, &" << scopeMember(scopeIndexOf(block.scope))
               << ", std::array<gfsim::SimQueue<"
               << blockInputTypes[index].front() << "> *, "
               << block.inputs.size() << ">{";
        for (auto [inputIndex, name] : llvm::enumerate(block.inputs)) {
          if (inputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "}, require_queue_port("
               << queueExpressions.lookup(block.outputs.front()) << ", \""
               << block.outputs.front() << "\"), " << blockPolicies[index]
               << ")";
        continue;
      }
      if (block.kind == "firing") {
        output << "firing_" << block.name << "\", object_" << blockId + index
               << "_id, &" << scopeMember(scopeIndexOf(block.scope)) << ", "
               << "std::tuple{}, std::tuple{";
        for (auto [inputIndex, name] : llvm::enumerate(block.inputs)) {
          if (inputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "}, std::tuple{";
        for (auto [outputIndex, name] : llvm::enumerate(block.outputs)) {
          if (outputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "}, std::array<gfsim::TableWriteMode, 0>{}, "
               << blockPolicies[index]
               << "{}, std::tuple{}, nullptr, "
                  "std::vector<gfsim::SlotReleaseResource *>{})";
        continue;
      }
      const bool oneByOne = blockInputTypes[index].size() == 1 &&
                            blockOutputTypes[index].size() == 1;
      output << "transform_" << block.name << "\", object_" << blockId + index
             << "_id, &" << scopeMember(scopeIndexOf(block.scope)) << ", ";
      if (oneByOne) {
        output << "require_queue_port("
               << queueExpressions.lookup(block.inputs.front()) << ", \""
               << block.inputs.front() << "\"), "
               << "require_queue_port("
               << queueExpressions.lookup(block.outputs.front()) << ", \""
               << block.outputs.front() << "\")";
      } else {
        output << "std::tuple{";
        for (auto [inputIndex, name] : llvm::enumerate(block.inputs)) {
          if (inputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "}, std::tuple{";
        for (auto [outputIndex, name] : llvm::enumerate(block.outputs)) {
          if (outputIndex)
            output << ", ";
          output << queueExpressions.lookup(name);
        }
        output << "}";
      }
      output << ")";
    }
    uint64_t childOffset = blockId + specialization.blocks.size();
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        return generatorError("mixed nested child specialization is missing");
      const uint64_t childCount = specializationObjectCount(*child);
      childOffset += childCount;
    }
    if (childOffset != objectCount)
      return generatorError("mixed nested object ID partition is incomplete: " +
                            std::to_string(childOffset) + " != " +
                            std::to_string(objectCount));
    output << " {\n";
    for (size_t index = 0; index < scopeNames.size(); ++index)
      output << "    attachChild(" << scopeMember(index) << ");\n";
    for (size_t index = 0; index < internalQueues.size(); ++index)
      output << "    attachChild(queue_" << index << "_);\n";
    for (auto [index, block] : llvm::enumerate(specialization.blocks))
      output << "    " << scopeMember(scopeIndexOf(block.scope))
             << ".attachChild(" << blockMember(index) << ");\n";
    childOffset = blockId + specialization.blocks.size();
    for (auto [index, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    child_" << index << "_ = std::make_unique<"
             << specializationClassName(*child) << ">(\""
             << identifier(instance.name) << "\"";
      for (uint64_t local = 0; local < childCount; ++local)
        output << ", object_" << childOffset + local << "_id";
      output << ", this";
      for (const std::string &input : instance.inputs)
        output << ", " << queueExpressions.lookup(input);
      for (const std::string &result : instance.outputs)
        output << ", " << queueExpressions.lookup(result);
      output << ");\n    attachChild(*child_" << index << "_);\n";
      childOffset += childCount;
    }
    output << "  }\n\n  gfsim::DispatchRow dispatch_row(size_t index) {\n";
    for (size_t index = 0; index < internalQueues.size(); ++index)
      output << "    if (index == " << index
             << ") return gfsim::makeDispatchRow(&queue_" << index << "_);\n";
    for (size_t index = 0; index < specialization.blocks.size(); ++index)
      output << "    if (index == " << blockId + index
             << ") return gfsim::makeDispatchRow(&" << blockMember(index)
             << ");\n";
    childOffset = blockId + specialization.blocks.size();
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      const uint64_t childCount = specializationObjectCount(*child);
      output << "    if (index < " << childOffset + childCount
             << ") return child_" << instanceIndex << "_->dispatch_row(index - "
             << childOffset << ");\n";
      childOffset += childCount;
    }
    output << "    return {};\n  }\n\nprivate:\n";
    for (size_t index = 0; index < scopeNames.size(); ++index)
      output << "  gfsim::Module " << scopeMember(index) << ";\n";
    for (auto [index, queue] : llvm::enumerate(internalQueues))
      output << "  gfsim::SimQueue<" << queueTypes.lookup(queue->name)
             << "> queue_" << index << "_;\n";
    for (auto [index, block] : llvm::enumerate(specialization.blocks)) {
      if (block.kind == "broadcast") {
        output << "  gfsim::QueueBroadcast<" << blockInputTypes[index].front()
               << ", " << block.outputs.size() << "> " << blockMember(index)
               << ";\n";
        continue;
      }
      if (block.kind == "merge") {
        output << "  gfsim::QueueMerge<" << blockOutputTypes[index].front()
               << ", " << block.inputs.size() << "> " << blockMember(index)
               << ";\n";
        continue;
      }
      if (block.kind == "firing") {
        output << "  gfsim::QueueStateTransition<" << blockPolicies[index]
               << ", std::tuple<>, std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(blockInputTypes[index])) {
          if (typeIndex)
            output << ", ";
          output << type;
        }
        output << ">, std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(blockOutputTypes[index])) {
          if (typeIndex)
            output << ", ";
          output << type;
        }
        output << ">, std::tuple<>> " << blockMember(index) << ";\n";
        continue;
      }
      const bool oneByOne = blockInputTypes[index].size() == 1 &&
                            blockOutputTypes[index].size() == 1;
      if (oneByOne) {
        output << "  gfsim::QueueTransform<" << blockInputTypes[index].front()
               << ", " << blockOutputTypes[index].front() << ", "
               << blockPolicies[index] << ", " << blockRates[index] << "> "
               << blockMember(index) << ";\n";
      } else {
        output << "  gfsim::QueueAtomicTransform<" << blockPolicies[index]
               << ", std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(blockInputTypes[index])) {
          if (typeIndex)
            output << ", ";
          output << type;
        }
        output << ">, std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(blockOutputTypes[index])) {
          if (typeIndex)
            output << ", ";
          output << type;
        }
        output << ">> " << blockMember(index) << ";\n";
      }
    }
    for (auto [instanceIndex, instance] :
         llvm::enumerate(specialization.moduleInstances)) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      output << "  std::unique_ptr<" << specializationClassName(*child)
             << "> child_" << instanceIndex << "_;\n";
    }
    output << "};\n\n";
    return llvm::Error::success();
  };

  for (const QueueGraphPlan *specialization : emissionOrder) {
    const std::string implementation = specializationClassName(*specialization);
    const std::string fileStem = specializationFileStem(*specialization);
    std::vector<std::string> childFileStems;
    for (const QueueModuleInstancePlan &instance :
         specialization->moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        return generatorError("specialization child is missing");
      childFileStems.push_back(specializationFileStem(*child));
    }
    const std::size_t begin = static_cast<std::size_t>(output.tellp());
    std::ostringstream provenance;
    emitDefinitionProvenance(provenance, *specialization);
    output << provenance.str();
    auto recordModule = [&]() -> llvm::Error {
      moduleSpans.push_back({fileStem, implementation, childFileStems,
                             provenance.str(), begin,
                             static_cast<std::size_t>(output.tellp())});
      return llvm::Error::success();
    };
    const bool multiBlockLocal =
        specialization->moduleInstances.empty() &&
        isWideMixedLocalShape(*specialization) &&
        specialization->blocks.size() > 1 &&
        llvm::any_of(specialization->blocks, [](const QueueBlockPlan &block) {
          return block.kind != "firing" && block.kind != "slot";
        });
    if (!specialization->moduleInstances.empty() || multiBlockLocal) {
      const bool broadcastAssembly =
          !specialization->moduleInstances.empty() &&
          !specialization->blocks.empty() &&
          llvm::all_of(specialization->blocks, [](const QueueBlockPlan &block) {
            return block.kind == "broadcast";
          });
      llvm::Error error =
          specialization->blocks.empty()
              ? emitNestedWrapper(*specialization, implementation)
          : broadcastAssembly
              ? emitNestedAssembly(*specialization, implementation)
              : emitMixedNested(*specialization, implementation);
      if (error)
        return std::move(error);
      if (auto error = recordModule())
        return std::move(error);
      continue;
    }
    if (specialization->blocks.empty()) {
      output << "class " << implementation
             << " final : public gfsim::Module {\npublic:\n  " << implementation
             << "(std::string name, gfsim::SimObject *parent)\n"
                "      : gfsim::Module(std::move(name), "
                "gfsim::kInvalidObjectId, parent) {}\n\n"
                "  gfsim::DispatchRow dispatch_row(size_t) { return {}; }\n"
                "};\n\n";
      if (auto error = recordModule())
        return std::move(error);
      continue;
    }
    const QueueBlockPlan &block = specialization->blocks.front();
    const std::string localScope = pathParts(block.scope).back();
    if (block.kind == "firing") {
      if (auto error =
              emitStatefulSpecialization(*specialization, implementation))
        return std::move(error);
      if (auto error = recordModule())
        return std::move(error);
      continue;
    }
    if (block.kind == "transform") {
      std::vector<std::string> inputTypes;
      std::vector<std::string> outputTypes;
      llvm::StringMap<std::string> queueTypes;
      for (const QueueInterfacePlan &input : specialization->interfaceInputs) {
        auto type = cppQueueType(plan, input);
        if (!type)
          return type.takeError();
        queueTypes[input.name] = *type;
      }
      for (const QueueInterfacePlan &result :
           specialization->interfaceOutputs) {
        auto type = cppQueueType(plan, result);
        if (!type)
          return type.takeError();
        queueTypes[result.name] = *type;
      }
      for (const std::string &inputName : block.inputs) {
        const QueuePlan *input = findQueue(*specialization, inputName);
        const std::string type = queueTypes.lookup(inputName);
        if (type.empty())
          return generatorError("specialization transform input is missing");
        if (input && (input->lanes != 1 || input->rate != 1))
          return generatorError(
              "multi-interface specialization transform requires scalar rate");
        inputTypes.push_back(type);
      }
      for (const std::string &outputName : block.outputs) {
        const QueuePlan *result = findQueue(*specialization, outputName);
        const std::string type = queueTypes.lookup(outputName);
        if (type.empty())
          return generatorError("specialization transform output is missing");
        if (result && (result->lanes != 1 || result->rate != 1))
          return generatorError(
              "multi-interface specialization transform requires scalar rate");
        outputTypes.push_back(type);
      }
      const bool oneByOne = inputTypes.size() == 1 && outputTypes.size() == 1;
      output << "struct " << implementation << "_policy {\n  ";
      if (oneByOne) {
        output << outputTypes.front();
      } else {
        output << "std::tuple<";
        for (auto [index, type] : llvm::enumerate(outputTypes)) {
          if (index)
            output << ", ";
          output << type;
        }
        output << ">";
      }
      output << " operator()(";
      for (auto [index, type] : llvm::enumerate(inputTypes)) {
        if (index)
          output << ", ";
        output << "const " << type << " &item";
        if (index)
          output << index;
      }
      output << ") const {\n";
      if (oneByOne) {
        auto body =
            emitExpressionBody(*specialization, block, block.yields.front(), 4);
        if (!body)
          return body.takeError();
        output << *body;
      } else {
        output << "    return {\n";
        for (auto [index, yield] : llvm::enumerate(block.yields)) {
          output << "      [&]() -> " << outputTypes[index] << " {\n";
          auto body = emitExpressionBody(*specialization, block, yield, 8);
          if (!body)
            return body.takeError();
          output << *body << "      }()"
                 << (index + 1 == block.yields.size() ? "\n" : ",\n");
        }
        output << "    };\n";
      }
      llvm::StringMap<std::string> portParameters =
          interfaceParameterNames(*specialization);
      output << "  }\n};\n\n"
             << "class " << implementation
             << " final : public gfsim::Module {\npublic:\n  " << implementation
             << "(std::string name, gfsim::ObjectId block_id, "
                "gfsim::SimObject *parent";
      for (const QueueInterfacePlan &input : specialization->interfaceInputs)
        output << ", gfsim::SimQueue<" << queueTypes.lookup(input.name) << "> *"
               << portParameters.lookup(input.name);
      for (const QueueInterfacePlan &result : specialization->interfaceOutputs)
        output << ", gfsim::SimQueue<" << queueTypes.lookup(result.name)
               << "> *" << portParameters.lookup(result.name);
      output
          << ")\n      : gfsim::Module(std::move(name), "
             "gfsim::kInvalidObjectId, parent),\n        scope_(\""
          << localScope
          << "\", gfsim::kInvalidObjectId, this),\n        block_(\"transform_"
          << block.name << "\", block_id, &scope_, ";
      if (oneByOne) {
        output << "require_queue_port("
               << portParameters.lookup(block.inputs.front()) << ", \""
               << block.inputs.front() << "\"), require_queue_port("
               << portParameters.lookup(block.outputs.front()) << ", \""
               << block.outputs.front() << "\")";
      } else {
        output << "std::tuple{";
        for (auto [index, input] : llvm::enumerate(block.inputs)) {
          if (index)
            output << ", ";
          output << portParameters.lookup(input);
        }
        output << "}, std::tuple{";
        for (auto [index, result] : llvm::enumerate(block.outputs)) {
          if (index)
            output << ", ";
          output << portParameters.lookup(result);
        }
        output << '}';
      }
      output << ") {\n    attachChild(scope_);\n    "
                "scope_.attachChild(block_);\n  "
                "}\n\n"
             << "  gfsim::DispatchRow dispatch_row(size_t index) {\n"
             << "    return index == 0 ? gfsim::makeDispatchRow(&block_) : "
                "gfsim::DispatchRow{};\n  }\n\nprivate:\n"
             << "  gfsim::Module scope_;\n  ";
      if (oneByOne) {
        output << "gfsim::QueueTransform<" << inputTypes.front() << ", "
               << outputTypes.front() << ", " << implementation
               << "_policy, 1> block_;\n";
      } else {
        output << "gfsim::QueueAtomicTransform<" << implementation
               << "_policy, std::tuple<";
        for (auto [index, type] : llvm::enumerate(inputTypes)) {
          if (index)
            output << ", ";
          output << type;
        }
        output << ">, std::tuple<";
        for (auto [index, type] : llvm::enumerate(outputTypes)) {
          if (index)
            output << ", ";
          output << type;
        }
        output << ">> block_;\n";
      }
      output << "};\n\n";
      if (auto error = recordModule())
        return std::move(error);
      continue;
    }

    return generatorError(
        "structured specialization contains an unsupported block");
  }

  const std::size_t rootBegin = static_cast<std::size_t>(output.tellp());
  const auto rootRuntimeNames = queueRuntimeNames(plan);
  const std::string modelClass = className(plan.system);
  emitDefinitionProvenance(output, plan);
  output << "class " << modelClass
         << " final : public gfsim::Module {\npublic:\n  " << modelClass
         << "() : gfsim::Module(\"" << plan.system
         << "\", gfsim::kInvalidObjectId, nullptr),\n";
  std::vector<std::string> initializers;
  for (const std::string &scope : plan.scopes) {
    llvm::StringRef parent = llvm::StringRef(scope).rsplit('/').first;
    if (parent.empty())
      parent = "/";
    auto parentPointer = modulePointer(parent);
    if (!parentPointer)
      return parentPointer.takeError();
    appendInitializer(initializers, scopeMembers[scope], "(\"",
                      pathParts(scope).back(), "\", gfsim::kInvalidObjectId, ",
                      *parentPointer, ")");
  }
  for (const QueuePlan &queue : plan.queues) {
    auto type = cppQueueType(plan, queue);
    auto parent = modulePointer(queueOwners[queue.name]);
    if (!type)
      return type.takeError();
    if (!parent)
      return parent.takeError();
    appendInitializer(initializers, queueMembers[queue.name], "(\"",
                      rootRuntimeNames.lookup(queue.name),
                      "\", ", queueIds[queue.name], ", ", *parent, ", ",
                      queue.depth,
                      ", std::numeric_limits<size_t>::max(), nullptr, ",
                      queue.latency, ", ", queue.rate, ", ", queue.lanes, ")");
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto parent = modulePointer(block->scope);
    if (!parent)
      return parent.takeError();
    const std::string member = "block_" + std::to_string(index) + "_";
    const std::string instanceName = block->kind + "_" + block->name;
    if (block->kind == "broadcast") {
      const QueuePlan *input = findQueue(plan, block->inputs.front());
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(generatorError(
                              "structured broadcast input is missing"));
      if (!type)
        return type.takeError();
      std::string outputs;
      for (auto [outputIndex, name] : llvm::enumerate(block->outputs)) {
        if (outputIndex)
          outputs.append(", ");
        outputs.append("&").append(queueMembers[name]);
      }
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[block], ", ", *parent, ", ",
                        queueMembers[block->inputs.front()],
                        ", std::array<gfsim::SimQueue<", *type, "> *, ",
                        block->outputs.size(), ">{", outputs, "})");
    } else if (block->kind == "sink" || block->kind == "observe") {
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[block], ", ", *parent, ", ",
                        queueMembers[block->inputs.front()], ")");
    }
  }
  for (auto [index, initializer] : llvm::enumerate(initializers))
    output << "        " << initializer
           << (index + 1 == initializers.size() ? "\n" : ",\n");
  output << "  {\n    setPath(\"/" << plan.system << "\");\n";
  for (const std::string &scope : plan.scopes) {
    llvm::StringRef parent = llvm::StringRef(scope).rsplit('/').first;
    if (parent.empty())
      parent = "/";
    auto line = attach(parent, scopeMembers[scope]);
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (const QueuePlan &queue : plan.queues) {
    auto line = attach(queueOwners[queue.name], queueMembers[queue.name]);
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (auto [index, instance] : llvm::enumerate(plan.moduleInstances)) {
    const QueueGraphPlan *specialization =
        findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
    if (!specialization)
      return generatorError("structured module specialization is missing");
    auto parent = modulePointer(instance.scope);
    if (!parent)
      return parent.takeError();
    output << "    instance_" << index << "_ = std::make_unique<"
           << specializationClassName(*specialization) << ">(\""
           << identifier(instance.name) << "\"";
    for (uint64_t objectId : instanceObjectIds[index])
      output << ", " << objectId;
    output << ", " << *parent;
    for (const std::string &input : instance.inputs)
      output << ", &" << queueMembers[input];
    for (const std::string &outputName : instance.outputs)
      output << ", &" << queueMembers[outputName];
    output << ");\n";
    auto line =
        attach(instance.scope, "*instance_" + std::to_string(index) + "_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto line = attach(block->scope, "block_" + std::to_string(index) + "_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  output << "  }\n\n";
  for (const QueueInterfacePlan &input : plan.interfaceInputs) {
    const QueuePlan *queue = findQueue(plan, input.name);
    auto type = queue ? cppQueueType(plan, *queue)
                      : llvm::Expected<std::string>(
                            generatorError("structured input is missing"));
    if (!type)
      return type.takeError();
    const std::string publicName =
        identifier(input.displayName.empty() ? input.name : input.displayName);
    output << "  gfsim::SimQueue<" << *type << "> &" << publicName
           << "() { return " << queueMembers[input.name] << "; }\n"
           << "  bool offer_" << publicName
           << "(gfsim::SimSystem &system, " << *type << " value) {\n"
           << "    if (!" << queueMembers[input.name]
           << ".canProposePush() || !system.scheduleExternalXfer("
           << queueMembers[input.name] << ".id()))\n"
           << "      return false;\n"
           << "    return " << queueMembers[input.name]
           << ".proposePush(std::move(value));\n  }\n";
  }
  for (const QueueBlockPlan &block : plan.blocks)
    if (block.kind == "source") {
      const QueuePlan *queue = findQueue(plan, block.outputs.front());
      auto type = queue ? cppQueueType(plan, *queue)
                        : llvm::Expected<std::string>(
                              generatorError("structured source is missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::SimQueue<" << *type << "> &" << block.outputs.front()
             << "() { return " << queueMembers[block.outputs.front()] << "; }\n"
             << "  bool offer_" << identifier(block.outputs.front())
             << "(gfsim::SimSystem &system, " << *type << " value) {\n"
             << "    if (!" << queueMembers[block.outputs.front()]
             << ".canProposePush() || !system.scheduleExternalXfer("
             << queueMembers[block.outputs.front()] << ".id()))\n"
             << "      return false;\n"
             << "    return " << queueMembers[block.outputs.front()]
             << ".proposePush(std::move(value));\n  }\n";
    }
  for (auto [index, result] : llvm::enumerate(plan.interfaceOutputs)) {
    const QueuePlan *queue = findQueue(plan, result.name);
    auto type = queue ? cppQueueType(plan, *queue)
                      : llvm::Expected<std::string>(
                            generatorError("structured result is missing"));
    if (!type)
      return type.takeError();
    output << "  const gfsim::SimQueue<" << *type << "> &result_" << index
           << "() const { return " << queueMembers[result.name] << "; }\n"
           << "  std::optional<" << *type << "> try_take_result_" << index
           << "(gfsim::SimSystem &system) {\n"
           << "    if (!" << queueMembers[result.name]
           << ".canProposePop() || !system.scheduleExternalXfer("
           << queueMembers[result.name] << ".id()))\n"
           << "      return std::nullopt;\n"
           << "    return " << queueMembers[result.name]
           << ".proposePop();\n  }\n";
  }
  size_t sinkIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "sink") {
      const QueuePlan *queue = findQueue(plan, block->inputs.front());
      auto type = queue ? cppQueueType(plan, *queue)
                        : llvm::Expected<std::string>(
                              generatorError("structured sink is missing"));
      if (!type)
        return type.takeError();
      output << "  const std::vector<" << *type << "> &sink_" << sinkIndex
             << "_values() const { return block_" << index
             << "_.received(); }\n";
      ++sinkIndex;
    }
  output << "  void set_sink_retention_limit(size_t limit) {\n";
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "sink")
      output << "    block_" << index << "_.setRetentionLimit(limit);\n";
  output << "  }\n";
  using ArbitrationId = std::pair<uint64_t, uint64_t>;
  std::vector<ArbitrationId> arbitrationIds;
  for (const QueueBlockPlan *block : runtimeBlocks)
    if (!block->arbitrationMembership.empty())
      arbitrationIds.emplace_back(block->priority, blockIds[block]);
  std::function<void(const QueueGraphPlan &, llvm::ArrayRef<uint64_t>)>
      collectNestedArbitration;
  collectNestedArbitration = [&](const QueueGraphPlan &specialization,
                                 llvm::ArrayRef<uint64_t> objectIds) {
    const size_t internalQueues =
        specializationInternalQueueCount(specialization);
    for (auto [index, block] : llvm::enumerate(specialization.blocks))
      if (!block.arbitrationMembership.empty())
        arbitrationIds.emplace_back(block.priority,
                                    objectIds[internalQueues + index]);
    size_t offset = internalQueues + specialization.blocks.size() +
                    specialization.tables.size();
    for (const QueueModuleInstancePlan &instance :
         specialization.moduleInstances) {
      const QueueGraphPlan *child =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      if (!child)
        continue;
      const size_t count = specializationObjectCount(*child);
      collectNestedArbitration(*child, objectIds.slice(offset, count));
      offset += count;
    }
  };
  for (auto [index, instance] : llvm::enumerate(plan.moduleInstances)) {
    const QueueGraphPlan *specialization =
        findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
    if (specialization)
      collectNestedArbitration(*specialization, instanceObjectIds[index]);
  }
  llvm::sort(arbitrationIds);
  output << "\n  std::array<gfsim::DispatchRow, " << nextId
         << "> dispatch_rows() {\n    return {\n";
  for (const QueuePlan &queue : plan.queues)
    output << "        gfsim::makeDispatchRow(&" << queueMembers[queue.name]
           << "),\n";
  for (const DispatchItem &item : dispatchItems) {
    if (item.block) {
      const size_t index = static_cast<size_t>(
          llvm::find(runtimeBlocks, item.block) - runtimeBlocks.begin());
      output << "        gfsim::makeDispatchRow(&block_" << index << "_),\n";
    } else {
      const QueueModuleInstancePlan &instance =
          plan.moduleInstances[item.instanceIndex];
      const QueueGraphPlan *specialization =
          findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
      for (uint64_t index = 0;
           index < specializationObjectCount(*specialization); ++index)
        output << "        instance_" << item.instanceIndex << "_->dispatch_row("
               << index << "),\n";
    }
  }
  output << "    };\n  }\n\n"
         << "  static constexpr std::array<gfsim::ObjectId, "
         << arbitrationIds.size() << "> arbitration_order() {\n    return {";
  for (auto [index, entry] : llvm::enumerate(arbitrationIds)) {
    if (index)
      output << ", ";
    output << entry.second;
  }
  output << "};\n  }\n\n"
         << "  static constexpr std::array<uint32_t, "
         << activationOffsets.size()
         << "> activation_offsets() {\n    return {";
  for (auto [index, offset] : llvm::enumerate(activationOffsets)) {
    if (index)
      output << ", ";
    output << offset;
  }
  output << "};\n  }\n\n"
         << "  static constexpr bool activation_complete() { return "
         << (activationComplete ? "true" : "false") << "; }\n\n"
         << "  static constexpr std::array<gfsim::ObjectId, "
         << activationTargets.size()
         << "> activation_targets() {\n    return {";
  for (auto [index, target] : llvm::enumerate(activationTargets)) {
    if (index)
      output << ", ";
    output << target;
  }
  output << "};\n  }\n\n"
         << "  static constexpr std::array<uint32_t, "
         << workClosureOffsets.size()
         << "> work_closure_offsets() {\n    return {";
  for (auto [index, offset] : llvm::enumerate(workClosureOffsets)) {
    if (index)
      output << ", ";
    output << offset;
  }
  output << "};\n  }\n\n"
         << "  static constexpr std::array<gfsim::ObjectId, "
         << workClosureTargets.size()
         << "> work_closure_targets() {\n    return {";
  for (auto [index, target] : llvm::enumerate(workClosureTargets)) {
    if (index)
      output << ", ";
    output << target;
  }
  output << "};\n  }\n\n"
         << "  static constexpr std::array<gfsim::ObjectId, "
         << initialActivation.size() << "> initial_work_ids() {\n    return {";
  for (auto [index, target] : llvm::enumerate(initialActivation)) {
    if (index)
      output << ", ";
    output << target;
  }
  output << "};\n  }\n\n"
         << "  bool configure_activation_scheduler("
            "gfsim::SimSystem &system) {\n"
         << "    const auto rows = dispatch_rows();\n"
         << "    constexpr auto arbitration = arbitration_order();\n"
         << "    constexpr auto activationOffsets = activation_offsets();\n"
         << "    constexpr auto activationTargets = activation_targets();\n"
         << "    constexpr auto closureOffsets = work_closure_offsets();\n"
         << "    constexpr auto closureTargets = work_closure_targets();\n"
         << "    return system.setDispatchTable(rows) &&\n"
         << "           system.setArbitrationOrder(arbitration) &&\n"
         << "           system.setActivationPlan(activationOffsets, "
            "activationTargets) &&\n"
         << "           system.setWorkClosurePlan(closureOffsets, "
            "closureTargets) &&\n"
         << "           schedule_initial_work(system);\n  }\n\n"
         << "  static bool schedule_initial_work(gfsim::SimSystem &system) {\n"
         << "    for (gfsim::ObjectId id : initial_work_ids())\n"
         << "      if (!system.scheduleWork(id, system.currentEpoch()))\n"
         << "        return false;\n"
         << "    return true;\n  }\n\nprivate:\n";
  for (const std::string &scope : plan.scopes)
    output << "  gfsim::Module " << scopeMembers[scope] << ";\n";
  for (const QueuePlan &queue : plan.queues) {
    auto type = cppQueueType(plan, queue);
    if (!type)
      return type.takeError();
    output << "  gfsim::SimQueue<" << *type << "> " << queueMembers[queue.name]
           << ";\n";
  }
  for (auto [index, instance] : llvm::enumerate(plan.moduleInstances)) {
    const QueueGraphPlan *specialization =
        findFamilyCaseBody(plan, instance.definition, instance.staticArguments);
    output << "  std::unique_ptr<" << specializationClassName(*specialization)
           << "> instance_" << index << "_;\n";
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    const QueuePlan *input = findQueue(plan, block->inputs.front());
    auto type = input ? cppQueueType(plan, *input)
                      : llvm::Expected<std::string>(
                            generatorError("structured block input missing"));
    if (!type)
      return type.takeError();
    if (block->kind == "broadcast")
      output << "  gfsim::QueueBroadcast<" << *type << ", "
             << block->outputs.size() << "> block_" << index << "_;\n";
    else if (block->kind == "sink")
      output << "  gfsim::"
             << (input->lanes > 1 ? "QueueLaneSink<" : "QueueSink<") << *type
             << "> block_" << index << "_;\n";
    else if (block->kind == "observe")
      output << "  gfsim::QueueObserve<" << *type << "> block_" << index
             << "_;\n";
  }
  output << "};\n\n} // namespace ac_generated\n";
  StructuredQueueGraphCpp result;
  result.concatenated = output.str();
  result.types = std::move(typeUnits);
  result.helperDeclarations = helperDeclarations.str();
  result.helperDefinitions = helperDefinitions.str();
  for (const ModuleSpan &span : moduleSpans) {
    auto outlined = outlineModuleBody(
        llvm::StringRef(result.concatenated).slice(span.begin, span.end));
    if (!outlined)
      return outlined.takeError();
    result.modules.push_back(
        {span.fileStem, span.className, span.childFileStems, span.provenance,
         std::move(outlined->first), std::move(outlined->second)});
  }
  llvm::StringRef root(result.concatenated);
  root = root.drop_front(rootBegin);
  root.consume_back("} // namespace ac_generated\n");
  result.rootClass = root.str();
  return result;
}

} // namespace

llvm::Expected<std::string> generateQueueGraphCpp(const QueueGraphPlan &plan) {
  if (!plan.definition.empty()) {
    auto structured = generateStructuredQueueGraphCpp(plan);
    if (!structured)
      return structured.takeError();
    return structured->concatenated;
  }
  if (plan.system.empty() || plan.queues.empty() || plan.blocks.empty())
    return generatorError("QueueGraph plan is incomplete");
  if (!plan.scopes.empty()) {
    const QueueBlockContract *scope = findQueueBlockContract("scope");
    if (!scope || !scope->gfsimAvailable)
      return generatorError("official opcode has no gfsim lowering: 'scope'");
  }
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "table_read_group")
      continue;
    const QueueBlockContract *contract = findQueueBlockContract(block.kind);
    if (!contract || !contract->gfsimAvailable)
      return generatorError("official opcode has no gfsim lowering: '" +
                            block.kind + "'");
    if (block.kind == "reorder" &&
        (block.inputs.size() != 1 || block.outputs.size() != 1 ||
         block.yields.size() != 1 || block.capacity == 0))
      return generatorError("reorder contract is unsupported");
    if (block.kind == "dependency" &&
        (block.inputs.size() != 1 || block.outputs.size() != 1 ||
         block.yields.size() != 4 || block.capacity == 0 ||
         block.resources == 0))
      return generatorError("dependency contract is unsupported");
    if (block.kind == "credit" &&
        (block.inputs.size() != 1 || block.outputs.size() != 1 ||
         block.yields.size() != 1 || block.credits == 0))
      return generatorError("credit contract is unsupported");
    if (block.kind == "barrier" &&
        (block.inputs.size() < 2 ||
         block.outputs.size() != block.inputs.size() ||
         block.depths.size() != block.outputs.size() ||
         block.latencies.size() != block.outputs.size()))
      return generatorError("barrier contract is unsupported");
    if (block.kind == "select" &&
        (block.inputs.size() < 3 || block.outputs.size() != 1 ||
         block.yields.size() != 1))
      return generatorError("select contract is unsupported");
    if (block.kind == "expect" &&
        (block.inputs.size() != 1 || !block.outputs.empty() ||
         block.yields.size() != 1 || block.message.empty()))
      return generatorError("expect contract is unsupported");
    if (block.kind == "memory_request" &&
        (block.inputs.size() != 1 || block.outputs.size() != 1 ||
         block.yields.size() != 3 || block.memoryInstance.empty() ||
         block.resultField.empty()))
      return generatorError("memory contract is unsupported");
    if (block.kind == "table_read" &&
        (block.inputs.size() > 1 || block.outputs.size() != 1 ||
         block.yields.size() != 2 || block.table.empty()))
      return generatorError("table read contract is unsupported");
    if (block.kind == "table_write" &&
        (block.inputs.size() > 1 || !block.outputs.empty() ||
         block.yields.size() != 3 || block.table.empty() ||
         (block.writeMode != "field" && block.writeMode != "replace")))
      return generatorError("table write contract is unsupported");
    if (block.kind == "table_masked_write" &&
        (!block.inputs.empty() || !block.outputs.empty() ||
         block.yields.size() != 3 || block.table.empty() ||
         block.writeMode != "field"))
      return generatorError("masked table write contract is unsupported");
    if (block.kind == "firing") {
      const bool hasStateWrites = !block.stateWrites.empty();
      if (block.yields.size() != block.outputs.size() || block.guard.empty() ||
          (hasStateWrites &&
           (block.table.empty() || block.tableIndex.empty() ||
            block.tableValue.empty() || block.writeFields.empty())))
        return generatorError("table firing contract is unsupported");
    }
    if (block.kind == "slot" &&
        (block.inputs.size() != 1 || !block.outputs.empty() ||
         block.yields.size() != 1 || block.slot.empty()))
      return generatorError("slot contract is unsupported");
  }
  if (auto error = verifyQueueGraphPlan(plan))
    return std::move(error);

  llvm::StringMap<std::string> queueMembers;
  llvm::StringMap<std::string> queueOwners;
  for (const QueuePlan &queue : plan.queues) {
    if (queueMembers.contains(queue.name))
      return generatorError("Queue names must be unique");
    queueMembers[queue.name] = identifier(queue.name) + "_";
    queueOwners[queue.name] = queue.scope;
  }
  for (const QueueBlockPlan &block : plan.blocks)
    for (const std::string &input : block.inputs) {
      auto owner = queueOwners.find(input);
      if (owner == queueOwners.end())
        return generatorError("block input references unknown Queue '" + input +
                              "'");
      owner->getValue() = commonPath(owner->getValue(), block.scope);
    }

  llvm::StringMap<std::string> scopeMembers;
  for (auto [index, scope] : llvm::enumerate(plan.scopes))
    scopeMembers[scope] = "scope_" + std::to_string(index) + "_";
  auto modulePointer =
      [&](llvm::StringRef path) -> llvm::Expected<std::string> {
    if (path == "/")
      return std::string("this");
    auto found = scopeMembers.find(path);
    if (found == scopeMembers.end())
      return generatorError("unknown scope path '" + path + "'");
    return "&" + found->getValue();
  };
  auto attach = [&](llvm::StringRef path,
                    llvm::StringRef member) -> llvm::Expected<std::string> {
    if (path == "/")
      return "    attachChild(" + member.str() + ");";
    auto found = scopeMembers.find(path);
    if (found == scopeMembers.end())
      return generatorError("unknown attachment scope '" + path + "'");
    return "    " + found->getValue() + ".attachChild(" + member.str() + ");";
  };

  std::vector<const QueueBlockPlan *> runtimeBlocks;
  for (const QueueBlockPlan &block : plan.blocks)
    if (isRuntimeBlock(block) && block.kind != "memory_request")
      runtimeBlocks.push_back(&block);
  runtimeBlocks = arbitrationDispatchOrder(runtimeBlocks);
  std::vector<std::string> blockSymbols;
  llvm::StringSet<> usedBlockSymbols;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind != "firing") {
      blockSymbols.push_back("block_" + std::to_string(index));
      usedBlockSymbols.insert(blockSymbols.back());
      continue;
    }
    llvm::StringRef display = block->displayRuleName.empty()
                                  ? llvm::StringRef(block->name)
                                  : llvm::StringRef(block->displayRuleName);
    blockSymbols.push_back(
        uniqueIdentifier(("rule_" + display).str(), usedBlockSymbols));
  }
  auto blockSymbol = [&](size_t index) -> const std::string & {
    return blockSymbols[index];
  };
  llvm::StringMap<std::vector<const QueueBlockPlan *>> memoryEndpoints;
  for (const QueueBlockPlan &block : plan.blocks)
    if (block.kind == "memory_request")
      memoryEndpoints[block.memoryInstance].push_back(&block);
  for (auto &entry : memoryEndpoints)
    llvm::sort(entry.getValue(),
               [](const QueueBlockPlan *left, const QueueBlockPlan *right) {
                 return left->endpointOrdinal < right->endpointOrdinal;
               });
  llvm::StringMap<uint64_t> queueIds;
  for (auto [index, queue] : llvm::enumerate(plan.queues))
    queueIds[queue.name] = index;
  uint64_t nextId = plan.queues.size();
  llvm::DenseMap<size_t, uint64_t> feedbackStateIds;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "feedback")
      feedbackStateIds[index] = nextId++;
  llvm::StringMap<uint64_t> blockIds;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    blockIds[block->name + "#" + std::to_string(index)] = nextId++;
  llvm::StringMap<uint64_t> memoryIds;
  for (const MemoryInstancePlan &instance : plan.memoryInstances)
    memoryIds[instance.name] = nextId++;
  llvm::StringMap<uint64_t> tableIds;
  llvm::StringMap<std::string> tableMembers;
  llvm::StringSet<> usedTableMembers;
  for (const TablePlan &table : plan.tables) {
    tableIds[table.name] = nextId++;
    tableMembers[table.name] =
        uniqueIdentifier("state_" + table.name, usedTableMembers) + "_";
  }

  std::ostringstream output;
  output << "// Generated from verified ACIR QueueGraph plan; do not edit.\n";
  output << "#include \"gfsim/bits.h\"\n"
            "#include \"gfsim/dispatch.h\"\n"
            "#include \"gfsim/object.h\"\n"
            "#include \"gfsim/count_zeros.h\"\n"
            "#include \"gfsim/popcount.h\"\n"
            "#include \"gfsim/priority_encode.h\"\n"
            "#include \"gfsim/queue.h\"\n"
            "#include \"gfsim/queue_blocks.h\"\n\n"
            "#include <array>\n#include <cstdint>\n#include <limits>\n"
            "#include <optional>\n#include <tuple>\n\n"
            "namespace ac_generated {\n\n";
  for (const QueueEnumPlan &enumeration : plan.enums) {
    output << "enum class " << enumeration.name << " : "
           << enumStorage(enumeration.width).str() << " {\n";
    for (auto [index, enumerant] : llvm::enumerate(enumeration.enumerants))
      output << "  " << enumerant << " = "
             << (enumeration.values.empty() ? index : enumeration.values[index])
             << ",\n";
    output << "};\n\n";
  }
  auto payloadOrder = payloadEmissionOrder(plan);
  if (!payloadOrder)
    return payloadOrder.takeError();
  for (const QueuePayloadPlan *payload : *payloadOrder) {
    output << "struct " << payload->name << " {\n";
    for (const QueuePayloadFieldPlan &field : payload->fields) {
      auto type = cppPayloadFieldType(plan, field);
      if (!type)
        return type.takeError();
      output << "  " << *type << ' ' << identifier(field.name) << "{};\n";
    }
    output << "  bool operator==(const " << payload->name
           << " &) const = default;\n";
    output << "};\n\n";
  }

  if (auto error = emitHelperDefinitions(output, plan))
    return std::move(error);

  for (const TableMatchPlan &match : plan.tableMatches) {
    const TablePlan *table = findTable(plan, match.table);
    auto entryType = table ? cppType(table->entryType)
                           : llvm::Expected<std::string>(
                                 generatorError("table.match Table missing"));
    if (!entryType)
      return entryType.takeError();
    QueueBlockPlan predicate;
    predicate.expressions = match.expressions;
    predicate.yields = {match.yield};
    auto body = emitExpressionBody(plan, predicate, match.yield, 4);
    if (!body)
      return body.takeError();
    output << "struct " << identifier(match.name) << "_predicate_policy {\n"
           << "  gfsim::SimTable<" << *entryType << "> *table{};\n";
    for (const SlotPlan &slot : plan.slots) {
      auto type = cppType(slot.payloadType);
      if (!type)
        return type.takeError();
      output << "  gfsim::SlotState<" << *type << "> *slot_"
             << identifier(slot.name) << "{};\n";
    }
    output << "  bool operator()(const " << *entryType << " &item) const {\n"
           << *body << "  }\n};\n"
           << "using " << identifier(match.name) << "_cache = "
           << "gfsim::TableMatchCache<" << *entryType << ", "
           << identifier(match.name) << "_predicate_policy>;\n\n";
  }
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    const TablePlan *table = findTable(plan, selection.table);
    auto entryType = table ? cppType(table->entryType)
                           : llvm::Expected<std::string>(
                                 generatorError("table.choose Table missing"));
    if (!entryType)
      return entryType.takeError();
    output << "struct " << identifier(selection.name) << "_mask_policy {\n"
           << "  " << identifier(selection.match) << "_cache *match{};\n"
           << "  const gfsim::CandidateSet &operator()(gfsim::Epoch epoch) "
              "const {\n"
           << "    return match->get(epoch);\n  }\n};\n";
    output << "struct " << identifier(selection.name) << "_key_policy {\n"
           << "  auto operator()(const " << *entryType << " &item) const {\n";
    if (selection.policy == "first" || selection.policy == "round_robin") {
      output << "    return std::uint64_t{0};\n";
    } else {
      QueueBlockPlan key;
      key.expressions = selection.keyExpressions;
      key.yields = {selection.keyYield};
      output << "    return [&]() {\n";
      auto body = emitExpressionBody(plan, key, selection.keyYield, 6);
      if (!body)
        return body.takeError();
      output << *body;
      output << "    }();\n";
    }
    output << "  }\n};\n"
           << "using " << identifier(selection.name) << "_cache = "
           << (selection.count == 1 && selection.policy != "round_robin"
                   ? "gfsim::TableSelectionCache<"
                   : "gfsim::TableMultiSelectionCache<")
           << *entryType << ", " << identifier(selection.name)
           << "_mask_policy, " << identifier(selection.name) << "_key_policy";
    if (selection.count != 1 || selection.policy == "round_robin")
      output << ", " << selection.count;
    output << ">;\n"
           << "inline constexpr auto " << identifier(selection.name)
           << "_choose_policy = gfsim::TableChoosePolicy::"
           << (selection.policy == "first" ? "First"
               : selection.policy == "min" ? "Min"
               : selection.policy == "max" ? "Max"
                                           : "RoundRobin")
           << ";\n"
           << "inline constexpr auto " << identifier(selection.name)
           << "_key_ordering = gfsim::TableKeyOrdering::"
           << (selection.keyOrdering == "signed" ? "Signed" : "Unsigned")
           << ";\n\n";
  }

  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind != "transform" && block->kind != "route" &&
        block->kind != "select" && block->kind != "expect" &&
        block->kind != "dependency" && block->kind != "credit" &&
        block->kind != "reorder" && block->kind != "feedback" &&
        block->kind != "table_read" && block->kind != "table_read_group" &&
        block->kind != "table_write" && block->kind != "table_masked_write" &&
        block->kind != "firing" && block->kind != "slot")
      continue;
    if (block->kind == "slot") {
      const SlotPlan *slot = findSlot(plan, block->slot);
      auto payloadType = slot ? cppType(slot->payloadType)
                              : llvm::Expected<std::string>(
                                    generatorError("slot declaration missing"));
      if (!payloadType)
        return payloadType.takeError();
      output << "struct block_" << index << "_release_policy {\n";
      for (const SlotPlan &candidate : plan.slots) {
        auto type = cppType(candidate.payloadType);
        if (!type)
          return type.takeError();
        output << "  gfsim::SlotState<" << *type << "> *slot_"
               << identifier(candidate.name) << "{};\n";
      }
      for (const TablePlan &table : plan.tables) {
        if (!referencesTable(block->expressions, table.name))
          continue;
        auto type = cppType(table.entryType);
        if (!type)
          return type.takeError();
        output << "  gfsim::SimTable<" << *type << "> *table_"
               << identifier(table.name) << "{};\n";
      }
      for (const TableMatchPlan &match : plan.tableMatches)
        output << "  " << identifier(match.name) << "_cache *"
               << identifier(match.name) << "{};\n";
      for (const TableSelectionPlan &selection : plan.tableSelections)
        output << "  " << identifier(selection.name) << "_cache *"
               << identifier(selection.name) << "{};\n";
      output << "  bool operator()(gfsim::Epoch epoch) const {\n";
      auto body =
          emitExpressionBody(plan, *block, block->yields.front(), 4, true);
      if (!body)
        return body.takeError();
      output << *body << "  }\n};\n\n";
      continue;
    }
    if (block->kind == "firing") {
      const std::vector<const TablePlan *> ownerTables =
          stateOwnerTables(plan, *block);
      if (ownerTables.size() != 1 || block->stateWrites.empty() ||
          !block->slotReleases.empty()) {
        const std::vector<const TablePlan *> readTables =
            readOnlyTables(plan, *block);
        std::vector<std::string> tableTypes;
        for (const TablePlan *table : ownerTables) {
          auto type = table ? cppType(table->entryType)
                            : llvm::Expected<std::string>(generatorError(
                                  "state firing owner Table missing"));
          if (!type)
            return type.takeError();
          tableTypes.push_back(std::move(*type));
        }
        std::vector<std::string> inputTypes;
        for (const std::string &inputName : block->inputs) {
          const QueuePlan *input = findQueue(plan, inputName);
          auto type = input ? cppQueueType(plan, *input)
                            : llvm::Expected<std::string>(
                                  generatorError("state firing input missing"));
          if (!type)
            return type.takeError();
          inputTypes.push_back(std::move(*type));
        }
        std::vector<std::string> outputTypes;
        for (const std::string &outputName : block->outputs) {
          const QueuePlan *result = findQueue(plan, outputName);
          auto type = result ? cppQueueType(plan, *result)
                             : llvm::Expected<std::string>(generatorError(
                                   "state firing output missing"));
          if (!type)
            return type.takeError();
          outputTypes.push_back(std::move(*type));
        }
        QueueBlockPlan evaluation = *block;
        std::vector<std::string> additional{block->guard};
        std::string tupleResult = "std::tuple{";
        bool tupleHasValue = false;
        auto appendTupleValue = [&](llvm::StringRef value) {
          if (tupleHasValue)
            tupleResult.append(", ");
          tupleResult.append(value);
          tupleHasValue = true;
        };
        for (auto [writeIndex, write] : llvm::enumerate(block->stateWrites)) {
          (void)writeIndex;
          appendTupleValue(write.index);
          appendTupleValue(write.value);
          appendTupleValue(write.present);
          additional.push_back(write.index);
          additional.push_back(write.value);
          additional.push_back(write.present);
          if (!write.versionedAction.empty()) {
            appendTupleValue(write.refGeneration);
            appendTupleValue(write.refEpoch);
            additional.push_back(write.refGeneration);
            additional.push_back(write.refEpoch);
            if (!write.refAttempt.empty()) {
              appendTupleValue(write.refAttempt);
              additional.push_back(write.refAttempt);
            }
          }
        }
        for (auto [outputIndex, yield] : llvm::enumerate(block->yields)) {
          const std::string &present =
              block->outputPresence[outputIndex].present;
          appendTupleValue(yield);
          appendTupleValue(present);
          additional.push_back(yield);
          additional.push_back(present);
        }
        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables)) {
          size_t reservationIndex = 0;
          for (const StateReservationPlan *reservation :
               findStateReservations(*block, table->name)) {
            if (reservation->indexKind == "all") {
              ++reservationIndex;
              continue;
            }
            if (reservation->indexKind == "set") {
              const std::string result = "snapshot_set_" +
                                         std::to_string(ownerIndex) + "_" +
                                         std::to_string(reservationIndex++);
              auto fieldMask =
                  reservationFieldMask(plan, *table, reservation->fields);
              if (!fieldMask)
                return fieldMask.takeError();
              QueueExpressionPlan expression{
                  result, "snapshot_set", "state_reservation", {}};
              expression.field = reservation->source;
              expression.table = reservation->table;
              expression.predicate =
                  fieldMask->complete ? "complete" : "fields";
              expression.mask = std::to_string(fieldMask->mask);
              expression.width = fieldMask->count;
              evaluation.expressions.push_back(std::move(expression));
              appendTupleValue(result);
              additional.push_back(result);
              continue;
            }
            ++reservationIndex;
            appendTupleValue(reservation->index);
            additional.push_back(reservation->index);
          }
        }
        for (const SlotReleaseEffectPlan &release : block->slotReleases) {
          appendTupleValue(release.when);
          additional.push_back(release.when);
        }
        appendTupleValue(block->guard);
        tupleResult.push_back('}');
        const std::string &primaryValue =
            !block->stateWrites.empty() ? block->stateWrites.front().index
            : !block->yields.empty()    ? block->yields.front()
                                        : block->guard;
        auto evaluationBody =
            emitExpressionBody(plan, evaluation, primaryValue, 6, true, false,
                               additional, tupleResult);
        if (!evaluationBody)
          return evaluationBody.takeError();

        std::string tableTuple = "std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(tableTypes)) {
          if (typeIndex)
            tableTuple.append(", ");
          tableTuple.append(type);
        }
        tableTuple.push_back('>');
        std::string outputTuple = "std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(outputTypes)) {
          if (typeIndex)
            outputTuple.append(", ");
          outputTuple.append(type);
        }
        outputTuple.push_back('>');
        const std::string planType = "gfsim::StateTransitionPlan<" +
                                     tableTuple + ", " + outputTuple + ">";
        emitRuleProvenance(output, *block);
        output << "struct " << blockSymbol(index) << "_policy {\n";
        for (const TablePlan *readTable : readTables) {
          auto readType = cppType(readTable->entryType);
          if (!readType)
            return readType.takeError();
          output << "  const gfsim::SimTable<" << *readType << "> *read_table_"
                 << identifier(readTable->name) << "{};\n";
        }
        for (const SlotPlan &slot : plan.slots) {
          auto type = cppType(slot.payloadType);
          if (!type)
            return type.takeError();
          output << "  const gfsim::SlotState<" << *type << "> *slot_"
                 << identifier(slot.name) << "{};\n";
        }
        for (const TableMatchPlan &match : plan.tableMatches)
          output << "  " << identifier(match.name) << "_cache *"
                 << identifier(match.name) << "{};\n";
        for (const TableSelectionPlan &selection : plan.tableSelections)
          output << "  " << identifier(selection.name) << "_cache *"
                 << identifier(selection.name) << "{};\n";
        output << "  std::optional<" << planType
               << "> operator()(gfsim::Epoch epoch, std::tuple<";
        for (auto [typeIndex, type] : llvm::enumerate(tableTypes)) {
          if (typeIndex)
            output << ", ";
          output << "const gfsim::SimTable<" << type << "> *";
        }
        output << "> table_refs";
        for (auto [inputIndex, type] : llvm::enumerate(inputTypes)) {
          output << ", const " << type << " &item";
          if (inputIndex)
            output << inputIndex;
        }
        output << ") const {\n";
        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables))
          output << "    const auto *table_" << identifier(table->name)
                 << " = std::get<" << ownerIndex << ">(table_refs);\n";
        for (const TablePlan *readTable : readTables)
          output << "    const auto *table_" << identifier(readTable->name)
                 << " = read_table_" << identifier(readTable->name) << ";\n";
        output << "    auto [";
        bool bindingHasValue = false;
        for (size_t writeIndex = 0; writeIndex < block->stateWrites.size();
             ++writeIndex) {
          if (bindingHasValue)
            output << ", ";
          output << stateWriteIndexName(*block, writeIndex) << ", "
                 << stateWriteValueName(*block, writeIndex) << ", "
                 << stateWritePresentName(*block, writeIndex);
          const StateWritePlan &write = block->stateWrites[writeIndex];
          if (!write.versionedAction.empty()) {
            output << ", "
                   << stateWriteRefGenerationName(*block, writeIndex) << ", "
                   << stateWriteRefEpochName(*block, writeIndex);
            if (!write.refAttempt.empty())
              output << ", "
                     << stateWriteRefAttemptName(*block, writeIndex);
          }
          bindingHasValue = true;
        }
        for (size_t outputIndex = 0; outputIndex < outputTypes.size();
             ++outputIndex) {
          if (bindingHasValue)
            output << ", ";
          output << outputValueName(*block, outputIndex) << ", "
                 << outputPresentName(*block, outputIndex);
          bindingHasValue = true;
        }
        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables)) {
          size_t reservationIndex = 0;
          for (const StateReservationPlan *reservation :
               findStateReservations(*block, table->name)) {
            if (reservation->indexKind == "all") {
              ++reservationIndex;
              continue;
            }
            if (bindingHasValue)
              output << ", ";
            output << reservationBindingName(table->name, *reservation,
                                             reservationIndex++);
            bindingHasValue = true;
          }
        }
        for (size_t releaseIndex = 0; releaseIndex < block->slotReleases.size();
             ++releaseIndex) {
          if (bindingHasValue)
            output << ", ";
          output << "slot_release_" << releaseIndex;
          bindingHasValue = true;
        }
        if (bindingHasValue)
          output << ", ";
        output << "rule_condition] = [&]() {\n"
               << *evaluationBody << "    }();\n"
               << "    if (!rule_condition)\n"
               << "      return std::nullopt;\n";
        emitVersionedWriteQualification(output, *block, plan, "    ");
        if (auto error =
                emitArchitectureObligationChecks(output, plan, *block, "    "))
          return error;
        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables))
          emitStateWriteBatch(output, *block, table->name, plan,
                              tableTypes[ownerIndex], ownerIndex, "    ");
        output << "    return " << planType << "{{";
        for (size_t ownerIndex = 0; ownerIndex < tableTypes.size();
             ++ownerIndex) {
          if (ownerIndex)
            output << ", ";
          output << "std::move("
                 << stateWriteBatchName(ownerTables[ownerIndex]->name) << ")";
        }
        output << "}, {";
        for (auto [outputIndex, type] : llvm::enumerate(outputTypes)) {
          if (outputIndex)
            output << ", ";
          output << outputPresentName(*block, outputIndex)
                 << " ? std::optional<" << type << ">{"
                 << outputValueName(*block, outputIndex) << "} : std::optional<"
                 << type << ">{}";
        }
        output << "}, {";
        for (size_t ownerIndex = 0; ownerIndex < tableTypes.size();
             ++ownerIndex) {
          if (ownerIndex)
            output << ", ";
          const TablePlan &table = *ownerTables[ownerIndex];
          output << "gfsim::StateReservation{}";
          size_t reservationIndex = 0;
          for (const StateReservationPlan *reservation :
               findStateReservations(*block, table.name)) {
            auto fieldMask =
                reservationFieldMask(plan, table, reservation->fields);
            if (!fieldMask)
              return fieldMask.takeError();
            if (reservation->indexKind == "all") {
              output << " | "
                     << (fieldMask->complete
                             ? "gfsim::StateReservation::all()"
                             : "gfsim::StateReservation::forAllFields("
                               "std::uint64_t{" +
                                   std::to_string(fieldMask->mask) + "}, " +
                                   std::to_string(fieldMask->count) + ")");
              ++reservationIndex;
            } else if (reservation->indexKind == "set") {
              output << " | "
                     << reservationBindingName(table.name, *reservation,
                                               reservationIndex++);
            } else {
              output << " | "
                     << (fieldMask->complete
                             ? "gfsim::StateReservation::forEntry("
                             : "gfsim::StateReservation::forFieldsAt(")
                     << "static_cast<std::size_t>("
                     << reservationBindingName(table.name, *reservation,
                                               reservationIndex++)
                     << ")";
              if (fieldMask->complete)
                output << ")";
              else
                output << ", std::uint64_t{" << fieldMask->mask << "}, "
                       << fieldMask->count << ")";
            }
          }
        }
        output << "}, {";
        for (size_t releaseIndex = 0; releaseIndex < block->slotReleases.size();
             ++releaseIndex) {
          if (releaseIndex)
            output << ", ";
          output << "slot_release_" << releaseIndex;
        }
        output << "}};\n  }\n";
        output << "  void accepted(gfsim::Epoch epoch) {\n";
        for (const TableSelectionPlan &selection : plan.tableSelections)
          if (selection.policy == "round_robin" &&
              referencesSelection(block->expressions, selection.name))
            output << "    " << identifier(selection.name)
                   << "->accept(epoch);\n";
        output << "  }\n};\n\n";

        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables)) {
          const std::string &entryType = tableTypes[ownerIndex];
          const StateWritePlan *write = findStateWrite(*block, table->name);
          const std::vector<std::string> fields =
              write ? write->fields : std::vector<std::string>{"$entry"};
          if (auto error = emitStructuredMergePolicy(
                  output, plan,
                  blockSymbol(index) + "_merge_policy_" +
                      std::to_string(ownerIndex),
                  entryType, fields))
            return std::move(error);
        }
        continue;
      }
      const TablePlan *table = ownerTables.front();
      auto entryType = cppType(table->entryType);
      if (!entryType)
        return entryType.takeError();
      const StateWritePlan *ownerWrite = findStateWrite(*block, block->table);
      const std::vector<std::string> &ownerWriteFields =
          ownerWrite ? ownerWrite->fields : block->writeFields;
      const std::vector<const TablePlan *> readTables =
          readOnlyTables(plan, *block);
      std::vector<std::string> inputTypes;
      for (const std::string &inputName : block->inputs) {
        const QueuePlan *input = findQueue(plan, inputName);
        auto inputType = input ? cppQueueType(plan, *input)
                               : llvm::Expected<std::string>(generatorError(
                                     "table firing input missing"));
        if (!inputType)
          return inputType.takeError();
        inputTypes.push_back(std::move(*inputType));
      }
      std::vector<std::string> outputTypes;
      for (const std::string &outputName : block->outputs) {
        const QueuePlan *result = findQueue(plan, outputName);
        auto outputType = result ? cppQueueType(plan, *result)
                                 : llvm::Expected<std::string>(generatorError(
                                       "table firing output missing"));
        if (!outputType)
          return outputType.takeError();
        outputTypes.push_back(std::move(*outputType));
      }
      QueueBlockPlan evaluation = *block;
      std::vector<std::string> additional{block->guard};
      std::string tupleResult = "std::tuple{";
      bool tupleHasValue = false;
      for (const StateWritePlan &write : block->stateWrites) {
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(write.index)
            .append(", ")
            .append(write.value)
            .append(", ")
            .append(write.present);
        tupleHasValue = true;
        additional.push_back(write.index);
        additional.push_back(write.value);
        additional.push_back(write.present);
        if (!write.versionedAction.empty()) {
          tupleResult.append(", ").append(write.refGeneration)
              .append(", ")
              .append(write.refEpoch);
          additional.push_back(write.refGeneration);
          additional.push_back(write.refEpoch);
          if (!write.refAttempt.empty()) {
            tupleResult.append(", ").append(write.refAttempt);
            additional.push_back(write.refAttempt);
          }
        }
      }
      for (auto [outputIndex, yield] : llvm::enumerate(block->yields)) {
        const std::string &present = block->outputPresence[outputIndex].present;
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(yield).append(", ").append(present);
        tupleHasValue = true;
        additional.push_back(yield);
        additional.push_back(present);
      }
      size_t snapshotOrdinal = 0;
      for (const StateReservationPlan *reservation :
           findStateReservations(*block, block->table)) {
        if (reservation->indexKind == "all") {
          ++snapshotOrdinal;
          continue;
        }
        if (reservation->indexKind == "set") {
          const std::string result =
              "snapshot_set_0_" + std::to_string(snapshotOrdinal++);
          auto fieldMask =
              reservationFieldMask(plan, *table, reservation->fields);
          if (!fieldMask)
            return fieldMask.takeError();
          QueueExpressionPlan expression{
              result, "snapshot_set", "state_reservation", {}};
          expression.field = reservation->source;
          expression.table = reservation->table;
          expression.predicate = fieldMask->complete ? "complete" : "fields";
          expression.mask = std::to_string(fieldMask->mask);
          expression.width = fieldMask->count;
          evaluation.expressions.push_back(std::move(expression));
          if (tupleHasValue)
            tupleResult.append(", ");
          tupleResult.append(result);
          tupleHasValue = true;
          additional.push_back(result);
          continue;
        }
        ++snapshotOrdinal;
        if (tupleHasValue)
          tupleResult.append(", ");
        tupleResult.append(reservation->index);
        tupleHasValue = true;
        additional.push_back(reservation->index);
      }
      tupleResult.append(", ").append(block->guard);
      tupleResult.push_back('}');
      auto evaluationBody = emitExpressionBody(
          plan, evaluation,
          !block->stateWrites.empty() ? block->stateWrites.front().index
          : !block->yields.empty()    ? block->yields.front()
                                      : block->guard,
          6, true, false, additional, tupleResult);
      if (!evaluationBody)
        return evaluationBody.takeError();
      std::string planType = "gfsim::TableTransitionPlan<" + *entryType;
      for (const std::string &outputType : outputTypes)
        planType.append(", ").append(outputType);
      planType.push_back('>');
      emitRuleProvenance(output, *block);
      output << "struct " << blockSymbol(index) << "_policy {\n";
      for (const TablePlan *readTable : readTables) {
        auto readType = cppType(readTable->entryType);
        if (!readType)
          return readType.takeError();
        output << "  const gfsim::SimTable<" << *readType << "> *read_table_"
               << identifier(readTable->name) << "{};\n";
      }
      for (const TableMatchPlan &match : plan.tableMatches)
        output << "  " << identifier(match.name) << "_cache *"
               << identifier(match.name) << "{};\n";
      for (const TableSelectionPlan &selection : plan.tableSelections)
        output << "  " << identifier(selection.name) << "_cache *"
               << identifier(selection.name) << "{};\n";
      output << "  std::optional<" << planType
             << "> operator()(gfsim::Epoch epoch, "
             << "const gfsim::SimTable<" << *entryType << "> &table_ref";
      for (auto [inputIndex, inputType] : llvm::enumerate(inputTypes)) {
        output << ", const " << inputType << " &item";
        if (inputIndex)
          output << inputIndex;
      }
      output << ") const {\n"
             << "    const auto *table_" << identifier(table->name)
             << " = &table_ref;\n";
      for (const TablePlan *readTable : readTables)
        output << "    const auto *table_" << identifier(readTable->name)
               << " = read_table_" << identifier(readTable->name) << ";\n";
      output << "    auto [";
      bool bindingHasValue = false;
      for (size_t writeIndex = 0; writeIndex < block->stateWrites.size();
           ++writeIndex) {
        if (bindingHasValue)
          output << ", ";
        output << stateWriteIndexName(*block, writeIndex) << ", "
               << stateWriteValueName(*block, writeIndex) << ", "
               << stateWritePresentName(*block, writeIndex);
        const StateWritePlan &write = block->stateWrites[writeIndex];
        if (!write.versionedAction.empty()) {
          output << ", "
                 << stateWriteRefGenerationName(*block, writeIndex) << ", "
                 << stateWriteRefEpochName(*block, writeIndex);
          if (!write.refAttempt.empty())
            output << ", " << stateWriteRefAttemptName(*block, writeIndex);
        }
        bindingHasValue = true;
      }
      for (size_t outputIndex = 0; outputIndex < outputTypes.size();
           ++outputIndex) {
        if (bindingHasValue)
          output << ", ";
        output << outputValueName(*block, outputIndex) << ", "
               << outputPresentName(*block, outputIndex);
        bindingHasValue = true;
      }
      size_t reservationIndex = 0;
      for (const StateReservationPlan *reservation :
           findStateReservations(*block, table->name)) {
        if (reservation->indexKind == "all") {
          ++reservationIndex;
          continue;
        }
        if (bindingHasValue)
          output << ", ";
        output << reservationBindingName(table->name, *reservation,
                                         reservationIndex++);
        bindingHasValue = true;
      }
      output << ", rule_condition] = [&]() {\n"
             << *evaluationBody << "    }();\n"
             << "    if (!rule_condition)\n"
             << "      return std::nullopt;\n";
      emitVersionedWriteQualification(output, *block, plan, "    ");
      if (auto error =
              emitArchitectureObligationChecks(output, plan, *block, "    "))
        return error;
      emitStateWriteBatch(output, *block, table->name, plan, *entryType, 0,
                          "    ");
      output << "    return " << planType << "{std::move("
             << stateWriteBatchName(table->name) << "), {";
      for (auto [outputIndex, outputType] : llvm::enumerate(outputTypes)) {
        if (outputIndex)
          output << ", ";
        output << outputPresentName(*block, outputIndex) << " ? std::optional<"
               << outputType << ">{" << outputValueName(*block, outputIndex)
               << "} : std::optional<" << outputType << ">{}";
      }
      output << "}, gfsim::StateReservation{}";
      reservationIndex = 0;
      for (const StateReservationPlan *reservation :
           findStateReservations(*block, table->name)) {
        auto fieldMask =
            reservationFieldMask(plan, *table, reservation->fields);
        if (!fieldMask)
          return fieldMask.takeError();
        if (reservation->indexKind == "all") {
          output << " | "
                 << (fieldMask->complete
                         ? "gfsim::StateReservation::all()"
                         : "gfsim::StateReservation::forAllFields("
                           "std::uint64_t{" +
                               std::to_string(fieldMask->mask) + "}, " +
                               std::to_string(fieldMask->count) + ")");
          ++reservationIndex;
        } else if (reservation->indexKind == "set") {
          output << " | "
                 << reservationBindingName(table->name, *reservation,
                                           reservationIndex++);
        } else {
          output << " | "
                 << (fieldMask->complete
                         ? "gfsim::StateReservation::forEntry("
                         : "gfsim::StateReservation::forFieldsAt(")
                 << "static_cast<std::size_t>("
                 << reservationBindingName(table->name, *reservation,
                                           reservationIndex++)
                 << ")";
          if (fieldMask->complete)
            output << ")";
          else
            output << ", std::uint64_t{" << fieldMask->mask << "}, "
                   << fieldMask->count << ")";
        }
      }
      output << "};\n  }\n";
      output << "  void accepted(gfsim::Epoch epoch) {\n";
      for (const TableSelectionPlan &selection : plan.tableSelections)
        if (selection.policy == "round_robin" &&
            referencesSelection(block->expressions, selection.name))
          output << "    " << identifier(selection.name)
                 << "->accept(epoch);\n";
      output << "  }\n};\n\n";
      if (auto error = emitStructuredMergePolicy(
              output, plan, blockSymbol(index) + "_merge_policy", *entryType,
              ownerWriteFields))
        return std::move(error);
      continue;
    }
    if (block->kind == "table_read" || block->kind == "table_write" ||
        block->kind == "table_masked_write") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(
                                   generatorError("table declaration missing"));
      if (!entryType)
        return entryType.takeError();
      std::string inputType;
      if (!block->inputs.empty()) {
        const QueuePlan *input = findQueue(plan, block->inputs.front());
        auto type = input ? cppQueueType(plan, *input)
                          : llvm::Expected<std::string>(
                                generatorError("table input Queue missing"));
        if (!type)
          return type.takeError();
        inputType = std::move(*type);
      }
      const std::vector<llvm::StringRef> policyNames =
          block->kind == "table_read"
              ? std::vector<llvm::StringRef>{"address", "when"}
          : block->kind == "table_masked_write"
              ? std::vector<llvm::StringRef>{"mask", "enable", "value"}
              : std::vector<llvm::StringRef>{"address", "enable", "value"};
      for (auto [policyIndex, policyName] : llvm::enumerate(policyNames)) {
        llvm::StringRef resultType = table->entryType;
        if (block->yields[policyIndex] != "item") {
          auto expression = std::find_if(
              block->expressions.begin(), block->expressions.end(),
              [&](const QueueExpressionPlan &candidate) {
                return candidate.result == block->yields[policyIndex];
              });
          if (expression == block->expressions.end())
            return generatorError("table policy yield type is missing");
          resultType = expression->type;
        } else if (!block->inputs.empty()) {
          const QueuePlan *input = findQueue(plan, block->inputs.front());
          resultType = input->payloadType;
        }
        auto resultCppType = cppType(resultType);
        if (!resultCppType)
          return resultCppType.takeError();
        if (block->yields[policyIndex] != "item") {
          auto expression = llvm::find_if(
              block->expressions, [&](const QueueExpressionPlan &candidate) {
                return candidate.result == block->yields[policyIndex];
              });
          if (expression != block->expressions.end() &&
              expression->kind == "table_index")
            *resultCppType = "std::size_t";
        }
        output << "struct block_" << index << '_' << policyName.str()
               << "_policy {\n  gfsim::SimTable<" << *entryType
               << "> *table{};\n";
        for (const SlotPlan &slot : plan.slots) {
          auto type = cppType(slot.payloadType);
          if (!type)
            return type.takeError();
          output << "  gfsim::SlotState<" << *type << "> *slot_"
                 << identifier(slot.name) << "{};\n";
        }
        for (const TableMatchPlan &match : plan.tableMatches)
          output << "  " << identifier(match.name) << "_cache *"
                 << identifier(match.name) << "{};\n";
        for (const TableSelectionPlan &selection : plan.tableSelections)
          output << "  " << identifier(selection.name) << "_cache *"
                 << identifier(selection.name) << "{};\n";
        output << "  " << *resultCppType << " operator()(gfsim::Epoch epoch";
        if (!inputType.empty())
          output << ", const " << inputType << " &item";
        else if (block->kind == "table_masked_write" && policyName == "value")
          output << ", const " << *entryType << " &item";
        output << ") const {\n";
        auto body =
            emitExpressionBody(plan, *block, block->yields[policyIndex], 4);
        if (!body)
          return body.takeError();
        output << *body << "  }\n};\n\n";
      }
      if (block->kind == "table_write" || block->kind == "table_masked_write") {
        output << "struct block_" << index
               << "_merge_policy {\n  static constexpr std::array<size_t, "
               << block->writeFields.size() << "> fields{";
        for (auto [fieldIndex, field] : llvm::enumerate(block->writeFields)) {
          if (fieldIndex)
            output << ", ";
          if (field == "$entry") {
            output << 0;
            continue;
          }
          auto payload = llvm::find_if(plan.payloads,
                                       [&](const QueuePayloadPlan &candidate) {
                                         return candidate.name == *entryType;
                                       });
          if (payload == plan.payloads.end())
            return generatorError("table Entry payload is missing");
          auto declared = llvm::find_if(
              payload->fields, [&](const QueuePayloadFieldPlan &candidate) {
                return candidate.name == field;
              });
          if (declared == payload->fields.end())
            return generatorError("table write field is missing");
          output << std::distance(payload->fields.begin(), declared);
        }
        output << "};\n  void operator()(" << *entryType << " &target, const "
               << *entryType << " &value) const {\n";
        for (const std::string &field : block->writeFields) {
          if (field == "$entry")
            output << "    target = value;\n";
          else
            output << "    target." << identifier(field) << " = value."
                   << identifier(field) << ";\n";
        }
        output << "  }\n};\n\n";
      }
      continue;
    }
    if (block->kind == "dependency") {
      const QueuePlan *input = findQueue(plan, block->inputs.front());
      if (!input)
        return generatorError("dependency input Queue is missing");
      auto inputType = cppQueueType(plan, *input);
      if (!inputType)
        return inputType.takeError();
      constexpr llvm::StringLiteral policyNames[] = {"key", "dependency",
                                                     "resource", "cost"};
      for (auto [policyIndex, policyName] : llvm::enumerate(policyNames)) {
        llvm::StringRef resultType = input->payloadType;
        if (block->yields[policyIndex] != "item") {
          auto expression = std::find_if(
              block->expressions.begin(), block->expressions.end(),
              [&](const QueueExpressionPlan &candidate) {
                return candidate.result == block->yields[policyIndex];
              });
          if (expression == block->expressions.end())
            return generatorError("dependency policy yield type is missing");
          resultType = expression->type;
        }
        auto resultCppType = cppType(resultType);
        if (!resultCppType)
          return resultCppType.takeError();
        output << "struct block_" << index << '_' << policyName.str()
               << "_policy {\n  " << *resultCppType << " operator()(const "
               << *inputType << " &item) const {\n";
        auto body =
            emitExpressionBody(plan, *block, block->yields[policyIndex], 4);
        if (!body)
          return body.takeError();
        output << *body << "  }\n};\n\n";
      }
      continue;
    }
    if (block->kind == "transform" &&
        (block->inputs.size() != 1 || block->outputs.size() != 1)) {
      if (block->inputs.empty() || block->outputs.empty() ||
          block->outputs.size() != block->yields.size())
        return generatorError("atomic transform arity is inconsistent");
      std::vector<std::string> inputTypes;
      std::vector<std::string> outputTypes;
      for (const std::string &inputName : block->inputs) {
        const QueuePlan *input = findQueue(plan, inputName);
        if (!input)
          return generatorError("atomic transform input Queue is missing");
        auto type = cppQueueType(plan, *input);
        if (!type)
          return type.takeError();
        inputTypes.push_back(std::move(*type));
      }
      for (const std::string &outputName : block->outputs) {
        const QueuePlan *result = findQueue(plan, outputName);
        if (!result)
          return generatorError("atomic transform output Queue is missing");
        auto type = cppQueueType(plan, *result);
        if (!type)
          return type.takeError();
        outputTypes.push_back(std::move(*type));
      }
      if (!block->displayRuleName.empty() || !block->ndfIds.empty() ||
          !block->ndfRequires.empty())
        emitRuleProvenance(output, *block);
      output << "struct block_" << index << "_policy {\n  std::tuple<";
      for (auto [typeIndex, type] : llvm::enumerate(outputTypes)) {
        if (typeIndex)
          output << ", ";
        output << type;
      }
      output << "> operator()(";
      for (auto [typeIndex, type] : llvm::enumerate(inputTypes)) {
        if (typeIndex)
          output << ", ";
        output << "const " << type << " &item";
        if (typeIndex)
          output << typeIndex;
      }
      output << ") const {\n    return {\n";
      for (auto [yieldIndex, yield] : llvm::enumerate(block->yields)) {
        output << "      [&]() -> " << outputTypes[yieldIndex] << " {\n";
        auto body = emitExpressionBody(plan, *block, yield, 8);
        if (!body)
          return body.takeError();
        output << *body << "      }()"
               << (yieldIndex + 1 == block->yields.size() ? "\n" : ",\n");
      }
      output << "    };\n  }\n};\n\n";
      continue;
    }
    if (block->kind == "table_read_group")
      continue;
    const size_t expectedYields = block->kind == "feedback" ? 2 : 1;
    if (block->yields.size() != expectedYields ||
        (block->kind != "select" && block->inputs.size() != 1))
      return generatorError("Queue policy arity is unsupported");
    const QueuePlan *input = findQueue(plan, block->inputs.front());
    if (!input)
      return generatorError("policy input Queue is missing");
    auto inputType = cppQueueType(plan, *input);
    if (!inputType)
      return inputType.takeError();
    std::string policy =
        "block_" + std::to_string(index) +
        (block->kind == "feedback" ? "_update_policy" : "_policy");
    if (!block->displayRuleName.empty() || !block->ndfIds.empty() ||
        !block->ndfRequires.empty())
      emitRuleProvenance(output, *block);
    output << "struct " << policy << " {\n  ";
    if (block->kind == "route" || block->kind == "select")
      output << "size_t";
    else if (block->kind == "expect")
      output << "bool";
    else if (block->kind == "reorder" || block->kind == "credit") {
      llvm::StringRef keyType = input->payloadType;
      if (block->yields.front() != "item") {
        auto expression =
            std::find_if(block->expressions.begin(), block->expressions.end(),
                         [&](const QueueExpressionPlan &candidate) {
                           return candidate.result == block->yields.front();
                         });
        if (expression == block->expressions.end())
          return generatorError(block->kind + " yield type is missing");
        keyType = expression->type;
      }
      auto keyCppType = cppType(keyType);
      if (!keyCppType)
        return keyCppType.takeError();
      output << *keyCppType;
    } else {
      const QueuePlan *result = findQueue(plan, block->outputs.front());
      if (!result)
        return generatorError("transform output Queue is missing");
      auto resultType = cppQueueType(plan, *result);
      if (!resultType)
        return resultType.takeError();
      output << *resultType;
    }
    output << " operator()(const " << *inputType << " &item) const {\n";
    auto body = emitExpressionBody(plan, *block, block->yields.front(), 4);
    if (!body)
      return body.takeError();
    if (block->kind == "route" || block->kind == "select")
      output << "    return static_cast<size_t>([&]() {\n"
             << *body << "    }());\n";
    else
      output << *body;
    output << "  }\n};\n\n";
    if (block->kind == "feedback") {
      output << "struct block_" << index
             << "_condition_policy {\n  bool operator()(const " << *inputType
             << " &item) const {\n";
      auto condition = emitExpressionBody(plan, *block, block->yields[1], 4);
      if (!condition)
        return condition.takeError();
      output << *condition << "  }\n};\n\n";
    }
  }

  for (auto [memoryIndex, instance] : llvm::enumerate(plan.memoryInstances)) {
    auto found = memoryEndpoints.find(instance.name);
    if (found == memoryEndpoints.end() || found->getValue().empty())
      return generatorError("memory instance has no endpoints");
    const auto &endpoints = found->getValue();
    const QueuePlan *input = findQueue(plan, endpoints.front()->inputs.front());
    if (!input)
      return generatorError("memory endpoint input Queue is missing");
    auto inputType = cppQueueType(plan, *input);
    auto dataType = cppType(instance.dataType);
    if (!inputType)
      return inputType.takeError();
    if (!dataType)
      return dataType.takeError();
    constexpr llvm::StringLiteral policyNames[] = {"address", "write", "data"};
    const std::array<std::string, 3> resultTypes = {"std::uint64_t", "bool",
                                                    *dataType};
    for (auto [policyIndex, policyName] : llvm::enumerate(policyNames)) {
      output << "struct memory_" << memoryIndex << '_' << policyName.str()
             << "_policy {\n  " << resultTypes[policyIndex]
             << " operator()(size_t endpoint, const " << *inputType
             << " &item) const {\n    switch (endpoint) {\n";
      for (const QueueBlockPlan *endpoint : endpoints) {
        output << "    case " << endpoint->endpointOrdinal << ": {\n";
        if (policyIndex == 0)
          output << "      return static_cast<std::uint64_t>([&]() {\n";
        auto body =
            emitExpressionBody(plan, *endpoint, endpoint->yields[policyIndex],
                               policyIndex == 0 ? 8 : 6);
        if (!body)
          return body.takeError();
        output << *body;
        if (policyIndex == 0)
          output << "      }());\n";
        output << "    }\n";
      }
      output << "    default: return {};\n    }\n  }\n};\n\n";
    }
    output << "struct memory_" << memoryIndex << "_response_policy {\n  "
           << *inputType << " operator()(size_t endpoint, const " << *inputType
           << " &item, const " << *dataType
           << " &old_data) const {\n    auto result = item;\n"
              "    switch (endpoint) {\n";
    for (const QueueBlockPlan *endpoint : endpoints)
      output << "    case " << endpoint->endpointOrdinal << ": result."
             << endpoint->resultField << " = old_data; break;\n";
    output << "    default: break;\n    }\n    return result;\n  }\n};\n\n";
  }

  std::string modelClass = className(plan.system);
  emitDefinitionProvenance(output, plan);
  output << "class " << modelClass
         << " final : public gfsim::Module {\npublic:\n  " << modelClass
         << "() : gfsim::Module(\"" << plan.system
         << "\", gfsim::kInvalidObjectId, nullptr),\n";
  std::vector<std::string> initializers;
  for (const std::string &scope : plan.scopes) {
    llvm::StringRef parent = llvm::StringRef(scope).rsplit('/').first;
    if (parent.empty())
      parent = "/";
    auto parentPointer = modulePointer(parent);
    if (!parentPointer)
      return parentPointer.takeError();
    appendInitializer(initializers, scopeMembers[scope], "(\"",
                      pathParts(scope).back(), "\", gfsim::kInvalidObjectId, ",
                      *parentPointer, ")");
  }
  for (const QueuePlan &queue : plan.queues) {
    auto type = cppQueueType(plan, queue);
    auto parent = modulePointer(queueOwners[queue.name]);
    if (!type)
      return type.takeError();
    if (!parent)
      return parent.takeError();
    appendInitializer(initializers, queueMembers[queue.name], "(\"", queue.name,
                      "\", ", queueIds[queue.name], ", ", *parent, ", ",
                      queue.depth,
                      ", std::numeric_limits<size_t>::max(), nullptr, ",
                      queue.latency, ", ", queue.rate, ", ", queue.lanes, ")");
  }
  for (const TablePlan &table : plan.tables) {
    auto parent = modulePointer(table.ownerPath);
    if (!parent)
      return parent.takeError();
    auto storage = tableStorageArgument(plan, table);
    if (!storage)
      return storage.takeError();
    appendInitializer(initializers, tableMembers[table.name], "(\"", table.name,
                      "\", ", tableIds[table.name], ", ", *parent, ", ",
                      *storage, ")");
  }
  std::string slotPolicyPointers;
  for (auto [index, slot] : llvm::enumerate(plan.slots)) {
    (void)slot;
    slotPolicyPointers.append(", &slot_")
        .append(std::to_string(index))
        .append("_state_");
  }
  std::string sharedPolicyPointers;
  for (const TableMatchPlan &match : plan.tableMatches)
    sharedPolicyPointers.append(", &")
        .append(identifier(match.name))
        .append("_");
  for (const TableSelectionPlan &selection : plan.tableSelections)
    sharedPolicyPointers.append(", &")
        .append(identifier(selection.name))
        .append("_");
  for (const TableMatchPlan &match : plan.tableMatches) {
    auto table = tableMembers.find(match.table);
    const TablePlan *tablePlan = findTable(plan, match.table);
    if (table == tableMembers.end() || !tablePlan)
      return generatorError("table.match declaration is missing");
    appendInitializer(initializers, identifier(match.name), "_(",
                      table->getValue(), ", ",
                      tableProjectionArgument(*tablePlan, match), ", ",
                      identifier(match.name), "_predicate_policy{&",
                      table->getValue(), slotPolicyPointers, "})");
  }
  for (const TableSelectionPlan &selection : plan.tableSelections) {
    auto table = tableMembers.find(selection.table);
    const TablePlan *tablePlan = findTable(plan, selection.table);
    auto match = llvm::find_if(plan.tableMatches, [&](const auto &candidate) {
      return candidate.name == selection.match;
    });
    if (table == tableMembers.end() || !tablePlan ||
        match == plan.tableMatches.end())
      return generatorError("table.choose declaration is missing");
    appendInitializer(
        initializers, identifier(selection.name), "_(", table->getValue(), ", ",
        tableProjectionArgument(*tablePlan, *match), ", ",
        identifier(selection.name), "_mask_policy{&",
        identifier(selection.match), "_}, ", identifier(selection.name),
        "_key_policy{}, ", identifier(selection.name), "_choose_policy");
    if (selection.count != 1 || selection.policy == "round_robin")
      initializers.back()
          .append(", ")
          .append(identifier(selection.name))
          .append("_key_ordering, ")
          .append(std::to_string(selection.initialCursor));
    initializers.back().append(")");
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto state = feedbackStateIds.find(index);
    if (state == feedbackStateIds.end())
      continue;
    const QueuePlan *input = findQueue(plan, block->inputs[0]);
    auto type = input ? cppQueueType(plan, *input)
                      : llvm::Expected<std::string>(
                            generatorError("feedback input Queue is missing"));
    auto parent = modulePointer(block->scope);
    if (!type)
      return type.takeError();
    if (!parent)
      return parent.takeError();
    appendInitializer(initializers, "block_", index,
                      "_state_(\"feedback_state_", block->name, "\", ",
                      state->second, ", ", *parent,
                      ", 1, std::numeric_limits<size_t>::max(), nullptr, 1)");
  }
  size_t sinkIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto parent = modulePointer(block->scope);
    if (!parent)
      return parent.takeError();
    std::string member = blockSymbol(index) + "_";
    std::string key = block->name + "#" + std::to_string(index);
    std::string instanceName = block->kind + "_" + block->name;
    if (block->kind == "firing") {
      std::string inputs;
      for (size_t input = 0; input < block->inputs.size(); ++input) {
        if (input)
          inputs.append(", ");
        inputs.append("&").append(queueMembers[block->inputs[input]]);
      }
      std::string outputs;
      for (size_t outputIndex = 0; outputIndex < block->outputs.size();
           ++outputIndex) {
        if (outputIndex)
          outputs.append(", ");
        outputs.append("&").append(queueMembers[block->outputs[outputIndex]]);
      }
      std::string policy = blockSymbol(index) + "_policy{";
      bool hasPolicyMember = false;
      for (auto [readIndex, readTable] :
           llvm::enumerate(readOnlyTables(plan, *block))) {
        auto table = tableMembers.find(readTable->name);
        if (table == tableMembers.end())
          return generatorError("read-only state declaration is missing");
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&").append(table->getValue());
        hasPolicyMember = true;
      }
      for (auto [slotIndex, slot] : llvm::enumerate(plan.slots)) {
        (void)slot;
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&slot_")
            .append(std::to_string(slotIndex))
            .append("_state_");
        hasPolicyMember = true;
      }
      for (const TableMatchPlan &match : plan.tableMatches) {
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&").append(identifier(match.name)).append("_");
        hasPolicyMember = true;
      }
      for (const TableSelectionPlan &selection : plan.tableSelections) {
        if (hasPolicyMember)
          policy.append(", ");
        policy.append("&").append(identifier(selection.name)).append("_");
        hasPolicyMember = true;
      }
      policy.push_back('}');
      const std::vector<const TablePlan *> ownerTables =
          stateOwnerTables(plan, *block);
      if (ownerTables.size() != 1 || block->stateWrites.empty() ||
          !block->slotReleases.empty()) {
        std::string tables;
        std::string modes;
        std::string merges;
        for (auto [ownerIndex, owner] : llvm::enumerate(ownerTables)) {
          auto table = tableMembers.find(owner->name);
          if (table == tableMembers.end())
            return generatorError("state firing declaration is missing");
          if (ownerIndex) {
            tables.append(", ");
            modes.append(", ");
            merges.append(", ");
          }
          const StateWritePlan *write = findStateWrite(*block, owner->name);
          tables.append("&").append(table->getValue());
          modes.append("gfsim::TableWriteMode::")
              .append(!write || write->mode == "replace" ? "Replace"
                                                         : "FieldMerge");
          merges.append(blockSymbol(index))
              .append("_merge_policy_")
              .append(std::to_string(ownerIndex))
              .append("{}");
        }
        std::string releaseResources;
        for (const SlotReleaseEffectPlan &release : block->slotReleases) {
          auto slot = llvm::find_if(plan.slots, [&](const SlotPlan &candidate) {
            return candidate.name == release.slot;
          });
          if (slot == plan.slots.end())
            return generatorError("state firing slot release is missing");
          if (!releaseResources.empty())
            releaseResources.append(", ");
          releaseResources.append("&slot_")
              .append(std::to_string(std::distance(plan.slots.begin(), slot)))
              .append("_state_");
        }
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", std::tuple{", tables, "}, std::tuple{", inputs,
            "}, std::tuple{", outputs, "}, ",
            ownerTables.empty() ? "std::array<gfsim::TableWriteMode, 0>{"
                                : "std::array{",
            modes, "}, ", policy, ", std::tuple{", merges,
            "}, nullptr, std::vector<gfsim::SlotReleaseResource *>{",
            releaseResources, "})");
      } else {
        auto table = tableMembers.find(block->table);
        if (table == tableMembers.end())
          return generatorError("table firing declaration is missing");
        const StateWritePlan *write = findStateWrite(*block, block->table);
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", table->getValue(), ", std::tuple{", inputs,
            "}, std::tuple{", outputs, "}, gfsim::TableWriteMode::",
            !write || write->mode == "replace" ? "Replace" : "FieldMerge", ", ",
            policy, ", ", blockSymbol(index), "_merge_policy{})");
      }
    } else if (block->kind == "transform") {
      if (block->inputs.size() == 1 && block->outputs.size() == 1) {
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ",
                          queueMembers[block->inputs[0]], ", ",
                          queueMembers[block->outputs[0]], ")");
      } else {
        std::string inputs;
        std::string outputs;
        for (size_t operand = 0; operand < block->inputs.size(); ++operand) {
          if (operand)
            inputs.append(", ");
          inputs.append("&").append(queueMembers[block->inputs[operand]]);
        }
        for (size_t result = 0; result < block->outputs.size(); ++result) {
          if (result)
            outputs.append(", ");
          outputs.append("&").append(queueMembers[block->outputs[result]]);
        }
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", std::tuple{", inputs,
                          "}, std::tuple{", outputs, "})");
      }
    } else if (block->kind == "broadcast" || block->kind == "fork" ||
               block->kind == "route") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(generatorError(
                              "topology input Queue is missing"));
      if (!type)
        return type.takeError();
      std::string outputs;
      for (auto [outputIndex, name] : llvm::enumerate(block->outputs)) {
        if (outputIndex)
          outputs.append(", ");
        outputs.append("&").append(queueMembers[name]);
      }
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]],
                        ", std::array<gfsim::SimQueue<", *type, "> *, ",
                        block->outputs.size(), ">{", outputs, "})");
    } else if (block->kind == "select") {
      const QueuePlan *result = findQueue(plan, block->outputs[0]);
      auto type = result ? cppQueueType(plan, *result)
                         : llvm::Expected<std::string>(generatorError(
                               "select output Queue is missing"));
      if (!type)
        return type.takeError();
      std::string inputs;
      for (size_t input = 1; input < block->inputs.size(); ++input) {
        if (input > 1)
          inputs.append(", ");
        inputs.append("&").append(queueMembers[block->inputs[input]]);
      }
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]],
                        ", std::array<gfsim::SimQueue<", *type, "> *, ",
                        block->inputs.size() - 1, ">{", inputs, "}, ",
                        queueMembers[block->outputs[0]], ")");
    } else if (block->kind == "merge") {
      const QueuePlan *result = findQueue(plan, block->outputs[0]);
      auto type = result ? cppQueueType(plan, *result)
                         : llvm::Expected<std::string>(
                               generatorError("merge output Queue is missing"));
      if (!type)
        return type.takeError();
      std::string inputs;
      for (auto [inputIndex, name] : llvm::enumerate(block->inputs)) {
        if (inputIndex)
          inputs.append(", ");
        inputs.append("&").append(queueMembers[name]);
      }
      std::string policy = block->policy == "priority"
                               ? "gfsim::QueueMergePolicy::Priority"
                               : "gfsim::QueueMergePolicy::RoundRobin";
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent,
                        ", std::array<gfsim::SimQueue<", *type, "> *, ",
                        block->inputs.size(), ">{", inputs, "}, ",
                        queueMembers[block->outputs[0]], ", ", policy, ")");
    } else if (block->kind == "barrier") {
      std::string inputs;
      std::string outputs;
      for (size_t operand = 0; operand < block->inputs.size(); ++operand) {
        if (operand)
          inputs.append(", ");
        inputs.append("&").append(queueMembers[block->inputs[operand]]);
        if (operand)
          outputs.append(", ");
        outputs.append("&").append(queueMembers[block->outputs[operand]]);
      }
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", std::tuple{", inputs,
                        "}, std::tuple{", outputs, "})");
    } else if (block->kind == "reorder") {
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]], ", ",
                        queueMembers[block->outputs[0]], ", ", block->capacity,
                        ", ", block->start, ")");
    } else if (block->kind == "dependency") {
      if (block->provider == "v2")
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ",
                          queueMembers[block->inputs[0]], ", ",
                          queueMembers[block->outputs[0]], ")");
      else
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", queueMembers[block->inputs[0]], ", ",
            queueMembers[block->outputs[0]], ", ", block->capacity, ", ",
            block->resources, ", ", block->noDependency, ")");
    } else if (block->kind == "credit") {
      appendInitializer(
          initializers, member, "(\"", instanceName, "\", ", blockIds[key],
          ", ", *parent, ", ", queueMembers[block->inputs[0]], ", ",
          queueMembers[block->outputs[0]], ", ", block->credits, ")");
    } else if (block->kind == "feedback") {
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]], ", block_", index,
                        "_state_, ", queueMembers[block->outputs[0]], ", ",
                        block->maxIterations, ")");
    } else if (block->kind == "expect") {
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]], ", ",
                        cppStringLiteral(block->message), ")");
    } else if (block->kind == "table_read_group") {
      auto table = tableMembers.find(block->table);
      if (table == tableMembers.end())
        return generatorError("selection read group Table is missing");
      std::string outputs;
      for (auto [lane, output] : llvm::enumerate(block->outputs)) {
        if (lane)
          outputs.append(", ");
        outputs.append("&").append(queueMembers[output]);
      }
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ", table->getValue(),
                        ", ", identifier(block->selection), "_, std::array{",
                        outputs, "})");
    } else if (block->kind == "table_read") {
      auto table = tableMembers.find(block->table);
      if (table == tableMembers.end())
        return generatorError("table read declaration is missing");
      if (block->inputs.empty())
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ", table->getValue(),
                          ", ", queueMembers[block->outputs[0]], ", block_",
                          index, "_address_policy{&", table->getValue(),
                          slotPolicyPointers, sharedPolicyPointers, "}, block_",
                          index, "_when_policy{&", table->getValue(),
                          slotPolicyPointers, sharedPolicyPointers, "})");
      else
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ", table->getValue(),
                          ", ", queueMembers[block->inputs[0]], ", ",
                          queueMembers[block->outputs[0]], ", block_", index,
                          "_address_policy{&", table->getValue(),
                          slotPolicyPointers, sharedPolicyPointers, "}, block_",
                          index, "_when_policy{&", table->getValue(),
                          slotPolicyPointers, sharedPolicyPointers, "})");
    } else if (block->kind == "table_write") {
      auto table = tableMembers.find(block->table);
      if (table == tableMembers.end())
        return generatorError("table write declaration is missing");
      if (block->inputs.empty())
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", table->getValue(), ", block_", index,
            "_address_policy{&", table->getValue(), slotPolicyPointers,
            sharedPolicyPointers, "}, block_", index, "_enable_policy{&",
            table->getValue(), slotPolicyPointers, sharedPolicyPointers,
            "}, block_", index, "_value_policy{&", table->getValue(),
            slotPolicyPointers, sharedPolicyPointers, "}, block_", index,
            "_merge_policy{}, gfsim::TableWriteMode::",
            block->writeMode == "replace" ? "Replace" : "FieldMerge", ")");
      else
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", table->getValue(), ", ",
            queueMembers[block->inputs[0]], ", block_", index,
            "_address_policy{&", table->getValue(), slotPolicyPointers,
            sharedPolicyPointers, "}, block_", index, "_enable_policy{&",
            table->getValue(), slotPolicyPointers, sharedPolicyPointers,
            "}, block_", index, "_value_policy{&", table->getValue(),
            slotPolicyPointers, sharedPolicyPointers, "}, block_", index,
            "_merge_policy{}, gfsim::TableWriteMode::",
            block->writeMode == "replace" ? "Replace" : "FieldMerge", ")");
    } else if (block->kind == "table_masked_write") {
      auto table = tableMembers.find(block->table);
      const TablePlan *tablePlan = findTable(plan, block->table);
      if (table == tableMembers.end() || !tablePlan)
        return generatorError("masked table write declaration is missing");
      std::string projection = "gfsim::TableDomainProjection(" +
                               std::to_string(tablePlan->entries) + ")";
      auto mask =
          llvm::find_if(block->expressions, [&](const auto &expression) {
            return !block->yields.empty() &&
                   expression.result == block->yields[0];
          });
      if (mask != block->expressions.end() && mask->kind == "table_match") {
        projection = tableProjectionArgument(*tablePlan, *mask);
      } else if (mask != block->expressions.end() &&
                 mask->kind == "table_match_ref") {
        auto shared = llvm::find_if(plan.tableMatches, [&](const auto &match) {
          return match.name == mask->field;
        });
        if (shared == plan.tableMatches.end())
          return generatorError("masked table write match is missing");
        projection = tableProjectionArgument(*tablePlan, *shared);
      }
      appendInitializer(
          initializers, member, "(\"", instanceName, "\", ", blockIds[key],
          ", ", *parent, ", ", table->getValue(), ", ", projection, ", block_",
          index, "_mask_policy{&", table->getValue(), slotPolicyPointers,
          sharedPolicyPointers, "}, block_", index, "_enable_policy{&",
          table->getValue(), slotPolicyPointers, sharedPolicyPointers,
          "}, block_", index, "_value_policy{&", table->getValue(),
          slotPolicyPointers, sharedPolicyPointers, "}, block_", index,
          "_merge_policy{})");
    } else if (block->kind == "slot") {
      const SlotPlan *slot = findSlot(plan, block->slot);
      if (!slot)
        return generatorError("slot declaration is missing");
      auto slotIndex = static_cast<size_t>(slot - plan.slots.data());
      std::string policyPointers = slotPolicyPointers;
      for (const TablePlan &table : plan.tables)
        if (referencesTable(block->expressions, table.name))
          policyPointers.append(", &").append(tableMembers[table.name]);
      policyPointers.append(sharedPolicyPointers);
      appendInitializer(
          initializers, member, "(\"", instanceName, "\", ", blockIds[key],
          ", ", *parent, ", ", queueMembers[block->inputs[0]], ", slot_",
          slotIndex, "_state_, block_", index, "_release_policy{",
          policyPointers.empty() ? std::string() : policyPointers.substr(2),
          "})");
    } else if (block->kind == "sink" || block->kind == "observe") {
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]], ")");
      ++sinkIndex;
    } else {
      return generatorError("unsupported native Queue block '" + block->kind +
                            "'");
    }
  }
  for (auto [memoryIndex, instance] : llvm::enumerate(plan.memoryInstances)) {
    auto found = memoryEndpoints.find(instance.name);
    if (found == memoryEndpoints.end() || found->getValue().empty())
      return generatorError("memory instance has no endpoints");
    const auto &endpoints = found->getValue();
    const QueuePlan *input = findQueue(plan, endpoints.front()->inputs.front());
    auto type = input ? cppQueueType(plan, *input)
                      : llvm::Expected<std::string>(
                            generatorError("memory input missing"));
    auto parent = modulePointer(instance.ownerPath);
    if (!type)
      return type.takeError();
    if (!parent)
      return parent.takeError();
    std::string inputs;
    std::string outputs;
    for (auto [index, endpoint] : llvm::enumerate(endpoints)) {
      if (index) {
        inputs.append(", ");
        outputs.append(", ");
      }
      inputs.append("&").append(queueMembers[endpoint->inputs.front()]);
      outputs.append("&").append(queueMembers[endpoint->outputs.front()]);
    }
    appendInitializer(initializers, "memory_", memoryIndex, "_(\"memory_",
                      instance.name, "\", ", memoryIds[instance.name], ", ",
                      *parent, ", std::array<gfsim::SimQueue<", *type, "> *, ",
                      endpoints.size(), ">{", inputs,
                      "}, std::array<gfsim::SimQueue<", *type, "> *, ",
                      endpoints.size(), ">{", outputs, "}, ", instance.entries,
                      ", ", instance.init, ", ", instance.latency, ")");
  }
  for (auto [index, initializer] : llvm::enumerate(initializers))
    output << "        " << initializer
           << (index + 1 == initializers.size() ? "\n" : ",\n");
  output << "  {\n    setPath(\"/" << plan.system << "\");\n";
  for (const std::string &scope : plan.scopes) {
    llvm::StringRef parent = llvm::StringRef(scope).rsplit('/').first;
    if (parent.empty())
      parent = "/";
    auto line = attach(parent, scopeMembers[scope]);
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (const TablePlan &table : plan.tables) {
    auto line = attach(table.ownerPath, tableMembers[table.name]);
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (const QueuePlan &queue : plan.queues) {
    auto line = attach(queueOwners[queue.name], queueMembers[queue.name]);
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (!feedbackStateIds.contains(index))
      continue;
    auto line =
        attach(block->scope, "block_" + std::to_string(index) + "_state_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (auto [memoryIndex, instance] : llvm::enumerate(plan.memoryInstances)) {
    auto line = attach(instance.ownerPath,
                       "memory_" + std::to_string(memoryIndex) + "_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto line = attach(block->scope, blockSymbol(index) + "_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  output << "  }\n\n  void reset() override {\n    gfsim::Module::reset();\n";
  for (const TableMatchPlan &match : plan.tableMatches)
    output << "    " << identifier(match.name) << "_.reset();\n";
  for (const TableSelectionPlan &selection : plan.tableSelections)
    output << "    " << identifier(selection.name) << "_.reset();\n";
  output << "  }\n\n";
  for (const QueueBlockPlan &block : plan.blocks)
    if (block.kind == "source") {
      const QueuePlan *queue = findQueue(plan, block.outputs.front());
      auto type = queue ? cppQueueType(plan, *queue)
                        : llvm::Expected<std::string>(
                              generatorError("source Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::SimQueue<" << *type << "> &" << block.outputs.front()
             << "() { return " << queueMembers[block.outputs.front()]
             << "; }\n";
    }
  for (const TablePlan &table : plan.tables) {
    auto type = cppType(table.entryType);
    if (!type)
      return type.takeError();
    output << "  const gfsim::SimTable<" << *type << "> &table_"
           << identifier(table.name) << "() const { return "
           << tableMembers[table.name] << "; }\n";
  }
  sinkIndex = 0;
  size_t observationIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "sink") {
      const QueuePlan *queue = findQueue(plan, block->inputs.front());
      auto type = queue ? cppQueueType(plan, *queue)
                        : llvm::Expected<std::string>(
                              generatorError("sink Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  const std::vector<" << *type << "> &sink_" << sinkIndex
             << "_values() const { return block_" << index
             << "_.received(); }\n"
             << "  gfsim::ObjectId sink_" << sinkIndex
             << "_id() const { return block_" << index << "_.id(); }\n";
      ++sinkIndex;
    } else if (block->kind == "observe") {
      const QueuePlan *queue = findQueue(plan, block->inputs.front());
      auto type = queue ? cppQueueType(plan, *queue)
                        : llvm::Expected<std::string>(
                              generatorError("observation Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  const std::vector<" << *type << "> &observation_"
             << observationIndex << "_values() const { return block_" << index
             << "_.observed(); }\n";
      ++observationIndex;
    }
  output << "  void set_sink_retention_limit(size_t limit) {\n";
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "sink")
      output << "    " << blockSymbol(index) << "_.setRetentionLimit(limit);\n";
  output << "  }\n";
  size_t dependencyIndex = 0;
  size_t reorderIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind == "dependency") {
      output << "  size_t dependency_" << dependencyIndex
             << "_active() const { return block_" << index << "_.active(); }\n"
             << "  size_t dependency_" << dependencyIndex
             << "_resource_active(size_t resource) const { return block_"
             << index << "_.resourceActive(resource); }\n";
      ++dependencyIndex;
    } else if (block->kind == "reorder") {
      output << "  size_t reorder_" << reorderIndex
             << "_active() const { return block_" << index << "_.active(); }\n";
      ++reorderIndex;
    }
  }
  std::vector<std::pair<uint64_t, uint64_t>> arbitrationIds;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (!block->arbitrationMembership.empty())
      arbitrationIds.emplace_back(
          block->priority, blockIds[block->name + "#" + std::to_string(index)]);
  llvm::sort(arbitrationIds);
  output << "\n  std::array<gfsim::DispatchRow, " << nextId
         << "> dispatch_rows() {\n    return {\n";
  for (const QueuePlan &queue : plan.queues)
    output << "        gfsim::makeDispatchRow(&" << queueMembers[queue.name]
           << "),\n";
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (feedbackStateIds.contains(index))
      output << "        gfsim::makeDispatchRow(&block_" << index
             << "_state_),\n";
  for (size_t index = 0; index < runtimeBlocks.size(); ++index)
    output << "        gfsim::makeDispatchRow(&" << blockSymbol(index)
           << "_),\n";
  for (size_t index = 0; index < plan.memoryInstances.size(); ++index)
    output << "        gfsim::makeDispatchRow(&memory_" << index << "_),\n";
  for (const TablePlan &table : plan.tables)
    output << "        gfsim::makeDispatchRow(&" << tableMembers[table.name]
           << "),\n";
  output << "    };\n  }\n\n"
         << "  static constexpr std::array<gfsim::ObjectId, "
         << arbitrationIds.size() << "> arbitration_order() {\n    return {";
  for (auto [index, entry] : llvm::enumerate(arbitrationIds)) {
    if (index)
      output << ", ";
    output << entry.second;
  }
  output << "};\n  }\n\nprivate:\n";
  for (const std::string &scope : plan.scopes)
    output << "  gfsim::Module " << scopeMembers[scope] << ";\n";
  for (const QueuePlan &queue : plan.queues) {
    auto type = cppQueueType(plan, queue);
    if (!type)
      return type.takeError();
    output << "  gfsim::SimQueue<" << *type << "> " << queueMembers[queue.name]
           << ";\n";
  }
  for (auto [index, slot] : llvm::enumerate(plan.slots)) {
    auto type = cppType(slot.payloadType);
    if (!type)
      return type.takeError();
    output << "  gfsim::SlotState<" << *type << "> slot_" << index
           << "_state_;\n";
  }
  for (const TablePlan &table : plan.tables) {
    auto type = cppType(table.entryType);
    if (!type)
      return type.takeError();
    output << "  gfsim::SimTable<" << *type << "> " << tableMembers[table.name]
           << ";\n";
  }
  for (const TableMatchPlan &match : plan.tableMatches)
    output << "  " << identifier(match.name) << "_cache "
           << identifier(match.name) << "_;\n";
  for (const TableSelectionPlan &selection : plan.tableSelections)
    output << "  " << identifier(selection.name) << "_cache "
           << identifier(selection.name) << "_;\n";
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (!feedbackStateIds.contains(index))
      continue;
    const QueuePlan *input = findQueue(plan, block->inputs[0]);
    auto type = input ? cppQueueType(plan, *input)
                      : llvm::Expected<std::string>(
                            generatorError("feedback state type is missing"));
    if (!type)
      return type.takeError();
    output << "  gfsim::SimQueue<gfsim::FeedbackToken<" << *type << ">> block_"
           << index << "_state_;\n";
  }
  sinkIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind == "firing") {
      const std::vector<const TablePlan *> ownerTables =
          stateOwnerTables(plan, *block);
      if (ownerTables.size() != 1 || block->stateWrites.empty() ||
          !block->slotReleases.empty()) {
        output << "  gfsim::QueueStateTransition<" << blockSymbol(index)
               << "_policy, std::tuple<";
        for (auto [ownerIndex, table] : llvm::enumerate(ownerTables)) {
          auto type = table ? cppType(table->entryType)
                            : llvm::Expected<std::string>(
                                  generatorError("state firing Table missing"));
          if (!type)
            return type.takeError();
          if (ownerIndex)
            output << ", ";
          output << *type;
        }
        output << ">, std::tuple<";
        for (auto [inputIndex, inputName] : llvm::enumerate(block->inputs)) {
          const QueuePlan *input = findQueue(plan, inputName);
          auto type = input ? cppQueueType(plan, *input)
                            : llvm::Expected<std::string>(
                                  generatorError("state firing input missing"));
          if (!type)
            return type.takeError();
          if (inputIndex)
            output << ", ";
          output << *type;
        }
        output << ">, std::tuple<";
        for (auto [outputIndex, outputName] : llvm::enumerate(block->outputs)) {
          const QueuePlan *result = findQueue(plan, outputName);
          auto type = result ? cppQueueType(plan, *result)
                             : llvm::Expected<std::string>(generatorError(
                                   "state firing output missing"));
          if (!type)
            return type.takeError();
          if (outputIndex)
            output << ", ";
          output << *type;
        }
        output << ">, std::tuple<";
        for (size_t ownerIndex = 0; ownerIndex < ownerTables.size();
             ++ownerIndex) {
          if (ownerIndex)
            output << ", ";
          output << blockSymbol(index) << "_merge_policy_" << ownerIndex;
        }
        output << ">> " << blockSymbol(index) << "_;\n";
        continue;
      }
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(generatorError(
                                   "table firing Table missing"));
      if (!entryType)
        return entryType.takeError();
      output << "  gfsim::QueueTableTransition<" << blockSymbol(index)
             << "_policy, " << *entryType << ", std::tuple<";
      for (auto [inputIndex, inputName] : llvm::enumerate(block->inputs)) {
        const QueuePlan *input = findQueue(plan, inputName);
        auto inputType = input ? cppQueueType(plan, *input)
                               : llvm::Expected<std::string>(generatorError(
                                     "table firing input missing"));
        if (!inputType)
          return inputType.takeError();
        if (inputIndex)
          output << ", ";
        output << *inputType;
      }
      output << ">, std::tuple<";
      for (auto [outputIndex, outputName] : llvm::enumerate(block->outputs)) {
        const QueuePlan *result = findQueue(plan, outputName);
        auto resultType = result ? cppQueueType(plan, *result)
                                 : llvm::Expected<std::string>(generatorError(
                                       "table firing output missing"));
        if (!resultType)
          return resultType.takeError();
        if (outputIndex)
          output << ", ";
        output << *resultType;
      }
      output << ">, " << blockSymbol(index) << "_merge_policy> "
             << blockSymbol(index) << "_;\n";
    } else if (block->kind == "transform") {
      if (block->inputs.size() == 1 && block->outputs.size() == 1) {
        const QueuePlan *input = findQueue(plan, block->inputs[0]);
        const QueuePlan *result = findQueue(plan, block->outputs[0]);
        auto inputType = input ? cppQueueType(plan, *input)
                               : llvm::Expected<std::string>(
                                     generatorError("transform input missing"));
        auto resultType = result ? cppQueueType(plan, *result)
                                 : llvm::Expected<std::string>(generatorError(
                                       "transform output missing"));
        if (!inputType)
          return inputType.takeError();
        if (!resultType)
          return resultType.takeError();
        if (input->lanes > 1 || result->lanes > 1)
          output << "  gfsim::QueueLaneTransform<" << *inputType << ", "
                 << *resultType << ", block_" << index << "_policy> block_"
                 << index << "_;\n";
        else
          output << "  gfsim::QueueTransform<" << *inputType << ", "
                 << *resultType << ", block_" << index << "_policy, "
                 << result->rate << "> block_" << index << "_;\n";
      } else {
        output << "  gfsim::QueueAtomicTransform<block_" << index
               << "_policy, std::tuple<";
        for (auto [inputIndex, inputName] : llvm::enumerate(block->inputs)) {
          const QueuePlan *input = findQueue(plan, inputName);
          auto type = input ? cppQueueType(plan, *input)
                            : llvm::Expected<std::string>(
                                  generatorError("atomic input missing"));
          if (!type)
            return type.takeError();
          if (inputIndex)
            output << ", ";
          output << *type;
        }
        output << ">, std::tuple<";
        for (auto [outputIndex, outputName] : llvm::enumerate(block->outputs)) {
          const QueuePlan *result = findQueue(plan, outputName);
          auto type = result ? cppQueueType(plan, *result)
                             : llvm::Expected<std::string>(generatorError(
                                   "atomic transform output missing"));
          if (!type)
            return type.takeError();
          if (outputIndex)
            output << ", ";
          output << *type;
        }
        output << ">> block_" << index << "_;\n";
      }
    } else if (block->kind == "broadcast" || block->kind == "fork" ||
               block->kind == "route") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("route input missing"));
      if (!type)
        return type.takeError();
      if (block->kind == "broadcast")
        output << "  gfsim::QueueBroadcast<" << *type << ", "
               << block->outputs.size() << "> block_" << index << "_;\n";
      else if (block->kind == "fork")
        output << "  gfsim::QueueFork<" << *type << ", "
               << block->outputs.size() << "> block_" << index << "_;\n";
      else
        output << "  gfsim::QueueRoute<" << *type << ", "
               << block->outputs.size() << ", block_" << index
               << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "select") {
      const QueuePlan *control = findQueue(plan, block->inputs[0]);
      const QueuePlan *result = findQueue(plan, block->outputs[0]);
      auto controlType = control ? cppType(control->payloadType)
                                 : llvm::Expected<std::string>(generatorError(
                                       "select control input missing"));
      auto dataType = result ? cppQueueType(plan, *result)
                             : llvm::Expected<std::string>(
                                   generatorError("select output missing"));
      if (!controlType)
        return controlType.takeError();
      if (!dataType)
        return dataType.takeError();
      output << "  gfsim::QueueSelect<" << *controlType << ", " << *dataType
             << ", " << block->inputs.size() - 1 << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "merge") {
      const QueuePlan *result = findQueue(plan, block->outputs[0]);
      auto type = result ? cppQueueType(plan, *result)
                         : llvm::Expected<std::string>(
                               generatorError("merge output missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueMerge<" << *type << ", " << block->inputs.size()
             << "> block_" << index << "_;\n";
    } else if (block->kind == "barrier") {
      output << "  gfsim::QueueBarrier<std::tuple<";
      for (auto [inputIndex, inputName] : llvm::enumerate(block->inputs)) {
        const QueuePlan *input = findQueue(plan, inputName);
        auto type = input ? cppQueueType(plan, *input)
                          : llvm::Expected<std::string>(
                                generatorError("barrier input missing"));
        if (!type)
          return type.takeError();
        if (inputIndex)
          output << ", ";
        output << *type;
      }
      output << ">> block_" << index << "_;\n";
    } else if (block->kind == "reorder") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("reorder input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueReorder<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "dependency") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("dependency input missing"));
      if (!type)
        return type.takeError();
      if (block->provider == "v2")
        output << "  gfsim::Schedule<" << *type << ", " << block->capacity
               << ", " << block->resources << ", " << block->noDependency
               << ", block_" << index << "_key_policy, block_" << index
               << "_dependency_policy, block_" << index
               << "_resource_policy, block_" << index << "_cost_policy> block_"
               << index << "_;\n";
      else
        output << "  gfsim::QueueDependency<" << *type << ", block_" << index
               << "_key_policy, block_" << index << "_dependency_policy, block_"
               << index << "_resource_policy, block_" << index
               << "_cost_policy> block_" << index << "_;\n";
    } else if (block->kind == "credit") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("credit input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueCredit<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "feedback") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("feedback input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueFeedback<" << *type << ", block_" << index
             << "_update_policy, block_" << index << "_condition_policy> block_"
             << index << "_;\n";
    } else if (block->kind == "expect") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("expect input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueExpect<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "table_read_group") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(generatorError(
                                   "selection read group Table missing"));
      if (!entryType)
        return entryType.takeError();
      output << "  gfsim::TableSelectionReadGroup<" << *entryType << ", "
             << identifier(block->selection) << "_cache, "
             << block->selectionCount << "> block_" << index << "_;\n";
    } else if (block->kind == "table_read") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(
                                   generatorError("table declaration missing"));
      if (!entryType)
        return entryType.takeError();
      if (block->inputs.empty()) {
        output << "  gfsim::TableReadSource<" << *entryType << ", block_"
               << index << "_address_policy, block_" << index
               << "_when_policy> block_" << index << "_;\n";
      } else {
        const QueuePlan *input = findQueue(plan, block->inputs.front());
        auto inputType = input ? cppQueueType(plan, *input)
                               : llvm::Expected<std::string>(generatorError(
                                     "table read input missing"));
        if (!inputType)
          return inputType.takeError();
        output << "  gfsim::QueueTableRead<" << *inputType << ", " << *entryType
               << ", block_" << index << "_address_policy, block_" << index
               << "_when_policy> block_" << index << "_;\n";
      }
    } else if (block->kind == "table_write") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(
                                   generatorError("table declaration missing"));
      if (!entryType)
        return entryType.takeError();
      if (block->inputs.empty()) {
        output << "  gfsim::TableWriteSource<" << *entryType << ", block_"
               << index << "_address_policy, block_" << index
               << "_enable_policy, block_" << index << "_value_policy, block_"
               << index << "_merge_policy> block_" << index << "_;\n";
      } else {
        const QueuePlan *input = findQueue(plan, block->inputs.front());
        auto inputType = input ? cppQueueType(plan, *input)
                               : llvm::Expected<std::string>(generatorError(
                                     "table write input missing"));
        if (!inputType)
          return inputType.takeError();
        output << "  gfsim::QueueTableWrite<" << *inputType << ", "
               << *entryType << ", block_" << index << "_address_policy, block_"
               << index << "_enable_policy, block_" << index
               << "_value_policy, block_" << index << "_merge_policy> block_"
               << index << "_;\n";
      }
    } else if (block->kind == "table_masked_write") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(
                                   generatorError("table declaration missing"));
      if (!entryType)
        return entryType.takeError();
      output << "  gfsim::TableMaskedWriteSource<" << *entryType << ", block_"
             << index << "_mask_policy, block_" << index
             << "_enable_policy, block_" << index << "_value_policy, block_"
             << index << "_merge_policy> block_" << index << "_;\n";
    } else if (block->kind == "slot") {
      const SlotPlan *slot = findSlot(plan, block->slot);
      auto type = slot ? cppType(slot->payloadType)
                       : llvm::Expected<std::string>(
                             generatorError("slot declaration missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueSlot<" << *type << ", block_" << index
             << "_release_policy> block_" << index << "_;\n";
    } else if (block->kind == "sink" || block->kind == "observe") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppQueueType(plan, *input)
                        : llvm::Expected<std::string>(
                              generatorError("sink input missing"));
      if (!type)
        return type.takeError();
      if (block->kind == "sink") {
        output << "  gfsim::"
               << (input->lanes > 1 ? "QueueLaneSink<" : "QueueSink<") << *type
               << "> block_" << index << "_;\n";
        ++sinkIndex;
      } else {
        output << "  gfsim::QueueObserve<" << *type << "> block_" << index
               << "_;\n";
      }
    }
  }
  for (auto [memoryIndex, instance] : llvm::enumerate(plan.memoryInstances)) {
    auto found = memoryEndpoints.find(instance.name);
    if (found == memoryEndpoints.end() || found->getValue().empty())
      return generatorError("memory instance has no endpoints");
    const auto &endpoints = found->getValue();
    const QueuePlan *input = findQueue(plan, endpoints.front()->inputs.front());
    auto type = input ? cppQueueType(plan, *input)
                      : llvm::Expected<std::string>(
                            generatorError("memory input missing"));
    auto dataType = cppType(instance.dataType);
    if (!type)
      return type.takeError();
    if (!dataType)
      return dataType.takeError();
    output << "  gfsim::QueueMemoryArbiter<" << *type << ", " << *dataType
           << ", " << endpoints.size() << ", memory_" << memoryIndex
           << "_address_policy, memory_" << memoryIndex
           << "_write_policy, memory_" << memoryIndex << "_data_policy, memory_"
           << memoryIndex << "_response_policy> memory_" << memoryIndex
           << "_;\n";
  }
  output << "};\n\n} // namespace ac_generated\n";
  return output.str();
}

llvm::Expected<std::vector<QueueGraphGeneratedFile>>
generateQueueGraphModelBundle(const QueueGraphPlan &plan) {
  std::optional<StructuredQueueGraphCpp> structured;
  std::string concatenated;
  if (!plan.definition.empty()) {
    auto units = generateStructuredQueueGraphCpp(plan);
    if (!units)
      return units.takeError();
    structured = std::move(*units);
    concatenated = structured->concatenated;
  } else {
    auto queueGraph = generateQueueGraphCpp(plan);
    if (!queueGraph)
      return queueGraph.takeError();
    concatenated = std::move(*queueGraph);
  }

  const std::string modelClass = className(plan.system);
  std::ostringstream queueGraphSource;
  if (structured && !structured->modules.empty()) {
    queueGraphSource << "#include \"generated/dut.h\"\n";
  } else {
    queueGraphSource << concatenated;
  }
  queueGraphSource << R"cpp(

#include "gfsim/model_input.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace agentic_generated_detail {

#if defined(__GNUC__) || defined(__clang__)
#define AGENTIC_GENERATED_HIDDEN __attribute__((visibility("hidden")))
#else
#define AGENTIC_GENERATED_HIDDEN
#endif

struct Runtime {
  gfsim::SimSystem system{"agentic_model"};
  ac_generated::)cpp"
                   << modelClass << R"cpp( model;
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{
      {"cycle", 1, 0, 1},
  }};

  Runtime() {
    model.set_sink_retention_limit(0);
    if (!system.root().attachChild(model))
      throw std::runtime_error("generated model attachment failed");
)cpp";
  if (!plan.definition.empty()) {
    queueGraphSource << R"cpp(    if (!system.setTimeDomains(timeDomains) ||
        !model.configure_activation_scheduler(system))
      throw std::runtime_error("generated model runtime initialization failed");
)cpp";
  } else {
    queueGraphSource << R"cpp(    const auto rows = model.dispatch_rows();
    constexpr auto arbitration = ac_generated::)cpp"
                     << modelClass << R"cpp(::arbitration_order();
    if (!system.setTimeDomains(timeDomains) ||
        !system.setDispatchTable(rows) ||
        !system.setArbitrationOrder(arbitration))
      throw std::runtime_error("generated model runtime initialization failed");
)cpp";
  }
  queueGraphSource << R"cpp(  }
};

namespace {

std::string escapeJson(std::string_view value) {
  std::ostringstream output;
  for (unsigned char character : value) {
    switch (character) {
    case '\\': output << "\\\\"; break;
    case '"': output << "\\\""; break;
    case '\b': output << "\\b"; break;
    case '\f': output << "\\f"; break;
    case '\n': output << "\\n"; break;
    case '\r': output << "\\r"; break;
    case '\t': output << "\\t"; break;
    default:
      if (character < 0x20) {
        constexpr char digits[] = "0123456789abcdef";
        output << "\\u00" << digits[character >> 4] << digits[character & 0xf];
      } else {
        output << static_cast<char>(character);
      }
    }
  }
  return output.str();
}

void appendJsonString(std::ostringstream &output, std::string_view value) {
  output << '"' << escapeJson(value) << '"';
}

} // namespace

AGENTIC_GENERATED_HIDDEN Runtime *create(std::string &error) noexcept {
  try {
    return new Runtime();
  } catch (const std::exception &exception) {
    error = exception.what();
  } catch (...) {
    error = "generated model construction failed";
  }
  return nullptr;
}

AGENTIC_GENERATED_HIDDEN void destroy(Runtime *runtime) noexcept {
  delete runtime;
}

AGENTIC_GENERATED_HIDDEN int configure(Runtime *runtime, std::string_view json,
                                       std::string &error) noexcept {
  if (!runtime) {
    error = "model handle is null";
    return 2;
  }
  try {
    gfsim::RuntimeLimits limits;
    if (!gfsim::parseModelConfigJson(json, limits, error))
      return 1;
    if (!runtime->system.setRuntimeLimits(limits)) {
      error = "generated model rejected runtime limits";
      runtime->system.resetScheduler();
      return 1;
    }
    return 0;
  } catch (const std::exception &exception) {
    error = exception.what();
  } catch (...) {
    error = "generated model configuration failed";
  }
  return 2;
}

AGENTIC_GENERATED_HIDDEN bool reset(Runtime *runtime,
                                    std::string &error) noexcept {
  if (!runtime) {
    error = "model handle is null";
    return false;
  }
  try {
    runtime->system.resetScheduler();
    runtime->model.reset();
)cpp";
  if (!plan.definition.empty()) {
    queueGraphSource << R"cpp(    if (!ac_generated::)cpp" << modelClass
                     << R"cpp(::schedule_initial_work(runtime->system)) {
      error = "generated model initial work could not be rescheduled";
      return false;
    }
)cpp";
  }
  queueGraphSource << R"cpp(    return true;
  } catch (const std::exception &exception) {
    error = exception.what();
  } catch (...) {
    error = "generated model reset failed";
  }
  return false;
}

AGENTIC_GENERATED_HIDDEN int step(Runtime *runtime, std::uint64_t &time,
                                  std::uint32_t &delta,
                                  std::string &error) noexcept {
  if (!runtime) {
    error = "model handle is null";
    return 3;
  }
  try {
    const bool advanced = runtime->system.step();
    const gfsim::Epoch epoch = runtime->system.currentEpoch();
    const gfsim::TerminationResult result = runtime->system.terminationResult();
    time = epoch.time;
    delta = epoch.delta;
    if (advanced)
      return 0;
    if (!runtime->system.isTerminated())
      return 1;
    if (result.classification == gfsim::TerminationClass::Failed) {
      error = result.diagnosticCode;
      if (result.message && !result.message->empty())
        error += ": " + *result.message;
      return 3;
    }
    return 2;
  } catch (const std::exception &exception) {
    error = exception.what();
  } catch (...) {
    error = "generated model step failed";
  }
  return 3;
}

AGENTIC_GENERATED_HIDDEN std::string statisticsJson(Runtime *runtime) {
  const auto statistics = runtime->system.statistics();
  std::ostringstream output;
  output << '[';
  for (std::size_t index = 0; index < statistics.size(); ++index) {
    const auto &statistic = statistics[index];
    if (index) output << ',';
    output << "{\"buckets\":[";
    for (std::size_t bucketIndex = 0; bucketIndex < statistic.buckets.size();
         ++bucketIndex) {
      if (bucketIndex) output << ',';
      const auto &bucket = statistic.buckets[bucketIndex];
      output << "{\"count\":" << bucket.count
             << ",\"upper_bound\":" << bucket.upperBound << '}';
    }
    output << "],\"count\":" << statistic.count << ",\"kind\":";
    constexpr std::string_view kinds[] = {"counter", "gauge", "histogram"};
    appendJsonString(output, kinds[static_cast<unsigned>(statistic.kind)]);
    output << ",\"last_update\":{\"delta\":" << statistic.lastUpdate.delta
           << ",\"time\":" << statistic.lastUpdate.time << '}'
           << ",\"maximum\":" << statistic.maximum
           << ",\"minimum\":" << statistic.minimum << ",\"name\":";
    appendJsonString(output, statistic.name);
    output << ",\"object_path\":";
    appendJsonString(output, statistic.objectPath);
    output << ",\"sum\":" << statistic.sum
           << ",\"value\":" << statistic.value << '}';
  }
  output << "]\n";
  return output.str();
}

#undef AGENTIC_GENERATED_HIDDEN
} // namespace agentic_generated_detail
)cpp";

  const std::string modelHeader = R"cpp(#ifndef AGENTIC_GENERATED_MODEL_H
#define AGENTIC_GENERATED_MODEL_H

#include "gfsim/model_api.h"

#endif // AGENTIC_GENERATED_MODEL_H
)cpp";

  std::ostringstream modelSource;
  modelSource << R"cpp(#define AGENTIC_MODEL_BUILD 1
#include "generated/model.h"

#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace agentic_generated_detail {
struct Runtime;
Runtime *create(std::string &error) noexcept;
void destroy(Runtime *runtime) noexcept;
int configure(Runtime *runtime, std::string_view json,
              std::string &error) noexcept;
bool reset(Runtime *runtime, std::string &error) noexcept;
int step(Runtime *runtime, std::uint64_t &time, std::uint32_t &delta,
         std::string &error) noexcept;
std::string statisticsJson(Runtime *runtime);
} // namespace agentic_generated_detail

struct AgenticModelV1 {
  enum class State { Created, Configured, Ready, Completed };
  agentic_generated_detail::Runtime *runtime = nullptr;
  State state = State::Created;
  std::string result;
  std::string error;
};

namespace {

void setBuffer(const std::string &value, AgenticModelBufferV1 *buffer) {
  buffer->data = reinterpret_cast<const std::uint8_t *>(value.data());
  buffer->size = value.size();
}

AgenticModelStatusV1 createModel(AgenticModelV1 **result) {
  if (!result)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  *result = nullptr;
  AgenticModelV1 *model = new (std::nothrow) AgenticModelV1();
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  model->runtime = agentic_generated_detail::create(model->error);
  if (!model->runtime) {
    delete model;
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
  *result = model;
  return AGENTIC_MODEL_STATUS_V1_OK;
}

void destroyModel(AgenticModelV1 *model) {
  if (!model) return;
  agentic_generated_detail::destroy(model->runtime);
  delete model;
}

AgenticModelStatusV1 failState(AgenticModelV1 *model, std::string message) {
  model->result.clear();
  model->error = std::move(message);
  return AGENTIC_MODEL_STATUS_V1_INVALID_STATE;
}

std::string_view inputView(const std::uint8_t *data, std::uint64_t size) {
  if (size == 0)
    return {};
  return {reinterpret_cast<const char *>(data), static_cast<std::size_t>(size)};
}

AgenticModelStatusV1 configure(AgenticModelV1 *model,
                               const std::uint8_t *data,
                               std::uint64_t size) {
  if (!model || (size != 0 && !data))
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  if (model->state != AgenticModelV1::State::Created)
    return failState(model, "configure_json requires a newly created model");
  std::string_view input = inputView(data, size);
  if (input.ends_with('\n'))
    input.remove_suffix(1);
  model->result.clear();
  model->error.clear();
  const int status =
      agentic_generated_detail::configure(model->runtime, input, model->error);
  if (status != 0)
    return status == 1 ? AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT
                       : AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  model->state = AgenticModelV1::State::Configured;
  return AGENTIC_MODEL_STATUS_V1_OK;
}

AgenticModelStatusV1 resetModel(AgenticModelV1 *model) {
  if (!model)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  if (model->state != AgenticModelV1::State::Configured &&
      model->state != AgenticModelV1::State::Ready &&
      model->state != AgenticModelV1::State::Completed)
    return failState(model, "reset requires configured state");
  model->result.clear();
  model->error.clear();
  if (!agentic_generated_detail::reset(model->runtime, model->error))
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  model->state = AgenticModelV1::State::Ready;
  return AGENTIC_MODEL_STATUS_V1_OK;
}

AgenticModelStatusV1 stepModel(AgenticModelV1 *model,
                               AgenticModelStepResultV1 *result) {
  if (!model || !result)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  if (result->struct_size != sizeof(*result)) {
    model->error = "step result struct_size does not match runtime ABI v1";
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  }
  if (model->state != AgenticModelV1::State::Ready)
    return failState(model, "step requires reset-ready state");
  model->result.clear();
  model->error.clear();
  result->reserved = 0;
  const int state = agentic_generated_detail::step(
      model->runtime, result->epoch_time, result->epoch_delta, model->error);
  result->state = state;
  if (state == AGENTIC_MODEL_STEP_V1_TERMINATED ||
      state == AGENTIC_MODEL_STEP_V1_FAILED)
    model->state = AgenticModelV1::State::Completed;
  return state == AGENTIC_MODEL_STEP_V1_FAILED
             ? AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE
             : AGENTIC_MODEL_STATUS_V1_OK;
}

AgenticModelStatusV1 statistics(AgenticModelV1 *model,
                                AgenticModelBufferV1 *result) {
  if (!model || !result)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  if (model->state != AgenticModelV1::State::Ready &&
      model->state != AgenticModelV1::State::Completed)
    return failState(model, "statistics_json requires reset-ready state");
  try {
    model->error.clear();
    model->result = agentic_generated_detail::statisticsJson(model->runtime);
    setBuffer(model->result, result);
    return AGENTIC_MODEL_STATUS_V1_OK;
  } catch (...) {
    model->error = "statistics serialization failed";
    return AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE;
  }
}

AgenticModelStatusV1 lastError(AgenticModelV1 *model,
                               AgenticModelBufferV1 *result) {
  if (!model || !result)
    return AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT;
  setBuffer(model->error, result);
  return AGENTIC_MODEL_STATUS_V1_OK;
}

const AgenticModelApiV1 api = {
    sizeof(AgenticModelApiV1), AGENTIC_MODEL_ABI_V1,
    createModel, destroyModel, configure, resetModel, stepModel, statistics,
    lastError};

} // namespace

extern "C" AGENTIC_MODEL_EXPORT const AgenticModelApiV1 *
agentic_model_query_v1(void) {
  return &api;
}
)cpp";

  std::vector<QueueGraphGeneratedFile> result;
  auto canonicalQueueGraph = plan.canonicalJson();
  if (!canonicalQueueGraph)
    return canonicalQueueGraph.takeError();
  auto sourceMap = plan.sourceMapJson();
  if (!sourceMap)
    return sourceMap.takeError();
  const std::string queueGraphBytes = *canonicalQueueGraph + "\n";
  const std::string sourceMapBytes = *sourceMap + "\n";
  auto costReport = generateQueueGraphCostReport(plan);
  if (!costReport)
    return costReport.takeError();
  result.push_back({"include/generated/model.h", modelHeader});
  if (structured && !structured->modules.empty()) {
    const std::string interfaceStem = sourceStem(plan) + "_interface";
    std::ostringstream interfaceHeader;
    interfaceHeader << "#pragma once\n\n"
                       "#include \"gfsim/bits.h\"\n\n"
                       "#include <array>\n#include <cstdint>\n#include <tuple>\n\n"
                       "namespace ac_generated {\n\n";
    for (const auto &type : structured->types) {
      interfaceHeader << type.definition;
    }
    interfaceHeader << "} // namespace ac_generated\n";
    result.push_back({"include/generated/interfaces/" + interfaceStem +
                          ".hpp",
                      interfaceHeader.str()});
    std::ostringstream helpersHeader;
    helpersHeader
        << "#pragma once\n\n"
           "#include \"generated/interfaces/"
        << interfaceStem
        << ".hpp\"\n"
           "#include \"gfsim/bits.h\"\n"
           "#include \"gfsim/dispatch.h\"\n"
           "#include \"gfsim/object.h\"\n"
           "#include \"gfsim/priority_encode.h\"\n"
           "#include \"gfsim/queue.h\"\n"
           "#include \"gfsim/queue_blocks.h\"\n\n"
           "#include <array>\n#include <cstdint>\n#include <limits>\n"
           "#include <optional>\n#include <stdexcept>\n#include <string>\n"
           "#include <string_view>\n#include <tuple>\n"
           "#include <utility>\n#include <vector>\n\n"
           "namespace ac_generated {\n\n"
        << structured->helperDeclarations << "} // namespace ac_generated\n";
    result.push_back({"include/generated/modules/queuegraph_helpers.hpp",
                      helpersHeader.str()});
    std::ostringstream helpersSource;
    helpersSource << "#include \"generated/modules/queuegraph_helpers.hpp\"\n\n"
                     "namespace ac_generated {\n\n"
                  << structured->helperDefinitions
                  << "} // namespace ac_generated\n";
    result.push_back(
        {"src/generated/helpers/queuegraph_helpers.cpp", helpersSource.str()});
    struct ModuleFileGroup {
      std::string fileStem;
      std::vector<std::string> childFileStems;
      std::string provenance;
      std::string header;
      std::string source;
    };
    std::vector<ModuleFileGroup> moduleFiles;
    llvm::StringMap<size_t> moduleFileIndices;
    for (const auto &unit : structured->modules) {
      auto [entry, inserted] =
          moduleFileIndices.try_emplace(unit.fileStem, moduleFiles.size());
      if (inserted)
        moduleFiles.push_back({unit.fileStem, {}, {}, {}, {}});
      ModuleFileGroup &group = moduleFiles[entry->getValue()];
      for (const std::string &child : unit.childFileStems)
        if (child != group.fileStem &&
            !llvm::is_contained(group.childFileStems, child))
          group.childFileStems.push_back(child);
      group.header.append(unit.header);
      group.provenance.append(unit.provenance);
      group.source.append(unit.source);
    }
    for (const ModuleFileGroup &group : moduleFiles) {
      std::ostringstream header;
      header << "#pragma once\n\n"
                "#include \"generated/interfaces/"
             << interfaceStem
             << ".hpp\"\n"
                "#include \"generated/modules/queuegraph_helpers.hpp\"\n"
                "#include <memory>\n";
      for (const std::string &child : group.childFileStems)
        header << "#include \"generated/modules/" << child << ".hpp\"\n";
      header << "\nnamespace ac_generated {\n\n"
             << group.header << "} // namespace ac_generated\n";
      result.push_back(
          {"include/generated/modules/" + group.fileStem + ".hpp", header.str()});
      std::ostringstream source;
      source << "#include \"generated/modules/" << group.fileStem
             << ".hpp\"\n\nnamespace ac_generated {\n\n"
             << group.provenance << group.source
             << "} // namespace ac_generated\n";
      result.push_back(
          {"src/generated/modules/" + group.fileStem + ".cpp", source.str()});
    }
    std::ostringstream dutHeader;
    dutHeader << "#pragma once\n\n"
                 "#include \"generated/interfaces/"
              << interfaceStem << ".hpp\"\n";
    llvm::StringSet<> includedDutModules;
    for (const auto &unit : structured->modules)
      if (includedDutModules.insert(unit.fileStem).second)
        dutHeader << "#include \"generated/modules/" << unit.fileStem
                  << ".hpp\"\n";
    dutHeader << "\nnamespace ac_generated {\n\n"
              << structured->rootClass << "} // namespace ac_generated\n";
    result.push_back({"include/generated/dut.h", dutHeader.str()});
  }
  result.push_back({"share/generated/cost-report.json", *costReport + "\n"});
  result.push_back({"share/generated/source-map.json", sourceMapBytes});
  result.push_back({"src/generated/model.cpp", modelSource.str()});
  result.push_back({"src/generated/queuegraph.cpp", queueGraphSource.str()});
  std::vector<std::string> generatedSources;
  for (const QueueGraphGeneratedFile &file : result)
    if (llvm::StringRef(file.relativePath).ends_with(".cpp"))
      generatedSources.push_back(file.relativePath);
  llvm::sort(generatedSources);
  std::ostringstream cmake;
  cmake << "cmake_minimum_required(VERSION 3.20)\n"
           "project(ac_generated_model LANGUAGES CXX)\n\n"
           "find_path(AC_GFSIM_INCLUDE_DIR gfsim/core.h REQUIRED)\n\n";
  std::vector<std::string> objectTargets;
  for (const std::string &source : generatedSources) {
    const llvm::StringRef path(source);
    std::string target = "ac_" + legalizeQueueGraphIdentifier(
                                      path.rsplit('/').second.drop_back(4));
    if (llvm::is_contained(objectTargets, target))
      return generatorError("generated C++ source target names collide");
    objectTargets.push_back(target);
    cmake << "add_library(" << target << " OBJECT " << source << ")\n"
          << "target_compile_features(" << target << " PUBLIC cxx_std_20)\n"
          << "target_include_directories(" << target << " PUBLIC\n"
          << "  ${CMAKE_CURRENT_SOURCE_DIR}/include\n"
          << "  ${AC_GFSIM_INCLUDE_DIR}\n"
          << ")\n\n";
  }
  cmake << "add_library(ac_generated_model STATIC\n";
  for (const std::string &target : objectTargets)
    cmake << "  $<TARGET_OBJECTS:" << target << ">\n";
  cmake << ")\n"
           "target_compile_features(ac_generated_model PUBLIC cxx_std_20)\n"
           "target_include_directories(ac_generated_model PUBLIC\n"
           "  ${CMAKE_CURRENT_SOURCE_DIR}/include\n"
           ")\n"
           "target_include_directories(ac_generated_model PUBLIC\n"
           "  ${AC_GFSIM_INCLUDE_DIR}\n"
           ")\n";
  result.push_back({"CMakeLists.txt", cmake.str()});
  return result;
}

} // namespace acir::codegen

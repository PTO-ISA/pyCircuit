#include "acir/CodeGen/QueueGraphGenerator.h"
#include "acir/CodeGen/QueueBlockContract.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <system_error>
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
  std::string result;
  for (char character : value)
    result.push_back(
        std::isalnum(static_cast<unsigned char>(character)) ? character : '_');
  if (result.empty() ||
      std::isdigit(static_cast<unsigned char>(result.front())))
    result.insert(result.begin(), '_');
  return result;
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

llvm::Expected<std::string> cppType(llvm::StringRef type) {
  if (type.starts_with('i')) {
    unsigned width = 0;
    if (!type.drop_front().getAsInteger(10, width) && width > 0) {
      if (width == 1)
        return std::string("bool");
      if (width <= 8)
        return std::string("std::uint8_t");
      if (width <= 16)
        return std::string("std::uint16_t");
      if (width <= 32)
        return std::string("std::uint32_t");
      if (width <= 64)
        return std::string("std::int64_t");
    }
  }
  constexpr llvm::StringLiteral prefix = "!ac.struct<@types::@";
  if (type.starts_with(prefix) && type.ends_with('>'))
    return type.drop_front(prefix.size()).drop_back().str();
  return generatorError("no C++ storage realization for ACIR type '" + type +
                        "'");
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

llvm::Expected<std::string> emitExpressionBody(const QueueBlockPlan &block,
                                               llvm::StringRef yield,
                                               unsigned indent,
                                               bool qualifyTables = false) {
  std::ostringstream output;
  std::string padding(indent, ' ');
  llvm::StringSet<> needed;
  needed.insert(yield);
  for (const QueueExpressionPlan &expression : llvm::reverse(block.expressions))
    if (needed.contains(expression.result))
      for (const std::string &operand : expression.operands)
        needed.insert(operand);
  for (const QueueExpressionPlan &expression : block.expressions) {
    if (!needed.contains(expression.result))
      continue;
    auto operand = [&](size_t index) -> llvm::Expected<llvm::StringRef> {
      if (index >= expression.operands.size())
        return generatorError("expression operand arity mismatch");
      return llvm::StringRef(expression.operands[index]);
    };
    if (expression.kind == "constant") {
      llvm::StringRef literal = expression.literal;
      output << padding << "auto " << expression.result << " = "
             << literal.split(" : ").first.str() << ";\n";
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
    if (expression.kind == "table_match") {
      if (expression.nestedYields.size() != 1)
        return generatorError("table.match predicate yield is missing");
      QueueBlockPlan nested;
      nested.expressions = expression.nestedExpressions;
      nested.yields = expression.nestedYields;
      auto predicate = emitExpressionBody(nested, nested.yields.front(),
                                          indent + 8, qualifyTables);
      if (!predicate)
        return predicate.takeError();
      const std::string table =
          qualifyTables ? "table_" + identifier(expression.table) : "table";
      output << padding << "std::uint64_t " << expression.result << " = 0;\n"
             << padding << "for (std::size_t index = 0; index < " << table
             << "->size(); "
                "++index) {\n"
             << padding << "  const auto &item = " << table << "->at(index);\n"
             << padding << "  if ([&]() {\n"
             << *predicate << padding << "  }())\n"
             << padding << "    " << expression.result
             << " |= (std::uint64_t{1} << index);\n"
             << padding << "}\n";
      continue;
    }
    auto first = operand(0);
    if (!first)
      return first.takeError();
    if (expression.kind == "get") {
      output << padding << "auto " << expression.result << " = " << first->str()
             << '.' << expression.field << ";\n";
      continue;
    }
    if (expression.kind == "table_get") {
      const std::string table =
          qualifyTables ? "table_" + identifier(expression.table) : "table";
      output << padding << "auto " << expression.result << " = " << table
             << "->checkedAt(static_cast<size_t>(" << first->str() << "));\n";
      continue;
    }
    if (expression.kind == "popcount") {
      output << padding << "auto " << expression.result
             << " = __builtin_popcountll(static_cast<unsigned long long>("
             << first->str() << "));\n";
      continue;
    }
    if (expression.kind == "table_choose_index" ||
        expression.kind == "table_choose_valid") {
      QueueBlockPlan nested;
      nested.expressions = expression.nestedExpressions;
      nested.yields = expression.nestedYields;
      output << padding << "std::uint64_t " << expression.result << " = 0;\n"
             << padding << "bool " << expression.result << "_found = false;\n"
             << padding << "std::uint64_t " << expression.result
             << "_best = 0;\n"
             << padding << "for (std::size_t index = 0; index < "
             << (qualifyTables ? "table_" + identifier(expression.table)
                               : std::string("table"))
             << "->size(); ++index) {\n"
             << padding << "  if ((static_cast<std::uint64_t>(" << first->str()
             << ") & (std::uint64_t{1} << index)) == 0) continue;\n";
      if (expression.predicate == "first") {
        output << padding << "  " << expression.result << " = index;\n"
               << padding << "  " << expression.result << "_found = true;\n"
               << padding << "  break;\n";
      } else {
        if (nested.yields.size() != 1)
          return generatorError("table.choose key yield is missing");
        auto key = emitExpressionBody(nested, nested.yields.front(), indent + 6,
                                      qualifyTables);
        if (!key)
          return key.takeError();
        const char *comparison = expression.predicate == "min" ? "<" : ">";
        output << padding << "  const auto &item = "
               << (qualifyTables ? "table_" + identifier(expression.table)
                                 : std::string("table"))
               << "->at(index);\n"
               << padding << "  auto key = [&]() {\n"
               << *key << padding << "  }();\n"
               << padding << "  if (!" << expression.result
               << "_found || static_cast<std::uint64_t>(key) " << comparison
               << " " << expression.result << "_best) {\n"
               << padding << "    " << expression.result << " = index;\n"
               << padding << "    " << expression.result << "_found = true;\n"
               << padding << "    " << expression.result
               << "_best = static_cast<std::uint64_t>(key);\n"
               << padding << "  }\n";
      }
      output << padding << "}\n";
      if (expression.kind == "table_choose_valid")
        output << padding << expression.result << " = " << expression.result
               << "_found;\n";
      continue;
    }
    auto second = operand(1);
    if (!second)
      return second.takeError();
    if (expression.kind == "with") {
      output << padding << "auto " << expression.result << " = " << first->str()
             << ";\n";
      output << padding << expression.result << '.' << expression.field << " = "
             << second->str() << ";\n";
      continue;
    }
    llvm::StringRef operation;
    if (expression.kind == "add")
      operation = "+";
    else if (expression.kind == "sub")
      operation = "-";
    else if (expression.kind == "mul")
      operation = "*";
    else if (expression.kind == "cmp") {
      operation = llvm::StringSwitch<llvm::StringRef>(expression.predicate)
                      .Case("eq", "==")
                      .Case("ne", "!=")
                      .Case("slt", "<")
                      .Case("sle", "<=")
                      .Case("sgt", ">")
                      .Case("sge", ">=")
                      .Default("");
    }
    if (operation.empty())
      return generatorError("unsupported Var expression kind '" +
                            expression.kind + "'");
    output << padding << "auto " << expression.result << " = " << first->str()
           << ' ' << operation.str() << ' ' << second->str() << ";\n";
  }
  output << padding << "return " << yield.str() << ";\n";
  return output.str();
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

const SlotPlan *findSlot(const QueueGraphPlan &plan, llvm::StringRef name) {
  auto found =
      std::find_if(plan.slots.begin(), plan.slots.end(),
                   [&](const SlotPlan &slot) { return slot.name == name; });
  return found == plan.slots.end() ? nullptr : &*found;
}

bool isRuntimeBlock(const QueueBlockPlan &block) {
  return block.kind != "source";
}

} // namespace

llvm::Expected<std::string> generateQueueGraphCpp(const QueueGraphPlan &plan) {
  if (plan.system.empty() || plan.queues.empty() || plan.blocks.empty())
    return generatorError("QueueGraph plan is incomplete");
  if (!plan.scopes.empty()) {
    const QueueBlockContract *scope = findQueueBlockContract("scope");
    if (!scope || !scope->gfsimAvailable)
      return generatorError("official opcode has no gfsim lowering: 'scope'");
  }
  for (const QueueBlockPlan &block : plan.blocks) {
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
         block.yields.size() != 3 || block.table.empty()))
      return generatorError("table write contract is unsupported");
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
  for (auto [index, table] : llvm::enumerate(plan.tables)) {
    tableIds[table.name] = nextId++;
    tableMembers[table.name] = "table_" + std::to_string(index) + "_";
  }

  std::ostringstream output;
  output << "// Generated from frozen ACIR QueueGraph plan; do not edit.\n";
  if (!plan.specializationFingerprint.empty())
    output << "// Specialization: " << plan.specializationFingerprint << "\n";
  output << "#include \"gfsim/dispatch.h\"\n"
            "#include \"gfsim/object.h\"\n"
            "#include \"gfsim/queue.h\"\n"
            "#include \"gfsim/queue_blocks.h\"\n\n"
            "#include <array>\n#include <cstdint>\n#include <limits>\n"
            "#include <tuple>\n\n"
            "namespace ac_generated {\n\n";
  for (const QueuePayloadPlan &payload : plan.payloads) {
    output << "struct " << payload.name << " {\n";
    for (const QueuePayloadFieldPlan &field : payload.fields) {
      auto type = cppType(field.type);
      if (!type)
        return type.takeError();
      output << "  " << *type << ' ' << field.name << "{};\n";
    }
    output << "  bool operator==(const " << payload.name
           << " &) const = default;\n";
    output << "};\n\n";
  }

  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind != "transform" && block->kind != "route" &&
        block->kind != "select" && block->kind != "expect" &&
        block->kind != "dependency" && block->kind != "credit" &&
        block->kind != "reorder" && block->kind != "feedback" &&
        block->kind != "table_read" && block->kind != "table_write" &&
        block->kind != "slot")
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
      output << "  bool operator()() const {\n";
      auto body = emitExpressionBody(*block, block->yields.front(), 4, true);
      if (!body)
        return body.takeError();
      output << *body << "  }\n};\n\n";
      continue;
    }
    if (block->kind == "table_read" || block->kind == "table_write") {
      const TablePlan *table = findTable(plan, block->table);
      auto entryType = table ? cppType(table->entryType)
                             : llvm::Expected<std::string>(
                                   generatorError("table declaration missing"));
      if (!entryType)
        return entryType.takeError();
      std::string inputType;
      if (!block->inputs.empty()) {
        const QueuePlan *input = findQueue(plan, block->inputs.front());
        auto type = input ? cppType(input->payloadType)
                          : llvm::Expected<std::string>(
                                generatorError("table input Queue missing"));
        if (!type)
          return type.takeError();
        inputType = std::move(*type);
      }
      const std::vector<llvm::StringRef> policyNames =
          block->kind == "table_read"
              ? std::vector<llvm::StringRef>{"address", "when"}
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
        output << "  " << *resultCppType << " operator()(";
        if (!inputType.empty())
          output << "const " << inputType << " &item";
        output << ") const {\n";
        auto body = emitExpressionBody(*block, block->yields[policyIndex], 4);
        if (!body)
          return body.takeError();
        output << *body << "  }\n};\n\n";
      }
      continue;
    }
    if (block->kind == "dependency") {
      const QueuePlan *input = findQueue(plan, block->inputs.front());
      if (!input)
        return generatorError("dependency input Queue is missing");
      auto inputType = cppType(input->payloadType);
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
        auto body = emitExpressionBody(*block, block->yields[policyIndex], 4);
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
        auto type = cppType(input->payloadType);
        if (!type)
          return type.takeError();
        inputTypes.push_back(std::move(*type));
      }
      for (const std::string &outputName : block->outputs) {
        const QueuePlan *result = findQueue(plan, outputName);
        if (!result)
          return generatorError("atomic transform output Queue is missing");
        auto type = cppType(result->payloadType);
        if (!type)
          return type.takeError();
        outputTypes.push_back(std::move(*type));
      }
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
        auto body = emitExpressionBody(*block, yield, 8);
        if (!body)
          return body.takeError();
        output << *body << "      }()"
               << (yieldIndex + 1 == block->yields.size() ? "\n" : ",\n");
      }
      output << "    };\n  }\n};\n\n";
      continue;
    }
    const size_t expectedYields = block->kind == "feedback" ? 2 : 1;
    if (block->yields.size() != expectedYields ||
        (block->kind != "select" && block->inputs.size() != 1))
      return generatorError("Queue policy arity is unsupported");
    const QueuePlan *input = findQueue(plan, block->inputs.front());
    if (!input)
      return generatorError("policy input Queue is missing");
    auto inputType = cppType(input->payloadType);
    if (!inputType)
      return inputType.takeError();
    std::string policy =
        "block_" + std::to_string(index) +
        (block->kind == "feedback" ? "_update_policy" : "_policy");
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
      auto resultType = cppType(result->payloadType);
      if (!resultType)
        return resultType.takeError();
      output << *resultType;
    }
    output << " operator()(const " << *inputType << " &item) const {\n";
    auto body = emitExpressionBody(*block, block->yields.front(), 4);
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
      auto condition = emitExpressionBody(*block, block->yields[1], 4);
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
    auto inputType = cppType(input->payloadType);
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
        auto body =
            emitExpressionBody(*endpoint, endpoint->yields[policyIndex], 6);
        if (!body)
          return body.takeError();
        output << *body << "    }\n";
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
    auto type = cppType(queue.payloadType);
    auto parent = modulePointer(queueOwners[queue.name]);
    if (!type)
      return type.takeError();
    if (!parent)
      return parent.takeError();
    appendInitializer(initializers, queueMembers[queue.name], "(\"", queue.name,
                      "\", ", queueIds[queue.name], ", ", *parent, ", ",
                      queue.depth,
                      ", std::numeric_limits<size_t>::max(), nullptr, ",
                      queue.latency, ", ", queue.rate, ")");
  }
  for (const TablePlan &table : plan.tables) {
    auto parent = modulePointer(table.ownerPath);
    if (!parent)
      return parent.takeError();
    appendInitializer(initializers, tableMembers[table.name], "(\"", table.name,
                      "\", ", tableIds[table.name], ", ", *parent, ", ",
                      table.entries, ")");
  }
  std::string slotPolicyPointers;
  for (auto [index, slot] : llvm::enumerate(plan.slots)) {
    (void)slot;
    slotPolicyPointers.append(", &slot_")
        .append(std::to_string(index))
        .append("_state_");
  }
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    auto state = feedbackStateIds.find(index);
    if (state == feedbackStateIds.end())
      continue;
    const QueuePlan *input = findQueue(plan, block->inputs[0]);
    auto type = input ? cppType(input->payloadType)
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
    std::string member = "block_" + std::to_string(index) + "_";
    std::string key = block->name + "#" + std::to_string(index);
    std::string instanceName = block->kind + "_" + block->name;
    if (block->kind == "transform") {
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
      auto type = input ? cppType(input->payloadType)
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
      auto type = result ? cppType(result->payloadType)
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
      auto type = result ? cppType(result->payloadType)
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
      appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                        blockIds[key], ", ", *parent, ", ",
                        queueMembers[block->inputs[0]], ", ",
                        queueMembers[block->outputs[0]], ", ", block->capacity,
                        ", ", block->resources, ", ", block->noDependency, ")");
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
    } else if (block->kind == "table_read") {
      auto table = tableMembers.find(block->table);
      if (table == tableMembers.end())
        return generatorError("table read declaration is missing");
      if (block->inputs.empty())
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ", table->getValue(),
                          ", ", queueMembers[block->outputs[0]], ", block_",
                          index, "_address_policy{&", table->getValue(),
                          slotPolicyPointers, "}, block_", index,
                          "_when_policy{&", table->getValue(),
                          slotPolicyPointers, "})");
      else
        appendInitializer(initializers, member, "(\"", instanceName, "\", ",
                          blockIds[key], ", ", *parent, ", ", table->getValue(),
                          ", ", queueMembers[block->inputs[0]], ", ",
                          queueMembers[block->outputs[0]], ", block_", index,
                          "_address_policy{&", table->getValue(),
                          slotPolicyPointers, "}, block_", index,
                          "_when_policy{&", table->getValue(),
                          slotPolicyPointers, "})");
    } else if (block->kind == "table_write") {
      auto table = tableMembers.find(block->table);
      if (table == tableMembers.end())
        return generatorError("table write declaration is missing");
      if (block->inputs.empty())
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", table->getValue(), ", block_", index,
            "_address_policy{&", table->getValue(), slotPolicyPointers,
            "}, block_", index, "_enable_policy{&", table->getValue(),
            slotPolicyPointers, "}, block_", index, "_value_policy{&",
            table->getValue(), slotPolicyPointers, "})");
      else
        appendInitializer(
            initializers, member, "(\"", instanceName, "\", ", blockIds[key],
            ", ", *parent, ", ", table->getValue(), ", ",
            queueMembers[block->inputs[0]], ", block_", index,
            "_address_policy{&", table->getValue(), slotPolicyPointers,
            "}, block_", index, "_enable_policy{&", table->getValue(),
            slotPolicyPointers, "}, block_", index, "_value_policy{&",
            table->getValue(), slotPolicyPointers, "})");
    } else if (block->kind == "slot") {
      const SlotPlan *slot = findSlot(plan, block->slot);
      if (!slot)
        return generatorError("slot declaration is missing");
      auto slotIndex = static_cast<size_t>(slot - plan.slots.data());
      std::string policyPointers = slotPolicyPointers;
      for (const TablePlan &table : plan.tables)
        if (referencesTable(block->expressions, table.name))
          policyPointers.append(", &").append(tableMembers[table.name]);
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
    auto type = input ? cppType(input->payloadType)
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
    auto line = attach(block->scope, "block_" + std::to_string(index) + "_");
    if (!line)
      return line.takeError();
    output << *line << '\n';
  }
  output << "  }\n\n";
  for (const QueueBlockPlan &block : plan.blocks)
    if (block.kind == "source") {
      const QueuePlan *queue = findQueue(plan, block.outputs.front());
      auto type = queue ? cppType(queue->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("source Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::SimQueue<" << *type << "> &" << block.outputs.front()
             << "() { return " << queueMembers[block.outputs.front()]
             << "; }\n";
    }
  sinkIndex = 0;
  size_t observationIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (block->kind == "sink") {
      const QueuePlan *queue = findQueue(plan, block->inputs.front());
      auto type = queue ? cppType(queue->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("sink Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  const std::vector<" << *type << "> &sink_" << sinkIndex
             << "_values() const { return block_" << index
             << "_.received(); }\n";
      ++sinkIndex;
    } else if (block->kind == "observe") {
      const QueuePlan *queue = findQueue(plan, block->inputs.front());
      auto type = queue ? cppType(queue->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("observation Queue is missing"));
      if (!type)
        return type.takeError();
      output << "  const std::vector<" << *type << "> &observation_"
             << observationIndex << "_values() const { return block_" << index
             << "_.observed(); }\n";
      ++observationIndex;
    }
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
  output << "\n  std::array<gfsim::DispatchRow, " << nextId
         << "> dispatch_rows() {\n    return {\n";
  for (const QueuePlan &queue : plan.queues)
    output << "        gfsim::makeDispatchRow(&" << queueMembers[queue.name]
           << "),\n";
  for (auto [index, block] : llvm::enumerate(runtimeBlocks))
    if (feedbackStateIds.contains(index))
      output << "        gfsim::makeDispatchRow(&block_" << index
             << "_state_),\n";
  for (size_t index = 0; index < runtimeBlocks.size(); ++index) {
    output << "        gfsim::makeDispatchRow(&block_" << index << "_),\n";
  }
  for (size_t index = 0; index < plan.memoryInstances.size(); ++index)
    output << "        gfsim::makeDispatchRow(&memory_" << index << "_),\n";
  for (const TablePlan &table : plan.tables)
    output << "        gfsim::makeDispatchRow(&" << tableMembers[table.name]
           << "),\n";
  output << "    };\n  }\n\nprivate:\n";
  for (const std::string &scope : plan.scopes)
    output << "  gfsim::Module " << scopeMembers[scope] << ";\n";
  for (const QueuePlan &queue : plan.queues) {
    auto type = cppType(queue.payloadType);
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
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (!feedbackStateIds.contains(index))
      continue;
    const QueuePlan *input = findQueue(plan, block->inputs[0]);
    auto type = input ? cppType(input->payloadType)
                      : llvm::Expected<std::string>(
                            generatorError("feedback state type is missing"));
    if (!type)
      return type.takeError();
    output << "  gfsim::SimQueue<gfsim::FeedbackToken<" << *type << ">> block_"
           << index << "_state_;\n";
  }
  sinkIndex = 0;
  for (auto [index, block] : llvm::enumerate(runtimeBlocks)) {
    if (block->kind == "transform") {
      if (block->inputs.size() == 1 && block->outputs.size() == 1) {
        const QueuePlan *input = findQueue(plan, block->inputs[0]);
        const QueuePlan *result = findQueue(plan, block->outputs[0]);
        auto inputType = input ? cppType(input->payloadType)
                               : llvm::Expected<std::string>(
                                     generatorError("transform input missing"));
        auto resultType = result ? cppType(result->payloadType)
                                 : llvm::Expected<std::string>(generatorError(
                                       "transform output missing"));
        if (!inputType)
          return inputType.takeError();
        if (!resultType)
          return resultType.takeError();
        output << "  gfsim::QueueTransform<" << *inputType << ", "
               << *resultType << ", block_" << index << "_policy, "
               << result->rate << "> block_" << index << "_;\n";
      } else {
        output << "  gfsim::QueueAtomicTransform<block_" << index
               << "_policy, std::tuple<";
        for (auto [inputIndex, inputName] : llvm::enumerate(block->inputs)) {
          const QueuePlan *input = findQueue(plan, inputName);
          auto type = input ? cppType(input->payloadType)
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
          auto type = result ? cppType(result->payloadType)
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
      auto type = input ? cppType(input->payloadType)
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
      auto dataType = result ? cppType(result->payloadType)
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
      auto type = result ? cppType(result->payloadType)
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
        auto type = input ? cppType(input->payloadType)
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
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("reorder input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueReorder<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "dependency") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("dependency input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueDependency<" << *type << ", block_" << index
             << "_key_policy, block_" << index << "_dependency_policy, block_"
             << index << "_resource_policy, block_" << index
             << "_cost_policy> block_" << index << "_;\n";
    } else if (block->kind == "credit") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("credit input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueCredit<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
    } else if (block->kind == "feedback") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("feedback input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueFeedback<" << *type << ", block_" << index
             << "_update_policy, block_" << index << "_condition_policy> block_"
             << index << "_;\n";
    } else if (block->kind == "expect") {
      const QueuePlan *input = findQueue(plan, block->inputs[0]);
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("expect input missing"));
      if (!type)
        return type.takeError();
      output << "  gfsim::QueueExpect<" << *type << ", block_" << index
             << "_policy> block_" << index << "_;\n";
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
        auto inputType = input ? cppType(input->payloadType)
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
               << "_enable_policy, block_" << index << "_value_policy> block_"
               << index << "_;\n";
      } else {
        const QueuePlan *input = findQueue(plan, block->inputs.front());
        auto inputType = input ? cppType(input->payloadType)
                               : llvm::Expected<std::string>(generatorError(
                                     "table write input missing"));
        if (!inputType)
          return inputType.takeError();
        output << "  gfsim::QueueTableWrite<" << *inputType << ", "
               << *entryType << ", block_" << index << "_address_policy, block_"
               << index << "_enable_policy, block_" << index
               << "_value_policy> block_" << index << "_;\n";
      }
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
      auto type = input ? cppType(input->payloadType)
                        : llvm::Expected<std::string>(
                              generatorError("sink input missing"));
      if (!type)
        return type.takeError();
      if (block->kind == "sink") {
        output << "  gfsim::QueueSink<" << *type << "> block_" << index
               << "_;\n";
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
    auto type = input ? cppType(input->payloadType)
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

} // namespace acir::codegen

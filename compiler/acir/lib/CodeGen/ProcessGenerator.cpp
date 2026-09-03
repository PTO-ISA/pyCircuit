#include "ProcessGenerator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FormatVariadic.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <utility>

namespace acir::codegen::detail {
namespace {

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

llvm::Error processError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      llvm::Twine("ACLOWER-PROCESS-STATE: ") + message);
}

bool isIdentifier(llvm::StringRef value) {
  if (value.empty() ||
      !(std::isalpha(static_cast<unsigned char>(value.front())) ||
        value.front() == '_'))
    return false;
  return std::all_of(value.drop_front().begin(), value.end(), [](char value) {
    return std::isalnum(static_cast<unsigned char>(value)) || value == '_';
  });
}

const BindingPlan *findBinding(const ModelPlan &plan, llvm::StringRef symbol) {
  auto found = std::find_if(
      plan.bindings.begin(), plan.bindings.end(),
      [&](const BindingPlan &binding) { return binding.symbol == symbol; });
  return found == plan.bindings.end() ? nullptr : &*found;
}

const BindingPlan *findImplementationBinding(const ModelPlan &plan,
                                             llvm::StringRef symbol) {
  auto found = std::find_if(plan.bindings.begin(), plan.bindings.end(),
                            [&](const BindingPlan &binding) {
                              return binding.implementation == symbol;
                            });
  return found == plan.bindings.end() ? nullptr : &*found;
}

const TypePlan *findType(const ModelPlan &plan, llvm::StringRef symbol) {
  auto found =
      std::find_if(plan.types.begin(), plan.types.end(),
                   [&](const TypePlan &type) { return type.symbol == symbol; });
  return found == plan.types.end() ? nullptr : &*found;
}

llvm::Expected<std::string> wakeKind(const TypePlan &implementation) {
  llvm::StringRef symbol(implementation.symbol);
  if (symbol.starts_with("acir_impl_wake_condition"))
    return std::string("Condition");
  if (symbol.starts_with("acir_impl_wake_resource"))
    return std::string("Resource");
  if (symbol.starts_with("acir_impl_wake_event_queue"))
    return std::string("EventQueue");
  if (symbol.starts_with("acir_impl_wake_next_delta"))
    return std::string("NextDelta");
  return processError("generated wake implementation has an unknown role");
}

llvm::Error emitWakeHelper(std::ostringstream &output,
                           const TypePlan &implementation) {
  llvm::StringRef name(implementation.cppType);
  if (!name.consume_front("acir::generated::") || !isIdentifier(name))
    return processError(
        "generated wake implementation has an invalid C++ name");
  auto kind = wakeKind(implementation);
  if (!kind)
    return kind.takeError();
  output << "inline gfsim::ProcessWake " << name.str()
         << "() { return {gfsim::ProcessWakeKind::" << *kind << ", 0}; }\n";
  return llvm::Error::success();
}

llvm::Expected<std::string> cppType(const ModelPlan &plan,
                                    llvm::StringRef type) {
  if (type == "i1")
    return std::string("bool");
  if (type == "i8")
    return std::string("std::int8_t");
  if (type == "i16")
    return std::string("std::int16_t");
  if (type == "i32")
    return std::string("std::int32_t");
  if (type == "i64")
    return std::string("std::int64_t");
  if (type == "index")
    return std::string("std::size_t");
  if (type == "f32")
    return std::string("float");
  if (type == "f64")
    return std::string("double");

  const size_t symbolStart = type.find('@');
  const size_t symbolEnd = type.find('>', symbolStart);
  if (symbolStart == llvm::StringRef::npos ||
      symbolEnd == llvm::StringRef::npos)
    return processError("process value has no C++ type realization");
  const llvm::StringRef symbol = type.slice(symbolStart + 1, symbolEnd);
  if (type.starts_with("!acsim.ref<")) {
    if (const BindingPlan *binding = findBinding(plan, symbol))
      return binding->cppSymbol;
  } else if (type.starts_with("!acsim.wake<")) {
    return std::string("gfsim::ProcessWake");
  } else if (const TypePlan *realization = findType(plan, symbol)) {
    return realization->cppType;
  }
  return processError("process value references an unknown type");
}

const LiveSlotPlan *findSlot(const ProcessPlan &process, llvm::StringRef name) {
  auto found =
      std::find_if(process.liveSlots.begin(), process.liveSlots.end(),
                   [&](const LiveSlotPlan &slot) { return slot.name == name; });
  return found == process.liveSlots.end() ? nullptr : &*found;
}

const PcStatePlan *findState(const ProcessPlan &process, llvm::StringRef name) {
  auto found = std::find_if(
      process.states.begin(), process.states.end(),
      [&](const PcStatePlan &state) { return state.name == name; });
  return found == process.states.end() ? nullptr : &*found;
}

llvm::Expected<std::string> pcType(const ProcessPlan &process) {
  const uint32_t maximum =
      process.states.empty() ? 0 : process.states.back().ordinal;
  if (maximum <= std::numeric_limits<uint8_t>::max())
    return std::string("uint8_t");
  if (maximum <= std::numeric_limits<uint16_t>::max())
    return std::string("uint16_t");
  return std::string("uint32_t");
}

GeneratedFile makeFile(std::string path, std::string content) {
  GeneratedFile file{std::move(path), std::move(content), {}};
  file.fingerprint = computeFingerprint(file.content);
  return file;
}

llvm::Expected<std::string> scalarLiteral(const llvm::json::Value &value,
                                          llvm::StringRef resultType) {
  if (auto boolean = value.getAsBoolean())
    return std::string(*boolean ? "true" : "false");
  if (auto integer = value.getAsInteger())
    return std::to_string(*integer);
  if (auto number = value.getAsNumber()) {
    std::string literal = llvm::formatv("{0}", *number).str();
    if (resultType == "f32")
      literal.push_back('f');
    return literal;
  }
  return processError("scalar constant has no closed C++ literal");
}

bool isArithmeticOperation(llvm::StringRef name) {
  return llvm::StringSwitch<bool>(name)
      .Cases({"arith.cmpi", "arith.cmpf", "arith.addi", "arith.subi"}, true)
      .Cases({"arith.muli", "arith.divui", "arith.divsi", "arith.remui"}, true)
      .Cases({"arith.remsi", "arith.andi", "arith.ori", "arith.xori"}, true)
      .Cases({"arith.shli", "arith.shrui", "arith.shrsi", "arith.select"}, true)
      .Cases({"arith.index_cast", "arith.extui", "arith.extsi", "arith.trunci"},
             true)
      .Cases({"arith.addf", "arith.subf", "arith.mulf", "arith.divf"}, true)
      .Case("arith.negf", true)
      .Default(false);
}

bool isIndexOperation(llvm::StringRef name) {
  return llvm::StringSwitch<bool>(name)
      .Cases({"index.add", "index.sub", "index.mul", "index.divs"}, true)
      .Cases({"index.divu", "index.rems", "index.remu", "index.cmp"}, true)
      .Cases({"index.casts", "index.castu"}, true)
      .Default(false);
}

size_t scalarArity(llvm::StringRef name) {
  if (name == "arith.negf" || name == "arith.index_cast" ||
      name == "arith.extui" || name == "arith.extsi" ||
      name == "arith.trunci" || name == "index.casts" || name == "index.castu")
    return 1;
  if (name == "arith.select")
    return 3;
  return 2;
}

llvm::Error validateScalarOperation(
    const ModelPlan &plan, const std::map<std::string, std::string> &values,
    llvm::StringRef operationName, const std::vector<std::string> &arguments,
    const std::vector<std::string> &results,
    const std::vector<std::string> &resultTypes, llvm::StringRef predicate,
    bool arithmetic) {
  if ((arithmetic ? !isArithmeticOperation(operationName)
                  : !isIndexOperation(operationName)) ||
      arguments.size() != scalarArity(operationName) || results.size() != 1 ||
      resultTypes.size() != 1)
    return processError("scalar operation shape is outside the closed subset");
  for (const std::string &argument : arguments)
    if (!values.contains(argument))
      return processError("operation uses a value outside its PC");
  auto resultType = cppType(plan, resultTypes.front());
  if (!resultType)
    return resultType.takeError();
  const bool isComparison = operationName == "arith.cmpi" ||
                            operationName == "arith.cmpf" ||
                            operationName == "index.cmp";
  if (isComparison != !predicate.empty())
    return processError("scalar comparison predicate is invalid");
  return llvm::Error::success();
}

llvm::Error
validateOperations(const ModelPlan &plan, const ProcessPlan &process,
                   const std::vector<ProcessOperationPlan> &operations,
                   std::map<std::string, std::string> &values) {
  auto requireValue = [&](llvm::StringRef value) -> llvm::Error {
    if (!values.contains(value.str()))
      return processError("operation uses a value outside its PC");
    return llvm::Error::success();
  };
  auto addResults = [&](const std::vector<std::string> &results,
                        const std::vector<std::string> &types) -> llvm::Error {
    if (results.size() != types.size())
      return processError("operation result arity is inconsistent");
    for (auto [result, type] : llvm::zip_equal(results, types)) {
      if (!isIdentifier(result) || values.contains(result))
        return processError("operation result is not a fresh identifier");
      auto realization = cppType(plan, type);
      if (!realization)
        return realization.takeError();
      values.emplace(result, type);
    }
    return llvm::Error::success();
  };

  for (const ProcessOperationPlan &operation : operations) {
    llvm::Error error = std::visit(
        Overloaded{
            [&](const ConstantPlan &constant) -> llvm::Error {
              if (!isIdentifier(constant.resultValue) ||
                  values.contains(constant.resultValue))
                return processError("scalar constant is invalid");
              auto type = cppType(plan, constant.resultType);
              if (!type)
                return type.takeError();
              auto literal =
                  scalarLiteral(constant.canonicalValue, constant.resultType);
              if (!literal)
                return literal.takeError();
              values.emplace(constant.resultValue, constant.resultType);
              return llvm::Error::success();
            },
            [&](const ArithmeticPlan &scalar) -> llvm::Error {
              if (auto validation = validateScalarOperation(
                      plan, values, scalar.operationName, scalar.arguments,
                      scalar.results, scalar.resultTypes, scalar.predicate,
                      true))
                return validation;
              return addResults(scalar.results, scalar.resultTypes);
            },
            [&](const IndexPlan &scalar) -> llvm::Error {
              if (auto validation = validateScalarOperation(
                      plan, values, scalar.operationName, scalar.arguments,
                      scalar.results, scalar.resultTypes, scalar.predicate,
                      false))
                return validation;
              return addResults(scalar.results, scalar.resultTypes);
            },
            [&](const LiveLoadPlan &load) -> llvm::Error {
              const LiveSlotPlan *slot = findSlot(process, load.slot);
              if (!slot || slot->type != load.type)
                return processError("live load slot or type is invalid");
              return addResults({load.resultValue}, {load.type});
            },
            [&](const LiveStorePlan &store) -> llvm::Error {
              if (auto validation = requireValue(store.sourceValue))
                return validation;
              const LiveSlotPlan *slot = findSlot(process, store.slot);
              if (!slot || values.at(store.sourceValue) != slot->type)
                return processError("live store slot or type is invalid");
              return llvm::Error::success();
            },
            [&](const InlineCallPlan &call) -> llvm::Error {
              const BindingPlan *binding = findBinding(plan, call.callee);
              const TypePlan *implementation = findType(plan, call.callee);
              if ((!binding || binding->effect != BindingEffect::Pure) &&
                  (!implementation ||
                   implementation->kind != TypeKind::Implementation))
                return processError("inline callee is not a pure binding");
              for (const std::string &argument : call.arguments)
                if (auto validation = requireValue(argument))
                  return validation;
              return addResults(call.results, call.resultTypes);
            },
            [&](const InvokePlan &call) -> llvm::Error {
              const BindingPlan *binding = findBinding(plan, call.callee);
              const TypePlan *implementation = findType(plan, call.callee);
              if ((!binding || binding->effect != BindingEffect::Stateful) &&
                  (!implementation ||
                   implementation->kind != TypeKind::Implementation))
                return processError("invoke callee is not a stateful binding");
              for (const std::string &argument : call.arguments)
                if (auto validation = requireValue(argument))
                  return validation;
              return addResults(call.results, call.resultTypes);
            }},
        operation);
    if (error)
      return error;
  }
  return llvm::Error::success();
}

llvm::Error validateProcess(const ModelPlan &plan, const ProcessPlan &process) {
  if (!isIdentifier(process.className) || !isIdentifier(process.symbol) ||
      process.states.empty() || process.fairnessWork == 0)
    return processError("process identity or fairness is invalid");

  std::set<std::string> pcNames;
  for (auto [ordinal, state] : llvm::enumerate(process.states)) {
    if (state.ordinal != ordinal || !isIdentifier(state.name) ||
        !pcNames.insert(state.name).second)
      return processError("process PCs are not dense identifiers");
  }
  if (!pcNames.contains(process.entryPc))
    return processError("entry PC is outside the closed PC set");

  std::set<std::string> slots;
  for (const LiveSlotPlan &slot : process.liveSlots) {
    if (!isIdentifier(slot.name) || !slots.insert(slot.name).second)
      return processError("live slots are not unique identifiers");
    auto type = cppType(plan, slot.type);
    if (!type)
      return type.takeError();
  }

  for (const PcStatePlan &state : process.states) {
    std::map<std::string, std::string> values;
    for (auto [index, capture] : llvm::enumerate(process.captures))
      values.emplace("arg" + std::to_string(index), capture.type);
    if (state.blocks.size() > 1) {
      for (const PcBlockPlan &block : state.blocks) {
        for (const BlockArgumentPlan &argument : block.arguments) {
          if (!isIdentifier(argument.name))
            return processError("block argument is not a fresh process value");
          auto prior = values.find(argument.name);
          if (prior != values.end()) {
            if (block.ordinal == 0 && prior->second == argument.type)
              continue;
            return processError("block argument is not a fresh process value");
          }
          auto type = cppType(plan, argument.type);
          if (!type)
            return type.takeError();
          values.emplace(argument.name, argument.type);
        }
      }
    }

    if (auto error =
            validateOperations(plan, process, state.operations, values))
      return error;

    if (state.blocks.size() > 1) {
      for (const PcBlockPlan &block : state.blocks) {
        std::map<std::string, std::string> localValues;
        for (auto [index, capture] : llvm::enumerate(process.captures))
          localValues.emplace("arg" + std::to_string(index), capture.type);
        for (const BlockArgumentPlan &argument : block.arguments)
          localValues.emplace(argument.name, argument.type);

        auto requireLocalValue = [&](llvm::StringRef value) -> llvm::Error {
          if (!localValues.contains(value.str()))
            return processError(
                "multi-block operation uses a value outside its block");
          return llvm::Error::success();
        };
        if (auto error = validateOperations(plan, process, block.operations,
                                            localValues))
          return error;

        auto validateSuccessor =
            [&](uint32_t targetOrdinal,
                const std::vector<std::string> &arguments) -> llvm::Error {
          if (targetOrdinal >= state.blocks.size())
            return processError("branch target is outside its PC");
          const PcBlockPlan &target = state.blocks[targetOrdinal];
          if (arguments.size() != target.arguments.size())
            return processError("branch successor arity is inconsistent");
          for (auto [argument, targetArgument] :
               llvm::zip_equal(arguments, target.arguments)) {
            if (auto error = requireLocalValue(argument))
              return error;
            if (localValues.at(argument) != targetArgument.type)
              return processError("branch successor type is inconsistent");
          }
          return llvm::Error::success();
        };
        llvm::Error blockError = std::visit(
            Overloaded{
                [&](const BranchPlan &branch) -> llvm::Error {
                  return validateSuccessor(branch.targetBlock,
                                           branch.arguments);
                },
                [&](const ConditionalBranchPlan &branch) -> llvm::Error {
                  if (auto error = requireLocalValue(branch.condition))
                    return error;
                  if (localValues.at(branch.condition) != "i1")
                    return processError(
                        "conditional branch condition is not i1");
                  if (auto error = validateSuccessor(branch.trueBlock,
                                                     branch.trueArguments))
                    return error;
                  return validateSuccessor(branch.falseBlock,
                                           branch.falseArguments);
                },
                [&](const ContinuePlan &next) -> llvm::Error {
                  return findState(process, next.targetPc)
                             ? llvm::Error::success()
                             : processError(
                                   "continue target is outside the PC set");
                },
                [&](const SuspendPlan &suspend) -> llvm::Error {
                  auto found = localValues.find(suspend.wakeValue);
                  if (found == localValues.end() ||
                      !llvm::StringRef(found->second)
                           .starts_with("!acsim.wake<") ||
                      !findState(process, suspend.targetPc))
                    return processError("suspend wake or target is invalid");
                  return llvm::Error::success();
                },
                [&](const TerminatePlan &terminate) -> llvm::Error {
                  return terminate.status == "success" ||
                                 terminate.status == "failure"
                             ? llvm::Error::success()
                             : processError("terminate status is invalid");
                }},
            block.terminator);
        if (blockError)
          return blockError;
      }
    }

    llvm::Error terminatorError = std::visit(
        Overloaded{
            [&](const ContinuePlan &next) -> llvm::Error {
              return findState(process, next.targetPc)
                         ? llvm::Error::success()
                         : processError(
                               "continue target is outside the PC set");
            },
            [&](const SuspendPlan &suspend) -> llvm::Error {
              auto found = values.find(suspend.wakeValue);
              if (found == values.end() ||
                  !llvm::StringRef(found->second).starts_with("!acsim.wake<") ||
                  !findState(process, suspend.targetPc))
                return processError("suspend wake or target is invalid");
              return llvm::Error::success();
            },
            [&](const TerminatePlan &terminate) -> llvm::Error {
              if (terminate.status != "success" &&
                  terminate.status != "failure")
                return processError("terminate status is invalid");
              return llvm::Error::success();
            }},
        state.terminator);
    if (terminatorError)
      return terminatorError;
  }
  return llvm::Error::success();
}

void emitArguments(std::ostringstream &output,
                   const std::vector<std::string> &arguments) {
  for (auto [index, argument] : llvm::enumerate(arguments)) {
    if (index != 0)
      output << ", ";
    output << argument;
  }
}

void emitResultAssignment(std::ostringstream &output,
                          const std::vector<std::string> &results) {
  if (results.empty())
    return;
  if (results.size() == 1) {
    output << "auto " << results.front() << " = ";
    return;
  }
  output << "auto [";
  emitArguments(output, results);
  output << "] = ";
}

std::string unsignedValue(llvm::StringRef value, llvm::StringRef cppTypeName) {
  return "static_cast<std::make_unsigned_t<" + cppTypeName.str() + ">>(" +
         value.str() + ")";
}

std::string signedValue(llvm::StringRef value, llvm::StringRef cppTypeName) {
  return "static_cast<std::make_signed_t<" + cppTypeName.str() + ">>(" +
         value.str() + ")";
}

llvm::Expected<std::string>
comparisonExpression(llvm::StringRef operationName, llvm::StringRef predicate,
                     const std::vector<std::string> &arguments,
                     llvm::StringRef operandCppType) {
  std::string left = arguments[0];
  std::string right = arguments[1];
  if (predicate.starts_with("u") && operationName != "arith.cmpf") {
    left = unsignedValue(left, operandCppType);
    right = unsignedValue(right, operandCppType);
  } else if (operationName == "index.cmp" && predicate.starts_with("s")) {
    left = signedValue(left, operandCppType);
    right = signedValue(right, operandCppType);
  }

  auto relation = llvm::StringSwitch<llvm::StringRef>(predicate)
                      .Cases({"eq", "oeq", "ueq"}, "==")
                      .Cases({"ne", "one", "une"}, "!=")
                      .Cases({"slt", "ult", "olt"}, "<")
                      .Cases({"sle", "ule", "ole"}, "<=")
                      .Cases({"sgt", "ugt", "ogt"}, ">")
                      .Cases({"sge", "uge", "oge"}, ">=")
                      .Default({});
  const std::string unordered =
      "(std::isnan(" + left + ") || std::isnan(" + right + "))";
  const std::string ordered =
      "(!std::isnan(" + left + ") && !std::isnan(" + right + "))";
  if (operationName == "arith.cmpf") {
    if (predicate == "false")
      return std::string("false");
    if (predicate == "true")
      return std::string("true");
    if (predicate == "ord")
      return ordered;
    if (predicate == "uno")
      return unordered;
    if (relation.empty())
      return processError("floating comparison predicate is unsupported");
    const std::string comparison =
        "(" + left + " " + relation.str() + " " + right + ")";
    return predicate.starts_with("u")
               ? "(" + comparison + " || " + unordered + ")"
               : "(" + comparison + " && " + ordered + ")";
  }
  if (relation.empty())
    return processError("integer comparison predicate is unsupported");
  return "(" + left + " " + relation.str() + " " + right + ")";
}

llvm::Expected<std::string>
scalarExpression(const ModelPlan &plan, llvm::StringRef operationName,
                 const std::vector<std::string> &arguments,
                 const std::vector<std::string> &resultTypes,
                 llvm::StringRef predicate) {
  auto resultCppType = cppType(plan, resultTypes.front());
  if (!resultCppType)
    return resultCppType.takeError();

  if (operationName == "arith.cmpi" || operationName == "arith.cmpf" ||
      operationName == "index.cmp") {
    // Integer comparison results are i1, so use the operand's declared C++
    // expression type instead of the result type for signedness conversions.
    const std::string inferredOperandType =
        operationName == "index.cmp"
            ? "std::size_t"
            : "std::remove_cvref_t<decltype(" + arguments.front() + ")>";
    return comparisonExpression(operationName, predicate, arguments,
                                inferredOperandType);
  }

  if (operationName == "arith.select")
    return "(" + arguments[0] + " ? " + arguments[1] + " : " + arguments[2] +
           ")";
  if (operationName == "arith.negf")
    return "(-" + arguments[0] + ")";
  if (operationName == "arith.index_cast" || operationName == "arith.extui" ||
      operationName == "arith.extsi" || operationName == "arith.trunci" ||
      operationName == "index.casts" || operationName == "index.castu")
    return "static_cast<" + *resultCppType + ">(" + arguments[0] + ")";

  llvm::StringRef binaryOperator =
      llvm::StringSwitch<llvm::StringRef>(operationName)
          .Cases({"arith.addi", "arith.addf", "index.add"}, "+")
          .Cases({"arith.subi", "arith.subf", "index.sub"}, "-")
          .Cases({"arith.muli", "arith.mulf", "index.mul"}, "*")
          .Cases({"arith.divsi", "arith.divf", "index.divs"}, "/")
          .Cases({"arith.remsi", "index.rems"}, "%")
          .Case("arith.andi", "&")
          .Case("arith.ori", "|")
          .Case("arith.xori", "^")
          .Case("arith.shli", "<<")
          .Case("arith.shrsi", ">>")
          .Default({});
  if (!binaryOperator.empty())
    return "(" + arguments[0] + " " + binaryOperator.str() + " " +
           arguments[1] + ")";

  binaryOperator = llvm::StringSwitch<llvm::StringRef>(operationName)
                       .Cases({"arith.divui", "index.divu"}, "/")
                       .Cases({"arith.remui", "index.remu"}, "%")
                       .Case("arith.shrui", ">>")
                       .Default({});
  if (!binaryOperator.empty())
    return "static_cast<" + *resultCppType + ">(" +
           unsignedValue(arguments[0], *resultCppType) + " " +
           binaryOperator.str() + " " +
           unsignedValue(arguments[1], *resultCppType) + ")";
  return processError("scalar operation has no C++ emission");
}

llvm::Error emitOperation(const ModelPlan &plan, const ProcessPlan &process,
                          std::ostringstream &output,
                          const ProcessOperationPlan &operation) {
  return std::visit(
      Overloaded{
          [&](const ConstantPlan &constant) -> llvm::Error {
            auto type = cppType(plan, constant.resultType);
            if (!type)
              return type.takeError();
            auto literal =
                scalarLiteral(constant.canonicalValue, constant.resultType);
            if (!literal)
              return literal.takeError();
            output << "    " << *type << ' ' << constant.resultValue << " = "
                   << *literal << ";\n";
            return llvm::Error::success();
          },
          [&](const ArithmeticPlan &scalar) -> llvm::Error {
            auto expression =
                scalarExpression(plan, scalar.operationName, scalar.arguments,
                                 scalar.resultTypes, scalar.predicate);
            if (!expression)
              return expression.takeError();
            emitResultAssignment(output, scalar.results);
            output << *expression << ";\n";
            return llvm::Error::success();
          },
          [&](const IndexPlan &scalar) -> llvm::Error {
            auto expression =
                scalarExpression(plan, scalar.operationName, scalar.arguments,
                                 scalar.resultTypes, scalar.predicate);
            if (!expression)
              return expression.takeError();
            emitResultAssignment(output, scalar.results);
            output << *expression << ";\n";
            return llvm::Error::success();
          },
          [&](const LiveLoadPlan &load) -> llvm::Error {
            auto type = cppType(plan, load.type);
            if (!type)
              return type.takeError();
            output << "    const " << *type << " &" << load.resultValue
                   << " = committed_" << load.slot << "_;\n";
            return llvm::Error::success();
          },
          [&](const LiveStorePlan &store) -> llvm::Error {
            output << "    proposed_" << store.slot
                   << "_ = " << store.sourceValue << ";\n";
            return llvm::Error::success();
          },
          [&](const InlineCallPlan &call) -> llvm::Error {
            const BindingPlan *binding = findBinding(plan, call.callee);
            emitResultAssignment(output, call.results);
            if (binding)
              output << binding->entryPoints.pure;
            else if (const BindingPlan *implementationBinding =
                         findImplementationBinding(plan, call.callee))
              output << implementationBinding->entryPoints.pure;
            else
              output << findType(plan, call.callee)->cppType;
            output << '(';
            emitArguments(output, call.arguments);
            output << ");\n";
            return llvm::Error::success();
          },
          [&](const InvokePlan &call) -> llvm::Error {
            emitResultAssignment(output, call.results);
            const BindingPlan *binding = findBinding(plan, call.callee);
            if (!binding)
              binding = findImplementationBinding(plan, call.callee);
            if (binding) {
              auto capture =
                  std::find_if(process.captures.begin(), process.captures.end(),
                               [&](const CapturePlan &candidate) {
                                 return llvm::StringRef(candidate.type)
                                     .contains("@" + binding->symbol + ">");
                               });
              if (capture == process.captures.end())
                return processError("stateful invoke has no matching capture");
              output << "arg"
                     << std::distance(process.captures.begin(), capture)
                     << ".invoke(";
            } else {
              output << findType(plan, call.callee)->cppType << '(';
            }
            emitArguments(output, call.arguments);
            output << ");\n";
            return llvm::Error::success();
          }},
      operation);
}

} // namespace

llvm::Expected<GeneratedFile>
generateProcessHeader(const ModelPlan &plan, const ProcessPlan &process) {
  if (auto error = validateProcess(plan, process))
    return std::move(error);
  auto underlyingType = pcType(process);
  if (!underlyingType)
    return underlyingType.takeError();

  std::set<std::string> headers;
  std::map<std::string, const TypePlan *> wakeHelpers;
  auto collectHelper = [&](const ProcessOperationPlan &operation) {
    std::visit(
        Overloaded{
            [&](const InvokePlan &call) {
              const TypePlan *type = findType(plan, call.callee);
              if (type && type->kind == TypeKind::Implementation &&
                  llvm::StringRef(type->symbol).starts_with("acir_impl_wake_"))
                wakeHelpers.emplace(type->symbol, type);
            },
            [&](const auto &) {}},
        operation);
  };
  for (const CapturePlan &capture : process.captures) {
    const size_t start = capture.type.find('@');
    const size_t end = capture.type.find('>', start);
    if (start != std::string::npos && end != std::string::npos)
      if (const BindingPlan *binding = findBinding(
              plan, llvm::StringRef(capture.type).slice(start + 1, end)))
        headers.insert(binding->header);
  }
  for (const PcStatePlan &state : process.states)
    for (const ProcessOperationPlan &operation : state.operations) {
      collectHelper(operation);
      std::visit(
          Overloaded{
              [&](const InlineCallPlan &call) {
                if (const BindingPlan *binding = findBinding(plan, call.callee))
                  headers.insert(binding->header);
                else if (const BindingPlan *binding =
                             findImplementationBinding(plan, call.callee))
                  headers.insert(binding->header);
              },
              [&](const InvokePlan &call) {
                if (const BindingPlan *binding = findBinding(plan, call.callee))
                  headers.insert(binding->header);
                else if (const BindingPlan *binding =
                             findImplementationBinding(plan, call.callee))
                  headers.insert(binding->header);
              },
              [&](const auto &) {}},
          operation);
    }
  for (const PcStatePlan &state : process.states)
    for (const PcBlockPlan &block : state.blocks)
      for (const ProcessOperationPlan &operation : block.operations)
        collectHelper(operation);

  std::ostringstream output;
  output << "#pragma once\n\n#include \"gfsim/process.h\"\n";
  for (const std::string &header : headers)
    output << "#include \"" << header << "\"\n";
  output << "\n#include <cmath>\n#include <cstdint>\n#include <string>\n"
            "#include <type_traits>\n\n"
         << "namespace acir::generated {\n";
  for (const auto &[symbol, implementation] : wakeHelpers)
    if (auto error = emitWakeHelper(output, *implementation))
      return std::move(error);
  output << "} // namespace acir::generated\n\n"
         << "namespace acsim_generated {\n\nclass " << process.className
         << " final : public gfsim::ProcessRuntime<" << process.className
         << "> {\npublic:\n  enum class Pc : " << *underlyingType << " {\n";
  for (auto [index, state] : llvm::enumerate(process.states))
    output << "    " << state.name << " = " << state.ordinal
           << (index + 1 == process.states.size() ? "\n" : ",\n");
  output << "  };\n\n  static constexpr uint64_t kFairnessWork = "
         << process.fairnessWork << ";\n\n  " << process.className
         << "(std::string name, gfsim::ObjectId id, gfsim::SimObject *parent";
  for (const CapturePlan &capture : process.captures) {
    auto type = cppType(plan, capture.type);
    if (!type)
      return type.takeError();
    output << ", " << *type << " &" << capture.name;
  }
  output
      << ");\n\n  gfsim::ProcessStep executeProcessStep(uint32_t pc, "
         "gfsim::Epoch epoch);\n  void doXfer(gfsim::Epoch epoch) override;\n"
         "  void reset() override;\n\nprivate:\n";
  for (const CapturePlan &capture : process.captures) {
    auto type = cppType(plan, capture.type);
    if (!type)
      return type.takeError();
    output << "  " << *type << " *capture_" << capture.name << "_;\n";
  }
  for (const LiveSlotPlan &slot : process.liveSlots) {
    auto type = cppType(plan, slot.type);
    if (!type)
      return type.takeError();
    output << "  " << *type << " committed_" << slot.name << "_{};\n"
           << "  " << *type << " proposed_" << slot.name << "_{};\n";
  }
  output << "};\n\n} // namespace acsim_generated\n";
  return makeFile("include/generated/processes/" + process.className + ".h",
                  output.str());
}

llvm::Expected<GeneratedFile>
generateProcessSource(const ModelPlan &plan, const ProcessPlan &process) {
  if (auto error = validateProcess(plan, process))
    return std::move(error);
  const PcStatePlan *entry = findState(process, process.entryPc);

  std::ostringstream output;
  output << "#include \"generated/processes/" << process.className
         << ".h\"\n\n#include <optional>\n\nnamespace acsim_generated {\n\n"
         << process.className << "::" << process.className
         << "(std::string name, gfsim::ObjectId id, gfsim::SimObject *parent";
  for (const CapturePlan &capture : process.captures) {
    auto type = cppType(plan, capture.type);
    if (!type)
      return type.takeError();
    output << ", " << *type << " &" << capture.name;
  }
  output << ")\n    : ProcessRuntime(std::move(name), id, parent, "
         << "static_cast<uint32_t>(Pc::" << entry->name << "), kFairnessWork)";
  for (const CapturePlan &capture : process.captures)
    output << ",\n      capture_" << capture.name << "_(&" << capture.name
           << ')';
  output << " {}\n\ngfsim::ProcessStep " << process.className
         << "::executeProcessStep(uint32_t pc, gfsim::Epoch epoch) {\n"
            "  (void)epoch;\n  switch (static_cast<Pc>(pc)) {\n";
  for (const PcStatePlan &state : process.states) {
    output << "  case Pc::" << state.name << ": {\n";
    for (auto [index, capture] : llvm::enumerate(process.captures))
      output << "    auto &arg" << index << " = *capture_" << capture.name
             << "_;\n";
    if (state.blocks.size() > 1) {
      output << "    enum class Block_" << state.name << " {";
      for (auto [index, block] : llvm::enumerate(state.blocks))
        output << (index == 0 ? " " : ", ") << "b" << block.ordinal;
      output << " };\n";
      for (const PcBlockPlan &block : state.blocks) {
        for (auto [index, argument] : llvm::enumerate(block.arguments)) {
          auto type = cppType(plan, argument.type);
          if (!type)
            return type.takeError();
          output << "    std::optional<" << *type << "> b" << block.ordinal
                 << "_arg" << index << ";\n";
        }
      }
      output << "    auto block_" << state.name << " = Block_" << state.name
             << "::b0;\n    for (;;) {\n      switch (block_" << state.name
             << ") {\n";
      for (const PcBlockPlan &block : state.blocks) {
        output << "      case Block_" << state.name << "::b" << block.ordinal
               << ": {\n";
        if (block.ordinal != 0)
          for (auto [index, argument] : llvm::enumerate(block.arguments))
            output << "        auto " << argument.name << " = *b"
                   << block.ordinal << "_arg" << index << ";\n";
        for (const ProcessOperationPlan &operation : block.operations)
          if (auto error = emitOperation(plan, process, output, operation))
            return std::move(error);
        llvm::Error terminatorError = std::visit(
            Overloaded{
                [&](const BranchPlan &branch) -> llvm::Error {
                  const PcBlockPlan &target =
                      state.blocks.at(branch.targetBlock);
                  for (auto [index, argument] :
                       llvm::enumerate(branch.arguments))
                    output << "        b" << target.ordinal << "_arg" << index
                           << " = " << argument << ";\n";
                  output << "        block_" << state.name << " = Block_"
                         << state.name << "::b" << target.ordinal
                         << ";\n        continue;\n";
                  return llvm::Error::success();
                },
                [&](const ConditionalBranchPlan &branch) -> llvm::Error {
                  output << "        if (" << branch.condition << ") {\n";
                  const PcBlockPlan &trueTarget =
                      state.blocks.at(branch.trueBlock);
                  for (auto [index, argument] :
                       llvm::enumerate(branch.trueArguments))
                    output << "          b" << trueTarget.ordinal << "_arg"
                           << index << " = " << argument << ";\n";
                  output << "          block_" << state.name << " = Block_"
                         << state.name << "::b" << trueTarget.ordinal
                         << ";\n        } else {\n";
                  const PcBlockPlan &falseTarget =
                      state.blocks.at(branch.falseBlock);
                  for (auto [index, argument] :
                       llvm::enumerate(branch.falseArguments))
                    output << "          b" << falseTarget.ordinal << "_arg"
                           << index << " = " << argument << ";\n";
                  output << "          block_" << state.name << " = Block_"
                         << state.name << "::b" << falseTarget.ordinal
                         << ";\n        }\n        continue;\n";
                  return llvm::Error::success();
                },
                [&](const ContinuePlan &next) -> llvm::Error {
                  output << "        return gfsim::ProcessStep::continueAt("
                         << "static_cast<uint32_t>(Pc::" << next.targetPc
                         << "));\n";
                  return llvm::Error::success();
                },
                [&](const SuspendPlan &suspend) -> llvm::Error {
                  const PcStatePlan *target =
                      findState(process, suspend.targetPc);
                  output << "        return gfsim::ProcessStep::suspendAt("
                         << "static_cast<uint32_t>(Pc::" << suspend.targetPc
                         << "), " << suspend.wakeValue << ", "
                         << static_cast<uint64_t>(target->ordinal) + 1
                         << ");\n";
                  return llvm::Error::success();
                },
                [&](const TerminatePlan &terminate) -> llvm::Error {
                  if (terminate.status == "success")
                    output
                        << "        return gfsim::ProcessStep::terminate();\n";
                  else
                    output << "        return gfsim::ProcessStep::fail("
                              "\"process_terminated_failure\");\n";
                  return llvm::Error::success();
                }},
            block.terminator);
        if (terminatorError)
          return std::move(terminatorError);
        output << "      }\n";
      }
      output << "      }\n    }\n";
    } else {
      for (const ProcessOperationPlan &operation : state.operations)
        if (auto error = emitOperation(plan, process, output, operation))
          return std::move(error);
    }
    if (state.blocks.size() > 1) {
      output << "  }\n";
      continue;
    }
    std::visit(
        Overloaded{
            [&](const ContinuePlan &next) {
              output << "    return gfsim::ProcessStep::continueAt("
                     << "static_cast<uint32_t>(Pc::" << next.targetPc
                     << "));\n";
            },
            [&](const SuspendPlan &suspend) {
              const PcStatePlan *target = findState(process, suspend.targetPc);
              output << "    return gfsim::ProcessStep::suspendAt("
                     << "static_cast<uint32_t>(Pc::" << suspend.targetPc
                     << "), " << suspend.wakeValue << ", "
                     << static_cast<uint64_t>(target->ordinal) + 1 << ");\n";
            },
            [&](const TerminatePlan &terminate) {
              if (terminate.status == "success")
                output << "    return gfsim::ProcessStep::terminate();\n";
              else
                output << "    return gfsim::ProcessStep::fail("
                          "\"process_terminated_failure\");\n";
            }},
        state.terminator);
    output << "  }\n";
  }
  output
      << "  default:\n    return gfsim::ProcessStep::fail("
         "\"invalid_process_pc\");\n  }\n}\n\nvoid "
      << process.className
      << "::doXfer(gfsim::Epoch epoch) {\n  ProcessRuntime::doXfer(epoch);\n";
  for (const LiveSlotPlan &slot : process.liveSlots)
    output << "  committed_" << slot.name << "_ = proposed_" << slot.name
           << "_;\n";
  output << "}\n\nvoid " << process.className
         << "::reset() {\n  ProcessRuntime::reset();\n";
  for (const LiveSlotPlan &slot : process.liveSlots)
    output << "  committed_" << slot.name << "_ = {};\n  proposed_" << slot.name
           << "_ = {};\n";
  output << "}\n\n} // namespace acsim_generated\n";
  return makeFile("src/generated/processes/" + process.className + ".cpp",
                  output.str());
}

} // namespace acir::codegen::detail

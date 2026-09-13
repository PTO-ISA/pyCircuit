#include "acir/CodeGen/QueueGraphPyc.h"
#include "acir/CodeGen/QueueBlockContract.h"
#include "acir/Support/PrimitiveWidths.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <sstream>
#include <system_error>

namespace acir::codegen {
namespace {

llvm::Error pycError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-PYC: " + message);
}

const QueuePayloadPlan *findPayload(const QueueGraphPlan &plan,
                                    llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.struct<@types::@";
  if (!type.starts_with(prefix) || !type.ends_with('>'))
    return nullptr;
  llvm::StringRef name = type.drop_front(prefix.size()).drop_back();
  auto found =
      std::find_if(plan.payloads.begin(), plan.payloads.end(),
                   [&](const auto &payload) { return payload.name == name; });
  return found == plan.payloads.end() ? nullptr : &*found;
}

const QueueEnumPlan *findEnum(const QueueGraphPlan &plan,
                              llvm::StringRef type) {
  constexpr llvm::StringLiteral prefix = "!ac.enum<@types::@";
  if (!type.starts_with(prefix) || !type.ends_with('>'))
    return nullptr;
  llvm::StringRef name = type.drop_front(prefix.size()).drop_back();
  auto found = std::find_if(
      plan.enums.begin(), plan.enums.end(),
      [&](const auto &enumeration) { return enumeration.name == name; });
  return found == plan.enums.end() ? nullptr : &*found;
}

const QueueAggregatePlan *findAggregate(const QueueGraphPlan &plan,
                                        llvm::StringRef type) {
  auto found = std::find_if(
      plan.aggregates.begin(), plan.aggregates.end(),
      [&](const auto &aggregate) { return aggregate.type == type; });
  return found == plan.aggregates.end() ? nullptr : &*found;
}

const QueueHelperPlan *findHelper(const QueueGraphPlan &plan,
                                  llvm::StringRef name) {
  auto found = std::find_if(
      plan.helpers.begin(), plan.helpers.end(),
      [&](const QueueHelperPlan &helper) { return helper.name == name; });
  return found == plan.helpers.end() ? nullptr : &*found;
}

llvm::Expected<unsigned> typeWidth(const QueueGraphPlan &plan,
                                   llvm::StringRef type) {
  if (type.starts_with('i')) {
    unsigned width = 0;
    if (!type.drop_front().getAsInteger(10, width) && width > 0)
      return width;
  }
  if (const QueueEnumPlan *enumeration = findEnum(plan, type))
    if (enumeration->width <= kMaximumPackedValueWidth)
      return static_cast<unsigned>(enumeration->width);
  if (const QueueAggregatePlan *aggregate = findAggregate(plan, type)) {
    if (aggregate->width > kMaximumPackedValueWidth)
      return pycError("aggregate width exceeds the backend template domain");
    return static_cast<unsigned>(aggregate->width);
  }
  const QueuePayloadPlan *payload = findPayload(plan, type);
  if (!payload)
    return pycError("unsupported PYC payload type '" + type + "'");
  unsigned total = 0;
  for (const QueuePayloadFieldPlan &field : payload->fields) {
    if (field.width == 0 || field.width > kMaximumPackedValueWidth - total)
      return pycError(
          "packed payload width exceeds the backend template domain");
    total += static_cast<unsigned>(field.width);
  }
  if (total == 0)
    return pycError("packed payload width must be positive");
  return total;
}

llvm::Expected<std::string> pycType(const QueueGraphPlan &plan,
                                    llvm::StringRef type) {
  auto width = typeWidth(plan, type);
  if (!width)
    return width.takeError();
  return "i" + std::to_string(*width);
}

bool pycIntegerCanRepresent(uint64_t value, llvm::StringRef type) {
  uint64_t width = 0;
  return type.consume_front("i") && !type.getAsInteger(10, width) &&
         (width >= 64 || value < (uint64_t{1} << width));
}

llvm::Error verifyTablePycProfile(const QueueGraphPlan &plan) {
  constexpr uint64_t maximumRank = 4;
  constexpr uint64_t maximumEntries = 256;
  constexpr uint64_t maximumEntryWidth = 256;
  constexpr uint64_t maximumStateBits = 65'536;
  constexpr uint64_t maximumWriters = 4;

  llvm::StringMap<uint64_t> writerCounts;
  for (const TableWritePlan &write : plan.tableWrites)
    ++writerCounts[write.table];
  for (const TableMaskedWritePlan &write : plan.tableMaskedWrites)
    ++writerCounts[write.table];
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind != "firing")
      continue;
    llvm::StringSet<> writtenOwners;
    for (const StateWritePlan &write : block.stateWrites)
      if (writtenOwners.insert(write.table).second)
        ++writerCounts[write.table];
  }

  for (const TablePlan &table : plan.tables) {
    const uint64_t rank = table.shape.empty() ? 1 : table.shape.size();
    if (rank > maximumRank)
      return pycError("bounded Table PYC rank exceeds 4 for '" + table.name +
                      "'");
    auto width = typeWidth(plan, table.entryType);
    if (!width)
      return width.takeError();
    if (*width > maximumEntryWidth)
      return pycError("bounded Table PYC Entry width exceeds 256 bits for '" +
                      table.name + "'");
    if (table.entries != 0 &&
        *width > std::numeric_limits<uint64_t>::max() / table.entries)
      return pycError("bounded Table PYC total state width overflows for '" +
                      table.name + "'");
    if (table.entries * *width > maximumStateBits)
      return pycError("bounded Table PYC total state exceeds 65536 bits for '" +
                      table.name + "'");
    if (table.entries > maximumEntries)
      return pycError("bounded Table PYC entry count exceeds 256 for '" +
                      table.name + "'");
    if (writerCounts[table.name] > maximumWriters)
      return pycError("bounded Table PYC writer count exceeds 4 for '" +
                      table.name + "'");
  }
  return llvm::Error::success();
}

llvm::Expected<llvm::APInt>
packTableInitValue(const QueueGraphPlan &plan,
                   const TableInitValuePlan &value) {
  auto width = typeWidth(plan, value.type);
  if (!width)
    return width.takeError();
  if (value.kind == "integer")
    return llvm::APInt(*width, value.value, 10);
  if (value.kind == "enum") {
    const QueueEnumPlan *enumeration = findEnum(plan, value.type);
    if (!enumeration)
      return pycError("typed Table enum initializer has unknown type");
    auto found = llvm::find(enumeration->enumerants, value.value);
    if (found == enumeration->enumerants.end())
      return pycError("typed Table enum initializer has unknown enumerant");
    const size_t ordinal =
        std::distance(enumeration->enumerants.begin(), found);
    return llvm::APInt(*width, enumeration->values.empty()
                                   ? ordinal
                                   : enumeration->values[ordinal]);
  }
  if (value.kind != "struct" && value.kind != "tuple" && value.kind != "array")
    return pycError("typed Table initializer kind is unsupported");
  llvm::APInt packed(*width, 0);
  uint64_t used = 0;
  for (const TableInitValuePlan &element : value.elements) {
    auto member = packTableInitValue(plan, element);
    if (!member)
      return member.takeError();
    if (used > *width || member->getBitWidth() > *width - used)
      return pycError("typed Table initializer width is inconsistent");
    packed = packed.shl(member->getBitWidth());
    packed |= member->zext(*width);
    used += member->getBitWidth();
  }
  if (used != *width)
    return pycError("typed Table initializer width is incomplete");
  return packed;
}

std::string unsignedDecimal(const llvm::APInt &value) {
  llvm::SmallString<96> storage;
  value.toString(storage, 10, /*Signed=*/false);
  return storage.str().str();
}

struct FieldLayout {
  unsigned lsb = 0;
  unsigned width = 0;
  std::string type;
};

struct RoundRobinPycState {
  std::string nextWire;
  std::string enableWire;
  std::string cursor;
  std::string type;
  std::string nextCandidate;
  std::string anyCandidate;
  std::string owner;
};

llvm::Expected<FieldLayout> fieldLayout(const QueueGraphPlan &plan,
                                        llvm::StringRef recordType,
                                        llvm::StringRef fieldName) {
  const QueuePayloadPlan *payload = findPayload(plan, recordType);
  if (!payload)
    return pycError("field access requires a packed struct payload");
  auto total = typeWidth(plan, recordType);
  if (!total)
    return total.takeError();
  unsigned cursor = *total;
  for (const QueuePayloadFieldPlan &field : payload->fields) {
    auto width = typeWidth(plan, field.type);
    if (!width)
      return width.takeError();
    cursor -= *width;
    if (field.name == fieldName)
      return FieldLayout{cursor, *width, field.type};
  }
  return pycError("unknown packed struct field '" + fieldName + "'");
}

const QueuePlan *findQueue(const QueueGraphPlan &plan, llvm::StringRef name) {
  auto found =
      std::find_if(plan.queues.begin(), plan.queues.end(),
                   [&](const QueuePlan &queue) { return queue.name == name; });
  return found == plan.queues.end() ? nullptr : &*found;
}

llvm::Expected<std::string> yieldedType(const QueueBlockPlan &block,
                                        llvm::StringRef yield,
                                        llvm::StringRef inputType) {
  if (yield == "item")
    return inputType.str();
  auto found = std::find_if(block.expressions.begin(), block.expressions.end(),
                            [&](const QueueExpressionPlan &expression) {
                              return expression.result == yield;
                            });
  if (found == block.expressions.end())
    return pycError("yield references unknown expression value");
  return found->type;
}

QueueBlockPlan expressionSlice(const QueueBlockPlan &block,
                               llvm::StringRef yield) {
  QueueBlockPlan sliced;
  sliced.yields.push_back(yield.str());
  llvm::StringSet<> needed;
  needed.insert(yield);
  for (const QueueExpressionPlan &expression :
       llvm::reverse(block.expressions)) {
    if (!needed.contains(expression.result))
      continue;
    sliced.expressions.push_back(expression);
    for (const std::string &operand : expression.operands)
      needed.insert(operand);
  }
  std::reverse(sliced.expressions.begin(), sliced.expressions.end());
  return sliced;
}

llvm::Expected<std::string> emitTransform(
    const QueueGraphPlan &plan, const QueueBlockPlan &block,
    llvm::ArrayRef<std::string> inputData,
    llvm::ArrayRef<std::string> inputTypes, size_t yieldIndex,
    unsigned &nextValue, std::ostringstream &body,
    llvm::StringMap<std::string> *emittedValues = nullptr,
    const llvm::StringMap<std::vector<std::string>> *tableValues = nullptr,
    llvm::StringMap<RoundRobinPycState> *roundRobinStates = nullptr,
    llvm::StringRef choiceOwner = {},
    const llvm::StringMap<std::string> *inheritedValues = nullptr,
    const llvm::StringMap<std::string> *inheritedTypes = nullptr,
    llvm::StringMap<std::string> *sharedTableValues = nullptr);

constexpr llvm::StringLiteral kStructMetrics =
    "{\\\"ast_node_count\\\":0,\\\"collection_count\\\":0,"
    "\\\"collection_instance_count\\\":0,"
    "\\\"estimated_inline_cost\\\":0,\\\"hardware_call_count\\\":0,"
    "\\\"instance_count\\\":0,\\\"loop_count\\\":0,"
    "\\\"module_call_count\\\":0,"
    "\\\"module_family_collection_count\\\":0,"
    "\\\"repeat_pressure\\\":0,\\\"repeated_body_clusters\\\":[],"
    "\\\"source_loc\\\":0,\\\"state_alloc_count\\\":0,"
    "\\\"state_call_count\\\":0}";

llvm::Expected<std::string>
generateLaneQueuePyc(const QueueGraphPlan &plan,
                     llvm::ArrayRef<const QueueBlockPlan *> sources,
                     llvm::ArrayRef<const QueueBlockPlan *> sinks) {
  if (sources.size() != 1 || sinks.size() != 1 || plan.queues.empty() ||
      sources.front()->outputs.size() != 1 || sinks.front()->inputs.size() != 1)
    return pycError("multi-lane PYC requires one source and one sink boundary");
  llvm::StringMap<const QueueBlockPlan *> transformsByOutput;
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "source" || block.kind == "sink")
      continue;
    if (block.kind != "transform" || block.inputs.size() != 1 ||
        block.outputs.size() != 1 || block.yields.size() != 1)
      return pycError(
          "multi-lane PYC supports only a chain of 1x1 pure transforms");
    transformsByOutput[block.outputs.front()] = &block;
  }
  const QueuePlan *boundaryQueue =
      findQueue(plan, sources.front()->outputs.front());
  const QueuePlan *sinkQueue = findQueue(plan, sinks.front()->inputs.front());
  if (!boundaryQueue || !sinkQueue)
    return pycError("multi-lane Queue boundary identity is missing");
  for (const QueuePlan &queue : plan.queues) {
    if (queue.lanes != boundaryQueue->lanes ||
        queue.rate != boundaryQueue->rate ||
        queue.payloadType != boundaryQueue->payloadType || queue.lanes <= 1 ||
        queue.rate == 0 || queue.rate > queue.lanes ||
        queue.depth < queue.rate || queue.latency != 1)
      return pycError(
          "multi-lane transform chain requires uniform payload/lanes/rate, "
          "latency=1, and depth>=rate");
    if (queue.laneOrdinals.size() != queue.lanes)
      return pycError("multi-lane Queue requires explicit lane ordinals");
    for (auto [ordinal, lane] : llvm::enumerate(queue.laneOrdinals))
      if (lane != ordinal)
        return pycError(
            "multi-lane Queue ordinals must be contiguous from zero");
  }
  auto payloadWidth = typeWidth(plan, boundaryQueue->payloadType);
  if (!payloadWidth)
    return payloadWidth.takeError();
  const QueuePlan &queue = *boundaryQueue;
  unsigned nextValue = 0;
  auto newValue = [&]() { return "%v" + std::to_string(nextValue++); };
  std::ostringstream body;
  auto emitConstant = [&](uint64_t value, llvm::StringRef type) {
    std::string result = newValue();
    body << "    " << result << " = pyc.constant " << value << " : "
         << type.str() << "\n";
    return result;
  };
  auto emitBinary = [&](llvm::StringRef operation, llvm::StringRef lhs,
                        llvm::StringRef rhs, llvm::StringRef type) {
    std::string result = newValue();
    body << "    " << result << " = pyc." << operation.str() << ' ' << lhs.str()
         << ", " << rhs.str() << " : " << type.str() << ", " << type.str()
         << " -> " << type.str() << "\n";
    return result;
  };
  auto emitNot = [&](llvm::StringRef value) {
    std::string result = newValue();
    body << "    " << result << " = pyc.not " << value.str() << " : i1\n";
    return result;
  };
  auto emitSelect = [&](llvm::StringRef condition, llvm::StringRef trueValue,
                        llvm::StringRef falseValue, llvm::StringRef type) {
    std::string result = newValue();
    body << "    " << result << " = pyc.select " << condition.str() << ", "
         << trueValue.str() << ", " << falseValue.str() << " : i1, "
         << type.str() << ", " << type.str() << " -> " << type.str() << "\n";
    return result;
  };

  const std::string payloadType = "i" + std::to_string(*payloadWidth);
  std::vector<std::string> inputValids;
  std::vector<std::string> inputData;
  for (uint64_t lane = 0; lane < queue.lanes; ++lane) {
    inputValids.push_back("%in_valid_" + std::to_string(lane));
    inputData.push_back("%in_data_" + std::to_string(lane));
  }
  for (uint64_t lane = 1; lane < queue.lanes; ++lane) {
    std::string prefixOk = emitBinary("or", emitNot(inputValids[lane]),
                                      inputValids[lane - 1], "i1");
    body << "    pyc.assert " << prefixOk
         << " {msg = \"queue_valid_prefix\"}\n";
  }
  for (uint64_t lane = queue.rate; lane < queue.lanes; ++lane) {
    std::string rateOk = emitNot(inputValids[lane]);
    body << "    pyc.assert " << rateOk << " {msg = \"queue_rate_exceeded\"}\n";
  }
  std::string zeroI1 = emitConstant(0, "i1");
  std::string zeroData = emitConstant(0, payloadType);
  struct LaneQueueState {
    std::vector<std::string> valid;
    std::vector<std::string> data;
    std::string dequeue;
    std::string producerReady;
  };
  llvm::StringMap<LaneQueueState> queueStates;
  std::string sourceReady;
  for (const QueuePlan &currentQueue : plan.queues) {
    std::vector<std::string> producerValids;
    std::vector<std::string> producerData;
    const QueueBlockPlan *transform = nullptr;
    if (currentQueue.name == sources.front()->outputs.front()) {
      producerValids = inputValids;
      producerData = inputData;
    } else {
      auto found = transformsByOutput.find(currentQueue.name);
      if (found == transformsByOutput.end())
        return pycError(
            "multi-lane transform chain is not topologically closed");
      transform = found->getValue();
      auto input = queueStates.find(transform->inputs.front());
      if (input == queueStates.end())
        return pycError(
            "multi-lane transform input is not in topological order");
      for (uint64_t lane = 0; lane < currentQueue.lanes; ++lane) {
        producerValids.push_back(
            lane < currentQueue.rate ? input->getValue().valid[lane] : zeroI1);
        auto transformed = emitTransform(
            plan, *transform, {input->getValue().data[lane]},
            {currentQueue.payloadType}, 0, nextValue, body, nullptr);
        if (!transformed)
          return transformed.takeError();
        producerData.push_back(std::move(*transformed));
      }
    }

    std::vector<std::string> validNext, validEnable, validState;
    std::vector<std::string> dataNext, dataEnable, dataState;
    for (uint64_t slot = 0; slot < currentQueue.depth; ++slot) {
      validNext.push_back(newValue());
      validEnable.push_back(newValue());
      validState.push_back(newValue());
      body << "    " << validNext.back() << " = pyc.wire : i1\n";
      body << "    " << validEnable.back() << " = pyc.wire : i1\n";
      body << "    " << validState.back() << " = pyc.reg %clk, %rst, "
           << validEnable.back() << ", " << validNext.back() << ", " << zeroI1
           << " : i1\n";
      dataNext.push_back(newValue());
      dataEnable.push_back(newValue());
      dataState.push_back(newValue());
      body << "    " << dataNext.back() << " = pyc.wire : " << payloadType
           << "\n";
      body << "    " << dataEnable.back() << " = pyc.wire : i1\n";
      body << "    " << dataState.back() << " = pyc.reg %clk, %rst, "
           << dataEnable.back() << ", " << dataNext.back() << ", " << zeroData
           << " : " << payloadType << "\n";
    }
    std::string dequeue = newValue();
    body << "    " << dequeue << " = pyc.wire : i1\n";
    std::vector<std::string> nextValid, nextData;
    for (uint64_t slot = 0; slot < currentQueue.depth; ++slot) {
      const std::string shiftedValid =
          slot + currentQueue.rate < currentQueue.depth
              ? validState[slot + currentQueue.rate]
              : zeroI1;
      const std::string shiftedData =
          slot + currentQueue.rate < currentQueue.depth
              ? dataState[slot + currentQueue.rate]
              : zeroData;
      nextValid.push_back(
          emitSelect(dequeue, shiftedValid, validState[slot], "i1"));
      nextData.push_back(
          emitSelect(dequeue, shiftedData, dataState[slot], payloadType));
    }
    std::string ready = emitConstant(1, "i1");
    for (uint64_t lane = 0; lane < currentQueue.rate; ++lane) {
      std::string capacity = emitNot(nextValid[currentQueue.depth - 1 - lane]);
      std::string laneFits =
          emitBinary("or", emitNot(producerValids[lane]), capacity, "i1");
      ready = emitBinary("and", ready, laneFits, "i1");
    }
    std::string accepted =
        emitBinary("and", producerValids.front(), ready, "i1");
    for (uint64_t lane = 0; lane < currentQueue.rate; ++lane) {
      const std::vector<std::string> beforeValid = nextValid;
      const std::vector<std::string> beforeData = nextData;
      for (uint64_t slot = 0; slot < currentQueue.depth; ++slot) {
        std::string firstFree = emitNot(beforeValid[slot]);
        if (slot != 0)
          firstFree = emitBinary("and", firstFree, beforeValid[slot - 1], "i1");
        std::string insert =
            emitBinary("and", ready, producerValids[lane], "i1");
        insert = emitBinary("and", insert, firstFree, "i1");
        nextData[slot] = emitSelect(insert, producerData[lane],
                                    beforeData[slot], payloadType);
        nextValid[slot] = emitBinary("or", beforeValid[slot], insert, "i1");
      }
    }
    std::string stateEnable = emitBinary("or", dequeue, accepted, "i1");
    for (uint64_t slot = 0; slot < currentQueue.depth; ++slot) {
      body << "    pyc.assign " << validNext[slot] << ", " << nextValid[slot]
           << " : i1\n";
      body << "    pyc.assign " << validEnable[slot] << ", " << stateEnable
           << " : i1\n";
      body << "    pyc.assign " << dataNext[slot] << ", " << nextData[slot]
           << " : " << payloadType << "\n";
      body << "    pyc.assign " << dataEnable[slot] << ", " << stateEnable
           << " : i1\n";
    }
    LaneQueueState state{std::move(validState), std::move(dataState), dequeue,
                         ready};
    auto inserted =
        queueStates.try_emplace(currentQueue.name, std::move(state)).first;
    if (transform) {
      LaneQueueState &input = queueStates[transform->inputs.front()];
      std::string consume = emitBinary("and", input.valid.front(), ready, "i1");
      body << "    pyc.assign " << input.dequeue << ", " << consume
           << " : i1\n";
    } else {
      sourceReady = ready;
    }
    (void)inserted;
  }
  LaneQueueState &outputState = queueStates[sinkQueue->name];
  std::string outputDequeue =
      emitBinary("and", outputState.valid.front(), "%out_ready", "i1");
  body << "    pyc.assign " << outputState.dequeue << ", " << outputDequeue
       << " : i1\n";

  std::vector<std::string> returnValues;
  std::vector<std::string> resultTypes;
  std::vector<std::string> resultNames;
  for (uint64_t lane = 0; lane < queue.lanes; ++lane) {
    std::string valid = lane < queue.rate ? outputState.valid[lane] : zeroI1;
    std::string data = lane < queue.rate ? outputState.data[lane] : zeroData;
    returnValues.push_back(valid);
    returnValues.push_back(data);
    resultTypes.push_back("i1");
    resultTypes.push_back(payloadType);
    resultNames.push_back("out_valid_" + std::to_string(lane));
    resultNames.push_back("out_data_" + std::to_string(lane));
  }
  returnValues.push_back(sourceReady);
  resultTypes.push_back("i1");
  resultNames.push_back("in_ready");

  std::vector<std::string> arguments = {"%clk: !pyc.clock", "%rst: !pyc.reset"};
  std::vector<std::string> argumentNames = {"clk", "rst"};
  for (uint64_t lane = 0; lane < queue.lanes; ++lane) {
    arguments.push_back(inputValids[lane] + ": i1");
    arguments.push_back(inputData[lane] + ": " + payloadType);
    argumentNames.push_back("in_valid_" + std::to_string(lane));
    argumentNames.push_back("in_data_" + std::to_string(lane));
  }
  arguments.push_back("%out_ready: i1");
  argumentNames.push_back("out_ready");
  auto writeList =
      [](std::ostringstream &stream, llvm::ArrayRef<std::string> values,
         llvm::StringRef prefix = {}, llvm::StringRef suffix = {}) {
        for (auto [index, value] : llvm::enumerate(values)) {
          if (index)
            stream << ", ";
          stream << prefix.str() << value << suffix.str();
        }
      };
  std::ostringstream output;
  output << "module attributes {pyc.top = @" << plan.system
         << ", pyc.frontend.contract = \"pycircuit\"} {\n  func.func @"
         << plan.system << '(';
  writeList(output, arguments);
  output << ") -> (";
  writeList(output, resultTypes);
  output << ") attributes {arg_names = [";
  writeList(output, argumentNames, "\"", "\"");
  output << "], result_names = [";
  writeList(output, resultNames, "\"", "\"");
  output << "], pyc.value_params = [], pyc.value_param_types = [], "
            "pyc.kind = \"module\", pyc.inline = \"false\", "
            "pyc.params = \"{}\", pyc.base = \""
         << plan.system << "\", pyc.struct.metrics = \"" << kStructMetrics.str()
         << "\", pyc.struct.collections = \"[]\"} {\n"
         << body.str() << "    func.return ";
  writeList(output, returnValues);
  output << " : ";
  writeList(output, resultTypes);
  output << "\n  }\n}\n";
  return output.str();
}

llvm::Expected<std::string>
emitTransform(const QueueGraphPlan &plan, const QueueBlockPlan &block,
              llvm::ArrayRef<std::string> inputData,
              llvm::ArrayRef<std::string> inputTypes, size_t yieldIndex,
              unsigned &nextValue, std::ostringstream &body,
              llvm::StringMap<std::string> *emittedValues,
              const llvm::StringMap<std::vector<std::string>> *tableValues,
              llvm::StringMap<RoundRobinPycState> *roundRobinStates,
              llvm::StringRef choiceOwner,
              const llvm::StringMap<std::string> *inheritedValues,
              const llvm::StringMap<std::string> *inheritedTypes,
              llvm::StringMap<std::string> *sharedTableValues) {
  if (inputData.size() != inputTypes.size())
    return pycError("transform input data/type arity mismatch");
  llvm::StringMap<std::string> values;
  llvm::StringMap<std::string> types;
  if (inheritedValues)
    for (const auto &entry : *inheritedValues)
      values[entry.getKey()] = entry.getValue();
  if (inheritedTypes)
    for (const auto &entry : *inheritedTypes)
      types[entry.getKey()] = entry.getValue();
  llvm::StringMap<std::pair<std::string, std::string>> priorityValues;
  struct ChoiceValues {
    std::vector<std::string> indices;
    std::vector<std::string> valids;
  };
  llvm::StringMap<ChoiceValues> choiceValues;
  llvm::StringMap<std::vector<std::string>> helperCallValues;
  for (size_t index = 0; index < inputData.size(); ++index) {
    std::string name = index == 0 ? "item" : "item" + std::to_string(index);
    values[name] = inputData[index];
    types[name] = inputTypes[index];
  }
  if (inputData.size() == 1) {
    values["entry"] = inputData.front();
    types["entry"] = inputTypes.front();
  }
  auto newValue = [&]() { return "%v" + std::to_string(nextValue++); };
  auto value = [&](llvm::StringRef name) -> llvm::Expected<std::string> {
    auto found = values.find(name);
    if (found == values.end())
      return pycError("transform expression references unknown value '" + name +
                      "'");
    return found->getValue();
  };
  auto valueType = [&](llvm::StringRef name) -> llvm::Expected<std::string> {
    auto found = types.find(name);
    if (found == types.end())
      return pycError("transform value has no type: '" + name + "'");
    return found->getValue();
  };
  for (const QueueExpressionPlan &expression : block.expressions) {
    if (values.contains(expression.result))
      continue;
    std::string result;
    if (expression.kind == "helper_call") {
      const QueueHelperPlan *helper = findHelper(plan, expression.field);
      if (!helper || expression.literal.empty() ||
          expression.selectionCount != helper->resultTypes.size() ||
          expression.laneOrdinal >= helper->yields.size())
        return pycError("helper_call expression is malformed");
      auto cached = helperCallValues.find(expression.literal);
      if (cached == helperCallValues.end()) {
        llvm::StringMap<std::string> arguments;
        llvm::StringMap<std::string> argumentTypes;
        for (auto [name, type, operandName] : llvm::zip_equal(
                 helper->inputNames, helper->inputTypes, expression.operands)) {
          auto operandValue = value(operandName);
          if (!operandValue)
            return operandValue.takeError();
          arguments[name] = *operandValue;
          argumentTypes[name] = type;
        }
        QueueBlockPlan helperBody;
        helperBody.expressions = helper->expressions;
        helperBody.yields = helper->yields;
        llvm::StringMap<std::string> emitted;
        auto lowered = emitTransform(
            plan, helperBody, {}, {}, helper->yields.size(), nextValue, body,
            &emitted, tableValues, roundRobinStates, choiceOwner, &arguments,
            &argumentTypes, sharedTableValues);
        if (!lowered)
          return lowered.takeError();
        std::vector<std::string> results;
        for (const std::string &yield : helper->yields) {
          auto found = emitted.find(yield);
          if (found == emitted.end())
            return pycError("helper result was not legalized exactly once");
          results.push_back(found->getValue());
        }
        helperCallValues[expression.literal] = std::move(results);
        cached = helperCallValues.find(expression.literal);
      }
      result = cached->getValue()[expression.laneOrdinal];
    } else if (expression.kind == "enum_constant") {
      result = newValue();
      auto type = pycType(plan, expression.type);
      if (!type)
        return type.takeError();
      body << "    " << result << " = pyc.constant " << expression.literal
           << " : " << *type << "\n";
    } else if (expression.kind == "constant") {
      result = newValue();
      llvm::StringRef literal = expression.literal;
      auto type = pycType(plan, expression.type);
      if (!type)
        return type.takeError();
      llvm::StringRef spelling = literal.split(" : ").first;
      if (spelling == "true")
        spelling = "1";
      else if (spelling == "false")
        spelling = "0";
      body << "    " << result << " = pyc.constant " << spelling.str() << " : "
           << *type << "\n";
    } else {
      if (expression.operands.empty() && expression.kind != "table_match" &&
          expression.kind != "table_match_ref" &&
          expression.kind != "table_selection_index_ref" &&
          expression.kind != "table_selection_valid_ref")
        return pycError("transform expression operand is missing");
      auto first = expression.operands.empty()
                       ? llvm::Expected<std::string>(std::string())
                       : value(expression.operands[0]);
      if (!first)
        return first.takeError();
      if (expression.kind == "table_selection_index_ref" ||
          expression.kind == "table_selection_valid_ref") {
        const std::string sharedKey =
            "selection:" + expression.table + ":" + expression.field + ":" +
            expression.kind + ":" + std::to_string(expression.laneOrdinal);
        auto shared = sharedTableValues
                          ? sharedTableValues->find(sharedKey)
                          : llvm::StringMap<std::string>::iterator();
        if (sharedTableValues && shared != sharedTableValues->end()) {
          result = shared->getValue();
        } else {
          auto selection = llvm::find_if(
              plan.tableSelections, [&](const TableSelectionPlan &candidate) {
                return candidate.name == expression.field &&
                       candidate.table == expression.table;
              });
          if (selection == plan.tableSelections.end())
            return pycError(
                "table_selection_ref references unknown shared selection");
          auto maskExpression = llvm::find_if(
              block.expressions, [&](const QueueExpressionPlan &candidate) {
                return candidate.kind == "table_match_ref" &&
                       candidate.field == selection->match;
              });
          if (maskExpression == block.expressions.end() ||
              !values.contains(maskExpression->result))
            return pycError("shared Table selection match value is missing");
          QueueBlockPlan evaluation;
          auto match = llvm::find_if(
              plan.tableMatches, [&](const TableMatchPlan &candidate) {
                return candidate.name == selection->match &&
                       candidate.table == selection->table;
              });
          if (match == plan.tableMatches.end())
            return pycError("shared Table selection match plan is missing");
          QueueExpressionPlan matchMetadata = *maskExpression;
          matchMetadata.kind = "table_match";
          matchMetadata.table = match->table;
          matchMetadata.type = match->resultType;
          matchMetadata.domainAxes = match->domainAxes;
          matchMetadata.domainShape = match->domainShape;
          matchMetadata.domainStrides = match->domainStrides;
          matchMetadata.domainOffset = match->domainOffset;
          matchMetadata.hasDomainProjection = match->hasDomainProjection;
          evaluation.expressions.push_back(std::move(matchMetadata));
          for (const QueueExpressionPlan &reference : block.expressions) {
            if ((reference.kind != "table_selection_index_ref" &&
                 reference.kind != "table_selection_valid_ref") ||
                reference.field != expression.field)
              continue;
            QueueExpressionPlan choice = reference;
            choice.kind = reference.kind == "table_selection_index_ref"
                              ? "table_choose_index"
                              : "table_choose_valid";
            choice.operands = {maskExpression->result};
            choice.field = selection->stableId.empty() ? selection->name
                                                       : selection->stableId;
            choice.predicate = selection->policy;
            choice.nestedExpressions = selection->keyExpressions;
            if (!selection->keyYield.empty())
              choice.nestedYields = {selection->keyYield};
            choice.selectionCount = selection->count;
            choice.keyOrdering = selection->keyOrdering;
            choice.initialCursor = selection->initialCursor;
            evaluation.yields.push_back(choice.result);
            evaluation.expressions.push_back(std::move(choice));
          }
          llvm::StringMap<std::string> emitted;
          auto selected =
              emitTransform(plan, evaluation, {}, {}, 0, nextValue, body,
                            &emitted, tableValues, roundRobinStates,
                            choiceOwner, &values, &types, sharedTableValues);
          if (!selected)
            return selected.takeError();
          for (const QueueExpressionPlan &reference : block.expressions) {
            if ((reference.kind == "table_selection_index_ref" ||
                 reference.kind == "table_selection_valid_ref") &&
                reference.field == expression.field) {
              auto found = emitted.find(reference.result);
              if (found == emitted.end())
                return pycError("shared Table selection result is missing");
              values[reference.result] = found->getValue();
              types[reference.result] = reference.type;
            }
          }
          result = values[expression.result];
          if (sharedTableValues)
            for (const QueueExpressionPlan &reference : block.expressions) {
              if ((reference.kind != "table_selection_index_ref" &&
                   reference.kind != "table_selection_valid_ref") ||
                  reference.field != expression.field)
                continue;
              const std::string key =
                  "selection:" + reference.table + ":" + reference.field + ":" +
                  reference.kind + ":" + std::to_string(reference.laneOrdinal);
              (*sharedTableValues)[key] = values[reference.result];
            }
        }
      } else if (expression.kind == "table_match_ref") {
        const std::string sharedKey =
            "match:" + expression.table + ":" + expression.field;
        auto shared = sharedTableValues
                          ? sharedTableValues->find(sharedKey)
                          : llvm::StringMap<std::string>::iterator();
        if (sharedTableValues && shared != sharedTableValues->end()) {
          result = shared->getValue();
        } else {
          auto match = llvm::find_if(
              plan.tableMatches, [&](const TableMatchPlan &candidate) {
                return candidate.name == expression.field &&
                       candidate.table == expression.table;
              });
          if (match == plan.tableMatches.end())
            return pycError("table_match_ref references unknown shared match");
          QueueExpressionPlan materialized;
          materialized.result = expression.result;
          materialized.kind = "table_match";
          materialized.type = expression.type;
          materialized.table = match->table;
          materialized.nestedExpressions = match->expressions;
          materialized.nestedYields = {match->yield};
          materialized.domainAxes = match->domainAxes;
          materialized.domainShape = match->domainShape;
          materialized.domainStrides = match->domainStrides;
          materialized.domainOffset = match->domainOffset;
          materialized.hasDomainProjection = match->hasDomainProjection;
          QueueBlockPlan matchBlock;
          matchBlock.expressions.push_back(std::move(materialized));
          matchBlock.yields.push_back(expression.result);
          auto emitted = emitTransform(plan, matchBlock, {}, {}, 0, nextValue,
                                       body, nullptr, tableValues);
          if (!emitted)
            return emitted.takeError();
          result = std::move(*emitted);
          if (sharedTableValues)
            (*sharedTableValues)[sharedKey] = result;
        }
      } else if (expression.kind == "table_choose_index" ||
                 expression.kind == "table_choose_valid") {
        if (!tableValues || expression.operands.size() != 1 ||
            expression.field.empty() || expression.selectionCount == 0)
          return pycError("table_choose expression contract is malformed");
        auto cached = choiceValues.find(expression.field);
        if (cached == choiceValues.end()) {
          const TablePlan *tablePlan = nullptr;
          for (const TablePlan &candidate : plan.tables)
            if (candidate.name == expression.table) {
              tablePlan = &candidate;
              break;
            }
          auto table = tableValues->find(expression.table);
          auto mask = value(expression.operands.front());
          auto maskSourceType = valueType(expression.operands.front());
          auto maskType =
              maskSourceType
                  ? pycType(plan, *maskSourceType)
                  : llvm::Expected<std::string>(maskSourceType.takeError());
          if (!tablePlan || table == tableValues->end())
            return pycError("table_choose references unknown Table bank");
          if (!mask)
            return mask.takeError();
          if (!maskType)
            return maskType.takeError();
          const QueueExpressionPlan *match = nullptr;
          for (const QueueExpressionPlan &candidate : block.expressions)
            if (candidate.result == expression.operands.front() &&
                candidate.kind == "table_match") {
              match = &candidate;
              break;
            }
          if (!match)
            return pycError("table_choose candidate mask has no match plan");
          std::vector<uint64_t> tableIndices;
          if (!match->domainBase.empty()) {
            uint64_t domainEntries = 1;
            for (uint64_t extent : match->domainShape)
              domainEntries *= extent;
            for (uint64_t ordinal = 0; ordinal < domainEntries; ++ordinal)
              tableIndices.push_back(ordinal);
          } else if (!match->hasDomainProjection) {
            for (uint64_t index = 0; index < tablePlan->entries; ++index)
              tableIndices.push_back(index);
          } else {
            uint64_t domainEntries = 1;
            for (uint64_t extent : match->domainShape)
              domainEntries *= extent;
            for (uint64_t ordinal = 0; ordinal < domainEntries; ++ordinal) {
              uint64_t remaining = ordinal;
              uint64_t tableIndex = match->domainOffset;
              for (size_t axis = match->domainShape.size(); axis-- > 0;) {
                const uint64_t coordinate =
                    remaining % match->domainShape[axis];
                remaining /= match->domainShape[axis];
                tableIndex += coordinate * match->domainStrides[axis];
              }
              tableIndices.push_back(tableIndex);
            }
          }
          const unsigned indexWidth =
              std::max(1u, static_cast<unsigned>(std::bit_width(
                               static_cast<unsigned>(tablePlan->entries - 1))));
          const std::string indexType = "i" + std::to_string(indexWidth);
          std::vector<std::string> tableIndexValues;
          tableIndexValues.reserve(tableIndices.size());
          if (!match->domainBase.empty()) {
            auto base = value(match->domainBase);
            if (!base)
              return base.takeError();
            for (uint64_t ordinal = 0; ordinal < tableIndices.size();
                 ++ordinal) {
              if (ordinal == 0) {
                tableIndexValues.push_back(*base);
                continue;
              }
              std::string offset = newValue();
              body << "    " << offset << " = pyc.constant " << ordinal << " : "
                   << indexType << "\n";
              std::string candidate = newValue();
              body << "    " << candidate << " = pyc.add " << *base << ", "
                   << offset << " : " << indexType << ", " << indexType
                   << " -> " << indexType << "\n";
              tableIndexValues.push_back(std::move(candidate));
            }
          } else {
            for (uint64_t tableIndex : tableIndices) {
              std::string candidate = newValue();
              body << "    " << candidate << " = pyc.constant " << tableIndex
                   << " : " << indexType << "\n";
              tableIndexValues.push_back(std::move(candidate));
            }
          }
          std::vector<std::string> remaining;
          remaining.reserve(tableIndices.size());
          for (uint64_t ordinal = 0; ordinal < tableIndices.size(); ++ordinal) {
            std::string valid = newValue();
            body << "    " << valid << " = pyc.extract " << *mask
                 << " {lsb = " << ordinal << "} : " << *maskType << " -> i1\n";
            remaining.push_back(std::move(valid));
          }
          std::vector<std::string> candidateKeys;
          std::string selectionKeyType;
          if (expression.predicate == "min" || expression.predicate == "max") {
            if (expression.nestedYields.size() != 1)
              return pycError("ordered table_choose requires one key");
            QueueBlockPlan keyBlock;
            keyBlock.expressions = expression.nestedExpressions;
            keyBlock.yields = expression.nestedYields;
            auto type = yieldedType(keyBlock, keyBlock.yields.front(),
                                    tablePlan->entryType);
            auto pyc = type ? pycType(plan, *type)
                            : llvm::Expected<std::string>(type.takeError());
            if (!pyc)
              return pyc.takeError();
            selectionKeyType = std::move(*pyc);
            for (uint64_t tableIndex : tableIndices) {
              auto key =
                  emitTransform(plan, keyBlock, {table->getValue()[tableIndex]},
                                {tablePlan->entryType}, 0, nextValue, body,
                                nullptr, tableValues);
              if (!key)
                return key.takeError();
              candidateKeys.push_back(std::move(*key));
            }
          }
          ChoiceValues selected;
          if (expression.predicate == "round_robin") {
            if (!roundRobinStates || choiceOwner.empty())
              return pycError(
                  "round-robin Table selection has no transaction owner");
            const unsigned cursorWidth = std::max(
                1u, static_cast<unsigned>(std::bit_width(
                        static_cast<unsigned>(tableIndices.size() - 1))));
            const std::string cursorType = "i" + std::to_string(cursorWidth);
            RoundRobinPycState rr;
            rr.nextWire = newValue();
            rr.enableWire = newValue();
            std::string initial = newValue();
            body << "    " << rr.nextWire << " = pyc.wire : " << cursorType
                 << "\n";
            body << "    " << rr.enableWire << " = pyc.wire : i1\n";
            body << "    " << initial << " = pyc.constant "
                 << expression.initialCursor << " : " << cursorType << "\n";
            rr.cursor = newValue();
            rr.type = cursorType;
            rr.owner = choiceOwner.str();
            body << "    " << rr.cursor << " = pyc.reg %clk, %rst, "
                 << rr.enableWire << ", " << rr.nextWire << ", " << initial
                 << " : " << cursorType << "\n";
            std::vector<ChoiceValues> byStart;
            std::vector<std::string> nextByStart;
            for (size_t start = 0; start < tableIndices.size(); ++start) {
              std::vector<std::string> localRemaining = remaining;
              ChoiceValues choice;
              std::string nextCursor = rr.cursor;
              for (uint64_t lane = 0; lane < expression.selectionCount;
                   ++lane) {
                std::string chosenValid = newValue();
                body << "    " << chosenValid << " = pyc.constant 0 : i1\n";
                std::string chosenIndex = newValue();
                body << "    " << chosenIndex
                     << " = pyc.constant 0 : " << indexType << "\n";
                std::string chosenLocal = newValue();
                body << "    " << chosenLocal << " = pyc.constant " << start
                     << " : " << cursorType << "\n";
                for (size_t offset = 0; offset < tableIndices.size();
                     ++offset) {
                  const size_t candidate =
                      (start + offset) % tableIndices.size();
                  std::string noChoice = newValue();
                  body << "    " << noChoice << " = pyc.not " << chosenValid
                       << " : i1\n";
                  std::string take = newValue();
                  body << "    " << take << " = pyc.and "
                       << localRemaining[candidate] << ", " << noChoice
                       << " : i1, i1 -> i1\n";
                  const std::string &candidateIndex =
                      tableIndexValues[candidate];
                  std::string nextIndex = newValue();
                  body << "    " << nextIndex << " = pyc.select " << take
                       << ", " << candidateIndex << ", " << chosenIndex
                       << " : i1, " << indexType << ", " << indexType << " -> "
                       << indexType << "\n";
                  chosenIndex = std::move(nextIndex);
                  std::string candidateLocal = newValue();
                  body << "    " << candidateLocal << " = pyc.constant "
                       << candidate << " : " << cursorType << "\n";
                  std::string nextLocal = newValue();
                  body << "    " << nextLocal << " = pyc.select " << take
                       << ", " << candidateLocal << ", " << chosenLocal
                       << " : i1, " << cursorType << ", " << cursorType
                       << " -> " << cursorType << "\n";
                  chosenLocal = std::move(nextLocal);
                  std::string nextValid = newValue();
                  body << "    " << nextValid << " = pyc.or " << chosenValid
                       << ", " << localRemaining[candidate]
                       << " : i1, i1 -> i1\n";
                  chosenValid = std::move(nextValid);
                }
                choice.indices.push_back(chosenIndex);
                choice.valids.push_back(chosenValid);
                for (auto [candidate, tableIndex] :
                     llvm::enumerate(tableIndices)) {
                  (void)tableIndex;
                  const std::string &candidateIndex =
                      tableIndexValues[candidate];
                  std::string same = newValue();
                  body << "    " << same << " = pyc.cmp " << chosenIndex << ", "
                       << candidateIndex
                       << " {predicate = \"eq\"} : " << indexType << ", "
                       << indexType << " -> i1\n";
                  std::string remove = newValue();
                  body << "    " << remove << " = pyc.and " << chosenValid
                       << ", " << same << " : i1, i1 -> i1\n";
                  std::string keep = newValue();
                  body << "    " << keep << " = pyc.not " << remove
                       << " : i1\n";
                  std::string nextRemaining = newValue();
                  body << "    " << nextRemaining << " = pyc.and "
                       << localRemaining[candidate] << ", " << keep
                       << " : i1, i1 -> i1\n";
                  localRemaining[candidate] = std::move(nextRemaining);
                }
                std::string advanced = nextCursor;
                for (size_t candidate = tableIndices.size(); candidate-- > 0;) {
                  std::string candidateLocal = newValue();
                  body << "    " << candidateLocal << " = pyc.constant "
                       << candidate << " : " << cursorType << "\n";
                  std::string same = newValue();
                  body << "    " << same << " = pyc.cmp " << chosenLocal << ", "
                       << candidateLocal
                       << " {predicate = \"eq\"} : " << cursorType << ", "
                       << cursorType << " -> i1\n";
                  std::string use = newValue();
                  body << "    " << use << " = pyc.and " << chosenValid << ", "
                       << same << " : i1, i1 -> i1\n";
                  std::string following = newValue();
                  body << "    " << following << " = pyc.constant "
                       << ((candidate + 1) % tableIndices.size()) << " : "
                       << cursorType << "\n";
                  advanced = [&]() {
                    std::string value = newValue();
                    body << "    " << value << " = pyc.select " << use << ", "
                         << following << ", " << advanced << " : i1, "
                         << cursorType << ", " << cursorType << " -> "
                         << cursorType << "\n";
                    return value;
                  }();
                }
                nextCursor = std::move(advanced);
              }
              byStart.push_back(std::move(choice));
              nextByStart.push_back(std::move(nextCursor));
            }
            selected = byStart.front();
            std::string selectedNext = nextByStart.front();
            for (size_t start = 1; start < tableIndices.size(); ++start) {
              std::string startValue = newValue();
              body << "    " << startValue << " = pyc.constant " << start
                   << " : " << cursorType << "\n";
              std::string atStart = newValue();
              body << "    " << atStart << " = pyc.cmp " << rr.cursor << ", "
                   << startValue << " {predicate = \"eq\"} : " << cursorType
                   << ", " << cursorType << " -> i1\n";
              for (size_t lane = 0; lane < selected.indices.size(); ++lane) {
                std::string index = newValue();
                body << "    " << index << " = pyc.select " << atStart << ", "
                     << byStart[start].indices[lane] << ", "
                     << selected.indices[lane] << " : i1, " << indexType << ", "
                     << indexType << " -> " << indexType << "\n";
                selected.indices[lane] = std::move(index);
                std::string valid = newValue();
                body << "    " << valid << " = pyc.select " << atStart << ", "
                     << byStart[start].valids[lane] << ", "
                     << selected.valids[lane] << " : i1, i1, i1 -> i1\n";
                selected.valids[lane] = std::move(valid);
              }
              std::string next = newValue();
              body << "    " << next << " = pyc.select " << atStart << ", "
                   << nextByStart[start] << ", " << selectedNext << " : i1, "
                   << cursorType << ", " << cursorType << " -> " << cursorType
                   << "\n";
              selectedNext = std::move(next);
            }
            rr.nextCandidate = std::move(selectedNext);
            rr.anyCandidate = selected.valids.front();
            for (size_t lane = 1; lane < selected.valids.size(); ++lane) {
              std::string any = newValue();
              body << "    " << any << " = pyc.or " << rr.anyCandidate << ", "
                   << selected.valids[lane] << " : i1, i1 -> i1\n";
              rr.anyCandidate = std::move(any);
            }
            (*roundRobinStates)[expression.table + "\x1f" + expression.field] =
                std::move(rr);
          } else {
            for (uint64_t lane = 0; lane < expression.selectionCount; ++lane) {
              std::string chosenValid = newValue();
              body << "    " << chosenValid << " = pyc.constant 0 : i1\n";
              std::string chosenIndex = newValue();
              body << "    " << chosenIndex
                   << " = pyc.constant 0 : " << indexType << "\n";
              std::string chosenKey;
              std::string keyType;
              for (auto [candidate, tableIndex] :
                   llvm::enumerate(tableIndices)) {
                (void)tableIndex;
                std::string take;
                std::string key;
                if (expression.predicate == "first") {
                  std::string noChoice = newValue();
                  body << "    " << noChoice << " = pyc.not " << chosenValid
                       << " : i1\n";
                  take = newValue();
                  body << "    " << take << " = pyc.and "
                       << remaining[candidate] << ", " << noChoice
                       << " : i1, i1 -> i1\n";
                } else {
                  if (candidate >= candidateKeys.size() ||
                      selectionKeyType.empty())
                    return pycError(
                        "ordered table_choose key cache is missing");
                  key = candidateKeys[candidate];
                  if (keyType.empty()) {
                    keyType = selectionKeyType;
                    chosenKey = newValue();
                    body << "    " << chosenKey
                         << " = pyc.constant 0 : " << keyType << "\n";
                  }
                  const bool selectMinimum = expression.predicate == "min";
                  if (!selectMinimum && expression.predicate != "max")
                    return pycError("unsupported Table selection policy");
                  const std::string predicate =
                      expression.keyOrdering == "signed" ? "slt" : "ult";
                  std::string better = newValue();
                  body << "    " << better << " = pyc.cmp "
                       << (selectMinimum ? key : chosenKey) << ", "
                       << (selectMinimum ? chosenKey : key)
                       << " {predicate = \"" << predicate << "\"} : " << keyType
                       << ", " << keyType << " -> i1\n";
                  std::string noChoice = newValue();
                  body << "    " << noChoice << " = pyc.not " << chosenValid
                       << " : i1\n";
                  std::string preferred = newValue();
                  body << "    " << preferred << " = pyc.or " << noChoice
                       << ", " << better << " : i1, i1 -> i1\n";
                  take = newValue();
                  body << "    " << take << " = pyc.and "
                       << remaining[candidate] << ", " << preferred
                       << " : i1, i1 -> i1\n";
                  std::string nextKey = newValue();
                  body << "    " << nextKey << " = pyc.select " << take << ", "
                       << key << ", " << chosenKey << " : i1, " << keyType
                       << ", " << keyType << " -> " << keyType << "\n";
                  chosenKey = std::move(nextKey);
                }
                const std::string &candidateIndex = tableIndexValues[candidate];
                std::string nextIndex = newValue();
                body << "    " << nextIndex << " = pyc.select " << take << ", "
                     << candidateIndex << ", " << chosenIndex << " : i1, "
                     << indexType << ", " << indexType << " -> " << indexType
                     << "\n";
                chosenIndex = std::move(nextIndex);
                std::string nextValid = newValue();
                body << "    " << nextValid << " = pyc.or " << chosenValid
                     << ", " << remaining[candidate] << " : i1, i1 -> i1\n";
                chosenValid = std::move(nextValid);
              }
              selected.indices.push_back(chosenIndex);
              selected.valids.push_back(chosenValid);
              for (auto [candidate, tableIndex] :
                   llvm::enumerate(tableIndices)) {
                (void)tableIndex;
                const std::string &candidateIndex = tableIndexValues[candidate];
                std::string same = newValue();
                body << "    " << same << " = pyc.cmp " << chosenIndex << ", "
                     << candidateIndex
                     << " {predicate = \"eq\"} : " << indexType << ", "
                     << indexType << " -> i1\n";
                std::string remove = newValue();
                body << "    " << remove << " = pyc.and " << chosenValid << ", "
                     << same << " : i1, i1 -> i1\n";
                std::string keep = newValue();
                body << "    " << keep << " = pyc.not " << remove << " : i1\n";
                std::string nextRemaining = newValue();
                body << "    " << nextRemaining << " = pyc.and "
                     << remaining[candidate] << ", " << keep
                     << " : i1, i1 -> i1\n";
                remaining[candidate] = std::move(nextRemaining);
              }
            }
          }
          choiceValues[expression.field] = std::move(selected);
          cached = choiceValues.find(expression.field);
        }
        if (expression.laneOrdinal >= expression.selectionCount ||
            expression.laneOrdinal >= cached->getValue().indices.size())
          return pycError("table_choose lane is outside result count");
        result = expression.kind == "table_choose_index"
                     ? cached->getValue().indices[expression.laneOrdinal]
                     : cached->getValue().valids[expression.laneOrdinal];
      } else if (expression.kind == "table_match") {
        if (!tableValues)
          return pycError("table_match expression has no PYC register bank");
        const TablePlan *tablePlan = nullptr;
        for (const TablePlan &candidate : plan.tables)
          if (candidate.name == expression.table) {
            tablePlan = &candidate;
            break;
          }
        auto table = tableValues->find(expression.table);
        if (!tablePlan || table == tableValues->end() ||
            expression.nestedYields.size() != 1)
          return pycError("table_match expression contract is malformed");
        auto maskType = pycType(plan, expression.type);
        auto maskWidth = typeWidth(plan, expression.type);
        if (!maskType)
          return maskType.takeError();
        if (!maskWidth)
          return maskWidth.takeError();
        std::vector<uint64_t> tableIndices;
        if (!expression.domainBase.empty()) {
          uint64_t domainEntries = 1;
          for (uint64_t extent : expression.domainShape)
            domainEntries *= extent;
          for (uint64_t ordinal = 0; ordinal < domainEntries; ++ordinal)
            tableIndices.push_back(ordinal);
        } else if (!expression.hasDomainProjection) {
          for (uint64_t index = 0; index < tablePlan->entries; ++index)
            tableIndices.push_back(index);
        } else {
          uint64_t domainEntries = 1;
          for (uint64_t extent : expression.domainShape)
            domainEntries *= extent;
          for (uint64_t ordinal = 0; ordinal < domainEntries; ++ordinal) {
            uint64_t remaining = ordinal;
            uint64_t tableIndex = expression.domainOffset;
            for (size_t axis = expression.domainShape.size(); axis-- > 0;) {
              const uint64_t coordinate =
                  remaining % expression.domainShape[axis];
              remaining /= expression.domainShape[axis];
              tableIndex += coordinate * expression.domainStrides[axis];
            }
            tableIndices.push_back(tableIndex);
          }
        }
        result = newValue();
        body << "    " << result << " = pyc.constant 0 : " << *maskType << "\n";
        QueueBlockPlan predicateBlock;
        predicateBlock.expressions = expression.nestedExpressions;
        predicateBlock.yields = expression.nestedYields;
        for (auto [ordinal, tableIndex] : llvm::enumerate(tableIndices)) {
          std::string entryValue;
          if (expression.domainBase.empty()) {
            entryValue = table->getValue()[tableIndex];
          } else {
            auto base = value(expression.domainBase);
            auto baseType = valueType(expression.domainBase);
            auto basePycType =
                baseType ? pycType(plan, *baseType)
                         : llvm::Expected<std::string>(baseType.takeError());
            if (!base)
              return base.takeError();
            if (!basePycType)
              return basePycType.takeError();
            entryValue = table->getValue()[ordinal];
            const uint64_t ways = tableIndices.size();
            const uint64_t rows = tablePlan->entries / ways;
            for (uint64_t row = 1; row < rows; ++row) {
              std::string rowBase = newValue();
              body << "    " << rowBase << " = pyc.constant " << row * ways
                   << " : " << *basePycType << "\n";
              std::string atRow = newValue();
              body << "    " << atRow << " = pyc.cmp " << *base << ", "
                   << rowBase << " {predicate = \"eq\"} : " << *basePycType
                   << ", " << *basePycType << " -> i1\n";
              std::string selectedEntry = newValue();
              auto entryPycType = pycType(plan, tablePlan->entryType);
              if (!entryPycType)
                return entryPycType.takeError();
              body << "    " << selectedEntry << " = pyc.select " << atRow
                   << ", " << table->getValue()[row * ways + ordinal] << ", "
                   << entryValue << " : i1, " << *entryPycType << ", "
                   << *entryPycType << " -> " << *entryPycType << "\n";
              entryValue = std::move(selectedEntry);
            }
          }
          llvm::StringMap<std::string> predicateValues(values);
          llvm::StringMap<std::string> predicateTypes(types);
          predicateValues["entry"] = entryValue;
          predicateTypes["entry"] = tablePlan->entryType;
          if (!predicateValues.contains("item")) {
            predicateValues["item"] = entryValue;
            predicateTypes["item"] = tablePlan->entryType;
          }
          auto predicate = emitTransform(
              plan, predicateBlock, {}, {}, 0, nextValue, body, nullptr,
              tableValues, nullptr, {}, &predicateValues, &predicateTypes);
          if (!predicate)
            return predicate.takeError();
          llvm::APInt bit(*maskWidth, 1);
          bit <<= ordinal;
          std::string bitValue = newValue();
          body << "    " << bitValue << " = pyc.constant "
               << unsignedDecimal(bit) << " : " << *maskType << "\n";
          std::string zero = newValue();
          body << "    " << zero << " = pyc.constant 0 : " << *maskType << "\n";
          std::string selected = newValue();
          body << "    " << selected << " = pyc.select " << *predicate << ", "
               << bitValue << ", " << zero << " : i1, " << *maskType << ", "
               << *maskType << " -> " << *maskType << "\n";
          std::string combined = newValue();
          body << "    " << combined << " = pyc.or " << result << ", "
               << selected << " : " << *maskType << ", " << *maskType << " -> "
               << *maskType << "\n";
          result = std::move(combined);
        }
      } else if (expression.kind == "table_index") {
        const TablePlan *tablePlan = nullptr;
        for (const TablePlan &candidate : plan.tables)
          if (candidate.name == expression.table) {
            tablePlan = &candidate;
            break;
          }
        if (!tablePlan)
          return pycError("Table index expression references unknown Table");
        std::vector<uint64_t> legacyShape;
        llvm::ArrayRef<uint64_t> shape = tablePlan->shape;
        if (shape.empty()) {
          legacyShape.push_back(tablePlan->entries);
          shape = legacyShape;
        }
        if (expression.operands.size() != shape.size())
          return pycError("Table index expression is malformed");
        auto resultType = pycType(plan, expression.type);
        if (!resultType)
          return resultType.takeError();
        std::vector<uint64_t> strides(shape.size(), 1);
        for (size_t axis = shape.size(); axis > 1; --axis)
          strides[axis - 2] = strides[axis - 1] * shape[axis - 1];
        result = newValue();
        body << "    " << result << " = pyc.constant 0 : " << *resultType
             << "\n";
        for (auto [axis, operandName] : llvm::enumerate(expression.operands)) {
          auto coordinate = value(operandName);
          auto coordinateType = valueType(operandName);
          auto coordinatePycType =
              coordinateType
                  ? pycType(plan, *coordinateType)
                  : llvm::Expected<std::string>(coordinateType.takeError());
          if (!coordinate)
            return coordinate.takeError();
          if (!coordinatePycType)
            return coordinatePycType.takeError();
          auto coordinateWidth = typeWidth(plan, *coordinateType);
          auto flattenedWidth = typeWidth(plan, expression.type);
          if (!coordinateWidth)
            return coordinateWidth.takeError();
          if (!flattenedWidth)
            return flattenedWidth.takeError();
          if (*coordinateWidth < 64 &&
              shape[axis] < (uint64_t{1} << *coordinateWidth)) {
            std::string extent = newValue();
            body << "    " << extent << " = pyc.constant " << shape[axis]
                 << " : " << *coordinatePycType << "\n";
            std::string inBounds = newValue();
            body << "    " << inBounds << " = pyc.cmp " << *coordinate << ", "
                 << extent << " {predicate = \"ult\"} : " << *coordinatePycType
                 << ", " << *coordinatePycType << " -> i1\n";
            body << "    pyc.assert " << inBounds
                 << " {msg = \"table_index_out_of_range\"}\n";
          }
          std::string widened = *coordinate;
          if (*coordinateWidth != *flattenedWidth) {
            widened = newValue();
            body << "    " << widened << " = pyc.zext " << *coordinate << " : "
                 << *coordinatePycType << " -> " << *resultType << "\n";
          }
          if (strides[axis] != 1) {
            std::string stride = newValue();
            body << "    " << stride << " = pyc.constant " << strides[axis]
                 << " : " << *resultType << "\n";
            std::string product = newValue();
            body << "    " << product << " = pyc.mul " << widened << ", "
                 << stride << " : " << *resultType << ", " << *resultType
                 << " -> " << *resultType << "\n";
            widened = std::move(product);
          }
          std::string sum = newValue();
          body << "    " << sum << " = pyc.add " << result << ", " << widened
               << " : " << *resultType << ", " << *resultType << " -> "
               << *resultType << "\n";
          result = std::move(sum);
        }
      } else if (expression.kind == "table_get") {
        if (!tableValues || expression.operands.size() != 1)
          return pycError("table_get expression has no PYC register bank");
        auto table = tableValues->find(expression.table);
        const TablePlan *tablePlan = nullptr;
        for (const TablePlan &candidate : plan.tables)
          if (candidate.name == expression.table) {
            tablePlan = &candidate;
            break;
          }
        if (table == tableValues->end() || !tablePlan ||
            table->getValue().size() != tablePlan->entries)
          return pycError("table_get expression references unknown Table bank");
        auto indexType = valueType(expression.operands.front());
        auto indexPycType =
            indexType ? pycType(plan, *indexType)
                      : llvm::Expected<std::string>(indexType.takeError());
        auto entryPycType = pycType(plan, tablePlan->entryType);
        if (!indexPycType)
          return indexPycType.takeError();
        if (!entryPycType)
          return entryPycType.takeError();
        result = table->getValue().back();
        for (size_t index = tablePlan->entries; index-- > 0;) {
          if (!pycIntegerCanRepresent(index, *indexPycType))
            continue;
          std::string indexValue = newValue();
          body << "    " << indexValue << " = pyc.constant " << index << " : "
               << *indexPycType << "\n";
          std::string selected = newValue();
          body << "    " << selected << " = pyc.cmp " << *first << ", "
               << indexValue << " {predicate = \"eq\"} : " << *indexPycType
               << ", " << *indexPycType << " -> i1\n";
          std::string next = newValue();
          body << "    " << next << " = pyc.select " << selected << ", "
               << table->getValue()[index] << ", " << result << " : i1, "
               << *entryPycType << ", " << *entryPycType << " -> "
               << *entryPycType << "\n";
          result = std::move(next);
        }
      } else if (expression.kind == "masked_match") {
        if (expression.operands.size() != 1)
          return pycError("matches expression arity mismatch");
        auto inputType = valueType(expression.operands[0]);
        if (!inputType)
          return inputType.takeError();
        auto type = pycType(plan, *inputType);
        if (!type)
          return type.takeError();
        std::string mask = newValue();
        std::string masked = newValue();
        std::string expected = newValue();
        result = newValue();
        body << "    " << mask << " = pyc.constant " << expression.mask << " : "
             << *type << "\n";
        body << "    " << masked << " = pyc.and " << *first << ", " << mask
             << " : " << *type << ", " << *type << " -> " << *type << "\n";
        body << "    " << expected << " = pyc.constant " << expression.value
             << " : " << *type << "\n";
        body << "    " << result << " = pyc.cmp " << masked << ", " << expected
             << " {predicate = \"eq\"} : " << *type << ", " << *type
             << " -> i1\n";
      } else if (expression.kind == "priority_index" ||
                 expression.kind == "priority_valid") {
        if (expression.operands.size() != 1 ||
            (expression.predicate != "low" && expression.predicate != "high"))
          return pycError("priority encoder expression contract is malformed");
        auto inputType = valueType(expression.operands[0]);
        if (!inputType)
          return inputType.takeError();
        auto sourceType = pycType(plan, *inputType);
        auto inputWidth = typeWidth(plan, *inputType);
        if (!sourceType)
          return sourceType.takeError();
        if (!inputWidth)
          return inputWidth.takeError();
        unsigned indexWidth = acir::primitivePriorityIndexWidth(*inputWidth);
        std::string key = expression.operands[0] + "#" + expression.predicate;
        auto found = priorityValues.find(key);
        if (found == priorityValues.end()) {
          std::string encodedIndex = newValue();
          std::string encodedValid = newValue();
          body << "    " << encodedIndex << ", " << encodedValid
               << " = pyc.priority_encode " << *first << " {order = \""
               << expression.predicate << "\"} : " << *sourceType << " -> i"
               << indexWidth << ", i1\n";
          found =
              priorityValues.try_emplace(key, encodedIndex, encodedValid).first;
        }
        result = expression.kind == "priority_index" ? found->getValue().first
                                                     : found->getValue().second;
      } else if (expression.kind == "popcount") {
        if (expression.operands.size() != 1)
          return pycError("popcount expression arity mismatch");
        auto inputType = valueType(expression.operands[0]);
        if (!inputType)
          return inputType.takeError();
        auto sourceType = pycType(plan, *inputType);
        auto resultType = pycType(plan, expression.type);
        if (!sourceType)
          return sourceType.takeError();
        if (!resultType)
          return resultType.takeError();
        result = newValue();
        body << "    " << result << " = pyc.popcount " << *first << " : "
             << *sourceType << " -> " << *resultType << "\n";
      } else if (expression.kind == "count_zeros") {
        if (expression.operands.size() != 1)
          return pycError("count_zeros expression arity mismatch");
        auto inputType = valueType(expression.operands[0]);
        if (!inputType)
          return inputType.takeError();
        auto sourceType = pycType(plan, *inputType);
        auto resultType = pycType(plan, expression.type);
        if (!sourceType)
          return sourceType.takeError();
        if (!resultType)
          return resultType.takeError();
        result = newValue();
        body << "    " << result << " = pyc.count_zeros " << *first
             << " {direction = \"" << expression.predicate
             << "\"} : " << *sourceType << " -> " << *resultType << "\n";
      } else if (expression.kind == "bit_extract" ||
                 expression.kind == "aggregate_get") {
        if (expression.operands.size() != 1 || expression.width == 0)
          return pycError("extract expression contract is malformed");
        auto inputType = valueType(expression.operands[0]);
        auto sourceType =
            inputType ? pycType(plan, *inputType)
                      : llvm::Expected<std::string>(inputType.takeError());
        auto resultType = pycType(plan, expression.type);
        if (!sourceType)
          return sourceType.takeError();
        if (!resultType)
          return resultType.takeError();
        result = newValue();
        body << "    " << result << " = pyc.extract " << *first
             << " {lsb = " << expression.lsb << "} : " << *sourceType << " -> "
             << *resultType << "\n";
      } else if (expression.kind == "bit_concat" ||
                 expression.kind == "tuple_create" ||
                 expression.kind == "array_create" ||
                 expression.kind == "record_create") {
        if (expression.operands.empty())
          return pycError(
              "concat/aggregate create requires at least one operand");
        result = newValue();
        body << "    " << result << " = pyc.concat(";
        for (auto [index, operandName] : llvm::enumerate(expression.operands)) {
          auto operandValue = value(operandName);
          if (!operandValue)
            return operandValue.takeError();
          if (index)
            body << ", ";
          body << *operandValue;
        }
        body << ") : (";
        for (auto [index, operandName] : llvm::enumerate(expression.operands)) {
          auto operandType = valueType(operandName);
          auto type =
              operandType
                  ? pycType(plan, *operandType)
                  : llvm::Expected<std::string>(operandType.takeError());
          if (!type)
            return type.takeError();
          if (index)
            body << ", ";
          body << *type;
        }
        auto resultType = pycType(plan, expression.type);
        if (!resultType)
          return resultType.takeError();
        body << ") -> " << *resultType << "\n";
      } else if (expression.kind == "bit_insert") {
        if (expression.operands.size() != 2)
          return pycError("bit_insert expression contract is malformed");
        auto inserted = value(expression.operands[1]);
        auto insertedType = valueType(expression.operands[1]);
        auto baseWidth = typeWidth(plan, expression.type);
        auto valueWidth =
            insertedType ? typeWidth(plan, *insertedType)
                         : llvm::Expected<unsigned>(insertedType.takeError());
        if (!inserted)
          return inserted.takeError();
        if (!baseWidth)
          return baseWidth.takeError();
        if (!valueWidth)
          return valueWidth.takeError();
        std::vector<std::pair<std::string, unsigned>> parts;
        const unsigned highWidth =
            *baseWidth - static_cast<unsigned>(expression.lsb) - *valueWidth;
        if (highWidth > 0) {
          std::string high = newValue();
          body << "    " << high << " = pyc.extract " << *first
               << " {lsb = " << expression.lsb + *valueWidth << "} : i"
               << *baseWidth << " -> i" << highWidth << "\n";
          parts.emplace_back(std::move(high), highWidth);
        }
        parts.emplace_back(*inserted, *valueWidth);
        if (expression.lsb > 0) {
          std::string low = newValue();
          body << "    " << low << " = pyc.extract " << *first
               << " {lsb = 0} : i" << *baseWidth << " -> i" << expression.lsb
               << "\n";
          parts.emplace_back(std::move(low), expression.lsb);
        }
        if (parts.size() == 1) {
          result = parts.front().first;
        } else {
          result = newValue();
          body << "    " << result << " = pyc.concat(";
          for (auto [index, part] : llvm::enumerate(parts)) {
            if (index)
              body << ", ";
            body << part.first;
          }
          body << ") : (";
          for (auto [index, part] : llvm::enumerate(parts)) {
            if (index)
              body << ", ";
            body << 'i' << part.second;
          }
          body << ") -> i" << *baseWidth << "\n";
        }
      } else if (expression.kind == "value_select") {
        if (expression.operands.size() != 3)
          return pycError("value_select expression arity mismatch");
        auto trueValue = value(expression.operands[1]);
        auto falseValue = value(expression.operands[2]);
        auto type = pycType(plan, expression.type);
        if (!trueValue)
          return trueValue.takeError();
        if (!falseValue)
          return falseValue.takeError();
        if (!type)
          return type.takeError();
        result = newValue();
        body << "    " << result << " = pyc.select " << *first << ", "
             << *trueValue << ", " << *falseValue << " : i1, " << *type << ", "
             << *type << " -> " << *type << "\n";
      } else if (expression.kind == "not") {
        if (expression.operands.size() != 1)
          return pycError("unary transform expression arity mismatch");
        auto type = pycType(plan, expression.type);
        if (!type)
          return type.takeError();
        result = newValue();
        body << "    " << result << " = pyc.not " << *first << " : " << *type
             << "\n";
      } else if (expression.kind == "add" || expression.kind == "sub" ||
                 expression.kind == "mul" || expression.kind == "udiv" ||
                 expression.kind == "urem" || expression.kind == "and" ||
                 expression.kind == "or" || expression.kind == "xor") {
        result = newValue();
        if (expression.operands.size() != 2)
          return pycError("binary transform expression arity mismatch");
        auto second = value(expression.operands[1]);
        if (!second)
          return second.takeError();
        auto type = pycType(plan, expression.type);
        if (!type)
          return type.takeError();
        body << "    " << result << " = pyc." << expression.kind << ' '
             << *first << ", " << *second << " : " << *type << ", " << *type
             << " -> " << *type << "\n";
      } else if (expression.kind == "shl" || expression.kind == "shr") {
        result = newValue();
        if (expression.operands.size() != 2)
          return pycError("shift transform expression arity mismatch");
        auto second = value(expression.operands[1]);
        if (!second)
          return second.takeError();
        auto type = pycType(plan, expression.type);
        if (!type)
          return type.takeError();
        body << "    " << result << " = pyc."
             << (expression.kind == "shl" ? "shl" : "lshr") << ' ' << *first
             << ", " << *second << " : " << *type << ", " << *type << "\n";
      } else if (expression.kind == "cmp") {
        if (expression.operands.size() != 2)
          return pycError("comparison expression arity mismatch");
        auto second = value(expression.operands[1]);
        auto firstType = valueType(expression.operands[0]);
        auto secondType = valueType(expression.operands[1]);
        if (!second)
          return second.takeError();
        if (!firstType)
          return firstType.takeError();
        if (!secondType)
          return secondType.takeError();
        if (*firstType != *secondType)
          return pycError("comparison operand types must match");
        auto type = pycType(plan, *firstType);
        if (!type)
          return type.takeError();

        llvm::StringRef opcode;
        std::string lhs = *first;
        std::string rhs = *second;
        bool negate = false;
        if (expression.predicate == "eq" || expression.predicate == "ne") {
          opcode = "eq";
          negate = expression.predicate == "ne";
        } else if (expression.predicate == "slt" ||
                   expression.predicate == "sge" ||
                   expression.predicate == "ult" ||
                   expression.predicate == "uge") {
          opcode = expression.predicate.starts_with("u") ? "ult" : "slt";
          negate = expression.predicate.ends_with("ge");
        } else if (expression.predicate == "sgt" ||
                   expression.predicate == "sle" ||
                   expression.predicate == "ugt" ||
                   expression.predicate == "ule") {
          opcode = expression.predicate.starts_with("u") ? "ult" : "slt";
          std::swap(lhs, rhs);
          negate = expression.predicate.ends_with("le");
        } else {
          return pycError("unsupported comparison predicate");
        }
        std::string compared = newValue();
        body << "    " << compared << " = pyc.cmp " << lhs << ", " << rhs
             << " {predicate = \"" << opcode.str() << "\"} : " << *type << ", "
             << *type << " -> i1\n";
        if (negate) {
          result = newValue();
          body << "    " << result << " = pyc.not " << compared << " : i1\n";
        } else {
          result = std::move(compared);
        }
      } else if (expression.kind == "get") {
        auto recordType = valueType(expression.operands[0]);
        if (!recordType)
          return recordType.takeError();
        auto layout = fieldLayout(plan, *recordType, expression.field);
        auto sourceType = pycType(plan, *recordType);
        auto resultType = pycType(plan, expression.type);
        if (!layout)
          return layout.takeError();
        if (!sourceType)
          return sourceType.takeError();
        if (!resultType)
          return resultType.takeError();
        result = newValue();
        body << "    " << result << " = pyc.extract " << *first
             << " {lsb = " << layout->lsb << "} : " << *sourceType << " -> "
             << *resultType << "\n";
      } else if (expression.kind == "with") {
        if (expression.operands.size() != 2)
          return pycError("packed field update arity mismatch");
        auto second = value(expression.operands[1]);
        auto recordType = valueType(expression.operands[0]);
        if (!second)
          return second.takeError();
        if (!recordType)
          return recordType.takeError();
        auto layout = fieldLayout(plan, *recordType, expression.field);
        auto totalWidth = typeWidth(plan, *recordType);
        if (!layout)
          return layout.takeError();
        if (!totalWidth)
          return totalWidth.takeError();
        std::vector<std::pair<std::string, unsigned>> parts;
        const unsigned highWidth = *totalWidth - layout->lsb - layout->width;
        if (highWidth > 0) {
          std::string high = newValue();
          body << "    " << high << " = pyc.extract " << *first
               << " {lsb = " << layout->lsb + layout->width << "} : i"
               << *totalWidth << " -> i" << highWidth << "\n";
          parts.emplace_back(std::move(high), highWidth);
        }
        parts.emplace_back(*second, layout->width);
        if (layout->lsb > 0) {
          std::string low = newValue();
          body << "    " << low << " = pyc.extract " << *first
               << " {lsb = 0} : i" << *totalWidth << " -> i" << layout->lsb
               << "\n";
          parts.emplace_back(std::move(low), layout->lsb);
        }
        if (parts.size() == 1) {
          result = parts.front().first;
        } else {
          result = newValue();
          body << "    " << result << " = pyc.concat(";
          for (auto [index, part] : llvm::enumerate(parts)) {
            if (index)
              body << ", ";
            body << part.first;
          }
          body << ") : (";
          for (auto [index, part] : llvm::enumerate(parts)) {
            if (index)
              body << ", ";
            body << 'i' << part.second;
          }
          body << ") -> i" << *totalWidth << "\n";
        }
      } else {
        return pycError("unsupported PYC transform expression");
      }
    }
    values[expression.result] = result;
    types[expression.result] = expression.type;
  }
  if (yieldIndex >= block.yields.size()) {
    if (emittedValues) {
      *emittedValues = std::move(values);
      return std::string();
    } else {
      return pycError("transform yield index is outside result arity");
    }
  }
  auto yielded = value(block.yields[yieldIndex]);
  if (!yielded)
    return yielded.takeError();
  std::string result = std::move(*yielded);
  if (emittedValues)
    *emittedValues = std::move(values);
  return result;
}

} // namespace

llvm::Expected<std::string> generateQueueGraphPyc(const QueueGraphPlan &plan) {
  if (!plan.definition.empty())
    return pycError(
        "module-preserving QueueGraph PYC lowering is not implemented");
  if (!plan.slots.empty())
    return pycError("Slot PYC lowering is not implemented");
  if (!plan.tables.empty()) {
    if (auto error = verifyTablePycProfile(plan))
      return std::move(error);
  }
  if (!plan.scopes.empty()) {
    const QueueBlockContract *scope = findQueueBlockContract("scope");
    if (!scope || !scope->pycAvailable)
      return pycError("official opcode has no PYC lowering: 'scope'");
  }
  struct TransformProducer {
    const QueueBlockPlan *block = nullptr;
    size_t index = 0;
  };
  struct RouteProducer {
    const QueueBlockPlan *block = nullptr;
    size_t index = 0;
  };
  struct TableReadGroupState {
    std::vector<std::string> indices;
    std::vector<std::string> valids;
  };
  struct SelectState {
    std::vector<std::string> conditions;
    std::string controlValid;
    std::string selectedValid;
    std::string selectorSafe;
  };
  struct MergeState {
    std::string nextWire;
    std::string enableWire;
    std::string cursor;
    std::string valid;
    std::string type;
  };
  struct ForkState {
    std::vector<std::string> nextWires;
    std::vector<std::string> enableWires;
    std::vector<std::string> delivered;
  };
  struct FeedbackState {
    std::string validNext;
    std::string validEnable;
    std::string valid;
    std::string dataNext;
    std::string dataEnable;
    std::string data;
    std::string iterationNext;
    std::string iterationEnable;
    std::string iteration;
    std::string selectedValid;
    std::string selectedIteration;
    std::string condition;
    std::string updated;
    std::string underLimit;
    std::string dataType;
    std::string iterationType;
  };
  struct TableState {
    std::vector<std::string> next;
    std::vector<std::string> enable;
    std::vector<std::string> value;
    std::string type;
  };
  struct ReorderSlotState {
    std::string next;
    std::string enable;
    std::string state;
    std::string valid;
    std::string key;
    std::string data;
    std::string free;
    std::string match;
  };
  struct ReorderState {
    std::vector<ReorderSlotState> slots;
    std::string expectedNext;
    std::string expectedEnable;
    std::string expected;
    std::string inputKey;
    std::string anyFree;
    std::string freeIndex;
    std::string freeIndexType;
    std::string outputMatch;
    std::string safeAdmission;
    std::string keyType;
    std::string dataType;
    std::string slotType;
  };
  struct DependencySlotState {
    std::string next;
    std::string enable;
    std::string state;
    std::string valid;
    std::string phase;
    std::string key;
    std::string predecessor;
    std::string resource;
    std::string remaining;
    std::string cost;
    std::string data;
    std::string free;
    std::string done;
  };
  struct DependencyState {
    std::vector<DependencySlotState> slots;
    std::string inputKey;
    std::string inputPredecessor;
    std::string inputResource;
    std::string inputCost;
    std::string anyFree;
    std::string freeIndex;
    std::string freeIndexType;
    std::string outputDone;
    std::string doneIndex;
    std::string safeAdmission;
    std::string keyType;
    std::string resourceType;
    std::string costType;
    std::string dataType;
    std::string slotType;
    uint64_t noDependency = 0;
    uint64_t resources = 0;
  };
  struct CreditSlotState {
    std::string next;
    std::string enable;
    std::string state;
    std::string valid;
    std::string remaining;
    std::string data;
    std::string free;
    std::string done;
  };
  struct CreditState {
    std::vector<CreditSlotState> slots;
    std::string inputCost;
    std::string anyFree;
    std::string freeIndex;
    std::string freeIndexType;
    std::string outputDone;
    std::string doneIndex;
    std::string safeAdmission;
    std::string costType;
    std::string dataType;
    std::string slotType;
  };
  std::vector<const QueueBlockPlan *> sources;
  std::vector<const QueueBlockPlan *> sinks;
  std::vector<const QueueBlockPlan *> observations;
  llvm::StringMap<TransformProducer> transformByOutput;
  llvm::StringMap<TransformProducer> firingByOutput;
  llvm::StringMap<const QueueBlockPlan *> tableReadByOutput;
  llvm::StringMap<TransformProducer> tableReadGroupByOutput;
  llvm::StringMap<TransformProducer> barrierByOutput;
  llvm::StringMap<const QueueBlockPlan *> broadcastByOutput;
  llvm::StringMap<const QueueBlockPlan *> forkByOutput;
  llvm::StringMap<RouteProducer> routeByOutput;
  llvm::StringMap<const QueueBlockPlan *> selectByOutput;
  llvm::StringMap<const QueueBlockPlan *> mergeByOutput;
  llvm::StringMap<const QueueBlockPlan *> creditByOutput;
  llvm::StringMap<const QueueBlockPlan *> memoryByOutput;
  llvm::StringMap<const QueueBlockPlan *> dependencyByOutput;
  llvm::StringMap<const QueueBlockPlan *> reorderByOutput;
  llvm::StringMap<const QueueBlockPlan *> feedbackByOutput;
  for (const QueueBlockPlan &block : plan.blocks) {
    const QueueBlockContract *contract = findQueueBlockContract(block.kind);
    const bool admittedTableBlock =
        block.kind == "table_read" || block.kind == "table_write" ||
        block.kind == "table_masked_write" || block.kind == "table_read_group";
    if (contract && contract->role == "verification")
      return pycError("verification-only opcode '" + contract->operation +
                      "' cannot appear in a design hierarchy; place it at "
                      "the PYC testbench boundary");
    if ((!contract || !contract->pycAvailable) && !admittedTableBlock)
      return pycError("official opcode has no PYC lowering: '" + block.kind +
                      "'");
    if (block.kind == "source")
      sources.push_back(&block);
    else if (block.kind == "sink")
      sinks.push_back(&block);
    else if (block.kind == "observe")
      observations.push_back(&block);
    else if (block.kind == "transform") {
      if (block.inputs.empty() || block.outputs.empty() ||
          block.yields.size() != block.outputs.size())
        return pycError("transform output arity is unsupported");
      for (auto [index, output] : llvm::enumerate(block.outputs))
        transformByOutput[output] = TransformProducer{&block, index};
    } else if (block.kind == "firing") {
      if (block.yields.size() != block.outputs.size() ||
          block.outputPresence.size() != block.outputs.size() ||
          block.guard.empty() ||
          (block.outputs.empty() && block.stateWrites.empty()))
        return pycError("stateful or malformed firing has no PYC lowering");
      for (auto [index, output] : llvm::enumerate(block.outputs))
        firingByOutput[output] = TransformProducer{&block, index};
    } else if (block.kind == "table_read") {
      if (block.outputs.size() != 1 || block.yields.size() != 2)
        return pycError("Table read contract is unsupported");
      tableReadByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "table_read_group") {
      if (block.inputs.size() != 0 || block.outputs.empty() ||
          block.outputs.size() != block.selectionCount)
        return pycError("grouped Table read contract is unsupported");
      for (auto [index, output] : llvm::enumerate(block.outputs))
        tableReadGroupByOutput[output] = TransformProducer{&block, index};
    } else if (block.kind == "table_write") {
      if (!block.outputs.empty() || block.yields.size() != 3)
        return pycError("Table write contract is unsupported");
    } else if (block.kind == "table_masked_write") {
      if (!block.inputs.empty() || !block.outputs.empty() ||
          block.yields.size() != 3 || block.writeMode != "field")
        return pycError("masked Table write contract is unsupported");
    } else if (block.kind == "route") {
      if (block.inputs.size() != 1 || block.outputs.size() < 2)
        return pycError("route arity is unsupported");
      for (auto [index, output] : llvm::enumerate(block.outputs))
        routeByOutput[output] = RouteProducer{&block, index};
    } else if (block.kind == "select") {
      if (block.inputs.size() < 3 || block.outputs.size() != 1 ||
          block.yields.size() != 1)
        return pycError("select contract is unsupported");
      selectByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "broadcast") {
      if (block.inputs.size() != 1 || block.outputs.size() < 2)
        return pycError("broadcast arity is unsupported");
      for (const std::string &output : block.outputs)
        broadcastByOutput[output] = &block;
    } else if (block.kind == "fork") {
      if (block.inputs.size() != 1 || block.outputs.size() < 2)
        return pycError("fork arity is unsupported");
      for (const std::string &output : block.outputs)
        forkByOutput[output] = &block;
    } else if (block.kind == "merge") {
      if (block.outputs.size() != 1 || block.inputs.size() < 2)
        return pycError("merge arity is unsupported");
      if (block.policy != "priority" && block.policy != "round_robin")
        return pycError("PYC merge policy must be priority or round_robin");
      mergeByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "barrier") {
      if (block.inputs.size() < 2 ||
          block.outputs.size() != block.inputs.size())
        return pycError("barrier contract is unsupported");
      for (auto [index, output] : llvm::enumerate(block.outputs))
        barrierByOutput[output] = TransformProducer{&block, index};
    } else if (block.kind == "credit") {
      if (block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.yields.size() != 1 || block.credits == 0)
        return pycError("credit contract is unsupported");
      creditByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "memory_request") {
      if (block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.yields.size() != 3 || block.memoryInstance.empty() ||
          block.resultField.empty())
        return pycError("memory contract is unsupported");
      memoryByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "reorder") {
      if (block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.yields.size() != 1 || block.capacity == 0)
        return pycError("reorder contract is unsupported");
      reorderByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "dependency") {
      if (block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.yields.size() != 4 || block.capacity == 0 ||
          block.resources == 0)
        return pycError("dependency contract is unsupported");
      dependencyByOutput[block.outputs.front()] = &block;
    } else if (block.kind == "feedback") {
      if (block.inputs.size() != 1 || block.outputs.size() != 1 ||
          block.yields.size() != 2 || block.maxIterations == 0)
        return pycError("feedback contract is unsupported");
      feedbackByOutput[block.outputs.front()] = &block;
    } else {
      return pycError("PYC QueueGraph supports "
                      "source/transform/broadcast/fork/route/select/"
                      "firing/merge/barrier/credit/memory_request/dependency/"
                      "reorder/feedback/"
                      "observe/sink");
    }
  }
  if (auto error = verifyQueueGraphPlan(plan))
    return std::move(error);
  if (sources.empty() && sinks.empty())
    return pycError("PYC lowering requires at least one external boundary");
  if (llvm::any_of(plan.queues, [](const QueuePlan &queue) {
        return queue.lanes > 1 || queue.rate > 1;
      }))
    return generateLaneQueuePyc(plan, sources, sinks);
  for (const QueuePlan &queue : plan.queues) {
    if (auto width = typeWidth(plan, queue.payloadType); !width)
      return width.takeError();
    if (queue.latency == 0)
      return pycError("PYC Queue latency must be positive");
    if (queue.lanes != 1 || queue.rate != 1)
      return pycError("PYC scalar Queue requires lanes=rate=1");
  }
  llvm::StringMap<size_t> sourceBoundary;
  std::vector<std::string> inputPortTypes;
  for (auto [index, source] : llvm::enumerate(sources)) {
    const QueuePlan *queue = findQueue(plan, source->outputs.front());
    if (!queue)
      return pycError("source Queue is missing");
    auto type = pycType(plan, queue->payloadType);
    if (!type)
      return type.takeError();
    sourceBoundary[source->outputs.front()] = index;
    inputPortTypes.push_back(std::move(*type));
  }
  std::vector<std::string> outputPortTypes;
  for (const QueueBlockPlan *sink : sinks) {
    const QueuePlan *queue = findQueue(plan, sink->inputs.front());
    if (!queue)
      return pycError("sink Queue is missing");
    auto type = pycType(plan, queue->payloadType);
    if (!type)
      return type.takeError();
    outputPortTypes.push_back(std::move(*type));
  }
  auto inputName = [&](size_t index, llvm::StringRef suffix) {
    return sources.size() == 1
               ? ("%in_" + suffix).str()
               : ("%in" + std::to_string(index) + "_" + suffix.str());
  };
  auto outputName = [&](size_t index, llvm::StringRef suffix) {
    return sinks.size() == 1
               ? ("%out_" + suffix).str()
               : ("%out" + std::to_string(index) + "_" + suffix.str());
  };

  llvm::StringMap<std::string> readyWires;
  llvm::StringMap<std::string> inputReady;
  llvm::StringMap<std::string> outputValid;
  llvm::StringMap<std::string> outputData;
  llvm::StringMap<std::string> routeSelector;
  llvm::StringMap<std::string> routeCondition;
  llvm::StringMap<SelectState> selectStates;
  llvm::StringMap<std::string> atomicTransformValid;
  llvm::StringMap<std::string> firingPresence;
  llvm::StringMap<std::string> firingGuard;
  llvm::StringMap<std::string> firingAccepted;
  llvm::StringMap<std::string> tableReadWhen;
  llvm::StringMap<std::shared_ptr<llvm::StringMap<std::string>>>
      tableWriteExpressionValues;
  llvm::StringMap<std::string> tableWriteAccepted;
  struct MaskedWriteValues {
    std::string mask;
    std::string enabled;
  };
  llvm::StringMap<MaskedWriteValues> tableMaskedWriteValues;
  llvm::StringMap<TableReadGroupState> tableReadGroupStates;
  llvm::StringMap<std::string> tableReadGroupValidWires;
  llvm::StringMap<std::string> tableSelectionAccepted;
  llvm::StringMap<std::shared_ptr<llvm::StringMap<std::string>>>
      firingExpressionValues;
  llvm::StringMap<std::shared_ptr<llvm::StringMap<std::string>>>
      firingExpressionValuesByBlock;
  llvm::StringMap<std::string> firingGuardByBlock;
  llvm::StringMap<std::vector<std::string>> mergeGrants;
  llvm::StringMap<MergeState> mergeStates;
  llvm::StringMap<ForkState> forkStates;
  llvm::StringMap<FeedbackState> feedbackStates;
  llvm::StringMap<CreditState> creditStates;
  llvm::StringMap<std::string> memoryResponseValid;
  llvm::StringMap<std::string> memoryResponseData;
  llvm::StringMap<DependencyState> dependencyStates;
  llvm::StringMap<ReorderState> reorderStates;
  llvm::StringMap<std::string> forkOfferValid;
  llvm::StringMap<TableState> tableStates;
  llvm::StringMap<std::vector<std::string>> tableStateValues;
  llvm::StringMap<RoundRobinPycState> roundRobinStates;
  llvm::StringMap<std::string> blockArbitrationAllowed;
  llvm::StringMap<std::string> sharedTableExpressionValues;
  std::ostringstream body;
  unsigned nextValue = 0;
  auto newValue = [&]() { return "%v" + std::to_string(nextValue++); };
  auto emitConstant = [&](uint64_t value, llvm::StringRef type) {
    std::string result = newValue();
    body << "    " << result << " = pyc.constant " << value << " : "
         << type.str() << "\n";
    return result;
  };
  auto emitBinary = [&](llvm::StringRef operation, llvm::StringRef lhs,
                        llvm::StringRef rhs, llvm::StringRef type) {
    std::string result = newValue();
    bool isCompare =
        operation == "eq" || operation == "ult" || operation == "slt";
    body << "    " << result << " = pyc."
         << (isCompare ? "cmp" : operation.str()) << ' ' << lhs.str() << ", "
         << rhs.str();
    if (isCompare)
      body << " {predicate = \"" << operation.str() << "\"}";
    body << " : " << type.str() << ", " << type.str() << " -> "
         << (isCompare ? "i1" : type.str()) << "\n";
    return result;
  };
  auto emitNot = [&](llvm::StringRef value) {
    std::string result = newValue();
    body << "    " << result << " = pyc.not " << value.str() << " : i1\n";
    return result;
  };
  auto emitMux = [&](llvm::StringRef select, llvm::StringRef trueValue,
                     llvm::StringRef falseValue, llvm::StringRef type) {
    std::string result = newValue();
    body << "    " << result << " = pyc.select " << select.str() << ", "
         << trueValue.str() << ", " << falseValue.str() << " : i1, "
         << type.str() << ", " << type.str() << " -> " << type.str() << "\n";
    return result;
  };
  auto emitExtract = [&](llvm::StringRef value, uint64_t lsb,
                         llvm::StringRef inputType,
                         llvm::StringRef resultType) {
    std::string result = newValue();
    body << "    " << result << " = pyc.extract " << value.str()
         << " {lsb = " << lsb << "} : " << inputType.str() << " -> "
         << resultType.str() << "\n";
    return result;
  };
  auto emitFieldReplace =
      [&](llvm::StringRef record, llvm::StringRef recordType,
          llvm::StringRef field,
          llvm::StringRef replacement) -> llvm::Expected<std::string> {
    auto layout = fieldLayout(plan, recordType, field);
    auto totalWidth = typeWidth(plan, recordType);
    if (!layout)
      return layout.takeError();
    if (!totalWidth)
      return totalWidth.takeError();
    std::vector<std::pair<std::string, unsigned>> parts;
    const unsigned highWidth = *totalWidth - layout->lsb - layout->width;
    if (highWidth > 0)
      parts.emplace_back(emitExtract(record, layout->lsb + layout->width,
                                     "i" + std::to_string(*totalWidth),
                                     "i" + std::to_string(highWidth)),
                         highWidth);
    parts.emplace_back(replacement.str(), layout->width);
    if (layout->lsb > 0)
      parts.emplace_back(emitExtract(record, 0,
                                     "i" + std::to_string(*totalWidth),
                                     "i" + std::to_string(layout->lsb)),
                         layout->lsb);
    if (parts.size() == 1)
      return parts.front().first;
    std::string result = newValue();
    body << "    " << result << " = pyc.concat(";
    for (auto [index, part] : llvm::enumerate(parts)) {
      if (index)
        body << ", ";
      body << part.first;
    }
    body << ") : (";
    for (auto [index, part] : llvm::enumerate(parts)) {
      if (index)
        body << ", ";
      body << 'i' << part.second;
    }
    body << ") -> i" << *totalWidth << "\n";
    return result;
  };
  auto reduceBalanced = [&](llvm::StringRef operation,
                            std::vector<std::string> values,
                            llvm::StringRef type) {
    while (values.size() > 1) {
      std::vector<std::string> next;
      next.reserve((values.size() + 1) / 2);
      for (size_t index = 0; index < values.size(); index += 2) {
        if (index + 1 == values.size())
          next.push_back(values[index]);
        else
          next.push_back(
              emitBinary(operation, values[index], values[index + 1], type));
      }
      values = std::move(next);
    }
    return values.front();
  };
  auto selectBalanced = [&](std::vector<std::string> valids,
                            std::vector<std::string> values,
                            llvm::StringRef type) {
    while (values.size() > 1) {
      std::vector<std::string> nextValids;
      std::vector<std::string> nextValues;
      nextValids.reserve((values.size() + 1) / 2);
      nextValues.reserve((values.size() + 1) / 2);
      for (size_t index = 0; index < values.size(); index += 2) {
        if (index + 1 == values.size()) {
          nextValids.push_back(valids[index]);
          nextValues.push_back(values[index]);
        } else {
          nextValues.push_back(
              emitMux(valids[index], values[index], values[index + 1], type));
          nextValids.push_back(
              emitBinary("or", valids[index], valids[index + 1], "i1"));
        }
      }
      valids = std::move(nextValids);
      values = std::move(nextValues);
    }
    return std::pair<std::string, std::string>{valids.front(), values.front()};
  };
  auto emitTableLookup =
      [&](const TablePlan &table, llvm::StringRef index,
          llvm::StringRef indexType) -> llvm::Expected<std::string> {
    auto state = tableStates.find(table.name);
    if (state == tableStates.end() || state->getValue().value.empty())
      return pycError("Table lookup has no register-bank state");
    std::string result = state->getValue().value.back();
    for (size_t slot = table.entries; slot-- > 0;) {
      if (!pycIntegerCanRepresent(slot, indexType))
        continue;
      std::string slotValue = emitConstant(slot, indexType);
      std::string selected = emitBinary("eq", index, slotValue, indexType);
      result = emitMux(selected, state->getValue().value[slot], result,
                       state->getValue().type);
    }
    return result;
  };
  auto emitTableFieldMerge =
      [&](llvm::StringRef current, llvm::StringRef proposed,
          llvm::StringRef entryType,
          llvm::ArrayRef<std::string> fields) -> llvm::Expected<std::string> {
    if (fields.size() == 1 && fields.front() == "$entry")
      return proposed.str();
    std::string merged = current.str();
    for (const std::string &field : fields) {
      auto layout = fieldLayout(plan, entryType, field);
      auto packedType = pycType(plan, entryType);
      if (!layout)
        return layout.takeError();
      if (!packedType)
        return packedType.takeError();
      std::string replacement =
          emitExtract(proposed, layout->lsb, *packedType,
                      "i" + std::to_string(layout->width));
      auto updated = emitFieldReplace(merged, entryType, field, replacement);
      if (!updated)
        return updated.takeError();
      merged = std::move(*updated);
    }
    return merged;
  };
  for (const TablePlan &table : plan.tables) {
    auto type = pycType(plan, table.entryType);
    if (!type)
      return type.takeError();
    TableState state;
    state.type = *type;
    for (uint64_t index = 0; index < table.entries; ++index) {
      std::string initial;
      if (table.initImage.empty()) {
        initial = emitConstant(table.init, state.type);
      } else {
        auto packed = packTableInitValue(plan, table.initImage[index]);
        if (!packed)
          return packed.takeError();
        initial = newValue();
        body << "    " << initial << " = pyc.constant "
             << unsignedDecimal(*packed) << " : " << state.type << "\n";
      }
      state.next.push_back(newValue());
      state.enable.push_back(newValue());
      state.value.push_back(newValue());
      body << "    " << state.next.back() << " = pyc.wire : " << state.type
           << "\n";
      body << "    " << state.enable.back() << " = pyc.wire : i1\n";
      body << "    " << state.value.back() << " = pyc.reg %clk, %rst, "
           << state.enable.back() << ", " << state.next.back() << ", "
           << initial << " : " << state.type << "\n";
    }
    tableStateValues[table.name] = state.value;
    tableStates[table.name] = std::move(state);
  }
  for (const QueuePlan &queue : plan.queues) {
    std::string ready = newValue();
    readyWires[queue.name] = ready;
    body << "    " << ready << " = pyc.wire : i1\n";
  }
  for (const QueuePlan &queue : plan.queues) {
    std::string producerValid;
    std::string producerData;
    auto source = sourceBoundary.find(queue.name);
    if (source != sourceBoundary.end()) {
      producerValid = inputName(source->getValue(), "valid");
      producerData = inputName(source->getValue(), "data");
    } else {
      auto transformProducer = transformByOutput.find(queue.name);
      auto firingProducer = firingByOutput.find(queue.name);
      auto tableReadProducer = tableReadByOutput.find(queue.name);
      auto tableReadGroupProducer = tableReadGroupByOutput.find(queue.name);
      auto barrierProducer = barrierByOutput.find(queue.name);
      auto broadcastProducer = broadcastByOutput.find(queue.name);
      auto forkProducer = forkByOutput.find(queue.name);
      auto routeProducer = routeByOutput.find(queue.name);
      auto selectProducer = selectByOutput.find(queue.name);
      auto mergeProducer = mergeByOutput.find(queue.name);
      auto creditProducer = creditByOutput.find(queue.name);
      auto memoryProducer = memoryByOutput.find(queue.name);
      auto dependencyProducer = dependencyByOutput.find(queue.name);
      auto reorderProducer = reorderByOutput.find(queue.name);
      auto feedbackProducer = feedbackByOutput.find(queue.name);
      if (transformProducer != transformByOutput.end()) {
        const TransformProducer &producer = transformProducer->getValue();
        const QueueBlockPlan &transform = *producer.block;
        std::vector<std::string> inputDataValues;
        std::vector<std::string> inputTypes;
        std::string allValid;
        for (const std::string &inputName : transform.inputs) {
          auto valid = outputValid.find(inputName);
          auto data = outputData.find(inputName);
          const QueuePlan *inputQueue = findQueue(plan, inputName);
          if (valid == outputValid.end() || data == outputData.end() ||
              !inputQueue)
            return pycError("Queue transforms are not in topological order");
          allValid = allValid.empty()
                         ? valid->getValue()
                         : emitBinary("and", allValid, valid->getValue(), "i1");
          inputDataValues.push_back(data->getValue());
          inputTypes.push_back(inputQueue->payloadType);
        }
        if (transform.inputs.size() == 1 && transform.outputs.size() == 1) {
          producerValid = allValid;
        } else {
          producerValid = newValue();
          body << "    " << producerValid << " = pyc.wire : i1\n";
          atomicTransformValid[queue.name] = producerValid;
        }
        auto transformed =
            emitTransform(plan, transform, inputDataValues, inputTypes,
                          producer.index, nextValue, body);
        if (!transformed)
          return transformed.takeError();
        producerData = std::move(*transformed);
      } else if (firingProducer != firingByOutput.end()) {
        const TransformProducer &producer = firingProducer->getValue();
        const QueueBlockPlan &firing = *producer.block;
        std::vector<std::string> inputDataValues;
        std::vector<std::string> inputTypes;
        for (const std::string &inputName : firing.inputs) {
          auto valid = outputValid.find(inputName);
          auto data = outputData.find(inputName);
          const QueuePlan *inputQueue = findQueue(plan, inputName);
          if (valid == outputValid.end() || data == outputData.end() ||
              !inputQueue)
            return pycError("firing inputs are not in topological order");
          inputDataValues.push_back(data->getValue());
          inputTypes.push_back(inputQueue->payloadType);
        }
        producerValid = newValue();
        body << "    " << producerValid << " = pyc.wire : i1\n";
        atomicTransformValid[queue.name] = producerValid;
        auto cached = firingExpressionValues.find(queue.name);
        if (cached == firingExpressionValues.end()) {
          auto values = std::make_shared<llvm::StringMap<std::string>>();
          auto emitted =
              emitTransform(plan, firing, inputDataValues, inputTypes,
                            producer.index, nextValue, body, values.get(),
                            &tableStateValues, &roundRobinStates, firing.name,
                            nullptr, nullptr, &sharedTableExpressionValues);
          if (!emitted)
            return emitted.takeError();
          for (const std::string &output : firing.outputs)
            firingExpressionValues[output] = values;
          firingExpressionValuesByBlock[firing.name] = values;
          cached = firingExpressionValues.find(queue.name);
        }
        auto lookup =
            [&](llvm::StringRef identity) -> llvm::Expected<std::string> {
          auto found = cached->getValue()->find(identity);
          if (found == cached->getValue()->end())
            return pycError("firing expression identity is missing: '" +
                            identity + "'");
          return found->getValue();
        };
        auto transformed = lookup(firing.yields[producer.index]);
        auto presence = lookup(firing.outputPresence[producer.index].present);
        auto guard = lookup(firing.guard);
        if (!transformed)
          return transformed.takeError();
        if (!presence)
          return presence.takeError();
        if (!guard)
          return guard.takeError();
        producerData = std::move(*transformed);
        firingPresence[queue.name] = std::move(*presence);
        firingGuard[queue.name] = std::move(*guard);
        firingGuardByBlock[firing.name] = firingGuard[queue.name];
      } else if (tableReadProducer != tableReadByOutput.end()) {
        const QueueBlockPlan &read = *tableReadProducer->getValue();
        const TablePlan *table = nullptr;
        for (const TablePlan &candidate : plan.tables)
          if (candidate.name == read.table) {
            table = &candidate;
            break;
          }
        if (!table)
          return pycError("Table read references unknown register bank");
        std::vector<std::string> inputDataValues;
        std::vector<std::string> inputTypes;
        std::string allValid = emitConstant(1, "i1");
        for (const std::string &input : read.inputs) {
          auto valid = outputValid.find(input);
          auto data = outputData.find(input);
          const QueuePlan *inputQueue = findQueue(plan, input);
          if (valid == outputValid.end() || data == outputData.end() ||
              !inputQueue)
            return pycError("Table read input is not in topological order");
          allValid = emitBinary("and", allValid, valid->getValue(), "i1");
          inputDataValues.push_back(data->getValue());
          inputTypes.push_back(inputQueue->payloadType);
        }
        auto values = std::make_shared<llvm::StringMap<std::string>>();
        auto index =
            emitTransform(plan, read, inputDataValues, inputTypes, 0, nextValue,
                          body, values.get(), &tableStateValues, nullptr, {},
                          nullptr, nullptr, &sharedTableExpressionValues);
        if (!index)
          return index.takeError();
        auto present = values->find(read.yields[1]);
        if (present == values->end())
          return pycError("Table read predicate value is missing");
        auto indexType = yieldedType(read, read.yields.front(),
                                     read.inputs.empty() ? table->entryType
                                                         : inputTypes.front());
        auto indexPycType =
            indexType ? pycType(plan, *indexType)
                      : llvm::Expected<std::string>(indexType.takeError());
        if (!indexPycType)
          return indexPycType.takeError();
        auto data = emitTableLookup(*table, *index, *indexPycType);
        if (!data)
          return data.takeError();
        producerValid = emitBinary("and", allValid, present->getValue(), "i1");
        producerData = std::move(*data);
        tableReadWhen[read.name] = present->getValue();
      } else if (tableReadGroupProducer != tableReadGroupByOutput.end()) {
        const TransformProducer &producer = tableReadGroupProducer->getValue();
        const QueueBlockPlan &group = *producer.block;
        const TablePlan *table = nullptr;
        const TableSelectionPlan *selection = nullptr;
        const TableMatchPlan *match = nullptr;
        for (const TablePlan &candidate : plan.tables)
          if (candidate.name == group.table) {
            table = &candidate;
            break;
          }
        for (const TableSelectionPlan &candidate : plan.tableSelections)
          if (candidate.name == group.selection) {
            selection = &candidate;
            break;
          }
        if (selection)
          for (const TableMatchPlan &candidate : plan.tableMatches)
            if (candidate.name == selection->match) {
              match = &candidate;
              break;
            }
        if (!table || !selection || !match ||
            selection->count != group.outputs.size())
          return pycError("grouped Table read selection is incomplete");
        auto state = tableReadGroupStates.find(group.name);
        if (state == tableReadGroupStates.end()) {
          QueueBlockPlan evaluation;
          QueueExpressionPlan mask;
          mask.result = "__pyc_match_" + group.name;
          mask.kind = "table_match";
          mask.type = match->resultType;
          mask.table = match->table;
          mask.nestedExpressions = match->expressions;
          mask.nestedYields = {match->yield};
          mask.domainAxes = match->domainAxes;
          mask.domainShape = match->domainShape;
          mask.domainStrides = match->domainStrides;
          mask.domainOffset = match->domainOffset;
          mask.hasDomainProjection = match->hasDomainProjection;
          evaluation.expressions.push_back(mask);
          std::vector<std::string> indexNames;
          std::vector<std::string> validNames;
          const std::string choiceIdentity = selection->stableId.empty()
                                                 ? selection->name
                                                 : selection->stableId;
          for (uint64_t lane = 0; lane < selection->count; ++lane) {
            QueueExpressionPlan index;
            index.result =
                "__pyc_choice_index_" + std::to_string(lane) + "_" + group.name;
            index.kind = "table_choose_index";
            index.type = selection->indexType;
            index.operands = {mask.result};
            index.field = choiceIdentity;
            index.predicate = selection->policy;
            index.table = selection->table;
            index.nestedExpressions = selection->keyExpressions;
            if (!selection->keyYield.empty())
              index.nestedYields = {selection->keyYield};
            index.selectionCount = selection->count;
            index.laneOrdinal = lane;
            index.keyOrdering = selection->keyOrdering;
            index.initialCursor = selection->initialCursor;
            indexNames.push_back(index.result);
            evaluation.expressions.push_back(std::move(index));
          }
          for (uint64_t lane = 0; lane < selection->count; ++lane) {
            QueueExpressionPlan valid;
            valid.result =
                "__pyc_choice_valid_" + std::to_string(lane) + "_" + group.name;
            valid.kind = "table_choose_valid";
            valid.type = "i1";
            valid.operands = {mask.result};
            valid.field = choiceIdentity;
            valid.predicate = selection->policy;
            valid.table = selection->table;
            valid.nestedExpressions = selection->keyExpressions;
            if (!selection->keyYield.empty())
              valid.nestedYields = {selection->keyYield};
            valid.selectionCount = selection->count;
            valid.laneOrdinal = lane;
            valid.keyOrdering = selection->keyOrdering;
            valid.initialCursor = selection->initialCursor;
            validNames.push_back(valid.result);
            evaluation.expressions.push_back(std::move(valid));
          }
          evaluation.yields = indexNames;
          evaluation.yields.insert(evaluation.yields.end(), validNames.begin(),
                                   validNames.end());
          llvm::StringMap<std::string> emittedValues;
          auto emitted = emitTransform(plan, evaluation, {}, {}, 0, nextValue,
                                       body, &emittedValues, &tableStateValues,
                                       &roundRobinStates, group.name, nullptr,
                                       nullptr, &sharedTableExpressionValues);
          if (!emitted)
            return emitted.takeError();
          TableReadGroupState created;
          for (const std::string &name : indexNames)
            created.indices.push_back(emittedValues.lookup(name));
          for (const std::string &name : validNames)
            created.valids.push_back(emittedValues.lookup(name));
          tableReadGroupStates[group.name] = std::move(created);
          state = tableReadGroupStates.find(group.name);
        }
        producerValid = newValue();
        body << "    " << producerValid << " = pyc.wire : i1\n";
        tableReadGroupValidWires[queue.name] = producerValid;
        auto data =
            emitTableLookup(*table, state->getValue().indices[producer.index],
                            selection->indexType);
        if (!data)
          return data.takeError();
        producerData = std::move(*data);
      } else if (barrierProducer != barrierByOutput.end()) {
        const TransformProducer &producer = barrierProducer->getValue();
        const QueueBlockPlan &barrier = *producer.block;
        for (const std::string &inputName : barrier.inputs) {
          auto valid = outputValid.find(inputName);
          if (valid == outputValid.end())
            return pycError(
                "barrier input is not available in topological order");
        }
        auto data = outputData.find(barrier.inputs[producer.index]);
        if (data == outputData.end())
          return pycError("barrier input data is missing");
        producerValid = newValue();
        body << "    " << producerValid << " = pyc.wire : i1\n";
        atomicTransformValid[queue.name] = producerValid;
        producerData = data->getValue();
      } else if (broadcastProducer != broadcastByOutput.end()) {
        const QueueBlockPlan &broadcast = *broadcastProducer->getValue();
        auto valid = outputValid.find(broadcast.inputs.front());
        auto data = outputData.find(broadcast.inputs.front());
        if (valid == outputValid.end() || data == outputData.end())
          return pycError(
              "broadcast input is not available in topological order");
        producerValid = valid->getValue();
        producerData = data->getValue();
      } else if (forkProducer != forkByOutput.end()) {
        const QueueBlockPlan &fork = *forkProducer->getValue();
        auto valid = outputValid.find(fork.inputs.front());
        auto data = outputData.find(fork.inputs.front());
        if (valid == outputValid.end() || data == outputData.end())
          return pycError("fork input is not available in topological order");
        auto state = forkStates.find(fork.name);
        if (state == forkStates.end()) {
          ForkState created;
          std::string zero = emitConstant(0, "i1");
          for (size_t index = 0; index < fork.outputs.size(); ++index) {
            std::string nextWire = newValue();
            std::string enableWire = newValue();
            std::string delivered = newValue();
            body << "    " << nextWire << " = pyc.wire : i1\n";
            body << "    " << enableWire << " = pyc.wire : i1\n";
            body << "    " << delivered << " = pyc.reg %clk, %rst, "
                 << enableWire << ", " << nextWire << ", " << zero << " : i1\n";
            created.nextWires.push_back(std::move(nextWire));
            created.enableWires.push_back(std::move(enableWire));
            created.delivered.push_back(std::move(delivered));
          }
          forkStates[fork.name] = std::move(created);
          state = forkStates.find(fork.name);
        }
        auto output =
            std::find(fork.outputs.begin(), fork.outputs.end(), queue.name);
        if (output == fork.outputs.end())
          return pycError("fork output identity is missing");
        const size_t index = std::distance(fork.outputs.begin(), output);
        std::string notDelivered = emitNot(state->getValue().delivered[index]);
        producerValid =
            emitBinary("and", valid->getValue(), notDelivered, "i1");
        forkOfferValid[queue.name] = producerValid;
        producerData = data->getValue();
      } else if (routeProducer != routeByOutput.end()) {
        const RouteProducer &producer = routeProducer->getValue();
        const QueueBlockPlan &route = *producer.block;
        auto valid = outputValid.find(route.inputs.front());
        auto data = outputData.find(route.inputs.front());
        const QueuePlan *inputQueue = findQueue(plan, route.inputs.front());
        if (valid == outputValid.end() || data == outputData.end() ||
            !inputQueue)
          return pycError("route input is not available in topological order");
        auto selector = routeSelector.find(route.name);
        if (selector == routeSelector.end()) {
          auto selected =
              emitTransform(plan, route, {data->getValue()},
                            {inputQueue->payloadType}, 0, nextValue, body);
          if (!selected)
            return selected.takeError();
          routeSelector[route.name] = *selected;
          selector = routeSelector.find(route.name);
        }
        auto selectorType =
            yieldedType(route, route.yields.front(), inputQueue->payloadType);
        if (!selectorType)
          return selectorType.takeError();
        auto selectorPycType = pycType(plan, *selectorType);
        if (!selectorPycType)
          return selectorPycType.takeError();
        std::string index = emitConstant(producer.index, *selectorPycType);
        std::string condition =
            emitBinary("eq", selector->getValue(), index, *selectorPycType);
        routeCondition[queue.name] = condition;
        producerValid = emitBinary("and", valid->getValue(), condition, "i1");
        producerData = data->getValue();
      } else if (selectProducer != selectByOutput.end()) {
        const QueueBlockPlan &select = *selectProducer->getValue();
        auto controlValid = outputValid.find(select.inputs.front());
        auto controlData = outputData.find(select.inputs.front());
        const QueuePlan *controlQueue = findQueue(plan, select.inputs.front());
        if (controlValid == outputValid.end() ||
            controlData == outputData.end() || !controlQueue)
          return pycError(
              "select control is not available in topological order");
        auto selector =
            emitTransform(plan, select, {controlData->getValue()},
                          {controlQueue->payloadType}, 0, nextValue, body);
        if (!selector)
          return selector.takeError();
        auto selectorType = yieldedType(select, select.yields.front(),
                                        controlQueue->payloadType);
        if (!selectorType)
          return selectorType.takeError();
        auto selectorPycType = pycType(plan, *selectorType);
        if (!selectorPycType)
          return selectorPycType.takeError();
        auto outputType = pycType(plan, queue.payloadType);
        if (!outputType)
          return outputType.takeError();

        SelectState state;
        state.controlValid = controlValid->getValue();
        std::vector<std::string> selectedValidTerms;
        std::vector<std::string> dataValues;
        for (size_t index = 1; index < select.inputs.size(); ++index) {
          auto valid = outputValid.find(select.inputs[index]);
          auto data = outputData.find(select.inputs[index]);
          if (valid == outputValid.end() || data == outputData.end())
            return pycError(
                "select data input is not available in topological order");
          std::string indexValue = emitConstant(index - 1, *selectorPycType);
          std::string condition =
              emitBinary("eq", *selector, indexValue, *selectorPycType);
          state.conditions.push_back(condition);
          selectedValidTerms.push_back(
              emitBinary("and", valid->getValue(), condition, "i1"));
          dataValues.push_back(data->getValue());
        }
        state.selectedValid = reduceBalanced("or", selectedValidTerms, "i1");
        std::string selectedData = dataValues.back();
        for (size_t index = dataValues.size() - 1; index-- > 0;)
          selectedData = emitMux(state.conditions[index], dataValues[index],
                                 selectedData, *outputType);
        std::string anyCondition = reduceBalanced("or", state.conditions, "i1");
        std::string invalidSelector =
            emitBinary("and", state.controlValid, emitNot(anyCondition), "i1");
        state.selectorSafe = emitNot(invalidSelector);
        producerValid =
            emitBinary("and", state.controlValid, state.selectedValid, "i1");
        producerData = std::move(selectedData);
        selectStates[select.name] = std::move(state);
      } else if (mergeProducer != mergeByOutput.end()) {
        const QueueBlockPlan &merge = *mergeProducer->getValue();
        std::vector<std::string> valids;
        std::vector<std::string> dataValues;
        for (const std::string &input : merge.inputs) {
          auto valid = outputValid.find(input);
          auto data = outputData.find(input);
          if (valid == outputValid.end() || data == outputData.end())
            return pycError(
                "merge input is not available in topological order");
          valids.push_back(valid->getValue());
          dataValues.push_back(data->getValue());
        }
        std::string any = valids.front();
        for (size_t index = 1; index < valids.size(); ++index) {
          any = emitBinary("or", any, valids[index], "i1");
        }
        std::vector<std::string> grants;
        if (merge.policy == "priority") {
          grants.push_back(valids.front());
          std::string prior = valids.front();
          for (size_t index = 1; index < valids.size(); ++index) {
            std::string notPrior = emitNot(prior);
            grants.push_back(emitBinary("and", valids[index], notPrior, "i1"));
            prior = emitBinary("or", prior, valids[index], "i1");
          }
        } else {
          unsigned pointerWidth = 1;
          while ((uint64_t{1} << pointerWidth) < valids.size())
            ++pointerWidth;
          std::string pointerType = "i" + std::to_string(pointerWidth);
          std::string nextWire = newValue();
          std::string enableWire = newValue();
          body << "    " << nextWire << " = pyc.wire : " << pointerType << "\n";
          body << "    " << enableWire << " = pyc.wire : i1\n";
          std::string zeroPointer = emitConstant(0, pointerType);
          std::string cursor = newValue();
          body << "    " << cursor << " = pyc.reg %clk, %rst, " << enableWire
               << ", " << nextWire << ", " << zeroPointer << " : "
               << pointerType << "\n";
          std::string zeroGrant = emitConstant(0, "i1");
          std::vector<std::vector<std::string>> grantsByCursor;
          grantsByCursor.reserve(valids.size());
          for (size_t start = 0; start < valids.size(); ++start) {
            std::vector<std::string> cursorGrants(valids.size(), zeroGrant);
            std::string prior = zeroGrant;
            for (size_t offset = 0; offset < valids.size(); ++offset) {
              const size_t input = (start + offset) % valids.size();
              cursorGrants[input] =
                  emitBinary("and", valids[input], emitNot(prior), "i1");
              prior = emitBinary("or", prior, valids[input], "i1");
            }
            grantsByCursor.push_back(std::move(cursorGrants));
          }
          grants.reserve(valids.size());
          for (size_t input = 0; input < valids.size(); ++input) {
            std::string selected = zeroGrant;
            for (size_t start = valids.size(); start-- > 0;) {
              std::string startValue = emitConstant(start, pointerType);
              std::string atStart =
                  emitBinary("eq", cursor, startValue, pointerType);
              selected = emitMux(atStart, grantsByCursor[start][input],
                                 selected, "i1");
            }
            std::string qualified = newValue();
            body << "    " << qualified << " = pyc.alias " << selected
                 << " {num_inputs = " << valids.size() << ", lane = " << input
                 << ", primitive_id = \"control.rr_arbiter.v1\", "
                    "implementation_id = \"internal.reference.rr_arbiter.v1\", "
                    "qualification_report = "
                    "\"DF-09/smoke_v072/arbiter_candidates\"} : i1\n";
            grants.push_back(std::move(qualified));
          }
          mergeStates[merge.name] =
              MergeState{nextWire, enableWire, cursor, any, pointerType};
        }
        auto outputType = pycType(plan, queue.payloadType);
        if (!outputType)
          return outputType.takeError();
        std::string selectedData = dataValues.back();
        for (size_t index = dataValues.size() - 1; index-- > 0;)
          selectedData = emitMux(grants[index], dataValues[index], selectedData,
                                 *outputType);
        mergeGrants[merge.name] = grants;
        producerValid = any;
        producerData = selectedData;
      } else if (creditProducer != creditByOutput.end()) {
        const QueueBlockPlan &credit = *creditProducer->getValue();
        auto inputValidValue = outputValid.find(credit.inputs.front());
        auto inputDataValue = outputData.find(credit.inputs.front());
        const QueuePlan *inputQueue = findQueue(plan, credit.inputs.front());
        if (inputValidValue == outputValid.end() ||
            inputDataValue == outputData.end() || !inputQueue)
          return pycError("credit input is not available in topological order");
        auto dataType = pycType(plan, inputQueue->payloadType);
        auto dataWidth = typeWidth(plan, inputQueue->payloadType);
        if (!dataType)
          return dataType.takeError();
        if (!dataWidth)
          return dataWidth.takeError();
        auto costValue =
            emitTransform(plan, credit, {inputDataValue->getValue()},
                          {inputQueue->payloadType}, 0, nextValue, body);
        if (!costValue)
          return costValue.takeError();
        auto costType =
            yieldedType(credit, credit.yields.front(), inputQueue->payloadType);
        if (!costType)
          return costType.takeError();
        auto costPycType = pycType(plan, *costType);
        auto costWidth = typeWidth(plan, *costType);
        if (!costPycType)
          return costPycType.takeError();
        if (!costWidth)
          return costWidth.takeError();
        if (*costWidth == 0 || *costWidth > 64)
          return pycError("credit cost width is unsupported");

        CreditState state;
        state.inputCost = std::move(*costValue);
        state.costType = std::move(*costPycType);
        state.dataType = *dataType;
        state.slotType = "i" + std::to_string(1 + *costWidth + *dataWidth);
        std::string zeroSlot = emitConstant(0, state.slotType);
        std::string zeroCost = emitConstant(0, state.costType);
        std::vector<std::string> freeValues;
        std::vector<std::string> doneValues;
        std::vector<std::string> dataValues;
        for (uint64_t index = 0; index < credit.credits; ++index) {
          CreditSlotState slot;
          slot.next = newValue();
          slot.enable = newValue();
          slot.state = newValue();
          body << "    " << slot.next << " = pyc.wire : " << state.slotType
               << "\n";
          body << "    " << slot.enable << " = pyc.wire : i1\n";
          body << "    " << slot.state << " = pyc.reg %clk, %rst, "
               << slot.enable << ", " << slot.next << ", " << zeroSlot << " : "
               << state.slotType << "\n";
          slot.valid = emitExtract(slot.state, *costWidth + *dataWidth,
                                   state.slotType, "i1");
          slot.remaining = emitExtract(slot.state, *dataWidth, state.slotType,
                                       state.costType);
          slot.data =
              emitExtract(slot.state, 0, state.slotType, state.dataType);
          slot.free = emitNot(slot.valid);
          std::string atZero =
              emitBinary("eq", slot.remaining, zeroCost, state.costType);
          slot.done = emitBinary("and", slot.valid, atZero, "i1");
          freeValues.push_back(slot.free);
          doneValues.push_back(slot.done);
          dataValues.push_back(slot.data);
          state.slots.push_back(std::move(slot));
        }
        unsigned indexWidth = 1;
        while ((uint64_t{1} << indexWidth) < credit.credits)
          ++indexWidth;
        state.freeIndexType = "i" + std::to_string(indexWidth);
        std::vector<std::string> indices;
        indices.reserve(credit.credits);
        for (uint64_t index = 0; index < credit.credits; ++index)
          indices.push_back(emitConstant(index, state.freeIndexType));
        auto selectedFree =
            selectBalanced(freeValues, indices, state.freeIndexType);
        state.anyFree = std::move(selectedFree.first);
        state.freeIndex = std::move(selectedFree.second);
        auto selectedDoneIndex =
            selectBalanced(doneValues, indices, state.freeIndexType);
        state.doneIndex = std::move(selectedDoneIndex.second);
        auto selectedDoneData =
            selectBalanced(doneValues, dataValues, state.dataType);
        state.outputDone = std::move(selectedDoneData.first);
        std::string selectedData = std::move(selectedDoneData.second);
        std::string invalidCost =
            emitBinary("eq", state.inputCost, zeroCost, state.costType);
        std::string invalidAdmission =
            emitBinary("and", inputValidValue->getValue(), invalidCost, "i1");
        state.safeAdmission = emitNot(invalidAdmission);
        producerValid = state.outputDone;
        producerData = std::move(selectedData);
        creditStates[credit.name] = std::move(state);
      } else if (memoryProducer != memoryByOutput.end()) {
        const QueueBlockPlan &memory = *memoryProducer->getValue();
        const QueuePlan *inputQueue = findQueue(plan, memory.inputs.front());
        if (!inputQueue)
          return pycError("memory input Queue is missing");
        auto requestType = pycType(plan, inputQueue->payloadType);
        if (!requestType)
          return requestType.takeError();
        producerValid = newValue();
        producerData = newValue();
        body << "    " << producerValid << " = pyc.wire : i1\n";
        body << "    " << producerData << " = pyc.wire : " << *requestType
             << "\n";
        memoryResponseValid[memory.outputs.front()] = producerValid;
        memoryResponseData[memory.outputs.front()] = producerData;
      } else if (dependencyProducer != dependencyByOutput.end()) {
        const QueueBlockPlan &dependency = *dependencyProducer->getValue();
        if (dependency.provider == "v2")
          return pycError(
              "schedule v2 PYC provider requires issue #21 lane lowering");
        auto inputValidValue = outputValid.find(dependency.inputs.front());
        auto inputDataValue = outputData.find(dependency.inputs.front());
        const QueuePlan *inputQueue =
            findQueue(plan, dependency.inputs.front());
        if (inputValidValue == outputValid.end() ||
            inputDataValue == outputData.end() || !inputQueue)
          return pycError(
              "dependency input is not available in topological order");
        auto dataType = pycType(plan, inputQueue->payloadType);
        auto dataWidth = typeWidth(plan, inputQueue->payloadType);
        if (!dataType)
          return dataType.takeError();
        if (!dataWidth)
          return dataWidth.takeError();

        std::vector<std::string> policyValues;
        std::vector<std::string> policyTypes;
        std::vector<unsigned> policyWidths;
        for (size_t index = 0; index < dependency.yields.size(); ++index) {
          auto value =
              emitTransform(plan, dependency, {inputDataValue->getValue()},
                            {inputQueue->payloadType}, index, nextValue, body);
          if (!value)
            return value.takeError();
          auto type = yieldedType(dependency, dependency.yields[index],
                                  inputQueue->payloadType);
          if (!type)
            return type.takeError();
          auto pycType = ::acir::codegen::pycType(plan, *type);
          auto width = typeWidth(plan, *type);
          if (!pycType)
            return pycType.takeError();
          if (!width)
            return width.takeError();
          if (*width == 0 || *width > 64)
            return pycError("dependency policy width is unsupported");
          policyValues.push_back(std::move(*value));
          policyTypes.push_back(std::move(*pycType));
          policyWidths.push_back(*width);
        }
        if (policyTypes[0] != policyTypes[1])
          return pycError("dependency key/predecessor types must match");
        if (policyWidths[1] < 64 &&
            dependency.noDependency >= (uint64_t{1} << policyWidths[1]))
          return pycError("dependency sentinel does not fit predecessor type");
        if (policyWidths[2] < 64 &&
            dependency.resources > (uint64_t{1} << policyWidths[2]))
          return pycError("dependency resources do not fit resource type");

        DependencyState state;
        state.inputKey = policyValues[0];
        state.inputPredecessor = policyValues[1];
        state.inputResource = policyValues[2];
        state.inputCost = policyValues[3];
        state.keyType = policyTypes[0];
        state.resourceType = policyTypes[2];
        state.costType = policyTypes[3];
        state.dataType = *dataType;
        state.noDependency = dependency.noDependency;
        state.resources = dependency.resources;
        const uint64_t slotWidth = 1 + 2 + 2 * policyWidths[0] +
                                   policyWidths[2] + 2 * policyWidths[3] +
                                   *dataWidth;
        state.slotType = "i" + std::to_string(slotWidth);
        std::string zeroSlot = emitConstant(0, state.slotType);
        std::string donePhase = emitConstant(2, "i2");

        const uint64_t costLsb = *dataWidth;
        const uint64_t remainingLsb = costLsb + policyWidths[3];
        const uint64_t resourceLsb = remainingLsb + policyWidths[3];
        const uint64_t predecessorLsb = resourceLsb + policyWidths[2];
        const uint64_t keyLsb = predecessorLsb + policyWidths[0];
        const uint64_t phaseLsb = keyLsb + policyWidths[0];
        const uint64_t validLsb = phaseLsb + 2;
        std::vector<std::string> freeValues;
        std::vector<std::string> doneValues;
        std::vector<std::string> dataValues;
        std::vector<std::string> duplicateValues;
        for (uint64_t index = 0; index < dependency.capacity; ++index) {
          DependencySlotState slot;
          slot.next = newValue();
          slot.enable = newValue();
          slot.state = newValue();
          body << "    " << slot.next << " = pyc.wire : " << state.slotType
               << "\n";
          body << "    " << slot.enable << " = pyc.wire : i1\n";
          body << "    " << slot.state << " = pyc.reg %clk, %rst, "
               << slot.enable << ", " << slot.next << ", " << zeroSlot << " : "
               << state.slotType << "\n";
          slot.valid = emitExtract(slot.state, validLsb, state.slotType, "i1");
          slot.phase = emitExtract(slot.state, phaseLsb, state.slotType, "i2");
          slot.key =
              emitExtract(slot.state, keyLsb, state.slotType, state.keyType);
          slot.predecessor = emitExtract(slot.state, predecessorLsb,
                                         state.slotType, state.keyType);
          slot.resource = emitExtract(slot.state, resourceLsb, state.slotType,
                                      state.resourceType);
          slot.remaining = emitExtract(slot.state, remainingLsb, state.slotType,
                                       state.costType);
          slot.cost =
              emitExtract(slot.state, costLsb, state.slotType, state.costType);
          slot.data =
              emitExtract(slot.state, 0, state.slotType, state.dataType);
          slot.free = emitNot(slot.valid);
          freeValues.push_back(slot.free);
          std::string isDone = emitBinary("eq", slot.phase, donePhase, "i2");
          slot.done = emitBinary("and", slot.valid, isDone, "i1");
          doneValues.push_back(slot.done);
          dataValues.push_back(slot.data);
          std::string sameInput =
              emitBinary("eq", slot.key, state.inputKey, state.keyType);
          duplicateValues.push_back(
              emitBinary("and", slot.valid, sameInput, "i1"));
          state.slots.push_back(std::move(slot));
        }

        unsigned indexWidth = 1;
        while ((uint64_t{1} << indexWidth) < dependency.capacity)
          ++indexWidth;
        state.freeIndexType = "i" + std::to_string(indexWidth);
        std::vector<std::string> indices;
        indices.reserve(dependency.capacity);
        for (uint64_t index = 0; index < dependency.capacity; ++index)
          indices.push_back(emitConstant(index, state.freeIndexType));
        auto selectedFree =
            selectBalanced(freeValues, indices, state.freeIndexType);
        state.anyFree = std::move(selectedFree.first);
        state.freeIndex = std::move(selectedFree.second);
        auto selectedDoneIndex =
            selectBalanced(doneValues, indices, state.freeIndexType);
        state.doneIndex = std::move(selectedDoneIndex.second);
        auto selectedDoneData =
            selectBalanced(doneValues, dataValues, state.dataType);
        state.outputDone = std::move(selectedDoneData.first);
        std::string selectedData = std::move(selectedDoneData.second);
        std::string duplicate = reduceBalanced("or", duplicateValues, "i1");
        std::string zeroCost = emitConstant(0, state.costType);
        std::string costIsZero =
            emitBinary("eq", state.inputCost, zeroCost, state.costType);
        std::string resourceInvalid = emitConstant(0, "i1");
        if (policyWidths[2] == 64 ||
            dependency.resources < (uint64_t{1} << policyWidths[2])) {
          std::string resourceLimit =
              emitConstant(dependency.resources, state.resourceType);
          std::string resourceValid = emitBinary(
              "ult", state.inputResource, resourceLimit, state.resourceType);
          resourceInvalid = emitNot(resourceValid);
        }
        std::string invalidInput =
            emitBinary("or", duplicate, costIsZero, "i1");
        invalidInput = emitBinary("or", invalidInput, resourceInvalid, "i1");
        std::string invalidAdmission =
            emitBinary("and", inputValidValue->getValue(), invalidInput, "i1");
        state.safeAdmission = emitNot(invalidAdmission);
        producerValid = state.outputDone;
        producerData = std::move(selectedData);
        dependencyStates[dependency.name] = std::move(state);
      } else if (reorderProducer != reorderByOutput.end()) {
        const QueueBlockPlan &reorder = *reorderProducer->getValue();
        auto inputValidValue = outputValid.find(reorder.inputs.front());
        auto inputDataValue = outputData.find(reorder.inputs.front());
        const QueuePlan *inputQueue = findQueue(plan, reorder.inputs.front());
        if (inputValidValue == outputValid.end() ||
            inputDataValue == outputData.end() || !inputQueue)
          return pycError(
              "reorder input is not available in topological order");
        auto dataType = pycType(plan, inputQueue->payloadType);
        if (!dataType)
          return dataType.takeError();
        auto keyValue =
            emitTransform(plan, reorder, {inputDataValue->getValue()},
                          {inputQueue->payloadType}, 0, nextValue, body);
        if (!keyValue)
          return keyValue.takeError();
        auto keyType = yieldedType(reorder, reorder.yields.front(),
                                   inputQueue->payloadType);
        if (!keyType)
          return keyType.takeError();
        auto keyPycType = pycType(plan, *keyType);
        auto keyWidth = typeWidth(plan, *keyType);
        auto dataWidth = typeWidth(plan, inputQueue->payloadType);
        if (!keyPycType)
          return keyPycType.takeError();
        if (!keyWidth)
          return keyWidth.takeError();
        if (!dataWidth)
          return dataWidth.takeError();
        if (*keyWidth == 0 || *keyWidth > 64 ||
            (*keyWidth < 64 && reorder.start >= (uint64_t{1} << *keyWidth)))
          return pycError("reorder start does not fit key type");

        ReorderState state;
        state.inputKey = std::move(*keyValue);
        state.keyType = *keyPycType;
        state.dataType = *dataType;
        state.slotType = "i" + std::to_string(1 + *keyWidth + *dataWidth);
        std::string zeroSlot = emitConstant(0, state.slotType);
        std::string initialExpected =
            emitConstant(reorder.start, state.keyType);
        state.expectedNext = newValue();
        state.expectedEnable = newValue();
        state.expected = newValue();
        body << "    " << state.expectedNext
             << " = pyc.wire : " << state.keyType << "\n";
        body << "    " << state.expectedEnable << " = pyc.wire : i1\n";
        body << "    " << state.expected << " = pyc.reg %clk, %rst, "
             << state.expectedEnable << ", " << state.expectedNext << ", "
             << initialExpected << " : " << state.keyType << "\n";

        std::vector<std::string> freeValues;
        std::vector<std::string> matchValues;
        std::vector<std::string> duplicateValues;
        std::vector<std::string> slotDataValues;
        for (uint64_t index = 0; index < reorder.capacity; ++index) {
          ReorderSlotState slot;
          slot.next = newValue();
          slot.enable = newValue();
          slot.state = newValue();
          body << "    " << slot.next << " = pyc.wire : " << state.slotType
               << "\n";
          body << "    " << slot.enable << " = pyc.wire : i1\n";
          body << "    " << slot.state << " = pyc.reg %clk, %rst, "
               << slot.enable << ", " << slot.next << ", " << zeroSlot << " : "
               << state.slotType << "\n";
          slot.valid = newValue();
          body << "    " << slot.valid << " = pyc.extract " << slot.state
               << " {lsb = " << (*keyWidth + *dataWidth)
               << "} : " << state.slotType << " -> i1\n";
          slot.key = newValue();
          body << "    " << slot.key << " = pyc.extract " << slot.state
               << " {lsb = " << *dataWidth << "} : " << state.slotType << " -> "
               << state.keyType << "\n";
          slot.data = newValue();
          body << "    " << slot.data << " = pyc.extract " << slot.state
               << " {lsb = 0} : " << state.slotType << " -> " << state.dataType
               << "\n";
          slot.free = emitNot(slot.valid);
          freeValues.push_back(slot.free);
          std::string expected =
              emitBinary("eq", slot.key, state.expected, state.keyType);
          slot.match = emitBinary("and", slot.valid, expected, "i1");
          matchValues.push_back(slot.match);
          slotDataValues.push_back(slot.data);
          std::string sameInput =
              emitBinary("eq", slot.key, state.inputKey, state.keyType);
          duplicateValues.push_back(
              emitBinary("and", slot.valid, sameInput, "i1"));
          state.slots.push_back(std::move(slot));
        }
        unsigned freeIndexWidth = 1;
        while ((uint64_t{1} << freeIndexWidth) < reorder.capacity)
          ++freeIndexWidth;
        state.freeIndexType = "i" + std::to_string(freeIndexWidth);
        std::vector<std::string> freeIndices;
        freeIndices.reserve(reorder.capacity);
        for (uint64_t index = 0; index < reorder.capacity; ++index)
          freeIndices.push_back(emitConstant(index, state.freeIndexType));
        auto selectedFree =
            selectBalanced(freeValues, freeIndices, state.freeIndexType);
        state.anyFree = std::move(selectedFree.first);
        state.freeIndex = std::move(selectedFree.second);
        auto selected =
            selectBalanced(matchValues, slotDataValues, state.dataType);
        state.outputMatch = std::move(selected.first);
        std::string selectedData = std::move(selected.second);
        std::string duplicate = reduceBalanced("or", duplicateValues, "i1");
        std::string stale =
            emitBinary("ult", state.inputKey, state.expected, state.keyType);
        std::string invalidKey = emitBinary("or", duplicate, stale, "i1");
        std::string invalidAdmission =
            emitBinary("and", inputValidValue->getValue(), invalidKey, "i1");
        state.safeAdmission = emitNot(invalidAdmission);
        producerValid = state.outputMatch;
        producerData = std::move(selectedData);
        reorderStates[reorder.name] = std::move(state);
      } else if (feedbackProducer != feedbackByOutput.end()) {
        const QueueBlockPlan &feedback = *feedbackProducer->getValue();
        auto inputValidValue = outputValid.find(feedback.inputs.front());
        auto inputDataValue = outputData.find(feedback.inputs.front());
        const QueuePlan *inputQueue = findQueue(plan, feedback.inputs.front());
        if (inputValidValue == outputValid.end() ||
            inputDataValue == outputData.end() || !inputQueue)
          return pycError(
              "feedback input is not available in topological order");
        auto dataType = pycType(plan, inputQueue->payloadType);
        if (!dataType)
          return dataType.takeError();
        unsigned iterationWidth = 1;
        while (iterationWidth < 64 &&
               (uint64_t{1} << iterationWidth) <= feedback.maxIterations)
          ++iterationWidth;
        std::string iterationType = "i" + std::to_string(iterationWidth);
        std::string zeroValid = emitConstant(0, "i1");
        std::string zeroData = emitConstant(0, *dataType);
        std::string zeroIteration = emitConstant(0, iterationType);
        FeedbackState state;
        state.validNext = newValue();
        state.validEnable = newValue();
        state.valid = newValue();
        body << "    " << state.validNext << " = pyc.wire : i1\n";
        body << "    " << state.validEnable << " = pyc.wire : i1\n";
        body << "    " << state.valid << " = pyc.reg %clk, %rst, "
             << state.validEnable << ", " << state.validNext << ", "
             << zeroValid << " : i1\n";
        state.dataNext = newValue();
        state.dataEnable = newValue();
        state.data = newValue();
        body << "    " << state.dataNext << " = pyc.wire : " << *dataType
             << "\n";
        body << "    " << state.dataEnable << " = pyc.wire : i1\n";
        body << "    " << state.data << " = pyc.reg %clk, %rst, "
             << state.dataEnable << ", " << state.dataNext << ", " << zeroData
             << " : " << *dataType << "\n";
        state.iterationNext = newValue();
        state.iterationEnable = newValue();
        state.iteration = newValue();
        body << "    " << state.iterationNext
             << " = pyc.wire : " << iterationType << "\n";
        body << "    " << state.iterationEnable << " = pyc.wire : i1\n";
        body << "    " << state.iteration << " = pyc.reg %clk, %rst, "
             << state.iterationEnable << ", " << state.iterationNext << ", "
             << zeroIteration << " : " << iterationType << "\n";
        std::string selectedData = emitMux(
            state.valid, state.data, inputDataValue->getValue(), *dataType);
        state.selectedValid =
            emitBinary("or", state.valid, inputValidValue->getValue(), "i1");
        state.selectedIteration =
            emitMux(state.valid, state.iteration, zeroIteration, iterationType);
        auto updated =
            emitTransform(plan, feedback, {selectedData},
                          {inputQueue->payloadType}, 0, nextValue, body);
        if (!updated)
          return updated.takeError();
        state.updated = std::move(*updated);
        auto condition =
            emitTransform(plan, feedback, {selectedData},
                          {inputQueue->payloadType}, 1, nextValue, body);
        if (!condition)
          return condition.takeError();
        auto conditionType =
            yieldedType(feedback, feedback.yields[1], inputQueue->payloadType);
        if (!conditionType)
          return conditionType.takeError();
        auto conditionPycType = pycType(plan, *conditionType);
        if (!conditionPycType)
          return conditionPycType.takeError();
        if (*conditionPycType != "i1")
          return pycError("feedback condition must lower to i1");
        state.condition = std::move(*condition);
        std::string limit = emitConstant(feedback.maxIterations, iterationType);
        state.underLimit =
            emitBinary("ult", state.selectedIteration, limit, iterationType);
        std::string done = emitNot(state.condition);
        producerValid = emitBinary("and", state.selectedValid, done, "i1");
        producerData = std::move(selectedData);
        state.dataType = *dataType;
        state.iterationType = std::move(iterationType);
        feedbackStates[feedback.name] = std::move(state);
      } else {
        return pycError("Queue has no supported producer: '" + queue.name +
                        "'");
      }
    }
    auto dataType = pycType(plan, queue.payloadType);
    if (!dataType)
      return dataType.takeError();
    std::vector<std::string> stageReady;
    for (uint64_t stage = 0; stage + 1 < queue.latency; ++stage) {
      std::string ready = newValue();
      body << "    " << ready << " = pyc.wire : i1\n";
      stageReady.push_back(std::move(ready));
    }
    stageReady.push_back(readyWires[queue.name]);
    std::string currentValid = producerValid;
    std::string currentData = producerData;
    std::string firstReady;
    for (uint64_t stage = 0; stage < queue.latency; ++stage) {
      std::string inReady = newValue();
      std::string outValid = newValue();
      std::string outData = newValue();
      const uint64_t depth = stage == 0 ? queue.depth : 1;
      body << "    " << inReady << ", " << outValid << ", " << outData
           << " = pyc.fifo %clk, %rst, " << currentValid << ", " << currentData
           << ", " << stageReady[stage] << " {depth = " << depth
           << "} : " << *dataType << "\n";
      if (stage == 0)
        firstReady = inReady;
      else
        body << "    pyc.assign " << stageReady[stage - 1] << ", " << inReady
             << " : i1\n";
      currentValid = std::move(outValid);
      currentData = std::move(outData);
    }
    inputReady[queue.name] = std::move(firstReady);
    outputValid[queue.name] = std::move(currentValid);
    outputData[queue.name] = std::move(currentData);
  }
  for (const MemoryInstancePlan &instance : plan.memoryInstances) {
    std::vector<const QueueBlockPlan *> endpoints;
    for (const QueueBlockPlan &block : plan.blocks)
      if (block.kind == "memory_request" &&
          block.memoryInstance == instance.name)
        endpoints.push_back(&block);
    llvm::sort(endpoints,
               [](const QueueBlockPlan *left, const QueueBlockPlan *right) {
                 return left->endpointOrdinal < right->endpointOrdinal;
               });
    if (endpoints.empty())
      return pycError("memory instance has no endpoints");
    const QueuePlan *firstInput =
        findQueue(plan, endpoints.front()->inputs.front());
    if (!firstInput)
      return pycError("memory endpoint input Queue is missing");
    auto requestType = pycType(plan, firstInput->payloadType);
    auto dataType = pycType(plan, instance.dataType);
    auto dataWidth = typeWidth(plan, instance.dataType);
    if (!requestType)
      return requestType.takeError();
    if (!dataType)
      return dataType.takeError();
    if (!dataWidth)
      return dataWidth.takeError();

    std::vector<std::string> endpointValids;
    std::vector<std::string> endpointData;
    std::vector<std::string> addresses;
    std::vector<unsigned> addressWidths;
    std::vector<std::string> writes;
    std::vector<std::string> writeData;
    unsigned addressWidth = 1;
    for (const QueueBlockPlan *endpoint : endpoints) {
      auto valid = outputValid.find(endpoint->inputs.front());
      auto data = outputData.find(endpoint->inputs.front());
      const QueuePlan *input = findQueue(plan, endpoint->inputs.front());
      if (valid == outputValid.end() || data == outputData.end() || !input)
        return pycError("memory inputs are not available after Queue lowering");
      endpointValids.push_back(valid->getValue());
      endpointData.push_back(data->getValue());
      for (size_t policy = 0; policy < 3; ++policy) {
        auto value =
            emitTransform(plan, *endpoint, {data->getValue()},
                          {input->payloadType}, policy, nextValue, body);
        if (!value)
          return value.takeError();
        auto yielded = yieldedType(*endpoint, endpoint->yields[policy],
                                   input->payloadType);
        if (!yielded)
          return yielded.takeError();
        auto width = typeWidth(plan, *yielded);
        if (!width)
          return width.takeError();
        if (policy == 0) {
          addresses.push_back(std::move(*value));
          addressWidths.push_back(*width);
          addressWidth = std::max(addressWidth, *width);
        } else if (policy == 1) {
          auto type = pycType(plan, *yielded);
          if (!type)
            return type.takeError();
          if (*type != "i1")
            return pycError("memory write policy must lower to i1");
          writes.push_back(std::move(*value));
        } else {
          if (*width != *dataWidth)
            return pycError(
                "memory data policy must match instance data width");
          writeData.push_back(std::move(*value));
        }
      }
    }
    const std::string addressType = "i" + std::to_string(addressWidth);
    for (size_t index = 0; index < addresses.size(); ++index) {
      if (addressWidths[index] == addressWidth)
        continue;
      std::string zero = emitConstant(
          0, "i" + std::to_string(addressWidth - addressWidths[index]));
      std::string extended = newValue();
      body << "    " << extended << " = pyc.concat(" << zero << ", "
           << addresses[index] << ") : (i"
           << addressWidth - addressWidths[index] << ", i"
           << addressWidths[index] << ") -> " << addressType << "\n";
      addresses[index] = std::move(extended);
    }

    std::string zeroI1 = emitConstant(0, "i1");
    std::string oneI1 = emitConstant(1, "i1");
    std::string busyNext = newValue();
    std::string busyEnable = newValue();
    body << "    " << busyNext << " = pyc.wire : i1\n";
    body << "    " << busyEnable << " = pyc.wire : i1\n";
    std::string busy = newValue();
    body << "    " << busy << " = pyc.reg %clk, %rst, " << busyEnable << ", "
         << busyNext << ", " << zeroI1 << " : i1\n";
    std::string idle = emitNot(busy);
    std::vector<std::string> grants;
    std::string noHigher = oneI1;
    for (size_t index = 0; index < endpoints.size(); ++index) {
      std::string ready = emitBinary("and", idle, noHigher, "i1");
      body << "    pyc.assign " << readyWires[endpoints[index]->inputs.front()]
           << ", " << ready << " : i1\n";
      grants.push_back(emitBinary("and", endpointValids[index], ready, "i1"));
      noHigher =
          emitBinary("and", noHigher, emitNot(endpointValids[index]), "i1");
    }
    std::string issue = reduceBalanced("or", grants, "i1");
    auto selectedAddress =
        selectBalanced(grants, addresses, addressType).second;
    auto selectedWrite = selectBalanced(grants, writes, "i1").second;
    auto selectedWriteData =
        selectBalanced(grants, writeData, *dataType).second;
    auto selectedRequest =
        selectBalanced(grants, endpointData, *requestType).second;
    const bool fullAddressRange =
        addressWidth < 64 && instance.entries == (uint64_t{1} << addressWidth);
    if (!fullAddressRange) {
      std::string addressLimit = emitConstant(instance.entries, addressType);
      std::string addressSafe =
          emitBinary("ult", selectedAddress, addressLimit, addressType);
      std::string accessSafe =
          emitBinary("or", emitNot(issue), addressSafe, "i1");
      body << "    pyc.assert " << accessSafe
           << " {msg = \"memory_address_out_of_range\"}\n";
    }

    unsigned ownerWidth = 1;
    while ((uint64_t{1} << ownerWidth) < endpoints.size())
      ++ownerWidth;
    std::string ownerType = "i" + std::to_string(ownerWidth);
    std::vector<std::string> ownerConstants;
    for (size_t index = 0; index < endpoints.size(); ++index)
      ownerConstants.push_back(emitConstant(index, ownerType));
    std::string selectedOwner =
        selectBalanced(grants, ownerConstants, ownerType).second;
    std::string ownerNext = newValue();
    std::string ownerEnable = newValue();
    body << "    " << ownerNext << " = pyc.wire : " << ownerType << "\n";
    body << "    " << ownerEnable << " = pyc.wire : i1\n";
    std::string owner = newValue();
    body << "    " << owner << " = pyc.reg %clk, %rst, " << ownerEnable << ", "
         << ownerNext << ", " << ownerConstants.front() << " : " << ownerType
         << "\n";
    body << "    pyc.assign " << ownerNext << ", " << selectedOwner << " : "
         << ownerType << "\n";
    body << "    pyc.assign " << ownerEnable << ", " << issue << " : i1\n";

    std::string requestNext = newValue();
    std::string requestEnable = newValue();
    body << "    " << requestNext << " = pyc.wire : " << *requestType << "\n";
    body << "    " << requestEnable << " = pyc.wire : i1\n";
    std::string zeroRequest = emitConstant(0, *requestType);
    std::string pendingRequest = newValue();
    body << "    " << pendingRequest << " = pyc.reg %clk, %rst, "
         << requestEnable << ", " << requestNext << ", " << zeroRequest << " : "
         << *requestType << "\n";
    body << "    pyc.assign " << requestNext << ", " << selectedRequest << " : "
         << *requestType << "\n";
    body << "    pyc.assign " << requestEnable << ", " << issue << " : i1\n";

    std::string writeValid = emitBinary("and", issue, selectedWrite, "i1");
    const unsigned strobeWidth = (*dataWidth + 7) / 8;
    const uint64_t strobeValue = (uint64_t{1} << strobeWidth) - 1;
    std::string strobeType = "i" + std::to_string(strobeWidth);
    std::string strobe = emitConstant(strobeValue, strobeType);
    std::string readData = newValue();
    body << "    " << readData << " = pyc.sync_mem %clk, %rst, " << issue
         << ", " << selectedAddress << ", " << writeValid << ", "
         << selectedAddress << ", " << selectedWriteData << ", " << strobe
         << " {depth = " << instance.entries << ", name = \"" << instance.name
         << "\"} : " << addressType << ", " << *dataType << ", " << strobeType
         << "\n";

    std::string responseMature = busy;
    if (instance.latency > 1) {
      const unsigned latencyWidth = std::max(
          1u, static_cast<unsigned>(std::bit_width(instance.latency - 1)));
      const std::string latencyType = "i" + std::to_string(latencyWidth);
      std::string zeroLatency = emitConstant(0, latencyType);
      std::string oneLatency = emitConstant(1, latencyType);
      std::string initialLatency =
          emitConstant(instance.latency - 1, latencyType);
      std::string latencyNext = newValue();
      std::string latencyEnable = newValue();
      body << "    " << latencyNext << " = pyc.wire : " << latencyType << "\n";
      body << "    " << latencyEnable << " = pyc.wire : i1\n";
      std::string remainingLatency = newValue();
      body << "    " << remainingLatency << " = pyc.reg %clk, %rst, "
           << latencyEnable << ", " << latencyNext << ", " << zeroLatency
           << " : " << latencyType << "\n";
      std::string latencyIsZero =
          emitBinary("eq", remainingLatency, zeroLatency, latencyType);
      std::string latencyActive =
          emitBinary("and", busy, emitNot(latencyIsZero), "i1");
      std::string decremented =
          emitBinary("sub", remainingLatency, oneLatency, latencyType);
      std::string nextLatency =
          emitMux(issue, initialLatency, decremented, latencyType);
      std::string updateLatency = emitBinary("or", issue, latencyActive, "i1");
      body << "    pyc.assign " << latencyNext << ", " << nextLatency << " : "
           << latencyType << "\n";
      body << "    pyc.assign " << latencyEnable << ", " << updateLatency
           << " : i1\n";
      responseMature = emitBinary("and", busy, latencyIsZero, "i1");
    }

    std::vector<std::string> accepted;
    for (size_t index = 0; index < endpoints.size(); ++index) {
      std::string selected =
          emitBinary("eq", owner, ownerConstants[index], ownerType);
      std::string responseValid =
          emitBinary("and", responseMature, selected, "i1");
      auto response = emitFieldReplace(pendingRequest, firstInput->payloadType,
                                       endpoints[index]->resultField, readData);
      if (!response)
        return response.takeError();
      body << "    pyc.assign "
           << memoryResponseValid[endpoints[index]->outputs.front()] << ", "
           << responseValid << " : i1\n";
      body << "    pyc.assign "
           << memoryResponseData[endpoints[index]->outputs.front()] << ", "
           << *response << " : " << *requestType << "\n";
      accepted.push_back(
          emitBinary("and", responseValid,
                     inputReady[endpoints[index]->outputs.front()], "i1"));
    }
    std::string responseAccepted = reduceBalanced("or", accepted, "i1");
    std::string retained =
        emitBinary("and", busy, emitNot(responseAccepted), "i1");
    std::string nextBusy = emitBinary("or", retained, issue, "i1");
    std::string updateBusy = emitBinary("or", responseAccepted, issue, "i1");
    body << "    pyc.assign " << busyNext << ", " << nextBusy << " : i1\n";
    body << "    pyc.assign " << busyEnable << ", " << updateBusy << " : i1\n";
  }
  for (const QueueBlockPlan &firing : plan.blocks) {
    if (firing.kind != "firing" || !firing.outputs.empty())
      continue;
    std::vector<std::string> inputDataValues;
    std::vector<std::string> inputTypes;
    for (const std::string &input : firing.inputs) {
      auto data = outputData.find(input);
      const QueuePlan *queue = findQueue(plan, input);
      if (data == outputData.end() || !queue)
        return pycError("outputless firing input is not available");
      inputDataValues.push_back(data->getValue());
      inputTypes.push_back(queue->payloadType);
    }
    auto values = std::make_shared<llvm::StringMap<std::string>>();
    auto emitted = emitTransform(
        plan, firing, inputDataValues, inputTypes, 0, nextValue, body,
        values.get(), &tableStateValues, &roundRobinStates, firing.name,
        nullptr, nullptr, &sharedTableExpressionValues);
    if (!emitted)
      return emitted.takeError();
    auto guard = values->find(firing.guard);
    if (guard == values->end())
      return pycError("outputless firing guard is missing");
    firingExpressionValuesByBlock[firing.name] = values;
    firingGuardByBlock[firing.name] = guard->getValue();
  }
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "table_write") {
      std::vector<std::string> inputDataValues;
      std::vector<std::string> inputTypes;
      std::string allValid = emitConstant(1, "i1");
      for (const std::string &input : block.inputs) {
        auto valid = outputValid.find(input);
        auto data = outputData.find(input);
        const QueuePlan *inputQueue = findQueue(plan, input);
        if (valid == outputValid.end() || data == outputData.end() ||
            !inputQueue)
          return pycError("Table write input is not available");
        allValid = emitBinary("and", allValid, valid->getValue(), "i1");
        inputDataValues.push_back(data->getValue());
        inputTypes.push_back(inputQueue->payloadType);
      }
      auto values = std::make_shared<llvm::StringMap<std::string>>();
      auto index =
          emitTransform(plan, block, inputDataValues, inputTypes, 0, nextValue,
                        body, values.get(), &tableStateValues, nullptr, {},
                        nullptr, nullptr, &sharedTableExpressionValues);
      if (!index)
        return index.takeError();
      auto present = values->find(block.yields[1]);
      auto proposed = values->find(block.yields[2]);
      if (present == values->end() || proposed == values->end())
        return pycError("Table write expression values are missing");
      tableWriteAccepted[block.name] =
          block.inputs.empty() ? present->getValue() : allValid;
      tableWriteExpressionValues[block.name] = values;
    } else if (block.kind == "table_masked_write") {
      QueueBlockPlan maskBlock = expressionSlice(block, block.yields[0]);
      QueueBlockPlan enableBlock = expressionSlice(block, block.yields[1]);
      auto mask = emitTransform(plan, maskBlock, {}, {}, 0, nextValue, body,
                                nullptr, &tableStateValues, nullptr, {},
                                nullptr, nullptr, &sharedTableExpressionValues);
      auto enabled =
          emitTransform(plan, enableBlock, {}, {}, 0, nextValue, body, nullptr,
                        &tableStateValues, nullptr, {}, nullptr, nullptr,
                        &sharedTableExpressionValues);
      if (!mask)
        return mask.takeError();
      if (!enabled)
        return enabled.takeError();
      tableMaskedWriteValues[block.name] = {*mask, *enabled};
    }
  }
  using ArbitrationMember =
      std::pair<const QueueBlockPlan *, const QueueWriterArbitrationPlan *>;
  llvm::StringMap<std::vector<ArbitrationMember>> arbitrationByOwner;
  llvm::StringMap<std::string> arbitrationCandidates;
  auto arbitrationKey = [](llvm::StringRef block, llvm::StringRef owner) {
    return (block + "\x1f" + owner).str();
  };
  for (const QueueBlockPlan &block : plan.blocks) {
    for (const QueueWriterArbitrationPlan &membership :
         block.arbitrationMembership) {
      arbitrationByOwner[membership.owner].push_back({&block, &membership});
      std::string candidate;
      if (block.kind == "firing") {
        auto guard = firingGuardByBlock.find(block.name);
        auto values = firingExpressionValuesByBlock.find(block.name);
        if (guard == firingGuardByBlock.end() ||
            values == firingExpressionValuesByBlock.end())
          return pycError("arbitrated firing values are missing");
        candidate = guard->getValue();
        for (const std::string &input : block.inputs)
          candidate = emitBinary("and", candidate, outputValid[input], "i1");
        std::vector<std::string> presents;
        for (const StateWritePlan &write : block.stateWrites) {
          if (write.table != membership.owner)
            continue;
          auto present = values->getValue()->find(write.present);
          if (present == values->getValue()->end())
            return pycError("arbitrated firing presence is missing");
          presents.push_back(present->getValue());
        }
        if (!presents.empty())
          candidate = emitBinary("and", candidate,
                                 presents.size() == 1
                                     ? presents.front()
                                     : reduceBalanced("or", presents, "i1"),
                                 "i1");
      } else if (block.kind == "table_write") {
        auto accepted = tableWriteAccepted.find(block.name);
        auto values = tableWriteExpressionValues.find(block.name);
        if (accepted == tableWriteAccepted.end() ||
            values == tableWriteExpressionValues.end())
          return pycError("arbitrated Table write values are missing");
        auto present = values->getValue()->find(block.yields[1]);
        if (present == values->getValue()->end())
          return pycError("arbitrated Table write enable is missing");
        candidate =
            emitBinary("and", accepted->getValue(), present->getValue(), "i1");
      } else if (block.kind == "table_masked_write") {
        auto masked = tableMaskedWriteValues.find(block.name);
        if (masked == tableMaskedWriteValues.end())
          return pycError("arbitrated masked write values are missing");
        auto type = yieldedType(block, block.yields[0], "i1");
        if (!type)
          return type.takeError();
        auto width = typeWidth(plan, *type);
        auto pyc = pycType(plan, *type);
        if (!width)
          return width.takeError();
        if (!pyc)
          return pyc.takeError();
        std::vector<std::string> bits;
        for (unsigned bit = 0; bit < *width; ++bit) {
          std::string value = newValue();
          body << "    " << value << " = pyc.extract "
               << masked->getValue().mask << " {lsb = " << bit << "} : " << *pyc
               << " -> i1\n";
          bits.push_back(std::move(value));
        }
        std::string any =
            bits.size() == 1 ? bits.front() : reduceBalanced("or", bits, "i1");
        candidate = emitBinary("and", masked->getValue().enabled, any, "i1");
      } else {
        return pycError("unsupported arbitrated Table writer kind");
      }
      arbitrationCandidates[arbitrationKey(block.name, membership.owner)] =
          std::move(candidate);
    }
  }
  for (auto &entry : arbitrationByOwner) {
    std::vector<ArbitrationMember> &members = entry.getValue();
    llvm::sort(members, [](const ArbitrationMember &left,
                           const ArbitrationMember &right) {
      return left.second->declaredRank < right.second->declaredRank;
    });
    std::string prior = emitConstant(0, "i1");
    for (const ArbitrationMember &member : members) {
      const std::string &candidate = arbitrationCandidates.lookup(
          arbitrationKey(member.first->name, entry.getKey()));
      std::string grant = emitBinary("and", candidate, emitNot(prior), "i1");
      std::string allowed = emitBinary("or", emitNot(candidate), grant, "i1");
      auto existing = blockArbitrationAllowed.find(member.first->name);
      blockArbitrationAllowed[member.first->name] =
          existing == blockArbitrationAllowed.end()
              ? allowed
              : emitBinary("and", existing->getValue(), allowed, "i1");
      prior = emitBinary("or", prior, candidate, "i1");
    }
  }
  for (const QueueBlockPlan &block : plan.blocks) {
    if (block.kind == "transform") {
      if (block.inputs.size() == 1 && block.outputs.size() == 1) {
        body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
             << inputReady[block.outputs.front()] << " : i1\n";
      } else {
        std::string allReady = inputReady[block.outputs.front()];
        for (size_t index = 1; index < block.outputs.size(); ++index)
          allReady = emitBinary("and", allReady,
                                inputReady[block.outputs[index]], "i1");
        std::string allValid = outputValid[block.inputs.front()];
        for (size_t index = 1; index < block.inputs.size(); ++index)
          allValid = emitBinary("and", allValid,
                                outputValid[block.inputs[index]], "i1");
        for (auto [index, input] : llvm::enumerate(block.inputs)) {
          std::string inputCanFire = allReady;
          for (auto [otherIndex, other] : llvm::enumerate(block.inputs)) {
            if (otherIndex == index)
              continue;
            inputCanFire =
                emitBinary("and", inputCanFire, outputValid[other], "i1");
          }
          body << "    pyc.assign " << readyWires[input] << ", " << inputCanFire
               << " : i1\n";
        }
        for (auto [index, output] : llvm::enumerate(block.outputs)) {
          auto validWire = atomicTransformValid.find(output);
          if (validWire == atomicTransformValid.end())
            return pycError("atomic transform valid wire is missing");
          std::string outputCanFire = allValid;
          for (auto [otherIndex, other] : llvm::enumerate(block.outputs)) {
            if (otherIndex == index)
              continue;
            outputCanFire =
                emitBinary("and", outputCanFire, inputReady[other], "i1");
          }
          body << "    pyc.assign " << validWire->getValue() << ", "
               << outputCanFire << " : i1\n";
        }
      }
    } else if (block.kind == "firing") {
      auto guard = firingGuardByBlock.find(block.name);
      if (guard == firingGuardByBlock.end())
        return pycError("firing guard value is missing");
      auto arbitration = blockArbitrationAllowed.find(block.name);
      std::string arbitrationAllowed =
          arbitration == blockArbitrationAllowed.end()
              ? std::string()
              : arbitration->getValue();
      std::vector<std::string> selectedReady;
      selectedReady.reserve(block.outputs.size());
      for (const std::string &output : block.outputs) {
        auto presence = firingPresence.find(output);
        auto ready = inputReady.find(output);
        if (presence == firingPresence.end() || ready == inputReady.end())
          return pycError("firing output handshake is missing");
        selectedReady.push_back(emitBinary("or", emitNot(presence->getValue()),
                                           ready->getValue(), "i1"));
      }
      std::string allSelectedReady =
          selectedReady.empty() ? emitConstant(1, "i1")
                                : reduceBalanced("and", selectedReady, "i1");
      for (auto [inputIndex, input] : llvm::enumerate(block.inputs)) {
        std::string inputCanFire =
            emitBinary("and", guard->getValue(), allSelectedReady, "i1");
        if (!arbitrationAllowed.empty())
          inputCanFire =
              emitBinary("and", inputCanFire, arbitrationAllowed, "i1");
        for (auto [otherIndex, other] : llvm::enumerate(block.inputs)) {
          if (otherIndex == inputIndex)
            continue;
          auto valid = outputValid.find(other);
          if (valid == outputValid.end())
            return pycError("firing input valid is missing");
          inputCanFire =
              emitBinary("and", inputCanFire, valid->getValue(), "i1");
        }
        body << "    pyc.assign " << readyWires[input] << ", " << inputCanFire
             << " : i1\n";
      }
      std::string allInputsValid = guard->getValue();
      for (const std::string &input : block.inputs) {
        auto valid = outputValid.find(input);
        if (valid == outputValid.end())
          return pycError("firing input valid is missing");
        allInputsValid =
            emitBinary("and", allInputsValid, valid->getValue(), "i1");
      }
      firingAccepted[block.name] =
          emitBinary("and", allInputsValid, allSelectedReady, "i1");
      if (!arbitrationAllowed.empty())
        firingAccepted[block.name] = emitBinary(
            "and", firingAccepted[block.name], arbitrationAllowed, "i1");
      for (auto [outputIndex, output] : llvm::enumerate(block.outputs)) {
        auto validWire = atomicTransformValid.find(output);
        auto presence = firingPresence.find(output);
        if (validWire == atomicTransformValid.end() ||
            presence == firingPresence.end())
          return pycError("firing output valid is missing");
        std::string outputCanFire =
            emitBinary("and", allInputsValid, presence->getValue(), "i1");
        if (!arbitrationAllowed.empty())
          outputCanFire =
              emitBinary("and", outputCanFire, arbitrationAllowed, "i1");
        for (auto [otherIndex, other] : llvm::enumerate(block.outputs)) {
          if (otherIndex == outputIndex)
            continue;
          auto otherPresence = firingPresence.find(other);
          auto otherReady = inputReady.find(other);
          if (otherPresence == firingPresence.end() ||
              otherReady == inputReady.end())
            return pycError("firing output handshake is missing");
          std::string otherSelectedReady =
              emitBinary("or", emitNot(otherPresence->getValue()),
                         otherReady->getValue(), "i1");
          outputCanFire =
              emitBinary("and", outputCanFire, otherSelectedReady, "i1");
        }
        body << "    pyc.assign " << validWire->getValue() << ", "
             << outputCanFire << " : i1\n";
      }
    } else if (block.kind == "table_read_group") {
      auto state = tableReadGroupStates.find(block.name);
      if (state == tableReadGroupStates.end() ||
          state->getValue().valids.size() != block.outputs.size())
        return pycError("grouped Table read values are missing");
      std::vector<std::string> selectedReady;
      for (auto [index, output] : llvm::enumerate(block.outputs)) {
        std::string notSelected = emitNot(state->getValue().valids[index]);
        selectedReady.push_back(
            emitBinary("or", notSelected, inputReady[output], "i1"));
      }
      std::string allSelectedReady = reduceBalanced("and", selectedReady, "i1");
      std::string anySelected =
          reduceBalanced("or", state->getValue().valids, "i1");
      tableSelectionAccepted[block.name] =
          emitBinary("and", anySelected, allSelectedReady, "i1");
      for (auto [index, output] : llvm::enumerate(block.outputs)) {
        auto valid = tableReadGroupValidWires.find(output);
        if (valid == tableReadGroupValidWires.end())
          return pycError("grouped Table read valid wire is missing");
        std::string publish = emitBinary("and", state->getValue().valids[index],
                                         allSelectedReady, "i1");
        body << "    pyc.assign " << valid->getValue() << ", " << publish
             << " : i1\n";
      }
    } else if (block.kind == "table_write") {
      auto arbitration = blockArbitrationAllowed.find(block.name);
      for (auto [inputIndex, input] : llvm::enumerate(block.inputs)) {
        std::string ready = arbitration == blockArbitrationAllowed.end()
                                ? emitConstant(1, "i1")
                                : arbitration->getValue();
        for (auto [otherIndex, other] : llvm::enumerate(block.inputs)) {
          if (otherIndex == inputIndex)
            continue;
          ready = emitBinary("and", ready, outputValid[other], "i1");
        }
        body << "    pyc.assign " << readyWires[input] << ", " << ready
             << " : i1\n";
      }
    } else if (block.kind == "table_masked_write") {
      // Stateful masked writers have no Queue handshake. Their register-bank
      // updates are emitted after arbitration below.
    } else if (block.kind == "table_read") {
      auto present = tableReadWhen.find(block.name);
      if (present == tableReadWhen.end())
        return pycError("Table read predicate is missing");
      for (auto [inputIndex, input] : llvm::enumerate(block.inputs)) {
        std::string ready = emitBinary("and", inputReady[block.outputs.front()],
                                       present->getValue(), "i1");
        for (auto [otherIndex, other] : llvm::enumerate(block.inputs)) {
          if (otherIndex == inputIndex)
            continue;
          ready = emitBinary("and", ready, outputValid[other], "i1");
        }
        body << "    pyc.assign " << readyWires[input] << ", " << ready
             << " : i1\n";
      }
    } else if (block.kind == "barrier") {
      std::string allReady = inputReady[block.outputs.front()];
      for (size_t index = 1; index < block.outputs.size(); ++index)
        allReady =
            emitBinary("and", allReady, inputReady[block.outputs[index]], "i1");
      std::string allValid = outputValid[block.inputs.front()];
      for (size_t index = 1; index < block.inputs.size(); ++index)
        allValid =
            emitBinary("and", allValid, outputValid[block.inputs[index]], "i1");
      for (auto [index, input] : llvm::enumerate(block.inputs)) {
        std::string inputCanFire = allReady;
        for (auto [otherIndex, other] : llvm::enumerate(block.inputs)) {
          if (otherIndex == index)
            continue;
          inputCanFire =
              emitBinary("and", inputCanFire, outputValid[other], "i1");
        }
        body << "    pyc.assign " << readyWires[input] << ", " << inputCanFire
             << " : i1\n";
      }
      for (auto [index, output] : llvm::enumerate(block.outputs)) {
        auto validWire = atomicTransformValid.find(output);
        if (validWire == atomicTransformValid.end())
          return pycError("barrier valid wire is missing");
        std::string outputCanFire = allValid;
        for (auto [otherIndex, other] : llvm::enumerate(block.outputs)) {
          if (otherIndex == index)
            continue;
          outputCanFire =
              emitBinary("and", outputCanFire, inputReady[other], "i1");
        }
        body << "    pyc.assign " << validWire->getValue() << ", "
             << outputCanFire << " : i1\n";
      }
    } else if (block.kind == "broadcast") {
      std::string allReady = inputReady[block.outputs.front()];
      for (size_t index = 1; index < block.outputs.size(); ++index)
        allReady =
            emitBinary("and", allReady, inputReady[block.outputs[index]], "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << allReady << " : i1\n";
    } else if (block.kind == "fork") {
      auto state = forkStates.find(block.name);
      auto inputValidValue = outputValid.find(block.inputs.front());
      if (state == forkStates.end() || inputValidValue == outputValid.end())
        return pycError("fork state or input valid is missing");
      std::vector<std::string> deliveredNow;
      for (auto [index, output] : llvm::enumerate(block.outputs)) {
        std::string accepted =
            emitBinary("and", forkOfferValid[output], inputReady[output], "i1");
        deliveredNow.push_back(emitBinary(
            "or", state->getValue().delivered[index], accepted, "i1"));
      }
      std::string deliveredAll = deliveredNow.front();
      for (size_t index = 1; index < deliveredNow.size(); ++index)
        deliveredAll =
            emitBinary("and", deliveredAll, deliveredNow[index], "i1");
      std::string complete =
          emitBinary("and", inputValidValue->getValue(), deliveredAll, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << complete << " : i1\n";
      std::string zero = emitConstant(0, "i1");
      for (size_t index = 0; index < block.outputs.size(); ++index) {
        std::string next = emitMux(complete, zero, deliveredNow[index], "i1");
        body << "    pyc.assign " << state->getValue().nextWires[index] << ", "
             << next << " : i1\n";
        body << "    pyc.assign " << state->getValue().enableWires[index]
             << ", " << inputValidValue->getValue() << " : i1\n";
      }
    } else if (block.kind == "route") {
      std::string selectedReady = emitConstant(0, "i1");
      for (size_t index = block.outputs.size(); index-- > 0;) {
        auto condition = routeCondition.find(block.outputs[index]);
        if (condition == routeCondition.end())
          return pycError("route output condition is missing");
        selectedReady =
            emitMux(condition->getValue(), inputReady[block.outputs[index]],
                    selectedReady, "i1");
      }
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << selectedReady << " : i1\n";
    } else if (block.kind == "select") {
      auto state = selectStates.find(block.name);
      if (state == selectStates.end())
        return pycError("select state is missing");
      const std::string &outputReady = inputReady[block.outputs.front()];
      std::string controlReady =
          emitBinary("and", outputReady, state->getValue().selectedValid, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << controlReady << " : i1\n";
      body << "    pyc.assert " << state->getValue().selectorSafe
           << " {msg = \"select_selector_out_of_range\"}\n";
      for (size_t index = 1; index < block.inputs.size(); ++index) {
        std::string selected =
            emitBinary("and", state->getValue().controlValid,
                       state->getValue().conditions[index - 1], "i1");
        std::string ready = emitBinary("and", outputReady, selected, "i1");
        body << "    pyc.assign " << readyWires[block.inputs[index]] << ", "
             << ready << " : i1\n";
      }
    } else if (block.kind == "merge") {
      auto grants = mergeGrants.find(block.name);
      if (grants == mergeGrants.end() ||
          grants->getValue().size() != block.inputs.size())
        return pycError("merge grant plan is missing");
      const std::string &ready = inputReady[block.outputs.front()];
      for (auto [index, input] : llvm::enumerate(block.inputs)) {
        std::string accepted =
            emitBinary("and", ready, grants->getValue()[index], "i1");
        body << "    pyc.assign " << readyWires[input] << ", " << accepted
             << " : i1\n";
      }
      auto state = mergeStates.find(block.name);
      if (state != mergeStates.end()) {
        std::string accepted =
            emitBinary("and", ready, state->getValue().valid, "i1");
        body << "    pyc.assign " << state->getValue().enableWire << ", "
             << accepted << " : i1\n";
        std::string next = state->getValue().cursor;
        for (size_t index = block.inputs.size(); index-- > 0;) {
          std::string nextValue = emitConstant(
              (index + 1) % block.inputs.size(), state->getValue().type);
          next = emitMux(grants->getValue()[index], nextValue, next,
                         state->getValue().type);
        }
        body << "    pyc.assign " << state->getValue().nextWire << ", " << next
             << " : " << state->getValue().type << "\n";
      }
    } else if (block.kind == "credit") {
      auto state = creditStates.find(block.name);
      auto inputValidValue = outputValid.find(block.inputs.front());
      auto inputDataValue = outputData.find(block.inputs.front());
      if (state == creditStates.end() || inputValidValue == outputValid.end() ||
          inputDataValue == outputData.end())
        return pycError("credit state or input is missing");
      const std::string &outputReady = inputReady[block.outputs.front()];
      std::string retire =
          emitBinary("and", state->getValue().outputDone, outputReady, "i1");
      std::string inputReadyValue =
          emitBinary("and", state->getValue().anyFree,
                     state->getValue().safeAdmission, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << inputReadyValue << " : i1\n";
      body << "    pyc.assert " << state->getValue().safeAdmission
           << " {msg = \"credit_nonpositive_cost\"}\n";
      std::string admit =
          emitBinary("and", inputValidValue->getValue(), inputReadyValue, "i1");
      std::string zeroValid = emitConstant(0, "i1");
      std::string oneValid = emitConstant(1, "i1");
      std::string oneCost = emitConstant(1, state->getValue().costType);
      std::vector<std::string> freeGrants;
      std::vector<std::string> doneGrants;
      for (size_t index = 0; index < state->getValue().slots.size(); ++index) {
        std::string indexValue =
            emitConstant(index, state->getValue().freeIndexType);
        freeGrants.push_back(emitBinary("eq", state->getValue().freeIndex,
                                        indexValue,
                                        state->getValue().freeIndexType));
        doneGrants.push_back(emitBinary("eq", state->getValue().doneIndex,
                                        indexValue,
                                        state->getValue().freeIndexType));
      }
      for (auto [index, slot] : llvm::enumerate(state->getValue().slots)) {
        std::string admitSlot =
            emitBinary("and", admit, freeGrants[index], "i1");
        std::string retireSlot =
            emitBinary("and", retire, doneGrants[index], "i1");
        std::string active =
            emitBinary("and", slot.valid, emitNot(slot.done), "i1");
        std::string decremented = emitBinary("sub", slot.remaining, oneCost,
                                             state->getValue().costType);
        std::string nextRemaining = emitMux(active, decremented, slot.remaining,
                                            state->getValue().costType);
        nextRemaining = emitMux(admitSlot, state->getValue().inputCost,
                                nextRemaining, state->getValue().costType);
        std::string nextValid =
            emitMux(retireSlot, zeroValid, slot.valid, "i1");
        nextValid = emitMux(admitSlot, oneValid, nextValid, "i1");
        std::string nextData = emitMux(admitSlot, inputDataValue->getValue(),
                                       slot.data, state->getValue().dataType);
        std::string nextState = newValue();
        body << "    " << nextState << " = pyc.concat(" << nextValid << ", "
             << nextRemaining << ", " << nextData << ") : (i1, "
             << state->getValue().costType << ", " << state->getValue().dataType
             << ") -> " << state->getValue().slotType << "\n";
        std::string changed =
            reduceBalanced("or", {admitSlot, retireSlot, active}, "i1");
        body << "    pyc.assign " << slot.next << ", " << nextState << " : "
             << state->getValue().slotType << "\n";
        body << "    pyc.assign " << slot.enable << ", " << changed
             << " : i1\n";
      }
    } else if (block.kind == "memory_request") {
      // Shared memory-instance arbitration and endpoint ready/response demux
      // are emitted once above, after every Queue signal is available.
    } else if (block.kind == "dependency") {
      auto state = dependencyStates.find(block.name);
      auto inputValidValue = outputValid.find(block.inputs.front());
      auto inputDataValue = outputData.find(block.inputs.front());
      if (state == dependencyStates.end() ||
          inputValidValue == outputValid.end() ||
          inputDataValue == outputData.end())
        return pycError("dependency state or input is missing");
      const std::string &outputReady = inputReady[block.outputs.front()];
      std::string retire =
          emitBinary("and", state->getValue().outputDone, outputReady, "i1");
      std::string inputReadyValue =
          emitBinary("and", state->getValue().anyFree,
                     state->getValue().safeAdmission, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << inputReadyValue << " : i1\n";
      std::string admit =
          emitBinary("and", inputValidValue->getValue(), inputReadyValue, "i1");
      std::string zeroValid = emitConstant(0, "i1");
      std::string oneValid = emitConstant(1, "i1");
      std::string waitingPhase = emitConstant(0, "i2");
      std::string executingPhase = emitConstant(1, "i2");
      std::string donePhase = emitConstant(2, "i2");
      std::string zeroCost = emitConstant(0, state->getValue().costType);
      std::string oneCost = emitConstant(1, state->getValue().costType);
      std::string noDependency = emitConstant(state->getValue().noDependency,
                                              state->getValue().keyType);

      std::vector<std::string> freeGrants;
      std::vector<std::string> doneGrants;
      std::vector<std::string> indexValues;
      freeGrants.reserve(state->getValue().slots.size());
      doneGrants.reserve(state->getValue().slots.size());
      indexValues.reserve(state->getValue().slots.size());
      for (size_t index = 0; index < state->getValue().slots.size(); ++index) {
        std::string indexValue =
            emitConstant(index, state->getValue().freeIndexType);
        indexValues.push_back(indexValue);
        freeGrants.push_back(emitBinary("eq", state->getValue().freeIndex,
                                        indexValue,
                                        state->getValue().freeIndexType));
        doneGrants.push_back(emitBinary("eq", state->getValue().doneIndex,
                                        indexValue,
                                        state->getValue().freeIndexType));
      }

      std::vector<std::string> waitingValues;
      std::vector<std::string> executingValues;
      std::vector<std::string> completeValues;
      std::vector<std::string> decrementValues;
      std::vector<std::string> decrementedValues;
      std::vector<std::string> dependencyReadyValues;
      for (const DependencySlotState &slot : state->getValue().slots) {
        std::string isWaiting =
            emitBinary("eq", slot.phase, waitingPhase, "i2");
        waitingValues.push_back(emitBinary("and", slot.valid, isWaiting, "i1"));
        std::string isExecuting =
            emitBinary("eq", slot.phase, executingPhase, "i2");
        isExecuting = emitBinary("and", slot.valid, isExecuting, "i1");
        executingValues.push_back(isExecuting);
        std::string sentinel = emitBinary("eq", slot.predecessor, noDependency,
                                          state->getValue().keyType);
        std::vector<std::string> predecessorMatches;
        predecessorMatches.reserve(state->getValue().slots.size());
        for (const DependencySlotState &candidate : state->getValue().slots) {
          std::string same = emitBinary("eq", candidate.key, slot.predecessor,
                                        state->getValue().keyType);
          predecessorMatches.push_back(
              emitBinary("and", candidate.done, same, "i1"));
        }
        std::string predecessorDone =
            reduceBalanced("or", predecessorMatches, "i1");
        dependencyReadyValues.push_back(
            emitBinary("or", sentinel, predecessorDone, "i1"));
        std::string atOne = emitBinary("eq", slot.remaining, oneCost,
                                       state->getValue().costType);
        completeValues.push_back(emitBinary("and", isExecuting, atOne, "i1"));
        std::string notAtOne = emitNot(atOne);
        decrementValues.push_back(
            emitBinary("and", isExecuting, notAtOne, "i1"));
        decrementedValues.push_back(emitBinary("sub", slot.remaining, oneCost,
                                               state->getValue().costType));
      }

      std::string noIssue = emitConstant(0, "i1");
      std::vector<std::string> issueValues(state->getValue().slots.size(),
                                           noIssue);
      for (uint64_t resource = 0; resource < state->getValue().resources;
           ++resource) {
        std::string resourceValue =
            emitConstant(resource, state->getValue().resourceType);
        std::vector<std::string> candidates;
        std::vector<std::string> blockers;
        candidates.reserve(state->getValue().slots.size());
        blockers.reserve(state->getValue().slots.size());
        for (auto [index, slot] : llvm::enumerate(state->getValue().slots)) {
          std::string sameResource =
              emitBinary("eq", slot.resource, resourceValue,
                         state->getValue().resourceType);
          std::string ready = emitBinary("and", waitingValues[index],
                                         dependencyReadyValues[index], "i1");
          candidates.push_back(emitBinary("and", ready, sameResource, "i1"));
          std::string stillExecuting =
              emitBinary("and", executingValues[index],
                         emitNot(completeValues[index]), "i1");
          blockers.push_back(
              emitBinary("and", stillExecuting, sameResource, "i1"));
        }
        auto selected = selectBalanced(candidates, indexValues,
                                       state->getValue().freeIndexType);
        std::string resourceBusy = reduceBalanced("or", blockers, "i1");
        std::string resourceCanIssue =
            emitBinary("and", selected.first, emitNot(resourceBusy), "i1");
        for (size_t index = 0; index < state->getValue().slots.size();
             ++index) {
          std::string selectedSlot =
              emitBinary("eq", selected.second, indexValues[index],
                         state->getValue().freeIndexType);
          std::string issue =
              emitBinary("and", resourceCanIssue, selectedSlot, "i1");
          issueValues[index] =
              emitBinary("or", issueValues[index], issue, "i1");
        }
      }

      for (auto [index, slot] : llvm::enumerate(state->getValue().slots)) {
        std::string admitSlot =
            emitBinary("and", admit, freeGrants[index], "i1");
        std::string retireSlot =
            emitBinary("and", retire, doneGrants[index], "i1");
        const std::string &issue = issueValues[index];
        const std::string &complete = completeValues[index];
        const std::string &decrement = decrementValues[index];
        const std::string &decremented = decrementedValues[index];

        std::string nextValid =
            emitMux(retireSlot, zeroValid, slot.valid, "i1");
        nextValid = emitMux(admitSlot, oneValid, nextValid, "i1");
        std::string nextPhase = emitMux(complete, donePhase, slot.phase, "i2");
        nextPhase = emitMux(issue, executingPhase, nextPhase, "i2");
        nextPhase = emitMux(admitSlot, waitingPhase, nextPhase, "i2");
        std::string nextRemaining = emitMux(
            decrement, decremented, slot.remaining, state->getValue().costType);
        nextRemaining = emitMux(complete, zeroCost, nextRemaining,
                                state->getValue().costType);
        nextRemaining = emitMux(issue, slot.cost, nextRemaining,
                                state->getValue().costType);
        nextRemaining = emitMux(admitSlot, zeroCost, nextRemaining,
                                state->getValue().costType);
        std::string nextKey = emitMux(admitSlot, state->getValue().inputKey,
                                      slot.key, state->getValue().keyType);
        std::string nextPredecessor =
            emitMux(admitSlot, state->getValue().inputPredecessor,
                    slot.predecessor, state->getValue().keyType);
        std::string nextResource =
            emitMux(admitSlot, state->getValue().inputResource, slot.resource,
                    state->getValue().resourceType);
        std::string nextCost = emitMux(admitSlot, state->getValue().inputCost,
                                       slot.cost, state->getValue().costType);
        std::string nextData = emitMux(admitSlot, inputDataValue->getValue(),
                                       slot.data, state->getValue().dataType);
        std::string nextState = newValue();
        body << "    " << nextState << " = pyc.concat(" << nextValid << ", "
             << nextPhase << ", " << nextKey << ", " << nextPredecessor << ", "
             << nextResource << ", " << nextRemaining << ", " << nextCost
             << ", " << nextData << ") : (i1, i2, " << state->getValue().keyType
             << ", " << state->getValue().keyType << ", "
             << state->getValue().resourceType << ", "
             << state->getValue().costType << ", " << state->getValue().costType
             << ", " << state->getValue().dataType << ") -> "
             << state->getValue().slotType << "\n";
        std::vector<std::string> changes = {admitSlot, retireSlot, issue,
                                            complete, decrement};
        std::string changed = reduceBalanced("or", changes, "i1");
        body << "    pyc.assign " << slot.next << ", " << nextState << " : "
             << state->getValue().slotType << "\n";
        body << "    pyc.assign " << slot.enable << ", " << changed
             << " : i1\n";
      }
    } else if (block.kind == "reorder") {
      auto state = reorderStates.find(block.name);
      auto inputValidValue = outputValid.find(block.inputs.front());
      auto inputDataValue = outputData.find(block.inputs.front());
      if (state == reorderStates.end() ||
          inputValidValue == outputValid.end() ||
          inputDataValue == outputData.end())
        return pycError("reorder state or input is missing");
      const std::string &outputReady = inputReady[block.outputs.front()];
      std::string retire =
          emitBinary("and", state->getValue().outputMatch, outputReady, "i1");
      std::string inputReadyValue =
          emitBinary("and", state->getValue().anyFree,
                     state->getValue().safeAdmission, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << inputReadyValue << " : i1\n";
      std::string admit =
          emitBinary("and", inputValidValue->getValue(), inputReadyValue, "i1");
      std::string zero = emitConstant(0, "i1");
      std::string one = emitConstant(1, "i1");
      std::string admittedState = newValue();
      body << "    " << admittedState << " = pyc.concat(" << one << ", "
           << state->getValue().inputKey << ", " << inputDataValue->getValue()
           << ") : (i1, " << state->getValue().keyType << ", "
           << state->getValue().dataType << ") -> "
           << state->getValue().slotType << "\n";
      std::vector<std::string> grants;
      grants.reserve(state->getValue().slots.size());
      for (size_t index = 0; index < state->getValue().slots.size(); ++index) {
        std::string indexValue =
            emitConstant(index, state->getValue().freeIndexType);
        grants.push_back(emitBinary("eq", state->getValue().freeIndex,
                                    indexValue,
                                    state->getValue().freeIndexType));
      }
      for (auto [index, slot] : llvm::enumerate(state->getValue().slots)) {
        std::string admitSlot = emitBinary("and", admit, grants[index], "i1");
        std::string retireSlot = emitBinary("and", retire, slot.match, "i1");
        std::string changed = emitBinary("or", admitSlot, retireSlot, "i1");
        std::string clearedState = newValue();
        body << "    " << clearedState << " = pyc.concat(" << zero << ", "
             << slot.key << ", " << slot.data << ") : (i1, "
             << state->getValue().keyType << ", " << state->getValue().dataType
             << ") -> " << state->getValue().slotType << "\n";
        std::string afterRetire = emitMux(retireSlot, clearedState, slot.state,
                                          state->getValue().slotType);
        std::string nextState = emitMux(admitSlot, admittedState, afterRetire,
                                        state->getValue().slotType);
        body << "    pyc.assign " << slot.next << ", " << nextState << " : "
             << state->getValue().slotType << "\n";
        body << "    pyc.assign " << slot.enable << ", " << changed
             << " : i1\n";
      }
      std::string keyOne = emitConstant(1, state->getValue().keyType);
      std::string nextExpected = emitBinary("add", state->getValue().expected,
                                            keyOne, state->getValue().keyType);
      body << "    pyc.assign " << state->getValue().expectedNext << ", "
           << nextExpected << " : " << state->getValue().keyType << "\n";
      body << "    pyc.assign " << state->getValue().expectedEnable << ", "
           << retire << " : i1\n";
    } else if (block.kind == "feedback") {
      auto state = feedbackStates.find(block.name);
      if (state == feedbackStates.end())
        return pycError("feedback state is missing");
      std::string notInternal = emitNot(state->getValue().valid);
      std::string canProceed =
          emitMux(state->getValue().condition, state->getValue().underLimit,
                  inputReady[block.outputs.front()], "i1");
      std::string accepted =
          emitBinary("and", state->getValue().selectedValid, canProceed, "i1");
      std::string externalReady =
          emitBinary("and", notInternal, canProceed, "i1");
      body << "    pyc.assign " << readyWires[block.inputs.front()] << ", "
           << externalReady << " : i1\n";
      std::string continueAccepted =
          emitBinary("and", accepted, state->getValue().condition, "i1");
      body << "    pyc.assign " << state->getValue().validNext << ", "
           << state->getValue().condition << " : i1\n";
      body << "    pyc.assign " << state->getValue().validEnable << ", "
           << accepted << " : i1\n";
      body << "    pyc.assign " << state->getValue().dataNext << ", "
           << state->getValue().updated << " : " << state->getValue().dataType
           << "\n";
      body << "    pyc.assign " << state->getValue().dataEnable << ", "
           << continueAccepted << " : i1\n";
      std::string one = emitConstant(1, state->getValue().iterationType);
      std::string nextIteration =
          emitBinary("add", state->getValue().selectedIteration, one,
                     state->getValue().iterationType);
      body << "    pyc.assign " << state->getValue().iterationNext << ", "
           << nextIteration << " : " << state->getValue().iterationType << "\n";
      body << "    pyc.assign " << state->getValue().iterationEnable << ", "
           << continueAccepted << " : i1\n";
      std::string atLimit = emitNot(state->getValue().underLimit);
      std::string limitCondition =
          emitBinary("and", state->getValue().condition, atLimit, "i1");
      std::string limitViolation = emitBinary(
          "and", state->getValue().selectedValid, limitCondition, "i1");
      std::string limitOk = emitNot(limitViolation);
      body << "    pyc.assert " << limitOk
           << " {msg = \"feedback_iteration_limit\"}\n";
    }
  }
  for (const auto &entry : roundRobinStates) {
    const RoundRobinPycState &state = entry.getValue();
    std::string acceptedValue;
    if (auto accepted = firingAccepted.find(state.owner);
        accepted != firingAccepted.end())
      acceptedValue = accepted->getValue();
    else if (auto accepted = tableSelectionAccepted.find(state.owner);
             accepted != tableSelectionAccepted.end())
      acceptedValue = accepted->getValue();
    if (acceptedValue.empty())
      return pycError("round-robin Table selection acceptance is missing");
    std::string advance =
        emitBinary("and", acceptedValue, state.anyCandidate, "i1");
    body << "    pyc.assign " << state.nextWire << ", " << state.nextCandidate
         << " : " << state.type << "\n";
    body << "    pyc.assign " << state.enableWire << ", " << advance
         << " : i1\n";
  }
  for (const TablePlan &table : plan.tables) {
    auto state = tableStates.find(table.name);
    if (state == tableStates.end())
      return pycError("Table register-bank state is missing");
    for (uint64_t slot = 0; slot < table.entries; ++slot) {
      std::string next = state->getValue().value[slot];
      std::string enabled = emitConstant(0, "i1");
      std::string firingNext = state->getValue().value[slot];
      std::string firingEnabled = emitConstant(0, "i1");
      for (const QueueBlockPlan &block : plan.blocks) {
        if (block.kind != "firing" || block.stateWrites.empty())
          continue;
        auto accepted = firingAccepted.find(block.name);
        if (accepted == firingAccepted.end())
          return pycError("stateful firing acceptance is missing");
        auto values = firingExpressionValuesByBlock.find(block.name);
        if (values == firingExpressionValuesByBlock.end())
          return pycError("stateful firing expression values are missing");
        auto lookup =
            [&](llvm::StringRef identity) -> llvm::Expected<std::string> {
          auto found = values->getValue()->find(identity);
          if (found == values->getValue()->end())
            return pycError("stateful firing value identity is missing: '" +
                            identity + "'");
          return found->getValue();
        };
        for (const StateWritePlan &write : block.stateWrites) {
          if (write.table != table.name)
            continue;
          auto index = lookup(write.index);
          auto value = lookup(write.value);
          auto present = lookup(write.present);
          if (!index)
            return index.takeError();
          if (!value)
            return value.takeError();
          if (!present)
            return present.takeError();
          auto indexType = yieldedType(
              block, write.index,
              block.inputs.empty()
                  ? table.entryType
                  : findQueue(plan, block.inputs.front())->payloadType);
          auto indexPycType =
              indexType ? pycType(plan, *indexType)
                        : llvm::Expected<std::string>(indexType.takeError());
          if (!indexPycType)
            return indexPycType.takeError();
          if (!pycIntegerCanRepresent(slot, *indexPycType))
            continue;
          std::string slotValue = emitConstant(slot, *indexPycType);
          std::string atSlot =
              emitBinary("eq", *index, slotValue, *indexPycType);
          std::string selected =
              emitBinary("and", accepted->getValue(), *present, "i1");
          selected = emitBinary("and", selected, atSlot, "i1");
          firingNext =
              emitMux(selected, *value, firingNext, state->getValue().type);
          firingEnabled = emitBinary("or", firingEnabled, selected, "i1");
        }
      }
      for (const QueueBlockPlan &block : plan.blocks) {
        if (block.kind != "table_masked_write" || block.table != table.name)
          continue;
        auto masked = tableMaskedWriteValues.find(block.name);
        if (masked == tableMaskedWriteValues.end() || block.expressions.empty())
          return pycError("masked Table write values are missing");
        const QueueExpressionPlan &maskExpression = block.expressions.front();
        auto match = llvm::find_if(
            plan.tableMatches, [&](const TableMatchPlan &candidate) {
              return candidate.name == maskExpression.field &&
                     candidate.table == table.name;
            });
        if (match == plan.tableMatches.end())
          return pycError("masked Table write match plan is missing");
        std::vector<uint64_t> tableIndices;
        if (!match->hasDomainProjection) {
          for (uint64_t index = 0; index < table.entries; ++index)
            tableIndices.push_back(index);
        } else {
          uint64_t domainEntries = 1;
          for (uint64_t extent : match->domainShape)
            domainEntries *= extent;
          for (uint64_t ordinal = 0; ordinal < domainEntries; ++ordinal) {
            uint64_t remaining = ordinal;
            uint64_t tableIndex = match->domainOffset;
            for (size_t axis = match->domainShape.size(); axis-- > 0;) {
              const uint64_t coordinate = remaining % match->domainShape[axis];
              remaining /= match->domainShape[axis];
              tableIndex += coordinate * match->domainStrides[axis];
            }
            tableIndices.push_back(tableIndex);
          }
        }
        auto position = llvm::find(tableIndices, slot);
        if (position == tableIndices.end())
          continue;
        const uint64_t ordinal = std::distance(tableIndices.begin(), position);
        auto maskType = pycType(plan, maskExpression.type);
        if (!maskType)
          return maskType.takeError();
        std::string selectedBit = newValue();
        body << "    " << selectedBit << " = pyc.extract "
             << masked->getValue().mask << " {lsb = " << ordinal
             << "} : " << *maskType << " -> i1\n";
        std::string selected =
            emitBinary("and", masked->getValue().enabled, selectedBit, "i1");
        if (auto arbitration = blockArbitrationAllowed.find(block.name);
            arbitration != blockArbitrationAllowed.end())
          selected = emitBinary("and", selected, arbitration->getValue(), "i1");
        QueueBlockPlan valueBlock = expressionSlice(block, block.yields[2]);
        auto proposed = emitTransform(
            plan, valueBlock, {state->getValue().value[slot]},
            {table.entryType}, 0, nextValue, body, nullptr, &tableStateValues,
            nullptr, {}, nullptr, nullptr, &sharedTableExpressionValues);
        if (!proposed)
          return proposed.takeError();
        auto merged = emitTableFieldMerge(next, *proposed, table.entryType,
                                          block.writeFields);
        if (!merged)
          return merged.takeError();
        next = emitMux(selected, *merged, next, state->getValue().type);
        enabled = emitBinary("or", enabled, selected, "i1");
      }
      for (const char *mode : {"field", "replace"}) {
        if (llvm::StringRef(mode) == "replace") {
          next =
              emitMux(firingEnabled, firingNext, next, state->getValue().type);
          enabled = emitBinary("or", enabled, firingEnabled, "i1");
        }
        for (const QueueBlockPlan &block : plan.blocks) {
          if (block.kind != "table_write" || block.table != table.name ||
              block.writeMode != mode)
            continue;
          auto accepted = tableWriteAccepted.find(block.name);
          auto values = tableWriteExpressionValues.find(block.name);
          if (accepted == tableWriteAccepted.end() ||
              values == tableWriteExpressionValues.end())
            return pycError("Table write acceptance or values are missing");
          auto lookup =
              [&](llvm::StringRef identity) -> llvm::Expected<std::string> {
            auto found = values->getValue()->find(identity);
            if (found == values->getValue()->end())
              return pycError("Table write value identity is missing: '" +
                              identity + "'");
            return found->getValue();
          };
          auto index = lookup(block.yields[0]);
          auto present = lookup(block.yields[1]);
          auto value = lookup(block.yields[2]);
          if (!index)
            return index.takeError();
          if (!present)
            return present.takeError();
          if (!value)
            return value.takeError();
          auto indexType = yieldedType(
              block, block.yields[0],
              block.inputs.empty()
                  ? table.entryType
                  : findQueue(plan, block.inputs.front())->payloadType);
          auto indexPycType =
              indexType ? pycType(plan, *indexType)
                        : llvm::Expected<std::string>(indexType.takeError());
          if (!indexPycType)
            return indexPycType.takeError();
          if (!pycIntegerCanRepresent(slot, *indexPycType))
            continue;
          std::string slotValue = emitConstant(slot, *indexPycType);
          std::string atSlot =
              emitBinary("eq", *index, slotValue, *indexPycType);
          std::string selected =
              emitBinary("and", accepted->getValue(), *present, "i1");
          if (auto arbitration = blockArbitrationAllowed.find(block.name);
              arbitration != blockArbitrationAllowed.end())
            selected =
                emitBinary("and", selected, arbitration->getValue(), "i1");
          selected = emitBinary("and", selected, atSlot, "i1");
          std::string proposed = *value;
          if (block.writeMode == "field") {
            auto merged = emitTableFieldMerge(next, proposed, table.entryType,
                                              block.writeFields);
            if (!merged)
              return merged.takeError();
            proposed = std::move(*merged);
          }
          next = emitMux(selected, proposed, next, state->getValue().type);
          enabled = emitBinary("or", enabled, selected, "i1");
        }
      }
      body << "    pyc.assign " << state->getValue().next[slot] << ", " << next
           << " : " << state->getValue().type << "\n";
      body << "    pyc.assign " << state->getValue().enable[slot] << ", "
           << enabled << " : i1\n";
    }
  }
  for (auto [index, sink] : llvm::enumerate(sinks))
    body << "    pyc.assign " << readyWires[sink->inputs.front()] << ", "
         << outputName(index, "ready") << " : i1\n";
  for (const QueueBlockPlan *observation : observations) {
    const QueuePlan *queue = findQueue(plan, observation->inputs.front());
    auto type = queue ? pycType(plan, queue->payloadType)
                      : llvm::Expected<std::string>(
                            pycError("observation Queue is missing"));
    if (!type)
      return type.takeError();
    std::string alias = newValue();
    body << "    " << alias << " = pyc.alias "
         << outputData[observation->inputs.front()] << " {pyc.name = \""
         << observation->name << "\"} : " << *type << "\n";
  }

  llvm::StringRef top = plan.system;
  std::vector<std::string> arguments = {"%clk: !pyc.clock", "%rst: !pyc.reset"};
  std::vector<std::string> argumentNames = {"clk", "rst"};
  for (size_t index = 0; index < sources.size(); ++index) {
    arguments.push_back(inputName(index, "valid") + ": i1");
    arguments.push_back(inputName(index, "data") + ": " +
                        inputPortTypes[index]);
    argumentNames.push_back(inputName(index, "valid").substr(1));
    argumentNames.push_back(inputName(index, "data").substr(1));
  }
  for (size_t index = 0; index < sinks.size(); ++index) {
    arguments.push_back(outputName(index, "ready") + ": i1");
    argumentNames.push_back(outputName(index, "ready").substr(1));
  }
  std::vector<std::string> resultTypes;
  std::vector<std::string> resultNames;
  std::vector<std::string> returnValues;
  for (auto [index, sink] : llvm::enumerate(sinks)) {
    resultTypes.push_back("i1");
    resultTypes.push_back(outputPortTypes[index]);
    resultNames.push_back(outputName(index, "valid").substr(1));
    resultNames.push_back(outputName(index, "data").substr(1));
    returnValues.push_back(outputValid[sink->inputs.front()]);
    returnValues.push_back(outputData[sink->inputs.front()]);
  }
  for (auto [index, source] : llvm::enumerate(sources)) {
    resultTypes.push_back("i1");
    resultNames.push_back(inputName(index, "ready").substr(1));
    returnValues.push_back(inputReady[source->outputs.front()]);
  }
  auto writeList =
      [](std::ostringstream &stream, const std::vector<std::string> &values,
         llvm::StringRef prefix = {}, llvm::StringRef suffix = {}) {
        for (auto [index, value] : llvm::enumerate(values)) {
          if (index)
            stream << ", ";
          stream << prefix.str() << value << suffix.str();
        }
      };
  std::ostringstream output;
  output << "module attributes {pyc.top = @" << top.str()
         << ", pyc.frontend.contract = \"pycircuit\"} {\n  func.func @"
         << top.str() << '(';
  writeList(output, arguments);
  output << ") -> (";
  writeList(output, resultTypes);
  output << ") attributes {arg_names = [";
  writeList(output, argumentNames, "\"", "\"");
  output << "], result_names = [";
  writeList(output, resultNames, "\"", "\"");
  output << "], pyc.value_params = [], pyc.value_param_types = [], "
            "pyc.kind = \"module\", pyc.inline = \"false\", "
            "pyc.params = \"{}\", pyc.base = \""
         << top.str() << "\", pyc.struct.metrics = \"" << kStructMetrics.str()
         << "\", pyc.struct.collections = \"[]\"} {\n"
         << body.str() << "    func.return ";
  writeList(output, returnValues);
  output << " : ";
  writeList(output, resultTypes);
  output << "\n  }\n}\n";
  return output.str();
}

} // namespace acir::codegen

// SPDX-License-Identifier: BSD-3-Clause

FailureOr<std::string>
BehavioralEmitter::expression(const gfir::StructuredWire &wire,
                             const gfir::StructuredDefinition &definition,
                             StringRef instancePrefix,
                             const gfir::StructuredWire *packedTable) {
  auto *op = wire.endpoint.value.getDefiningOp();
  SmallVector<std::string> args;
  for (unsigned dep : wire.dependencies)
    args.push_back(wireName(dep));
  if (wire.call) {
    auto member = instanceName(wire.call->receiver);
    if (failed(member))
      return failure();
    if (program.definitions[wire.call->definition].outputs[wire.call->output].requiresPreparation)
      args.clear();
    usedOutputs[wire.call->definition].set(wire.call->output);
    return instancePrefix.str() + *member + "." + functionName(wire.call->output) +
           "(" + join(args, ", ") + ")";
  }
  if (wire.fifoQuery) {
    auto [index, query] = *wire.fifoQuery;
    const auto &fifo = definition.clocked->fifos[index];
    auto name = instanceName(fifo.instance);
    if (failed(name))
      return failure();
    std::string read = instancePrefix.str() + *name;
    if (query == gfir::StructuredFifoQuery::Data)
      return graphLeaf(wire, read + ".data()");
    if (query == gfir::StructuredFifoQuery::Valid)
      return read + ".valid().packed()";
    auto ready = fifo.downstreamPop
        ? recipient(definition, fifo)
        : FailureOr<std::string>(levelName.str() + "(false)");
    if (failed(ready))
      return failure();
    return read + ".ready(" + *ready + ").packed()";
  }
  if (wire.memory) {
    const auto &memory = definition.clocked->memories[*wire.memory];
    auto name = instanceName(memory.instance);
    if (failed(name))
      return failure();
    std::string read = instancePrefix.str() + *name;
    if (memory.asynchronous()) {
      auto raw = memory.raw;
      auto address = graphWire(raw.getInputs().front(), definition, raw);
      if (failed(address))
        return failure();
      read += ".read(" + *address + ")";
    } else {
      unsigned port = cast<OpResult>(wire.endpoint.value).getResultNumber();
      read += ".result(" + std::to_string(port) + ")";
    }
    return graphLeaf(wire, read);
  }
  if (wire.state) {
    if (auto field = recordField(definition, wire, instancePrefix))
      return *field;
    return graphLeaf(wire, instancePrefix.str() + stateName(*wire.state) + ".value()");
  }
  if (wire.ruleResult) {
    auto [rule, result] = *wire.ruleResult;
    return graphLeaf(wire, frameName(rule) + "." + resultName(result));
  }
  if (packedTable) {
    auto get = cast<ac::TableGetOp>(op);
    auto key = graphWire(get.getIndex(), definition, get, wire.dependencies);
    if (failed(key))
      return failure();
    if (auto field = recordField(definition, *packedTable, instancePrefix))
      return graphLeaf(wire, "std::get<0>(pyc_behavior::lookup(" + *field + ", " + *key + "))");
    auto type = graphType(*packedTable, tableWireTypes);
    auto state = instancePrefix.str() + stateName(*packedTable->state) + ".value()";
    auto value = "std::get<0>(pyc_behavior::lookupPacked<" + type + ">(" +
                 pack(state) + ", " + std::to_string(packedTable->low) + ", " +
                 *key + "))";
    return graphLeaf(wire, value);
  }
  if (isa<ac::TableSplatOp, ac::TableGetOp, ac::TableMapOp>(op)) {
    SmallVector<std::string> operands;
    for (Value value : op->getOperands()) {
      auto operand = graphWire(value, definition, op, wire.dependencies);
      if (failed(operand))
        return failure();
      operands.push_back(std::move(*operand));
    }
    auto value = valueExpression(wire.endpoint.value, operands);
    if (failed(value))
      return failure();
    return graphLeaf(wire, *value);
  }
  if (isa<ac::StructCreateOp, ac::StructGetOp>(op))
    return args.front();
  return bitExpression(op, wire.width, args);
}

FailureOr<std::string>
BehavioralEmitter::bitExpression(Operation *op, unsigned bitWidth,
                                 ArrayRef<std::string> args) {
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op))
    return constant4(constant.getPattern());
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto number = integer(constant.getValue(), analysis, op);
    if (failed(number))
      return failure();
    return carrier(bitWidth) + "::known(" +
           bitWords(number->zextOrTrunc(bitWidth)) + ")";
  }
  auto call = [&](StringRef name) {
    return "gfsim::" + name.str() + "(" + join(args, ", ") + ")";
  };
  if (isa<ac::BitsUnaryOp>(op))
    return call("bit_not");
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op))
    return call(llvm::StringSwitch<StringRef>(binary.getOpcode())
                    .Case("and", "bit_and")
                    .Case("or", "bit_or")
                    .Case("xor", "bit_xor")
                    .Default(binary.getOpcode()));
  if (auto compare = dyn_cast<ac::BitsCompareOp>(op))
    return call("compare<gfsim::ComparePredicate::" +
                compare.getPredicate().upper() + ">");
  if (auto select = dyn_cast<ac::BitsSelectOp>(op)) {
    if (select.getTrueValue() == select.getFalseValue())
      return args[1];
    return call("select");
  }
  if (isa<ac::BitsConcatOp>(op))
    return concatenate(args);
  std::string width = std::to_string(bitWidth);
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = integer(extract.getLow(), analysis, op);
    if (failed(low))
      return failure();
    return "pyc_behavior::slice<" + width + ">(" + args.front() + ", " +
           std::to_string(low->getZExtValue()) + ")";
  }
  if (auto resize = dyn_cast<ac::BitsResizeOp>(op)) {
    auto mode = llvm::StringSwitch<StringRef>(resize.getMode())
                    .Case("trunc", "truncate")
                    .Case("zext", "zero_extend")
                    .Case("sext", "sign_extend")
                    .Default("");
    return call(mode.str() + "<" + width + ">");
  }
  return op->emitOpError("operation lacks behavioral C++ emission");
}

bool BehavioralEmitter::isNativeScalar(Operation &op, bool allowShifts) {
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op)) {
    if (constant.getPattern().find_first_not_of("01") != StringRef::npos)
      return false;
  } else if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
    auto opcode = binary.getOpcode();
    bool shift = opcode == "shl" || opcode == "lshr";
    if (!(allowShifts && shift) && !llvm::is_contained(
            ArrayRef<StringRef>{"add", "sub", "mul", "and", "or", "xor"},
            opcode))
      return false;
  } else if (!isa<ac::BitsConstantOp, ac::BitsUnaryOp, ac::BitsCompareOp,
                  ac::BitsSelectOp, ac::BitsConcatOp, ac::BitsExtractOp,
                  ac::BitsResizeOp>(op)) {
    return false;
  }
  auto fits = [&](Type type) {
    auto width = analysis.getPackedWidth(type, {}, &op);
    return succeeded(width) && *width > 0 && *width <= 64;
  };
  return llvm::all_of(op.getResultTypes(), fits) &&
         llvm::all_of(op.getOperandTypes(), fits);
}

FailureOr<std::string> BehavioralEmitter::nativeScalarExpression(
    Operation &op, unsigned width, ArrayRef<std::string> args) {
  auto operandWidth = [&](Value operand) {
    return analysis.getPackedWidth(operand.getType(), {}, &op);
  };
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto value = integer(constant.getValue(), analysis, &op);
    if (failed(value))
      return failure();
    return std::to_string(value->zextOrTrunc(width).getZExtValue()) + "ULL";
  }
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op)) {
    uint64_t value;
    if (constant.getPattern().getAsInteger(2, value))
      return constant.emitOpError("invalid known scalar literal");
    return std::to_string(value) + "ULL";
  }
  if (isa<ac::BitsUnaryOp>(op))
    return "~" + args.front();
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
    if (binary.getOpcode() == "shl" || binary.getOpcode() == "lshr") {
      StringRef helper = binary.getOpcode() == "shl" ? "shiftLeft" : "shiftRight";
      return "pyc_behavior::" + helper.str() + "<" + std::to_string(width) + ">(" +
             join(args, ", ") + ")";
    }
    auto token = llvm::StringSwitch<StringRef>(binary.getOpcode())
                     .Case("add", "+").Case("sub", "-").Case("mul", "*")
                     .Case("and", "&").Case("or", "|").Case("xor", "^").Default("");
    return args[0] + " " + token.str() + " " + args[1];
  }
  if (auto compare = dyn_cast<ac::BitsCompareOp>(op)) {
    auto predicate = compare.getPredicate();
    std::string lhs = args[0], rhs = args[1];
    if (predicate.starts_with("s")) {
      auto bits = operandWidth(compare.getLhs());
      if (failed(bits))
        return failure();
      auto sign = "(std::uint64_t{1} << " + std::to_string(*bits - 1) + ")";
      lhs = "(" + lhs + " ^ " + sign + ")";
      rhs = "(" + rhs + " ^ " + sign + ")";
    }
    if (predicate.starts_with("u") || predicate.starts_with("s"))
      predicate = predicate.drop_front();
    auto token = llvm::StringSwitch<StringRef>(predicate)
                     .Cases({"eq", "case_eq"}, "==").Case("ne", "!=")
                     .Case("lt", "<").Case("le", "<=").Case("gt", ">").Case("ge", ">=")
                     .Default("");
    return lhs + " " + token.str() + " " + rhs;
  }
  if (auto select = dyn_cast<ac::BitsSelectOp>(op)) {
    if (select.getTrueValue() == select.getFalseValue())
      return args[1];
    return args[0] + " ? " + args[1] + " : " + args[2];
  }
  if (auto concat = dyn_cast<ac::BitsConcatOp>(op)) {
    std::string result = args.front();
    for (unsigned i = 1; i < args.size(); ++i) {
      auto bits = operandWidth(concat.getOperand(i));
      if (failed(bits))
        return failure();
      result = "((" + result + " << " + std::to_string(*bits) + ") | " + args[i] + ")";
    }
    return result;
  }
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = integer(extract.getLow(), analysis, &op);
    if (failed(low))
      return failure();
    return args.front() + " >> " + std::to_string(low->getZExtValue());
  }
  auto resize = cast<ac::BitsResizeOp>(op);
  if (resize.getMode() == "sext") {
    auto bits = operandWidth(resize.getInput());
    if (failed(bits))
      return failure();
    return "pyc_behavior::extendSign<" + std::to_string(*bits) + ">(" + args.front() + ")";
  }
  return args.front();
}

FailureOr<bool> BehavioralEmitter::emitScalarRegion(
    raw_ostream &out, ArrayRef<Operation *> operations,
    llvm::DenseMap<Value, std::string> &values) {
  llvm::SmallDenseSet<Operation *> members(operations.begin(), operations.end());
  llvm::SmallDenseSet<Value> seenInputs;
  SmallVector<Value> inputs, outputs;
  for (Operation *op : operations) {
    for (Value input : op->getOperands())
      if (!members.contains(input.getDefiningOp()) && seenInputs.insert(input).second)
        inputs.push_back(input);
    Value result = op->getResult(0);
    if (llvm::any_of(result.getUsers(),
                     [&](Operation *user) { return !members.contains(user); }))
      outputs.push_back(result);
  }
  if (inputs.empty() || outputs.empty())
    return false;

  llvm::DenseMap<Value, std::string> local;
  SmallVector<std::string> parameters, arguments, known, exported;
  for (auto [index, input] : llvm::enumerate(inputs)) {
    auto name = "pyc_scalar_input_" + std::to_string(index);
    parameters.push_back("const auto &" + name);
    arguments.push_back(pack(values.lookup(input)));
    known.push_back(name + ".isFullyKnown()");
    local.try_emplace(input, std::move(name));
  }
  for (Value output : outputs) {
    exported.push_back(wireName(values.size()));
    values.try_emplace(output, exported.back());
  }
  llvm::DenseMap<Value, unsigned> widths;
  for (Operation *op : operations) {
    Value result = op->getResult(0);
    auto width = analysis.getPackedWidth(result.getType(), {}, op);
    if (failed(width))
      return failure();
    widths.try_emplace(result, *width);
    local.try_emplace(result, "pyc_scalar_" + std::to_string(local.size()));
  }

  auto emit = [&](bool native) -> LogicalResult {
    llvm::DenseMap<Value, std::string> singleUse;
    for (Operation *op : operations) {
      Value result = op->getResult(0);
      unsigned width = widths.lookup(result);
      SmallVector<std::string> operands;
      for (Value input : op->getOperands()) {
        if (auto expression = singleUse.find(input); expression != singleUse.end()) {
          operands.push_back(expression->second);
          continue;
        }
        auto argument = local.lookup(input);
        if (native && seenInputs.contains(input))
          argument += ".value().value()";
        operands.push_back(std::move(argument));
      }
      auto expression = native ? nativeScalarExpression(*op, width, operands)
                               : bitExpression(op, width, operands);
      if (failed(expression))
        return failure();
      if (native) {
        *expression = "pyc_behavior::limitBits<" + std::to_string(width) + ">(" +
                      *expression + ")";
        // Expressions used only in one arm stay inside that arm's conditional.
        if (result.hasOneUse() && members.contains(*result.getUsers().begin()) &&
            !llvm::is_contained(outputs, result)) {
          singleUse.try_emplace(result, std::move(*expression));
          continue;
        }
      }
      out << "        const auto " << local.lookup(result) << " = " << *expression << ";\n";
    }
    SmallVector<std::string> returned;
    for (Value result : outputs) {
      unsigned width = widths.lookup(result);
      auto value = local.lookup(result);
      if (native)
        value = carrier(width) + "::known(gfsim::Bits<" + std::to_string(width) + ">{" +
                value + "})";
      auto type = typedWire(result.getType(), result.getDefiningOp());
      if (failed(type))
        return failure();
      returned.push_back(unpack(*type, value));
    }
    out << "        return std::tuple{" << join(returned, ", ") << "};\n";
    return success();
  };

  out << "    const auto [" << join(exported, ", ") << "] = []("
      << join(parameters, ", ") << ") {\n"
      << "      if (" << join(known, " && ") << ") {\n";
  if (failed(emit(true)))
    return failure();
  out << "      }\n      return pyc_behavior::evaluateFourState([]("
      << join(parameters, ", ") << ") {\n";
  if (failed(emit(false)))
    return failure();
  SmallVector<std::string> names;
  for (Value input : inputs)
    names.push_back(local.lookup(input));
  out << "      }, " << join(names, ", ") << ");\n"
      << "    }(" << join(arguments, ", ") << ");\n";
  return true;
}

LogicalResult BehavioralEmitter::emitWires(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    ArrayRef<unsigned> outputs, StringRef instancePrefix, bool groupSelects,
    SmallVectorImpl<CaptureGraphStep> *schedule, bool emitAvailability) {
  llvm::DenseMap<unsigned, unsigned> packedLookups;
  auto needed = wireClosure(definition, outputs, [&](unsigned index, unsigned input) {
    if (index >= definition.wires.size() || input >= definition.wires.size() ||
        !isa<ac::TableType>(definition.wires[input].type))
      return false;
    auto result = dyn_cast<OpResult>(definition.wires[index].endpoint.value);
    if (!result || result.getResultNumber() != 0)
      return false;
    auto get = dyn_cast<ac::TableGetOp>(result.getOwner());
    if (!get || definition.wires[input].endpoint.value != get.getInput())
      return false;
    if (auto origin = packedStateTable(definition, input)) {
      packedLookups.try_emplace(index, *origin);
      return true;
    }
    return false;
  });
  SelectSchedule selectSchedule;
  if (groupSelects)
    selectSchedule = planSelects(definition, needed);
  llvm::BitVector emitted(definition.order.size());
  for (auto [index, wire] : llvm::enumerate(definition.wires))
    if (isa<BlockArgument>(wire.endpoint.value))
      emitted.set(index);
  llvm::DenseMap<Operation *, SmallVector<unsigned>> callMembers;
  for (unsigned index : definition.order)
    if (index < definition.wires.size() && needed.test(index))
      if (const auto &call = definition.wires[index].call)
        callMembers[call->receiver].push_back(index);
  GroupKnownness knownness;
  for (unsigned index : definition.order) {
    if (!needed.test(index) || emitted.test(index))
      continue;
    if (groupSelects && selectSchedule.delayed.test(index))
      continue;
    if (auto group = selectSchedule.groups.find(index); group != selectSchedule.groups.end()) {
      emitSelectGroup(out, definition, group->second);
      for (unsigned member : group->second.selects)
        emitted.set(member);
      for (const auto &alias : group->second.forwarded)
        emitted.set(alias.first);
      continue;
    }
    if (index >= definition.firstClockedCallNode()) {
      if (schedule)
        schedule->push_back({index, std::nullopt});
      if (failed(emitClockedCall(out, definition,
                                 index - definition.firstClockedCallNode(), instancePrefix)))
        return failure();
      emitted.set(index);
      continue;
    }
    if (index >= definition.wires.size()) {
      if (schedule)
        schedule->push_back({index, std::nullopt});
      if (failed(emitGrant(out, definition, index, instancePrefix)))
        return failure();
      emitted.set(index);
      continue;
    }
    const auto &wire = definition.wires[index];
    if (isa<BlockArgument>(wire.endpoint.value))
      continue;
    auto candidates = wire.call ? ArrayRef<unsigned>(callMembers[wire.call->receiver])
                                : ArrayRef<unsigned>{};
    auto grouped = emitOutputGroup(out, definition, index, candidates, emitted,
                                   instancePrefix, callMembers.size() > 1 ? &knownness : nullptr,
                                   schedule);
    if (failed(grouped))
      return failure();
    if (*grouped)
      continue;
    if (schedule)
      schedule->push_back({index, std::nullopt});
    auto lookup = packedLookups.find(index);
    auto value = expression(wire, definition, instancePrefix,
                            lookup == packedLookups.end() ? nullptr
                                                         : &definition.wires[lookup->second]);
    if (failed(value))
      return failure();
    const bool borrow =
        recordField(definition, wire, instancePrefix).has_value() ||
        (groupSelects && forwardedBits(definition, index).has_value()) ||
        (isa<ac::TableType>(wire.type) &&
         isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp, ac::TableMapOp>(
             wire.endpoint.value.getDefiningOp()));
    out << "    const auto " << (borrow ? "&" : "") << wireName(index)
        << " = " << *value << ";\n";
    if (emitAvailability && wire.call) {
      const auto &child = program.definitions[wire.call->definition];
      const auto &output = child.outputs[wire.call->output];
      if (!child.clocked && !child.wireAvailability.empty() &&
          !child.wireAvailability[output.wire].empty() &&
          !definition.wireAvailability.empty() && !definition.wireAvailability[index].empty()) {
        auto member = instanceName(wire.call->receiver);
        if (failed(member)) return failure();
        SmallVector<std::string> arguments;
        for (unsigned input : wire.dependencies)
          arguments.push_back(wireName(input));
        auto call = cast<gfir::CallOp>(wire.endpoint.value.getDefiningOp());
        for (unsigned ordinal : pureAvailabilityInputs(child, wire.call->output)) {
          const auto &input = child.inputAvailability[ordinal];
          auto actual = call.getInputs()[input.port];
          const auto &leaves = definition.valueWires.find(actual)->second;
          auto leaf = llvm::find_if(leaves, [&](unsigned wire) {
            return definition.wires[wire].endpoint.path == input.path;
          });
          if (leaf == leaves.end()) return failure();
          const auto &sources = definition.wireAvailability.empty()
              ? gfir::StructuredAvailability{} : definition.wireAvailability[*leaf];
          auto available = availabilityExpression(definition, sources, instancePrefix);
          if (failed(available)) return failure();
          arguments.push_back(definition.fourStateAvailability ? *available : levelName.str() + "(" + *available + ")");
        }
        out << "    const auto pyc_availability_call_" << index << " = " << instancePrefix
            << *member << '.' << pureAvailabilityName(wire.call->output) << '('
            << join(arguments, ", ") << ");\n";
      }
    }
    if (emitAvailability) for (auto [ordinal, select] : llvm::enumerate(definition.availabilitySelects)) {
      if (select.wire != index)
        continue;
      auto yes = availabilityExpression(definition, select.yes, instancePrefix);
      auto no = availabilityExpression(definition, select.no, instancePrefix);
      if (failed(yes) || failed(no))
        return failure();
      out << "    const auto pyc_availability_select_" << ordinal << " = pyc_behavior::Level"
          << "::fromPacked(gfsim::select(" << wireName(select.selector) << ", ("
          << *yes << ").packed(), (" << *no << ").packed()));\n";
    }
    emitted.set(index);
  }
  return success();
}

FailureOr<std::optional<unsigned>> BehavioralEmitter::internOutputGroup(
    unsigned index, ArrayRef<unsigned> outputs, bool reuseKnownness) {
  const auto &definition = program.definitions[index];
  reuseKnownness = reuseKnownness && llvm::none_of(definition.wires, [](const auto &wire) {
    return wire.call.has_value();
  });
  auto &groups = outputGroups[index];
  for (auto [ordinal, group] : llvm::enumerate(groups))
    if (group.reuseKnownness == reuseKnownness && llvm::equal(group.outputs, outputs))
      return std::optional<unsigned>(ordinal);
  // Bound specialization count and total added closure size by the existing
  // singleton surface; a refused specialization keeps ordinary emission.
  if (groups.size() >= definition.outputs.size())
    return std::optional<unsigned>();
  OutputGroup group;
  group.reuseKnownness = reuseKnownness;
  group.outputs.append(outputs.begin(), outputs.end());
  SmallVector<unsigned> roots;
  for (unsigned output : outputs) {
    const auto &field = definition.outputs[output];
    roots.push_back(field.wire);
    group.inputs.append(field.inputs.begin(), field.inputs.end());
  }
  auto work = wireClosure(definition, roots).count();
  if (work > groupWorkRemaining[index])
    return std::optional<unsigned>();
  groupWorkRemaining[index] -= work;
  llvm::sort(group.inputs);
  group.inputs.erase(std::unique(group.inputs.begin(), group.inputs.end()), group.inputs.end());
  if (reuseKnownness) {
    // Build the native body and its exact scalar frontier together.
    llvm::raw_string_ostream body(group.nativeBody);
    if (failed(emitKnownOutputGroup(body, definition, group, roots, &group.checkedInputs)))
      return failure();
  }
  groups.push_back(std::move(group));
  return std::optional<unsigned>(groups.size() - 1);
}

FailureOr<bool> BehavioralEmitter::emitOutputGroup(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    unsigned first, ArrayRef<unsigned> candidates, llvm::BitVector &emitted,
    StringRef instancePrefix, GroupKnownness *knownness,
    SmallVectorImpl<CaptureGraphStep> *schedule) {
  const auto &initial = definition.wires[first];
  if (!initial.call)
    return false;
  const auto &call = *initial.call;
  const auto &child = program.definitions[call.definition];
  if (isClocked(child) || child.hasChecks() || !child.inputAvailability.empty())
    return false;
  SmallVector<unsigned> members;
  SmallVector<unsigned> outputs;
  std::map<unsigned, unsigned> frontier;
  for (unsigned index : candidates) {
    if (emitted.test(index))
      continue;
    const auto &wire = definition.wires[index];
    if (!wire.call || wire.call->receiver != call.receiver || wire.call->definition != call.definition)
      continue;
    const auto &output = child.outputs[wire.call->output];
    if (output.requiresPreparation || output.inputs.size() != wire.dependencies.size() ||
        !llvm::all_of(wire.dependencies, [&](unsigned input) { return emitted.test(input); }))
      continue;
    // Actual SSA bindings, rather than port names or declaration order, own
    // the union frontier. Different invocation bindings must remain separate.
    for (auto [input, actual] : llvm::zip(output.inputs, wire.dependencies)) {
      auto [found, inserted] = frontier.emplace(input, actual);
      if (!inserted && found->second != actual)
        return false;
    }
    members.push_back(index);
    outputs.push_back(wire.call->output);
  }
  llvm::sort(outputs);
  outputs.erase(std::unique(outputs.begin(), outputs.end()), outputs.end());
  if (outputs.size() < 2 || !llvm::is_contained(members, first))
    return false;
  auto ordinal = internOutputGroup(call.definition, outputs, knownness != nullptr);
  if (failed(ordinal))
    return failure();
  if (!*ordinal)
    return false;
  const auto &strategy = outputGroups[call.definition][**ordinal];
  auto member = instanceName(call.receiver);
  if (failed(member))
    return failure();
  std::map<unsigned, unsigned> bindings;
  for (unsigned index : members)
    bindings.try_emplace(definition.wires[index].call->output, index);
  SmallVector<std::string> results, arguments;
  for (unsigned output : outputs)
    results.push_back(wireName(bindings.at(output)));
  for (unsigned input : strategy.inputs)
    arguments.push_back(wireName(frontier.at(input)));
  if (!strategy.checkedInputs.empty()) {
    // Each proof belongs to an actual SSA leaf in this evaluation. A native
    // predecessor established known masks for all its results. A predecessor
    // that took the four-state path only supplies its value, so check the mask.
    SmallVector<std::string> known;
    for (unsigned input : strategy.checkedInputs) {
      unsigned actual = frontier.at(input);
      while (auto source = forwardedBits(definition, actual)) {
        if (!emitted.test(*source))
          break;
        actual = *source;
      }
      auto name = inputKnownName(actual);
      if (knownness->checked.insert(actual).second) {
        out << "    const bool " << name << " = ";
        if (auto prior = knownness->nativeResults.find(actual);
            prior != knownness->nativeResults.end())
          out << resultKnownName(prior->second) << " || ";
        out << wireName(actual) << ".isFullyKnown();\n";
      }
      if (!llvm::is_contained(known, name))
        known.push_back(std::move(name));
    }
    arguments.push_back(join(known, " && "));
    results.push_back(resultKnownName(first));
  }
  out << "    const auto [" << join(results, ", ") << "] = " << instancePrefix << *member
      << '.' << outputGroupName(**ordinal) << '(' << join(arguments, ", ") << ");\n";
  for (unsigned index : members) {
    unsigned original = bindings.at(definition.wires[index].call->output);
    if (index != original)
      out << "    const auto &" << wireName(index) << " = " << wireName(original) << ";\n";
    if (!strategy.checkedInputs.empty())
      knownness->nativeResults.try_emplace(index, first);
    emitted.set(index);
  }
  if (schedule) {
    BoundOutputGroup binding{call.definition, **ordinal, call.receiver, {}, std::move(members)};
    for (unsigned input : strategy.inputs)
      binding.inputs.push_back(frontier.at(input));
    schedule->push_back({first, std::move(binding)});
  }
  return true;
}

FailureOr<bool> BehavioralEmitter::emitKnownOutputGroup(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const OutputGroup &group, ArrayRef<unsigned> roots,
    SmallVectorImpl<unsigned> *checkedInputs,
    SmallVectorImpl<unsigned> *nativeTables, bool scalarResult) {
  auto forwarded = [&](unsigned index) {
    auto bits = forwardedBits(definition, index);
    return bits ? bits : forwardedTable(definition, index);
  };
  auto needed = wireClosure(definition, roots, [&](unsigned index, unsigned input) {
    auto source = forwarded(index);
    return source && *source != input;
  });
  for (unsigned root : roots)
    if (!isa<ac::BitsType>(definition.wires[root].type) ||
        definition.wires[root].width > 64)
      return false;
  auto leaf = [&](Value value) -> std::optional<unsigned> {
    auto found = definition.valueWires.find(value);
    if (found == definition.valueWires.end() || found->second.size() != 1)
      return std::nullopt;
    return found->second.front();
  };
  auto tablePort = [&](unsigned index) -> std::optional<unsigned> {
    for (unsigned depth = 0; depth < definition.wires.size(); ++depth) {
      if (isa<BlockArgument>(definition.wires[index].endpoint.value))
        return index;
      auto source = forwardedTable(definition, index);
      if (!source)
        return std::nullopt;
      index = *source;
    }
    return std::nullopt;
  };
  struct TableRead {
    unsigned table, key;
    bool range;
  };
  llvm::DenseMap<unsigned, TableRead> tableReads;
  SmallVector<unsigned> origins(definition.wires.size()), uses(definition.wires.size());
  SmallVector<unsigned> inputs, operations, tables;
  bool readsRows = false;
  for (unsigned index : definition.order) {
    if (!needed.test(index))
      continue;
    const auto &wire = definition.wires[index];
    origins[index] = index;
    if (auto source = forwarded(index)) {
      origins[index] = origins[*source];
      continue;
    }
    if (isa<ac::TableType>(wire.type)) {
      if (scalarResult || !isa<BlockArgument>(wire.endpoint.value) ||
          !llvm::is_contained(group.inputs, index))
        return false;
      tables.push_back(index);
      continue;
    }
    if (!isa<ac::BitsType>(wire.type) || wire.width > 64)
      return false;
    if (isa<BlockArgument>(wire.endpoint.value)) {
      if (!llvm::is_contained(group.inputs, index))
        return false;
      inputs.push_back(index);
    } else {
      auto *op = wire.endpoint.value.getDefiningOp();
      if (auto get = dyn_cast_or_null<ac::TableGetOp>(op)) {
        auto table = cast<ac::TableType>(get.getInput().getType());
        auto source = leaf(get.getInput()), key = leaf(get.getIndex());
        auto size = analysis.getTableSize(table, {}, get);
        auto keyWidth = analysis.getPackedWidth(get.getIndex().getType(), {}, get);
        auto elementWidth = analysis.getPackedWidth(table.getElementType(), {}, get);
        if (table.getShape().size() != 1 || !isa<ac::BitsType>(table.getElementType()) ||
            !source || !key || !tablePort(*source) || failed(size) ||
            failed(keyWidth) || *keyWidth == 0 || *keyWidth >= 64 ||
            (uint64_t{1} << *keyWidth) > *size ||
            failed(elementWidth) || *elementWidth == 0 || *elementWidth > 64)
          return false;
        bool range = cast<OpResult>(wire.endpoint.value).getResultNumber() == 1;
        tableReads.try_emplace(index, TableRead{*source, *key, range});
        readsRows |= !range;
      } else if (!op || !isNativeScalar(*op, /*allowShifts=*/true)) {
        return false;
      }
      operations.push_back(index);
      for (unsigned input : wire.dependencies)
        ++uses[origins[input]];
    }
  }
  // Small singleton bridges already have a caller-side knownness check.
  // Keep their inlined closure smaller than an ordinary output group.
  if (scalarResult ? operations.size() > minScalarOperations
                   : inputs.empty() || operations.size() < minScalarOperations)
    return false;
  for (unsigned root : roots)
    ++uses[origins[root]];

  if (scalarResult) {
    checkedInputs->append(inputs.begin(), inputs.end());
  } else if (nativeTables) {
    checkedInputs->append(inputs.begin(), inputs.end());
    nativeTables->append(tables.begin(), tables.end());
  } else if (checkedInputs) {
    checkedInputs->append(inputs.begin(), inputs.end());
    out << "    if (pyc_inputs_known) {\n";
  } else {
    SmallVector<std::string> known;
    for (unsigned input : inputs)
      known.push_back(wireName(input) + ".isFullyKnown()");
    out << "    if (" << join(known, " &&\n        ") << ") {\n";
  }
  if (readsRows && !nativeTables)
    out << "      do {\n";
  SmallVector<std::string> expressions(definition.wires.size());
  for (unsigned input : inputs) {
    if (scalarResult) {
      expressions[input] = wireName(input);
      continue;
    }
    expressions[input] = "pyc_native_" + std::to_string(input);
    out << "      const auto " << expressions[input] << " = ";
    if (nativeTables)
      out << "static_cast<std::uint64_t>(pyc_inputs." << wireName(input) << ')';
    else
      out << wireName(input) << ".value().value()";
    out << ";\n";
  }
  for (unsigned index : operations) {
    const auto &wire = definition.wires[index];
    if (auto read = tableReads.find(index); read != tableReads.end()) {
      if (read->second.range) {
        expressions[index] = "1ULL";
      } else {
        auto row = "pyc_row_" + std::to_string(index);
        out << "      const auto &" << row << " = ";
        if (nativeTables)
          out << "pyc_inputs.";
        out << wireName(origins[read->second.table]) << ".element("
            << expressions[origins[read->second.key]] << ");\n"
            << "      if (!" << row << ".isFullyKnown()) "
            << (nativeTables ? "return false" : "break") << ";\n";
        expressions[index] = "pyc_native_" + std::to_string(index);
        out << "      const auto " << expressions[index] << " = "
            << row << ".value().value();\n";
      }
      continue;
    }
    SmallVector<std::string> operands;
    for (unsigned input : wire.dependencies)
      operands.push_back(expressions[origins[input]]);
    auto expression = nativeScalarExpression(*wire.endpoint.value.getDefiningOp(),
                                             wire.width, operands);
    if (failed(expression))
      return failure();
    *expression = "pyc_behavior::limitBits<" + std::to_string(wire.width) + ">(" +
                  *expression + ")";
    // Exclusive scalar work can stay in a select arm. Bound each substitution
    // so long dependency chains do not become deeply nested C++ expressions.
    if (uses[index] == 1 && expression->size() <= 512) {
      expressions[index] = std::move(*expression);
      continue;
    }
    expressions[index] = "pyc_native_" + std::to_string(index);
    out << "      const auto " << expressions[index] << " = " << *expression << ";\n";
  }
  if (scalarResult) {
    auto root = roots.front();
    out << "        return static_cast<" << nativeInteger(definition.wires[root].width)
        << ">(" << expressions[origins[root]] << ");\n";
    return true;
  }
  SmallVector<std::string> results;
  for (unsigned root : roots) {
    unsigned width = definition.wires[root].width;
    if (nativeTables)
      results.push_back("static_cast<" + nativeInteger(width).str() + ">(" +
                        expressions[origins[root]] + ")");
    else
      results.push_back(carrier(width) + "::known(gfsim::Bits<" + std::to_string(width) +
                        ">{" + expressions[origins[root]] + "})");
  }
  if (nativeTables) {
    out << "      pyc_results = {" << join(results, ", ") << "};\n"
        << "      return true;\n";
    return true;
  }
  if (checkedInputs)
    results.push_back("true");
  out << "      return {" << join(results, ", ") << "};\n";
  if (readsRows)
    out << "      } while (false);\n";
  out << "    }\n";
  return true;
}

#include "BehavioralNativeGroups.inc"

LogicalResult BehavioralEmitter::planNativeOutputInlining() {
  // The memoized parent has multiple native calls, so its prepare is
  // out-of-line. Include a definition only where that source can use it.
  if (inlineRootWork(program))
    return success();
  constexpr std::size_t maxBytesPerCaller = 256 * 1024;
  llvm::StringMap<std::size_t> bytesUsed;
  for (auto [index, entries] : llvm::enumerate(outputGroups)) {
    for (auto &output : entries) {
      auto *native = output.nativeEntry.get();
      if (!native || native->callSites != 1)
        continue;
      auto caller = getGroup(native->soleCaller);
      auto owner = getGroup(program.definitions[index].module);
      if (failed(caller) || failed(owner))
        return failure();
      auto &used = bytesUsed[(*caller)->sourcePath];
      if (native->body.size() > maxBytesPerCaller - used)
        continue;
      used += native->body.size();
      native->inlineBody = true;
      if (*caller == *owner)
        continue;
      auto include = "#include \"" + (*owner)->headerPath + "\"\n";
      if (!StringRef((*caller)->source).contains(include))
        (*caller)->source.insert(0, include);
    }
  }
  return success();
}

LogicalResult BehavioralEmitter::emitOutputGroupBody(unsigned index,
                                                    unsigned ordinal) {
  const auto &definition = program.definitions[index];
  auto group = getGroup(definition.module);
  if (failed(group))
    return failure();
  llvm::raw_string_ostream out((*group)->source);
  bool groupSelects = !isClocked(definition) &&
      llvm::none_of(definition.wires, [](const gfir::StructuredWire &wire) {
        return wire.call.has_value();
      });
  const auto &outputs = outputGroups[index][ordinal];
  groupSignature(out, definition, tableWireTypes, outputs, ordinal,
                 moduleName(index) + "::");
  out << " {\n";
  SmallVector<unsigned> roots;
  SmallVector<std::string> values;
  for (unsigned output : outputs.outputs) {
    unsigned wire = definition.outputs[output].wire;
    roots.push_back(wire);
    values.push_back(wireName(wire));
  }
  bool specialized = !outputs.checkedInputs.empty();
  if (specialized) {
    out << outputs.nativeBody;
  } else if (groupSelects) {
    auto strategy = emitKnownOutputGroup(out, definition, outputs, roots);
    if (failed(strategy))
      return failure();
    specialized = *strategy;
  }
  if (specialized)
    out << "    return pyc_behavior::evaluateFourState([&]() -> "
        << outputGroupType(ordinal) << " {\n";
  if (failed(emitWires(out, definition, roots, {}, groupSelects)))
    return failure();
  out << "    return {" << join(values, ", ") << "};\n";
  if (specialized)
    out << "    });\n";
  out << "}\n";
  if (const auto *native = outputs.nativeEntry.get()) {
    llvm::raw_string_ostream body(native->inlineBody ? (*group)->header : (*group)->source);
    if (native->inlineBody)
      body << nativeInlineAttribute << "inline ";
    body << "bool " << moduleName(index) << "::" << outputGroupName(ordinal)
         << "_known(const " << nativeGroupInput(ordinal) << " &pyc_inputs, "
         << nativeGroupResult(ordinal) << " &pyc_results) const {\n"
         << native->body << "}\n";
  }
  return success();
}

LogicalResult BehavioralEmitter::emitOutputCallables() {
  SmallVector<llvm::BitVector> emittedOutputs;
  SmallVector<unsigned> nextGroup(program.definitions.size());
  for (const auto &definition : program.definitions)
    emittedOutputs.emplace_back(definition.outputs.size());
  bool pending = true;
  while (pending) {
    pending = false;
    // Preserve the old singleton traversal order among reached callables. A
    // body's actual grouped/fallback choices may discover more singletons.
    bool foundOutput;
    do {
      foundOutput = false;
      for (unsigned index = 0; index < program.definitions.size(); ++index) {
        for (unsigned result = 0; result < usedOutputs[index].size(); ++result) {
          if (!usedOutputs[index].test(result) || emittedOutputs[index].test(result))
            continue;
          auto group = getGroup(program.definitions[index].module);
          if (failed(group))
            return failure();
          llvm::raw_string_ostream out((*group)->source);
          if (failed(emitOutput(out, index, result)))
            return failure();
          emittedOutputs[index].set(result);
          foundOutput = pending = true;
        }
      }
    } while (foundOutput);
    // Definitions are callee-before-caller. Descending discovery allows each
    // scheduled group to register nested groups before its child is emitted.
    for (unsigned index = program.definitions.size(); index-- != 0;) {
      for (unsigned &ordinal = nextGroup[index];
           ordinal < outputGroups[index].size(); ++ordinal) {
        if (failed(emitOutputGroupBody(index, ordinal)))
          return failure();
        pending = true;
      }
    }
  }
  return success();
}

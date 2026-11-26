// SPDX-License-Identifier: BSD-3-Clause

LogicalResult BehavioralEmitter::emitClockedCall(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    unsigned index, StringRef instancePrefix) {
  const auto &call = definition.clockedCalls[index];
  const auto &child = program.definitions[call.definition];
  auto operation = call.operation;
  auto member = instanceName(call.receiver);
  if (failed(member))
    return failure();
  out << "    " << instancePrefix << *member << ".prepare(";
  if (call.domainInputs) {
    auto entry = program.root;
    auto policy = entry.getExecutionPolicyAttr();
    bool ifOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
    out << "pyc_behavior::Level::fromPacked(" << wireName(call.domainInputs->first)
        << "), gfsim::interpretReset(pyc_behavior::Level::fromPacked("
        << wireName(call.domainInputs->second) << "), gfsim::ResetSemantics::"
        << (ifOne ? "IfOne" : "Strict") << ')';
  } else {
    out << "pyc_clock, pyc_reset";
  }
  if (const auto *whole = wholeInput(child)) {
    auto actual = operation.getInputs()[whole->argument.getArgNumber()];
    auto input = graphWire(actual, definition, operation);
    if (failed(input))
      return failure();
    out << ", " << *input;
  } else {
    for (unsigned input : child.preparationInputs) {
      const auto &endpoint = child.wires[input].endpoint;
      unsigned argument = cast<BlockArgument>(endpoint.value).getArgNumber();
      auto actual = operation.getInputs()[argument];
      const auto &leaves = definition.valueWires.find(actual)->second;
      auto leaf = llvm::find_if(leaves, [&](unsigned wire) {
        return definition.wires[wire].endpoint.path == endpoint.path;
      });
      if (leaf == leaves.end())
        return operation.emitOpError("clocked child preparation input is unresolved");
      out << ", " << wireName(*leaf);
    }
  }
  for (const auto &input : child.inputAvailability) {
    auto actual = operation.getInputs()[input.port];
    const auto &leaves = definition.valueWires.find(actual)->second;
    auto leaf = llvm::find_if(leaves, [&](unsigned wire) {
      return definition.wires[wire].endpoint.path == input.path;
    });
    if (leaf == leaves.end()) return failure();
    const auto &sources = definition.wireAvailability.empty()
        ? gfir::StructuredAvailability{} : definition.wireAvailability[*leaf];
    auto available = availabilityExpression(definition, sources, instancePrefix);
    if (failed(available))
      return failure();
    out << ", " << (definition.fourStateAvailability ? *available
                      : levelName.str() + "(" + *available + ")");
  }
  if (hasIncomingWrites(child)) {
    SmallVector<std::string> candidates;
    for (const auto &incoming : child.incomingStates) {
      if (!incoming.writable)
        continue;
      auto route = incoming.route;
      route.path.insert(route.path.begin(), call.receiver);
      auto slot = llvm::find_if(definition.childStates, [&](const auto &state) {
        return state.route == route && state.writer;
      });
      if (slot != definition.childStates.end()) {
        auto frame = frameName(*slot->writer);
        candidates.push_back(frame + ".pyc_running ? &" + frame + "." +
                             childCandidateName(slot - definition.childStates.begin()) + " : nullptr");
        continue;
      }
      auto parent = llvm::find_if(definition.incomingStates, [&](const auto &state) {
        return state.route == route && state.writable;
      });
      candidates.push_back(parent == definition.incomingStates.end() ? "nullptr" :
          "pyc_incoming." + incomingCandidateName(parent - definition.incomingStates.begin()));
    }
    out << ", {" << join(candidates, ", ") << '}';
  }
  out << ");\n";
  return success();
}

LogicalResult BehavioralEmitter::emitTemporalWork(
    raw_ostream &out, unsigned module) {
  const auto &definition = program.definitions[module];
  const auto &wide = wholeOutputs[module];
  SmallVector<unsigned> needed;
  if (definition.hasChecks())
    out << "    pyc_checks_valid = false;\n"
        << "    pyc_failed_rule = " << noCheck << ";\n"
        << "    pyc_failed_check = " << noCheck << ";\n"
        << "    pyc_sampling_fault = gfsim::SampledClockFault::None;\n";
  for (const auto &output : definition.outputs)
    if (output.requiresPreparation && !wide.contains(output.port.port))
      needed.push_back(output.wire);
  if (definition.clocked) {
    const auto &plan = *definition.clocked;
    if (needsDomain(plan)) {
      out << "    const auto pyc_sample = pyc_domain.evaluate(pyc_clock, pyc_reset);\n";
      if (definition.hasChecks())
        out << "    pyc_sampling_fault = pyc_sample.fault;\n";
      if (llvm::any_of(plan.rules, [](const gfir::StructuredRule &rule) {
            return rule.sampled || rule.resultUses.empty() || rule.afterSnapshot;
          }))
        out << "    const bool pyc_active = pyc_sample.rising && !pyc_sample.resetActive && "
               "pyc_sample.fault == gfsim::SampledClockFault::None;\n";
    }
    for (unsigned index = 0; index < plan.rules.size(); ++index)
      needed.push_back(definition.wires.size() + index);
    auto needInputs = [&](gfir::CallOp call) {
      if (call)
        for (Value input : call.getInputs())
          needed.append(definition.valueWires.find(input)->second);
    };
    for (const auto &fifo : plan.fifos)
      for (auto call : {fifo.rawInput, fifo.rawOutput})
        needInputs(call);
    for (const auto &memory : plan.memories)
      needInputs(memory.raw);
  }
  for (unsigned index = 0; index < definition.clockedCalls.size(); ++index)
    needed.push_back(definition.firstClockedCallNode() + index);
  auto emitted = memoizedRules.test(module) ? emitMemoizedRule(out, definition)
                                            : emitWires(out, definition, needed);
  if (failed(emitted))
    return failure();
  if (definition.clocked && failed(emitPreparation(out, definition, "pyc_clock")))
    return failure();
  for (auto [index, output] : llvm::enumerate(definition.outputs))
    if (output.requiresPreparation && !wide.contains(output.port.port))
      out << "    " << preparedName(index) << " = " << wireName(output.wire) << ";\n";
  auto operation = definition.module;
  for (unsigned port = 0, count = operation.getFunctionType().getNumResults();
       port < count; ++port) {
    auto whole = wide.find(port);
    if (whole != wide.end() && whole->second.snapshot)
      out << "    " << wholeSnapshotName(port) << " = " << whole->second.source << ";\n";
  }
  if (knownRecord(definition))
    out << "    }\n";
  auto assignAvailability = [&](StringRef member,
      ArrayRef<gfir::StructuredAvailabilitySource> sources) -> LogicalResult {
    auto available = availabilityExpression(definition, sources);
    if (failed(available))
      return failure();
    out << "    " << member << " = " << *available << ";\n";
    return success();
  };
  for (const auto &output : definition.outputAvailability)
    if (failed(assignAvailability(availabilityName(output.port), output.sources)))
      return failure();
  for (auto [index, field] : llvm::enumerate(definition.fieldOutputs))
    if (!field.sources.empty() &&
        failed(assignAvailability(fieldAvailabilityName(index), field.sources)))
      return failure();
  if (definition.hasChecks())
    out << "    pyc_checks_valid = true;\n";
  return success();
}

LogicalResult BehavioralEmitter::emitPreparation(raw_ostream &out,
                                                  const gfir::StructuredDefinition &definition,
                                                  StringRef clock) {
  const auto &plan = *definition.clocked;
  const bool hasState = !plan.variables.empty();
  // All Rules have finished issuing requests. Only prepare borrowed inputs
  // here; no owner is committed until SimSystem has accepted the sample.
  for (auto [index, fifo] : llvm::enumerate(plan.fifos)) {
    auto name = instanceName(fifo.instance);
    auto type = typedWire(fifo.element, fifo.instance);
    auto outReady = recipient(definition, fifo);
    if (failed(name) || failed(type) || failed(outReady))
      return failure();
    std::string enabled = levelName.str() + "(false)";
    std::string data = *type + "{}";
    if (auto raw = fifo.rawInput) {
      auto valid = graphWire(raw.getInputs()[0], definition, raw);
      auto payload = graphWire(raw.getInputs()[1], definition, raw);
      if (failed(valid) || failed(payload))
        return failure();
      enabled = *valid;
      data = *payload;
    } else if (fifo.producer) {
      enabled = wireName(definition.wires.size() + *fifo.producer);
      data = frameName(*fifo.producer) + "." + payloadName(index);
      if (hasOrderedFifoBranches(plan.rules[*fifo.producer])) {
        auto frame = frameName(*fifo.producer);
        enabled = levelName.str() + "(" + frame + ".pyc_running && " + frame +
                  ".pyc_push_reached[" + std::to_string(index) + "])";
      }
    }
    out << "    " << *name << ".prepare(" << clock << ", pyc_reset, "
        << enabled << ", " << data << ", " << *outReady << ");\n";
  }
  for (const auto &memory : plan.memories)
    if (failed(prepareMemory(out, definition, memory, clock)))
      return failure();
  if (!hasState) {
    if (needsDomain(plan))
      out << "    if (pyc_sample.fault == gfsim::SampledClockFault::None) "
             "pyc_domain.prepare(pyc_sample);\n";
    return success();
  }

  // FIFO preparation precedes state validation, including when a resource
  // and the sampled domain would both reject the same inputs.
  out << "    pyc_domain.prepare(pyc_sample);\n"
      << "    if (pyc_sample.rising) {\n"
      << "      if (pyc_sample.resetActive) {\n"
      << "        pyc_reset_state();\n"
      << "      } else {\n";
  llvm::DenseMap<unsigned, unsigned> writers;
  for (auto [index, rule] : llvm::enumerate(plan.rules))
    for (unsigned state : rule.writes)
      writers.try_emplace(state, index);
  for (unsigned index = 0; index < plan.variables.size(); ++index) {
    auto state = stateName(index);
    if (auto writer = writers.find(index); writer != writers.end()) {
      std::string enabled = wireName(definition.wires.size() + writer->second);
      if (plan.rules[writer->second].sampled)
        enabled = "pyc_behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + levelName.str() + "(" + frameName(writer->second) + ".pyc_running).packed()))";
      if (plan.conditionalWrites.contains(index))
        enabled = "pyc_behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + frameName(writer->second) + '.' + intentName(index) + ".packed()))";
      out << "        " << state << ".prepare(" << enabled
          << ", " << frameName(writer->second) << '.' << stateName(index) << ");\n";
    } else {
      auto incoming = llvm::find_if(definition.incomingStates, [&](const auto &state) {
        return state.writable && state.route.path.empty() && state.route.state == index;
      });
      if (incoming != definition.incomingStates.end()) {
        out << "        if (const auto *candidate = pyc_incoming."
            << incomingCandidateName(incoming - definition.incomingStates.begin()) << ")\n"
            << "          " << state << ".prepare(" << levelName << "(true), *candidate);\n"
               "        else\n  ";
      }
      out << "        " << state << ".hold();\n";
    }
  }
  out << "      }\n    }\n";
  return success();
}

LogicalResult BehavioralEmitter::prepareMemory(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const gfir::StructuredMemory &memory, StringRef clock) {
  auto name = instanceName(memory.instance);
  if (failed(name))
    return failure();
  if (memory.transactional) {
    const auto &plan = *definition.clocked;
    unsigned memoryIndex = &memory - plan.memories.data();
    auto zero = [&](unsigned width) {
      return "gfsim::wire<gfsim::Bits<" + std::to_string(width) + ">>::known(gfsim::Bits<" + std::to_string(width) + ">{0})";
    };
    auto enabled = [&](gfir::CallOp call, unsigned port) {
      if (!call)
        return std::string("false");
      auto frame = frameName(plan.transactionMemoryCalls.lookup(call).rule);
      return "(" + frame + ".pyc_running && " + frame + "." + memoryPinName(memoryIndex, port, 3) + ")";
    };
    auto pin = [&](gfir::CallOp call, unsigned port, unsigned argument, unsigned width) {
      if (!call)
        return zero(width);
      auto frame = frameName(plan.transactionMemoryCalls.lookup(call).rule);
      return "(" + enabled(call, port) + " ? " + frame + "." +
             memoryPinName(memoryIndex, port, argument) + " : " + zero(width) + ")";
    };
    SmallVector<std::string> reads, addresses;
    for (unsigned port = 0; port < memory.readPorts(); ++port) {
      reads.push_back(levelName.str() + "(" + enabled(memory.requests[port], port) + ")");
      addresses.push_back(pin(memory.requests[port], port, 0, memory.addressWidth));
    }
    auto width = analysis.getPackedWidth(memory.element, {}, memory.instance);
    if (failed(width))
      return failure();
    if (memory.asynchronous()) {
      std::string address = zero(memory.addressWidth);
      if (memory.reader) {
        auto frame = frameName(plan.transactionMemoryCalls.lookup(memory.reader).rule);
        address = "(" + frame + "." + memoryPinName(memoryIndex, 0, 3) + " ? " +
                  frame + "." + memoryPinName(memoryIndex, 0, 0) + " : " + address + ")";
      }
      out << "    " << *name << ".prepare(" << clock << ", pyc_reset, " << address
          << ", " << levelName << '(' << enabled(memory.writer, 2) << "), "
          << pin(memory.writer, 2, 0, memory.addressWidth) << ", "
          << pin(memory.writer, 2, 1, *width) << ", " << pin(memory.writer, 2, 2, memory.maskWidth)
          << ");\n";
      return success();
    }
    out << "    " << *name << ".prepare(" << clock << ", pyc_reset, {" << join(reads, ", ")
        << "}, {" << join(addresses, ", ") << "}, " << levelName << '(' << enabled(memory.writer, 2)
        << "), " << pin(memory.writer, 2, 0, memory.addressWidth) << ", "
        << pin(memory.writer, 2, 1, *width) << ", " << pin(memory.writer, 2, 2, memory.maskWidth)
        << ");\n";
    return success();
  }
  auto raw = memory.raw;
  SmallVector<std::string> inputs;
  for (Value value : raw.getInputs()) {
    auto input = graphWire(value, definition, raw);
    if (failed(input))
      return failure();
    inputs.push_back(std::move(*input));
  }
  out << "    " << *name << ".prepare(" << clock << ", pyc_reset, ";
  if (memory.asynchronous()) {
    out << inputs.front() << ", ";
  } else {
    SmallVector<std::string> enabled, addresses;
    for (unsigned port = 0; port < memory.readPorts(); ++port) {
      enabled.push_back(inputs[2 * port]);
      addresses.push_back(inputs[2 * port + 1]);
    }
    out << '{' << join(enabled, ", ") << "}, {" << join(addresses, ", ") << "}, ";
  }
  out << join(ArrayRef<std::string>(inputs).drop_front(memory.writeOffset()), ", ")
      << ");\n";
  return success();
}

LogicalResult BehavioralEmitter::emitLifecycle(
    raw_ostream &source, const gfir::StructuredDefinition &definition,
    StringRef name, bool memoized, raw_ostream *header) {
  for (StringRef action : {"reset", "discard", "commit"}) {
    const bool inlineCommit = header && action == "commit";
    auto &out = inlineCommit ? *header : source;
    if (inlineCommit)
      out << "inline ";
    out << "void " << name << "::" << action << "() noexcept {\n";
    if (memoized && action != "commit")
      out << "  pyc_memo_key.reset();\n";
    if (definition.hasChecks())
      out << "  pyc_checks_valid = false;\n";
    for (const auto &call : definition.clockedCalls) {
      auto member = instanceName(call.receiver);
      if (failed(member))
        return failure();
      out << "  " << *member << '.' << action << "();\n";
    }
    if (definition.clocked) {
      const auto &plan = *definition.clocked;
      for (const auto &fifo : plan.fifos) {
        auto member = instanceName(fifo.instance);
        if (failed(member))
          return failure();
        out << "  " << *member << '.' << action << "();\n";
      }
      for (const auto &memory : plan.memories) {
        auto member = instanceName(memory.instance);
        if (failed(member))
          return failure();
        out << "  " << *member << '.' << action << "();\n";
      }
      if (needsDomain(plan)) {
        if (action == "commit") {
          out << "  if (pyc_domain.commit()) {\n";
          if (memoized)
            out << "    pyc_memo_key.reset();\n";
          for (unsigned index = 0; index < plan.variables.size(); ++index) {
            out << "    " << stateName(index) << ".commit(";
            if (index == 0 && knownRecord(definition))
              out << "pyc_memo_known ? &pyc_memo_known->edit : nullptr";
            out << ");\n";
          }
          out << "  }\n";
        } else {
          out << "  pyc_domain." << action << "();\n";
          if (action == "reset" && !plan.variables.empty())
            out << "  pyc_reset_state();\n";
          if (action == "discard" && knownRecord(definition))
            out << "  " << stateName(0) << ".discard();\n";
        }
      }
    }
    out << "}\n";
  }
  return success();
}

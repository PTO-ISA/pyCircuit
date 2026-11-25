// SPDX-License-Identifier: BSD-3-Clause

FailureOr<bool> BehavioralEmitter::emitNativeCaptures(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const gfir::StructuredRule &rule, ArrayRef<CaptureGraphStep> schedule,
    ArrayRef<std::string> captures) {
  if (llvm::count_if(schedule, [](const auto &step) { return step.group.has_value(); }) < 2)
    return false;
  std::map<std::pair<unsigned, unsigned>, NativeOutputEntry> entries;
  std::map<std::pair<unsigned, unsigned>, unsigned> callSites;
  std::string text;
  llvm::raw_string_ostream native(text);
  auto record = planKnownRecord(definition, rule);
  const auto *whole = wholeInput(definition);
  auto packedInput = [&](unsigned index) {
    return whole && definition.wires[index].endpoint.value == whole->argument;
  };
  if (whole)
    native << "      if (!" << wholeInputName << ".isFullyKnown()) return false;\n";
  if (record)
    native << "      const bool pyc_committed_known = " << stateName(0)
           << ".isFullyKnown();\n";
  SmallVector<unsigned> origins(definition.wires.size());
  SmallVector<std::string> scalars(definition.wires.size());
  llvm::BitVector ready(definition.wires.size()), materialized(definition.wires.size());
  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    origins[index] = index;
    if (isa<BlockArgument>(wire.endpoint.value)) {
      ready.set(index);
      if (!packedInput(index))
        materialized.set(index);
    }
  }
  auto materialize = [&](unsigned index) {
    if (!ready.test(index))
      return false;
    if (materialized.test(index))
      return true;
    unsigned origin = origins[index];
    const auto &wire = definition.wires[index];
    if (materialized.test(origin)) {
      native << "      const auto &" << wireName(index) << " = " << wireName(origin)
             << ";\n";
    } else if (!scalars[origin].empty()) {
      native << "      const auto " << wireName(index) << " = " << carrier(wire.width)
             << "::known(gfsim::Bits<" << wire.width << ">{" << scalars[origin] << "});\n";
    } else if (packedInput(origin)) {
      native << "      const auto " << wireName(index) << " = "
             << graphLeaf(definition.wires[origin], wholeInputName) << ";\n";
    } else {
      return false;
    }
    materialized.set(index);
    return true;
  };
  auto scalar = [&](unsigned index) -> StringRef {
    unsigned origin = origins[index];
    auto &name = scalars[origin];
    if (!name.empty())
      return name;
    const auto &wire = definition.wires[origin];
    if (!isa<ac::BitsType>(wire.type) || wire.width > 64)
      return {};
    auto type = nativeInteger(wire.width);
    if (packedInput(origin)) {
      name = "pyc_native_input_" + std::to_string(origin);
      native << "      const auto " << name << " = static_cast<" << type
             << ">(pyc_behavior::sliceWords<" << wire.width << ">("
             << wholeInputName << ".packed().value(), " << wire.low << ").value());\n";
      return name;
    }
    if (!materialize(origin))
      return {};
    name = "pyc_native_input_" + std::to_string(origin);
    native << "      if (";
    if (record && wire.state)
      native << "!pyc_committed_known && ";
    native << '!' << wireName(origin) << ".isFullyKnown()) return false;\n"
           << "      const auto " << name << " = static_cast<" << type
           << ">(" << wireName(origin) << ".value().value());\n";
    return name;
  };
  for (const auto &step : schedule) {
    const unsigned index = step.wire;
    if (index >= definition.wires.size())
      return false;
    const auto &wire = definition.wires[index];
    if (const auto &bound = step.group) {
      const auto &child = program.definitions[bound->definition];
      auto &group = outputGroups[bound->definition][bound->ordinal];
      if (group.checkedInputs.empty())
        return false;
      ArrayRef<unsigned> tables;
      if (group.nativeEntry) {
        tables = group.nativeEntry->tables;
      } else {
        auto [position, inserted] = entries.try_emplace({bound->definition, bound->ordinal});
        auto &entry = position->second;
        if (inserted) {
          SmallVector<unsigned> roots, inputs;
          for (unsigned output : group.outputs)
            roots.push_back(child.outputs[output].wire);
          llvm::raw_string_ostream body(entry.body);
          auto built = emitKnownOutputGroup(body, child, group, roots, &inputs, &entry.tables);
          if (failed(built))
            return failure();
          if (!*built || inputs != group.checkedInputs)
            return false;
        }
        tables = entry.tables;
      }
      auto actual = [&](unsigned input) {
        auto found = llvm::lower_bound(group.inputs, input);
        return bound->inputs[found - group.inputs.begin()];
      };
      SmallVector<std::string> arguments;
      for (unsigned input : group.checkedInputs) {
        auto value = scalar(actual(input));
        if (value.empty())
          return false;
        arguments.push_back(value.str());
      }
      for (unsigned input : tables) {
        unsigned value = actual(input);
        if (!materialize(value))
          return false;
        arguments.push_back(wireName(value));
      }
      auto member = instanceName(bound->receiver);
      if (failed(member))
        return failure();
      const auto result = "pyc_native_group_" + std::to_string(index);
      native << "      " << moduleName(bound->definition) << "::" << nativeGroupResult(bound->ordinal)
             << ' ' << result << ";\n"
             << "      if (!" << *member << '.' << outputGroupName(bound->ordinal)
             << "_known({" << join(arguments, ", ") << "}, " << result << ")) return false;\n";
      ++callSites[{bound->definition, bound->ordinal}];
      for (unsigned output : bound->results) {
        auto field = definition.wires[output].call->output;
        scalars[output] = result + '.' + resultName(field);
        ready.set(output);
      }
      continue;
    }
    auto source = forwardedBits(definition, index);
    if (!source)
      source = forwardedTable(definition, index);
    if (source) {
      if (!ready.test(*source))
        return false;
      origins[index] = origins[*source];
      ready.set(index);
      continue;
    }
    if (wire.call) {
      const auto &child = program.definitions[wire.call->definition];
      const auto &output = child.outputs[wire.call->output];
      if (isClocked(child) || child.hasChecks() || output.requiresPreparation ||
          output.inputs.size() != wire.dependencies.size())
        return false;
      // A small pure singleton can stay native between two grouped calls.
      // Bind the exact scheduled leaves and let unsupported closures fall
      // through to their ordinary call.
      std::string body;
      llvm::raw_string_ostream bridge(body);
      OutputGroup single;
      single.inputs = output.inputs;
      SmallVector<unsigned> used;
      auto built = emitKnownOutputGroup(bridge, child, single, {output.wire},
                                        &used, nullptr, /*scalarResult=*/true);
      if (failed(built))
        return failure();
      if (*built) {
        SmallVector<std::string> parameters, arguments;
        for (unsigned input : used) {
          auto position = llvm::find(output.inputs, input) - output.inputs.begin();
          auto actual = scalar(wire.dependencies[position]);
          if (actual.empty())
            return false;
          parameters.push_back("std::uint64_t " + wireName(input));
          arguments.push_back(actual.str());
        }
        auto &name = scalars[index];
        name = "pyc_native_bridge_" + std::to_string(index);
        native << "      const auto " << name << " = [](" << join(parameters, ", ")
               << ") {\n" << body << "      }(" << join(arguments, ", ") << ");\n";
        ready.set(index);
        continue;
      }
      if (!llvm::all_of(wire.dependencies, materialize))
        return false;
    } else if (!wire.state || !recordField(definition, wire, {})) {
      return false;
    }
    auto value = expression(wire, definition, {});
    if (failed(value))
      return failure();
    native << "      const auto " << (wire.state ? "&" : "") << wireName(index)
           << " = " << *value << ";\n";
    ready.set(index);
    materialized.set(index);
  }
  auto operation = rule.operation;
  auto materializeCapture = [&](Value capture) {
    auto found = definition.valueWires.find(capture);
    return found != definition.valueWires.end() && llvm::all_of(found->second, materialize);
  };
  if (record) {
    SmallVector<std::string> values;
    for (const auto &edit : record->edits) {
      StringRef value = scalar(edit.value);
      if (value.empty())
        return false;
      values.push_back(value.str());
      if (!edit.row)
        continue;
      StringRef row = scalar(*edit.row);
      if (row.empty())
        return false;
      values.push_back(row.str());
      if (edit.condition) {
        StringRef enabled = scalar(*edit.condition);
        if (enabled.empty())
          return false;
        values.push_back(enabled.str() + " != 0");
      } else {
        values.push_back("true");
      }
    }
    auto snapshot = operation.getCaptures()[record->snapshotCapture];
    std::string published = captures[record->snapshotCapture];
    auto fields = definition.valueWires.find(snapshot);
    // These leaves already have native values (or belong to the checked
    // root input). A snapshot may otherwise contain X/Z independently of
    // its candidate writes, so keep the original four-state assembly then.
    if (fields != definition.valueWires.end() && !fields->second.empty() &&
        llvm::all_of(fields->second, [&](unsigned index) {
          const auto &wire = definition.wires[index];
          return ready.test(index) && isa<ac::BitsType>(wire.type) && wire.width <= 64 &&
                 (!scalars[origins[index]].empty() || packedInput(origins[index]));
        })) {
      auto type = typedWire(snapshot.getType(), operation);
      if (failed(type))
        return failure();
      SmallVector<unsigned> ordered(fields->second);
      llvm::sort(ordered, [&](unsigned a, unsigned b) {
        return definition.wires[a].low > definition.wires[b].low;
      });
      SmallVector<std::string> widths, parts;
      for (unsigned index : ordered) {
        widths.push_back(std::to_string(definition.wires[index].width));
        parts.push_back(scalar(index).str());
      }
      published = unpack(*type, "pyc_behavior::concatenateKnown<" + join(widths, ", ") +
                               ">(" + join(parts, ", ") + ")");
    } else if (!materializeCapture(snapshot)) {
      return false;
    }
    native << "      pyc_memo_known.emplace(KnownMemo{"
           << published << ", {" << join(values, ", ") << "}});\n";
    knownRecords.try_emplace(definition.module, std::move(*record));
  } else {
    if (!llvm::all_of(operation.getCaptures(), materializeCapture))
      return false;
    native << "      pyc_memo_values.emplace(" << join(captures, ", ") << ");\n";
  }
  native << "      return true;\n";
  for (auto &[position, entry] : entries) {
    auto &[index, ordinal] = position;
    auto &group = outputGroups[index][ordinal];
    group.nativeEntry = std::make_unique<NativeOutputEntry>(std::move(entry));
  }
  for (const auto &[position, count] : callSites) {
    auto [index, ordinal] = position;
    auto &entry = *outputGroups[index][ordinal].nativeEntry;
    if (entry.callSites == 0 && count == 1) {
      entry.soleCaller = definition.module;
      entry.callSites = 1;
    } else {
      entry.soleCaller = {};
      entry.callSites = 2;
    }
  }
  out << text;
  return true;
}

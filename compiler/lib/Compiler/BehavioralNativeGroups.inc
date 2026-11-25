// SPDX-License-Identifier: BSD-3-Clause

FailureOr<bool> BehavioralEmitter::emitNativeCaptures(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const gfir::StructuredRule &rule, ArrayRef<CaptureGraphStep> schedule,
    ArrayRef<std::string> captures) {
  if (llvm::count_if(schedule, [](const auto &step) { return step.group.has_value(); }) < 2)
    return false;
  std::map<std::pair<unsigned, unsigned>, NativeOutputEntry> entries;
  std::string text;
  llvm::raw_string_ostream native(text);
  SmallVector<unsigned> origins(definition.wires.size());
  SmallVector<std::string> scalars(definition.wires.size());
  llvm::BitVector ready(definition.wires.size()), materialized(definition.wires.size());
  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    origins[index] = index;
    if (isa<BlockArgument>(wire.endpoint.value)) {
      ready.set(index);
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
    if (!isa<ac::BitsType>(wire.type) || wire.width > 64 || !materialize(origin))
      return {};
    name = "pyc_native_input_" + std::to_string(origin);
    native << "      if (!" << wireName(origin) << ".isFullyKnown()) return false;\n"
           << "      const auto " << name << " = static_cast<" << nativeInteger(wire.width)
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
    // A singleton may bridge two native groups. Materialize its exact inputs
    // and keep using the ordinary pure function. Scalar consumers prove its
    // result known just as they do for a committed field or a root input.
    if (wire.call) {
      const auto &child = program.definitions[wire.call->definition];
      if (isClocked(child) || child.hasChecks() ||
          child.outputs[wire.call->output].requiresPreparation ||
          !llvm::all_of(wire.dependencies, materialize))
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
  for (Value capture : operation.getCaptures()) {
    auto found = definition.valueWires.find(capture);
    if (found == definition.valueWires.end() || !llvm::all_of(found->second, materialize))
      return false;
  }
  native << "      pyc_memo_values.emplace(" << join(captures, ", ") << ");\n"
         << "      return true;\n";
  for (auto &[position, entry] : entries) {
    auto &[index, ordinal] = position;
    auto &group = outputGroups[index][ordinal];
    group.nativeEntry = std::make_unique<NativeOutputEntry>(std::move(entry));
  }
  out << text;
  return true;
}

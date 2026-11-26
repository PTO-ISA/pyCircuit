// SPDX-License-Identifier: BSD-3-Clause

FailureOr<FinalCppSourceGroup *> BehavioralEmitter::getGroup(gfir::ModuleOp module) {
  auto owner = module.getSourceOwner();
  auto diagnostic = [&] { return module.emitOpError(); };
  auto names = sourceOwnerComponents(owner, diagnostic);
  if (failed(names))
    return failure();
  auto path = sourcePathName(names->filePath, ".hpp", diagnostic);
  if (failed(path))
    return failure();
  auto [entry, inserted] = groups.try_emplace(*path, parts.sourceGroups.size());
  if (inserted) {
    auto source = sourcePathName(names->filePath, ".cpp", diagnostic);
    if (failed(source))
      return failure();
    parts.sourceGroups.push_back(
        {owner, *path, *source,
         "#pragma once\n#include \"pycircuit_support.hpp\"\n",
         "#include \"" + *path + "\"\n"});
  }
  auto &group = parts.sourceGroups[entry->second];
  if (group.sourceOwner != owner)
    return module.emitOpError("C++ source owner header path collision");
  return &group;
}

FailureOr<std::string> BehavioralEmitter::emitObservationMetadata(
    const gfir::StructuredDefinition &definition) {
  if (!hasObservations(definition)) {
    parts.systemHeader += "inline std::span<const gfsim::RunnerObservation> "
                          "pyc_observation_metadata() { return {}; }\n";
    return std::string("observations_.Configure({}, 0)");
  }
  std::string descriptors, metadata;
  llvm::raw_string_ostream descriptorStream(descriptors), metadataStream(metadata);
  uint64_t registration = 0, site = 0, events = 0, slots = 0;
  for (const auto &rule : definition.clocked->rules) {
    if (rule.observations.empty())
      continue;
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      bool gauge = effect.getKind() == "report";
      bool carriesValue = !effect.getValues().empty();
      unsigned count = std::max<std::size_t>(1, effect.getValues().size());
      for (unsigned index = 0; index < count; ++index) {
        uint64_t width = 1;
        if (carriesValue) {
          auto payload = analysis.getPackedWidth(effect.getValues()[index].getType(), {}, effect);
          if (failed(payload))
            return failure();
          width = *payload;
        }
        uint64_t slot = observation.firstSlot + index;
        appendCppObservationDescriptor(descriptorStream,
            {slot, 0, registration, site, gauge ? 1u : 0u}, index, count, width);
        if (failed(appendCppObservationMetadata(metadataStream, effect, observation.id,
                effect.getSpecAttr(), effect.getKind(), "root", slot, carriesValue, index, count)))
          return failure();
      }
      slots += count;
      if (!gauge)
        events += count;
      ++site;
    }
    ++registration;
  }
  llvm::raw_string_ostream header(parts.systemHeader);
  header << "static constexpr gfsim::ObservationDescriptor pyc_observation_descriptors["
         << slots << "] = {" << descriptors << "\n};\n"
         << "inline std::span<const gfsim::RunnerObservation> "
            "pyc_observation_metadata() {\n"
            "  static const gfsim::RunnerObservation entries[] = {\n"
         << metadata << "  };\n  return entries;\n}\n";
  return "observations_.Configure(pyc_observation_descriptors, " +
         std::to_string(events) + ")";
}

LogicalResult BehavioralEmitter::emitRoot(FinalCppSourceGroup &group, bool inlineWork) {
  const auto &definition = program.definitions[program.rootIndex];
  auto root = definition.module;
  auto entry = program.root;
  auto policy = entry.getExecutionPolicyAttr();
  const bool resetIfOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
  const bool system = entry.getKind() == "system";
  const bool observes = hasObservations(definition);
  auto name = qualifiedName(root.getSymName(), root);
  if (failed(name))
    return failure();
  parts.rootCppName = *name;
  parts.rootHeaderPath = group.headerPath;
  parts.sourceCheckCount = definition.sourceCheckCount;
  SmallVector<StringRef> components;
  StringRef(*name).drop_front(2).split(components, "::");
  llvm::raw_string_ostream header(group.header);
  for (auto component : ArrayRef<StringRef>(components).drop_back())
    header << "namespace " << component << " {\n";
  auto className = components.back();
  header << "class " << className << " final : public gfsim::SimModule {\n"
         << "public:\n  explicit " << className
         << "(std::string name, gfsim::WorkExecutor* = nullptr";
  if (observes)
    header << ", pyc_observation_sink *sink = nullptr, std::size_t base = 0";
  header << ")\n      : gfsim::SimModule(std::move(name))";
  if (observes)
    header << ", pyc_sink_(sink), pyc_observation_base_(base)";
  header << " {\n";
  if (definition.hasChecks())
    header << "    pyc_body.configure_checks(std::string(this->name()));\n";
  header << "  }\n"
         << "  bool HasWork() const noexcept override { return true; }\n"
         << "  " << (inlineWork ? "inline " : "") << "void Work() override;\n";
  if (definition.hasChecks())
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept;\n";
  else
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &) const noexcept "
              "{ return true; }\n";
  if (isClocked(definition))
    header << "  void Reset() noexcept override;\n"
           << "  void DiscardNext() noexcept override;\n"
           << "  " << (inlineWork ? "inline " : "") << "void Xfer() noexcept override;\n";
  SmallVector<std::string> inputs, outputs;
  llvm::json::Array inputRows, outputRows, controls;
  SmallVector<std::pair<unsigned, std::string>> companions;
  auto ports = [&](TypeRange types, ArrayAttr names, SmallVectorImpl<std::string> &wires,
                   llvm::json::Array &rows, bool output) -> LogicalResult {
    for (auto [index, pair] : llvm::enumerate(llvm::zip(types, names))) {
      auto [type, raw] = pair;
      auto original = cast<StringAttr>(raw).getValue();
      auto wire = legalizeIdentifier(original, [&] { return root.emitOpError(); });
      auto carrierType = common.cppType(type, root);
      auto leaves = analysis.getLeaves(type, {}, root);
      if (failed(wire) || failed(carrierType) || failed(leaves))
        return failure();
      header << "  gfsim::wire<" << *carrierType << "> " << *wire << ";\n";
      wires.push_back(*wire);
      llvm::json::Array fields;
      uint64_t width = 0;
      for (const auto &leaf : *leaves) {
        llvm::json::Array path;
        for (auto field : leaf.path)
          path.emplace_back(field.getValue());
        fields.emplace_back(llvm::json::Object{
            {"path", std::move(path)}, {"low", leaf.low}, {"width", leaf.width}});
        width += leaf.width;
      }
      llvm::json::Object row{{"ordinal", index}, {"name", original},
                             {"wire", *wire}, {"width", width}, {"fields", std::move(fields)}};
      if (output)
        row["availability"] = nullptr;
      rows.emplace_back(std::move(row));
    }
    return success();
  };
  auto type = root.getFunctionType();
  if (failed(ports(type.getInputs(), root.getInputNames(), inputs, inputRows, false)) ||
      failed(ports(type.getResults(), root.getOutputNames(), outputs, outputRows, true)))
    return failure();
  for (const auto &output : definition.outputAvailability) {
    auto wire = legalizeIdentifier(output.name.getValue(), [&] { return root.emitOpError(); });
    if (failed(wire))
      return failure();
    header << "  pyc_behavior::Level " << *wire << ";\n";
    (*outputRows[output.port].getAsObject())["availability"] = *wire;
    controls.emplace_back(llvm::json::Object{{"direction", "output"},
                                             {"role", "availability"},
                                             {"wire", *wire}, {"width", 1}});
    outputs.push_back(*wire);
    companions.emplace_back(output.port, std::move(*wire));
  }
  llvm::json::Object domains{{"clock", nullptr}, {"reset", nullptr}};
  std::string clock, reset;
  if (isClocked(definition) && !definition.explicitDomains) {
    for (auto [role, port] : {std::pair{"clock", "pyc_clk"}, {"reset", "pyc_rst"}}) {
      auto pin = legalizeIdentifier(port, [&] { return root.emitOpError(); });
      if (failed(pin))
        return failure();
      domains[role] = inputs.size();
      inputRows.emplace_back(llvm::json::Object{
          {"ordinal", inputs.size()}, {"name", port}, {"wire", *pin}, {"width", 1},
          {"fields", llvm::json::Array{llvm::json::Object{
              {"path", llvm::json::Array{}}, {"low", 0}, {"width", 1}}}}});
      inputs.push_back(*pin);
      header << "  pyc_behavior::Level " << *pin << ";\n";
      (StringRef(role) == "clock" ? clock : reset) = *pin;
    }
  }

  header << "private:\n  " << moduleName(program.rootIndex) << " pyc_body;\n";
  if (observes)
    header << "  pyc_observation_sink *pyc_sink_;\n"
           << "  std::size_t pyc_observation_base_;\n";
  if (system)
    header << "  pyc_system_sampled_clock pyc_system_clock;\n";
  header << "};\n";
  for (size_t i = 1; i < components.size(); ++i)
    header << "}\n";

  llvm::raw_string_ostream source(group.source);
  StringRef scopedName = StringRef(*name).drop_front(2);
  auto &work = inlineWork ? header : source;
  if (inlineWork)
    work << "inline ";
  work << "void " << scopedName << "::Work() {\n";
  if (isClocked(definition)) {
    work << "  try {\n";
    if (!definition.explicitDomains)
      work << "    const auto pyc_reset = gfsim::interpretReset(" << reset
           << ", gfsim::ResetSemantics::" << (resetIfOne ? "IfOne" : "Strict") << ");\n";
  }

  const auto &wide = wholeOutputs[program.rootIndex];
  llvm::SmallDenseSet<unsigned> declaredInputs;
  const auto *whole = wholeInput(definition);
  for (const auto &output : definition.outputs)
    if (!output.requiresPreparation && !wide.contains(output.port.port))
      declaredInputs.insert(output.inputs.begin(), output.inputs.end());
  if (!whole)
    declaredInputs.insert(definition.preparationInputs.begin(), definition.preparationInputs.end());

  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    if (!isa<BlockArgument>(wire.endpoint.value) ||
        !declaredInputs.contains(index))
      continue;
    auto arg = cast<BlockArgument>(wire.endpoint.value).getArgNumber();
    const bool directTable = isa<ac::TableType>(wire.endpoint.value.getType());
    work << "    const auto " << (directTable ? "&" : "") << wireName(index)
           << " = " << graphLeaf(wire, inputs[arg]) << ";\n";
  }
  if (isClocked(definition)) {
    work << "    pyc_body.prepare(";
    bool comma = !definition.explicitDomains;
    if (comma)
      work << clock << ", pyc_reset";
    auto inputArgument = [&](StringRef value) {
      if (comma)
        work << ", ";
      work << value;
      comma = true;
    };
    if (whole)
      inputArgument(inputs[whole->argument.getArgNumber()]);
    else
      for (unsigned input : definition.preparationInputs)
        inputArgument(wireName(input));
    for (const auto &input : definition.inputAvailability)
      inputArgument(levelName.str() + "(true)");
    if (hasIncomingWrites(definition))
      inputArgument("{}");
    if (observes)
      work << ", pyc_sink_, pyc_observation_base_";
    work << ");\n";
  }
  if (system)
    work << "    pyc_system_clock.prepare(" << clock << ", " << reset
           << ", gfsim::ResetSemantics::" << (resetIfOne ? "IfOne" : "Strict") << ");\n";

  llvm::SmallDenseSet<unsigned> assigned;
  for (auto [index, output] : llvm::enumerate(definition.outputs)) {
    if (wide.contains(output.port.port)) {
      if (assigned.insert(output.port.port).second)
        work << "    " << outputs[output.port.port] << " = pyc_body."
               << wholeOutputName(output.port.port) << "();\n";
      continue;
    }
    // The source leaf may be a scalar projected from a wider result port.
    // Use the layout of that port, not the layout of the producing value.
    auto leaves = analysis.getLeaves(type.getResult(output.port.port), {}, root);
    if (failed(leaves))
      return failure();
    auto leaf = llvm::find_if(*leaves, [&](const ac::PackedLeaf &item) {
      return item.path == output.port.path;
    });
    if (leaf == leaves->end())
      return root.emitOpError("unresolved behavioral output layout");
    SmallVector<std::string> actuals;
    if (!output.requiresPreparation)
      for (unsigned input : output.inputs)
        actuals.push_back(wireName(input));
    auto value = "pyc_body." + functionName(index) + '(' + join(actuals, ", ") + ')';
    usedOutputs[program.rootIndex].set(index);
    if (isa<ac::TableType>(type.getResult(output.port.port))) {
      work << "    " << outputs[output.port.port] << " = " << value << ";\n";
    } else {
      if (isa<ac::TableType>(leaf->type))
        value = pack(value);
      work << "    " << outputs[output.port.port] << ".assignSlice(" << leaf->low
             << ", " << value << ");\n";
    }
  }
  for (const auto &[port, wire] : companions) {
    work << "    " << wire << " = ";
    if (!isClocked(definition)) {
      auto output = llvm::find_if(definition.outputs, [&](const auto &value) { return value.port.port == port; });
      unsigned result = output - definition.outputs.begin();
      SmallVector<std::string> arguments;
      for (unsigned input : output->inputs)
        arguments.push_back(wireName(input));
      for (unsigned input : pureAvailabilityInputs(definition, result))
        arguments.push_back(levelName.str() + "(true)");
      work << "pyc_body." << pureAvailabilityName(result) << '(' << join(arguments, ", ") << ");\n";
      continue;
    }
    if (!definition.fourStateAvailability)
      work << levelName << '(';
    work << "pyc_body." << availabilityName(port);
    if (!definition.fourStateAvailability)
      work << ')';
    work << ";\n";
  }
  if (isClocked(definition))
    work << "  } catch (...) {\n    DiscardNext();\n    throw;\n  }\n";
  work << "}\n";
  if (isClocked(definition)) {
    for (auto [hook, action] : {std::pair{"Reset", "reset"},
                              {"DiscardNext", "discard"}, {"Xfer", "commit"}}) {
      const bool inlineXfer = inlineWork && StringRef(hook) == "Xfer";
      auto &target = inlineXfer ? header : source;
      if (inlineXfer)
        target << "inline ";
      target << "void " << scopedName << "::" << hook << "() noexcept {\n"
             << "  pyc_body." << action << "();\n";
      if (system)
        target << "  pyc_system_clock." << action << "();\n";
      target << "}\n";
    }
  }

  if (definition.hasChecks()) {
    source << "bool " << scopedName
           << "::pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept {\n"
              "  failure = {};\n"
              "  return pyc_body.validate_sampling(failure) && pyc_body.validate_checks(failure)";
    if (hasAssertions(definition))
      source << " && pyc_body.validate_assertions(failure)";
    source << ";\n}\n";
  }

  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\nusing pyc_root = " + *name + ";\n";
  std::string configure = "observations_.Configure({}, 0)";
  if (system) {
    parts.systemHeader += "#include <gfsim/SystemRunner.h>\n";
    auto observations = emitObservationMetadata(definition);
    if (failed(observations))
      return failure();
    configure = std::move(*observations);
    addCppSystemDriver(parts, clock, reset, program.samples, resetIfOne);
  }
  appendCppDutHeader(parts, inputs, outputs, configure,
                     /*hasChecks=*/definition.hasChecks(), observes);
  llvm::raw_string_ostream map(parts.portMap);
  map << llvm::json::Value(llvm::json::Object{
      {"schema_version", 1}, {"target", "cpp"}, {"inputs", std::move(inputRows)},
      {"outputs", std::move(outputRows)}, {"controls", std::move(controls)},
      {"domains", std::move(domains)}});
  return success();
}

  // Memory issue proposals follow Queue proposals and precede write domains.
  if (memoryProposals && failed(charge(budget, rule, memoryProposals.size(),
                                       sizeof(SampledMemoryProposalView)))) return failure();
  if (memoryProposals) for (Attribute raw : memoryProposals) {
    auto row = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(row, {"memory", "kind", "port", "occurrence", "location",
                           "raw_path", "path", "arguments"}, rule,
                     "sampled Memory proposal"))) return failure();
    auto memory = row.getAs<DictionaryAttr>("memory");
    if (failed(exact(memory, {"owner", "declaration"}, rule, "Memory identity")) ||
        failed(verifyOwnerRef(memory.getAs<DictionaryAttr>("owner"), [&] { return rule->emitOpError(); })) ||
        failed(verifyOccurrence(memory.getAs<DictionaryAttr>("declaration"), [&] { return rule->emitOpError(); })))
      return failure();
    auto kind = row.getAs<StringAttr>("kind");
    auto occurrence = row.getAs<DictionaryAttr>("occurrence");
    auto location = row.getAs<DictionaryAttr>("location");
    auto port = index(row, "port", rule), rawPath = index(row, "raw_path", rule), path = index(row, "path", rule);
    auto arguments = row.getAs<ArrayAttr>("arguments");
    if (!kind || (kind.getValue() != "request" && kind.getValue() != "read" && kind.getValue() != "write") ||
        failed(port) || failed(rawPath) || failed(path) || uint64_t(*rawPath) != queueResult ||
        uint64_t(*path) != queueResult + 1 || uint64_t(*path) >= yielded->getNumOperands() ||
        !arguments || arguments.size() != (kind.getValue() == "write" ? 3 : 1) ||
        failed(verifyOccurrence(occurrence, [&] { return rule->emitOpError(); })) ||
        failed(verifySourceSpan(location, [&] { return rule->emitOpError(); })) ||
        failed(bitOne(yielded->getOperand(*rawPath), analysis, bindings, rule, budget)) ||
        failed(bitOne(yielded->getOperand(*path), analysis, bindings, rule, budget)))
      return rule->emitOpError() << "Memory proposal requires exact action and path suffix";
    SampledMemoryProposalView proposal;
    proposal.memory = memory; proposal.kind = kind; proposal.port = *port;
    proposal.occurrence = occurrence; proposal.location = location;
    proposal.rawPathResult = *rawPath; proposal.pathResult = *path;
    proposal.rawPath = yielded->getOperand(*rawPath); proposal.path = yielded->getOperand(*path);
    if (failed(charge(budget, rule, arguments.size(), sizeof(uint64_t) + sizeof(Value)))) return failure();
    for (auto [ordinal, rawArgument] : llvm::enumerate(arguments)) {
      auto argument = dyn_cast<IntegerAttr>(rawArgument);
      if (!argument || !argument.getType().isSignlessInteger(64) || argument.getInt() < 0 ||
          uint64_t(argument.getInt()) != queueResult + 2 + ordinal ||
          uint64_t(argument.getInt()) >= yielded->getNumOperands())
        return rule->emitOpError() << "Memory arguments require exact ordered suffix slots";
      proposal.argumentResults.push_back(argument.getInt());
      proposal.arguments.push_back(yielded->getOperand(argument.getInt()));
    }
    result.memoryProposals.push_back(std::move(proposal));
    queueResult += 2 + arguments.size();
  }

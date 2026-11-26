// SPDX-License-Identifier: BSD-3-Clause

LogicalResult BehavioralEmitter::emitInitial(raw_ostream &out, gfir::VarOp variable,
                                             unsigned index) {
  out << "  static const auto &" << initialName(index) << "() noexcept {\n"
      << "    static const auto init = [] {\n";
  Value initial = variable.getInitial();
  SmallVector<std::pair<Value, bool>> pending{{initial, false}};
  llvm::DenseMap<Value, std::string> values;
  while (!pending.empty()) {
    auto [value, ready] = pending.pop_back_val();
    if (values.contains(value))
      continue;
    auto *op = value.getDefiningOp();
    if (!ready) {
      pending.emplace_back(value, true);
      for (Value input : llvm::reverse(op->getOperands()))
        pending.emplace_back(input, false);
      continue;
    }
    SmallVector<std::string> args;
    for (Value input : op->getOperands())
      args.push_back(values.lookup(input));
    auto text = valueExpression(value, args);
    if (failed(text))
      return failure();
    auto name = wireName(values.size());
    out << "      const auto " << name << " = " << *text << ";\n";
    values.try_emplace(value, std::move(name));
  }
  out << "      return " << values.lookup(initial) << ";\n    }();\n"
      << "    return init;\n  }\n";
  return success();
}

FailureOr<std::string> methodName(gfir::MethodOp method) {
  auto name = legalizeIdentifier(method.getSymName(),
                                 [&] { return method.emitOpError(); });
  if (failed(name))
    return failure();
  return "pyc_method_" + *name;
}

LogicalResult BehavioralEmitter::emitBlockedReturn(raw_ostream &out, Block &body,
                                                   bool ruleResults) {
  SmallVector<std::string> values;
  if (!ruleResults)
    for (Value value : cast<gfir::YieldOp>(body.getTerminator()).getValues()) {
      auto type = typedWire(value.getType(), body.getParentOp());
      if (failed(type))
        return failure();
      values.push_back(*type + "{}");
    }
  out << "return";
  if (values.size() == 1)
    out << ' ' << values.front();
  else if (!values.empty())
    out << " std::tuple{" << join(values, ", ") << '}';
  out << ";\n";
  return success();
}

FailureOr<bool> BehavioralEmitter::emitKnownRuleSuffix(
    raw_ostream &out, Operation &first, const gfir::StructuredClocked &clocked,
    const llvm::DenseMap<Value, std::string> &prefix) {
  for (Operation &op : *first.getBlock()) {
    if (&op == &first)
      break;
    if (!isa<gfir::ReadOp, ac::StructCreateOp, ac::StructGetOp>(op) &&
        !isNativeScalar(op, /*allowShifts=*/true))
      return false;
  }

  llvm::DenseMap<Value, unsigned> reads;
  llvm::DenseMap<Value, std::pair<Value, Value>> branchResults;
  llvm::SmallDenseSet<unsigned> written;
  SmallVector<Operation *> operations;
  SmallVector<gfir::WriteOp> writes;
  Value predicate;
  auto usePredicate = [&](Value condition) {
    if (!predicate)
      predicate = condition;
    return predicate == condition;
  };
  auto scalar = [&](Value value) {
    if (!isa<ac::BitsType>(value.getType()))
      return false;
    auto width = analysis.getPackedWidth(value.getType(), {}, &first);
    return succeeded(width) && *width > 0 && *width <= 64;
  };
  for (Operation *op = &first; !isa<gfir::YieldOp>(op); op = op->getNextNode()) {
    if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      if (!writes.empty() || !scalar(read.getValue()))
        return false;
      reads.try_emplace(read.getValue(), clocked.stateAccesses.lookup(read));
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      if (write.getIndex() || clocked.writePaths.contains(write) || !scalar(write.getValue()) ||
          !written.insert(clocked.stateAccesses.lookup(write)).second)
        return false;
      if (auto select = write.getValue().getDefiningOp<ac::BitsSelectOp>();
          select && !usePredicate(select.getCondition()))
        return false;
      writes.push_back(write);
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      if (!usePredicate(branch.getPredicate()) || clocked.checkedBranches.contains(branch) ||
          !llvm::all_of(branch->getResults(), scalar))
        return false;
      auto &thenBody = branch.getThenBody().front();
      auto &elseBody = branch.getElseBody().front();
      for (Block *arm : {&thenBody, &elseBody}) {
        for (Operation &arithmetic : arm->without_terminator()) {
          if (!isNativeScalar(arithmetic, /*allowShifts=*/true))
            return false;
          operations.push_back(&arithmetic);
        }
      }
      auto thenYield = cast<gfir::YieldOp>(thenBody.getTerminator());
      auto elseYield = cast<gfir::YieldOp>(elseBody.getTerminator());
      for (auto [result, yes, no] :
           llvm::zip(branch->getResults(), thenYield.getValues(), elseYield.getValues()))
        branchResults.try_emplace(result, yes, no);
      operations.push_back(op);
    } else if (isNativeScalar(*op, /*allowShifts=*/true)) {
      operations.push_back(op);
    } else {
      return false;
    }
  }
  if (!predicate || !prefix.contains(predicate) || operations.size() < minScalarOperations)
    return false;

  auto selected = [&](Value value, bool take) -> Value {
    if (auto branch = branchResults.find(value); branch != branchResults.end())
      return take ? branch->second.first : branch->second.second;
    auto select = value.getDefiningOp<ac::BitsSelectOp>();
    if (!select || select.getCondition() != predicate)
      return Value{};
    return take ? select.getTrueValue() : select.getFalseValue();
  };
  // Compute actual operand demand separately for each known branch. The union
  // of their incoming values is checked before publishing any candidate.
  SmallVector<Value> frontier{predicate};
  llvm::SmallDenseSet<Value> seen;
  seen.insert(predicate);
  auto demand = [&](bool take) {
    SmallVector<Value> pending;
    for (auto write : writes)
      pending.push_back(write.getValue());
    llvm::SmallDenseSet<Value> needed;
    while (!pending.empty()) {
      Value value = pending.pop_back_val();
      if (!needed.insert(value).second)
        continue;
      if (prefix.contains(value) || reads.contains(value)) {
        if (seen.insert(value).second)
          frontier.push_back(value);
      } else if (auto operand = selected(value, take)) {
        pending.push_back(operand);
      } else {
        llvm::append_range(pending, value.getDefiningOp()->getOperands());
      }
    }
    return needed;
  };
  auto yes = demand(true);
  auto no = demand(false);
  if (!llvm::all_of(frontier, scalar))
    return false;

  SmallVector<std::string> sources, known;
  llvm::DenseMap<Value, std::string> inputs;
  for (auto [ordinal, value] : llvm::enumerate(frontier)) {
    std::string source = prefix.lookup(value);
    if (auto read = reads.find(value); read != reads.end()) {
      auto state = stateName(read->second);
      source = "pyc_known_read_" + std::to_string(ordinal);
      out << "    const auto &" << source << " = txn." << state
          << " ? *txn." << state << " : " << state << ".value();\n";
    }
    sources.push_back(pack(source));
    known.push_back(sources.back() + ".isFullyKnown()");
    inputs.try_emplace(value, "pyc_known_value_" + std::to_string(ordinal));
  }
  out << "    if (" << join(known, " &&\n        ") << ") {\n";
  for (auto [source, value] : llvm::zip(sources, frontier))
    out << "      const auto " << inputs.lookup(value) << " = "
        << source << ".value().value();\n";
  auto emitArm = [&](bool take, const llvm::SmallDenseSet<Value> &needed) -> LogicalResult {
    auto values = inputs;
    for (Operation *op : operations) {
      if (isa<gfir::IfOp>(op)) {
        for (Value result : op->getResults())
          if (needed.contains(result))
            values.try_emplace(result, values.lookup(selected(result, take)));
        continue;
      }
      Value value = op->getResult(0);
      if (!needed.contains(value))
        continue;
      if (auto operand = selected(value, take)) {
        values.try_emplace(value, values.lookup(operand));
        continue;
      }
      auto width = analysis.getPackedWidth(value.getType(), {}, op);
      if (failed(width))
        return failure();
      SmallVector<std::string> operands;
      for (Value operand : op->getOperands())
        operands.push_back(values.lookup(operand));
      auto expression = nativeScalarExpression(*op, *width, operands);
      if (failed(expression))
        return failure();
      auto name = "pyc_known_value_" + std::to_string(values.size());
      out << "        const auto " << name << " = pyc_behavior::limitBits<" << *width
          << ">(" << *expression << ");\n";
      values.try_emplace(value, std::move(name));
    }
    for (auto write : writes) {
      unsigned state = clocked.stateAccesses.lookup(write);
      auto value = write.getValue();
      auto type = typedWire(value.getType(), write);
      auto width = analysis.getPackedWidth(value.getType(), {}, write);
      if (failed(type) || failed(width))
        return failure();
      if (clocked.conditionalWrites.contains(state))
        out << "        txn." << intentName(state) << " = " << levelName << "(true);\n";
      out << "        txn." << stateName(state) << " = " << *type
          << "::known(gfsim::Bits<" << *width << ">{" << values.lookup(value) << "});\n";
    }
    return success();
  };
  out << "      if (" << inputs.lookup(predicate) << ") {\n";
  if (failed(emitArm(true, yes)))
    return failure();
  out << "      } else {\n";
  if (failed(emitArm(false, no)))
    return failure();
  out << "      }\n      return;\n    }\n";
  return true;
}

LogicalResult BehavioralEmitter::emitBody(raw_ostream &out, Block &body,
                                          const gfir::StructuredDefinition &definition,
                                          const gfir::StructuredRule *rule,
                                          const llvm::DenseMap<Value, std::string> *outer,
                                          ArrayRef<std::string> stepTargets,
                                          StringRef stepNatural, StringRef stepOrdinal,
                                          OrderedFifoBranchEmission *ordered) {
  const auto &clocked = *definition.clocked;
  bool ruleResults = rule != nullptr;
  unsigned childCallOrdinal = 0;
  bool orderedFifo = rule && rule->sampled && !rule->requirements.empty();
  using DemandKind = ResourceDemandKind;
  OrderedFifoBranchEmission branchControls;
  bool ownsBranchControls = rule && hasOrderedFifoBranches(*rule);
  if (ownsBranchControls) {
    ordered = &branchControls;
    ordered->rule = rule;
    out << "    txn.pyc_raw_controls.fill(" << levelName << "(true));\n"
           "    txn.pyc_control_reached.fill(false);\n"
           "    txn.pyc_pop_reached.fill(false);\n"
           "    txn.pyc_push_reached.fill(false);\n"
           "    txn.pyc_raw_pops.fill(" << levelName << "(false));\n";
    auto operation = rule->operation;
    auto initialized = operation.walk([&](gfir::CallOp call) {
      auto use = clocked.fifoCalls.find(call);
      if (use == clocked.fifoCalls.end() ||
          (use->second.access != gfir::StructuredFifoAccess::Out &&
           use->second.access != gfir::StructuredFifoAccess::View))
        return WalkResult::advance();
      auto name = instanceName(clocked.fifos[use->second.fifo].instance);
      if (failed(name)) return WalkResult::interrupt();
      out << "    txn.pyc_raw_controls[" << clocked.checkOffsets.lookup(call)
          << "] = " << *name << ".valid();\n";
      return WalkResult::advance();
    });
    if (initialized.wasInterrupted()) return failure();
  }
  struct ReachedControl {
    unsigned ordinal;
    DemandKind demand;
    std::string raw, reached;
  };
  SmallVector<ReachedControl> orderedControls;
  llvm::DenseMap<Value, std::string> values = outer ? *outer : llvm::DenseMap<Value, std::string>{};
  for (Value arg : body.getArguments())
    values.try_emplace(arg, wireName(values.size()));
  auto snapshot = [&] {
    auto yield = cast<gfir::YieldOp>(body.getTerminator());
    for (auto [index, value] : llvm::enumerate(yield.getValues()))
      out << "    txn." << resultName(index) << " = " << values.lookup(value) << ";\n";
  };
  auto requireKnownOne = [&](StringRef predicate, Operation *site,
                            DemandKind demand = DemandKind::Boundary) -> LogicalResult {
    if (ordered) {
      unsigned ordinal = clocked.checkOffsets.lookup(site);
      auto raw = "txn.pyc_raw_controls[" + std::to_string(ordinal) + "]";
      out << "    " << raw << " = " << predicate << ";\n"
          << "    txn.pyc_control_reached[" << ordinal << "] = txn.pyc_running;\n"
          << "    txn.pyc_running = txn.pyc_running && ";
      if (demand == DemandKind::Branch)
        out << raw << ".isFullyKnown();\n";
      else
        out << "pyc_behavior::isOne(" << raw << ");\n";
      ordered->controls.push_back({ordinal, demand, ordered->path});
      return success();
    }
    if (orderedFifo) {
      unsigned ordinal = clocked.checkOffsets.lookup(site);
      auto raw = "pyc_control_" + std::to_string(ordinal);
      auto reached = raw + "_reached";
      out << "    const auto " << raw << " = " << predicate << ";\n"
          << "    const bool " << reached << " = txn.pyc_running;\n"
          << "    txn.pyc_running = txn.pyc_running && pyc_behavior::isOne("
          << raw << ");\n";
      orderedControls.push_back({ordinal, demand, std::move(raw), std::move(reached)});
      return success();
    }
    out << "    if (!pyc_behavior::isOne(" << predicate << ")) {\n"
        << "      if (txn.pyc_check_active && !" << predicate << ".isFullyKnown()) "
        << "txn.pyc_failed_check = pyc_check_base + " << clocked.checkOffsets.lookup(site) << ";\n"
           "      txn.pyc_running = false;\n      ";
    if (failed(emitBlockedReturn(out, body, ruleResults)))
      return failure();
    out << "    }\n";
    return success();
  };
  Operation *snapshotBoundary = rule && rule->assertions.empty() ? rule->afterSnapshot : nullptr;
  bool coldSuffix = false;
  auto nextRegion = body.begin();
  for (auto iterator = body.begin(); iterator != body.end();) {
    auto current = iterator++;
    Operation &op = *current;
    if (auto uses = clocked.dataCaptureUses.find(&op); uses != clocked.dataCaptureUses.end()) {
      for (auto [ordinal, use] : llvm::enumerate(uses->second)) {
        std::string available;
        for (unsigned capture : use.captures) {
          auto next = "txn.pyc_capture_available_" + std::to_string(capture);
          if (available.empty()) available = next;
          else if (definition.fourStateAvailability)
            available = "pyc_behavior::Level::fromPacked(gfsim::bit_and((" + available +
                        ").packed(), " + next + ".packed()))";
          else
            available = "(" + available + " && " + next + ")";
        }
        out << "    if (!";
        if (definition.fourStateAvailability)
          out << "pyc_behavior::isOne(" << available << ')';
        else
          out << available;
        out << ") { ";
        if (definition.fourStateAvailability)
          out << "if (txn.pyc_check_active && !(" << available
              << ").isFullyKnown()) txn.pyc_failed_check = pyc_check_base + "
              << clocked.checkOffsets.lookup(&op) + ordinal << "; ";
        out << "txn.pyc_running = false; ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << " }\n";
      }
    }
    if (snapshotBoundary == &op) {
      snapshot();
      out << "    if (!pyc_active) return;\n";
      if (!rule->sampled && rule->requirements.empty() && rule->guardCaptures.empty() &&
          rule->observations.empty() && clocked.rules.size() == 1 &&
          clocked.fifos.empty() && clocked.memories.empty()) {
        auto known = emitKnownRuleSuffix(out, op, clocked, values);
        if (failed(known))
          return failure();
        if (*known) {
          coldSuffix = true;
          out << "    pyc_behavior::evaluateFourState([&]() {\n";
        }
      }
    }
    if (current == nextRegion) {
      SmallVector<Operation *, 16> scalar;
      for (; nextRegion != body.end() &&
             scalar.size() < maxScalarOperations &&
             (nextRegion == current || &*nextRegion != snapshotBoundary) &&
             isNativeScalar(*nextRegion); ++nextRegion)
        scalar.push_back(&*nextRegion);
      if (scalar.empty()) {
        nextRegion = iterator;
      } else if (scalar.size() >= minScalarOperations) {
        auto emitted = emitScalarRegion(out, scalar, values);
        if (failed(emitted))
          return failure();
        if (*emitted) {
          iterator = nextRegion;
          continue;
        }
      }
    }
    SmallVector<std::string> args, results;
    for (Value input : op.getOperands())
      args.push_back(values.lookup(input));
    for (Value result : op.getResults()) {
      results.push_back(wireName(values.size()));
      values.try_emplace(result, results.back());
    }
    std::string text;
    bool borrow = false;
    if (auto effect = dyn_cast<gfir::EffectOp>(op)) {
      if (effect.getKind() == "assert") {
        unsigned ordinal = clocked.checkOffsets.lookup(effect);
        out << "    txn.pyc_assert_reached_" << ordinal << " = true;\n"
            << "    txn.pyc_assert_condition_" << ordinal
            << " = " << args.front() << ";\n";
      } else {
        const gfir::StructuredObservation *observation = nullptr;
        for (const auto &owner : clocked.rules)
          for (const auto &candidate : owner.observations)
            if (candidate.operation == effect)
              observation = &candidate;
        if (!observation)
          return effect.emitOpError("missing verified observation slot");
        unsigned slot = observation->firstSlot;
        out << "    txn." << observationReachedName(slot) << " = true;\n";
        for (const auto &argument : args)
          out << "    txn." << observationName(slot++) << " = " << argument << ";\n";
      }
      continue;
    } else if (auto loop = dyn_cast<gfir::LoopOp>(op)) {
      if (failed(emitLoop(out, loop, definition, values, results)))
        return failure();
      if (cast<gfir::StepOp>(loop.getBody().front().getTerminator()).getBreakIf() &&
          loop.getTripCountAttr().getInt() != 0) {
        out << "    if (!txn.pyc_running) { ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
      }
      continue;
    } else if (auto ordinal = dyn_cast<gfir::OrdinalBitsOp>(op)) {
      auto type = typedWire(ordinal.getValue().getType(), ordinal);
      auto width = analysis.getPackedWidth(ordinal.getValue().getType(), {}, ordinal);
      if (failed(type) || failed(width))
        return failure();
      text = *type + "::known(gfsim::Bits<" + std::to_string(*width) + ">{" +
             values.lookup(ordinal.getOrdinal()) + "})";
    } else if (auto step = dyn_cast<gfir::StepOp>(op)) {
      if (!stepTargets.empty())
        out << "    {\n      const auto pyc_next = std::tuple{"
            << join(ArrayRef<std::string>(args).take_front(step.getValues().size()), ", ")
            << "};\n      std::tie(" << join(stepTargets, ", ") << ") = pyc_next;\n    }\n";
      if (step.getBreakIf()) {
        auto predicate = values.lookup(step.getBreakIf());
        out << "    if (!" << predicate << ".isFullyKnown()) {\n"
            << "      if (txn.pyc_check_active) txn.pyc_failed_check = pyc_check_base + "
            << clocked.checkOffsets.lookup(step) << " + " << stepOrdinal << ";\n"
               "      txn.pyc_running = false;\n"
            << "      " << stepNatural << " = " << levelName << "(false);\n"
               "      break;\n    }\n"
            << "    if (pyc_behavior::isOne(" << predicate << ")) { "
            << stepNatural << " = " << levelName << "(false); break; }\n";
      }
      continue;
    } else if (auto guard = dyn_cast<gfir::GuardOp>(op)) {
      if (auto capture = clocked.guardedCaptureUses.find(guard);
          capture != clocked.guardedCaptureUses.end()) {
        auto available = "txn.pyc_capture_available_" + std::to_string(capture->second);
        out << "    if (!";
        if (definition.fourStateAvailability)
          out << "pyc_behavior::isOne(" << available << ')';
        else
          out << available;
        out << ") { ";
        if (definition.fourStateAvailability)
          out << "if (txn.pyc_check_active && !" << available
              << ".isFullyKnown()) txn.pyc_failed_check = pyc_check_base + "
              << clocked.checkOffsets.lookup(guard) - 1 << "; ";
        out << "txn.pyc_running = false; ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << " }\n";
      }
      if (failed(requireKnownOne(values.lookup(guard.getPredicate()), guard)))
        return failure();
      continue;
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      if (clocked.checkedBranches.contains(branch)) {
        auto predicate = values.lookup(branch.getPredicate());
        if (ordered) {
          if (failed(requireKnownOne(predicate, branch, DemandKind::Branch)))
            return failure();
        } else {
        out << "    if (!" << predicate << ".isFullyKnown()) {\n"
            << "      if (txn.pyc_check_active) txn.pyc_failed_check = pyc_check_base + "
            << clocked.checkOffsets.lookup(branch) << ";\n"
               "      txn.pyc_running = false;\n      ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
        }
      }
      if (failed(emitBranch(out, branch, definition, values, results, ordered)))
        return failure();
      if (clocked.checkedBranches.contains(branch) && !ordered) {
        out << "    if (!txn.pyc_running) { ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
      }
      continue;
    } else if (auto remote = clocked.childMethodCalls.find(&op);
               remote != clocked.childMethodCalls.end()) {
      const auto &binding = remote->second;
      const auto &child = *program.definitions[binding.definition].clocked;
      auto operation = child.methods[binding.method];
      auto method = methodName(operation);
      if (failed(method))
        return failure();
      auto frame = "pyc_child_call_" + std::to_string(childCallOrdinal++);
      std::string receiver, transaction = "txn";
      if (!binding.path.empty()) {
        auto instance = instancePath(binding.path);
        if (failed(instance))
          return failure();
        receiver = *instance + ".";
        transaction = frame;
        SmallVector<std::string> candidates;
        out << "    struct {\n";
        for (auto [state, slot] : binding.states) {
          auto candidate = "txn." + childCandidateName(slot);
          out << "      decltype(" << candidate << ") &" << stateName(state) << ";\n";
          candidates.push_back(std::move(candidate));
        }
        out << "    } " << frame << '{' << join(candidates, ", ") << "};\n";
      }
      if (!binding.formals.empty()) {
        SmallVector<std::pair<unsigned, std::string>> places;
        for (const auto &bound : binding.formals) {
          auto place = frame + "_" + formalStateName(bound.formal);
          std::string committed, candidate;
          if (bound.local) {
            committed = stateName(bound.slot);
            candidate = committed;
          } else {
            const auto &route = definition.childStates[bound.slot].route;
            auto owner = instancePath(route.path);
            if (failed(owner))
              return failure();
            committed = *owner + "." + stateName(route.state);
            candidate = childCandidateName(bound.slot);
          }
          std::string owner = "pyc_behavior::BoundState{" + committed +
                              ".value(), txn." + candidate + "}";
          unsigned low = bound.projection ? bound.projection->low : 0;
          if (bound.indexOperand) {
            auto table = typedWire(bound.table, &op);
            if (failed(table))
              return failure();
            owner = "pyc_behavior::bindRow<" + *table + ", " + std::to_string(low) +
                    ">(" + owner + ", " + args[*bound.indexOperand] + ")";
          } else if (bound.projection) {
            auto type = typedWire(bound.projection->types.back(), &op);
            if (failed(type))
              return failure();
            owner = "pyc_behavior::bindField<" + *type + ", " + std::to_string(low) +
                    ">(" + owner + ")";
          }
          out << "    auto " << place << " = " << owner << ";\n";
          places.emplace_back(bound.formal, std::move(place));
        }
        bindMethodArguments(out, operation, places, args, frame);
      }
      args.insert(args.begin(), "0");
      args.insert(args.begin(), transaction);
      text = receiver + *method + "(" + join(args, ", ") + ")";
    } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
      auto memoryUse = clocked.transactionMemoryCalls.find(call);
      if (memoryUse != clocked.transactionMemoryCalls.end()) {
        const auto &use = memoryUse->second;
        const auto &memory = clocked.memories[use.memory];
        auto name = instanceName(memory.instance);
        if (failed(name))
          return failure();
        if (use.access == gfir::StructuredMemoryAccess::Read) {
          out << "    txn." << memoryPinName(use.memory, 0, 0) << " = " << args.front() << ";\n"
              << "    txn." << memoryPinName(use.memory, 0, 3) << " = "
              << (orderedFifo ? "txn.pyc_running" : "true") << ";\n";
          auto address = args.front();
          if (orderedFifo) {
            auto width = std::to_string(memory.addressWidth);
            address = "(txn.pyc_running ? " + address +
                " : gfsim::wire<gfsim::Bits<" + width + ">>::known(gfsim::Bits<" + width + ">{0}))";
          }
          text = *name + ".read(" + address + ")";
        } else if (use.access == gfir::StructuredMemoryAccess::Result) {
          if (orderedFifo) {
            if (failed(requireKnownOne(*name + ".live(" + std::to_string(use.port) + ")",
                                       call, DemandKind::MemoryLive)))
              return failure();
          } else {
            out << "    if (!pyc_behavior::isOne(" << *name << ".live(" << use.port
                << "))) { txn.pyc_running = false; ";
            if (failed(emitBlockedReturn(out, body, ruleResults)))
              return failure();
            out << " }\n";
          }
          text = *name + ".result(" + std::to_string(use.port) + ")";
        } else {
          // Typed local arguments are total. Payload knownness belongs to the
          // kernel, after the entire transaction has admitted these pins.
          unsigned port = use.access == gfir::StructuredMemoryAccess::Write ? 2 : use.port;
          for (auto [argument, value] : llvm::enumerate(args))
            out << "    txn." << memoryPinName(use.memory, port, argument) << " = " << value << ";\n";
          out << "    txn." << memoryPinName(use.memory, port, 3) << " = "
              << (orderedFifo ? "txn.pyc_running" : "true") << ";\n";
          continue;
        }
      } else {
        auto use = clocked.fifoCalls.find(call);
        if (use != clocked.fifoCalls.end()) {
          unsigned fifoIndex = use->second.fifo;
          const auto &fifo = clocked.fifos[fifoIndex];
          bool isInput = use->second.access == gfir::StructuredFifoAccess::In;
          auto name = instanceName(fifo.instance);
          if (failed(name))
            return failure();
          if ((rule && rule->sampled) || ordered) {
            // Before initialization all local FIFOs share unknown committed
            // controls. Once initialized, unknown readiness can only come
            // from an effective raw pop, which the FIFO reports during Work.
            auto available = "pyc_fifo_available_" + std::to_string(clocked.checkOffsets.lookup(call));
            out << "    const auto " << available << " = " << *name;
            if (isInput) {
              out << ".ready(";
              if (fifo.downstreamPop) {
                if (ordered)
                  out << orderedFifoCredit(*ordered, fifoIndex);
                else
                  out << "txn." << recipientName(fifoIndex);
              } else {
                out << levelName << "(false)";
              }
              out << ')';
            } else {
              out << ".valid()";
            }
            out << ";\n";
            if (failed(requireKnownOne(available, call, DemandKind::Queue)))
              return failure();
          }
          // Reads use old Q. Payload writes are staged in the caller's frame.
          if (isInput) {
            if (ordered)
              out << "    txn.pyc_push_reached[" << fifoIndex << "] = txn.pyc_running;\n";
            text = "txn." + payloadName(fifoIndex) + " = " + args.front();
          } else {
            if (ordered && use->second.access == gfir::StructuredFifoAccess::Out)
              out << "    txn.pyc_pop_reached[" << fifoIndex << "] = txn.pyc_running;\n";
            if (ordered && use->second.access == gfir::StructuredFifoAccess::Out &&
                !ordered->path)
              out << "    txn.pyc_raw_pops[" << fifoIndex << "] = " << levelName << "(true);\n";
            text = *name + ".data()";
          }
        } else {
          auto target = clocked.methods[clocked.methodCalls.lookup(call)];
          auto callee = methodName(target);
          if (failed(callee))
            return failure();
          if (auto forwarding = clocked.forwardedFormals.find(call);
              forwarding != clocked.forwardedFormals.end()) {
            SmallVector<std::pair<unsigned, std::string>> places;
            for (auto [formal, actual] : forwarding->second)
              places.emplace_back(formal, formalStateName(actual));
            bindMethodArguments(out, target, places, args,
                                "pyc_child_call_" + std::to_string(childCallOrdinal++));
          }
          args.insert(args.begin(), "pyc_check_base + " + std::to_string(clocked.checkOffsets.lookup(call)));
          args.insert(args.begin(), "txn");
          text = *callee + "(" + join(args, ", ") + ")";
        }
      }
    } else if (auto formal = clocked.formalAccesses.find(&op);
               formal != clocked.formalAccesses.end()) {
      auto place = formalStateName(formal->second);
      if (isa<gfir::ReadOp>(op))
        text = place + ".value()";
      else
        text = place + ".write(" + args.front() + ")";
    } else if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      auto state = stateName(clocked.stateAccesses.lookup(read));
      text = "txn." + state + " ? *txn." + state + " : " + state + ".value()";
      borrow = canBorrowTableRead(read);
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      unsigned index = clocked.stateAccesses.lookup(write);
      auto state = stateName(index);
      std::string frame = "txn." + state;
      if (clocked.conditionalWrites.contains(index))
        out << "    txn." << intentName(index) << " = " << levelName << "(true);\n";
      if (auto path = clocked.writePaths.find(write); path != clocked.writePaths.end()) {
        if (const auto *layout = recordLayout(definition, index)) {
          emitRecordWrite(out, *layout, write, path->second, frame, values);
          continue;
        }
        out << "    {\n      if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        if (write.getIndex()) {
          auto type = typedWire(path->second.types.back(), write);
          if (failed(type))
            return failure();
          out << "      pyc_behavior::replacePackedRows<" << *type << ">(*" << frame
              << ", " << path->second.low << ", "
              << values.lookup(write.getIndex()) << ", "
              << values.lookup(write.getValue()) << ");\n";
        } else {
          out << "      " << frame << "->assignSlice(" << path->second.low << ", "
              << pack(values.lookup(write.getValue())) << ");\n";
        }
        out << "    }\n";
        continue;
      }
      if (write.getIndex()) {
        out << "    if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        text = "pyc_behavior::replace(*" + frame + ", " +
               values.lookup(write.getIndex()) + ", " + values.lookup(write.getValue()) + ")";
      } else {
        text = frame + " = " + values.lookup(write.getValue());
      }
    } else if (isa<gfir::YieldOp>(op)) {
      if (ruleResults) {
        if (!rule->afterSnapshot || !rule->assertions.empty())
          snapshot();
      } else if (args.size() == 1)
        out << "    return " << args.front() << ";\n";
      else if (!args.empty())
        out << "    return std::tuple{" << join(args, ", ") << "};\n";
      continue;
    } else if (isa<ac::TableGetOp>(op)) {
      text = "pyc_behavior::lookup(" + join(args, ", ") + ")";
    } else {
      auto value = valueExpression(op.getResult(0), args);
      if (failed(value))
        return failure();
      text = *value;
    }
    out << "    ";
    if (results.size() == 1)
      out << "const auto " << (borrow ? "&" : "") << results.front() << " = ";
    else if (!results.empty())
      out << "const auto [" << join(results, ", ") << "] = ";
    out << text << ";\n";
    auto method = clocked.methodCalls.find(&op);
    if (method != clocked.methodCalls.end() &&
        clocked.sampledMethods.contains(method->second)) {
      out << "    if (!txn.pyc_running) { ";
      if (failed(emitBlockedReturn(out, body, ruleResults)))
        return failure();
      out << "    }\n";
    }
  }
  if (ownsBranchControls) {
    SmallVector<SmallVector<unsigned>> blocks(1 + 2 * rule->checks.size());
    for (auto [index, control] : llvm::enumerate(ordered->controls)) {
      unsigned block = control.path
          ? 1 + 2 * control.path->first + (control.path->second ? 0 : 1) : 0;
      blocks[block].push_back(index);
    }
    auto raw = [](unsigned ordinal) {
      return "txn.pyc_raw_controls[" + std::to_string(ordinal) + "]";
    };
    unsigned nextDemand = 0;
    std::function<std::string(unsigned, std::string)> fold =
        [&](unsigned block, std::string continuation) -> std::string {
      for (unsigned index : llvm::reverse(blocks[block])) {
        const auto &control = ordered->controls[index];
        if (control.kind == DemandKind::MemoryLive) continue;
        auto name = "pyc_demand_" + std::to_string(nextDemand++);
        std::string expression;
        if (control.kind == DemandKind::Branch) {
          auto yes = fold(1 + 2 * control.ordinal, continuation);
          auto no = fold(2 + 2 * control.ordinal, continuation);
          expression = "pyc_behavior::Level::fromPacked(gfsim::select(" +
              raw(control.ordinal) + ".packed(), " + yes + ".packed(), " + no + ".packed()))";
        } else if (control.kind == DemandKind::Queue) {
          expression = "pyc_behavior::Level::fromPacked(gfsim::bit_and(" +
              raw(control.ordinal) + ".packed(), " + continuation + ".packed()))";
        } else {
          expression = "(pyc_behavior::isOne(" + raw(control.ordinal) + ") ? " +
              continuation + " : " + levelName.str() + "(true))";
        }
        out << "    const auto " << name << " = " << expression << ";\n";
        continuation = name;
      }
      return continuation;
    };
    auto demand = fold(0, levelName.str() + "(true)");
    out << "    const auto pyc_resource_demand = " << demand << ";\n"
           "    if (txn.pyc_check_active && (!pyc_resource_demand.isFullyKnown() || "
           "pyc_resource_demand.value().toBool())) {\n";
    for (auto [index, control] : llvm::enumerate(ordered->controls))
      out << "      " << (index ? "else " : "") << "if (txn.pyc_control_reached["
          << control.ordinal << "] && !" << raw(control.ordinal)
          << ".isFullyKnown()) txn.pyc_failed_check = pyc_check_base + "
          << control.ordinal << ";\n";
    out << "    }\n";
  } else if (orderedFifo) {
    // Match the independently verified HWIR diagnostic demand. Continue only
    // private value/candidate work after a failed control, retaining each actual
    // source-point reach. A later unavailable Queue can suppress an earlier
    // unknown Queue, but an explicit false/unknown guard preserves diagnostics.
    out << "    auto pyc_resource_demand = " << levelName << "(true);\n";
    for (const auto &control : llvm::reverse(orderedControls)) {
      if (control.demand == DemandKind::Queue)
        out << "    pyc_resource_demand = pyc_behavior::Level::fromPacked(gfsim::bit_and("
            << control.raw << ".packed(), pyc_resource_demand.packed()));\n";
      else if (control.demand == DemandKind::Boundary)
        out << "    if (!pyc_behavior::isOne(" << control.raw
            << ")) pyc_resource_demand = " << levelName << "(true);\n";
    }
    out << "    if (txn.pyc_check_active && (!pyc_resource_demand.isFullyKnown() || "
           "pyc_resource_demand.value().toBool())) {\n";
    for (auto [index, control] : llvm::enumerate(orderedControls))
      out << "      " << (index ? "else " : "") << "if (" << control.reached
          << " && !" << control.raw << ".isFullyKnown()) txn.pyc_failed_check = "
          << "pyc_check_base + " << control.ordinal << ";\n";
    out << "    }\n";
  }
  if (coldSuffix)
    out << "    });\n";
  return success();
}

LogicalResult BehavioralEmitter::emitLoop(
    raw_ostream &out, gfir::LoopOp loop, const gfir::StructuredDefinition &definition,
    const llvm::DenseMap<Value, std::string> &values,
    ArrayRef<std::string> results) {
  for (auto [result, initial, name] :
       llvm::zip(loop.getResults().drop_back(), loop.getInitial(),
                 results.drop_back())) {
    auto type = typedWire(result.getType(), loop);
    if (failed(type))
      return failure();
    out << "    " << *type << ' ' << name << " = " << values.lookup(initial) << ";\n";
  }
  uint64_t count = loop.getTripCountAttr().getInt();
  auto step = cast<gfir::StepOp>(loop.getBody().front().getTerminator());
  bool canBreak = static_cast<bool>(step.getBreakIf());
  if (canBreak)
    out << "    auto " << results.back() << " = " << levelName << "(true);\n";
  if (count) {
    auto ordinal = "pyc_ordinal_" + std::to_string(nextLoop++);
    out << "    for (std::uint64_t " << ordinal << " = 0; " << ordinal << " < "
        << count << "ULL; ++" << ordinal << ") {\n";
    auto &body = loop.getBody().front();
    llvm::DenseMap<Value, std::string> iteration(values);
    iteration.try_emplace(body.getArgument(0), ordinal);
    for (auto [argument, name] :
         llvm::zip(body.getArguments().drop_front(), results.drop_back()))
      iteration.try_emplace(argument, name);
    if (failed(emitBody(out, body, definition, nullptr, &iteration,
                        results.drop_back(), canBreak ? StringRef(results.back()) : StringRef{},
                        ordinal)))
      return failure();
    out << "    }\n";
  }
  if (!canBreak)
    out << "    const auto " << results.back() << " = " << levelName << "(true);\n";
  return success();
}

LogicalResult BehavioralEmitter::emitBranch(
    raw_ostream &out, gfir::IfOp branch, const gfir::StructuredDefinition &definition,
    const llvm::DenseMap<Value, std::string> &values,
    ArrayRef<std::string> results, OrderedFifoBranchEmission *ordered) {
  const auto &clocked = *definition.clocked;
  const auto suffix = std::to_string(nextBranch++);
  const auto thenName = "pyc_then_" + suffix;
  const auto elseName = "pyc_else_" + suffix;
  const auto thenFrame = "pyc_then_frame_" + suffix;
  const auto elseFrame = "pyc_else_frame_" + suffix;
  const auto thenValue = "pyc_then_value_" + suffix;
  const auto elseValue = "pyc_else_value_" + suffix;
  const auto predicate = values.lookup(branch.getPredicate());
  auto savedPath = ordered ? ordered->path : std::nullopt;
  for (auto [arm, name] : {std::pair{&branch.getThenBody(), thenName},
                          std::pair{&branch.getElseBody(), elseName}}) {
    if (ordered) {
      bool positive = arm == &branch.getThenBody();
      ordered->path = std::pair{unsigned(clocked.checkOffsets.lookup(branch)), positive};
      for (auto call : arm->front().getOps<gfir::CallOp>()) {
        auto use = clocked.fifoCalls.find(call);
        if (use == clocked.fifoCalls.end() ||
            use->second.access != gfir::StructuredFifoAccess::Out)
          continue;
        auto packed = predicate + ".packed()";
        if (!positive) packed = "gfsim::bit_not(" + packed + ")";
        out << "    txn.pyc_raw_pops[" << use->second.fifo
            << "] = pyc_behavior::Level::fromPacked(" << packed << ");\n";
      }
      for (auto call : arm->front().getOps<gfir::CallOp>()) {
        auto use = clocked.fifoCalls.find(call);
        if (use == clocked.fifoCalls.end() ||
            use->second.access != gfir::StructuredFifoAccess::In)
          continue;
        const auto &fifo = clocked.fifos[use->second.fifo];
        auto name = instanceName(fifo.instance);
        if (failed(name)) return failure();
        out << "    txn.pyc_raw_controls[" << clocked.checkOffsets.lookup(call)
            << "] = " << *name << ".ready("
            << (fifo.downstreamPop ? orderedFifoCredit(*ordered, use->second.fifo)
                                   : levelName.str() + "(false)") << ");\n";
      }
    }
    out << "    const auto " << name << " = [&](auto &txn) {\n";
    if (failed(emitBody(out, arm->front(), definition, nullptr, &values, {}, {}, {}, ordered)))
      return failure();
    out << "    };\n";
  }
  if (ordered) ordered->path = savedPath;
  for (auto [index, result] : llvm::enumerate(branch.getResults())) {
    auto type = typedWire(result.getType(), branch);
    if (failed(type))
      return failure();
    out << "    " << *type << ' ' << results[index] << ";\n";
  }
  auto assign = [&](StringRef arm) {
    out << "      ";
    if (results.size() == 1)
      out << results.front() << " = ";
    else if (!results.empty())
      out << "std::tie(" << join(results, ", ") << ") = ";
    out << arm << "(txn);\n";
  };
  out << "    if (" << predicate << ".isFullyKnown()) {\n"
      << "      if (" << predicate << ".value().toBool()) {\n";
  assign(thenName);
  out << "      } else {\n";
  assign(elseName);
  if (clocked.checkedBranches.contains(branch)) {
    out << "      }\n    }\n";
    return success();
  }
  out << "      }\n    } else {\n"
      << "      auto " << thenFrame << " = txn;\n"
      << "      auto " << elseFrame << " = txn;\n"
      << "      ";
  if (!results.empty())
    out << "const auto " << thenValue << " = ";
  out << thenName << '(' << thenFrame << ");\n      ";
  if (!results.empty())
    out << "const auto " << elseValue << " = ";
  out << elseName << '(' << elseFrame << ");\n";
  for (unsigned state : clocked.branchWrites.find(branch)->second) {
    if (recordLayout(definition, state)) {
      const auto owner = stateName(state);
      out << "      " << owner << ".merge(txn." << owner << ", " << predicate
          << ", " << thenFrame << '.' << owner << ", " << elseFrame << '.' << owner << ");\n";
    } else {
      auto variable = clocked.variables[state];
      auto type = typedWire(variable.getInitial().getType(), variable);
      if (failed(type))
        return failure();
      auto candidate = [&](StringRef frame) {
        auto slot = frame.str() + '.' + stateName(state);
        return pack(slot + " ? *" + slot + " : " + stateName(state) + ".value()");
      };
      auto selected = "gfsim::select(" + pack(predicate) + ", " +
                      candidate(thenFrame) + ", " + candidate(elseFrame) + ")";
      out << "      txn." << stateName(state) << " = " << unpack(*type, selected) << ";\n";
    }
    out << "      txn." << intentName(state)
        << " = pyc_behavior::Level::fromPacked(gfsim::select(" << predicate
        << ".packed(), " << thenFrame << '.' << intentName(state) << ".packed(), "
        << elseFrame << '.' << intentName(state) << ".packed()));\n";
  }
  for (auto [index, result] : llvm::enumerate(branch.getResults())) {
    auto type = typedWire(result.getType(), branch);
    if (failed(type))
      return failure();
    auto returned = [&](StringRef arm) {
      return results.size() == 1 ? arm.str()
          : "std::get<" + std::to_string(index) + ">(" + arm.str() + ")";
    };
    auto selected = "gfsim::select(" + pack(predicate) + ", " +
                    pack(returned(thenValue)) + ", " + pack(returned(elseValue)) + ")";
    out << "      " << results[index] << " = " << unpack(*type, selected) << ";\n";
  }
  out << "    }\n";
  return success();
}

#include "BehavioralNativeTransactions.inc"

LogicalResult
BehavioralEmitter::declareClocked(raw_ostream &out,
                                 const gfir::StructuredDefinition &definition) {
  const auto &clocked = *definition.clocked;
  if (needsDomain(clocked))
    out << "  pyc_behavior::SampledDomain pyc_domain;\n";
  for (const auto &fifo : clocked.fifos) {
    auto name = instanceName(fifo.instance);
    auto type = common.cppType(fifo.element, fifo.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  pyc_behavior::Fifo<" << *type << ", " << fifo.depth
        << ", gfsim::QueueReadyPolicy::"
        << (fifo.downstreamPop ? "DownstreamPop" : "LocalOccupancy") << ", "
        << fifo.latency << "> " << *name << ";\n";
  }
  for (const auto &memory : clocked.memories) {
    auto name = instanceName(memory.instance);
    auto type = common.cppType(memory.element, memory.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  pyc_behavior::" << (memory.asynchronous() ? "ByteMemory" : "SyncMemory")
        << '<' << *type << ", " << memory.addressWidth << ", " << memory.depth;
    if (!memory.asynchronous())
      out << ", " << memory.readPorts();
    out << "> " << *name << ";\n";
  }
  for (unsigned index = 0; index < clocked.variables.size(); ++index) {
    auto variable = clocked.variables[index];
    auto type = common.cppType(variable.getInitial().getType(), variable);
    if (failed(type) || failed(emitInitial(out, variable, index)))
      return failure();
    if (const auto *layout = recordLayout(definition, index))
      declareRecordState(out, *layout, *type, index, knownRecord(definition));
    else
      out << "  pyc_behavior::State<" << *type << "> " << stateName(index) << ";\n";
  }
  if (!clocked.variables.empty()) {
    out << "  void pyc_reset_state() noexcept {\n";
    for (unsigned index = 0; index < clocked.variables.size(); ++index)
      out << "    " << stateName(index) << ".reset(" << initialName(index) << "());\n";
    out << "  }\n";
  }
  for (unsigned index : clocked.methodOrder) {
    auto method = clocked.methods[index];
    auto name = methodName(method);
    if (failed(name))
      return failure();
    out << "  template <class Frame> auto " << *name
        << "(Frame &txn, unsigned pyc_check_base";
    for (auto [index, type] : llvm::enumerate(method.getFunctionType().getInputs())) {
      auto wire = typedWire(type, method);
      if (failed(wire))
        return failure();
      out << ", const " << *wire << " &" << wireName(index);
    }
    if (auto writable = method.getWritableAttr()) {
      SmallVector<unsigned> formals;
      for (auto row : writable)
        formals.push_back(cast<IntegerAttr>(row.getValue()).getValue().getZExtValue());
      llvm::sort(formals);
      for (unsigned formal : formals)
        out << ", auto &" << formalStateName(formal);
    }
    out << ") {\n";
    if (failed(emitBody(out, method.getBody().front(), definition)))
      return failure();
    out << "  }\n";
  }
  for (auto [index, rule] : llvm::enumerate(clocked.rules)) {
    auto name = "TxnFrame" + std::to_string(index);
    out << "  struct " << name << " {\n";
    for (unsigned slot : rule.childStates) {
      const auto &route = definition.childStates[slot].route;
      auto variable = program.definitions[route.definition].clocked->variables[route.state];
      auto type = typedWire(variable.getInitial().getType(), variable);
      if (failed(type))
        return failure();
      out << "    std::optional<" << *type << "> " << childCandidateName(slot) << ";\n";
    }
    for (const auto &capture : rule.guardCaptures)
      out << "    " << (definition.fourStateAvailability ? "pyc_behavior::Level" : "bool")
          << " pyc_capture_available_" << capture.capture
          << (definition.fourStateAvailability ? " = pyc_behavior::level(false);\n" : " = false;\n");
    if (rule.sampled)
      out << "    bool pyc_running = true;\n"
          << "    unsigned pyc_failed_check = " << noCheck << ";\n"
          << "    bool pyc_check_active = false;\n";
    if (hasOrderedFifoBranches(rule))
      out << "    std::array<pyc_behavior::Level, " << rule.checks.size()
          << "> pyc_raw_controls;\n"
          << "    std::array<bool, " << rule.checks.size() << "> pyc_control_reached{};\n"
          << "    std::array<pyc_behavior::Level, " << clocked.fifos.size()
          << "> pyc_raw_pops;\n"
          << "    std::array<bool, " << clocked.fifos.size() << "> pyc_pop_reached{};\n"
          << "    std::array<bool, " << clocked.fifos.size() << "> pyc_push_reached{};\n";
    for (auto assertion : rule.assertions) {
      unsigned ordinal = clocked.checkOffsets.lookup(assertion);
      out << "    bool pyc_assert_reached_" << ordinal << " = false;\n"
          << "    pyc_behavior::Level pyc_assert_condition_" << ordinal << ";\n";
    }
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      unsigned slot = observation.firstSlot;
      out << "    bool " << observationReachedName(slot) << " = false;\n";
      for (Value value : effect.getValues()) {
        auto type = typedWire(value.getType(), effect);
        if (failed(type))
          return failure();
        out << "    " << *type << ' ' << observationName(slot++) << ";\n";
      }
    }
    for (unsigned state : rule.states) {
      if (recordLayout(definition, state)) {
        out << "    " << recordLayoutName(state) << "::Candidate " << stateName(state) << ";\n";
      } else {
        auto variable = clocked.variables[state];
        auto wire = typedWire(variable.getInitial().getType(), variable);
        if (failed(wire))
          return failure();
        out << "    std::optional<" << *wire << "> " << stateName(state) << ";\n";
      }
      if (clocked.conditionalWrites.contains(state))
        out << "    pyc_behavior::Level " << intentName(state)
            << " = " << levelName << "(false);\n";
    }
    for (auto [fifoId, fifo] : llvm::enumerate(clocked.fifos)) {
      if (fifo.producer != index)
        continue;
      auto type = typedWire(fifo.element, fifo.instance);
      if (failed(type))
        return failure();
      out << "    " << *type << " " << payloadName(fifoId) << ";\n";
      if (rule.sampled && fifo.downstreamPop)
        out << "    pyc_behavior::Level " << recipientName(fifoId) << ";\n";
    }
    for (auto [memoryIndex, memory] : llvm::enumerate(clocked.memories)) {
      auto declare = [&](gfir::CallOp call, unsigned port) -> LogicalResult {
        if (!call || clocked.transactionMemoryCalls.lookup(call).rule != index)
          return success();
        for (auto [argument, type] : llvm::enumerate(call.getInputs().getTypes())) {
          auto wire = typedWire(type, call);
          if (failed(wire))
            return failure();
          out << "    " << *wire << ' ' << memoryPinName(memoryIndex, port, argument) << ";\n";
        }
        out << "    bool " << memoryPinName(memoryIndex, port, 3) << " = false;\n";
        return success();
      };
      for (unsigned port = 0; port < memory.readPorts(); ++port)
        if (failed(declare(memory.requests[port], port)))
          return failure();
      if (failed(declare(memory.reader, 0)) || failed(declare(memory.writer, 2)))
        return failure();
    }
    auto operation = rule.operation;
    for (auto [result, type] : llvm::enumerate(operation.getFunctionType().getResults())) {
      auto wire = typedWire(type, operation);
      if (failed(wire))
        return failure();
      out << "    " << *wire << ' ' << resultName(result) << ";\n";
    }
    out << "  };\n  void " << ruleName(index) << "(" << name << " &txn";
    if (rule.afterSnapshot)
      out << ", bool pyc_active";
    for (auto [input, value] : llvm::enumerate(operation.getBody().front().getArguments())) {
      auto wire = ruleInputType(value, operation);
      if (failed(wire))
        return failure();
      out << ", const " << *wire << " &" << wireName(input);
    }
    out << ") {\n";
    out << "    constexpr unsigned pyc_check_base = 0;\n";
    auto native = emitNativeTransaction(out, definition, rule);
    if (failed(native))
      return failure();
    if (*native)
      out << "    pyc_behavior::evaluateFourState([&]() {\n";
    if (failed(emitBody(out, operation.getBody().front(), definition, &rule)))
      return failure();
    if (*native)
      out << "    });\n";
    out << "  }\n";
  }
  return success();
}

FailureOr<std::string>
BehavioralEmitter::recipient(const gfir::StructuredDefinition &definition,
                             const gfir::StructuredFifo &fifo,
                             std::optional<unsigned> rule, bool ownPop) {
  if (fifo.rawOutput) {
    auto call = fifo.rawOutput;
    auto wires = definition.valueWires.find(call.getInputs().front());
    if (wires == definition.valueWires.end() || wires->second.size() != 1)
      return call.emitOpError("FIFO recipient has no single-bit graph value");
    return "pyc_behavior::Level::fromPacked(" + wireName(wires->second.front()) + ")";
  }
  if (fifo.consumer && fifo.consumer != rule) {
    if (hasOrderedFifoBranches(definition.clocked->rules[*fifo.consumer])) {
      unsigned index = &fifo - definition.clocked->fifos.data();
      auto frame = frameName(*fifo.consumer);
      return levelName.str() + "(" + frame + ".pyc_running && " + frame +
             ".pyc_pop_reached[" + std::to_string(index) + "])";
    }
    return wireName(definition.wires.size() + *fifo.consumer);
  }
  return levelName.str() + (ownPop ? "(true)" : "(false)");
}

FailureOr<std::string> BehavioralEmitter::availabilityExpression(
    const gfir::StructuredDefinition &definition,
    ArrayRef<gfir::StructuredAvailabilitySource> sources,
    StringRef instancePrefix) {
  SmallVector<std::string> terms;
  for (const auto &source : sources) {
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::FieldInput) {
      terms.push_back("pyc_input_field_available_" + std::to_string(source.index));
      continue;
    }
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::PureOutput) {
      terms.push_back("pyc_availability_call_" + std::to_string(source.index));
      continue;
    }
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::Input) {
      terms.push_back("pyc_input_available_" + std::to_string(source.index));
      continue;
    }
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::Select) {
      terms.push_back("pyc_availability_select_" + std::to_string(source.index));
      continue;
    }
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::Rule) {
      auto value = frameName(source.index) + ".pyc_running";
      terms.push_back(definition.fourStateAvailability ? levelName.str() + "(" + value + ")" : value);
      continue;
    }
    auto member = instanceName(definition.clockedCalls[source.index].receiver);
    if (failed(member))
      return failure();
    auto result = source.kind == gfir::StructuredAvailabilitySource::Kind::ChildField
                      ? fieldAvailabilityName(source.port)
                      : availabilityName(source.port);
    auto value = instancePrefix.str() + *member + "." + result;
    if (definition.fourStateAvailability &&
        !program.definitions[definition.clockedCalls[source.index].definition].fourStateAvailability)
      value = levelName.str() + "(" + value + ")";
    terms.push_back(std::move(value));
  }
  if (definition.fourStateAvailability) {
    if (terms.empty())
      return levelName.str() + "(true)";
    std::string combined = terms.front();
    for (const auto &term : ArrayRef<std::string>(terms).drop_front())
      combined = "pyc_behavior::Level::fromPacked(gfsim::bit_and((" + combined +
                 ").packed(), (" + term + ").packed()))";
    return combined;
  }
  if (terms.empty())
    return std::string("true");
  return join(terms, " && ");
}

LogicalResult BehavioralEmitter::emitGrant(raw_ostream &out,
                                            const gfir::StructuredDefinition &definition,
                                            unsigned grant, StringRef instancePrefix) {
  unsigned index = grant - definition.wires.size();
  const auto &rule = definition.clocked->rules[index];
  bool orderedFifo = rule.sampled && !rule.requirements.empty();
  if (!orderedFifo) {
    SmallVector<std::string> checks;
    for (const auto &requirement : rule.requirements) {
      const auto &fifo = definition.clocked->fifos[requirement.fifo];
      auto member = instanceName(fifo.instance);
      if (failed(member))
        return failure();
      std::string query = instancePrefix.str() + *member;
      if (requirement.requiresReady) {
        auto ready = fifo.downstreamPop
            ? recipient(definition, fifo, index, requirement.ownPop)
            : FailureOr<std::string>(levelName.str() + "(false)");
        if (failed(ready))
          return failure();
        query += ".ready(" + *ready + ")";
      } else {
        query += ".valid()";
      }
      checks.push_back(query + ".packed()");
    }
    std::string grantExpr = levelName.str() + "(true)";
    if (!checks.empty()) {
      std::string combined = checks.front();
      for (const auto &check : ArrayRef<std::string>(checks).drop_front())
        combined = "gfsim::bit_and(" + combined + ", " + check + ")";
      grantExpr = "pyc_behavior::Level::fromPacked(" + combined + ")";
    }
    out << "    const auto " << wireName(grant) << " = " << grantExpr << ";\n";
  }
  out << "    TxnFrame" << index << ' ' << frameName(index) << ";\n";
  if (rule.sampled)
    out << "    " << frameName(index) << ".pyc_check_active = pyc_active;\n";
  if (orderedFifo) {
    for (const auto &requirement : rule.requirements) {
      const auto &fifo = definition.clocked->fifos[requirement.fifo];
      if (!requirement.requiresReady || !fifo.downstreamPop)
        continue;
      auto ready = recipient(definition, fifo, index,
                             requirement.ownPop && !hasOrderedFifoBranches(rule));
      if (failed(ready))
        return failure();
      out << "    " << frameName(index) << '.' << recipientName(requirement.fifo)
          << " = " << *ready << ";\n";
    }
  }
  for (const auto &capture : rule.guardCaptures) {
    auto available = availabilityExpression(definition, capture.sources, instancePrefix);
    if (failed(available))
      return failure();
    out << "    " << frameName(index) << ".pyc_capture_available_" << capture.capture
        << " = " << *available << ";\n";
  }
  out << "    ";
  if (!orderedFifo) {
    out << "if (";
    if (needsDomain(*definition.clocked) && rule.resultUses.empty() &&
        !rule.readsByteMemory && rule.assertions.empty() && rule.observations.empty())
      out << "pyc_active && ";
    out << "pyc_behavior::isOne(" << wireName(grant) << ")) ";
  }
  out << instancePrefix << ruleName(index) << '(' << frameName(index);
  if (rule.afterSnapshot)
    out << ", pyc_active";
  auto captures = captureExpressions(definition, rule);
  if (failed(captures))
    return failure();
  for (const auto &value : *captures)
    out << ", " << value;
  out << ");\n";
  if (orderedFifo)
    out << "    const auto " << wireName(grant) << " = " << levelName << '('
        << frameName(index) << ".pyc_running);\n";
  for (auto assertion : rule.assertions) {
    unsigned ordinal = definition.clocked->checkOffsets.lookup(assertion);
    out << "    {\n      const auto pyc_error = gfsim::bit_and(gfsim::bit_and("
        << levelName << '(' << frameName(index) << ".pyc_running && "
        << frameName(index) << ".pyc_assert_reached_" << ordinal << ").packed(), "
           "gfsim::bit_not(pyc_reset.packed())), gfsim::bit_not("
        << frameName(index) << ".pyc_assert_condition_" << ordinal << ".packed()));\n"
        << "      " << assertionName(index, ordinal)
        << " = !pyc_error.isFullyKnown() || pyc_error.value() != gfsim::Bits<1>{0};\n    }\n";
  }
  if (!rule.observations.empty()) {
    auto frame = frameName(index);
    out << "    if (" << frame
        << ".pyc_running && pyc_sink && pyc_sink->slots && pyc_sink->system) {\n";
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      unsigned slot = observation.firstSlot;
      out << "      if (" << frame << '.' << observationReachedName(slot) << ") {\n";
      auto stage = [&](StringRef packed, bool eventBool) {
        out << "      pyc_sink->slots->StagePacked(pyc_observation_base + " << slot++
            << ", " << packed << ", " << (eventBool ? "true" : "false")
            << ", true, true, pyc_sink->system->cycle());\n";
      };
      if (effect.getValues().empty()) {
        stage("pyc_behavior::level(true).packed()", true);
        out << "      }\n";
        continue;
      }
      for (Value value : effect.getValues()) {
        auto width = analysis.getPackedWidth(value.getType(), {}, effect);
        if (failed(width))
          return failure();
        stage(frame + "." + observationName(slot) + ".packed()",
              effect.getKind() == "log" && *width == 1);
      }
      out << "      }\n";
    }
    out << "    }\n";
  }
  if (rule.sampled)
    out << "    if (" << frameName(index) << ".pyc_failed_check != " << noCheck
        << " && " << index << " < pyc_failed_rule) {\n"
        << "      pyc_failed_rule = " << index << ";\n"
        << "      pyc_failed_check = " << frameName(index) << ".pyc_failed_check;\n"
        << "    }\n";
  return success();
}

FailureOr<SmallVector<std::string>> BehavioralEmitter::captureExpressions(
    const gfir::StructuredDefinition &definition, const gfir::StructuredRule &rule) {
  auto operation = rule.operation;
  SmallVector<std::string> values;
  const auto *whole = wholeInput(definition);
  for (auto [argument, capture] :
       llvm::zip_equal(operation.getBody().front().getArguments(), operation.getCaptures())) {
    if (auto fields = recordCaptures.find(argument); fields != recordCaptures.end()) {
      SmallVector<std::string> parts;
      for (unsigned field : fields->second.fields)
        parts.push_back(wireName(field));
      values.push_back("std::tuple{" + join(parts, ", ") + "}");
      continue;
    }
    if (whole && capture == whole->argument) {
      values.push_back(wholeInputName.str());
      continue;
    }
    auto value = graphWire(capture, definition, operation);
    if (failed(value))
      return failure();
    values.push_back(std::move(*value));
  }
  return values;
}

LogicalResult BehavioralEmitter::emitMemoizedRule(
    raw_ostream &out, const gfir::StructuredDefinition &definition) {
  const auto &rule = definition.clocked->rules.front();
  const unsigned grant = definition.wires.size();
  const auto *whole = wholeInput(definition);
  out << "    const auto " << wireName(grant) << " = " << levelName << "(true);\n"
      << "    if (!pyc_memo_key";
  if (whole)
    out << "\n        || !pyc_behavior::sameMasks(pyc_memo_key->packed(), "
        << wholeInputName << ".packed())";
  else
    for (auto [slot, input] : llvm::enumerate(definition.preparationInputs))
      out << "\n        || !pyc_behavior::sameMasks(std::get<" << slot << ">(*pyc_memo_key), "
          << wireName(input) << ')';
  out << ") {\n      pyc_memo_key.reset();\n";
  // Capture the existing schedule before attempting its compact carrier. It
  // remains the source of truth when any input or selected row is unknown.
  std::string ordinary;
  llvm::raw_string_ostream cold(ordinary);
  SmallVector<CaptureGraphStep> schedule;
  if (whole && rule.afterSnapshot)
    for (unsigned input : definition.preparationInputs)
      cold << "      const auto " << wireName(input) << " = "
           << graphLeaf(definition.wires[input], wholeInputName) << ";\n";
  auto *graph = rule.afterSnapshot ? &cold : &out;
  if (failed(emitWires(*graph, definition, rule.dependencies, {}, false,
                       rule.afterSnapshot ? &schedule : nullptr)))
    return failure();
  auto captures = captureExpressions(definition, rule);
  if (failed(captures))
    return failure();
  if (rule.afterSnapshot) {
    std::string body;
    llvm::raw_string_ostream native(body);
    auto specialized = emitNativeCaptures(native, definition, rule, schedule, *captures);
    if (failed(specialized))
      return failure();
    if (knownRecord(definition))
      out << "      " << stateName(0) << ".discard();\n"
             "      pyc_memo_known.reset();\n"
             "      pyc_memo_values.reset();\n";
    if (*specialized)
      out << "      if (![&]() {\n" << body << "      }()) {\n";
    out << ordinary << "      pyc_memo_values.emplace(" << join(*captures, ", ") << ");\n";
    if (*specialized)
      out << "      }\n";
  } else {
    out << "      " << ruleName(0) << "(pyc_memo_frame.emplace()";
    for (const auto &capture : *captures)
      out << ", " << capture;
    out << ");\n";
  }
  out << "      pyc_memo_key.emplace(";
  if (whole)
    out << wholeInputName;
  else
    llvm::interleaveComma(definition.preparationInputs, out,
                          [&](unsigned input) { out << wireName(input); });
  out << ");\n    }\n";
  if (rule.afterSnapshot) {
    if (const auto *known = knownRecord(definition))
      out << "    if (pyc_memo_known) {\n"
             "      pyc_domain.prepare(pyc_sample);\n"
             "      if (pyc_active)\n"
          << "        " << stateName(0) << ".prepareKnown();\n"
             "      else if (pyc_sample.rising)\n"
             "        pyc_reset_state();\n"
          << "      if (!pyc_memo_known->published) {\n"
          << "        " << wholeSnapshotName(known->snapshotPort)
          << " = pyc_memo_known->snapshot;\n"
             "        pyc_memo_known->published = true;\n"
             "      }\n"
             "    } else {\n";
    out << "    TxnFrame0 " << frameName(0) << ";\n"
        << "    " << ruleName(0) << '(' << frameName(0) << ", pyc_active";
    for (unsigned i = 0; i < captures->size(); ++i)
      out << ", std::get<" << i << ">(*pyc_memo_values)";
    out << ");\n";
  } else {
    out << "    const auto &" << frameName(0) << " = *pyc_memo_frame;\n";
  }
  return success();
}

  FailureOr<ac::BoundValueId> memoryBoundValue(const ac::TransactionStatePlan &plan,
      Value original, ac::LexicalInstanceId anchor, ac::InvocationFrameId frame) {
    if (anchor >= plan.instances.size() || !charge(budget, plan.transaction, plan.values.size())) return failure();
    if (moduleValue(original, plan.instances[anchor].definition)) frame = plan.rootFrame;
    for (auto [id, ref] : llvm::enumerate(plan.values))
      if (ref.value == original && ref.anchor == anchor && ref.frame == frame) return id;
    return failure();
  }

  // Follow only the existing invocation formal/actual and Instance operand
  // bindings. Reads, saved call results and computations retain SSA identity.
  FailureOr<ac::BoundValueId> memoryBoundIdentity(const ac::TransactionStatePlan &plan,
      ac::BoundValueId id, ac::TransactionPoint) {
    for (unsigned depth = 0; depth <= 258; ++depth) {
      if (id >= plan.values.size() || !charge(budget, plan.transaction, 4)) return failure();
      const auto &ref = plan.values[id];
      auto argument = dyn_cast<BlockArgument>(ref.value);
      if (!argument) return id;
      const auto &instance = plan.instances[ref.anchor];
      if (argument.getOwner()->getParentOp() == instance.definition) {
        if (!instance.parent) return id;
        auto allocation = instance.allocation;
        if (argument.getArgNumber() >= allocation.getNumOperands()) return failure();
        auto actual = memoryBoundValue(plan, allocation.getInputs()[argument.getArgNumber()],
                                       *instance.parent, plan.rootFrame);
        if (failed(actual)) return failure();
        id = *actual;
        continue;
      }
      if (ref.frame >= plan.frames.size()) return failure();
      const auto &frame = plan.frames[ref.frame];
      bool mapped = false;
      for (auto arguments : {ArrayRef<ac::TransactionValueBinding>(frame.guardArguments),
                             ArrayRef<ac::TransactionValueBinding>(frame.bodyArguments)})
        for (const auto &binding : arguments) {
          if (!charge(budget, plan.transaction)) return failure();
          if (binding.formal == argument) { id = binding.actual; mapped = true; break; }
        }
      if (!mapped) return id;
    }
    return failure();
  }

  LogicalResult memoryControl(TransactionPreparation &prepared, uint64_t id) {
    const auto &plan = *prepared.plan;
    const auto &access = plan.memoryAccesses[id];
    if (!charge(budget, access.operation, 32)) return failure();
    if (access.kind == ac::TransactionMemoryAccessKind::Result) {
      const auto &owner = plan.memoryOwners[access.owner];
      detail::TransactionQueueResource resource;
      resource.memoryAccess = id;
      resource.control = prepared.controls.size();
      resource.predicate = owner.outputs[owner.layout.liveResult(access.port)];
      resource.kind = Builder(package.getContext()).getStringAttr("memory_live");
      uint64_t index = prepared.resources.size();
      prepared.resources.push_back(std::move(resource));
      prepared.controls.push_back({std::nullopt, std::nullopt, index});
    } else {
      detail::TransactionMemoryProposal proposal;
      proposal.access = id;
      if (access.kind == ac::TransactionMemoryAccessKind::Read) {
        proposal.reachedControlEnd = prepared.controls.size();
        ac::detail::SourceFormalProposalProof identity(plan, hardware,
            [&](ac::BoundValueId value, ac::TransactionPoint point) {
              return memoryBoundIdentity(plan, value, point);
            }, [&](Value value, ac::LexicalInstanceId anchor, ac::InvocationFrameId frame) {
              return memoryBoundValue(plan, value, anchor, frame);
            }, [&](uint64_t work) { return charge(budget, access.operation, work); });
        auto same = [&](ac::BoundValueId left, ac::BoundValueId right, ac::TransactionPoint point) {
          return ac::detail::memoryAddressesEqual(plan, left, right, point, hardware,
              [&](ac::BoundValueId value, ac::TransactionPoint context) { return identity.identity(value, context); },
              [&](uint64_t work) { return budget.debit(work, access.operation); });
        };
        for (const auto &earlier : prepared.memoryProposals) {
          if (earlier.physicalReuse) continue;
          auto covered = ac::detail::memoryReadCoveredBy(plan, earlier.access, id, same,
              [&](uint64_t work) { return budget.debit(work, access.operation); });
          if (failed(covered)) return failure();
          if (*covered) { proposal.physicalReuse = earlier.access; break; }
        }
      }
      prepared.memoryProposals.push_back(std::move(proposal));
    }
    return success();
  }

  LogicalResult memoryRoutes(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    for (const auto &proposal : prepared.memoryProposals) {
      const auto &access = plan.memoryAccesses[proposal.access];
      const auto &owner = plan.memoryOwners[access.owner];
      auto allocation = owner.allocation;
      auto path = anchorPath(plan, owner.anchor);
      for (unsigned offset = 0; offset < path.size(); ++offset) {
        auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
        if (!child) return failure();
        auto suffix = ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1);
        auto &definition = routing(child);
        for (unsigned operand : memoryOperands(access, owner.layout)) {
          QueueRoutingKey key{{routePath(package.getContext(), suffix), allocation.getOutputs()[0]}, operand};
          if (definition.memoryIncomingIndex.contains(key)) continue;
          ac::HardwareBindings bindings;
          bindings.owner = child;
          for (auto edge : suffix) {
            auto next = hardware.bindInstance(edge, bindings);
            if (failed(next)) return failure();
            bindings = *next;
          }
          auto type = hardware.resolveType(allocation.getInputs()[operand].getType(), bindings, child);
          auto predicate = hardware.resolveType(allocation.getInputs()[0].getType(), bindings, child);
          if (failed(type) || failed(predicate) || !charge(budget, access.operation, 32 + suffix.size())) return failure();
          definition.memoryIncomingIndex.try_emplace(key, definition.memoryIncoming.size());
          definition.memoryIncoming.push_back({SmallVector<ac::InstanceOp>(suffix), owner.allocation, operand, *type, *predicate});
        }
      }
    }
    return success();
  }

  LogicalResult memoryAdmissionMetadata(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    Builder b(package.getContext());
    SmallVector<Attribute> proposals;
    for (const auto &proposal : prepared.memoryProposals) {
      const auto &access = plan.memoryAccesses[proposal.access];
      const auto &owner = plan.memoryOwners[access.owner];
      if (!charge(budget, access.operation, 40 + proposal.argumentResults.size())) return failure();
      SmallVector<Attribute> arguments;
      for (uint64_t result : proposal.argumentResults) arguments.push_back(b.getI64IntegerAttr(result));
      proposals.push_back(b.getDictionaryAttr({
          b.getNamedAttr("memory", queueID(anchorPath(plan, owner.anchor), owner.allocation)),
          b.getNamedAttr("kind", b.getStringAttr(memoryKind(access.kind))),
          b.getNamedAttr("port", b.getI64IntegerAttr(access.port)),
          b.getNamedAttr("occurrence", access.occurrence), b.getNamedAttr("location", access.location),
          b.getNamedAttr("raw_path", b.getI64IntegerAttr(proposal.rawResult)),
          b.getNamedAttr("path", b.getI64IntegerAttr(proposal.pathResult)),
          b.getNamedAttr("arguments", b.getArrayAttr(arguments))}));
    }
    if (!proposals.empty()) {
      NamedAttrList admission(prepared.expectedAdmission);
      admission.append("memory_proposals", b.getArrayAttr(proposals));
      prepared.expectedAdmission = admission.getDictionary(b.getContext());
    }
    return success();
  }

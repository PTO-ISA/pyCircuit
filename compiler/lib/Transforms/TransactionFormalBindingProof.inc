      // Reconstruct invocation mode from original call descriptors and actual
      // SSA. The prepared frame's owner vector is only a locator to compare.
      unsigned explicitCount = callBinding ? callBinding->arguments.size() : 0;
      if (!product(budget, function, explicitCount,
                   4 + SourceCheckBudget::bytes(sizeof(ac::BoundValueId) +
                                                sizeof(std::optional<ac::StateOwnerId>))))
        return failure();
      replayFormalOwners[frame].resize(explicitCount);
      if (callBinding) replayFormalValues[frame].assign(callBinding->arguments.begin(),
                                                       callBinding->arguments.end());
      auto sameConstraint = [&](Attribute actual, Attribute expected) -> LogicalResult {
        auto lhs = dyn_cast_or_null<DictionaryAttr>(actual);
        auto rhs = dyn_cast_or_null<DictionaryAttr>(expected);
        if (!rhs) return reject("original writable formal constraint is absent");
        if (!lhs) {
          if (rhs.getAs<StringAttr>("source_kind").getValue() == "fixed_bits" &&
              isa<ac::BitsType>(rhs.getAs<TypeAttr>("type").getValue())) return success();
          return reject("original writable owner constraint is absent");
        }
        if (lhs.get("source_kind") != rhs.get("source_kind") ||
            lhs.get("domain") != rhs.get("domain"))
          return reject("original writable formal logical kind/domain differs");
        return success();
      };
      if (callBinding) {
        auto call = callBinding->operation;
        auto mappings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
        ac::SourceMethodView methodView{plan.instances[binding.anchor].definition, method, {}};
        auto declaredProposals = methodView.getFormalProposals();
        if (mappings) for (Attribute raw : mappings) {
          if (!charge(budget, call)) return failure();
          auto formal = cast<DictionaryAttr>(raw).get("formal");
          if (!declaredProposals || !llvm::is_contained(declaredProposals, formal))
            return reject("original owner binding has no actual callee proposal");
        }
        llvm::DenseSet<ac::StateOwnerId> actualOwners;
        llvm::DenseSet<uint64_t> callerFormals;
        if (mappings) for (Attribute raw : mappings) {
          if (!charge(budget, call, 8)) return failure();
          auto row = cast<DictionaryAttr>(raw);
          unsigned ordinal = row.getAs<IntegerAttr>("formal").getInt();
          if (ordinal >= explicitCount) return reject("original formal owner ordinal differs");
          auto constraint = cast<DictionaryAttr>(methodView.getParameters()[ordinal]).get("constraint");
          auto caller = callBinding->caller;
          std::optional<ac::StateOwnerId> physical;
          if (auto name = row.getAs<StringAttr>("state")) {
            for (auto [id, owner] : llvm::enumerate(plan.owners)) {
              if (!charge(budget, call)) return failure();
              if (owner.anchor == plan.frames[caller].anchor &&
                  owner.declaration->getAttrOfType<StringAttr>("instance_name") == name) {
                if (physical) return reject("original formal physical owner is ambiguous");
                physical = id;
              }
            }
            if (!physical) return reject("original formal physical owner is absent");
            auto actual = replayFormalProof->identity(callBinding->arguments[ordinal], point());
            if (failed(actual)) return failure();
            const ac::StateReadBinding *current = nullptr;
            for (uint64_t read = 0; read < readIndex; ++read) {
              if (!charge(budget, call)) return failure();
              if (plan.reads[read].result == *actual) current = &plan.reads[read];
            }
            if (!current || current->owner != *physical ||
                current->version != environment[*physical] ||
                failed(sameConstraint(plan.owners[*physical].declaration->getAttr("ac.payload_constraint"),
                                      constraint)))
              return reject("original writable actual is not the current whole owner read");
          } else {
            unsigned parentOrdinal = row.getAs<IntegerAttr>("caller_formal").getInt();
            if (parentOrdinal >= replayFormalValues[caller].size() ||
                !callerFormals.insert(parentOrdinal).second)
              return reject("original caller formal alias or ordinal differs");
            auto sameCurrent = replayFormalProof->equal(callBinding->arguments[ordinal],
                replayFormalValues[caller][parentOrdinal], point());
            if (failed(sameCurrent) || !*sameCurrent)
              return reject("original writable actual is not the current caller formal SSA");
            ac::SourceMethodView callerView{plan.instances[plan.frames[caller].anchor].definition,
                                           plan.frames[caller].method, {}};
            if (failed(sameConstraint(
                    cast<DictionaryAttr>(callerView.getParameters()[parentOrdinal]).get("constraint"),
                    constraint))) return failure();
            physical = replayFormalOwners[caller][parentOrdinal];
          }
          if (physical) {
            if (!actualOwners.insert(*physical).second)
              return reject("original writable owner aliases overlap");
            auto actualType = hardware.resolveType(call.getArguments()[ordinal].getType(),
                                                    plan.frames[caller].bindings, call);
            auto formalType = hardware.resolveType(method.getFunctionType().getInput(ordinal),
                                                    binding.bindings, call);
            auto ownerType = hardware.resolveType(plan.owners[*physical].payloadType,
                                                    plan.instances[0].bindings, call);
            if (failed(actualType) || failed(formalType) || failed(ownerType) ||
                !ac::areEquivalentHardwareTypes(*actualType, *formalType) ||
                !ac::areEquivalentHardwareTypes(*actualType, *ownerType) ||
                plan.owners[*physical].domain != plan.domain)
              return reject("original writable owner physical type/domain differs");
          }
          replayFormalOwners[frame][ordinal] = physical;
        }
        // Independently authenticate every owner-effect consumer of a hidden
        // proposal; source metadata cannot erase an otherwise reached demand.
        auto proposals = methodView.getFormalProposals();
        if (proposals) for (auto [ordinal, rawFormal] : llvm::enumerate(proposals)) {
          Value data = call->getResult(methodView.getNumPublicResults() + 2 * ordinal);
          Value intent = call->getResult(methodView.getNumPublicResults() + 2 * ordinal + 1);
          DictionaryAttr mapping;
          if (mappings) for (Attribute raw : mappings) {
            auto row = cast<DictionaryAttr>(raw);
            if (row.get("formal") == rawFormal) mapping = row;
          }
          unsigned consumers = 0;
          for (Operation *user : data.getUsers()) {
            if (!charge(budget, call)) return failure();
            if (auto write = dyn_cast<ac::SourceStateWriteOp>(user)) {
              if (write.getValue() != data || write.getPath() != intent || !mapping ||
                  mapping.get("state") != write.getStateAttr())
                return reject("original owner proposal consumer binding differs");
              ++consumers;
            } else if (auto write = dyn_cast<ac::SourceFormalWriteOp>(user)) {
              if (write.getPayload() != data) continue;
              if (!write.getPrevious() || write.getPath() != intent || !mapping ||
                  mapping.get("caller_formal") != write.getFormalAttr())
                return reject("original forwarded proposal binding differs");
              ++consumers;
            }
          }
          if (mapping && consumers != 1)
            return reject("original owner binding does not locate exactly one actual proposal effect consumer");
        }
      }
      if (binding.formalOwners != replayFormalOwners[frame])
        return reject("original formal invocation owner inventory differs");

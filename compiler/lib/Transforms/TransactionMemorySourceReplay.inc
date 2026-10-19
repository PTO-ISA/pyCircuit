            if (memoryIndex >= plan.memoryAccesses.size())
              return reject("original Memory access inventory is incomplete");
            const auto &access = plan.memoryAccesses[memoryIndex];
            auto anchor = plan.frames[frame].anchor;
            if (access.operation != call || access.frame != frame ||
                access.owner >= plan.memoryOwners.size() ||
                !sameSourcePoint(access.point, point()) ||
                access.occurrence != call.getOccurrence() ||
                access.location != call->getAttr("ac.location") ||
                failed(replay.identify(access.authoredPath, call.getPath(), anchor, frame)) ||
                failed(replay.requireAvailable(access.authoredPath, point())))
              return reject("original Memory access source identity differs");
            auto fact = replay.fact(access.authoredPath, point());
            if (failed(fact) || *fact != access.pathFact)
              return reject("original Memory authored path fact differs");
            const auto &owner = plan.memoryOwners[access.owner];
            auto allocation = resolved->method.nativeMemoryInstance;
            if (!allocation || owner.allocation != allocation ||
                owner.anchor != anchor ||
                owner.instancePath != plan.instances[anchor].occurrencePath ||
                owner.occurrence != allocation.getOccurrence() ||
                !allocation->hasAttr("ac.source_memory") ||
                owner.payloadConstraint != allocation->getAttr("ac.payload_constraint") ||
                owner.inputs.size() != allocation.getNumOperands() ||
                owner.outputs.size() != allocation.getNumResults())
              return reject("original Memory owner allocation differs");
            auto layout = ac::getMemoryPrimitiveLayout(
                allocation, plan.instances[anchor].bindings);
            if (failed(layout)) return failure();
            const auto &saved = owner.layout;
            if (saved.allocation != layout->allocation || saved.primitive != layout->primitive ||
                saved.kind != layout->kind || saved.readPorts != layout->readPorts ||
                saved.readEnableOperands != layout->readEnableOperands ||
                saved.readAddressOperands != layout->readAddressOperands ||
                saved.writeEnableOperand != layout->writeEnableOperand ||
                saved.writeAddressOperand != layout->writeAddressOperand ||
                saved.writeDataOperand != layout->writeDataOperand ||
                saved.writeMaskOperand != layout->writeMaskOperand ||
                !ac::areEquivalentHardwareTypes(owner.payloadType, layout->payloadType) ||
                !ac::areEquivalentHardwareTypes(saved.payloadType, layout->payloadType) ||
                !ac::areEquivalentHardwareTypes(saved.addressType, layout->addressType) ||
                !ac::areEquivalentHardwareTypes(saved.maskType, layout->maskType) ||
                !ac::areEquivalentHardwareTypes(saved.predicateType, layout->predicateType))
              return reject("original Memory physical layout differs");
            for (auto [id, value] : llvm::zip(owner.inputs, allocation.getInputs()))
              if (failed(replay.identify(id, value, anchor, plan.rootFrame))) return failure();
            for (auto [id, value] : llvm::zip(owner.outputs, allocation.getOutputs()))
              if (failed(replay.identify(id, value, anchor, plan.rootFrame))) return failure();
            auto clock = replay.endpoint(owner.inputs[0], point());
            auto reset = replay.endpoint(owner.inputs[1], point());
            if (failed(clock) || failed(reset) ||
                !(owner.domain == ac::TransactionDomain{*clock, *reset}) ||
                !(owner.domain == plan.domain))
              return reject("original Memory domain differs");
            auto action = resolved->method.nativeMemoryAction;
            auto kind = action == ac::NativeMemoryAction::Request
                ? ac::TransactionMemoryAccessKind::Request
                : action == ac::NativeMemoryAction::Result
                    ? ac::TransactionMemoryAccessKind::Result
                    : action == ac::NativeMemoryAction::Read
                        ? ac::TransactionMemoryAccessKind::Read
                        : ac::TransactionMemoryAccessKind::Write;
            bool reads = kind == ac::TransactionMemoryAccessKind::Result ||
                         kind == ac::TransactionMemoryAccessKind::Read;
            if (access.kind != kind || access.port != resolved->method.nativeMemoryPort ||
                access.result.has_value() != reads ||
                access.arguments.size() != call.getArguments().size() ||
                call.getNumResults() != unsigned(reads))
              return reject("original Memory operation signature differs");
            for (auto [id, argument] : llvm::zip(access.arguments, call.getArguments()))
              if (failed(replay.identify(id, argument, anchor, frame)) ||
                  failed(replay.requireAvailable(id, point()))) return failure();
            if (reads) {
              Value value = call.getResult(0);
              Value committed = bound(owner.outputs[layout->dataResult(access.port)]);
              if (failed(replay.identify(*access.result, value, anchor, frame)) ||
                  failed(replay.assignValidity(*access.result, point())) ||
                  record.frames[frame].lookupOrNull(value) != committed)
                return reject("original Memory result is not the committed response");
              authenticated[frame].map(value, committed);
            }
            if (failed(event(ac::TransactionStateEventKind::MemoryAccess, frame,
                             memoryIndex++))) return failure();

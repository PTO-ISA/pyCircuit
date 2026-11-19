          if (queueIndex >= plan.queueAccesses.size())
            return reject("original queue access inventory is incomplete");
          const auto &access = plan.queueAccesses[queueIndex];
          auto anchor = plan.frames[frame].anchor;
          if (access.operation != call || access.frame != frame ||
              access.owner >= plan.queueOwners.size() ||
              !sameSourcePoint(access.point, point()) ||
              access.occurrence != call.getOccurrence() ||
              access.location != call->getAttr("ac.location") ||
              failed(replay.identify(access.authoredPath, call.getPath(),
                                     anchor, frame)) ||
              failed(replay.requireAvailable(access.authoredPath, point())))
            return reject("original queue access source identity differs");
          auto fact = replay.fact(access.authoredPath, point());
          if (failed(fact) || *fact != access.pathFact)
            return reject("original queue authored path fact differs");
          const auto &owner = plan.queueOwners[access.owner];
          auto actual = ac::detail::resolveTransactionQueueReceiver(plan, anchor,
              resolved->receiver, call, [&](uint64_t work) { return success(charge(budget, call, work)); });
          if (failed(actual)) return failure();
          auto ownerAnchor = actual->first;
          auto allocation = actual->second;
          auto pins = ac::getQueuePrimitiveLayout(allocation);
          if (!allocation || owner.allocation != allocation ||
              owner.anchor != ownerAnchor ||
              owner.instancePath != plan.instances[ownerAnchor].occurrencePath ||
              owner.occurrence != allocation.getOccurrence() ||
              owner.unboundPush !=
                  allocation->hasAttr("ac.source_unbound_push") ||
              owner.unboundPop != allocation->hasAttr("ac.source_unbound_pop"))
            return reject("original queue owner allocation differs");
          for (auto [id, value] : {
                   std::pair<ac::BoundValueId, Value>{
                       owner.data, pins.data()},
                   {owner.valid, pins.valid()},
                   {owner.ready, pins.ready()},
                   {owner.localCapacity, pins.capacity()},
                   {owner.inValid, pins.push()},
                   {owner.inData, pins.payload()},
                   {owner.outReady, pins.pop()}})
            if (failed(replay.identify(id, value, ownerAnchor, plan.rootFrame)))
              return failure();
          auto clock = replay.find(pins.clock(access.kind == ac::TransactionQueueAccessKind::Push), ownerAnchor,
                                   plan.rootFrame);
          auto reset = replay.find(pins.reset(), ownerAnchor,
                                   plan.rootFrame);
          if (failed(clock) || failed(reset)) return failure();
          auto c = replay.endpoint(*clock, point());
          auto r = replay.endpoint(*reset, point());
          auto config = hardware.resolveQueue(
              allocation, plan.instances[ownerAnchor].bindings);
          auto payload = hardware.resolveType(
              pins.data().getType(),
              plan.instances[ownerAnchor].bindings, allocation);
          if (failed(c) || failed(r) || failed(config) || failed(payload) ||
              !(owner.domain == ac::TransactionDomain{*c, ac::isAsyncQueueInstance(allocation) ? plan.domain.reset : *r}) ||
              !(owner.domain == plan.domain) ||
              !ac::areEquivalentHardwareTypes(*payload, owner.payloadType) ||
              owner.config.depth != config->depth ||
              owner.config.availabilityLatency !=
                  config->availabilityLatency ||
              owner.config.readyPolicy != config->readyPolicy ||
              owner.config.tokenShape != config->tokenShape)
            return reject("original queue type/configuration/domain differs");
          auto action = resolved->method.nativeQueueAction;
          auto kind = action == ac::NativeQueueAction::Push
                          ? ac::TransactionQueueAccessKind::Push
                          : action == ac::NativeQueueAction::Pop
                                ? ac::TransactionQueueAccessKind::Pop
                                : ac::TransactionQueueAccessKind::Peek;
          if (access.kind != kind ||
              access.payload.has_value() !=
                  (kind == ac::TransactionQueueAccessKind::Push) ||
              access.result.has_value() !=
                  (kind != ac::TransactionQueueAccessKind::Push) ||
              (access.payload && !owner.unboundPush) ||
              (kind == ac::TransactionQueueAccessKind::Pop &&
               !owner.unboundPop))
            return reject("original queue access kind/raw direction differs");
          if (access.payload) {
            if (call.getArguments().size() != 1 ||
                failed(replay.identify(*access.payload,
                                       call.getArguments().front(), anchor,
                                       frame)) ||
                failed(replay.requireAvailable(*access.payload, point())))
              return failure();
          } else {
            if (call.getNumResults() != 1 ||
                failed(replay.identify(*access.result,
                                       call.getResults().front(), anchor,
                                       frame)) ||
                failed(replay.assignValidity(*access.result, point())) ||
                record.frames[frame].lookupOrNull(call.getResults().front()) !=
                    bound(owner.data))
              return reject("original queue read is not the committed head");
            authenticated[frame].map(call.getResults().front(),
                                     bound(owner.data));
          }
          if (failed(event(ac::TransactionStateEventKind::QueueAccess, frame,
                           queueIndex++)))
            return failure();

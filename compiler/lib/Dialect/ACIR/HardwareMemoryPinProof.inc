        using Paths = SmallVector<HardwareIdentityEndpoint, 0>;
        auto unionMatches = [&](const HardwareIdentityEndpoint &active,
                                ArrayRef<HardwareIdentityEndpoint> paths) -> LogicalResult {
          SmallVector<HardwareIdentityEndpoint, 0> pending;
          if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
          pending.push_back(active);
          size_t ordinal = 0;
          while (!pending.empty()) {
            auto actual = pending.pop_back_val();
            if (failed(step(module))) return failure();
            // Proposal paths are leaves even if a future path happens to use
            // OR internally. A route union cannot inspect their internals.
            if (ordinal < paths.size() && same(actual, paths[ordinal])) {
              ++ordinal; continue;
            }
            auto zero = literal(actual.value, actual.scope, false);
            if (failed(zero)) return failure();
            if (*zero) continue;
            auto joined = actual.value.getDefiningOp<BitsBinaryOp>();
            if (!joined || joined.getOpcode() != "or")
              return module->emitOpError() << "Memory incoming active union differs from its exact data contributors";
            auto left = normal(joined.getLhs(), actual.scope), right = normal(joined.getRhs(), actual.scope);
            if (failed(left) || failed(right) || failed(pay(2 * sizeof(HardwareIdentityEndpoint)))) return failure();
            pending.push_back(*right); pending.push_back(*left);
          }
          if (ordinal != paths.size())
            return module->emitOpError() << "Memory incoming active union omits a data contributor";
          return success();
        };
        std::function<FailureOr<Paths>(HardwareIdentityEndpoint, unsigned)> consume =
            [&](HardwareIdentityEndpoint actual, unsigned depth) -> FailureOr<Paths> {
          if (depth > 128 || failed(step(module))) return failure();
          SmallVector<Paths, 0> pieces;
          for (;;) {
            auto selected = memorySelection(actual);
            if (failed(selected)) return failure();
            if (!*selected) {
              auto idle = memoryIdle(actual);
              if (failed(idle) || !*idle)
                return module->emitOpError() << "Memory physical pin requires its known safe idle carrier";
              break;
            }
            const auto &selection = **selected;
            auto match = expected.end();
            for (auto candidate = expected.begin(); candidate != expected.end(); ++candidate) {
              if (failed(step(module))) return failure();
              auto path = normal(candidate->proposal->proposal->path, candidate->proposal->scope);
              if (failed(path)) return failure();
              if (!same(*path, selection.condition)) continue;
              bool dataMatches;
              if (candidate->argument) {
                auto data = normal(candidate->proposal->proposal->arguments[*candidate->argument], candidate->proposal->scope);
                if (failed(data)) return failure();
                dataMatches = same(*data, selection.payload);
              } else {
                auto one = literal(selection.payload.value, selection.payload.scope, true);
                if (failed(one)) return failure();
                dataMatches = *one;
              }
              if (dataMatches) { match = candidate; break; }
            }
            Paths current;
            if (match != expected.end()) {
              expected.erase(match);
              if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
              current.push_back(selection.condition);
            } else {
              auto path = dyn_cast<BlockArgument>(selection.condition.value),
                   data = dyn_cast<BlockArgument>(selection.payload.value);
              bool abstract = path && data && selection.condition.scope.path.empty() && selection.payload.scope.path.empty() &&
                  path.getOwner()->getParentOp() == module && data.getOwner() == path.getOwner() &&
                  path.getArgNumber() >= abstractStart && data.getArgNumber() < abstractEnd &&
                  (path.getArgNumber() - abstractStart) % 2 == 0 && data.getArgNumber() == path.getArgNumber() + 1;
              if (abstract) {
                if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
                current.push_back(selection.condition);
              } else {
                // An actual child incoming pair adds a select(active,data,idle)
                // around its parent's already merged contributions. Authenticate
                // both sides: exact payload origins below, exact active OR above.
                auto nested = consume(selection.payload, depth + 1);
                if (failed(nested) || failed(unionMatches(selection.condition, *nested))) return failure();
                current = std::move(*nested);
              }
            }
            if (failed(pay(sizeof(Paths)))) return failure();
            pieces.push_back(std::move(current));
            actual = selection.fallback;
          }
          Paths result;
          for (const auto &piece : llvm::reverse(pieces)) {
            if (failed(pay(SourceCheckBudget::multiply(piece.size(), sizeof(HardwareIdentityEndpoint))))) return failure();
            llvm::append_range(result, piece);
          }
          return result;
        };
        auto actual = normal(layout.allocation->getOperand(pin), representative.target.scope);
        if (failed(actual)) return failure();
        auto consumed = consume(*actual, 0);
        if (failed(consumed)) return failure();
        if (!expected.empty())
          return module->emitOpError() << "Memory physical pin differs from complete actual proposals";

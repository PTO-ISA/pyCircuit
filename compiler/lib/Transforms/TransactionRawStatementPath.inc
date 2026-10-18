  FailureOr<Value> rawStatementPath(Operation *site, ac::BoundValueId authored,
                                     ac::TransactionSourcePath ancestry,
                                     OpBuilder &builder,
                                     SmallVectorImpl<Operation *> *generated = nullptr) {
    auto retain = [&](Value value) -> LogicalResult {
      if (!generated) return success();
      if (!prepayAppend(budget, site, *generated)) return failure();
      generated->push_back(value.getDefiningOp()); return success();
    };
    auto one = intentLiteral(prefix.getType(), true, site->getLoc(), builder);
    if (failed(one) || failed(retain(*one))) return failure();
    Value path = *one;
    for (unsigned depth = 0; ancestry.branch; ++depth) {
      if (depth > 128 || !charge(budget, site)) return failure();
      const auto &branch = plan.branches[*ancestry.branch];
      auto condition = bound(branch.condition); if (failed(condition)) return failure();
      Value selected = *condition;
      if (!ancestry.whenTrue) {
        if (!charge(budget, site, 8)) return failure();
        OperationState invert(site->getLoc(), ac::BitsUnaryOp::getOperationName());
        invert.addOperands(selected); invert.addTypes(selected.getType());
        invert.addAttribute("opcode", builder.getStringAttr("not"));
        selected = builder.create(invert)->getResult(0);
        if (failed(retain(selected))) return failure();
      }
      auto joined = binary(path, selected, "and", site->getLoc(), builder);
      if (failed(joined) || failed(retain(*joined))) return failure();
      path = *joined; ancestry = branch.entry.path;
    }
    auto raw = bound(authored); if (failed(raw)) return failure();
    auto result = binary(path, *raw, "and", site->getLoc(), builder);
    if (failed(result) || failed(retain(*result))) return failure();
    return *result;
  }

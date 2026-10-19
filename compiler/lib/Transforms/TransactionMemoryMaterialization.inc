  LogicalResult emitMemoryAccess(Operation *operation, ac::InvocationFrameId frame,
                                  OpBuilder &builder) {
    auto found = llvm::find_if(plan.memoryAccesses, [&](const auto &access) {
      return access.operation == operation && access.frame == frame;
    });
    if (found == plan.memoryAccesses.end()) return failure();
    uint64_t id = std::distance(plan.memoryAccesses.begin(), found);
    auto path = rawStatementPath(operation, found->authoredPath, found->point.path, builder);
    if (failed(path)) return failure();
    memoryPaths[id] = *path;
    // Save the reached source prefix before the loaded data can affect later
    // controls. An async address must never depend on the final transaction fire.
    if (found->kind == ac::TransactionMemoryAccessKind::Read)
      memoryReadPrefixes[id] = prefix;
    const auto &owner = plan.memoryOwners[found->owner];
    if (found->result) {
      auto data = bound(owner.outputs[owner.layout.dataResult(found->port)]);
      Value original = plan.values[*found->result].value;
      if (failed(data) || !prepayValueMapping(budget, operation, frames[frame], original)) return failure();
      frames[frame].map(original, *data);
    }
    for (auto [ordinal, resource] : llvm::enumerate(prepared.resources)) {
      if (resource.memoryAccess != id) continue;
      auto raw = bound(resource.predicate);
      auto one = intentLiteral(prefix.getType(), true, operation->getLoc(), builder);
      if (failed(raw) || failed(one)) return failure();
      auto selected = select(*path, *raw, *one, prefix.getType(), operation->getLoc(), builder);
      if (failed(selected)) return failure();
      resources[ordinal] = *selected;
      if (failed(admitControl(resource.control, selected->getResult(), operation->getLoc(), builder, true))) return failure();
    }
    return success();
  }

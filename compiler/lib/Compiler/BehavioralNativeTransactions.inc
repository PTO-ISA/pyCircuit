// SPDX-License-Identifier: BSD-3-Clause
// Compiler-private specialization of a complete, known local transaction.
// Refusal leaves the existing four-state Rule and Method bodies untouched.
FailureOr<bool> BehavioralEmitter::emitNativeTransaction(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const gfir::StructuredRule &rule) {
  const auto &clocked = *definition.clocked;
  if (clocked.rules.size() != 1 || rule.sampled || rule.afterSnapshot ||
      definition.hasChecks() || definition.incomingState ||
      !definition.childStates.empty() || !definition.clockedCalls.empty() ||
      !clocked.fifos.empty() || !clocked.memories.empty() ||
      !rule.requirements.empty() || !rule.guardCaptures.empty() ||
      !rule.assertions.empty() || !rule.observations.empty())
    return false;

  struct NativeType {
    unsigned width;
    unsigned rows = 0;
    std::string cpp() const {
      return rows ? "std::array<std::uint64_t, " + std::to_string(rows) + ">"
                  : "std::uint64_t";
    }
  };
  llvm::DenseMap<Type, NativeType> types;
  std::function<bool(Type)> admitType = [&](Type type) {
    if (types.contains(type))
      return true;
    if (auto table = dyn_cast<ac::TableType>(type)) {
      if (table.getShape().size() != 1 || !admitType(table.getElementType()))
        return false;
      auto element = types.lookup(table.getElementType());
      auto size = analysis.getTableSize(table, {}, rule.operation);
      // Bound stack storage and generated unpacking independently of a design.
      if (element.rows || failed(size) || *size == 0 || *size > 64)
        return false;
      types.try_emplace(type, NativeType{element.width, unsigned(*size)});
      return true;
    }
    if (!isa<ac::BitsType, ac::StructType>(type))
      return false;
    if (isa<ac::StructType>(type)) {
      auto leaves = analysis.getLeaves(type, {}, rule.operation);
      if (failed(leaves) || !llvm::all_of(*leaves, [](const auto &leaf) {
            return isa<ac::BitsType>(leaf.type);
          }))
        return false;
    }
    auto width = analysis.getPackedWidth(type, {}, rule.operation);
    if (failed(width) || *width == 0 || *width > 64)
      return false;
    types.try_emplace(type, NativeType{unsigned(*width)});
    return true;
  };
  auto inRange = [&](Value key, unsigned rows) {
    if (!isa<ac::BitsType>(key.getType()) || !admitType(key.getType()))
      return false;
    unsigned width = types.lookup(key.getType()).width;
    return width < 64 && (uint64_t{1} << width) <= rows;
  };
  auto stateType = [&](unsigned state) {
    auto variable = clocked.variables[state];
    return variable.getInitial().getType();
  };
  for (unsigned state : rule.states)
    if (!admitType(stateType(state)))
      return false;

  // Validate the entire reachable body before emitting even a knownness check.
  // Methods are direct local leaves; neither nested calls nor recursion enter
  // this specialization. The existing analyzer retains semantic authority.
  unsigned remaining = 4096, calls = 0;
  llvm::DenseMap<Operation *, unsigned> fieldOffsets;
  std::function<bool(Block &, bool, unsigned)> admitBody =
      [&](Block &body, bool allowCalls, unsigned depth) {
    if (depth > 64)
      return false;
    for (Value argument : body.getArguments())
      if (!admitType(argument.getType()))
        return false;
    for (Operation &op : body) {
      if (remaining == 0)
        return false;
      --remaining;
      for (Type type : op.getOperandTypes())
        if (!admitType(type))
          return false;
      for (Type type : op.getResultTypes())
        if (!admitType(type))
          return false;
      if (isa<gfir::YieldOp>(op))
        continue;
      if (auto read = dyn_cast<gfir::ReadOp>(op)) {
        auto state = clocked.stateAccesses.find(read);
        if (state == clocked.stateAccesses.end() ||
            !llvm::is_contained(rule.states, state->second))
          return false;
      } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
        auto state = clocked.stateAccesses.find(write);
        if (state == clocked.stateAccesses.end() ||
            !llvm::is_contained(rule.states, state->second) ||
            clocked.writePaths.contains(write) || write->hasAttr("fields"))
          return false;
        auto type = types.lookup(stateType(state->second));
        if (write.getIndex() && (!type.rows || !inRange(write.getIndex(), type.rows)))
          return false;
      } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
        if (clocked.checkedBranches.contains(branch) ||
            !admitBody(branch.getThenBody().front(), false, depth + 1) ||
            !admitBody(branch.getElseBody().front(), false, depth + 1))
          return false;
      } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
        auto method = clocked.methodCalls.find(call);
        if (!allowCalls || method == clocked.methodCalls.end())
          return false;
        auto callee = clocked.methods[method->second];
        if (!admitBody(callee.getBody().front(), false, depth + 1))
          return false;
        ++calls;
      } else if (auto get = dyn_cast<ac::StructGetOp>(op)) {
        auto offset = fieldOffset(get);
        if (failed(offset))
          return false;
        fieldOffsets.try_emplace(get, *offset);
      } else if (isa<ac::StructCreateOp, ac::TableSplatOp>(op)) {
        continue;
      } else if (auto get = dyn_cast<ac::TableGetOp>(op)) {
        if (!inRange(get.getIndex(), types.lookup(get.getInput().getType()).rows))
          return false;
      } else if (!isNativeScalar(op, /*allowShifts=*/true)) {
        return false;
      }
    }
    return true;
  };
  auto operation = rule.operation;
  if (!admitBody(operation.getBody().front(), true, 0) || !calls)
    return false;

  std::string text;
  llvm::raw_string_ostream native(text);
  auto stateField = [&](unsigned state) {
    return "pyc_native_txn." + stateName(state);
  };
  auto dirty = [&](unsigned state) {
    return "pyc_native_txn.dirty_" + std::to_string(state);
  };
  SmallVector<std::pair<Type, std::string>> sources;
  for (auto [index, type] : llvm::enumerate(operation.getFunctionType().getInputs()))
    sources.emplace_back(type, wireName(index));
  for (unsigned state : rule.states) {
    auto name = "pyc_native_source_" + std::to_string(state);
    auto original = stateName(state);
    native << "    const auto &" << name << " = txn." << original
           << " ? *txn." << original << " : " << original << ".value();\n";
    sources.emplace_back(stateType(state), name);
  }
  native << "    bool pyc_native_known = true;\n";
  for (const auto &[type, source] : sources) {
    if (auto rows = types.lookup(type).rows)
      native << "    for (std::size_t row = 0; row < " << rows << "; ++row)\n"
             << "      pyc_native_known &= " << source << ".element(row).isFullyKnown();\n";
    else
      native << "    pyc_native_known &= " << source << ".isFullyKnown();\n";
  }
  native << "    if (pyc_native_known) {\n      struct {\n";
  for (unsigned state : rule.states) {
    auto type = stateType(state);
    native << "        " << types.lookup(type).cpp() << ' ' << stateName(state)
           << ";\n        bool dirty_" << state << " = false;\n";
  }
  native << "      } pyc_native_txn;\n";
  auto load = [&](Type type, StringRef destination, StringRef source) {
    if (auto rows = types.lookup(type).rows)
      native << "      for (std::size_t row = 0; row < " << rows << "; ++row)\n"
             << "        " << destination << "[row] = " << source
             << ".element(row).packed().value().value();\n";
    else
      native << "      " << destination << " = " << source << ".packed().value().value();\n";
  };
  for (unsigned state : rule.states)
    load(stateType(state), stateField(state),
         "pyc_native_source_" + std::to_string(state));

  unsigned nextValue = 0;
  auto fresh = [&] { return "pyc_native_value_" + std::to_string(nextValue++); };
  llvm::DenseMap<Value, std::string> inputs;
  for (auto [index, argument] : llvm::enumerate(operation.getBody().front().getArguments())) {
    auto name = fresh();
    native << "      " << types.lookup(argument.getType()).cpp() << ' ' << name << ";\n";
    load(argument.getType(), name, wireName(index));
    inputs.try_emplace(argument, std::move(name));
  }
  using NativeValues = llvm::DenseMap<Value, std::string>;
  std::function<FailureOr<SmallVector<std::string>>(Block &, NativeValues)> emitBlock;
  emitBlock = [&](Block &body, NativeValues values) -> FailureOr<SmallVector<std::string>> {
    for (Operation &op : body) {
      SmallVector<std::string> arguments;
      for (Value value : op.getOperands())
        arguments.push_back(values.lookup(value));
      if (auto yield = dyn_cast<gfir::YieldOp>(op))
        return arguments;
      if (auto write = dyn_cast<gfir::WriteOp>(op)) {
        unsigned state = clocked.stateAccesses.lookup(write);
        native << "      " << stateField(state);
        if (write.getIndex())
          native << '[' << values.lookup(write.getIndex()) << ']';
        native << " = " << values.lookup(write.getValue()) << ";\n"
               << "      " << dirty(state) << " = true;\n";
        continue;
      }
      if (auto branch = dyn_cast<gfir::IfOp>(op)) {
        SmallVector<std::string> results;
        for (Value result : branch.getResults()) {
          results.push_back(fresh());
          native << "      " << types.lookup(result.getType()).cpp() << ' '
                 << results.back() << ";\n";
        }
        native << "      if (" << values.lookup(branch.getPredicate()) << ") {\n";
        for (auto [index, region] : llvm::enumerate(
                 SmallVector<Region *>{&branch.getThenBody(), &branch.getElseBody()})) {
          if (index)
            native << "      } else {\n";
          auto yielded = emitBlock(region->front(), values);
          if (failed(yielded))
            return failure();
          for (auto [result, value] : llvm::zip(results, *yielded))
            native << "      " << result << " = " << value << ";\n";
        }
        native << "      }\n";
        for (auto [result, value] : llvm::zip(branch.getResults(), results))
          values.try_emplace(result, value);
        continue;
      }
      if (auto call = dyn_cast<gfir::CallOp>(op)) {
        auto method = clocked.methods[clocked.methodCalls.lookup(call)];
        NativeValues local;
        for (auto [argument, value] : llvm::zip(method.getBody().front().getArguments(), arguments))
          local.try_emplace(argument, value);
        auto returned = emitBlock(method.getBody().front(), std::move(local));
        if (failed(returned))
          return failure();
        for (auto [result, value] : llvm::zip(call.getOutputs(), *returned))
          values.try_emplace(result, value);
        continue;
      }
      if (auto get = dyn_cast<ac::TableGetOp>(op)) {
        auto name = fresh();
        native << "      const auto " << name << " = " << arguments[0]
               << '[' << arguments[1] << "];\n";
        values.try_emplace(get.getValue(), std::move(name));
        values.try_emplace(get.getInRange(), "1ULL");
        continue;
      }
      Value result = op.getResult(0);
      auto type = types.lookup(result.getType());
      std::string expression;
      bool borrow = false;
      if (auto read = dyn_cast<gfir::ReadOp>(op)) {
        expression = stateField(clocked.stateAccesses.lookup(read));
        borrow = canBorrowTableRead(read);
      } else if (isa<ac::TableSplatOp>(op)) {
        auto name = fresh();
        native << "      " << type.cpp() << ' ' << name << ";\n"
               << "      " << name << ".fill(" << arguments.front() << ");\n";
        values.try_emplace(result, std::move(name));
        continue;
      } else if (auto get = dyn_cast<ac::StructGetOp>(op)) {
        expression = "pyc_behavior::limitBits<" + std::to_string(type.width) + ">(" +
                     arguments.front() + " >> " + std::to_string(fieldOffsets.lookup(get)) + ")";
      } else if (isa<ac::StructCreateOp>(op)) {
        unsigned remaining = type.width;
        SmallVector<std::string> fields;
        for (auto [operand, argument] : llvm::zip(op.getOperands(), arguments)) {
          remaining -= types.lookup(operand.getType()).width;
          fields.push_back(remaining ? "(" + argument + " << " + std::to_string(remaining) + ")"
                                     : argument);
        }
        expression = "(" + join(fields, " | ") + ")";
      } else {
        auto computed = nativeScalarExpression(op, type.width, arguments);
        if (failed(computed))
          return failure();
        expression = "pyc_behavior::limitBits<" + std::to_string(type.width) + ">(" + *computed + ")";
      }
      auto name = fresh();
      // A single immediate lookup can borrow the table. Other reads retain a
      // snapshot across later writes to the transaction's private state.
      native << "      const auto " << (borrow ? "&" : "") << name
             << " = " << expression << ";\n";
      values.try_emplace(result, std::move(name));
    }
    return failure();
  };
  auto results = emitBlock(operation.getBody().front(), std::move(inputs));
  if (failed(results))
    return failure();

  auto publish = [&](Type type, StringRef destination, StringRef value) -> LogicalResult {
    auto wire = typedWire(type, operation);
    if (failed(wire))
      return failure();
    auto nativeType = types.lookup(type);
    auto known = [&](StringRef scalar) {
      return carrier(nativeType.width) + "::known(gfsim::Bits<" +
             std::to_string(nativeType.width) + ">{" + scalar.str() + "})";
    };
    if (auto table = dyn_cast<ac::TableType>(type)) {
      auto row = typedWire(table.getElementType(), operation);
      if (failed(row))
        return failure();
      auto name = fresh();
      native << "      " << *wire << ' ' << name << ";\n"
             << "      for (std::size_t row = 0; row < " << nativeType.rows << "; ++row)\n"
             << "        " << name << ".element(row) = "
             << unpack(*row, known(value.str() + "[row]")) << ";\n"
             << "      " << destination << " = std::move(" << name << ");\n";
    } else {
      native << "      " << destination << " = " << unpack(*wire, known(value)) << ";\n";
    }
    return success();
  };
  // All work and validation finished in a private frame. Publish each dirty
  // candidate once; unmodified owners retain their original optional/intent.
  for (unsigned state : rule.states) {
    native << "      if (" << dirty(state) << ") {\n";
    if (failed(publish(stateType(state),
                       "txn." + stateName(state), stateField(state))))
      return failure();
    if (clocked.conditionalWrites.contains(state))
      native << "      txn." << intentName(state) << " = " << levelName << "(true);\n";
    native << "      }\n";
  }
  for (auto [index, type] : llvm::enumerate(operation.getFunctionType().getResults()))
    if (failed(publish(type, "txn." + resultName(index), (*results)[index])))
      return failure();
  native << "      return;\n    }\n";
  native.flush();
  out << text;
  return true;
}

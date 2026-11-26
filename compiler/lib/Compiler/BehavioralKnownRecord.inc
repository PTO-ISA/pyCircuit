// SPDX-License-Identifier: BSD-3-Clause

std::optional<KnownRecordPlan> BehavioralEmitter::planKnownRecord(
    const gfir::StructuredDefinition &definition,
    const gfir::StructuredRule &rule) const {
  const auto &plan = *definition.clocked;
  if (plan.variables.size() != 1 || rule.writes.size() != 1 ||
      rule.resultUses.size() != 1 || !definition.outputAvailability.empty() ||
      llvm::any_of(definition.fieldOutputs, [](const auto &field) {
        return !field.sources.empty();
      }))
    return std::nullopt;
  const auto *layout = recordLayout(definition, 0);
  if (!layout)
    return std::nullopt;
  auto operation = rule.operation;
  auto &body = operation.getBody().front();
  auto yield = cast<gfir::YieldOp>(body.getTerminator());
  if (rule.afterSnapshot != &body.front() || yield.getValues().size() != 1)
    return std::nullopt;
  auto snapshot = dyn_cast<BlockArgument>(yield.getValues().front());
  if (!snapshot || snapshot.getOwner() != &body ||
      !isa<ac::StructType>(snapshot.getType()))
    return std::nullopt;
  const auto &outputs = wholeOutputs[&definition - program.definitions.data()];
  std::optional<unsigned> port;
  for (const auto &[index, output] : outputs) {
    if (!output.snapshot)
      continue;
    if (port || output.source != frameName(0) + "." + resultName(0))
      return std::nullopt;
    port = index;
  }
  if (!port)
    return std::nullopt;
  KnownRecordPlan known{*port, snapshot.getArgNumber(), {}};

  auto scalar = [&](Value value) -> std::optional<unsigned> {
    auto argument = dyn_cast<BlockArgument>(value);
    if (!argument || argument.getOwner() != &body)
      return std::nullopt;
    auto input = operation.getCaptures()[argument.getArgNumber()];
    auto found = definition.valueWires.find(input);
    if (found == definition.valueWires.end() || found->second.size() != 1)
      return std::nullopt;
    unsigned wire = found->second.front();
    const auto &leaf = definition.wires[wire];
    if (!isa<ac::BitsType>(leaf.type) || leaf.width > 64)
      return std::nullopt;
    return wire;
  };
  llvm::SmallDenseSet<unsigned> written;
  bool hasRow = false;
  auto add = [&](const RecordFieldLayout &field, unsigned value,
                 std::optional<unsigned> row, std::optional<unsigned> condition) {
    const auto &leaf = definition.wires[value];
    if (!isa<ac::BitsType>(leaf.type) || leaf.width > 64 ||
        (!row && (!isa<ac::BitsType>(field.type) || leaf.width != field.width)) ||
        !written.insert(field.low).second)
      return false;
    known.edits.push_back({&field, value, leaf.width, row, condition});
    return true;
  };
  auto writeEdit = [&](gfir::WriteOp write, std::optional<unsigned> condition) {
    auto found = plan.writePaths.find(write);
    if (found == plan.writePaths.end() || plan.stateAccesses.lookup(write) != 0)
      return false;
    const auto &path = found->second;
    if (write.getIndex()) {
      auto field = llvm::find_if(*layout, [&](const auto &field) {
        return field.low == path.low && field.indexWidth;
      });
      auto row = scalar(write.getIndex()), value = scalar(write.getValue());
      if (hasRow || field == layout->end() || !row || !value)
        return false;
      hasRow = true;
      return add(*field, *value, row, condition);
    }
    if (condition)
      return false;
    auto capture = recordCaptures.find(write.getValue());
    if (capture != recordCaptures.end()) {
      unsigned next = 0;
      for (const auto &field : *layout) {
        if (!fieldInPath(field, path))
          continue;
        if (next == capture->second.fields.size() ||
            !add(field, capture->second.fields[next++], {}, {}))
          return false;
      }
      return next != 0 && next == capture->second.fields.size();
    }
    auto field = llvm::find_if(*layout, [&](const auto &field) {
      return field.low == path.low && field.width == path.width && !field.indexWidth;
    });
    auto value = scalar(write.getValue());
    return field != layout->end() && value && add(*field, *value, {}, {});
  };

  for (Operation &op : body.without_terminator()) {
    if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      if (!writeEdit(write, {}))
        return std::nullopt;
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      auto condition = scalar(branch.getPredicate());
      auto &yes = branch.getThenBody().front();
      auto &no = branch.getElseBody().front();
      if (branch->getNumResults() || !condition ||
          std::distance(yes.begin(), yes.end()) != 2 || !no.without_terminator().empty())
        return std::nullopt;
      auto write = dyn_cast<gfir::WriteOp>(yes.front());
      if (!write || !writeEdit(write, condition))
        return std::nullopt;
    } else if (!isa<ac::BitsConstantOp, ac::BitsConstant4Op>(op) || !op.use_empty()) {
      return std::nullopt;
    }
  }
  if (known.edits.empty())
    return std::nullopt;
  return known;
}

const KnownRecordPlan *BehavioralEmitter::knownRecord(
    const gfir::StructuredDefinition &definition) const {
  auto found = knownRecords.find(definition.module);
  return found == knownRecords.end() ? nullptr : &found->second;
}

void BehavioralEmitter::declareKnownRecord(raw_ostream &out, const KnownRecordPlan &plan) {
  out << "    struct KnownEdit {\n";
  for (const auto &edit : plan.edits) {
    auto field = recordFieldName(edit.field->low);
    out << "      " << nativeInteger(edit.width) << ' ' << field << ";\n";
    if (edit.row)
      out << "      " << nativeInteger(edit.field->indexWidth) << ' ' << field << "_row;\n"
          << "      bool " << field << "_enabled;\n";
  }
  out << "    };\n"
         "    void applyKnown(Wire &mirror, const KnownEdit &edit) noexcept {\n"
         "      auto image = mirror.packed().value();\n";
  for (const auto &edit : plan.edits) {
    auto field = recordFieldName(edit.field->low);
    auto low = std::to_string(edit.field->low);
    if (edit.row) {
      out << "      if (edit." << field << "_enabled && edit." << field
          << "_row < decltype(" << field << ")::size) {\n"
          << "        const auto row = edit." << field << "_row;\n"
          << "        const unsigned low = " << low << " + (decltype(" << field
          << ")::size - 1 - row) * " << edit.width << ";\n"
          << "        pyc_behavior::applyKnownRecordRow(image, " << field
          << ".element(row), low, edit." << field << ");\n"
          << "      }\n";
    } else {
      out << "      " << field << " = pyc_behavior::limitBits<" << edit.width
          << ">(edit." << field << ");\n"
          << "      pyc_behavior::writeKnownBits<" << edit.width << ">(image, "
          << low << ", " << field << ");\n";
    }
  }
  out << "      mirror = Wire::fromPacked(Wire::packed_type::known(image));\n"
         "    }\n";
}

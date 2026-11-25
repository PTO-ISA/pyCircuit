// SPDX-License-Identifier: BSD-3-Clause

std::string recordLayoutName(unsigned state) {
  return stateName(state) + "_layout";
}

std::string recordFieldName(unsigned low) {
  return "field_" + std::to_string(low);
}

bool fieldInPath(const RecordFieldLayout &leaf, const gfir::StructuredWritePath &path) {
  return leaf.low >= path.low && leaf.low + leaf.width <= path.low + path.width;
}

LogicalResult BehavioralEmitter::planRecordStates() {
  for (const auto &definition : program.definitions) {
    if (!definition.clocked)
      continue;
    const auto &clocked = *definition.clocked;
    for (unsigned state = 0; state < clocked.variables.size(); ++state) {
      auto variable = clocked.variables[state];
      const auto type = variable.getInitial().getType();
      if (!isa<ac::StructType>(type) || definition.incomingState == state ||
          llvm::count_if(clocked.rules, [&](const auto &rule) {
            return llvm::is_contained(rule.writes, state);
          }) != 1)
        continue;
      auto width = analysis.getPackedWidth(type, {}, variable);
      auto leaves = analysis.getLeaves(type, {}, variable);
      if (failed(width) || failed(leaves))
        return failure();
      if (*width <= 128 || *width > std::numeric_limits<unsigned>::max())
        continue;

      RecordStateLayout layout;
      for (const auto &leaf : *leaves) {
        auto table = dyn_cast<ac::TableType>(leaf.type);
        if (table && (table.getShape().size() != 1 ||
                      !isa<ac::BitsType>(table.getElementType())))
          break;
        auto wire = typedWire(leaf.type, variable);
        if (failed(wire))
          return failure();
        layout.push_back({static_cast<unsigned>(leaf.low),
                          static_cast<unsigned>(leaf.width), leaf.type, std::move(*wire)});
      }
      if (layout.size() != leaves->size())
        continue;

      // A frame for this layout contains only edits. Whole-value and
      // transaction-local reads still require the ordinary packed frame.
      bool supported = true;
      llvm::DenseMap<Value, RecordCaptureLayout> captures;
      for (auto [op, owner] : clocked.stateAccesses) {
        if (owner != state)
          continue;
        if (isa<gfir::ReadOp>(op) && op->getParentOp() == definition.module)
          continue;
        auto write = dyn_cast<gfir::WriteOp>(op);
        auto writePath = clocked.writePaths.find(op);
        auto rule = op->getParentOfType<gfir::RuleOp>();
        if (!write || writePath == clocked.writePaths.end() ||
            !rule || op->getParentOfType<gfir::LoopOp>() ||
            op->getParentOfType<gfir::MethodOp>()) {
          supported = false;
          break;
        }
        const auto &path = writePath->second;
        SmallVector<RecordFieldLayout *> fields;
        unsigned covered = 0;
        for (auto &field : layout) {
          if (!fieldInPath(field, path))
            continue;
          fields.push_back(&field);
          covered += field.width;
        }
        if (fields.empty() || covered != path.width ||
            llvm::any_of(fields, [&](const auto *field) {
              return isa<ac::TableType>(field->type) && (!write.getIndex() || field->written);
            })) {
          supported = false;
          break;
        }
        if (write.getIndex()) {
          auto indexWidth = analysis.getPackedWidth(write.getIndex().getType(), {}, write);
          if (failed(indexWidth))
            return failure();
          if (*indexWidth > 64) {
            supported = false;
            break;
          }
          fields.front()->indexWidth = *indexWidth;
        } else if (isa<ac::StructType>(path.types.back())) {
          auto argument = dyn_cast<BlockArgument>(write.getValue());
          if (!argument || argument.getOwner() != &rule.getBody().front() ||
              !argument.hasOneUse()) {
            supported = false;
            break;
          }
          // The aggregate is used only to assign these leaf positions. Keep
          // the actual graph values through Rule capture and memoization.
          auto capture = rule.getCaptures()[argument.getArgNumber()];
          auto bindings = definition.valueWires.find(capture);
          if (bindings == definition.valueWires.end()) {
            supported = false;
            break;
          }
          SmallVector<unsigned> inputs(bindings->second);
          llvm::sort(inputs, [&](unsigned a, unsigned b) {
            return definition.wires[a].low > definition.wires[b].low;
          });
          if (inputs.size() != fields.size() ||
              !llvm::all_of(llvm::zip_equal(inputs, fields), [&](const auto &pair) {
                const auto &[input, field] = pair;
                const auto &wire = definition.wires[input];
                return wire.low == field->low - path.low && wire.width == field->width &&
                       !isa<ac::TableType>(wire.type);
              })) {
            supported = false;
            break;
          }
          SmallVector<std::string> types;
          for (unsigned input : inputs)
            types.push_back(carrier(definition.wires[input].width));
          captures.try_emplace(argument, RecordCaptureLayout{
              std::move(inputs), "std::tuple<" + join(types, ", ") + ">"});
        }
        for (auto *field : fields)
          field->written = true;
      }
      if (supported) {
        recordLayouts.try_emplace(variable, std::move(layout));
        for (auto &[argument, capture] : captures)
          recordCaptures.try_emplace(argument, std::move(capture));
      }
    }
  }
  return success();
}

const RecordStateLayout *BehavioralEmitter::recordLayout(
    const gfir::StructuredDefinition &definition, unsigned state) const {
  auto position = recordLayouts.find(definition.clocked->variables[state]);
  return position == recordLayouts.end() ? nullptr : &position->second;
}

std::optional<std::string> BehavioralEmitter::recordField(
    const gfir::StructuredDefinition &definition, const gfir::StructuredWire &wire,
    StringRef instancePrefix) const {
  if (!wire.state)
    return std::nullopt;
  const auto *layout = recordLayout(definition, *wire.state);
  if (!layout || llvm::none_of(*layout, [&](const RecordFieldLayout &field) {
        return field.low == wire.low && field.width == wire.width;
      }))
    return std::nullopt;
  auto value = instancePrefix.str() + stateName(*wire.state) + ".fields." +
               recordFieldName(wire.low);
  if (!isa<ac::TableType>(wire.type))
    value += ".packed()";
  return value;
}

FailureOr<std::string> BehavioralEmitter::ruleInputType(BlockArgument argument,
                                                        gfir::RuleOp rule) {
  if (auto found = recordCaptures.find(argument); found != recordCaptures.end())
    return found->second.type;
  return typedWire(argument.getType(), rule);
}

void BehavioralEmitter::emitRecordWrite(
    raw_ostream &out, const RecordStateLayout &layout, gfir::WriteOp write,
    const gfir::StructuredWritePath &path, StringRef frame,
    const llvm::DenseMap<Value, std::string> &values) {
  auto value = values.lookup(write.getValue());
  if (recordCaptures.contains(write.getValue())) {
    unsigned ordinal = 0;
    for (const auto &field : layout) {
      if (!fieldInPath(field, path))
        continue;
      out << "    " << frame << '.' << recordFieldName(field.low) << " = "
          << field.wire << "::fromPacked(std::get<" << ordinal++ << ">(" << value << "));\n";
    }
    return;
  }
  out << "    " << frame << '.' << recordFieldName(path.low) << " = ";
  if (write.getIndex())
    out << "pyc_behavior::RecordRowEdit{" << values.lookup(write.getIndex())
        << ", " << value << '}';
  else
    out << value;
  out << ";\n";
}

void BehavioralEmitter::declareRecordState(raw_ostream &out, const RecordStateLayout &layout,
                                           StringRef payload, unsigned state) {
  const auto name = recordLayoutName(state);
  out << "  struct " << name << " {\n"
      << "    using Wire = gfsim::wire<" << payload << ">;\n"
      << "    struct Candidate {\n";
  for (const auto &field : layout) {
    if (!field.written)
      continue;
    out << "      std::optional<";
    if (field.indexWidth)
      out << "pyc_behavior::RecordRowEdit<gfsim::wire<gfsim::Bits<" << field.indexWidth
          << ">>, gfsim::wire<" << field.wire << "::element_type>>";
    else
      out << field.wire;
    out << "> " << recordFieldName(field.low) << ";\n";
  }
  out << "    };\n";
  for (const auto &field : layout)
    out << "    " << field.wire << ' ' << recordFieldName(field.low) << ";\n";

  out << "    void load(const Wire &value) noexcept {\n";
  for (const auto &field : layout)
    out << "      " << recordFieldName(field.low) << " = pyc_behavior::unpack<"
        << field.wire << ">(pyc_behavior::slice<" << field.width << ">(value.packed(), "
        << field.low << "));\n";
  out << "    }\n";

  out << "    void apply(Wire &mirror, const Candidate &candidate) noexcept {\n";
  for (const auto &field : layout) {
    if (!field.written)
      continue;
    const auto member = recordFieldName(field.low);
    out << "      pyc_behavior::" << (field.indexWidth ? "applyRecordRow" : "applyRecordField")
        << '<' << field.low << ">(mirror, " << member << ", candidate." << member << ");\n";
  }
  out << "    }\n"
      << "    void merge(Candidate &result, const pyc_behavior::Level &condition, "
         "const Candidate &yes, const Candidate &no) const noexcept {\n";
  for (const auto &field : layout) {
    if (!field.written)
      continue;
    const auto member = recordFieldName(field.low);
    out << "      result." << member << " = pyc_behavior::";
    if (field.indexWidth)
      out << "mergeRecordRowEdit(";
    else
      out << "mergeRecordField(condition, " << member << ", ";
    out << "yes." << member << ", no." << member << ");\n";
  }
  out << "    }\n  };\n"
      << "  pyc_behavior::RecordState<" << name << "::Wire, " << name << "> "
      << stateName(state) << ";\n";
}

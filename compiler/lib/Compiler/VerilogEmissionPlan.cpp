#include "VerilogEmissionPlan.h"

#include "FinalEmissionNames.h"
#include "HardwareEmitCommon.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {
FailureOr<std::string> identifier(StringRef raw, Operation *site) {
  auto value = legalizeIdentifier(raw, [&] { return site->emitOpError(); });
  if (failed(value))
    return failure();
  return *value;
}

template <class Occurrence>
FailureOr<SmallVector<VerilogParameterPlan>>
buildParameters(HardwareEmitContext &context, Occurrence instance,
                Operation *callee, bool primitive, bool managedPrimitive) {
  SmallVector<VerilogParameterPlan> parameters;
  auto formals = callee->getAttrOfType<ArrayAttr>("parameters");
  if (primitive) {
    for (auto [index, raw] : llvm::enumerate(instance.getParameters())) {
      auto formal = cast<DictionaryAttr>(formals[index]);
      auto name = identifier(
          formal.template getAs<StringAttr>("name").getValue(), instance);
      auto value =
          context.staticRtlValue(cast<ac::StaticExprAttr>(raw), instance);
      if (failed(name) || failed(value))
        return failure();
      parameters.push_back({std::move(*name), std::move(*value)});
    }
  } else {
    // Generated definitions declare values before types. Keep positional
    // overrides so a formal cannot collide with a local SystemVerilog type.
    ac::HardwareBindings parent;
    parent.owner = instance->getParentOp();
    auto bindings = context.analysis.bindInstance(instance, parent);
    if (failed(bindings))
      return failure();
    for (auto raw : formals) {
      auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
      auto value = bindings->integers.lookup(name.getValue());
      if (auto integer = dyn_cast<ac::MathIntAttr>(value)) {
        parameters.push_back({std::nullopt, integer.getCanonicalValue().str()});
      } else {
        auto rendered = context.staticRtlValue(cast<ac::StaticExprAttr>(value),
                                               *bindings, instance);
        if (failed(rendered))
          return failure();
        parameters.push_back({std::nullopt, std::move(*rendered)});
      }
    }
  }

  auto typeFormals = callee->getAttrOfType<ArrayAttr>("type_parameters");
  for (auto [index, raw] : llvm::enumerate(instance.getTypeArguments())) {
    auto type = context.rtlType(cast<TypeAttr>(raw).getValue(), instance);
    if (failed(type))
      return failure();
    if (primitive) {
      auto name =
          identifier(cast<StringAttr>(typeFormals[index]).getValue(), instance);
      if (failed(name))
        return failure();
      parameters.push_back({std::move(*name), std::move(*type)});
    } else {
      parameters.push_back({std::nullopt, std::move(*type)});
    }
  }
  if (managedPrimitive)
    parameters.push_back(
        {std::string("pyc_managed"), std::string("1'b1"), true});
  return parameters;
}
} // namespace

FailureOr<VerilogModuleEmissionPlan>
buildVerilogModuleEmissionPlan(HardwareEmitContext &context,
                               ac::ModuleOp definition) {
  VerilogModuleEmissionPlan modulePlan;
  for (auto raw : definition.getParameters()) {
    auto formal = cast<DictionaryAttr>(raw);
    auto name = formal.getAs<StringAttr>("name");
    auto id = context.rtlFormalName(name.getValue(), definition);
    if (failed(id))
      return failure();
    std::string text = "parameter longint signed " + *id;
    if (auto value = formal.getAs<ac::StaticExprAttr>("default")) {
      auto rendered = context.staticRtlValue(value, definition);
      if (failed(rendered))
        return failure();
      text += " = " + *rendered;
    } else {
      text += " = 1";
    }
    modulePlan.parameters.push_back(std::move(text));
  }
  for (auto raw : definition.getTypeParameters()) {
    auto id =
        context.rtlFormalName(cast<StringAttr>(raw).getValue(), definition);
    if (failed(id))
      return failure();
    modulePlan.parameters.push_back("parameter type " + *id + " = logic [0:0]");
  }
  auto appendModulePorts =
      [&](ArrayAttr rawNames, TypeRange types,
          VerilogPortDirection direction) -> LogicalResult {
    for (auto [raw, type] : llvm::zip(rawNames, types)) {
      auto name = identifier(cast<StringAttr>(raw).getValue(), definition);
      auto rendered = context.rtlType(type, definition);
      if (failed(name) || failed(rendered))
        return failure();
      modulePlan.ports.push_back(
          {direction, std::move(*rendered), std::move(*name)});
    }
    return success();
  };
  auto signature = definition.getFunctionType();
  if (failed(appendModulePorts(definition.getInputNames(),
                               signature.getInputs(),
                               VerilogPortDirection::Input)) ||
      failed(appendModulePorts(definition.getOutputNames(),
                               signature.getResults(),
                               VerilogPortDirection::Output)))
    return failure();
  if (context.managesChecks()) {
    modulePlan.ports.append({
        {VerilogPortDirection::Input, "[2:0]", "pyc_phase"},
        {VerilogPortDirection::Input, {}, "pyc_root_commit_ok"},
        {VerilogPortDirection::Input, {}, "pyc_reset_active"},
        {VerilogPortDirection::Output, {}, "pyc_check_complete"},
        {VerilogPortDirection::Output, {}, "pyc_sampling_error"},
        {VerilogPortDirection::Output, {}, "pyc_local_error"},
    });
  }
  auto yield = cast<ac::YieldOp>(definition.getBody().front().back());
  for (auto [rawName, value] :
       llvm::zip(definition.getOutputNames(), yield.getValues())) {
    auto name = identifier(cast<StringAttr>(rawName).getValue(), definition);
    if (failed(name))
      return failure();
    modulePlan.outputAssignments.push_back({std::move(*name), value});
  }
  auto buildCombinational = [&](Operation &operation) -> LogicalResult {
    if (isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsUnaryOp,
            ac::BitsBinaryOp, ac::BitsCompareOp, ac::BitsSelectOp,
            ac::BitsConcatOp, ac::BitsExtractOp, ac::BitsResizeOp,
            ac::EnumCreateOp, ac::EnumToBitsOp, ac::EnumFromBitsOp,
            ac::StructCreateOp, ac::StructGetOp>(operation)) {
      VerilogCombinationalPlan plan;
      plan.operation = &operation;
      for (Value result : operation.getResults()) {
        auto resultType = context.rtlType(result.getType(), &operation);
        if (failed(resultType))
          return failure();
        plan.resultTypes.push_back(std::move(*resultType));
      }
      if (auto pattern = dyn_cast<ac::BitsConstant4Op>(operation)) {
        plan.kind = VerilogCombinationalKind::BitsConstant4;
        plan.staticValue = std::to_string(pattern.getPattern().size()) +
                           "'b" + pattern.getPattern().str();
      } else if (auto constant = dyn_cast<ac::BitsConstantOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Constant;
        auto literal = context.staticRtlValue(constant.getValue(), constant);
        auto width = context.staticRtlValue(
            cast<ac::BitsType>(constant.getResult().getType()).getWidth(),
            constant);
        if (failed(literal) || failed(width))
          return failure();
        if (context.isClosedStatic(constant.getValue())) {
          llvm::APSInt value(*literal);
          *literal = std::to_string(std::max(1u, value.getActiveBits())) +
                     "'d" + *literal;
        }
        plan.staticValue = "(" + *width + ")'( " + *literal + " )";
      } else if (auto unary = dyn_cast<ac::BitsUnaryOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Unary;
        if (unary.getOpcode() != "not")
          return unary.emitOpError() << "unsupported RTL unary op";
        plan.opcode = "~";
      } else if (auto binary = dyn_cast<ac::BitsBinaryOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Binary;
        plan.alwaysComb =
            binary.getOpcode() == "udiv" || binary.getOpcode() == "urem";
        plan.signedOperands = binary.getOpcode() == "ashr";
        plan.opcode = llvm::StringSwitch<StringRef>(binary.getOpcode())
                          .Case("and", "&")
                          .Case("or", "|")
                          .Case("xor", "^")
                          .Case("add", "+")
                          .Case("sub", "-")
                          .Case("mul", "*")
                          .Case("shl", "<<")
                          .Case("lshr", ">>")
                          .Case("ashr", ">>>")
                          .Case("udiv", "/")
                          .Case("urem", "%")
                          .Default("")
                          .str();
        if (plan.opcode.empty())
          return binary.emitOpError() << "unsupported RTL binary op";
      } else if (auto compare = dyn_cast<ac::BitsCompareOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Compare;
        plan.signedOperands = compare.getPredicate().starts_with("s");
        plan.opcode = llvm::StringSwitch<StringRef>(compare.getPredicate())
                          .Case("eq", "==")
                          .Case("ne", "!=")
                          .Case("case_eq", "===")
                          .Case("ult", "<")
                          .Case("ule", "<=")
                          .Case("ugt", ">")
                          .Case("uge", ">=")
                          .Case("slt", "<")
                          .Case("sle", "<=")
                          .Case("sgt", ">")
                          .Case("sge", ">=")
                          .Default("")
                          .str();
        if (plan.opcode.empty())
          return compare.emitOpError() << "unsupported RTL comparison";
      } else if (isa<ac::BitsSelectOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Select;
      } else if (isa<ac::BitsConcatOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Concat;
      } else if (auto extract = dyn_cast<ac::BitsExtractOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Extract;
        auto low = context.staticRtlValue(extract.getLow(), extract);
        auto width = context.staticRtlValue(
            cast<ac::BitsType>(extract.getResult().getType()).getWidth(),
            extract);
        if (failed(low) || failed(width))
          return failure();
        plan.staticValue = "(" + *low + ") +: " + *width;
      } else if (auto resize = dyn_cast<ac::BitsResizeOp>(operation)) {
        plan.kind = VerilogCombinationalKind::Resize;
        if (resize.getMode() != "sext" && resize.getMode() != "zext" &&
            resize.getMode() != "trunc")
          return resize.emitOpError() << "unsupported RTL resize";
        plan.opcode = resize.getMode().str();
        auto width = context.staticRtlValue(
            cast<ac::BitsType>(resize.getResult().getType()).getWidth(),
            resize);
        if (failed(width))
          return failure();
        plan.staticValue = std::move(*width);
      } else if (auto create = dyn_cast<ac::EnumCreateOp>(operation)) {
        plan.kind = VerilogCombinationalKind::EnumCreate;
        auto enumDefinition =
            context.analysis.resolveEnum(create.getResult().getType(), create);
        if (failed(enumDefinition))
          return failure();
        for (const auto &member : enumDefinition->members) {
          if (member.name != create.getMemberAttr())
            continue;
          plan.staticValue = std::to_string(enumDefinition->width) + "'d" +
                             member.code.getCanonicalValue().str();
          break;
        }
        if (plan.staticValue.empty())
          return create.emitOpError()
                 << "unresolved enum member during RTL emission";
      } else if (isa<ac::EnumToBitsOp>(operation)) {
        plan.kind = VerilogCombinationalKind::EnumToBits;
      } else if (auto fromBits = dyn_cast<ac::EnumFromBitsOp>(operation)) {
        plan.kind = VerilogCombinationalKind::EnumFromBits;
        auto enumDefinition = context.analysis.resolveEnum(
            fromBits.getValue().getType(), fromBits);
        if (failed(enumDefinition))
          return failure();
        for (const auto &member : enumDefinition->members)
          plan.staticValues.push_back(std::to_string(enumDefinition->width) +
                                      "'d" +
                                      member.code.getCanonicalValue().str());
      } else if (isa<ac::StructCreateOp>(operation)) {
        plan.kind = VerilogCombinationalKind::StructCreate;
      } else if (auto get = dyn_cast<ac::StructGetOp>(operation)) {
        plan.kind = VerilogCombinationalKind::StructGet;
        auto field = identifier(get.getField(), get);
        if (failed(field))
          return failure();
        plan.staticValue = std::move(*field);
      }
      modulePlan.combinational.try_emplace(&operation, std::move(plan));
    }
    return success();
  };
  auto planned = definition.walk([&](Operation *operation) {
    if (failed(buildCombinational(*operation)))
      return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (planned.wasInterrupted())
    return failure();

  auto buildTable = [&](Operation &operation) -> LogicalResult {
    if (!isa<ac::TableCreateOp, ac::TableSplatOp, ac::TableMapOp,
             ac::TableViewOp, ac::TableIndexOp, ac::TableGetOp,
             ac::TableMatchOp, ac::TableChooseOp, ac::TableFoldOp,
             ac::ValueMergeOp>(operation))
      return success();
    VerilogTablePlan plan;
    plan.operation = &operation;
    for (Value result : operation.getResults()) {
      auto type = context.rtlType(result.getType(), &operation);
      if (failed(type))
        return failure();
      plan.resultTypes.push_back(std::move(*type));
    }
    auto tableGeometry = [&](ac::TableType type) -> LogicalResult {
      auto count = context.shapeSize(type.getShape(), &operation, true);
      auto width =
          context.payloadWidth(type.getElementType(), &operation, true);
      if (failed(count) || failed(width))
        return failure();
      plan.elementCount = std::move(*count);
      plan.elementWidth = std::move(*width);
      return success();
    };
    if (auto create = dyn_cast<ac::TableCreateOp>(operation)) {
      plan.kind = VerilogTableKind::Create;
      if (failed(
              tableGeometry(cast<ac::TableType>(create.getResult().getType()))))
        return failure();
    } else if (auto splat = dyn_cast<ac::TableSplatOp>(operation)) {
      plan.kind = VerilogTableKind::Splat;
      auto count = context.shapeSize(splat.getShape(), splat, true);
      auto width =
          context.payloadWidth(splat.getInput().getType(), splat, true);
      if (failed(count) || failed(width))
        return failure();
      plan.elementCount = std::move(*count);
      plan.elementWidth = std::move(*width);
    } else if (auto view = dyn_cast<ac::TableViewOp>(operation)) {
      plan.kind = VerilogTableKind::View;
      if (failed(
              tableGeometry(cast<ac::TableType>(view.getResult().getType()))))
        return failure();
      auto sourceType = cast<ac::TableType>(view.getInput().getType());
      auto sourceCount = context.shapeSize(sourceType.getShape(), view, true);
      auto sourceWidth =
          context.payloadWidth(sourceType.getElementType(), view, true);
      auto mappedIndex = context.viewIndex(view, "__pyc_index__", true);
      if (failed(sourceCount) || failed(sourceWidth) || failed(mappedIndex))
        return failure();
      plan.sourceElementCount = std::move(*sourceCount);
      plan.sourceElementWidth = std::move(*sourceWidth);
      plan.mappedIndex = std::move(*mappedIndex);
    } else if (auto index = dyn_cast<ac::TableIndexOp>(operation)) {
      plan.kind = VerilogTableKind::Index;
      auto count = context.shapeSize(index.getShape(), index, true);
      auto targetWidth =
          context.payloadWidth(index.getResult().getType(), index, true);
      if (failed(count) || failed(targetWidth))
        return failure();
      plan.elementCount = std::move(*count);
      plan.targetWidth = std::move(*targetWidth);
      for (auto raw : index.getShape()) {
        auto dimension =
            context.staticRtlValue(cast<ac::StaticExprAttr>(raw), index);
        if (failed(dimension))
          return failure();
        plan.shape.push_back(std::move(*dimension));
      }
      for (Value coordinate : index.getCoords()) {
        auto width = context.payloadWidth(coordinate.getType(), index, true);
        if (failed(width))
          return failure();
        plan.operandWidths.push_back(std::move(*width));
      }
    } else if (auto get = dyn_cast<ac::TableGetOp>(operation)) {
      plan.kind = VerilogTableKind::Get;
      if (failed(tableGeometry(cast<ac::TableType>(get.getInput().getType()))))
        return failure();
      auto width = context.payloadWidth(get.getIndex().getType(), get, true);
      if (failed(width))
        return failure();
      plan.operandWidths.push_back(std::move(*width));
    } else if (auto map = dyn_cast<ac::TableMapOp>(operation)) {
      plan.kind = VerilogTableKind::Map;
      auto count = context.shapeSize(map.getShape(), map, true);
      if (failed(count))
        return failure();
      plan.elementCount = std::move(*count);
      Block &block = map.getRegion().front();
      unsigned argument = 0;
      auto ordinalWidth = context.payloadWidth(
          block.getArgument(argument).getType(), map, true);
      if (failed(ordinalWidth))
        return failure();
      plan.regionArguments.push_back({block.getArgument(argument++),
                                      VerilogRegionArgumentKind::Ordinal,
                                      0,
                                      std::move(*ordinalWidth),
                                      {},
                                      {}});
      for (auto [operand, table] : llvm::enumerate(map.getTables())) {
        auto tableType = cast<ac::TableType>(table.getType());
        auto type = context.rtlType(tableType.getElementType(), map);
        auto elements = context.shapeSize(tableType.getShape(), map, true);
        auto width =
            context.payloadWidth(tableType.getElementType(), map, true);
        if (failed(type) || failed(elements) || failed(width))
          return failure();
        plan.regionArguments.push_back({block.getArgument(argument++),
                                        VerilogRegionArgumentKind::TableElement,
                                        static_cast<unsigned>(operand),
                                        std::move(*type), std::move(*elements),
                                        std::move(*width)});
      }
      for (unsigned operand = map.getTables().size();
           operand < map->getNumOperands(); ++operand)
        plan.regionArguments.push_back({block.getArgument(argument++),
                                        VerilogRegionArgumentKind::Capture,
                                        operand,
                                        {},
                                        {},
                                        {}});
      for (auto [result, value] :
           llvm::enumerate(cast<ac::YieldOp>(block.back()).getValues())) {
        auto tableType = cast<ac::TableType>(map->getResult(result).getType());
        auto elements = context.shapeSize(tableType.getShape(), map, true);
        auto width =
            context.payloadWidth(tableType.getElementType(), map, true);
        if (failed(elements) || failed(width))
          return failure();
        plan.regionYields.push_back({value, static_cast<unsigned>(result), true,
                                     std::move(*elements), std::move(*width)});
      }
    } else if (auto match = dyn_cast<ac::TableMatchOp>(operation)) {
      plan.kind = VerilogTableKind::Match;
      if (failed(tableGeometry(
              cast<ac::TableType>(match->getOperand(0).getType()))))
        return failure();
      Block &block = match.getRegion().front();
      auto tableType = cast<ac::TableType>(match->getOperand(0).getType());
      auto type = context.rtlType(tableType.getElementType(), match);
      if (failed(type))
        return failure();
      plan.regionArguments.push_back(
          {block.getArgument(0), VerilogRegionArgumentKind::TableElement, 0,
           std::move(*type), plan.elementCount, plan.elementWidth});
      for (unsigned operand = 1; operand < match->getNumOperands(); ++operand)
        plan.regionArguments.push_back({block.getArgument(operand),
                                        VerilogRegionArgumentKind::Capture,
                                        operand,
                                        {},
                                        {},
                                        {}});
      for (auto [result, value] :
           llvm::enumerate(cast<ac::YieldOp>(block.back()).getValues()))
        plan.regionYields.push_back(
            {value, static_cast<unsigned>(result), false, {}, {}});
    } else if (auto choose = dyn_cast<ac::TableChooseOp>(operation)) {
      plan.kind = VerilogTableKind::Choose;
      if (failed(
              tableGeometry(cast<ac::TableType>(choose.getInput().getType()))))
        return failure();
      auto width = context.payloadWidth(choose.getResults().front().getType(),
                                        choose, true);
      if (failed(width))
        return failure();
      plan.targetWidth = std::move(*width);
      plan.mode = choose.getOrder().str();
      plan.count = choose.getCount();
    } else if (auto fold = dyn_cast<ac::TableFoldOp>(operation)) {
      plan.kind = VerilogTableKind::Fold;
      if (failed(tableGeometry(cast<ac::TableType>(fold.getInput().getType()))))
        return failure();
      plan.mode = fold.getKind().str();
      if (plan.mode == "min" || plan.mode == "max") {
        plan.conditionalFold = true;
        plan.opcode = plan.mode == "min" ? "<=" : ">=";
      } else {
        plan.opcode = llvm::StringSwitch<StringRef>(plan.mode)
                          .Case("add", "+")
                          .Case("mul", "*")
                          .Case("and", "&")
                          .Case("or", "|")
                          .Case("xor", "^")
                          .Default("")
                          .str();
        if (plan.opcode.empty())
          return fold.emitOpError() << "unsupported RTL table fold";
      }
    } else if (auto merge = dyn_cast<ac::ValueMergeOp>(operation)) {
      plan.kind = VerilogTableKind::ValueMerge;
      auto suffix = [&](ArrayRef<StringAttr> path) -> FailureOr<std::string> {
        std::string text;
        for (auto item : path) {
          auto field = identifier(item.getValue(), merge);
          if (failed(field))
            return failure();
          text += "." + *field;
        }
        return text;
      };
      ac::HardwareBindings bindings;
      bindings.owner = definition;
      auto leaves = context.analysis.getFieldPaths(merge.getBase().getType(),
                                                   bindings, merge);
      if (failed(leaves))
        return failure();
      SmallVector<ac::FieldPath> packedPaths;
      for (auto path : *leaves) {
        Type type = merge.getBase().getType();
        for (size_t index = 0; index < path.size(); ++index) {
          auto resolved = context.analysis.resolveType(type, bindings, merge);
          if (failed(resolved))
            return failure();
          if (isa<ac::TableType>(*resolved)) {
            path.resize(index);
            break;
          }
          auto record = dyn_cast<ac::StructType>(*resolved);
          auto declaration =
              record ? context.analysis.lookupStruct(record) : ac::StructOp();
          if (!declaration)
            return merge.emitOpError() << "RTL merge has unresolved field path";
          auto fields = declaration.getFields();
          auto found = llvm::find_if(fields, [&](Attribute raw) {
            return cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
                   path[index];
          });
          if (found == fields.end())
            return merge.emitOpError() << "RTL merge has unknown field path";
          type =
              cast<DictionaryAttr>(*found).getAs<TypeAttr>("type").getValue();
        }
        if (!llvm::is_contained(packedPaths, path))
          packedPaths.push_back(std::move(path));
      }
      for (auto [index, raw] : llvm::enumerate(merge.getPaths())) {
        ac::FieldPath path;
        for (auto item : cast<ArrayAttr>(raw))
          path.push_back(cast<StringAttr>(item));
        auto fieldSuffix = suffix(path);
        if (failed(fieldSuffix))
          return failure();
        auto guard = static_cast<unsigned>(1 + index);
        plan.mergeWrites.push_back(
            {std::move(*fieldSuffix), guard,
             static_cast<unsigned>(1 + merge.getGuards().size() + index)});
        plan.mergeEnableOperands.push_back(guard);
      }
      for (auto &path : packedPaths) {
        bool written = llvm::any_of(merge.getPaths(), [&](Attribute raw) {
          auto mergePath = cast<ArrayAttr>(raw);
          return mergePath.size() <= path.size() &&
                 std::equal(mergePath.begin(), mergePath.end(), path.begin());
        });
        if (written)
          continue;
        auto fieldSuffix = suffix(path);
        if (failed(fieldSuffix))
          return failure();
        plan.mergePassthroughs.push_back(std::move(*fieldSuffix));
      }
    }
    modulePlan.tables.try_emplace(&operation, std::move(plan));
    return success();
  };
  auto tablesPlanned = definition.walk([&](Operation *operation) {
    if (failed(buildTable(*operation)))
      return WalkResult::interrupt();
    return WalkResult::advance();
  });
  if (tablesPlanned.wasInterrupted())
    return failure();

  for (Operation &operation : definition.getBody().front()) {
    if (auto queue = dyn_cast<ac::QueueOp>(operation)) {
      VerilogQueuePlan plan;
      plan.occurrence = queue;
      plan.managedParameter = context.managesChecks();
      plan.dataPorts = {
          {"clk", VerilogValueSource::Operand, 0},
          {"rst", VerilogValueSource::Operand, 1},
          {"in_valid", VerilogValueSource::Operand, 2},
          {"in_data", VerilogValueSource::Operand, 3},
          {"out_ready", VerilogValueSource::Operand, 4},
          {"in_ready", VerilogValueSource::Result, 0},
          {"out_valid", VerilogValueSource::Result, 1},
          {"out_data", VerilogValueSource::Result, 2},
      };
      if (context.managesChecks())
        plan.managedPorts = {
            {"pyc_phase", VerilogManagedSignal::Phase},
            {"pyc_root_commit_ok", VerilogManagedSignal::RootCommitOk},
            {"pyc_local_error", VerilogManagedSignal::LocalError},
        };
      if (context.managesChecks())
        plan.lifecycleWires = {
            {VerilogManagedSignal::LocalError, "", false,
             VerilogLifecycleAggregate::Error,
             VerilogLifecycleReduction::Direct},
        };
      auto name = identifier(queue.getInstanceName(), queue);
      auto payload = context.rtlType(queue.getInData().getType(), queue);
      auto depth = context.staticRtlValue(queue.getDepth(), queue);
      auto latency = context.unsignedStaticValue(queue.getAvailabilityLatency(),
                                                 queue, true);
      if (failed(name) || failed(payload) || failed(depth) || failed(latency))
        return failure();
      plan.instanceName = std::move(*name);
      plan.payloadType = std::move(*payload);
      plan.depth = std::move(*depth);
      plan.availabilityLatency = std::move(*latency);
      if (queue.getReadyPolicy() == "local_occupancy")
        plan.readyPolicy = 0;
      else if (queue.getReadyPolicy() == "downstream_pop")
        plan.readyPolicy = 1;
      else
        return queue.emitOpError() << "unsupported queue ready policy";
      modulePlan.queues.try_emplace(queue, std::move(plan));
      continue;
    }
    if (!isa<ac::InstanceOp, ac::CollectionOp>(operation))
      continue;

    auto *occurrence = &operation;
    const bool collection = isa<ac::CollectionOp>(operation);
    auto *callee =
        collection
            ? context.analysis.resolveCallee(cast<ac::CollectionOp>(operation))
            : context.analysis.resolveCallee(cast<ac::InstanceOp>(operation));
    auto bindings =
        collection
            ? context.analysis.bindInstance(cast<ac::CollectionOp>(operation))
            : context.analysis.bindInstance(cast<ac::InstanceOp>(operation));
    if (failed(bindings))
      return failure();

    auto kind = context.analysis.getPrimitiveKind(callee);
    VerilogOccurrencePlan plan;
    plan.occurrence = occurrence;
    plan.collection = collection;
    plan.primitive = !kind.empty();
    plan.managedPrimitive =
        context.managesChecks() && plan.primitive &&
        llvm::any_of(context.sourceChecks().commits,
                     [&](const ac::HardwareCheckCommitEndpoint &endpoint) {
                       return endpoint.allocation == occurrence;
                     });
    plan.moduleName = plan.primitive ? kind.str() : context.names(callee).rtl;
    if (context.managesChecks()) {
      plan.managedPorts = {
          {"pyc_phase", VerilogManagedSignal::Phase},
          {"pyc_root_commit_ok", VerilogManagedSignal::RootCommitOk},
      };
      if (!plan.primitive) {
        plan.managedPorts.push_back(
            {"pyc_reset_active", VerilogManagedSignal::ResetActive});
        plan.managedPorts.push_back({"pyc_check_complete",
                                     VerilogManagedSignal::CheckComplete,
                                     collection});
        plan.managedPorts.push_back({"pyc_sampling_error",
                                     VerilogManagedSignal::SamplingError,
                                     collection});
      }
      plan.managedPorts.push_back(
          {"pyc_local_error", VerilogManagedSignal::LocalError, collection});
      plan.lifecycleWires.push_back(
          {VerilogManagedSignal::LocalError, "", collection,
           VerilogLifecycleAggregate::Error,
           collection ? VerilogLifecycleReduction::Any
                      : VerilogLifecycleReduction::Direct});
      if (!plan.primitive) {
        plan.lifecycleWires.push_back(
            {VerilogManagedSignal::CheckComplete, "_complete", collection,
             VerilogLifecycleAggregate::Completion,
             collection ? VerilogLifecycleReduction::All
                        : VerilogLifecycleReduction::Direct});
        plan.lifecycleWires.push_back(
            {VerilogManagedSignal::SamplingError, "_sampling", collection,
             VerilogLifecycleAggregate::SamplingError,
             collection ? VerilogLifecycleReduction::Any
                        : VerilogLifecycleReduction::Direct});
      }
    }

    auto name = identifier(
        occurrence->getAttrOfType<StringAttr>("instance_name").getValue(),
        occurrence);
    auto parameters =
        collection
            ? buildParameters(context, cast<ac::CollectionOp>(operation),
                              callee, plan.primitive, plan.managedPrimitive)
            : buildParameters(context, cast<ac::InstanceOp>(operation), callee,
                              plan.primitive, plan.managedPrimitive);
    if (failed(name) || failed(parameters))
      return failure();
    plan.instanceName = std::move(*name);
    plan.parameters = std::move(*parameters);

    if (collection) {
      auto count = context.shapeSize(
          cast<ac::CollectionOp>(operation).getShape(), occurrence, true);
      if (failed(count))
        return failure();
      plan.collectionCount = std::move(*count);
    }

    auto calleeSignature = cast<FunctionType>(
        callee->getAttrOfType<TypeAttr>("function_type").getValue());
    auto appendPorts = [&](ArrayAttr rawNames, TypeRange types,
                           VerilogValueSource source) -> LogicalResult {
      for (auto [ordinal, pair] : llvm::enumerate(llvm::zip(rawNames, types))) {
        auto [raw, type] = pair;
        auto portName =
            identifier(cast<StringAttr>(raw).getValue(), occurrence);
        if (failed(portName))
          return failure();
        VerilogDataPortPlan port{
            std::move(*portName), source, static_cast<unsigned>(ordinal), {}};
        if (collection) {
          auto width = context.payloadWidth(type, occurrence, true, *bindings);
          if (failed(width))
            return failure();
          port.collectionWidth = std::move(*width);
        }
        plan.dataPorts.push_back(std::move(port));
      }
      return success();
    };
    if (failed(appendPorts(callee->getAttrOfType<ArrayAttr>("input_names"),
                           calleeSignature.getInputs(),
                           VerilogValueSource::Operand)) ||
        failed(appendPorts(callee->getAttrOfType<ArrayAttr>("output_names"),
                           calleeSignature.getResults(),
                           VerilogValueSource::Result)))
      return failure();

    modulePlan.occurrences.try_emplace(occurrence, std::move(plan));
  }
  return modulePlan;
}

FailureOr<VerilogWrapperEmissionPlan>
buildVerilogWrapperEmissionPlan(HardwareEmitContext &context, ac::ModuleOp root,
                                StringRef callee) {
  VerilogWrapperEmissionPlan plan;
  auto bindings = context.rootBindings();
  auto layout = ac::getModulePortLayout(root);
  if (failed(bindings) || failed(layout))
    return failure();
  auto signature = root.getFunctionType();
  auto appendPublicPorts = [&](ArrayAttr rawNames, TypeRange types,
                               VerilogPortDirection direction,
                               unsigned count) -> LogicalResult {
    for (auto [raw, type] : llvm::zip(rawNames.getValue().take_front(count),
                                      types.take_front(count))) {
      auto name = identifier(cast<StringAttr>(raw).getValue(), root);
      auto resolved = context.analysis.resolveType(type, *bindings, root);
      if (failed(name) || failed(resolved))
        return failure();
      auto rendered = context.rtlType(*resolved, root);
      if (failed(rendered))
        return failure();
      plan.ports.push_back({direction, std::move(*rendered), std::move(*name)});
    }
    return success();
  };
  if (failed(appendPublicPorts(root.getInputNames(), signature.getInputs(),
                               VerilogPortDirection::Input,
                               layout->sourceInputCount)) ||
      failed(appendPublicPorts(root.getOutputNames(), signature.getResults(),
                               VerilogPortDirection::Output,
                               layout->sourceOutputCount)))
    return failure();
  if (context.managesChecks()) {
    plan.ports.append({
        {VerilogPortDirection::Input, "[2:0]", "pyc_phase"},
        {VerilogPortDirection::Input, {}, "pyc_root_commit_ok"},
        {VerilogPortDirection::Output, {}, "pyc_check_complete"},
        {VerilogPortDirection::Output, {}, "pyc_sampling_error"},
        {VerilogPortDirection::Output, {}, "pyc_local_error"},
    });
  }

  for (auto [index, raw] : llvm::enumerate(root.getInputNames())) {
    auto name = identifier(cast<StringAttr>(raw).getValue(), root);
    if (failed(name))
      return failure();
    std::string expression;
    if (index < layout->sourceInputCount) {
      expression = *name;
    } else {
      auto width = context.analysis.getPackedWidth(signature.getInput(index),
                                                   *bindings, root);
      if (failed(width))
        return failure();
      expression = std::to_string(*width) + "'d0";
    }
    plan.connections.push_back({std::move(*name), std::move(expression)});
  }
  for (auto [index, raw] : llvm::enumerate(root.getOutputNames())) {
    auto name = identifier(cast<StringAttr>(raw).getValue(), root);
    if (failed(name))
      return failure();
    std::string expression =
        index < layout->sourceOutputCount ? *name : std::string();
    plan.connections.push_back({std::move(*name), std::move(expression)});
  }
  if (context.managesChecks()) {
    std::string reset = "1'b0";
    auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    if (domain && !domain.empty()) {
      auto resetIndex = domain.getAs<IntegerAttr>("reset");
      auto sourceNames =
          root.getInputNames().getValue().take_front(layout->sourceInputCount);
      if (!resetIndex || resetIndex.getValue().getActiveBits() > 64 ||
          resetIndex.getValue().getZExtValue() >= sourceNames.size())
        return root.emitOpError()
               << "managed module requires verified reset context";
      auto name = identifier(
          cast<StringAttr>(sourceNames[resetIndex.getValue().getZExtValue()])
              .getValue(),
          root);
      if (failed(name))
        return failure();
      reset = std::move(*name);
    }
    plan.connections.append({
        {"pyc_phase", "pyc_phase"},
        {"pyc_root_commit_ok", "pyc_root_commit_ok"},
        {"pyc_reset_active", std::move(reset)},
        {"pyc_check_complete", "pyc_check_complete"},
        {"pyc_sampling_error", "pyc_sampling_error"},
        {"pyc_local_error", "pyc_local_error"},
    });
  }
  auto parameters = callee == context.names(root).rtl
                        ? context.rootRtlParameters()
                        : FailureOr<std::string>(std::string());
  if (failed(parameters))
    return failure();
  plan.calleeParameters = std::move(*parameters);
  return plan;
}

} // namespace acir::compiler

#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSwitch.h"
using namespace mlir;
namespace acir::compiler {
namespace {
class NetEmitter {
public:
  NetEmitter(HardwareEmitContext &context, ac::ModuleOp definition,
             raw_ostream &out)
      : context(context), definition(definition), out(out) {}
  LogicalResult emitSourceChecks() {
    if (!context.isSystem())
      return success();
    std::string reset = "pyc_reset_active";
    if (auto domain =
            definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs")) {
      if (auto ordinal = domain.getAs<IntegerAttr>("reset")) {
        if (ordinal.getValue().getActiveBits() > 64 ||
            ordinal.getValue().getZExtValue() >=
                definition.getBody().front().getNumArguments())
          return definition.emitOpError()
                 << "RTL source-check reset input is outside the module";
        auto localReset = net(definition.getBody().front().getArgument(
            ordinal.getValue().getZExtValue()));
        if (failed(localReset))
          return failure();
        reset = std::move(*localReset);
      }
    }
    for (const auto &binding : context.sourceChecks().bindings) {
      if (binding.definition != definition)
        continue;
      auto condition = net(binding.condition);
      auto path = net(binding.path);
      if (failed(condition) || failed(path))
        return failure();
      auto name = "pyc_check_error_" + std::to_string(localErrors.size());
      out << "  wire " << name << " = ((" << *path << " & ~" << reset << " & ~"
          << *condition << ") !== 1'b0);\n";
      localErrors.push_back(std::move(name));
    }
    return success();
  }
  LogicalResult emitObservations() {
    if (!context.isSystem())
      return success();
    auto plan = observationEmissionPlan(context);
    if (failed(plan))
      return failure();
    auto found = plan->definitions.find(definition);
    if (found == plan->definitions.end() || found->second.empty())
      return success();
    for (auto [index, site] : llvm::enumerate(found->second)) {
      auto observe = cast<ac::SourceObserveOp>(site.op);
      auto path = net(observe.getPath());
      Value observed = site.carriesValue ? observe.getValues()[site.valueIndex]
                                         : observe.getPath();
      auto value = net(observed);
      auto type = context.rtlType(observed.getType(), observe);
      if (failed(path) || failed(value) || failed(type))
        return failure();
      out << "  " << *type << " pyc_observation_value_" << index << ";\n"
          << "  logic pyc_observation_valid_" << index << ";\n"
          << "  logic pyc_observation_path_" << index << ";\n"
          << "  always @(pyc_phase) if (pyc_phase == 3'd1) begin\n"
          << "    pyc_observation_value_" << index << " = " << *value << ";\n"
          << "    pyc_observation_valid_" << index << " = !$isunknown("
          << *value << ") && !$isunknown(" << *path << ");\n"
          << "    pyc_observation_path_" << index << " = (" << *path
          << " === 1'b1);\n"
          << "  end\n";
    }
    return success();
  }
  FailureOr<std::string> net(Value value) {
    if (auto found = names.find(value); found != names.end())
      return found->second;
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *parent = argument.getOwner()->getParentOp();
      if (parent == definition) {
        auto port = cast<StringAttr>(
            definition.getInputNames()[argument.getArgNumber()]);
        auto id = legalizeIdentifier(port.getValue(),
                                     [&] { return definition.emitOpError(); });
        if (failed(id))
          return failure();
        names.try_emplace(value, *id);
        return *id;
      }
      if (auto rule = dyn_cast<ac::RuleOp>(parent))
        return net(rule.getCaptures()[argument.getArgNumber()]);
      return definition.emitOpError() << "unexpected RTL block argument";
    }
    auto *op = value.getDefiningOp();
    if (!op)
      return definition.emitOpError() << "RTL wire has no producer";
    if (isa<ac::InstanceOp, ac::CollectionOp, ac::QueueOp>(op)) {
      auto name = "pyc_net_" + std::to_string(next++);
      auto type = context.rtlType(value.getType(), op);
      if (failed(type))
        return failure();
      out << "  wire " << *type << " " << name << ";\n";
      names.try_emplace(value, name);
      instances.insert(op);
      return name;
    }
    if (auto rule = dyn_cast<ac::RuleOp>(op)) {
      auto yield = cast<ac::YieldOp>(rule.getBody().front().back());
      auto source =
          net(yield.getValues()[cast<OpResult>(value).getResultNumber()]);
      if (failed(source))
        return failure();
      names.try_emplace(value, *source);
      return *source;
    }
    if (isa<ac::TableCreateOp, ac::TableSplatOp, ac::TableMapOp,
            ac::TableViewOp, ac::TableIndexOp, ac::TableGetOp, ac::TableMatchOp,
            ac::TableChooseOp, ac::TableFoldOp, ac::ValueMergeOp>(op)) {
      if (failed(tableOperation(op)))
        return failure();
      return names.lookup(value);
    }
    // Register the complete net before following producers. A legal
    // field-level graph can refer to another field of this same aggregate.
    auto name = "pyc_net_" + std::to_string(next++);
    auto type = context.rtlType(value.getType(), op);
    if (failed(type))
      return failure();
    bool combinationalDivRem = false;
    if (auto binary = dyn_cast<ac::BitsBinaryOp>(op))
      combinationalDivRem =
          binary.getOpcode() == "udiv" || binary.getOpcode() == "urem";
    out << "  " << (combinationalDivRem ? "" : "wire ") << *type << " " << name
        << ";\n";
    names.try_emplace(value, name);
    SmallVector<std::string> operands;
    for (auto input : op->getOperands()) {
      auto item = net(input);
      if (failed(item))
        return failure();
      operands.push_back(*item);
    }
    std::string expression;
    if (auto create = dyn_cast<ac::EnumCreateOp>(op)) {
      auto definition =
          context.analysis.resolveEnum(create.getResult().getType(), op);
      if (failed(definition))
        return failure();
      for (const auto &member : definition->members)
        if (member.name == create.getMemberAttr())
          expression = std::to_string(definition->width) + "'d" +
                       member.code.getCanonicalValue().str();
      if (expression.empty())
        return op->emitOpError()
               << "unresolved enum member during RTL emission";
    } else if (isa<ac::EnumToBitsOp>(op))
      expression = operands[0];
    else if (auto fromBits = dyn_cast<ac::EnumFromBitsOp>(op)) {
      if (cast<OpResult>(value).getResultNumber() == 0)
        expression = operands[0];
      else {
        auto definition =
            context.analysis.resolveEnum(fromBits.getValue().getType(), op);
        if (failed(definition))
          return failure();
        for (const auto &member : definition->members) {
          auto equal = "(" + operands[0] +
                       " == " + std::to_string(definition->width) + "'d" +
                       member.code.getCanonicalValue().str() + ")";
          expression = expression.empty()
                           ? equal
                           : "(" + expression + " | " + equal + ")";
        }
      }
    } else if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
      auto literal = context.staticRtlValue(constant.getValue(), op);
      if (failed(literal))
        return failure();
      auto width = context.staticRtlValue(
          cast<ac::BitsType>(value.getType()).getWidth(), op);
      if (failed(width))
        return failure();
      if (context.isClosedStatic(constant.getValue())) {
        llvm::APSInt value(*literal);
        *literal = std::to_string(std::max(1u, value.getActiveBits())) + "'d" +
                   *literal;
      }
      expression = "(" + *width + ")'( " + *literal + " )";
    } else if (auto unary = dyn_cast<ac::BitsUnaryOp>(op)) {
      if (unary.getOpcode() != "not")
        return op->emitOpError() << "unsupported RTL unary op";
      expression = "(~" + operands[0] + ")";
    } else if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
      if (binary.getOpcode() == "udiv" || binary.getOpcode() == "urem") {
        const auto &lhs = operands[0];
        const auto &rhs = operands[1];
        expression = "($unsigned(" + lhs + ") " +
                     (binary.getOpcode() == "udiv" ? "/" : "%") +
                     " $unsigned(" + rhs + "))";
      } else {
        auto symbol = llvm::StringSwitch<StringRef>(binary.getOpcode())
                          .Case("and", "&")
                          .Case("or", "|")
                          .Case("xor", "^")
                          .Case("add", "+")
                          .Case("sub", "-")
                          .Case("mul", "*")
                          .Case("shl", "<<")
                          .Case("lshr", ">>")
                          .Case("ashr", ">>>")
                          .Default("");
        if (symbol.empty())
          return op->emitOpError() << "unsupported RTL binary op";
        expression =
            "(" +
            (binary.getOpcode() == "ashr" ? "$signed(" + operands[0] + ")"
                                          : operands[0]) +
            " " + symbol.str() + " " + operands[1] + ")";
      }
    } else if (auto compare = dyn_cast<ac::BitsCompareOp>(op)) {
      auto predicate = llvm::StringSwitch<StringRef>(compare.getPredicate())
                           .Case("eq", "==")
                           .Case("ne", "!=")
                           .Case("ult", "<")
                           .Case("ule", "<=")
                           .Case("ugt", ">")
                           .Case("uge", ">=")
                           .Case("slt", "<")
                           .Case("sle", "<=")
                           .Case("sgt", ">")
                           .Case("sge", ">=")
                           .Default("");
      if (predicate.empty())
        return op->emitOpError() << "unsupported RTL comparison";
      bool signedCompare = compare.getPredicate().starts_with("s");
      expression = std::string("(") + (signedCompare ? "$signed(" : "") +
                   operands[0] + (signedCompare ? ")" : "") + " " +
                   predicate.str() + " " + (signedCompare ? "$signed(" : "") +
                   operands[1] + (signedCompare ? ")" : "") + ")";
    } else if (isa<ac::BitsSelectOp>(op))
      expression =
          "(" + operands[0] + " ? " + operands[1] + " : " + operands[2] + ")";
    else if (isa<ac::BitsConcatOp>(op)) {
      expression = "{";
      for (auto [i, item] : llvm::enumerate(operands))
        expression += (i ? ", " : "") + item;
      expression += "}";
    } else if (auto extracted = dyn_cast<ac::BitsExtractOp>(op)) {
      auto low = context.staticRtlValue(extracted.getLow(), op);
      if (failed(low))
        return failure();
      auto type = context.rtlType(value.getType(), op);
      if (failed(type))
        return failure();
      auto width = context.staticRtlValue(
          cast<ac::BitsType>(value.getType()).getWidth(), op);
      if (failed(width))
        return failure();
      expression = operands[0] + "[(" + *low + ") +: " + *width + "]";
    } else if (auto resized = dyn_cast<ac::BitsResizeOp>(op)) {
      auto type = context.rtlType(value.getType(), op);
      if (failed(type))
        return failure();
      auto width = context.staticRtlValue(
          cast<ac::BitsType>(value.getType()).getWidth(), op);
      if (failed(width))
        return failure();
      if (resized.getMode() == "sext")
        expression = "(" + *width + ")'($signed(" + operands[0] + "))";
      else if (resized.getMode() == "zext")
        expression = "(" + *width + ")'($unsigned(" + operands[0] + "))";
      else if (resized.getMode() == "trunc")
        expression = "(" + *width + ")'(" + operands[0] + ")";
      else
        return op->emitOpError() << "unsupported RTL resize";
    } else if (isa<ac::StructCreateOp>(op)) {
      expression = "{";
      for (auto [i, item] : llvm::enumerate(operands))
        expression += (i ? ", " : "") + item;
      expression += "}";
    } else if (auto get = dyn_cast<ac::StructGetOp>(op)) {
      auto name =
          legalizeIdentifier(get.getField(), [&] { return op->emitOpError(); });
      if (failed(name))
        return failure();
      expression = operands[0] + "." + *name;
    } else
      return op->emitOpError() << "operation lacks RTL hardware emission";
    out << "  " << (combinationalDivRem ? "always_comb " : "assign ") << name
        << " = " << expression << ";\n";
    return name;
  }
  LogicalResult tableOperation(Operation *op) {
    for (Value result : op->getResults()) {
      auto type = context.rtlType(result.getType(), op);
      if (failed(type))
        return failure();
      auto name = "pyc_net_" + std::to_string(next++);
      out << "  wire " << *type << " " << name << ";\n";
      names.try_emplace(result, name);
    }
    SmallVector<std::string> operands;
    for (Value value : op->getOperands()) {
      auto source = net(value);
      if (failed(source))
        return failure();
      operands.push_back(*source);
    }
    auto resultName = [&](unsigned i = 0) {
      return names.lookup(op->getResult(i));
    };
    auto width = [&](Type type) {
      return context.payloadWidth(type, op, true);
    };
    auto count = [&](ac::TableType type) {
      return context.shapeSize(type.getShape(), op, true);
    };
    auto element = [&](StringRef value, StringRef ordinal,
                       ac::TableType type) -> FailureOr<std::string> {
      auto n = count(type), w = width(type.getElementType());
      if (failed(n) || failed(w))
        return failure();
      return value.str() + "[((" + *n + ")-1-(" + ordinal.str() + "))*(" + *w +
             ") +: (" + *w + ")]";
    };
    auto generate = [&](StringRef n, StringRef label) {
      std::string i = "pyc_index_" + std::to_string(next++);
      out << "  for (genvar " << i << "=0; " << i << " < (" << n << "); ++" << i
          << ") begin : " << label << "\n";
      return i;
    };
    if (auto create = dyn_cast<ac::TableCreateOp>(op)) {
      auto type = cast<ac::TableType>(create.getResult().getType());
      for (auto [i, input] : llvm::enumerate(operands)) {
        auto target = element(resultName(), std::to_string(i), type);
        if (failed(target))
          return failure();
        out << "  assign " << *target << " = " << input << ";\n";
      }
      return success();
    }
    if (auto splat = dyn_cast<ac::TableSplatOp>(op)) {
      auto n = context.shapeSize(splat.getShape(), op, true);
      auto w = width(splat.getInput().getType());
      if (failed(n) || failed(w))
        return failure();
      auto i = generate(*n, "pyc_broadcast_" + resultName());
      out << "    assign " << resultName() << "[((" << *n << ")-1-" << i
          << ")*(" << *w << ") +: (" << *w << ")] = " << operands[0]
          << ";\n  end\n";
      return success();
    }
    if (auto view = dyn_cast<ac::TableViewOp>(op)) {
      auto srcType = cast<ac::TableType>(view.getInput().getType());
      auto dstType = cast<ac::TableType>(view.getResult().getType());
      auto n = count(dstType);
      if (failed(n))
        return failure();
      auto i = generate(*n, "pyc_view_" + resultName());
      auto mapped = context.viewIndex(view, i, true);
      if (failed(mapped))
        return failure();
      auto src = element(operands[0], *mapped, srcType);
      auto dst = element(resultName(), i, dstType);
      if (failed(src) || failed(dst))
        return failure();
      out << "    assign " << *dst << " = " << *src << ";\n  end\n";
      return success();
    }
    if (auto index = dyn_cast<ac::TableIndexOp>(op)) {
      auto n = context.shapeSize(index.getShape(), op, true);
      auto targetWidth = width(index.getResult().getType());
      if (failed(n) || failed(targetWidth))
        return failure();
      SmallVector<std::string> shape, widths;
      std::string sumWidth = "64";
      for (auto raw : index.getShape()) {
        auto dim = context.staticRtlValue(cast<ac::StaticExprAttr>(raw), op);
        if (failed(dim))
          return failure();
        shape.push_back(*dim);
      }
      for (auto value : index.getCoords()) {
        auto bits = width(value.getType());
        if (failed(bits))
          return failure();
        widths.push_back(*bits);
        sumWidth += " + (" + *bits + ")";
      }
      std::string inBounds = "1'b1", sum = "(" + sumWidth + ")'(0)";
      std::string stride = "1";
      for (size_t axis = shape.size(); axis-- > 0;) {
        auto boundWidth = "$clog2(64'(" + shape[axis] + ") + 64'd1)";
        auto compareWidth = "((" + widths[axis] + ") > (" + boundWidth +
                            ") ? (" + widths[axis] + ") : (" + boundWidth +
                            "))";
        inBounds += " & ((" + compareWidth + ")'($unsigned(" + operands[axis] +
                    ")) < (" + compareWidth + ")'(" + shape[axis] + "))";
        sum += " + (" + sumWidth + ")'($unsigned(" + operands[axis] + ")) * (" +
               sumWidth + ")'(" + stride + ")";
        stride = "(" + stride + " * (" + shape[axis] + "))";
      }
      out << "  assign " << resultName() << " = (" << inBounds << ") ? ("
          << *targetWidth << ")'(" << sum << ") : (" << *targetWidth << ")'("
          << *n << ");\n";
      return success();
    }
    if (auto get = dyn_cast<ac::TableGetOp>(op)) {
      auto type = cast<ac::TableType>(get.getInput().getType());
      auto n = count(type), k = width(get.getIndex().getType());
      if (failed(n) || failed(k))
        return failure();
      auto b = "$clog2(64'(" + *n + ") + 64'd1)";
      auto w = "((" + *k + ") > (" + b + ") ? (" + *k + ") : (" + b + "))";
      auto selected =
          element(operands[0], "$unsigned(" + operands[1] + ")", type);
      if (failed(selected))
        return failure();
      out << "  assign " << resultName(1) << " = (" << w << ")'($unsigned("
          << operands[1] << ")) < (" << w << ")'(" << *n << ");\n"
          << "  assign " << resultName() << " = " << resultName(1) << " ? "
          << *selected << " : 'x;\n";
      return success();
    }
    if (isa<ac::TableMapOp, ac::TableMatchOp>(op)) {
      bool isMap = isa<ac::TableMapOp>(op);
      SmallVector<Value> tables;
      if (isMap)
        llvm::append_range(tables, cast<ac::TableMapOp>(op).getTables());
      else
        tables.push_back(op->getOperand(0));
      auto shape =
          isMap ? cast<ac::TableMapOp>(op).getShape()
                : cast<ac::TableType>(op->getOperand(0).getType()).getShape();
      auto n = context.shapeSize(shape, op, true);
      if (failed(n))
        return failure();
      auto i = generate(*n, "pyc_map_" + resultName());
      Block &block = op->getRegion(0).front();
      unsigned arg = 0;
      if (isMap) {
        auto w = width(block.getArgument(0).getType());
        if (failed(w))
          return failure();
        names[block.getArgument(arg++)] = "(" + *w + ")'(" + i + ")";
      }
      for (auto [j, value] : llvm::enumerate(tables)) {
        auto type = cast<ac::TableType>(value.getType());
        auto selected = element(operands[j], i, type);
        auto dataType = context.rtlType(type.getElementType(), op);
        if (failed(selected) || failed(dataType))
          return failure();
        auto local = "pyc_lane_" + std::to_string(next++);
        out << "    wire " << *dataType << " " << local << " = " << *selected
            << ";\n";
        names[block.getArgument(arg++)] = local;
      }
      for (size_t j = tables.size(); j < operands.size(); ++j)
        names[block.getArgument(arg++)] = operands[j];
      auto yielded = cast<ac::YieldOp>(block.back()).getValues();
      for (auto [j, value] : llvm::enumerate(yielded)) {
        auto source = net(value);
        if (failed(source))
          return failure();
        if (isMap) {
          auto dst = element(resultName(j), i,
                             cast<ac::TableType>(op->getResult(j).getType()));
          if (failed(dst))
            return failure();
          out << "    assign " << *dst << " = " << *source << ";\n";
        } else
          out << "    assign " << resultName() << "[" << i << "] = " << *source
              << ";\n";
      }
      out << "  end\n";
      return success();
    }
    if (auto choose = dyn_cast<ac::TableChooseOp>(op)) {
      auto type = cast<ac::TableType>(choose.getInput().getType());
      auto n = count(type), w = width(choose.getResults().front().getType());
      if (failed(n) || failed(w))
        return failure();
      auto c = choose.getCount();
      auto prefix = "pyc_choice_" + resultName();
      out << "  wire [(" << *n << ")-1:0] " << prefix << "_mask [0:" << c
          << "];\n"
          << "  wire [(" << *w << ")-1:0] " << prefix << "_index [0:" << c - 1
          << "][0:(" << *n << ")];\n"
          << "  wire " << prefix << "_valid [0:" << c - 1 << "];\n"
          << "  assign " << prefix << "_mask[0] = " << operands[1] << ";\n";
      auto pass = generate(std::to_string(c), prefix + "_passes");
      out << "    assign " << prefix << "_index[" << pass << "][0] = '0;\n"
          << "    assign " << prefix << "_valid[" << pass << "] = |" << prefix
          << "_mask[" << pass << "];\n";
      auto step = generate(*n, prefix + "_scan");
      auto k =
          choose.getOrder() == "low" ? "((" + *n + ")-1-" + step + ")" : step;
      out << "    assign " << prefix << "_index[" << pass << "][" << step
          << "+1] = " << prefix << "_mask[" << pass << "][" << k << "] ? ("
          << *w << ")'(" << k << ") : " << prefix << "_index[" << pass << "]["
          << step << "];\n"
          << "    assign " << prefix << "_mask[" << pass << "+1][" << step
          << "] = " << prefix << "_mask[" << pass << "][" << step << "] & ~("
          << prefix << "_valid[" << pass << "] & (" << prefix << "_index["
          << pass << "][" << *n << "] == (" << *w << ")'(" << step << ")));\n"
          << "  end\n  end\n";
      for (int64_t j = 0; j < c; ++j)
        out << "  assign " << resultName(j) << " = " << prefix << "_index[" << j
            << "][" << *n << "];\n"
            << "  assign " << resultName(c + j) << " = " << prefix << "_valid["
            << j << "];\n";
      return success();
    }
    if (auto fold = dyn_cast<ac::TableFoldOp>(op)) {
      auto type = cast<ac::TableType>(fold.getInput().getType());
      auto n = count(type), w = width(type.getElementType());
      if (failed(n) || failed(w))
        return failure();
      auto prefix = "pyc_fold_" + resultName();
      out << "  wire [(" << *n << ")*(" << *w << ")-1:0] " << prefix
          << " [0:$clog2(" << *n << ")];\n"
          << "  assign " << prefix << "[0] = " << operands[0] << ";\n";
      auto level = generate("$clog2(" + *n + ")", prefix + "_levels");
      auto previousCount = "(((" + *n + ") + ((64'd1 << " + level +
                           ")-1)) / (64'd1 << " + level + "))";
      auto lane = generate("((" + previousCount + "+1)/2)", prefix + "_pairs");
      auto a = prefix + "[" + level + "][((" + *n + ")-1-2*" + lane + ")*(" +
               *w + ") +: (" + *w + ")]";
      auto b = prefix + "[" + level + "][((" + *n + ")-2-2*" + lane + ")*(" +
               *w + ") +: (" + *w + ")]";
      auto dst = prefix + "[" + level + "+1][((" + *n + ")-1-" + lane + ")*(" +
                 *w + ") +: (" + *w + ")]";
      std::string expression;
      if (fold.getKind() == "min" || fold.getKind() == "max")
        expression = "(" + a + (fold.getKind() == "min" ? " <= " : " >= ") + b +
                     " ? " + a + " : " + b + ")";
      else {
        auto symbol = llvm::StringSwitch<StringRef>(fold.getKind())
                          .Case("add", "+")
                          .Case("mul", "*")
                          .Case("and", "&")
                          .Case("or", "|")
                          .Case("xor", "^")
                          .Default("");
        expression = "(" + a + " " + symbol.str() + " " + b + ")";
      }
      out << "    if (2*" << lane << "+1 < " << previousCount << ") begin\n"
          << "      assign " << dst << " = " << expression << ";\n"
          << "    end else begin\n      assign " << dst << " = " << a
          << ";\n    end\n  end\n  end\n"
          << "  assign " << resultName() << " = " << prefix << "[$clog2(" << *n
          << ")][((" << *n << ")-1)*(" << *w << ") +: (" << *w << ")];\n";
      return success();
    }
    if (auto merge = dyn_cast<ac::ValueMergeOp>(op)) {
      auto field = [&](StringRef base,
                       ArrayRef<StringAttr> path) -> FailureOr<std::string> {
        std::string text = base.str();
        for (auto item : path) {
          auto id = legalizeIdentifier(item.getValue(),
                                       [&] { return op->emitOpError(); });
          if (failed(id))
            return failure();
          text += "." + *id;
        }
        return text;
      };
      ac::HardwareBindings bindings;
      bindings.owner = definition;
      auto leaves = context.analysis.getFieldPaths(merge.getBase().getType(),
                                                   bindings, op);
      if (failed(leaves))
        return failure();
      std::string enables = "1'b0";
      for (auto [j, raw] : llvm::enumerate(merge.getPaths())) {
        auto attr = cast<ArrayAttr>(raw);
        ac::FieldPath path;
        for (auto item : attr)
          path.push_back(cast<StringAttr>(item));
        auto dst = field(resultName(), path), old = field(operands[0], path);
        if (failed(dst) || failed(old))
          return failure();
        auto guard = operands[1 + j];
        auto value = operands[1 + merge.getGuards().size() + j];
        out << "  assign " << *dst << " = " << guard << " ? " << value << " : "
            << *old << ";\n";
        enables += " | " + guard;
      }
      for (auto &path : *leaves) {
        bool written = llvm::any_of(merge.getPaths(), [&](Attribute raw) {
          auto p = cast<ArrayAttr>(raw);
          return p.size() <= path.size() &&
                 std::equal(p.begin(), p.end(), path.begin());
        });
        if (written)
          continue;
        auto dst = field(resultName(), path), old = field(operands[0], path);
        if (failed(dst) || failed(old))
          return failure();
        out << "  assign " << *dst << " = " << *old << ";\n";
      }
      out << "  assign " << resultName(1) << " = " << enables << ";\n";
      return success();
    }
    return op->emitOpError() << "table operation lacks RTL emission";
  }

  LogicalResult emitInstances() {
    for (auto &operation : definition.getBody().front()) {
      if (auto queue = dyn_cast<ac::QueueOp>(operation)) {
        SmallVector<std::string> pins;
        for (auto input : queue->getOperands()) {
          auto value = net(input);
          if (failed(value))
            return failure();
          pins.push_back(*value);
        }
        for (auto output : queue->getResults()) {
          auto value = net(output);
          if (failed(value))
            return failure();
          pins.push_back(*value);
        }
        auto payload = context.rtlType(queue.getInData().getType(), queue);
        auto depth = context.staticRtlValue(queue.getDepth(), queue);
        auto latency = context.unsignedStaticValue(
            queue.getAvailabilityLatency(), queue, true);
        auto name = legalizeIdentifier(queue.getInstanceName(),
                                       [&] { return queue.emitOpError(); });
        if (failed(payload) || failed(depth) || failed(latency) || failed(name))
          return failure();
        unsigned policy;
        if (queue.getReadyPolicy() == "local_occupancy")
          policy = 0;
        else if (queue.getReadyPolicy() == "downstream_pop")
          policy = 1;
        else
          return queue.emitOpError() << "unsupported queue ready policy";
        std::string error;
        if (context.isSystem()) {
          error = "pyc_child_error_" + std::to_string(localErrors.size());
          out << "  wire " << error << ";\n";
          localErrors.push_back(error);
        }
        out << "  fifo #(.T(" << *payload << "), .DEPTH(" << *depth
            << "), .READY_POLICY(" << policy << "), .AVAILABILITY_LATENCY("
            << *latency;
        if (context.isSystem())
          out << "), .pyc_managed(1'b1";
        out << ")) pyc_instance_" << *name << " (\n";
        const StringRef ports[] = {"clk",       "rst",       "in_valid",
                                   "in_data",   "out_ready", "in_ready",
                                   "out_valid", "out_data"};
        for (auto [index, port] : llvm::enumerate(ports))
          out << "    ." << port << "(" << pins[index] << ")"
              << (index + 1 == std::size(ports) && !context.isSystem() ? "\n"
                                                                       : ",\n");
        if (context.isSystem())
          out << "    .pyc_phase(pyc_phase),\n"
                 "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
                 "    .pyc_local_error("
              << error << ")\n  );\n";
        else
          out << "  );\n";
        continue;
      }
      if (!isa<ac::InstanceOp, ac::CollectionOp>(operation))
        continue;
      Operation *occurrence = &operation;
      bool collection = isa<ac::CollectionOp>(operation);
      auto *callee =
          collection
              ? context.analysis.resolveCallee(
                    cast<ac::CollectionOp>(operation))
              : context.analysis.resolveCallee(cast<ac::InstanceOp>(operation));
      auto bindings =
          collection
              ? context.analysis.bindInstance(cast<ac::CollectionOp>(operation))
              : context.analysis.bindInstance(cast<ac::InstanceOp>(operation));
      if (failed(bindings))
        return failure();
      auto kind = context.analysis.getPrimitiveKind(callee);
      bool managedPrimitive = false;
      if (context.isSystem() && !kind.empty())
        managedPrimitive =
            llvm::any_of(context.sourceChecks().commits,
                         [&](const ac::HardwareCheckCommitEndpoint &endpoint) {
                           return endpoint.allocation == occurrence;
                         });
      SmallVector<std::string> inputs, outputs;
      for (auto input : occurrence->getOperands()) {
        auto value = net(input);
        if (failed(value))
          return failure();
        inputs.push_back(*value);
      }
      for (auto output : occurrence->getResults()) {
        auto value = net(output);
        if (failed(value))
          return failure();
        outputs.push_back(*value);
      }
      auto parameters =
          collection
              ? context.rtlInstanceParameters(cast<ac::CollectionOp>(operation))
              : context.rtlInstanceParameters(cast<ac::InstanceOp>(operation));
      auto name = legalizeIdentifier(
          occurrence->getAttrOfType<StringAttr>("instance_name").getValue(),
          [&] { return occurrence->emitOpError(); });
      if (failed(parameters) || failed(name))
        return failure();
      if (managedPrimitive) {
        if (parameters->empty())
          *parameters = " #( .pyc_managed(1'b1) )";
        else {
          auto close = parameters->rfind(')');
          if (close == std::string::npos)
            return occurrence->emitOpError()
                   << "malformed RTL primitive parameter list";
          parameters->insert(close, ", .pyc_managed(1'b1) ");
        }
      }
      std::string i, count;
      if (collection) {
        auto size = context.shapeSize(
            cast<ac::CollectionOp>(operation).getShape(), occurrence, true);
        if (failed(size))
          return failure();
        count = *size;
        i = "pyc_instance_lane_" + std::to_string(next++);
      }
      std::string childError;
      if (context.isSystem()) {
        childError = "pyc_child_error_" + std::to_string(localErrors.size());
        if (collection)
          out << "  wire [(" << count << ")-1:0] " << childError << ";\n";
        else
          out << "  wire " << childError << ";\n";
        localErrors.push_back(collection ? "(|" + childError + ")"
                                         : childError);
      }
      if (collection)
        out << "  for (genvar " << i << "=0; " << i << " < (" << count
            << "); ++" << i << ") begin : " << "pyc_instances_" << *name
            << "\n";
      out << "  " << (kind.empty() ? context.names(callee).rtl : kind.str())
          << *parameters << " pyc_instance_" << *name << " (";
      bool first = true;
      auto connect = [&](StringRef port, StringRef signal,
                         Type type) -> LogicalResult {
        auto id =
            legalizeIdentifier(port, [&] { return occurrence->emitOpError(); });
        if (failed(id))
          return failure();
        std::string net = signal.str();
        if (collection) {
          auto width = context.payloadWidth(type, occurrence, true, *bindings);
          if (failed(width))
            return failure();
          net += "[((" + count + ")-1-" + i + ")*(" + *width + ") +: (" +
                 *width + ")]";
        }
        out << (first ? "\n" : ",\n") << "    ." << *id << "(" << net << ")";
        first = false;
        return success();
      };
      auto signature = cast<FunctionType>(
          callee->getAttrOfType<TypeAttr>("function_type").getValue());
      auto inputNames = callee->getAttrOfType<ArrayAttr>("input_names");
      auto outputNames = callee->getAttrOfType<ArrayAttr>("output_names");
      for (auto [raw, signal, type] :
           llvm::zip(inputNames, inputs, signature.getInputs()))
        if (failed(connect(cast<StringAttr>(raw).getValue(), signal, type)))
          return failure();
      for (auto [raw, signal, type] :
           llvm::zip(outputNames, outputs, signature.getResults()))
        if (failed(connect(cast<StringAttr>(raw).getValue(), signal, type)))
          return failure();
      if (context.isSystem()) {
        auto managedConnection = [&](StringRef port, StringRef signal) {
          out << (first ? "\n" : ",\n") << "    ." << port << "(" << signal
              << ")";
          first = false;
        };
        managedConnection("pyc_phase", "pyc_phase");
        managedConnection("pyc_root_commit_ok", "pyc_root_commit_ok");
        if (kind.empty())
          managedConnection("pyc_reset_active", "pyc_reset_active");
        managedConnection("pyc_local_error",
                          collection ? childError + "[" + i + "]" : childError);
      }
      out << (first ? ");\n" : "\n  );\n");
      if (collection)
        out << "  end\n";
    }
    return success();
  }

  void emitAggregateError() {
    if (!context.isSystem())
      return;
    out << "  assign pyc_local_error = ";
    if (localErrors.empty())
      out << "1'b0";
    else
      out << join(localErrors, " | ");
    out << ";\n";
  }

private:
  HardwareEmitContext &context;
  ac::ModuleOp definition;
  raw_ostream &out;
  llvm::DenseMap<Value, std::string> names;
  llvm::DenseSet<Operation *> instances;
  SmallVector<std::string> localErrors;
  unsigned next = 0;
};
} // namespace
LogicalResult emitHardwareVerilogDefinition(HardwareEmitContext &context,
                                            ac::ModuleOp definition,
                                            raw_ostream &out) {
  auto &names = context.names(definition);
  out << "module " << names.rtl;
  SmallVector<std::string> parameters;
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
    } else
      text += " = 1";
    parameters.push_back(text);
  }
  for (auto raw : definition.getTypeParameters()) {
    auto id =
        context.rtlFormalName(cast<StringAttr>(raw).getValue(), definition);
    if (failed(id))
      return failure();
    parameters.push_back("parameter type " + *id + " = logic [0:0]");
  }
  if (!parameters.empty())
    out << " #(\n" << join(parameters, ",\n") << "\n)";
  out << " (";
  bool first = true;
  auto emitPort = [&](StringRef direction, Type type,
                      StringRef name) -> LogicalResult {
    auto rendered = context.rtlType(type, definition);
    if (failed(rendered))
      return failure();
    auto id =
        legalizeIdentifier(name, [&] { return definition.emitOpError(); });
    if (failed(id))
      return failure();
    out << (first ? "\n" : ",\n") << "  " << direction << " wire " << *rendered
        << " " << *id;
    first = false;
    return success();
  };
  auto signature = definition.getFunctionType();
  for (auto [i, type] : llvm::enumerate(signature.getInputs()))
    if (failed(emitPort(
            "input", type,
            cast<StringAttr>(definition.getInputNames()[i]).getValue())))
      return failure();
  for (auto [i, type] : llvm::enumerate(signature.getResults()))
    if (failed(emitPort(
            "output", type,
            cast<StringAttr>(definition.getOutputNames()[i]).getValue())))
      return failure();
  if (context.isSystem()) {
    out << (first ? "\n" : ",\n")
        << "  input wire [2:0] pyc_phase,\n"
           "  input wire pyc_root_commit_ok,\n"
           "  input wire pyc_reset_active,\n"
           "  output wire pyc_local_error";
    first = false;
  }
  out << (first ? ");\n" : "\n);\n");
  NetEmitter nets(context, definition, out);
  auto yield = cast<ac::YieldOp>(definition.getBody().front().back());
  for (auto [i, value] : llvm::enumerate(yield.getValues())) {
    auto wire = nets.net(value);
    if (failed(wire))
      return failure();
    auto id = legalizeIdentifier(
        cast<StringAttr>(definition.getOutputNames()[i]).getValue(),
        [&] { return definition.emitOpError(); });
    if (failed(id))
      return failure();
    out << "  assign " << *id << " = " << *wire << ";\n";
  }
  if (failed(nets.emitSourceChecks()))
    return failure();
  if (failed(nets.emitObservations()))
    return failure();
  if (failed(nets.emitInstances()))
    return failure();
  nets.emitAggregateError();
  out << "endmodule\n";
  return success();
}
} // namespace acir::compiler

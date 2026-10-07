#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "HardwareEmitCppChecks.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSwitch.h"
#include <array>
#include <deque>
#include <tuple>
using namespace mlir;
namespace acir::compiler {
namespace {
using Path = ac::FieldPath;
ArrayRef<StringRef> queuePinNames(bool output) {
  static const StringRef inputs[] = {"clk", "rst", "in_valid", "in_data",
                                     "out_ready"};
  static const StringRef outputs[] = {"in_ready", "out_valid", "out_data"};
  return output ? ArrayRef<StringRef>(outputs) : ArrayRef<StringRef>(inputs);
}
StringRef ownerPrefix(Operation *op) {
  return isa<ac::QueueOp>(op) ? "pyc_queue_" : "pyc_instance_";
}
FailureOr<std::string> queueKernelType(HardwareEmitContext &ctx,
                                       ac::QueueOp queue,
                                       const ac::HardwareBindings &bindings) {
  auto payload = ctx.cppType(queue.getInData().getType(), bindings, queue);
  auto depth = ctx.staticValue(queue.getDepth(), bindings, queue);
  auto latency = ctx.unsignedStaticValue(queue.getAvailabilityLatency(), queue,
                                         false, bindings);
  if (failed(payload) || failed(depth) || failed(latency))
    return failure();
  StringRef policy;
  if (queue.getReadyPolicy() == "local_occupancy")
    policy = "LocalOccupancy";
  else if (queue.getReadyPolicy() == "downstream_pop")
    policy = "DownstreamPop";
  else
    return queue.emitOpError() << "unsupported queue ready policy";
  return "gfsim::fifo_kernel<" + *payload + ", " + *depth +
         ", gfsim::QueueReadyPolicy::" + policy.str() + ", " + *latency + ">";
}
struct Scope {
  ac::ModuleOp definition;
  ac::HardwareBindings bindings;
  Scope *parent = nullptr;
  Operation *occurrence = nullptr;
  std::string object, count;
};
struct Node {
  Scope *scope;
  Value value;
  Path path;
  std::string name;
};
struct Leaf {
  Scope *scope;
  Operation *op;
  ac::HardwareBindings bindings;
  std::string prefix, count, kernel;
  bool sampled = false;
  std::array<bool, 3> queueRead{};
};
class WorkEmitter {
public:
  WorkEmitter(HardwareEmitContext &context, ac::ModuleOp definition,
              raw_ostream &stream, bool resetting = false)
      : ctx(context), out(stream), resetting(resetting) {
    ac::HardwareBindings b;
    b.owner = definition;
    scopes.push_back({definition, b, nullptr, nullptr, "this->", "pyc_count"});
  }
  LogicalResult run() {
    if (failed(collect(scopes.front())))
      return failure();
    if (resetting) {
      for (auto &leaf : storage) {
        StringRef kind = ctx.analysis.getPrimitiveKind(leaf.bindings.owner);
        if (kind == "dff" || kind == "dffe") {
          unsigned index = kind == "dff" ? 3 : 4;
          auto init = emit(*leaf.scope, leaf.op->getOperand(index));
          if (failed(init) ||
              failed(setPort(*leaf.scope, leaf.op, index, {}, *init)))
            return failure();
        }
        auto inputs = kernelInputs(leaf, "pyc_lane");
        auto outputs = kernelOutputs(leaf, "pyc_lane");
        if (failed(inputs) || failed(outputs))
          return failure();
        out << "    " << leaf.prefix
            << "state->reset([&](std::size_t pyc_lane) noexcept { return "
            << *inputs << "; });\n";
      }
      return success();
    }
    auto partitions =
        ctx.analysis.getIndependentWorkSubtrees(scopes.front().definition);
    if (failed(partitions))
      return failure();
    delegated = std::move(*partitions);
    // Prepare immutable task inputs through the existing pure SSA schedule.
    // Self-feedback reads temporal outputs; no leaf Work runs in this step.
    for (Operation *task : delegated)
      for (auto [port, value] : llvm::enumerate(task->getOperands())) {
        auto input = emit(scopes.front(), value);
        if (failed(input) ||
            failed(setPort(scopes.front(), task, port, {}, *input)))
          return failure();
      }
    if (!delegated.empty()) {
      out << "    std::array<gfsim::WorkItem, " << delegated.size()
          << "> pyc_tasks{{\n";
      for (Operation *task : delegated) {
        auto child = id(
            task->getAttrOfType<StringAttr>("instance_name").getValue(), task);
        if (failed(child))
          return failure();
        auto member = "pyc_instance_" + *child;
        out << "      {" << member << ".get(), +[](void *instance) { "
            << "static_cast<typename decltype(" << member
            << ")::element_type *>(instance)->Work(); }},\n";
      }
      out << "    }};\n    if (pyc_work_executor_) "
             "pyc_work_executor_->run(pyc_tasks);\n"
             "    else for (auto task : pyc_tasks) task.work(task.instance);\n";
    }
    tasksFinished = true;
    if (failed(outputs(scopes.front())))
      return failure();
    for (auto &leaf : storage)
      if (!isDelegated(*leaf.scope) && failed(sample(leaf)))
        return failure();
    for (auto &scope : scopes)
      if (&scope != &scopes.front() && !isDelegated(scope) &&
          failed(outputs(scope)))
        return failure();
    // Check-only roots use the same memoized SSA/old-Q evaluation as outputs
    // and storage pins. Flattened child scopes capture their own lane planes.
    for (auto &scope : scopes)
      if (!isDelegated(scope) &&
          failed(emitCppCheckCapture(
              ctx, scope.definition, scope.object, scope.count,
              [&](Value value) { return emit(scope, value); }, out)))
        return failure();
    // Native observations stage into the system-owned slots inside Work, so
    // `SimSystem::Precommit(cycle_)`/`Xfer(cycle_ + 1)` publish exactly the
    // epoch that evaluated them. A subtree that observes is never delegated
    // (`HardwareWorkPartition::hasWorkDependentRead`), so every observed scope
    // is emitted on this in-order path.
    for (auto &scope : scopes)
      if (!isDelegated(scope) &&
          failed(cppObservationStaging(
              ctx, scope.definition, scope.bindings, scope.object, scope.count,
              [&](Value value) { return emit(scope, value); }, out)))
        return failure();
    return success();
  }

private:
  bool isDelegated(Scope &scope) const {
    for (Scope *current = &scope; current->parent; current = current->parent)
      if (current->parent == &scopes.front())
        return llvm::is_contained(delegated, current->occurrence);
    return false;
  }
  FailureOr<std::string> id(StringRef raw, Operation *site) {
    return legalizeIdentifier(raw, [&] { return site->emitOpError(); });
  }
  FailureOr<Type> type(Type t, Scope &s, Operation *site) {
    return ctx.analysis.resolveType(t, s.bindings, site);
  }
  FailureOr<std::string> payload(Type t, Scope &s, Operation *site) {
    auto r = type(t, s, site);
    if (failed(r))
      return failure();
    return ctx.cppType(*r, s.bindings, site);
  }
  FailureOr<std::string> width(Type t, Scope &s, Operation *site) {
    auto p = payload(t, s, site);
    if (failed(p))
      return failure();
    return "gfsim::hardware_traits<" + *p + ">::width";
  }
  FailureOr<Type> element(Type t, Scope &s, Operation *site) {
    auto r = type(t, s, site);
    if (failed(r))
      return failure();
    if (auto table = dyn_cast<ac::TableType>(*r))
      return table.getElementType();
    return *r;
  }
  FailureOr<Type> fieldType(Type t, ArrayRef<StringAttr> path, Scope &s,
                            Operation *site) {
    auto r = element(t, s, site);
    if (failed(r))
      return failure();
    t = *r;
    for (auto name : path) {
      auto record = dyn_cast<ac::StructType>(t);
      if (!record)
        return site->emitOpError() << "field plan projects non-struct payload";
      bool found = false;
      for (auto raw : ctx.analysis.lookupStruct(record).getFields()) {
        auto field = cast<DictionaryAttr>(raw);
        if (field.getAs<StringAttr>("name") == name) {
          t = field.getAs<TypeAttr>("type").getValue();
          found = true;
          break;
        }
      }
      if (!found)
        return site->emitOpError() << "unknown field in C++ work plan";
    }
    return t;
  }
  FailureOr<std::string> offset(Type t, ArrayRef<StringAttr> path, Scope &s,
                                Operation *site) {
    auto r = element(t, s, site);
    if (failed(r))
      return failure();
    t = *r;
    std::string result = "0";
    for (auto name : path) {
      auto record = dyn_cast<ac::StructType>(t);
      if (!record)
        return site->emitOpError() << "packed projection requires struct";
      bool after = false, found = false;
      Type selected;
      for (auto raw : ctx.analysis.lookupStruct(record).getFields()) {
        auto f = cast<DictionaryAttr>(raw);
        auto ft = f.getAs<TypeAttr>("type").getValue();
        if (after) {
          auto w = width(ft, s, site);
          if (failed(w))
            return failure();
          result += " + " + *w;
        }
        if (f.getAs<StringAttr>("name") == name) {
          selected = ft;
          found = after = true;
        }
      }
      if (!found)
        return site->emitOpError() << "unknown packed projection field";
      t = selected;
    }
    return result;
  }
  FailureOr<std::string> cardinality(Type t, Scope &s, Operation *site) {
    auto r = type(t, s, site);
    if (failed(r))
      return failure();
    if (auto table = dyn_cast<ac::TableType>(*r))
      return ctx.shapeSize(table.getShape(), site, false, s.bindings);
    return std::string("1");
  }
  FailureOr<std::string> planeCount(Type t, Scope &s, Operation *site) {
    auto n = cardinality(t, s, site);
    if (failed(n))
      return failure();
    return "(" + s.count + " * (" + *n + "))";
  }
  FailureOr<std::string> planeType(Type t, Scope &s, Path path,
                                   Operation *site) {
    auto e = fieldType(t, path, s, site);
    auto n = planeCount(t, s, site);
    if (failed(e) || failed(n))
      return failure();
    auto p = payload(*e, s, site);
    if (failed(p))
      return failure();
    return "gfsim::wire<gfsim::table<" + *p + ", " + *n + ">>";
  }
  Scope *childScope(Scope &parent, Operation *op) {
    for (auto &s : scopes)
      if (s.parent == &parent && s.occurrence == op)
        return &s;
    return nullptr;
  }
  FailureOr<ac::HardwareBindings> bind(Operation *op, Scope &s) {
    if (isa<ac::QueueOp>(op))
      return s.bindings;
    return isa<ac::InstanceOp>(op)
               ? ctx.analysis.bindInstance(cast<ac::InstanceOp>(op), s.bindings)
               : ctx.analysis.bindInstance(cast<ac::CollectionOp>(op),
                                           s.bindings);
  }
  LogicalResult collect(Scope &s) {
    for (Operation &op : s.definition.getBody().front())
      if (isa<ac::InstanceOp, ac::CollectionOp, ac::QueueOp>(op)) {
        auto name =
            id(op.getAttrOfType<StringAttr>("instance_name").getValue(), &op);
        auto b = bind(&op, s);
        if (failed(name) || failed(b))
          return failure();
        std::string count = s.count;
        if (auto c = dyn_cast<ac::CollectionOp>(op)) {
          auto n = ctx.shapeSize(c.getShape(), &op, false, s.bindings);
          if (failed(n))
            return failure();
          count = "(" + count + " * (" + *n + "))";
        }
        if (isa<ac::QueueOp>(op) ||
            !ctx.analysis.getPrimitiveKind(b->owner).empty()) {
          auto kernel = isa<ac::QueueOp>(op)
                            ? queueKernelType(ctx, cast<ac::QueueOp>(op), *b)
                            : ctx.cppBoundModuleType(b->owner, *b, &op, true);
          if (failed(kernel))
            return failure();
          storage.push_back({&s, &op, *b,
                             s.object + ownerPrefix(&op).str() + *name + "_",
                             count, *kernel, false});
        } else {
          scopes.push_back({cast<ac::ModuleOp>(b->owner), *b, &s, &op,
                            s.object + "pyc_instance_" + *name + "->", count});
          if (failed(collect(scopes.back())))
            return failure();
        }
      }
    return success();
  }
  FailureOr<std::string> pinName(Operation *op, unsigned port, bool output) {
    if (isa<ac::QueueOp>(op))
      return id(queuePinNames(output)[port], op);
    auto *callee = ctx.analysis.lookupDefinition(
        op->getAttrOfType<FlatSymbolRefAttr>("callee").getValue());
    return id(
        cast<StringAttr>(callee->getAttrOfType<ArrayAttr>(
                             output ? "output_names" : "input_names")[port])
            .getValue(),
        op);
  }
  FailureOr<std::string> pinObject(Scope &s, Operation *op, unsigned port,
                                   bool output) {
    auto name =
        id(op->getAttrOfType<StringAttr>("instance_name").getValue(), op);
    auto pin = pinName(op, port, output);
    if (failed(name) || failed(pin))
      return failure();
    auto b = bind(op, s);
    if (failed(b))
      return failure();
    return s.object + ownerPrefix(op).str() + *name +
           (!isa<ac::QueueOp>(op) &&
                    ctx.analysis.getPrimitiveKind(b->owner).empty()
                ? "->"
                : "_") +
           *pin;
  }
  LogicalResult setPort(Scope &s, Operation *op, unsigned port, Path path,
                        StringRef value) {
    auto target = pinObject(s, op, port, false);
    if (failed(target))
      return failure();
    auto count = planeCount(op->getOperand(port).getType(), s, op);
    if (failed(count))
      return failure();
    if (path.empty())
      out << "    for (std::size_t pyc_pin = 0; pyc_pin < " << *count
          << "; ++pyc_pin) " << *target << ".element(pyc_pin) = " << value
          << ".element(pyc_pin);\n";
    else {
      auto e = element(op->getOperand(port).getType(), s, op);
      auto p =
          failed(e) ? FailureOr<std::string>(failure()) : payload(*e, s, op);
      auto low = offset(op->getOperand(port).getType(), path, s, op);
      if (failed(p) || failed(low))
        return failure();
      out << "    for (std::size_t pyc_pin = 0; pyc_pin < " << *count
          << "; ++pyc_pin) " << *target << ".element(pyc_pin).assignSlice("
          << *low << ", " << value << ".element(pyc_pin).packed());\n";
    }
    return success();
  }
  FailureOr<std::string> kernelInputs(Leaf &leaf, StringRef lane) {
    if (isa<ac::QueueOp>(leaf.op)) {
      SmallVector<std::string> pins;
      for (auto [index, name] : llvm::enumerate(queuePinNames(false)))
        pins.push_back(leaf.prefix +
                       (index == 3
                            ? "input_tokens[" + lane.str() + "]"
                            : name.str() + ".element(" + lane.str() + ")"));
      return "typename " + leaf.kernel + "::Inputs{" + join(pins, ", ") + "}";
    }
    auto names = leaf.bindings.owner->getAttrOfType<ArrayAttr>("input_names");
    StringRef kind = ctx.analysis.getPrimitiveKind(leaf.bindings.owner);
    SmallVector<std::string> pins;
    for (auto raw : names) {
      auto name = id(cast<StringAttr>(raw).getValue(), leaf.op);
      if (failed(name))
        return failure();
      pins.push_back(leaf.prefix + *name + ".element(" + lane.str() + ")");
    }
    if (kind == "sync_mem")
      pins = {pins[0],
              pins[1],
              "{&" + pins[2] + "}",
              "{&" + pins[3] + "}",
              pins[4],
              pins[5],
              pins[6],
              pins[7]};
    else if (kind == "sync_mem_dp")
      pins = {pins[0],
              pins[1],
              "{&" + pins[2] + ", &" + pins[4] + "}",
              "{&" + pins[3] + ", &" + pins[5] + "}",
              pins[6],
              pins[7],
              pins[8],
              pins[9]};
    return "typename " + leaf.kernel + "::Inputs{" + join(pins, ", ") + "}";
  }
  FailureOr<std::string> kernelOutputs(Leaf &leaf, StringRef lane) {
    if (isa<ac::QueueOp>(leaf.op))
      return "typename " + leaf.kernel + "::Outputs{" + leaf.prefix +
             "in_ready.element(" + lane.str() + "), " + leaf.prefix +
             "out_valid.element(" + lane.str() + "), " + leaf.prefix +
             "output_tokens[" + lane.str() + "]}";
    SmallVector<std::string> pins;
    for (auto raw :
         leaf.bindings.owner->getAttrOfType<ArrayAttr>("output_names")) {
      auto name = id(cast<StringAttr>(raw).getValue(), leaf.op);
      if (failed(name))
        return failure();
      pins.push_back(leaf.prefix + *name + ".element(" + lane.str() + ")");
    }
    StringRef kind = ctx.analysis.getPrimitiveKind(leaf.bindings.owner);
    if (kind == "sync_mem" || kind == "sync_mem_dp") {
      for (auto &pin : pins)
        pin = "&" + pin;
      return "typename " + leaf.kernel + "::Outputs{{" + join(pins, ", ") +
             "}}";
    }
    return "typename " + leaf.kernel + "::Outputs{" + join(pins, ", ") + "}";
  }
  LogicalResult queueTokenCopy(Leaf &leaf, bool gather) {
    auto queue = cast<ac::QueueOp>(leaf.op);
    auto t = type(queue.getInData().getType(), *leaf.scope, leaf.op);
    auto n = cardinality(queue.getInData().getType(), *leaf.scope, leaf.op);
    if (failed(t) || failed(n))
      return failure();
    std::string token =
        leaf.prefix + (gather ? "input_tokens" : "output_tokens");
    std::string plane = leaf.prefix + (gather ? "in_data" : "out_data");
    out << "    for (std::size_t pyc_queue_lane = 0; pyc_queue_lane < "
        << leaf.count << "; ++pyc_queue_lane) {\n";
    if (isa<ac::TableType>(*t)) {
      out << "      for (std::size_t pyc_token_element = 0; pyc_token_element "
             "< ("
          << *n << "); ++pyc_token_element) ";
      token += "[pyc_queue_lane].element(pyc_token_element)";
      plane += ".element(pyc_queue_lane * (" + *n + ") + pyc_token_element)";
    } else {
      out << "      ";
      token += "[pyc_queue_lane]";
      plane += ".element(pyc_queue_lane)";
    }
    out << (gather ? token : plane) << " = " << (gather ? plane : token)
        << ";\n    }\n";
    return success();
  }
  LogicalResult sample(Leaf &leaf) {
    if (leaf.sampled)
      return success();
    if (resetting)
      return leaf.op->emitOpError()
             << "reset init requires asynchronous memory read; pure leaf read "
                "kernel is required";
    for (auto [i, v] : llvm::enumerate(leaf.op->getOperands())) {
      auto text = emit(*leaf.scope, v);
      if (failed(text) || failed(setPort(*leaf.scope, leaf.op, i, {}, *text)))
        return failure();
    }
    if (isa<ac::QueueOp>(leaf.op) && failed(queueTokenCopy(leaf, true)))
      return failure();
    auto in = kernelInputs(leaf, "pyc_lane"),
         q = kernelOutputs(leaf, "pyc_lane");
    if (failed(in) || failed(q))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << leaf.count
        << "; ++pyc_lane) " << leaf.prefix << "state->work(pyc_lane, " << *in
        << ", " << *q << ");\n";
    leaf.sampled = true;
    return success();
  }
  std::string name() { return "pyc_value_" + std::to_string(nextName++); }
  void memoize(Scope &s, Value v, Path p, std::string text) {
    memo.push_back({&s, v, p, std::move(text)});
  }
  FailureOr<std::string> emit(Scope &s, Value v, Path path = {}) {
    for (auto &n : memo)
      if (n.scope == &s && n.value == v && n.path == path)
        return n.name;
    for (auto &n : active)
      if (n.scope == &s && n.value == v && n.path == path)
        return s.definition.emitOpError()
               << "static field schedule has an unresolved cycle";
    active.push_back({&s, v, path, {}});
    llvm::scope_exit cleanup([&] { active.pop_back(); });
    auto selected =
        fieldType(v.getType(), path, s,
                  v.getDefiningOp() ? v.getDefiningOp() : s.definition);
    if (failed(selected))
      return failure();
    FailureOr<std::string> result = failure();
    if (isa<ac::StructType>(*selected)) {
      auto leaves =
          ctx.analysis.getFieldPaths(*selected, s.bindings, s.definition);
      auto temp = declarePlane(s, v, path);
      auto count = planeCount(v.getType(), s, s.definition);
      auto p = payload(*selected, s, s.definition);
      if (failed(leaves) || failed(temp) || failed(count) || failed(p))
        return failure();
      std::string packed;
      for (auto suffix : *leaves) {
        Path full = path;
        llvm::append_range(full, suffix);
        auto field = emit(s, v, full);
        if (failed(field))
          return failure();
        auto expression = *field + ".element(pyc_lane).packed()";
        packed = packed.empty()
                     ? expression
                     : "gfsim::concat(" + packed + ", " + expression + ")";
      }
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << *count
          << "; ++pyc_lane) " << *temp << ".element(pyc_lane) = gfsim::wire<"
          << *p << ">::fromPacked(" << packed << ");\n";
      result = *temp;
    } else
      result = build(s, v, path);
    if (failed(result))
      return failure();
    memoize(s, v, path, *result);
    return *result;
  }
  FailureOr<std::string> projectPlane(Scope &s, Value v, Path path,
                                      StringRef source) {
    if (path.empty())
      return source.str();
    auto t = planeType(v.getType(), s, path,
                       v.getDefiningOp() ? v.getDefiningOp() : s.definition);
    auto e = fieldType(v.getType(), path, s, s.definition);
    auto w = failed(e) ? FailureOr<std::string>(failure())
                       : width(*e, s, s.definition);
    auto low = offset(v.getType(), path, s, s.definition);
    auto count = planeCount(v.getType(), s, s.definition);
    if (failed(t) || failed(w) || failed(low) || failed(count))
      return failure();
    auto result = name();
    out << "    " << *t << " " << result << ";\n";
    // element_type is a payload type, while element() is a wire. Use its
    // known wire type explicitly for all four-state planes.
    auto p = payload(*e, s, s.definition);
    if (failed(p))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << *count
        << "; ++pyc_lane) " << result << ".element(pyc_lane) = gfsim::wire<"
        << *p << ">::fromPacked(gfsim::extract<" << *w << ">(" << source
        << ".element(pyc_lane).packed(), " << *low << "));\n";
    return result;
  }
  std::string knownInteger(ac::MathIntAttr integer, StringRef width) {
    llvm::APSInt n(integer.getCanonicalValue());
    unsigned wordCount = std::max(
        1u, n.getActiveBits() / 64 + unsigned(n.getActiveBits() % 64 != 0));
    SmallVector<std::string> words;
    for (unsigned i = 0; i < wordCount; ++i)
      words.push_back(std::to_string(n.getRawData()[i]) + "ULL");
    return "gfsim::FourState<" + width.str() + ">::known(gfsim::Bits<" +
           width.str() + ">{" + join(words, ", ") + "})";
  }
  FailureOr<std::string> constant(ac::BitsConstantOp op, Scope &s) {
    auto w = width(op.getResult().getType(), s, op);
    if (failed(w))
      return failure();
    if (ctx.isClosedStatic(op.getValue(), s.bindings)) {
      auto raw = ctx.analysis.evaluateStatic(op.getValue(), s.bindings, op);
      if (failed(raw))
        return failure();
      auto integer = dyn_cast<ac::MathIntAttr>(*raw);
      if (!integer)
        return op.emitOpError() << "bits constant requires integer";
      return knownInteger(integer, *w);
    }
    auto literal = ctx.staticValue(op.getValue(), s.bindings, op);
    if (failed(literal))
      return failure();
    return "gfsim::FourState<" + *w + ">::known(pyc_bits<" + *w + ">(" +
           *literal + "))";
  }
  FailureOr<std::string>
  scalarExpression(Scope &s, Value value, Path path,
                   function_ref<FailureOr<std::string>(Value, Path)> operand) {
    Operation *op = value.getDefiningOp();
    if (!op)
      return failure();
    if (auto constantOp = dyn_cast<ac::BitsConstantOp>(op))
      return constant(constantOp, s);
    if (auto create = dyn_cast<ac::EnumCreateOp>(op)) {
      auto definition =
          ctx.analysis.resolveEnum(create.getResult().getType(), op);
      if (failed(definition))
        return failure();
      for (const auto &member : definition->members)
        if (member.name == create.getMemberAttr())
          return knownInteger(member.code, std::to_string(definition->width));
      return op->emitOpError() << "unresolved enum member during C++ emission";
    }
    if (auto toBits = dyn_cast<ac::EnumToBitsOp>(op))
      return operand(toBits.getInput(), {});
    if (auto fromBits = dyn_cast<ac::EnumFromBitsOp>(op)) {
      auto raw = operand(fromBits.getInput(), {});
      if (failed(raw))
        return failure();
      if (cast<OpResult>(value).getResultNumber() == 0)
        return *raw;
      auto definition =
          ctx.analysis.resolveEnum(fromBits.getValue().getType(), op);
      if (failed(definition))
        return failure();
      SmallVector<std::string> membership;
      for (const auto &member : definition->members) {
        auto code =
            knownInteger(member.code, std::to_string(definition->width));
        membership.push_back("gfsim::compare<gfsim::ComparePredicate::EQ>(" +
                             *raw + ", " + code + ")");
      }
      // Bound expression depth by log(member count), preserving the same
      // four-state EQ/OR function without repeatedly copying a growing prefix.
      while (membership.size() > 1) {
        SmallVector<std::string> combined;
        for (size_t i = 0; i < membership.size(); i += 2)
          if (i + 1 == membership.size())
            combined.push_back(std::move(membership[i]));
          else
            combined.push_back("gfsim::bit_or(" + membership[i] + ", " +
                               membership[i + 1] + ")");
        membership = std::move(combined);
      }
      return membership.front();
    }
    if (auto get = dyn_cast<ac::StructGetOp>(op)) {
      Path p{get.getFieldAttr()};
      llvm::append_range(p, path);
      return operand(get.getValue(), p);
    }
    if (auto create = dyn_cast<ac::StructCreateOp>(op)) {
      auto fields =
          ctx.analysis.lookupStruct(create.getResult().getType()).getFields();
      if (!path.empty())
        for (auto [i, raw] : llvm::enumerate(fields))
          if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
              path.front())
            return operand(create.getValues()[i],
                           Path(path.begin() + 1, path.end()));
      std::string text;
      for (auto v : create.getValues()) {
        auto e = operand(v, {});
        if (failed(e))
          return failure();
        text = text.empty() ? *e : "gfsim::concat(" + text + ", " + *e + ")";
      }
      return text;
    }
    if (auto merge = dyn_cast<ac::ValueMergeOp>(op)) {
      unsigned result = cast<OpResult>(value).getResultNumber();
      if (result) {
        std::string en = "gfsim::FourState<1>::known(gfsim::Bits<1>{0})";
        for (auto guard : merge.getGuards()) {
          auto g = operand(guard, {});
          if (failed(g))
            return failure();
          en = "gfsim::bit_or(" + en + ", " + *g + ")";
        }
        return en;
      }
      auto baseType = type(merge.getBase().getType(), s, op);
      if (failed(baseType))
        return failure();
      if (merge.getValues().empty())
        return operand(merge.getBase(), path);
      if (!path.empty() ||
          isa<ac::BitsType, ac::EnumType, ac::TypeParamType>(*baseType)) {
        auto base = operand(merge.getBase(), path);
        if (failed(base))
          return failure();
        for (auto [i, raw] : llvm::enumerate(merge.getPaths())) {
          Path selected;
          for (auto n : cast<ArrayAttr>(raw))
            selected.push_back(cast<StringAttr>(n));
          if (selected.size() <= path.size() &&
              std::equal(selected.begin(), selected.end(), path.begin())) {
            auto g = operand(merge.getGuards()[i], {}),
                 candidate =
                     operand(merge.getValues()[i],
                             Path(path.begin() + selected.size(), path.end()));
            if (failed(g) || failed(candidate))
              return failure();
            return "gfsim::select(" + *g + ", " + *candidate + ", " + *base +
                   ")";
          }
        }
        return *base;
      }
      auto leaves = ctx.analysis.getFieldPaths(value.getType(), s.bindings, op);
      if (failed(leaves))
        return failure();
      std::string text;
      for (auto p : *leaves) {
        auto e = scalarExpression(s, value, p, operand);
        if (failed(e))
          return failure();
        text = text.empty() ? *e : "gfsim::concat(" + text + ", " + *e + ")";
      }
      return text;
    }
    SmallVector<std::string> operands;
    for (auto v : op->getOperands()) {
      auto e = operand(v, {});
      if (failed(e))
        return failure();
      operands.push_back(*e);
    }
    auto w = width(value.getType(), s, op);
    if (failed(w))
      return failure();
    if (isa<ac::BitsUnaryOp>(op))
      return "gfsim::bit_not(" + operands[0] + ")";
    if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
      auto opcode = llvm::StringSwitch<StringRef>(binary.getOpcode())
                        .Case("and", "bit_and")
                        .Case("or", "bit_or")
                        .Case("xor", "bit_xor")
                        .Default(binary.getOpcode());
      return "gfsim::" + opcode.str() + "(" + operands[0] + ", " + operands[1] +
             ")";
    }
    if (auto compare = dyn_cast<ac::BitsCompareOp>(op)) {
      auto predicate = llvm::StringSwitch<StringRef>(compare.getPredicate())
                           .Case("eq", "EQ")
                           .Case("ne", "NE")
                           .Case("ult", "ULT")
                           .Case("ule", "ULE")
                           .Case("ugt", "UGT")
                           .Case("uge", "UGE")
                           .Case("slt", "SLT")
                           .Case("sle", "SLE")
                           .Case("sgt", "SGT")
                           .Case("sge", "SGE")
                           .Default("");
      return "gfsim::compare<gfsim::ComparePredicate::" + predicate.str() +
             ">(" + operands[0] + ", " + operands[1] + ")";
    }
    if (isa<ac::BitsSelectOp>(op))
      return "gfsim::select(" + operands[0] + ", " + operands[1] + ", " +
             operands[2] + ")";
    if (isa<ac::BitsConcatOp>(op)) {
      std::string text = operands[0];
      for (unsigned i = 1; i < operands.size(); ++i)
        text = "gfsim::concat(" + text + ", " + operands[i] + ")";
      return text;
    }
    if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
      auto low = ctx.staticValue(extract.getLow(), s.bindings, op);
      if (failed(low))
        return failure();
      return "gfsim::extract<" + *w + ">(" + operands[0] + ", " + *low + ")";
    }
    if (auto resize = dyn_cast<ac::BitsResizeOp>(op)) {
      auto mode = llvm::StringSwitch<StringRef>(resize.getMode())
                      .Case("trunc", "truncate")
                      .Case("zext", "zero_extend")
                      .Case("sext", "sign_extend")
                      .Default("");
      return "gfsim::" + mode.str() + "<" + *w + ">(" + operands[0] + ")";
    }
    return op->emitOpError() << "operation lacks scalar C++ hardware emission";
  }
  FailureOr<std::string> scalarProjection(Scope &s, Type original, Path path,
                                          StringRef packed, Operation *site) {
    if (path.empty())
      return packed.str();
    auto t = fieldType(original, path, s, site);
    auto w = failed(t) ? FailureOr<std::string>(failure()) : width(*t, s, site);
    auto low = offset(original, path, s, site);
    if (failed(w) || failed(low))
      return failure();
    return "gfsim::extract<" + *w + ">(" + packed.str() + ", " + *low + ")";
  }
  FailureOr<std::string> regionExpression(
      Scope &s, Region &region, Value result, Path path,
      function_ref<FailureOr<std::string>(BlockArgument, Path)> argument) {
    struct ScalarNode {
      Value value;
      Path path;
      std::string text;
    };
    SmallVector<ScalarNode> local;
    std::function<FailureOr<std::string>(Value, Path)> visit;
    visit = [&](Value v, Path p) -> FailureOr<std::string> {
      for (auto &n : local)
        if (n.value == v && n.path == p)
          return n.text;
      FailureOr<std::string> expr = failure();
      if (auto arg = dyn_cast<BlockArgument>(v)) {
        if (arg.getOwner() != &region.front())
          return failure();
        expr = argument(arg, p);
      } else
        expr = scalarExpression(s, v, p,
                                [&](Value x, Path q) { return visit(x, q); });
      if (failed(expr))
        return failure();
      auto temp = name();
      out << "      auto " << temp << " = " << *expr << ";\n";
      local.push_back({v, p, temp});
      return temp;
    };
    return visit(result, path);
  }
  FailureOr<std::string> declarePlane(Scope &s, Value v, Path path = {}) {
    auto p = planeType(v.getType(), s, path,
                       v.getDefiningOp() ? v.getDefiningOp() : s.definition);
    if (failed(p))
      return failure();
    auto temp = name();
    out << "    " << *p << " " << temp << ";\n";
    return temp;
  }
  FailureOr<std::string> scalarPlane(Scope &s, Value v, Path path) {
    auto result = declarePlane(s, v, path);
    if (failed(result))
      return failure();
    Operation *op = v.getDefiningOp();
    SmallVector<std::tuple<Value, Path, std::string>> inputs;
    auto gather = [&](Value input, Path p) -> FailureOr<std::string> {
      for (auto &entry : inputs)
        if (std::get<0>(entry) == input && std::get<1>(entry) == p)
          return std::get<2>(entry) + ".element(pyc_lane).packed()";
      auto text = emit(s, input, p);
      if (failed(text))
        return failure();
      inputs.push_back({input, p, *text});
      return *text + ".element(pyc_lane).packed()";
    };
    auto expr = scalarExpression(s, v, path, gather);
    if (failed(expr))
      return failure();
    auto e = fieldType(v.getType(), path, s, op);
    auto p = failed(e) ? FailureOr<std::string>(failure()) : payload(*e, s, op);
    if (failed(p))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
        << "; ++pyc_lane) " << *result << ".element(pyc_lane) = gfsim::wire<"
        << *p << ">::fromPacked(" << *expr << ");\n";
    return *result;
  }
  FailureOr<std::string> build(Scope &s, Value v, Path path) {
    if (auto arg = dyn_cast<BlockArgument>(v)) {
      if (auto rule = dyn_cast<ac::RuleOp>(arg.getOwner()->getParentOp()))
        return emit(s, rule.getCaptures()[arg.getArgNumber()], path);
      unsigned port = arg.getArgNumber();
      if (s.parent) {
        auto source = emit(*s.parent, s.occurrence->getOperand(port), path);
        if (failed(source) ||
            failed(setPort(*s.parent, s.occurrence, port, path, *source)))
          return failure();
        return *source;
      }
      auto pin =
          id(cast<StringAttr>(s.definition.getInputNames()[port]).getValue(),
             s.definition);
      if (failed(pin))
        return failure();
      return projectPlane(s, v, path, s.object + *pin);
    }
    auto *op = v.getDefiningOp();
    if (!op)
      return failure();
    if (auto rule = dyn_cast<ac::RuleOp>(op))
      return emit(s,
                  cast<ac::YieldOp>(rule.getBody().front().back())
                      .getValues()[cast<OpResult>(v).getResultNumber()],
                  path);
    if (auto queue = dyn_cast<ac::QueueOp>(op)) {
      unsigned port = cast<OpResult>(v).getResultNumber();
      for (auto &leaf : storage) {
        if (leaf.scope != &s || leaf.op != op)
          continue;
        if (!leaf.queueRead[port]) {
          if (port == 0 && queue.getReadyPolicy() == "downstream_pop") {
            auto ready = emit(s, queue.getOutReady());
            if (failed(ready) || failed(setPort(s, op, 4, {}, *ready)))
              return failure();
          }
          out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << leaf.count
              << "; ++pyc_lane) " << leaf.prefix;
          if (port == 2)
            out << "output_tokens[pyc_lane] = " << leaf.kernel << "::readData(";
          else
            out << queuePinNames(true)[port]
                << ".element(pyc_lane) = " << leaf.kernel
                << (port == 0 ? "::readReady(" : "::readValid(");
          out << leaf.prefix << "state->current(pyc_lane)";
          if (port == 0)
            out << ", " << leaf.prefix << "out_ready.element(pyc_lane)";
          out << ");\n";
          if (port == 2 && failed(queueTokenCopy(leaf, false)))
            return failure();
          leaf.queueRead[port] = true;
        }
        auto pin = pinObject(s, op, port, true);
        if (failed(pin))
          return failure();
        return projectPlane(s, v, path, *pin);
      }
      return queue.emitOpError() << "missing queue storage scope";
    }
    if (isa<ac::InstanceOp, ac::CollectionOp>(op)) {
      auto b = bind(op, s);
      if (failed(b))
        return failure();
      unsigned port = cast<OpResult>(v).getResultNumber();
      if (ctx.analysis.getPrimitiveKind(b->owner).empty()) {
        if (tasksFinished && &s == &scopes.front() &&
            llvm::is_contained(delegated, op)) {
          auto pin = pinObject(s, op, port, true);
          if (failed(pin))
            return failure();
          return projectPlane(s, v, path, *pin);
        }
        auto *nested = childScope(s, op);
        if (!nested)
          return op->emitOpError() << "missing hardware occurrence scope";
        auto yield =
            cast<ac::YieldOp>(nested->definition.getBody().front().back());
        return emit(*nested, yield.getValues()[port], path);
      }
      if (ctx.analysis.getPrimitiveKind(b->owner) == "byte_mem")
        for (auto &leaf : storage)
          if (leaf.scope == &s && leaf.op == op) {
            if (resetting && !leaf.sampled) {
              auto address = emit(s, op->getOperand(2));
              if (failed(address) || failed(setPort(s, op, 2, {}, *address)))
                return failure();
              auto pins = kernelOutputs(leaf, "pyc_lane");
              if (failed(pins))
                return failure();
              out << "    for (std::size_t pyc_lane = 0; pyc_lane < "
                  << leaf.count << "; ++pyc_lane) " << leaf.kernel << "::read("
                  << leaf.prefix << "state->current(pyc_lane), " << leaf.prefix
                  << "raddr.element(pyc_lane), " << *pins << ");\n";
              leaf.sampled = true;
            } else if (!resetting && failed(sample(leaf)))
              return failure();
          }
      auto pin = pinObject(s, op, port, true);
      if (failed(pin))
        return failure();
      return projectPlane(s, v, path, *pin);
    }
    if (auto create = dyn_cast<ac::StructCreateOp>(op);
        create && !path.empty()) {
      for (auto [i, raw] : llvm::enumerate(
               ctx.analysis.lookupStruct(create.getResult().getType())
                   .getFields()))
        if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == path.front())
          return emit(s, create.getValues()[i],
                      Path(path.begin() + 1, path.end()));
    }
    if (auto get = dyn_cast<ac::StructGetOp>(op)) {
      Path p{get.getFieldAttr()};
      llvm::append_range(p, path);
      return emit(s, get.getValue(), p);
    }
    if (isa<ac::TableCreateOp, ac::TableSplatOp, ac::TableViewOp,
            ac::TableMapOp, ac::TableIndexOp, ac::TableGetOp, ac::TableMatchOp,
            ac::TableChooseOp, ac::TableFoldOp>(op))
      return tableOperation(s, v, path);
    return scalarPlane(s, v, path);
  }
  struct RegionInput {
    unsigned argument;
    Path path;
    std::string plane;
  };
  FailureOr<SmallVector<RegionInput>> regionInputs(Scope &s, Region &r,
                                                   Value yield, Path path,
                                                   ValueRange operands,
                                                   bool ordinal) {
    SmallVector<RegionInput> inputs;
    SmallVector<std::pair<Value, Path>> seen;
    std::function<LogicalResult(Value, Path)> visit;
    visit = [&](Value v, Path p) -> LogicalResult {
      auto key = std::make_pair(v, p);
      if (llvm::is_contained(seen, key))
        return success();
      seen.push_back(key);
      if (auto arg = dyn_cast<BlockArgument>(v)) {
        unsigned i = arg.getArgNumber();
        if (ordinal && !i)
          return success();
        unsigned operand = ordinal ? i - 1 : i;
        auto text = emit(s, operands[operand], p);
        if (failed(text))
          return failure();
        inputs.push_back({i, p, *text});
        return success();
      }
      auto deps = ctx.analysis.getDependencies(v, p, s.bindings);
      if (failed(deps))
        return failure();
      for (auto d : *deps)
        if (failed(visit(d.value, d.path)))
          return failure();
      return success();
    };
    if (failed(visit(yield, path)))
      return failure();
    return inputs;
  }
  FailureOr<std::string> tableOperation(Scope &s, Value v, Path path) {
    auto *op = v.getDefiningOp();
    auto result = declarePlane(s, v, path);
    if (failed(result))
      return failure();
    auto e = fieldType(v.getType(), path, s, op);
    auto p = failed(e) ? FailureOr<std::string>(failure()) : payload(*e, s, op);
    if (failed(p))
      return failure();
    auto resultCount = planeCount(v.getType(), s, op);
    if (failed(resultCount))
      return failure();
    if (auto create = dyn_cast<ac::TableCreateOp>(op)) {
      for (auto [i, input] : llvm::enumerate(create.getInputs())) {
        auto text = emit(s, input, path);
        if (failed(text))
          return failure();
        out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
            << "; ++pyc_lane) " << *result << ".element(pyc_lane * "
            << create.getInputs().size() << " + " << i << ") = " << *text
            << ".element(pyc_lane);\n";
      }
      return *result;
    }
    if (auto splat = dyn_cast<ac::TableSplatOp>(op)) {
      auto input = emit(s, splat.getInput(), path);
      auto in = cardinality(splat.getInput().getType(), s, op),
           n = cardinality(v.getType(), s, op);
      if (failed(input) || failed(in) || failed(n))
        return failure();
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << *resultCount
          << "; ++pyc_lane) " << *result << ".element(pyc_lane) = " << *input
          << ".element((pyc_lane / (" << *n << ")) * (" << *in
          << ") + pyc_lane % (" << *in << "));\n";
      return *result;
    }
    if (auto view = dyn_cast<ac::TableViewOp>(op)) {
      auto input = emit(s, view.getInput(), path);
      auto in = cardinality(view.getInput().getType(), s, op),
           n = cardinality(v.getType(), s, op);
      if (failed(input) || failed(in) || failed(n))
        return failure();
      auto index =
          ctx.viewIndex(view, "(pyc_lane % (" + *n + "))", false, s.bindings);
      if (failed(index))
        return failure();
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << *resultCount
          << "; ++pyc_lane) " << *result << ".element(pyc_lane) = " << *input
          << ".element((pyc_lane / (" << *n << ")) * (" << *in << ") + ("
          << *index << "));\n";
      return *result;
    }
    if (auto map = dyn_cast<ac::TableMapOp>(op)) {
      unsigned output = cast<OpResult>(v).getResultNumber();
      auto yielded =
          cast<ac::YieldOp>(map.getBody().front().back()).getValues()[output];
      auto inputs = regionInputs(s, map.getBody(), yielded, path,
                                 map.getOperands(), true);
      auto n = ctx.shapeSize(map.getShape(), op, false, s.bindings);
      auto ordinalWidth =
          width(map.getBody().front().getArgument(0).getType(), s, op);
      if (failed(inputs) || failed(n) || failed(ordinalWidth))
        return failure();
      out << "    for (std::size_t pyc_element = 0; pyc_element < "
          << *resultCount << "; ++pyc_element) {\n";
      auto expr = regionExpression(
          s, map.getBody(), yielded, path,
          [&](BlockArgument a, Path q) -> FailureOr<std::string> {
            if (!a.getArgNumber())
              return "gfsim::FourState<" + *ordinalWidth +
                     ">::known(gfsim::Bits<" + *ordinalWidth +
                     ">{pyc_element % (" + *n + ")})";
            for (auto &input : *inputs)
              if (input.argument == a.getArgNumber() && input.path == q) {
                auto i = input.argument <= map.getTables().size()
                             ? "pyc_element"
                             : "(pyc_element / (" + *n + "))";
                return input.plane + ".element(" + i + ").packed()";
              }
            return failure();
          });
      if (failed(expr))
        return failure();
      out << "      " << *result << ".element(pyc_element) = gfsim::wire<" << *p
          << ">::fromPacked(" << *expr << ");\n    }\n";
      return *result;
    }
    if (auto match = dyn_cast<ac::TableMatchOp>(op)) {
      auto yielded =
          cast<ac::YieldOp>(match.getBody().front().back()).getValues()[0];
      auto inputs = regionInputs(s, match.getBody(), yielded, {},
                                 match.getOperands(), false);
      auto n = cardinality(match.getInput().getType(), s, op);
      if (failed(inputs) || failed(n))
        return failure();
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
          << "; ++pyc_lane) {\n      " << *result
          << ".element(pyc_lane) = gfsim::wire<" << *p << ">::known(" << *p
          << "{0});\n      for (std::size_t pyc_element = 0; pyc_element < "
          << *n << "; ++pyc_element) {\n";
      auto expr = regionExpression(
          s, match.getBody(), yielded, {},
          [&](BlockArgument a, Path q) -> FailureOr<std::string> {
            for (auto &input : *inputs)
              if (input.argument == a.getArgNumber() && input.path == q)
                return input.plane + ".element(" +
                       (a.getArgNumber()
                            ? "pyc_lane"
                            : "(pyc_lane * (" + *n + ") + pyc_element)") +
                       ").packed()";
            return failure();
          });
      if (failed(expr))
        return failure();
      out << "        " << *result
          << ".element(pyc_lane).assignSlice(pyc_element, " << *expr
          << ");\n      }\n    }\n";
      return *result;
    }
    if (auto get = dyn_cast<ac::TableGetOp>(op)) {
      auto index = emit(s, get.getIndex());
      auto n = cardinality(get.getInput().getType(), s, op);
      auto iw = width(get.getIndex().getType(), s, op);
      if (failed(index) || failed(n) || failed(iw))
        return failure();
      std::string compareWidth =
          "std::max<unsigned>(" + *iw +
          ", std::bit_width(static_cast<std::uint64_t>(" + *n + ")))";
      auto range =
          "gfsim::compare<gfsim::ComparePredicate::ULT>(gfsim::zero_extend<" +
          compareWidth + ">(" + *index +
          ".element(pyc_lane).packed()), gfsim::FourState<" + compareWidth +
          ">::known(gfsim::Bits<" + compareWidth + ">{" + *n + "}))";
      if (cast<OpResult>(v).getResultNumber()) {
        out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
            << "; ++pyc_lane) " << *result
            << ".element(pyc_lane) = gfsim::wire<" << *p << ">::fromPacked("
            << range << ");\n";
        return *result;
      }
      auto input = emit(s, get.getInput(), path);
      if (failed(input))
        return failure();
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
          << "; ++pyc_lane) {\n      auto pyc_range = " << range << ";\n      "
          << *result << ".element(pyc_lane) = gfsim::wire<" << *p
          << ">::unknown();\n      if (pyc_range.isFullyKnown() && "
             "pyc_range.value().toBool()) "
          << *result << ".element(pyc_lane) = " << *input
          << ".element(pyc_lane * (" << *n << ") + " << *index
          << ".element(pyc_lane).packed().value().value());\n    }\n";
      return *result;
    }
    if (auto index = dyn_cast<ac::TableIndexOp>(op)) {
      auto n = ctx.shapeSize(index.getShape(), op, false, s.bindings);
      auto w = width(v.getType(), s, op);
      if (failed(n) || failed(w))
        return failure();
      SmallVector<std::string> coords, widths, extents;
      for (auto c : index.getCoords()) {
        auto text = emit(s, c);
        auto bits = width(c.getType(), s, op);
        if (failed(text) || failed(bits))
          return failure();
        coords.push_back(*text);
        widths.push_back(*bits);
      }
      for (auto raw : index.getShape()) {
        auto text =
            ctx.staticValue(cast<ac::StaticExprAttr>(raw), s.bindings, op);
        if (failed(text))
          return failure();
        extents.push_back(*text);
      }
      std::string sumWidth = "(std::max<unsigned>({" + join(widths, ", ") +
                             "}) + std::bit_width(static_cast<std::uint64_t>(" +
                             *n +
                             ")) + std::bit_width(static_cast<std::uint64_t>(" +
                             std::to_string(widths.size()) + ")))";
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
          << "; ++pyc_lane) {\n      auto pyc_bounds = "
             "gfsim::FourState<1>::known(gfsim::Bits<1>{1});\n      auto "
             "pyc_sum = gfsim::FourState<"
          << sumWidth << ">::known(gfsim::Bits<" << sumWidth << ">{0});\n";
      for (unsigned i = 0; i < coords.size(); ++i) {
        std::string cw = "std::max<unsigned>(" + widths[i] +
                         ", std::bit_width(static_cast<std::uint64_t>(" +
                         extents[i] + ")))";
        std::string stride = "1";
        for (unsigned j = i + 1; j < extents.size(); ++j)
          stride += " * (" + extents[j] + ")";
        out << "      pyc_bounds = gfsim::bit_and(pyc_bounds, "
               "gfsim::compare<gfsim::ComparePredicate::ULT>(gfsim::zero_"
               "extend<"
            << cw << ">(" << coords[i]
            << ".element(pyc_lane).packed()), gfsim::FourState<" << cw
            << ">::known(gfsim::Bits<" << cw << ">{" << extents[i]
            << "})));\n      pyc_sum = gfsim::add(pyc_sum, "
               "gfsim::mul(gfsim::zero_extend<"
            << sumWidth << ">(" << coords[i]
            << ".element(pyc_lane).packed()), gfsim::FourState<" << sumWidth
            << ">::known(gfsim::Bits<" << sumWidth << ">{" << stride
            << "})));\n";
      }
      out << "      " << *result << ".element(pyc_lane) = gfsim::wire<" << *p
          << ">::fromPacked(gfsim::select(pyc_bounds, gfsim::truncate<" << *w
          << ">(pyc_sum), gfsim::FourState<" << *w << ">::known(gfsim::Bits<"
          << *w << ">{" << *n << "})));\n    }\n";
      return *result;
    }
    if (auto choose = dyn_cast<ac::TableChooseOp>(op)) {
      auto mask = emit(s, choose.getMask());
      auto n = cardinality(choose.getInput().getType(), s, op);
      auto iw = width(choose.getResult(0).getType(), s, op);
      auto mw = width(choose.getMask().getType(), s, op);
      if (failed(mask) || failed(n) || failed(iw) || failed(mw))
        return failure();
      SmallVector<std::string> results;
      for (Value output : choose.getResults()) {
        if (output == v)
          results.push_back(*result);
        else {
          auto text = declarePlane(s, output);
          if (failed(text))
            return failure();
          results.push_back(*text);
        }
        memoize(s, output, {}, results.back());
      }
      auto c = choose.getCount();
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
          << "; ++pyc_lane) {\n      auto pyc_remaining = " << *mask
          << ".element(pyc_lane).packed();\n      "
             "std::array<gfsim::wire<gfsim::Bits<"
          << *iw << ">>, " << c
          << "> pyc_indices;\n      std::array<gfsim::wire<gfsim::Bits<1>>, "
          << c
          << "> pyc_valids;\n      for (std::size_t pyc_rank = 0; pyc_rank < "
          << c << "; ++pyc_rank) {\n        auto pyc_index = gfsim::FourState<"
          << *iw << ">::known(gfsim::Bits<" << *iw
          << ">{0});\n        auto pyc_valid = "
             "gfsim::FourState<1>::known(gfsim::Bits<1>{0});\n";
      if (choose.getOrder() == "low")
        out << "        for (std::size_t pyc_bit = " << *n
            << "; pyc_bit-- > 0;) {\n";
      else
        out << "        for (std::size_t pyc_bit = 0; pyc_bit < " << *n
            << "; ++pyc_bit) {\n";
      out << "          auto pyc_hit = gfsim::extract<1>(pyc_remaining, "
             "pyc_bit);\n          pyc_valid = gfsim::bit_or(pyc_valid, "
             "pyc_hit);\n          pyc_index = gfsim::select(pyc_hit, "
             "gfsim::FourState<"
          << *iw << ">::known(gfsim::Bits<" << *iw
          << ">{pyc_bit}), pyc_index);\n        }\n        "
             "pyc_indices[pyc_rank] = gfsim::wire<gfsim::Bits<"
          << *iw
          << ">>::fromPacked(pyc_index); pyc_valids[pyc_rank] = "
             "gfsim::wire<gfsim::Bits<1>>::fromPacked(pyc_valid);\n        for "
             "(std::size_t pyc_bit = 0; pyc_bit < "
          << *n
          << "; ++pyc_bit) {\n          auto pyc_remove = "
             "gfsim::bit_and(pyc_valid, "
             "gfsim::compare<gfsim::ComparePredicate::EQ>(pyc_index, "
             "gfsim::FourState<"
          << *iw << ">::known(gfsim::Bits<" << *iw
          << ">{pyc_bit})));\n          pyc_remaining.assignSlice(pyc_bit, "
             "gfsim::bit_and(gfsim::extract<1>(pyc_remaining, pyc_bit), "
             "gfsim::bit_not(pyc_remove)));\n        }\n      }\n";
      for (unsigned i = 0; i < results.size(); ++i) {
        auto pt = payload(choose.getResult(i).getType(), s, op);
        if (failed(pt))
          return failure();
        out << "      " << results[i] << ".element(pyc_lane) = gfsim::wire<"
            << *pt << ">::fromPacked("
            << (i < uint64_t(c) ? "pyc_indices[" : "pyc_valids[")
            << (i % uint64_t(c)) << "].packed());\n";
      }
      out << "    }\n";
      return results[cast<OpResult>(v).getResultNumber()];
    }
    if (auto fold = dyn_cast<ac::TableFoldOp>(op)) {
      auto input = emit(s, fold.getInput());
      auto n = cardinality(fold.getInput().getType(), s, op);
      auto w = width(v.getType(), s, op);
      if (failed(input) || failed(n) || failed(w))
        return failure();
      std::string combine;
      if (fold.getKind() == "min" || fold.getKind() == "max")
        combine = "gfsim::select(gfsim::compare<gfsim::ComparePredicate::" +
                  std::string(fold.getKind() == "min" ? "ULE" : "UGE") +
                  ">(pyc_left, pyc_right), pyc_left, pyc_right)";
      else {
        auto kind = llvm::StringSwitch<StringRef>(fold.getKind())
                        .Case("and", "bit_and")
                        .Case("or", "bit_or")
                        .Case("xor", "bit_xor")
                        .Default(fold.getKind());
        combine = "gfsim::" + kind.str() + "(pyc_left, pyc_right)";
      }
      out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << s.count
          << "; ++pyc_lane) {\n      std::array<gfsim::wire<gfsim::Bits<" << *w
          << ">>, " << *n
          << "> pyc_tree;\n      for (std::size_t pyc_element = 0; pyc_element "
             "< "
          << *n << "; ++pyc_element) pyc_tree[pyc_element] = " << *input
          << ".element(pyc_lane * (" << *n
          << ") + pyc_element);\n      for (std::size_t pyc_nodes = " << *n
          << "; pyc_nodes > 1; pyc_nodes = (pyc_nodes + 1) / 2) {\n        for "
             "(std::size_t pyc_pair = 0; pyc_pair < pyc_nodes / 2; ++pyc_pair) "
             "{\n          auto pyc_left = pyc_tree[2 * pyc_pair].packed(), "
             "pyc_right = "
             "pyc_tree[2 * pyc_pair + 1].packed();\n          "
             "pyc_tree[pyc_pair] = gfsim::wire<"
          << *p << ">::fromPacked(" << combine
          << ");\n        }\n        if (pyc_nodes % 2) pyc_tree[pyc_nodes / "
             "2] "
             "= pyc_tree[pyc_nodes - 1];\n      }\n      "
          << *result << ".element(pyc_lane) = gfsim::wire<" << *p
          << ">::fromPacked(pyc_tree[0].packed());\n    }\n";
      return *result;
    }
    return op->emitOpError() << "operation lacks table C++ emission";
  }
  LogicalResult outputs(Scope &s) {
    auto yield = cast<ac::YieldOp>(s.definition.getBody().front().back());
    for (auto [port, value] : llvm::enumerate(yield.getValues())) {
      auto text = emit(s, value);
      auto name =
          id(cast<StringAttr>(s.definition.getOutputNames()[port]).getValue(),
             s.definition);
      auto count = planeCount(value.getType(), s, s.definition);
      if (failed(text) || failed(name) || failed(count))
        return failure();
      out << "    for (std::size_t pyc_pin = 0; pyc_pin < " << *count
          << "; ++pyc_pin) " << s.object << *name
          << ".element(pyc_pin) = " << *text << ".element(pyc_pin);\n";
    }
    return success();
  }
  HardwareEmitContext &ctx;
  raw_ostream &out;
  bool resetting;
  bool tasksFinished = false;
  SmallVector<Operation *> delegated;
  std::deque<Scope> scopes;
  SmallVector<Leaf> storage;
  SmallVector<Node> memo, active;
  unsigned nextName = 0;
};
struct ChildInfo {
  Operation *op;
  ac::HardwareBindings bindings;
  std::string name, count, type;
  bool leaf;
  // Position of this occurrence among the definition's `ac.instance` and
  // `ac.collection` children, which is the order the observation plan uses.
  unsigned observationIndex;
};
FailureOr<SmallVector<ChildInfo>> childInfo(HardwareEmitContext &ctx,
                                            ac::ModuleOp definition) {
  SmallVector<ChildInfo> result;
  unsigned observationIndex = 0;
  ac::HardwareBindings parent;
  parent.owner = definition;
  for (Operation &op : definition.getBody().front())
    if (isa<ac::InstanceOp, ac::CollectionOp, ac::QueueOp>(op)) {
      bool observed = isa<ac::InstanceOp, ac::CollectionOp>(op);
      auto name = legalizeIdentifier(
          op.getAttrOfType<StringAttr>("instance_name").getValue(),
          [&] { return op.emitOpError(); });
      auto b =
          isa<ac::QueueOp>(op) ? FailureOr<ac::HardwareBindings>(parent)
          : isa<ac::InstanceOp>(op)
              ? ctx.analysis.bindInstance(cast<ac::InstanceOp>(op), parent)
              : ctx.analysis.bindInstance(cast<ac::CollectionOp>(op), parent);
      if (failed(name) || failed(b))
        return failure();
      std::string count = "pyc_count";
      if (auto c = dyn_cast<ac::CollectionOp>(op)) {
        auto n = ctx.shapeSize(c.getShape(), &op, false, parent);
        if (failed(n))
          return failure();
        count = "(pyc_count * (" + *n + "))";
      }
      bool leaf = isa<ac::QueueOp>(op) ||
                  !ctx.analysis.getPrimitiveKind(b->owner).empty();
      auto type = isa<ac::QueueOp>(op)
                      ? queueKernelType(ctx, cast<ac::QueueOp>(op), *b)
                  : leaf ? ctx.cppBoundModuleType(b->owner, *b, &op, true)
                         : ctx.cppBoundFamilyType(b->owner, *b, &op, count);
      if (failed(type))
        return failure();
      result.push_back({&op, *b, ownerPrefix(&op).str() + *name, count, *type,
                        leaf, observed ? observationIndex++ : 0});
    }
  return result;
}
} // namespace
LogicalResult emitHardwareCppDefinition(HardwareEmitContext &ctx,
                                        ac::ModuleOp definition,
                                        raw_ostream &out) {
  const auto &names = ctx.names(definition);
  // Native observation plumbing is emitted only for a definition whose subtree
  // carries at least one `ac.observe` site, which keeps the generated code of
  // every unobserved design byte-identical.
  auto subtreeWidth = cppObservationSubtreeWidth(ctx, definition);
  if (failed(subtreeWidth))
    return failure();
  bool observes = *subtreeWidth != 0;
  auto laneStride = cppObservationLaneStride(ctx, definition);
  if (failed(laneStride))
    return failure();
  auto id = [&](StringRef raw) {
    return legalizeIdentifier(raw, [&] { return definition.emitOpError(); });
  };
  SmallVector<std::string> formals, actuals;
  for (auto raw : definition.getParameters()) {
    auto name = ctx.cppFormalName(
        cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue(),
        definition);
    if (failed(name))
      return failure();
    formals.push_back("std::int64_t " + *name);
    actuals.push_back(*name);
  }
  for (auto raw : definition.getTypeParameters()) {
    auto name = ctx.cppFormalName(cast<StringAttr>(raw).getValue(), definition);
    if (failed(name))
      return failure();
    formals.push_back("typename " + *name);
    actuals.push_back(*name);
  }
  auto children = childInfo(ctx, definition);
  if (failed(children))
    return failure();
  if (!names.cppNamespace.empty())
    out << "namespace " << names.cppNamespace << " {\n";
  out << "template <std::size_t pyc_count"
      << (formals.empty() ? "" : ", " + join(formals, ", ")) << ">\nclass "
      << names.cppFamilyClass << " final {\npublic:\n  explicit "
      << names.cppFamilyClass
      << "(std::string instanceName, "
         "gfsim::WorkExecutor *executor = nullptr";
  if (observes)
    out << ", pyc_observation_sink *pyc_sink = nullptr, "
           "std::size_t pyc_observation_base = 0";
  out << ") : "
         "pyc_work_executor_(executor)";
  if (observes)
    out << ", pyc_sink_(pyc_sink), pyc_observation_base_(pyc_observation_base)";
  for (auto &child : *children) {
    out << ", ";
    if (child.leaf)
      out << child.name << "_state(std::make_shared<gfsim::collection_storage<"
          << child.type << ">>(" << child.count << "))";
    else {
      auto observedChild = cppObservationSubtreeWidth(
          ctx, cast<ac::ModuleOp>(child.bindings.owner));
      if (failed(observedChild))
        return failure();
      out << child.name << "(std::make_shared<" << child.type
          << ">(instanceName + \"." << child.name << "\", executor";
      if (*observedChild != 0) {
        auto units =
            cppObservationChildBase(ctx, definition, child.observationIndex);
        if (failed(units))
          return failure();
        out << ", pyc_sink, pyc_observation_base_ + pyc_count * (" << *units
            << ")";
      }
      out << "))";
    }
  }
  out << " {}\n  " << names.cppFamilyClass << "(const " << names.cppFamilyClass
      << " &) = delete;\n  " << names.cppFamilyClass << " &operator=(const "
      << names.cppFamilyClass << " &) = delete;\n";
  auto familyPorts = [&](TypeRange types, ArrayAttr names, StringRef prefix,
                         StringRef count, const ac::HardwareBindings &bindings,
                         Operation *site) -> LogicalResult {
    for (auto [type, raw] : llvm::zip(types, names)) {
      auto p = ctx.cppType(type, bindings, site);
      auto n = id(cast<StringAttr>(raw).getValue());
      if (failed(p) || failed(n))
        return failure();
      out << "  gfsim::wire<gfsim::table<" << *p << ", " << count << ">> "
          << prefix << *n << ";\n";
    }
    return success();
  };
  ac::HardwareBindings parent;
  parent.owner = definition;
  if (failed(familyPorts(definition.getFunctionType().getInputs(),
                         definition.getInputNames(), "", "pyc_count", parent,
                         definition)) ||
      failed(familyPorts(definition.getFunctionType().getResults(),
                         definition.getOutputNames(), "", "pyc_count", parent,
                         definition)))
    return failure();
  for (auto &child : *children) {
    if (!child.leaf) {
      out << "  std::shared_ptr<" << child.type << "> " << child.name << ";\n";
      continue;
    }
    out << "  std::shared_ptr<gfsim::collection_storage<" << child.type << ">> "
        << child.name << "_state;\n";
    if (auto queue = dyn_cast<ac::QueueOp>(child.op)) {
      Builder builder(definition.getContext());
      auto inputNames = builder.getStrArrayAttr(queuePinNames(false));
      auto outputNames = builder.getStrArrayAttr(queuePinNames(true));
      if (failed(familyPorts(queue->getOperandTypes(), inputNames,
                             child.name + "_", child.count, child.bindings,
                             queue)) ||
          failed(familyPorts(queue->getResultTypes(), outputNames,
                             child.name + "_", child.count, child.bindings,
                             queue)))
        return failure();
      auto token =
          ctx.cppType(queue.getInData().getType(), child.bindings, queue);
      if (failed(token))
        return failure();
      for (StringRef direction : {"input", "output"})
        out << "  std::array<gfsim::wire<" << *token << ">, " << child.count
            << "> " << child.name << "_" << direction << "_tokens;\n";
      continue;
    }
    auto sig = cast<FunctionType>(
        child.bindings.owner->getAttrOfType<TypeAttr>("function_type")
            .getValue());
    if (failed(familyPorts(
            sig.getInputs(),
            child.bindings.owner->getAttrOfType<ArrayAttr>("input_names"),
            child.name + "_", child.count, child.bindings, child.op)) ||
        failed(familyPorts(
            sig.getResults(),
            child.bindings.owner->getAttrOfType<ArrayAttr>("output_names"),
            child.name + "_", child.count, child.bindings, child.op)))
      return failure();
  }
  emitCppCheckStorage(ctx, definition, out);
  out << "  void __pyc_clear_check_snapshots() noexcept {\n";
  emitCppCheckClear(ctx, definition, out);
  for (auto &child : *children)
    if (!child.leaf)
      out << "    " << child.name << "->__pyc_clear_check_snapshots();\n";
  out << "  }\n";
  if (observes)
    out << "  // One staging entry per descriptor owned by this definition. A\n"
           "  // Gauge publishes the raw unsigned bit pattern; a one-bit "
           "Event\n"
           "  // keeps the source bool shape the runner protocol carries.\n"
           "  template <bool pyc_event_bool>\n"
           "  void __pyc_stage_observation(std::size_t pyc_lane, "
           "std::size_t pyc_site, std::uint64_t pyc_value, bool pyc_valid, "
           "bool pyc_path) noexcept {\n"
           "    if (pyc_sink_ == nullptr || pyc_sink_->slots == nullptr || "
           "pyc_sink_->system == nullptr) return;\n"
           "    pyc_sink_->slots->Stage(pyc_observation_base_ + pyc_lane * "
        << *laneStride
        << " + pyc_site, "
           "pyc_event_bool ? gfsim::SlotValue::Bool(pyc_value != 0) : "
           "gfsim::SlotValue::Unsigned(pyc_value), pyc_valid, pyc_path, "
           "pyc_sink_->system->cycle());\n"
           "  }\n";
  out << "  void Work() {\n    __pyc_clear_check_snapshots();\n    try "
         "{\n";
  WorkEmitter work(ctx, definition, out);
  if (failed(work.run()))
    return failure();
  out << "    } catch (...) { DiscardNext(); throw; }\n  }\n";
  for (StringRef method : {"Xfer", "DiscardNext"}) {
    out << "  void " << method << "() noexcept {\n";
    if (method == "DiscardNext")
      out << "    __pyc_clear_check_snapshots();\n";
    for (auto &child : *children) {
      if (!child.leaf) {
        out << "    " << child.name << "->" << method << "();\n";
        continue;
      }
      if (method == "DiscardNext")
        out << "    " << child.name << "_state->discard();\n";
      else {
        SmallVector<std::string> q;
        if (isa<ac::QueueOp>(child.op)) {
          q = {child.name + "_in_ready.element(pyc_lane)",
               child.name + "_out_valid.element(pyc_lane)",
               child.name + "_output_tokens[pyc_lane]"};
        } else
          for (auto raw :
               child.bindings.owner->getAttrOfType<ArrayAttr>("output_names")) {
            auto n = id(cast<StringAttr>(raw).getValue());
            if (failed(n))
              return failure();
            q.push_back(child.name + "_" + *n + ".element(pyc_lane)");
          }
        auto kind = ctx.analysis.getPrimitiveKind(child.bindings.owner);
        std::string result;
        if (kind == "sync_mem" || kind == "sync_mem_dp") {
          for (auto &pin : q)
            pin = "&" + pin;
          result = "{{" + join(q, ", ") + "}}";
        } else
          result = "{" + join(q, ", ") + "}";
        out << "    " << child.name
            << "_state->xfer([&](std::size_t pyc_lane) noexcept { return "
               "typename "
            << child.type << "::Outputs" << result << "; });\n";
      }
    }
    if (method == "Xfer")
      out << "    __pyc_clear_check_snapshots();\n";
    out << "  }\n";
  }
  out << "  void Reset() noexcept {\n    __pyc_clear_check_snapshots();\n";
  WorkEmitter reset(ctx, definition, out, true);
  if (failed(reset.run()))
    return failure();
  out << "  }\nprivate:\n  gfsim::WorkExecutor *pyc_work_executor_;\n";
  if (observes)
    out << "  pyc_observation_sink *pyc_sink_ = nullptr;\n"
           "  std::size_t pyc_observation_base_ = 0;\n";
  out << "};\n";
  if (!formals.empty())
    out << "template <" << join(formals, ", ") << ">\n";
  auto family = names.cppFamilyClass + "<1" +
                (actuals.empty() ? "" : ", " + join(actuals, ", ")) + ">";
  out << "class " << names.cppClass
      << " final : public gfsim::SimModule {\npublic:\n  explicit "
      << names.cppClass
      << "(std::string instanceName, gfsim::WorkExecutor *executor = nullptr";
  if (observes)
    out << ", pyc_observation_sink *pyc_sink = nullptr, "
           "std::size_t pyc_observation_base = 0";
  out << ") "
         ": gfsim::SimModule(instanceName), "
         "pyc_implementation(std::make_shared<"
      << family << ">(std::move(instanceName), executor";
  if (observes)
    out << ", pyc_sink, pyc_observation_base";
  out << ")) {\n";
  if (cppHasRootChecks(ctx, definition) &&
      failed(emitCppCheckInstanceNames(ctx, out)))
    return failure();
  out << "  }\n";
  out << "  " << names.cppClass << "(const " << names.cppClass
      << " &) = delete;\n  " << names.cppClass << " &operator=(const "
      << names.cppClass << " &) = delete;\n";
  auto scalarPorts = [&](TypeRange types,
                         ArrayAttr portNames) -> LogicalResult {
    for (auto [t, raw] : llvm::zip(types, portNames)) {
      auto p = ctx.cppType(t, definition);
      auto n = id(cast<StringAttr>(raw).getValue());
      if (failed(p) || failed(n))
        return failure();
      out << "  gfsim::wire<" << *p << "> " << *n << ";\n";
    }
    return success();
  };
  if (failed(scalarPorts(definition.getFunctionType().getInputs(),
                         definition.getInputNames())) ||
      failed(scalarPorts(definition.getFunctionType().getResults(),
                         definition.getOutputNames())))
    return failure();
  auto bridge = [&](bool input) -> LogicalResult {
    auto types = input ? definition.getFunctionType().getInputs()
                       : definition.getFunctionType().getResults();
    auto portNames =
        input ? definition.getInputNames() : definition.getOutputNames();
    for (auto [t, raw] : llvm::zip(types, portNames)) {
      auto n = id(cast<StringAttr>(raw).getValue());
      if (failed(n))
        return failure();
      if (isa<ac::TableType>(t)) {
        out << "    for (std::size_t pyc_lane = 0; pyc_lane < decltype(" << *n
            << ")::size; ++pyc_lane) ";
        if (input)
          out << "pyc_implementation->" << *n << ".element(pyc_lane) = " << *n
              << ".element(pyc_lane);\n";
        else
          out << *n << ".element(pyc_lane) = pyc_implementation->" << *n
              << ".element(pyc_lane);\n";
      } else if (input)
        out << "    pyc_implementation->" << *n << ".element(0) = " << *n
            << ";\n";
      else
        out << "    " << *n << " = pyc_implementation->" << *n
            << ".element(0);\n";
    }
    return success();
  };
  out << "  void Work() override {\n";
  if (failed(bridge(true)))
    return failure();
  out << "    pyc_implementation->Work();\n";
  if (failed(bridge(false)))
    return failure();
  out << "  }\n  void Xfer() noexcept override { pyc_implementation->Xfer(); "
         "}\n  "
         "void DiscardNext() noexcept override { "
         "pyc_implementation->DiscardNext(); }\n  void Reset() noexcept "
         "override "
         "{\n";
  if (failed(bridge(true)))
    return failure();
  out << "    pyc_implementation->Reset();\n  }\n  bool HasWork() const "
         "noexcept "
         "override { return true; }\n";
  if (cppHasRootChecks(ctx, definition) &&
      failed(emitCppRootCheckValidator(ctx, out)))
    return failure();
  out << "private:\n";
  if (cppHasRootChecks(ctx, definition))
    out << "  std::array<std::string, " << ctx.sourceChecks().checks.size()
        << "> pyc_check_instances_;\n";
  out << "  std::shared_ptr<" << family << "> pyc_implementation;\n};\n";
  if (!names.cppNamespace.empty())
    out << "} // namespace " << names.cppNamespace << "\n";
  return success();
}
} // namespace acir::compiler

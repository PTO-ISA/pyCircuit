#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
using namespace mlir;
namespace acir::ac {
mlir::FailureOr<Value> getVerifiedTransactionResetMask(RuleOp rule) {
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  if (!row || !row.get("reset_masked")) return Value();
  auto reject = [&]() { return rule.emitOpError("invalid common transaction reset mask"); };
  auto masked = row.getAs<BoolAttr>("reset_masked");
  auto resets = row.getAs<ArrayAttr>("resets");
  auto fire = row.getAs<IntegerAttr>("fire_result");
  if (!masked || !masked.getValue() || !resets || resets.empty() || !fire ||
      !fire.getType().isSignlessInteger(64) || fire.getInt() < 0 ||
      !rule.getBody().hasOneBlock() || rule.getBody().front().empty()) return reject();
  auto &block = rule.getBody().front();
  auto yield = dyn_cast<YieldOp>(block.back());
  if (!yield || uint64_t(fire.getInt()) >= yield->getNumOperands()) return reject();
  auto gate = yield->getOperand(fire.getInt()).getDefiningOp<BitsBinaryOp>();
  if (!gate || gate->getBlock() != &block || gate.getOpcode() != "and") return reject();
  Value mask = gate.getRhs(), cursor = mask;
  SmallVector<Value> leaves(resets.size());
  for (unsigned i = resets.size(); i > 1; --i) {
    auto join = cursor.getDefiningOp<BitsBinaryOp>();
    if (!join || join->getBlock() != &block || join.getOpcode() != "and") return reject();
    leaves[i - 1] = join.getRhs(); cursor = join.getLhs();
  }
  leaves[0] = cursor;
  for (auto [raw, leaf] : llvm::zip(resets, leaves)) {
    auto reset = dyn_cast<DictionaryAttr>(raw);
    auto capture = reset ? reset.getAs<IntegerAttr>("capture") : IntegerAttr();
    auto polarity = reset ? reset.getAs<StringAttr>("polarity") : StringAttr();
    auto kind = reset ? reset.getAs<StringAttr>("kind") : StringAttr();
    if (!reset || reset.size() != 3 || !capture || !capture.getType().isSignlessInteger(64) ||
        capture.getInt() < 0 || uint64_t(capture.getInt()) >= block.getNumArguments() ||
        !polarity || (polarity.getValue() != "high" && polarity.getValue() != "low") ||
        !kind || (kind.getValue() != "sync" && kind.getValue() != "async")) return reject();
    auto invert = leaf.getDefiningOp<BitsUnaryOp>();
    auto active = invert ? invert.getInput().getDefiningOp<BitsCompareOp>() : BitsCompareOp();
    if (!invert || invert->getBlock() != &block || invert.getOpcode() != "not" || !active || active->getBlock() != &block || active.getPredicate() != "case_eq" ||
        active.getLhs() != block.getArgument(capture.getInt())) return reject();
    auto constant = active.getRhs().getDefiningOp<BitsConstantOp>();
    auto tree = constant ? constant.getValue().getTree() : DictionaryAttr();
    auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
    auto number = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
    if (!constant || constant->getBlock() != &block || !tree || !tree.getAs<StringAttr>("kind") || tree.getAs<StringAttr>("kind").getValue() != "literal" ||
        !value || !value.getAs<StringAttr>("kind") || value.getAs<StringAttr>("kind").getValue() != "integer" ||
        !number || number.getCanonicalValue() != (polarity.getValue() == "high" ? "1" : "0")) return reject();
  }
  return mask;
}
namespace {
LogicalResult primitiveSchema(const HardwareAnalysis &analysis,
                              ModuleImportOp op) {
  StringRef kind = analysis.getPrimitiveKind(op);
  bool reg = kind == "dff" || kind == "dffe";
  if (op->hasAttr("ac.register_contract") && !reg)
    return op.emitOpError("register contract requires a DFF/DFFE import");
  if (kind.empty())
    return success();
  if (isCDCPrimitive(op))
    return verifyCDCPrimitiveDeclaration(analysis, op);
  unsigned registerVersion = 1;
  if (reg) {
    auto version = getRegisterContract(op);
    if (failed(version)) return failure();
    registerVersion = *version;
  }
  bool fifo = kind == "fifo_local" || kind == "fifo_downstream_pop";
  if (fifo && op.getSymName() != (Twine("pycircuit.__builtins__.") + kind).str())
    return op.emitOpError() << "trusted FIFO requires its canonical builtin symbol";
  SmallVector<StringRef> inputs, outputs;
  if (kind == "dff") {
    inputs = {"clk", "rst", "d", "init"};
    outputs = {"q"};
  } else if (kind == "dffe") {
    inputs = {"clk", "rst", "en", "d", "init"};
    outputs = {"q"};
  } else if (kind == "sync_mem") {
    inputs = {"clk",    "rst",   "ren",   "raddr",
              "wvalid", "waddr", "wdata", "wstrb"};
    outputs = {"rdata", "rlive"};
  } else if (kind == "sync_mem_dp") {
    inputs = {"clk",    "rst",    "ren0",  "raddr0", "ren1",
              "raddr1", "wvalid", "waddr", "wdata",  "wstrb"};
    outputs = {"rdata0", "rdata1", "rlive0", "rlive1"};
  } else if (kind == "byte_mem") {
    inputs = {"clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"};
    outputs = {"rdata"};
  } else if (fifo) {
    inputs = {"clk", "rst", "in_valid", "in_data", "out_ready"};
    outputs = {"ready", "valid", "data", "local_capacity"};
  } else
    return op.emitOpError() << "unknown trusted primitive kind";
  if (reg && registerVersion == 3) inputs.push_back("reset_value");
  if (op.getInputNames().size() != inputs.size() ||
      op.getOutputNames().size() != outputs.size())
    return op.emitOpError() << "trusted primitive pin count mismatch";
  for (auto [raw, name] : llvm::zip(op.getInputNames(), inputs))
    if (cast<StringAttr>(raw).getValue() != name)
      return op.emitOpError() << "trusted primitive input pin schema mismatch";
  for (auto [raw, name] : llvm::zip(op.getOutputNames(), outputs))
    if (cast<StringAttr>(raw).getValue() != name)
      return op.emitOpError() << "trusted primitive output pin schema mismatch";
  if (op.getTypeParameters().size() != 1 ||
      cast<StringAttr>(op.getTypeParameters()[0]).getValue() != "T")
    return op.emitOpError()
           << "trusted primitive requires payload type formal T";
  bool memory = kind == "sync_mem" || kind == "sync_mem_dp" || kind == "byte_mem";
  if (op.getParameters().size() != (memory || fifo ? 2 : 0))
    return op.emitOpError()
           << "trusted primitive integer parameter schema mismatch";
  constexpr StringLiteral memoryParameters[] = {"ADDR_WIDTH", "DEPTH"};
  if (memory)
    for (auto [raw, name] : llvm::zip(op.getParameters(), memoryParameters))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() !=
          name)
        return op.emitOpError()
               << "trusted memory requires ADDR_WIDTH, DEPTH parameters";
  constexpr StringLiteral fifoParameters[] = {"DEPTH", "AVAILABILITY_LATENCY"};
  if (fifo)
    for (auto [raw, name] : llvm::zip(op.getParameters(), fifoParameters))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() != name)
        return op.emitOpError()
               << "trusted FIFO requires DEPTH, AVAILABILITY_LATENCY parameters";
  auto sig = op.getFunctionType();
  auto payload = TypeParamType::get(
      op.getContext(), FlatSymbolRefAttr::get(op.getContext(), op.getSymName()),
      StringAttr::get(op.getContext(), "T"));
  for (auto [index, name] : llvm::enumerate(inputs)) {
    Type type = sig.getInput(index);
    if (name == "d" || name == "init" || name == "reset_value" || name == "wdata" || name == "in_data") {
      if (type != payload)
        return op.emitOpError()
               << "trusted primitive data pins require payload T";
      continue;
    }
    auto bits = dyn_cast<BitsType>(type);
    if (!bits)
      return op.emitOpError()
             << "trusted primitive control/address/strobe pins require bits";
    if (name.contains("addr")) {
      auto tree = bits.getWidth().getTree();
      auto ref = tree.getAs<DictionaryAttr>("ref");
      if (!ref || !ref.getAs<StringAttr>("name") ||
          ref.getAs<StringAttr>("name").getValue() != "ADDR_WIDTH" ||
          ref.getAs<FlatSymbolRefAttr>("owner").getValue() != op.getSymName())
        return op.emitOpError()
               << "trusted memory address width must reference ADDR_WIDTH";
    } else if (name != "wstrb") {
      auto value = analysis.evaluateStatic(bits.getWidth(), {}, op);
      if (failed(value))
        return failure();
      auto integer = dyn_cast<MathIntAttr>(*value);
      if (!integer || integer.getCanonicalValue() != "1")
        return op.emitOpError()
               << "trusted primitive control pin must be one bit";
    }
  }
  for (auto [index, type] : llvm::enumerate(sig.getResults())) {
    bool memoryLive = (kind == "sync_mem" && index == 1) ||
                      (kind == "sync_mem_dp" && index >= 2);
    if ((fifo && index != 2) || memoryLive) {
      auto bits = dyn_cast<BitsType>(type);
      StringRef message = memoryLive
          ? "trusted synchronous memory live output must be one bit"
          : "trusted FIFO status output must be one bit";
      if (!bits)
        return op.emitOpError() << message;
      auto width = analysis.evaluateStatic(bits.getWidth(), {}, op);
      auto integer = succeeded(width) ? dyn_cast<MathIntAttr>(*width) : MathIntAttr();
      if (!integer || integer.getCanonicalValue() != "1")
        return op.emitOpError() << message;
    } else if (type != payload)
      return op.emitOpError() << "trusted primitive output requires payload T";
  }
  auto deps = analysis.getImportDependencies(op, {});
  if (failed(deps))
    return failure();
  for (auto [index, dep] : llvm::enumerate(*deps)) {
    if (kind == "fifo_downstream_pop" && index == 0) {
      if (dep.inputs.size() != 1 || dep.inputs[0].port != 4 ||
          !dep.inputs[0].path.empty())
        return op.emitOpError() << "FIFO ready must depend only on out_ready";
    } else if (kind == "byte_mem") {
      if (dep.inputs.size() != 1 || dep.inputs[0].port != 2 ||
          !dep.inputs[0].path.empty())
        return op.emitOpError() << "byte_mem output must depend only on raddr";
    } else if (!dep.inputs.empty())
      return op.emitOpError()
             << "registered outputs must be temporal dependency cuts";
  }
  return success();
}
} // namespace
LogicalResult HardwareAnalysis::verifyImport(ModuleImportOp imported) const {
  if (failed(detail::verifyModuleImportSummarySelection(imported)))
    return failure();
  if (imported.getSemanticKind() != "hardware")
    return imported.emitOpError()
           << "hardware analysis cannot consume a transactions import";
  if (failed(primitiveSchema(*this, imported)))
    return failure();
  return success(succeeded(getImportDependencies(imported, {})));
}
LogicalResult HardwareAnalysis::verify() const {
  // The shared builder orders raw budget preflight, structural verification,
  // envelope/schema validation and compact closure analysis consistently.
  return success(succeeded(getSourceCheckPlan()));
}
} // namespace acir::ac

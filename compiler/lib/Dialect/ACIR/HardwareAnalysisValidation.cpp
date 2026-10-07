#include "HardwareSourceChecks.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult primitiveSchema(const HardwareAnalysis &analysis,
                              ModuleImportOp op) {
  StringRef kind = analysis.getPrimitiveKind(op);
  if (kind.empty())
    return success();
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
    outputs = {"rdata"};
  } else if (kind == "sync_mem_dp") {
    inputs = {"clk",    "rst",    "ren0",  "raddr0", "ren1",
              "raddr1", "wvalid", "waddr", "wdata",  "wstrb"};
    outputs = {"rdata0", "rdata1"};
  } else if (kind == "byte_mem") {
    inputs = {"clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"};
    outputs = {"rdata"};
  } else
    return op.emitOpError() << "unknown trusted primitive kind";
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
  bool memory = kind != "dff" && kind != "dffe";
  if (op.getParameters().size() != (memory ? 2 : 0))
    return op.emitOpError()
           << "trusted primitive integer parameter schema mismatch";
  constexpr StringLiteral memoryParameters[] = {"ADDR_WIDTH", "DEPTH"};
  if (memory)
    for (auto [raw, name] : llvm::zip(op.getParameters(), memoryParameters))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue() !=
          name)
        return op.emitOpError()
               << "trusted memory requires ADDR_WIDTH, DEPTH parameters";
  auto sig = op.getFunctionType();
  auto payload = TypeParamType::get(
      op.getContext(), FlatSymbolRefAttr::get(op.getContext(), op.getSymName()),
      StringAttr::get(op.getContext(), "T"));
  for (auto [index, name] : llvm::enumerate(inputs)) {
    Type type = sig.getInput(index);
    if (name == "d" || name == "init" || name == "wdata") {
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
  for (Type type : sig.getResults())
    if (type != payload)
      return op.emitOpError() << "trusted primitive output requires payload T";
  auto deps = analysis.getImportDependencies(op, {});
  if (failed(deps))
    return failure();
  for (auto &dep : *deps) {
    if (kind == "byte_mem") {
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
  if (failed(primitiveSchema(*this, imported)))
    return failure();
  return success(succeeded(getImportDependencies(imported, {})));
}
LogicalResult
detail::verifyHardwarePackageEnvelope(const HardwareAnalysis &analysis) {
  auto package = analysis.getPackage();
  if (!package)
    return failure();
  if (failed(collectFinalSourceUnits(package,
                                     [&] { return package.emitOpError(); })))
    return failure();
  for (Operation &op : package.getBody()->getOperations())
    if (!isa<ModuleOp, ModuleImportOp, StructOp, EnumOp, SystemOp>(op))
      return op.emitOpError()
             << "operation outside closed executable hardware package";
  for (auto record : analysis.getStructs())
    if (failed(analysis.getPackedWidth(
            StructType::get(record.getContext(), record.getSymNameAttr()), {},
            record)))
      return failure();
  for (auto enumeration : package.getOps<EnumOp>())
    if (failed(analysis.resolveEnum(EnumType::get(enumeration.getContext(),
                                                  enumeration.getSymNameAttr()),
                                    enumeration)))
      return failure();
  for (auto imported : package.getOps<ModuleImportOp>()) {
    if (failed(primitiveSchema(analysis, imported)))
      return failure();
    if (failed(analysis.getImportDependencies(imported, {})))
      return failure();
    if (analysis.getPrimitiveKind(imported).empty())
      return imported.emitOpError() << "executable hardware package has "
                                       "unresolved ordinary module import";
  }
  return success();
}
LogicalResult HardwareAnalysis::verify() const {
  // The shared builder orders raw budget preflight, structural verification,
  // envelope/schema validation and compact closure analysis consistently.
  return success(succeeded(getSourceCheckPlan()));
}
} // namespace acir::ac

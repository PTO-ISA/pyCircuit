#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>

using namespace mlir;

namespace acir::ac {
ArrayAttr serializeOutputDependencies(MLIRContext *context,
                                      ArrayRef<OutputDependency> dependencies) {
  Builder builder(context);
  auto endpoint = [&](const PortEndpoint &value) {
    SmallVector<Attribute> path(value.path.begin(), value.path.end());
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("port", builder.getI64IntegerAttr(value.port)),
         builder.getNamedAttr("path", builder.getArrayAttr(path))});
  };
  auto less = [](const PortEndpoint &left, const PortEndpoint &right) {
    if (left.port != right.port)
      return left.port < right.port;
    return std::lexicographical_compare(
        left.path.begin(), left.path.end(), right.path.begin(),
        right.path.end(),
        [](StringAttr a, StringAttr b) { return a.getValue() < b.getValue(); });
  };
  SmallVector<OutputDependency> sorted(dependencies.begin(),
                                       dependencies.end());
  llvm::sort(sorted, [&](const auto &a, const auto &b) {
    return less(a.output, b.output);
  });
  SmallVector<Attribute> rows;
  for (auto &dependency : sorted) {
    llvm::sort(dependency.inputs, less);
    SmallVector<Attribute> inputs;
    for (const auto &input : dependency.inputs)
      inputs.push_back(endpoint(input));
    rows.push_back(builder.getDictionaryAttr(
        {builder.getNamedAttr("output", endpoint(dependency.output)),
         builder.getNamedAttr("inputs", builder.getArrayAttr(inputs))}));
  }
  return builder.getArrayAttr(rows);
}
} // namespace acir::ac

#ifndef ACIR_LIB_DIALECT_ACIR_ANALYSISRESOLVER_H
#define ACIR_LIB_DIALECT_ACIR_ANALYSISRESOLVER_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

namespace acir::ac {
// The binding-aware resolver over one analysis instance: the shared
// packed-width core (HardwarePackedWidth.cpp) reaches the HardwareAnalysis
// members through it (r8 review). One core, two resolvers — the transactional
// closed-payload verifier supplies the closed counterpart.
class AnalysisResolver : public detail::PackedWidthResolver {
public:
  explicit AnalysisResolver(const HardwareAnalysis &analysis)
      : analysis(analysis) {}
  mlir::ModuleOp getPackage() const override { return analysis.getPackage(); }
  mlir::Operation *lookupDefinition(llvm::StringRef name) const override {
    return analysis.lookupDefinition(name);
  }
  StructOp lookupStruct(StructType type) const override {
    return analysis.lookupStruct(type);
  }
  mlir::FailureOr<EnumDefinitionView>
  resolveEnum(EnumType type, mlir::Operation *site) const override {
    return analysis.resolveEnum(type, site);
  }
  mlir::FailureOr<mlir::Attribute>
  evaluateStatic(StaticExprAttr expression, const HardwareBindings &bindings,
                 mlir::Operation *site) const override {
    return analysis.evaluateStatic(expression, bindings, site);
  }
  mlir::FailureOr<mlir::Type>
  resolveType(mlir::Type type, const HardwareBindings &bindings,
              mlir::Operation *site) const override {
    return analysis.resolveType(type, bindings, site);
  }
  mlir::FailureOr<uint64_t>
  getPackedWidth(mlir::Type type, const HardwareBindings &bindings,
                 mlir::Operation *site) const override {
    return analysis.getPackedWidth(type, bindings, site);
  }
  llvm::SmallVector<std::pair<mlir::Attribute, mlir::Operation *>> &
  activeStatic() const override {
    return analysis.activeStatic();
  }

private:
  const HardwareAnalysis &analysis;
};
} // namespace acir::ac
#endif

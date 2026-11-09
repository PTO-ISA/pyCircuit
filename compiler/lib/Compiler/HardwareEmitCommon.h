#ifndef PYCIRCUIT_HARDWARE_EMIT_COMMON_H
#define PYCIRCUIT_HARDWARE_EMIT_COMMON_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
namespace acir::compiler {
struct EmitNames {
  std::string rtl;
  std::string verilog;
};
struct ObservationEmissionSite {
  mlir::Operation *op = nullptr;
  unsigned valueIndex = 0;
  bool carriesValue = false;
  bool gauge = false;
  uint64_t registrationKey = 0, siteKey = 0;
};
struct ObservationEmissionChild {
  mlir::Operation *op = nullptr;
  ac::ModuleOp definition;
  uint64_t shape = 1;
};
struct ObservationEmissionOccurrence {
  ObservationEmissionSite site;
  uint64_t ordinal = 0;
  llvm::SmallVector<std::pair<mlir::Operation *, uint64_t>> path;
  llvm::SmallVector<std::pair<mlir::Operation *, uint64_t>> logicalOwnerPath;
};
struct ObservationEmissionPlan {
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<ObservationEmissionSite>>
      definitions;
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<ObservationEmissionChild>>
      children;
  llvm::DenseMap<mlir::Operation *, uint64_t> subtreeWidth;
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<uint64_t>> childBaseUnits;
  llvm::SmallVector<std::array<uint64_t, 5>> table;
  llvm::SmallVector<ObservationEmissionOccurrence> occurrences;
  uint64_t events = 0;
};
class HardwareEmitContext {
public:
  HardwareEmitContext(mlir::ModuleOp package, ac::HardwareAnalysis &analysis);
  mlir::LogicalResult prepare();
  bool isSystem();
  bool managesChecks();
  // Prepare the verified check lifecycle shared by generated systems and
  // checked module callers; each emitter retains its observation boundary.
  mlir::LogicalResult prepareNativeChecks();
  const ac::HardwareSourceCheckPlan &sourceChecks() const;
  const EmitNames &names(mlir::Operation *definition) const;
  // Build the full package observation plan once and memoize it for every
  // definition and the simulation-top emitter in a single emission run.
  mlir::FailureOr<const ObservationEmissionPlan *> observationPlan();
  mlir::FailureOr<std::string> rtlFormalName(llvm::StringRef name,
                                             mlir::Operation *site) const;
  mlir::FailureOr<std::string> rtlType(mlir::Type type,
                                       mlir::Operation *site) const;
  bool isClosedStatic(ac::StaticExprAttr expression,
                      const ac::HardwareBindings &bindings = {}) const;
  mlir::FailureOr<std::string> staticRtlValue(ac::StaticExprAttr expression,
                                              mlir::Operation *site) const;
  mlir::FailureOr<std::string>
  staticRtlValue(ac::StaticExprAttr expression,
                 const ac::HardwareBindings &bindings,
                 mlir::Operation *site) const;
  mlir::FailureOr<ac::HardwareBindings> rootBindings() const;
  mlir::FailureOr<std::string> rootRtlParameters() const;
  mlir::FailureOr<std::string>
  shapeSize(mlir::ArrayAttr shape, mlir::Operation *site, bool rtl = false,
            const ac::HardwareBindings &bindings = {}) const;
  mlir::FailureOr<std::string>
  payloadWidth(mlir::Type type, mlir::Operation *site, bool rtl = false,
               const ac::HardwareBindings &bindings = {}) const;
  mlir::FailureOr<std::string>
  unsignedStaticValue(ac::StaticExprAttr expression, mlir::Operation *site,
                      bool rtl = false,
                      const ac::HardwareBindings &bindings = {}) const;
  mlir::FailureOr<std::string>
  viewIndex(ac::TableViewOp view, llvm::StringRef ordinal, bool rtl = false,
            const ac::HardwareBindings &bindings = {}) const;
  mlir::ModuleOp package;
  ac::HardwareAnalysis &analysis;

private:
  mlir::LogicalResult prepareImpl(bool allowSourceChecks);
  std::optional<ac::HardwareSourceCheckPlan> sourceChecks_;
  llvm::DenseMap<mlir::Operation *, EmitNames> names_;
  mutable bool observationPlanPrepared_ = false;
  mutable std::optional<ObservationEmissionPlan> observationPlan_;
  // rtlType is a pure Type->string projection; cache successful results so
  // repeated nets/fields sharing a packed type skip re-rendering. Failures are
  // re-reported against their originating site and never cached.
  mutable llvm::DenseMap<mlir::Type, std::string> rtlTypeCache_;
};

mlir::FailureOr<ObservationEmissionPlan>
observationEmissionPlan(HardwareEmitContext &context);
mlir::FailureOr<std::string> emissionMetadataJson(mlir::Attribute value,
                                                  mlir::Operation *site);
mlir::FailureOr<std::string>
observationSiteMetadataJson(ac::SourceObserveOp observe);
mlir::LogicalResult emitHardwareVerilogDefinition(HardwareEmitContext &context,
                                                  ac::ModuleOp definition,
                                                  llvm::raw_ostream &out);
} // namespace acir::compiler
#endif

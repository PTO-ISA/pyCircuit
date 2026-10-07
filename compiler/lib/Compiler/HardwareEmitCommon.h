#ifndef PYCIRCUIT_HARDWARE_EMIT_COMMON_H
#define PYCIRCUIT_HARDWARE_EMIT_COMMON_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"
#include <array>
#include <cstdint>
#include <utility>
#include <functional>
#include <optional>
#include <string>
namespace acir::compiler {
struct EmitNames {
  std::string cppQualified;
  std::string cppClass;
  std::string cppNamespace;
  std::string cppFamilyClass;
  std::string cppFamilyQualified;
  std::string rtl;
  std::string header;
  std::string source;
  std::string verilog;
};
class HardwareEmitContext {
public:
  HardwareEmitContext(mlir::ModuleOp package, ac::HardwareAnalysis &analysis);
  mlir::LogicalResult prepare();
  bool isSystem();
  // Internal staged native gate; public emission remains closed until both
  // backends implement the verified source-check lifecycle.
  mlir::LogicalResult prepareNativeChecks();
  const ac::HardwareSourceCheckPlan &sourceChecks() const;
  const EmitNames &names(mlir::Operation *definition) const;
  mlir::FailureOr<std::string> cppFormalName(llvm::StringRef name,
                                             mlir::Operation *site) const;
  mlir::FailureOr<std::string> rtlFormalName(llvm::StringRef name,
                                             mlir::Operation *site) const;
  mlir::FailureOr<std::string> cppType(mlir::Type type,
                                       mlir::Operation *site) const;
  mlir::FailureOr<std::string> cppType(mlir::Type type,
                                       const ac::HardwareBindings &bindings,
                                       mlir::Operation *site) const;
  mlir::FailureOr<std::string> staticValue(ac::StaticExprAttr expression,
                                           const ac::HardwareBindings &bindings,
                                           mlir::Operation *site) const;
  mlir::FailureOr<std::string> rtlType(mlir::Type type,
                                       mlir::Operation *site) const;
  mlir::FailureOr<std::string> staticValue(ac::StaticExprAttr expression,
                                           mlir::Operation *site) const;
  bool isClosedStatic(ac::StaticExprAttr expression,
                      const ac::HardwareBindings &bindings = {}) const;
  mlir::FailureOr<std::string> staticRtlValue(ac::StaticExprAttr expression,
                                              mlir::Operation *site) const;
  mlir::FailureOr<ac::HardwareBindings> rootBindings() const;
  mlir::FailureOr<std::string> rootCppType() const;
  mlir::FailureOr<std::string> rootRtlParameters() const;
  mlir::FailureOr<std::string> cppInstanceType(ac::InstanceOp instance) const;
  mlir::FailureOr<std::string>
  cppBoundModuleType(mlir::Operation *definition,
                     const ac::HardwareBindings &bindings,
                     mlir::Operation *site, bool kernel = false) const;
  mlir::FailureOr<std::string>
  cppBoundFamilyType(mlir::Operation *definition,
                     const ac::HardwareBindings &bindings,
                     mlir::Operation *site, llvm::StringRef count) const;
  mlir::FailureOr<std::string>
  rtlInstanceParameters(ac::InstanceOp instance) const;
  mlir::FailureOr<std::string>
  rtlInstanceParameters(ac::CollectionOp collection) const;
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
  mlir::FailureOr<std::string>
  cppWireExpression(mlir::Value value, const ac::HardwareBindings &bindings,
                    mlir::Operation *site) const;
  mlir::ModuleOp package;
  ac::HardwareAnalysis &analysis;

private:
  mlir::LogicalResult prepareImpl(bool allowSourceChecks);
  std::optional<ac::HardwareSourceCheckPlan> sourceChecks_;
  llvm::DenseMap<mlir::Operation *, EmitNames> names_;
};
mlir::LogicalResult emitHardwareCppDefinition(HardwareEmitContext &context,
                                              ac::ModuleOp definition,
                                              llvm::raw_ostream &out);
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
};
struct ObservationEmissionPlan {
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<ObservationEmissionSite>> definitions;
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<ObservationEmissionChild>> children;
  llvm::DenseMap<mlir::Operation *, uint64_t> subtreeWidth;
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<uint64_t>> childBaseUnits;
  llvm::SmallVector<std::array<uint64_t, 5>> table;
  llvm::SmallVector<ObservationEmissionOccurrence> occurrences;
  uint64_t events = 0;
};
mlir::FailureOr<ObservationEmissionPlan>
observationEmissionPlan(HardwareEmitContext &context);
mlir::FailureOr<std::string> emissionMetadataJson(mlir::Attribute value,
                                               mlir::Operation *site);
mlir::FailureOr<std::string>
cppObservationRunnerMetadata(HardwareEmitContext &context);
mlir::FailureOr<std::string>
cppObservationDescriptorTable(HardwareEmitContext &context);
mlir::FailureOr<std::string>
cppObservationConfigureCall(HardwareEmitContext &context);
mlir::FailureOr<std::string>
cppObservationChildBase(HardwareEmitContext &context, ac::ModuleOp definition,
                        unsigned childIndex);
mlir::FailureOr<uint64_t>
cppObservationLaneStride(HardwareEmitContext &context,
                         ac::ModuleOp definition);
mlir::FailureOr<uint64_t>
cppObservationSubtreeWidth(HardwareEmitContext &context,
                           ac::ModuleOp definition);
mlir::LogicalResult cppObservationStaging(
    HardwareEmitContext &context, ac::ModuleOp definition,
    const ac::HardwareBindings &bindings, llvm::StringRef object,
    llvm::StringRef count,
    const std::function<mlir::FailureOr<std::string>(mlir::Value)> &value,
    llvm::raw_ostream &out);
mlir::LogicalResult emitHardwareVerilogDefinition(HardwareEmitContext &context,
                                                  ac::ModuleOp definition,
                                                  llvm::raw_ostream &out);
} // namespace acir::compiler
#endif

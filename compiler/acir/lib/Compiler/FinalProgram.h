#ifndef ACIR_LIB_COMPILER_FINALPROGRAM_H
#define ACIR_LIB_COMPILER_FINALPROGRAM_H

#include "CheckGraph.h"
#include "FinalNumeric.h"
#include "ObservationGraph.h"
#include "ProposalGraph.h"

#include <memory>
#include <optional>
#include <string>

namespace acir::compiler {

enum class FinalProgramState { AnalysisClosed, EmitReady };
struct FrozenRegControls;

class FinalProgram {
public:
  struct StateCarrierSnapshot {
    mlir::DictionaryAttr stateID;
    InstanceView *view = nullptr;
    mlir::DictionaryAttr owner;
    mlir::Value handle;
    mlir::Operation *declaration = nullptr;
    mlir::DictionaryAttr attributes;
    mlir::Type physicalType;
    unsigned width = 0;
    mlir::Attribute initializer;
    mlir::DictionaryAttr formalPort;
    size_t formalPortIndex = 0;
    mlir::Value clock;
    mlir::Value reset;
    llvm::SmallVector<mlir::Value, 2> controlPrefix;
    llvm::SmallVector<mlir::Type, 2> controlPrefixTypes;
    mlir::Block *controlPrefixBlock = nullptr;
    mlir::DictionaryAttr controlDescriptor;
    mlir::StringAttr domain;
  };
  struct StateAliasSnapshot {
    InstanceView *view = nullptr;
    mlir::Value handle;
    mlir::DictionaryAttr stateID;
    mlir::DictionaryAttr formalState;
  };
  struct FinalInstanceSnapshot {
    size_t ordinal = 0;
    InstanceView *view = nullptr;
    mlir::DictionaryAttr owner;
    mlir::FlatSymbolRefAttr definition;
    mlir::ArrayAttr staticArguments;
    ac::ModuleOp module;
    ac::InstanceOp placement;
    std::optional<size_t> parentOrdinal;
    llvm::SmallVector<size_t> childOrdinals;
    llvm::SmallVector<size_t> ownedStateOrdinals;
    llvm::SmallVector<size_t> ruleOrdinals;
    llvm::SmallVector<size_t> proposalContributionOrdinals;
    llvm::SmallVector<size_t> proposalStateOrdinals;
    llvm::SmallVector<size_t> checkOrdinals;
    llvm::SmallVector<size_t> observationOrdinals;
  };

  FinalProgram(FinalProgram &&) = default;
  FinalProgram &operator=(FinalProgram &&) = default;
  FinalProgram(const FinalProgram &) = delete;
  FinalProgram &operator=(const FinalProgram &) = delete;

  FinalProgramState state() const { return state_; }
  bool isEmitReady() const { return state_ == FinalProgramState::EmitReady; }
  const ModuleGraph &modules() const { return *modules_; }
  const CheckGraph &checks() const { return *checks_; }
  const ProposalGraph &proposals() const { return *proposals_; }
  const ObservationGraph &observations() const { return *observations_; }
  llvm::ArrayRef<StateCarrierSnapshot> stateCarriers() const {
    return stateCarrierSnapshot_;
  }
  llvm::ArrayRef<StateAliasSnapshot> stateAliases() const {
    return stateAliasSnapshot_;
  }
  llvm::ArrayRef<FinalInstanceSnapshot> instances() const {
    return instanceSnapshot_;
  }
  size_t rootInstanceOrdinal() const { return rootInstanceOrdinalSnapshot_; }
  llvm::ArrayRef<size_t> postOrderInstanceOrdinals() const {
    return postOrderInstanceOrdinalsSnapshot_;
  }
  llvm::ArrayRef<FinalNumericRuleSnapshot> numericRules() const {
    return numericRuleSnapshot_;
  }
  mlir::ModuleOp hardware() const {
    return finalHardware_ ? *finalHardware_ : mlir::ModuleOp();
  }
  mlir::DictionaryAttr resolveStateID(const InstanceView *view,
                                      mlir::Value handle) const {
    for (const StateAliasSnapshot &alias : stateAliasSnapshot_)
      if (alias.view == view && alias.handle == handle)
        return alias.stateID;
    return {};
  }

private:
  FinalProgram() = default;
  mlir::LogicalResult
  verifyOwnedSourceClosure(ac::detail::EmitError emitError) const;

  friend mlir::FailureOr<FinalProgram>
  buildFinalProgram(llvm::ArrayRef<SourceLinkUnit> units,
                    ac::detail::EmitError emitError);
  friend mlir::LogicalResult
  verifyFinalProgram(const FinalProgram &program,
                     ac::detail::EmitError emitError);
  friend mlir::FailureOr<FinalProgram>
  materializeFinalProgram(FinalProgram &&program,
                          ac::detail::EmitError emitError);
  friend mlir::FailureOr<FinalProgram>
  buildFinalProgramFromHardware(mlir::ModuleOp package,
                                ac::detail::EmitError emitError);
  friend mlir::LogicalResult freezeEmitReadySnapshots(
      FinalProgram &program,
      const llvm::DenseMap<mlir::Operation *, FrozenRegControls> &controls,
      ac::detail::EmitError emitError);

  struct ModuleSnapshot {
    InstanceView *view = nullptr;
    mlir::DictionaryAttr owner;
    mlir::FlatSymbolRefAttr definition;
    mlir::ArrayAttr staticArguments;
    ac::ModuleOp module;
    ac::InstanceOp placement;
    mlir::DictionaryAttr attributes;
    mlir::ArrayAttr ports;
    bool root = false;
  };
  struct ProposalContributionSnapshot {
    mlir::DictionaryAttr stateID;
    mlir::DictionaryAttr logicalType;
    mlir::Value data;
    mlir::Value enabled;
    mlir::Value value;
    mlir::Value valid;
    mlir::Value path;
    mlir::DictionaryAttr useID;
    mlir::DictionaryAttr sourceID;
    InstanceView *owner = nullptr;
    ac::RuleOp rule;
    ac::InstanceOp child;
    llvm::SmallVector<ProposalUseFact> sourceFacts;
    bool composition = false;
  };
  struct CommitSnapshot {
    mlir::DictionaryAttr stateID;
    CommitPairKind kind = CommitPairKind::Hold;
    mlir::Value data;
    mlir::Value enable;
    llvm::SmallVector<size_t> contributions;
    bool permitAlways = true;
  };
  struct CheckSnapshot {
    std::uint32_t stableOrdinal = 0;
    size_t requiredIndex = 0;
    InstanceView *owner = nullptr;
    mlir::DictionaryAttr ownerRef;
    ac::RuleOp rule;
    mlir::DictionaryAttr registration;
    mlir::DictionaryAttr checkID;
    mlir::StringAttr kind;
    mlir::DictionaryAttr location;
    ac::SourceExpectOp expect;
    mlir::DictionaryAttr attributes;
    mlir::Value condition;
    mlir::Value path;
  };
  struct ObservationSnapshot {
    std::uint32_t stableOrdinal = 0;
    size_t requiredIndex = 0;
    InstanceView *owner = nullptr;
    mlir::DictionaryAttr ownerRef;
    ac::RuleOp rule;
    mlir::DictionaryAttr registration;
    mlir::DictionaryAttr observationID;
    mlir::StringAttr kind;
    mlir::DictionaryAttr spec;
    mlir::ArrayAttr valueIDs;
    mlir::ArrayAttr valueConstraints;
    ac::SourceObserveOp observe;
    mlir::DictionaryAttr attributes;
    mlir::Value path;
    llvm::SmallVector<mlir::Value> values;
  };
  struct UnitSnapshot {
    mlir::ModuleOp unit;
    mlir::DictionaryAttr attributes;
  };

public:
  struct RuleSnapshot {
    InstanceView *owner = nullptr;
    ac::RuleOp rule;
    mlir::Block *moduleEntryBlock = nullptr;
    mlir::Block *ruleBodyBlock = nullptr;
    llvm::SmallVector<mlir::Value> inputs, targets, results, blockArguments;
    llvm::SmallVector<mlir::Type> inputTypes, outputTypes, resultTypes;
    llvm::SmallVector<mlir::Type> blockArgumentTypes;
    mlir::DictionaryAttr finalAttributes;
    llvm::SmallVector<mlir::DictionaryAttr> inputStateIDs, outputStateIDs;
  };
  struct OperationSnapshot {
    mlir::Operation *operation = nullptr;
    InstanceView *owner = nullptr;
    ac::RuleOp rule;
    std::string name;
    mlir::DictionaryAttr attributes;
    llvm::SmallVector<mlir::Value> operands;
    llvm::SmallVector<mlir::Type> resultTypes;
    mlir::Operation *parent = nullptr;
    mlir::Block *block = nullptr;
  };

private:
  FinalProgramState state_ = FinalProgramState::AnalysisClosed;
  llvm::SmallVector<SourceLinkUnit> units_;
  llvm::SmallVector<mlir::OwningOpRef<mlir::ModuleOp>> ownedUnits_;
  mlir::OwningOpRef<mlir::ModuleOp> finalHardware_;
  std::unique_ptr<SourceHeaderRegistry> registry_;
  std::unique_ptr<ModuleGraph> modules_;
  std::unique_ptr<CheckGraph> checks_;
  std::unique_ptr<ProposalGraph> proposals_;
  std::unique_ptr<ObservationGraph> observations_;
  llvm::SmallVector<ModuleSnapshot> moduleSnapshot_;
  llvm::SmallVector<ProposalContributionSnapshot> proposalContributionSnapshot_;
  llvm::SmallVector<StateCarrierSnapshot> stateCarrierSnapshot_;
  llvm::SmallVector<StateAliasSnapshot> stateAliasSnapshot_;
  llvm::SmallVector<FinalInstanceSnapshot, 0> instanceSnapshot_;
  size_t rootInstanceOrdinalSnapshot_ = 0;
  llvm::SmallVector<CommitSnapshot> commitSnapshot_;
  llvm::SmallVector<CheckSnapshot> checkSnapshot_;
  llvm::SmallVector<ObservationSnapshot> observationSnapshot_;
  llvm::SmallVector<UnitSnapshot> unitSnapshot_;
  llvm::SmallVector<FinalNumericRuleSnapshot, 0> numericRuleSnapshot_;
  llvm::SmallVector<RuleSnapshot, 0> ruleSnapshot_;
  llvm::SmallVector<OperationSnapshot, 0> expressionSnapshot_;
  llvm::SmallVector<OperationSnapshot, 0> placementSnapshot_;
  llvm::SmallVector<size_t> postOrderInstanceOrdinalsSnapshot_;
  bool globalPermitSnapshot_ = true;
};

mlir::FailureOr<FinalProgram>
buildFinalProgram(llvm::ArrayRef<SourceLinkUnit> units,
                  ac::detail::EmitError emitError);
mlir::LogicalResult verifyFinalProgram(const FinalProgram &program,
                                       ac::detail::EmitError emitError);
mlir::FailureOr<FinalProgram>
materializeFinalProgram(FinalProgram &&program,
                        ac::detail::EmitError emitError);
mlir::FailureOr<FinalProgram>
buildFinalProgramFromHardware(mlir::ModuleOp package,
                              ac::detail::EmitError emitError);
mlir::FailureOr<std::string> emitFinalCpp(const FinalProgram &program,
                                          ac::detail::EmitError emitError);
mlir::FailureOr<std::string> emitFinalVerilog(const FinalProgram &program,
                                              ac::detail::EmitError emitError);

// One verified Verilog emission, split by the C3 generated-file roles. `rtl` is
// the hardware artifact (module families plus the portless FinalModel top) and
// carries the `rtl` role. `runtimeGlue` is the simulation observation wrapper
// that instantiates the hardware top and carries the `runtime-glue` role. This
// is only a file-role boundary: `rtl + runtimeGlue` is byte-identical to what
// emitFinalVerilog returns for the same program.
struct FinalVerilogEmission {
  std::string rtl;
  std::string runtimeGlue;
};

mlir::FailureOr<FinalVerilogEmission>
emitFinalVerilogParts(const FinalProgram &program,
                      ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALPROGRAM_H

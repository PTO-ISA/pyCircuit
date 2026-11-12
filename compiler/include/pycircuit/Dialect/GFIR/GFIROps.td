#ifndef ACIR_DIALECT_GFIR_GFIROPS_TD
#define ACIR_DIALECT_GFIR_GFIROPS_TD

include "pycircuit/Dialect/GFIR/GFIRDialect.td"
include "pycircuit/Dialect/ACIR/ACIRTypes.td"
include "pycircuit/Interfaces/LogicalStateInterfaces.td"
include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class GFIR_Op<string mnemonic, list<Trait> traits = []>
    : Op<GFIR_Dialect, mnemonic, traits>;

// GFIR owns exactly these two routed logical-state operations. The native
// verifier proves local structure and placement only; publication,
// refinement and execution additionally require the staged package
// pipeline's bound checks (B1-boundary.md). State effects are never Pure.
def GFIR_StateOp : GFIR_Op<"state", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>,
    LogicalStateOpInterface,
    NativeOpTrait<"acir::LogicalStateContractTrait">]> {
  let summary = "Declare one routed logical state and its old-Q value";
  let description = [{
    The routed replacement of one authenticated source ac.state declaration,
    carrying that declaration's exact clock/reset/initial operands, one Q
    result, instance_name, occurrence and optional ac.payload_constraint.
    Source authentication proves the exact cross-operation correspondence
    before publication; parsing or a native local verifier alone never
    authorizes refinement or emission.
  }];
  let arguments = (ins ACIR_BitsType:$clk, ACIR_BitsType:$rst,
      ACIR_HardwareType:$initial, StrAttr:$instance_name,
      DictionaryAttr:$occurrence);
  let results = (outs ACIR_HardwareType:$q);
  let hasVerifier = 1;
  let extraClassDeclaration = [{
    ::mlir::DictionaryAttr getPayloadConstraint() {
      return (*this)->getAttrOfType<::mlir::DictionaryAttr>(
          "ac.payload_constraint");
    }
    ::mlir::Value getStateClock() { return getClk(); }
    ::mlir::Value getStateReset() { return getRst(); }
    ::mlir::Value getStateInitial() { return getInitial(); }
    ::mlir::Value getStateQ() { return getQ(); }
    ::mlir::StringAttr getStateInstanceName() {
      return (*this)->getAttrOfType<::mlir::StringAttr>("instance_name");
    }
    ::mlir::DictionaryAttr getStateOccurrence() {
      return (*this)->getAttrOfType<::mlir::DictionaryAttr>("occurrence");
    }
    ::mlir::DictionaryAttr getStatePayloadConstraint() {
      return getPayloadConstraint();
    }
  }];
}

def GFIR_StateUpdateOp : GFIR_Op<"state.update", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>,
    LogicalStateUpdateOpInterface,
    NativeOpTrait<"acir::LogicalStateContractTrait">]> {
  let summary = "Bind one routed logical state's candidate and write intent";
  let description = [{
    A declarative binding in the owning module graph of a routed package.
    state is the direct old-Q result of one same-block gfir.state
    declaration; candidate has exactly that payload type and intent resolves
    to one bit. Every routed logical state has exactly one update. This
    operation has no attributes, results or regions and is not a Work-time
    state mutation. False intent holds old Q; commit, reset and failure
    retain the package's verified transaction semantics. Hardware refinement
    removes the logical declaration and update together before final
    verification and emission.
  }];
  let arguments = (ins ACIR_HardwareType:$state,
      ACIR_HardwareType:$candidate, ACIR_BitsType:$intent);
  let hasVerifier = 1;
  let extraClassDeclaration = [{
    ::mlir::Value getUpdatedStateQ() { return getState(); }
    ::mlir::Value getStateCandidate() { return getCandidate(); }
    ::mlir::Value getStateWriteIntent() { return getIntent(); }
  }];
}

#endif // ACIR_DIALECT_GFIR_GFIROPS_TD

#ifndef ACIR_DIALECT_GFIR_GFIROPS_TD
#define ACIR_DIALECT_GFIR_GFIROPS_TD

include "pycircuit/Dialect/GFIR/GFIRDialect.td"
include "pycircuit/Dialect/ACIR/ACIRTypes.td"
include "pycircuit/Interfaces/LogicalStateInterfaces.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/RegionKindInterface.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class GFIR_Op<string mnemonic, list<Trait> traits = []>
    : Op<GFIR_Dialect, mnemonic, traits>;

def GFIR_ModuleOp : GFIR_Op<"module", [
    Symbol, SymbolTable, IsolatedFromAbove, SingleBlock,
    RegionKindInterface, HasOnlyGraphRegion]> {
  let summary = "Define a reusable structured module graph";
  let description = [{
    The function_type owns the ordered input and output types. Body arguments
    bind inputs and the final gfir.yield connects outputs. Static instances
    retain identity independently of their calls. Graph order is not an
    execution schedule; closed-package verification proves executable types,
    call bindings, value-operation admission and acyclic dependencies.
  }];
  let arguments = (ins StrAttr:$sym_name,
      TypeAttrOf<FunctionType>:$function_type, DictionaryAttr:$source_owner,
      StrArrayAttr:$input_names, StrArrayAttr:$output_names);
  let regions = (region AnyRegion:$body);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def GFIR_InstanceOp : GFIR_Op<"instance", [Symbol]> {
  let summary = "Declare a static child module instance";
  let arguments = (ins SymbolNameAttr:$sym_name, FlatSymbolRefAttr:$callee);
  let assemblyFormat = "$sym_name `of` $callee attr-dict";
  let hasVerifier = 1;
}

def GFIR_CallOp : GFIR_Op<"call", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Invoke a logical interface on an existing receiver";
  let arguments = (ins OptionalAttr<SymbolRefAttr>:$receiver,
      OptionalAttr<StrAttr>:$interface, Variadic<AnyType>:$inputs,
      OptionalAttr<ArrayAttr>:$bindings,
      OptionalAttr<TypeAttrOf<FunctionType>>:$binding_type);
  let results = (outs Variadic<AnyType>:$outputs);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
}

def GFIR_PrimitiveOp : GFIR_Op<"primitive", [Symbol]> {
  let summary = "Declare a versioned executable native module contract";
  let arguments = (ins SymbolNameAttr:$sym_name, StrAttr:$native_kind,
      I64Attr:$version);
  let assemblyFormat =
      "$sym_name `native` $native_kind `version` $version attr-dict";
  let hasVerifier = 1;
}

def GFIR_VarOp : GFIR_Op<"var", [Symbol,
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Declare persistent state owned by this module instance";
  let arguments = (ins SymbolNameAttr:$sym_name, AnyType:$initial);
  let assemblyFormat =
      "$sym_name `init` $initial attr-dict `:` type($initial)";
  let hasVerifier = 1;
}

def GFIR_ReadOp : GFIR_Op<"read", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Read committed state or the current transaction candidate";
  let arguments = (ins FlatSymbolRefAttr:$state);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$state attr-dict `:` type($value)";
  let hasVerifier = 1;
}

def GFIR_WriteOp : GFIR_Op<"write", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Update a transaction candidate, nested field or indexed row";
  let arguments = (ins FlatSymbolRefAttr:$state, AnyType:$value,
      Optional<AnyType>:$index, OptionalAttr<StrArrayAttr>:$fields);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
}

def GFIR_RuleOp : GFIR_Op<"rule", [Symbol, IsolatedFromAbove,
    SingleBlock, RecursiveMemoryEffects]> {
  let summary = "Register an ordered autonomous transaction";
  let arguments = (ins SymbolNameAttr:$sym_name,
      TypeAttrOf<FunctionType>:$function_type, Variadic<AnyType>:$captures);
  let regions = (region AnyRegion:$body);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def GFIR_ResultOp : GFIR_Op<"result"> {
  let summary = "Refer to a result of this module's autonomous Rule";
  let arguments = (ins FlatSymbolRefAttr:$rule, I64Attr:$ordinal);
  let results = (outs AnyType:$value);
  let assemblyFormat = "$rule `[` $ordinal `]` attr-dict `:` type($value)";
  let hasVerifier = 1;
}

def GFIR_MethodOp : GFIR_Op<"method", [Symbol, IsolatedFromAbove,
    SingleBlock, RecursiveMemoryEffects]> {
  let summary = "Define an ordered interface in its caller transaction";
  let arguments = (ins SymbolNameAttr:$sym_name,
      TypeAttrOf<FunctionType>:$function_type, OptionalAttr<DictionaryAttr>:$writable);
  let regions = (region AnyRegion:$body);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def GFIR_IfOp : GFIR_Op<"if", [SingleBlock, RecursiveMemoryEffects]> {
  let summary = "Select ordered values and transaction candidates";
  let arguments = (ins AnyType:$predicate);
  let results = (outs Variadic<AnyType>:$results);
  let regions = (region AnyRegion:$thenBody, AnyRegion:$elseBody);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def GFIR_LoopOp : GFIR_Op<"loop", [SingleBlock, RecursiveMemoryEffects]> {
  let summary = "Execute a fixed range with ordered carried values";
  let arguments = (ins I64Attr:$trip_count, Variadic<AnyType>:$initial);
  let results = (outs Variadic<AnyType>:$results);
  let regions = (region AnyRegion:$body);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def GFIR_StepOp : GFIR_Op<"step", [Terminator, MemoryEffects<[]>, AttrSizedOperandSegments]> {
  let summary = "Carry values to the next fixed-range iteration";
  let arguments = (ins Variadic<AnyType>:$values, Optional<AnyType>:$break_if);
  let assemblyFormat = "`(` $values `)` attr-dict `:` `(` type($values) `)` (`break_if` $break_if^ `:` type($break_if))?";
  let hasVerifier = 1;
}

def GFIR_OrdinalBitsOp : GFIR_Op<"ordinal_bits", [MemoryEffects<[]>]> {
  let summary = "Convert a bounded loop ordinal to unsigned Bits";
  let arguments = (ins I64:$ordinal);
  let results = (outs ACIR_BitsType:$value);
  let assemblyFormat = "$ordinal attr-dict `:` type($ordinal) `->` type($value)";
  let hasVerifier = 1;
}

def GFIR_GuardOp : GFIR_Op<"guard", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Require a known true predicate at this point in the transaction";
  let arguments = (ins AnyType:$predicate);
  let assemblyFormat = "$predicate attr-dict `:` type($predicate)";
  let hasVerifier = 1;
}

def GFIR_EffectOp : GFIR_Op<"effect", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Capture an ordered assertion or observation";
  let arguments = (ins Variadic<AnyType>:$values, StrAttr:$kind,
      OptionalAttr<StrAttr>:$message, OptionalAttr<DictionaryAttr>:$spec);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
}

def GFIR_YieldOp : GFIR_Op<"yield", [Terminator, MemoryEffects<[]>]> {
  let summary = "Return the outputs of a graph or ordered body";
  let arguments = (ins Variadic<AnyType>:$values);
  let assemblyFormat = "attr-dict ($values^ `:` type($values))?";
  let hasVerifier = 1;
}

def GFIR_RootOp : GFIR_Op<"root"> {
  let summary = "Select a closed structured GFIR entry definition";
  let arguments = (ins FlatSymbolRefAttr:$callee, StrAttr:$kind,
      OptionalAttr<DictionaryAttr>:$execution_policy,
      OptionalAttr<DictionaryAttr>:$system_samples);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
}

// Retained routed logical-state operations. The native
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

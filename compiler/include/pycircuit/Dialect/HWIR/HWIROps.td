#ifndef PYCIRCUIT_DIALECT_HWIR_HWIROPS_TD
#define PYCIRCUIT_DIALECT_HWIR_HWIROPS_TD

include "pycircuit/Dialect/HWIR/HWIRDialect.td"
include "pycircuit/Dialect/ACIR/ACIRTypes.td"
include "mlir/IR/CommonAttrConstraints.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/IR/RegionKindInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class HWIR_Op<string mnemonic, list<Trait> traits = []>
    : Op<HWIR_Dialect, mnemonic, traits>;

// ---------------------------------------------------------------------------
// Definitions and declarations
// ---------------------------------------------------------------------------

def HWIR_ModuleOp : HWIR_Op<"module",
    [Symbol, IsolatedFromAbove, SingleBlock, RegionKindInterface,
     HasOnlyGraphRegion]> {
  let summary = "Define one reusable hardware module and its graph";
  let description = [{
    The definition owns its port signature, parameter formals and hardware
    body. Block arguments are the input wires; the final hwir.yield binds the
    output wires in signature order. function_type is the only owner of port
    types and direction; the optional name arrays only label those positions.

    The body contains combinational computation, fixed child instances and
    retained checks/observations. It owns no independently scheduled object.
    Every instance of this symbol shares the definition and owns distinct
    wires and leaf storage; a definition is never an occurrence.
  }];
  let arguments = (ins StrAttr:$sym_name,
      OptionalAttr<DictionaryAttr>:$source_owner,
      OptionalAttr<DictArrayAttr>:$parameters,
      OptionalAttr<StrArrayAttr>:$type_parameters,
      TypeAttrOf<FunctionType>:$function_type,
      OptionalAttr<StrArrayAttr>:$input_names,
      OptionalAttr<StrArrayAttr>:$output_names);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

def HWIR_SystemOp : HWIR_Op<"system"> {
  let summary = "Select one verified hardware root and execution domain";
  let description = [{
    entry carries {callee, parameters, type_arguments} exactly as the selected
    occurrence specifies it. The callee resolves to one hwir.module definition
    in the same package. A package that declares a hardware contract has
    exactly one system; a fragment may omit it.
  }];
  let arguments = (ins DictionaryAttr:$entry, StrAttr:$domain);
  let hasVerifier = 1;
  let assemblyFormat = "$entry `domain` $domain attr-dict";
}

// ---------------------------------------------------------------------------
// Occurrences
// ---------------------------------------------------------------------------

def HWIR_InstanceOp : HWIR_Op<"instance"> {
  let summary = "Instantiate one module with typed SSA input and output wires";
  let description = [{
    The callee resolves to a hwir.module definition or a closed trusted
    import. Input operands bind the callee input positions; results are its
    output wires. The static parameter and type argument arrays must match the
    callee declaration.

    Every instance is distinct hardware, including independently owned leaf
    state. It is not a pure software call and must not be merged by value CSE.
    instance_name is unique within its parent module body. occurrence records
    source identity and never decides admission.
  }];
  let arguments = (ins Variadic<AnyType>:$inputs,
      StrAttr:$instance_name, FlatSymbolRefAttr:$callee,
      OptionalAttr<ArrayAttr>:$parameters,
      OptionalAttr<ArrayAttr>:$type_arguments,
      OptionalAttr<DictionaryAttr>:$occurrence);
  let results = (outs Variadic<AnyType>:$outputs);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

def HWIR_CollectionOp : HWIR_Op<"collection"> {
  let summary = "Compact family of independent instances of one definition";
  let description = [{
    A collection denotes one static family of independent occurrences sharing
    a definition, input lanes and result lanes. shape records the static
    dimensions; each element remains independent hardware. The callee and
    static bindings follow hwir.instance.
  }];
  let arguments = (ins Variadic<AnyType>:$inputs,
      StrAttr:$instance_name, FlatSymbolRefAttr:$callee,
      OptionalAttr<ArrayAttr>:$parameters,
      OptionalAttr<ArrayAttr>:$type_arguments,
      ArrayAttr:$shape,
      OptionalAttr<DictArrayAttr>:$layout,
      OptionalAttr<DictionaryAttr>:$occurrence);
  let results = (outs Variadic<AnyType>:$outputs);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

def HWIR_QueueOp : HWIR_Op<"queue"> {
  let summary = "Own one bounded FIFO of complete hardware tokens";
  let arguments = (ins ACIR_BitsType:$clk, ACIR_BitsType:$rst,
      ACIR_BitsType:$in_valid, ACIR_HardwareType:$in_data,
      ACIR_BitsType:$out_ready, StrAttr:$instance_name,
      ACIR_StaticExprAttr:$depth, StrAttr:$ready_policy,
      ACIR_StaticExprAttr:$availability_latency,
      ACIR_StaticExprAttr:$head_read_latency, BoolAttr:$empty_flow,
      StrAttr:$read_during_write, StrAttr:$reset_policy,
      StrAttr:$empty_data, DictionaryAttr:$occurrence);
  let results = (outs ACIR_BitsType:$in_ready,
      ACIR_BitsType:$out_valid, ACIR_HardwareType:$out_data);
  let hasVerifier = 1;
}

def HWIR_TableCreateOp : HWIR_Op<"table.create", [Pure]> {
  let arguments = (ins Variadic<ACIR_ScalarHardwareType>:$inputs);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def HWIR_TableSplatOp : HWIR_Op<"table.splat", [Pure]> {
  let arguments = (ins ACIR_HardwareType:$input, ArrayAttr:$shape);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def HWIR_TableMapOp : HWIR_Op<"table.map", [Pure, IsolatedFromAbove,
    SingleBlock, AttrSizedOperandSegments]> {
  let arguments = (ins Variadic<ACIR_TableType>:$tables,
      Variadic<ACIR_ScalarHardwareType>:$captures, ArrayAttr:$shape);
  let results = (outs Variadic<ACIR_TableType>:$results);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
}
def HWIR_TableViewOp : HWIR_Op<"table.view", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, StrAttr:$kind,
      DictionaryAttr:$parameters);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def HWIR_TableIndexOp : HWIR_Op<"table.index", [Pure]> {
  let arguments = (ins Variadic<ACIR_BitsType>:$coords, ArrayAttr:$shape);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}
def HWIR_TableGetOp : HWIR_Op<"table.get", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, ACIR_BitsType:$index);
  let results = (outs ACIR_ScalarHardwareType:$value, ACIR_BitsType:$in_range);
  let hasVerifier = 1;
}
def HWIR_TableMatchOp : HWIR_Op<"table.match", [Pure, IsolatedFromAbove,
    SingleBlock]> {
  let arguments = (ins ACIR_TableType:$input,
      Variadic<ACIR_ScalarHardwareType>:$captures);
  let results = (outs ACIR_BitsType:$mask);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
}
def HWIR_TableChooseOp : HWIR_Op<"table.choose", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, ACIR_BitsType:$mask,
      I64Attr:$count, StrAttr:$policy, StrAttr:$order);
  let results = (outs Variadic<ACIR_BitsType>:$results);
  let hasVerifier = 1;
}
def HWIR_TableFoldOp : HWIR_Op<"table.fold", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, StrAttr:$kind);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}
def HWIR_ValueMergeOp : HWIR_Op<"value.merge", [Pure,
    AttrSizedOperandSegments]> {
  let arguments = (ins ACIR_ScalarHardwareType:$base,
      Variadic<ACIR_BitsType>:$guards,
      Variadic<ACIR_HardwareType>:$values, ArrayAttr:$paths);
  let results = (outs ACIR_ScalarHardwareType:$next, ACIR_BitsType:$en);
  let hasVerifier = 1;
}

def HWIR_RuleOp : HWIR_Op<"rule", [IsolatedFromAbove, SingleBlock, Pure]> {
  let summary = "Define one module-local combinational calculation";
  let description = [{
    Captures bind the rule arguments and results are combinational wires.
    The body computes results from explicit captures and yields them in order.
    It owns no storage, child instance or retained effect and has no implicit
    writeback or source-order visibility into another rule's work. Its
    placement follows data dependencies, not textual order.
  }];
  let arguments = (ins Variadic<ACIR_HardwareType>:$captures, StrAttr:$name,
      OptionalAttr<DictionaryAttr>:$occurrence);
  let results = (outs Variadic<ACIR_HardwareType>:$results);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

def HWIR_YieldOp : HWIR_Op<"yield", [Terminator, MemoryEffects<[]>]> {
  let summary = "Bind module outputs or rule results";
  let description = [{
    Values bind the enclosing definition or rule results in order and their
    types must match exactly. Yield is an output connection, never storage or
    a clock boundary.
  }];
  let arguments = (ins Variadic<ACIR_HardwareType>:$values);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

// ---------------------------------------------------------------------------
// Retained effects
// ---------------------------------------------------------------------------

def HWIR_ExpectOp : HWIR_Op<"expect"> {
  let summary = "Retain one check with its condition, path and identity";
  let description = [{
    condition is the raw guard and path is its reached prefix; both are
    one-bit wires. guard_known sampling names the physical clock and reset
    pins, clock first. kind selects the check contract and location carries
    the retained check identity. A check is a retained effect and cannot
    appear inside a pure rule.
  }];
  let arguments = (ins ACIR_BitsType:$condition, ACIR_BitsType:$path,
      Variadic<ACIR_BitsType>:$sampling, StrAttr:$kind,
      OptionalAttr<DictionaryAttr>:$location);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

def HWIR_ObserveOp : HWIR_Op<"observe"> {
  let summary = "Retain one managed observation and its payload";
  let description = [{
    path is the reached prefix and values are the retained payload wires.
    kind selects the observation contract; spec carries the layout and
    identity needed by managed publication. An observation is a retained
    effect and cannot appear inside a pure rule.
  }];
  let arguments = (ins ACIR_BitsType:$path,
      Variadic<ACIR_HardwareType>:$values, StrAttr:$kind,
      OptionalAttr<DictionaryAttr>:$spec);
  let hasVerifier = 1;
  let hasCustomAssemblyFormat = 1;
}

#endif // PYCIRCUIT_DIALECT_HWIR_HWIROPS_TD

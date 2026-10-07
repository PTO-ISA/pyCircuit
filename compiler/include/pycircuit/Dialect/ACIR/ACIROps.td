#ifndef ACIR_DIALECT_ACIR_ACIROPS_TD
#define ACIR_DIALECT_ACIR_ACIROPS_TD

include "pycircuit/Dialect/ACIR/ACIRTypes.td"
include "mlir/IR/CommonAttrConstraints.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/RegionKindInterface.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class ACIR_Op<string mnemonic, list<Trait> traits = []>
    : Op<ACIR_Dialect, mnemonic, traits>;

def ACIR_TypeAliasOp : ACIR_Op<"type_alias", [Symbol]> {
  let summary = "Declare one source-owned transparent logical type alias";
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$target);
  let hasVerifier = 1;
  let assemblyFormat = "$sym_name `target` $target attr-dict";
}

def ACIR_ConstantOp : ACIR_Op<"constant", [Symbol]> {
  let summary = "Declare one source-owned compile-time constant";
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$type,
      DictionaryAttr:$value);
  let hasVerifier = 1;
}

def ACIR_MathConstantOp : ACIR_Op<"math.constant"> {
  let summary = "Materialize one source mathematical integer constant";
  let arguments = (ins ACIR_MathIntAttr:$value);
  let results = (outs ACIR_MathIntType:$result);
  let hasVerifier = 1;
}

def ACIR_ValueBindingOp : ACIR_Op<"value.binding"> {
  let summary = "Bind one proof-scoped ValueID to actual SSA and controls";
  let arguments = (ins AnyType:$value, I1:$valid, I1:$path,
      DictionaryAttr:$id, DictionaryAttr:$domain);
  let hasVerifier = 1;
}

def ACIR_ValueUseOp : ACIR_Op<"value.use"> {
  let summary = "Bind one proof-scoped source use to verified finite SSA";
  let arguments = (ins AnyType:$value, I1:$valid, I1:$path,
      DictionaryAttr:$id, DictionaryAttr:$source);
  let hasVerifier = 1;
}

def ACIR_SystemOp : ACIR_Op<"system"> {
  let summary = "Select one verified final hardware instance tree";
  let arguments = (ins DictionaryAttr:$entry, StrAttr:$domain);
  let hasVerifier = 1;
}

def ACIR_NumericProofOp : ACIR_Op<"numeric.proof"> {
  let summary = "Verify one constant-only exact numeric witness";
  let arguments = (ins Variadic<AnyType>:$values,
      DenseI32ArrayAttr:$operand_segment_sizes, StrAttr:$mode,
      DictionaryAttr:$result_domain, ArrayAttr:$input_ids,
      ArrayAttr:$input_domains, DictionaryAttr:$result_id,
      ArrayAttr:$obligations, ArrayAttr:$checks, DictionaryAttr:$origin);
  let hasVerifier = 1;
}

def ACIR_MathFromBitsOp : ACIR_Op<"math.from_bits"> {
  let summary = "Lift a bounded source integer into mathematical arithmetic";
  let arguments = (ins AnySignlessInteger:$value, DictionaryAttr:$domain);
  let results = (outs ACIR_MathIntType:$result);
  let hasVerifier = 1;
}

def ACIR_MathCompareOp : ACIR_Op<"math.compare"> {
  let summary = "Compare two lifted source mathematical integers";
  let arguments = (ins I1:$path, ACIR_MathIntType:$lhs, I1:$lhs_valid,
      ACIR_MathIntType:$rhs, I1:$rhs_valid, StrAttr:$predicate);
  let results = (outs AnySignlessInteger:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_MathBinaryOp : ACIR_Op<"math.binary"> {
  let summary = "Evaluate one guarded source mathematical binary operation";
  let arguments = (ins I1:$path, ACIR_MathIntType:$lhs, I1:$lhs_valid,
      ACIR_MathIntType:$rhs, I1:$rhs_valid);
  let results = (outs ACIR_MathIntType:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_MathToBitsOp : ACIR_Op<"math.to_bits"> {
  let summary = "Range-check and lower a mathematical integer to bounded bits";
  let arguments = (ins I1:$path, ACIR_MathIntType:$value, I1:$value_valid,
      DictionaryAttr:$domain);
  let results = (outs AnySignlessInteger:$result, I1:$valid);
  let hasVerifier = 1;
}

def ACIR_BitsConstantOp : ACIR_Op<"bits.constant", [Pure]> {
  let summary = "Create one known fixed-width hardware bit vector";
  let arguments = (ins ACIR_StaticExprAttr:$value);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsUnaryOp : ACIR_Op<"bits.unary", [Pure]> {
  let summary = "Apply one closed bit-vector unary operation";
  let arguments = (ins ACIR_BitsType:$input, StrAttr:$opcode);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsBinaryOp : ACIR_Op<"bits.binary", [Pure]> {
  let summary = "Apply one closed bit-vector binary operation";
  let arguments = (ins ACIR_BitsType:$lhs, ACIR_BitsType:$rhs,
      StrAttr:$opcode);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsCompareOp : ACIR_Op<"bits.compare", [Pure]> {
  let summary = "Compare two same-width hardware bit vectors";
  let arguments = (ins ACIR_BitsType:$lhs, ACIR_BitsType:$rhs,
      StrAttr:$predicate);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsSelectOp : ACIR_Op<"bits.select", [Pure]> {
  let summary = "Four-state conditional bit merge";
  let arguments = (ins ACIR_BitsType:$condition, ACIR_BitsType:$true_value,
      ACIR_BitsType:$false_value);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsConcatOp : ACIR_Op<"bits.concat", [Pure]> {
  let summary = "Concatenate hardware bit vectors";
  let arguments = (ins Variadic<ACIR_BitsType>:$inputs);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsExtractOp : ACIR_Op<"bits.extract", [Pure]> {
  let summary = "Extract a statically bounded hardware bit slice";
  let arguments = (ins ACIR_BitsType:$input, ACIR_StaticExprAttr:$low);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_BitsResizeOp : ACIR_Op<"bits.resize", [Pure]> {
  let summary = "Explicitly truncate or extend a hardware bit vector";
  let arguments = (ins ACIR_BitsType:$input, StrAttr:$mode);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_ModuleImportOp : ACIR_Op<"module.import", [Symbol]> {
  let summary = "Declare a source-owned hardware module or standard leaf";
  let description = [{
    Publishes the same signature as ac.module without copying its body.
    function_type owns the input/result types and direction; input_names and
    output_names name those positions. Integer parameters and type_parameters
    are compile-time formals, with the same binding rules as ac.module.

    dependency_summary is derived from actual combinational dataflow. Each
    entry is {output = {port, path}, inputs = [{port, path}, ...]}, where port
    is an ordinal within its direction and path is an ordered array of struct
    field names. An empty path selects the entire port payload; other paths
    select field subtrees. Generic definitions may retain these prefixes.

    After type binding, expand each output subtree to its bits leaves. For
    each output leaf, a bits input endpoint contributes that fixed dependency;
    an aggregate input endpoint of the same subtree type contributes only
    the input leaf at the same relative field path. Aggregate inputs never
    imply all-fields dependence. Cross-field dependencies name input leaves
    explicitly; mismatched aggregate endpoints are invalid. An empty inputs
    array denotes no combinational input dependency for the whole output
    subtree, including a type parameter whose fields are not yet known.

    Table axes are compact in this summary: a field path denotes that element
    field across the original table port, without lane ordinals. Classification
    compares logical element payloads after removing table wrappers. Bits or
    table-of-bits inputs contribute their original endpoint; struct prefixes
    require exactly matching nominal subtrees and preserve corresponding-field
    dependence. Endpoint table shapes need not match for a dependency (for
    example view, get or fold); operation and instance verifiers still enforce
    shape legality. This does not imply broadcasting or same-lane correspondence.

    Expanded entries must cover every output leaf exactly once, without
    overlapping prefixes or invalid paths. Expansion is derived analysis,
    not module cloning. Linked ordinary definitions must reproduce their
    summaries from the actual field-level dataflow.

    primitive_kind identifies a trusted standard leaf's hardware behavior.
    Ordinary imported modules omit it. Leaf and ordinary instances use the
    same signature and connections. Storage outputs such as DFF Q are temporal
    dependency cuts; asynchronous memory reads retain their address dependency.
    Backend class names, filenames and runtime lifecycle hooks are not part of
    this declaration's hardware semantics.
  }];
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$source_owner,
      DictArrayAttr:$parameters, StrArrayAttr:$type_parameters,
      TypeAttrOf<FunctionType>:$function_type, StrArrayAttr:$input_names,
      StrArrayAttr:$output_names, DictArrayAttr:$dependency_summary,
      OptionalAttr<StrAttr>:$primitive_kind);
  let hasVerifier = 1;
}

def ACIR_ModuleOp : ACIR_Op<"module", [Symbol, SymbolTable,
    IsolatedFromAbove, SingleBlock, RegionKindInterface,
    HasOnlyGraphRegion]> {
  let summary = "Define reusable hardware structure and its Work rules";
  let description = [{
    Defines a reusable hardware graph. Block arguments are input wires and
    the final ac.yield binds output wires. function_type is the sole owner
    of their types and direction. input_names and output_names match the
    corresponding positions exactly; names are nonempty and unique across
    both lists. Every signature element has an ACIR_HardwareType payload.

    parameters declares ordered integer formals using {name, type, default?},
    with !ac.math_int as type and StaticExprAttr for an optional default.
    type_parameters declares ordered, required hardware type formal names.
    Names are unique across both parameter lists. TypeParamType references
    use this definition's symbol as owner; widths use its integer formals.
    Parameters are elaboration inputs, not runtime ports.

    This operation is a definition, not an occurrence in the circuit. Its
    body and nested rules are shared by ac.instance uses of the symbol.
    Instance-local wire values and leaf state belong to each occurrence,
    never to the shared definition.

    The body contains combination and fixed child instances. The graph's
    textual order does not define an execution schedule. SSA supplies one
    driver per signal and permits fanout. Feedback through a storage leaf is
    legal; an actual combinational cycle is not. The module introduces no
    implicit storage, current/next handles, or mutable wire references.
  }];
  let arguments = (ins StrAttr:$sym_name, DictionaryAttr:$source_owner,
      DictArrayAttr:$parameters, StrArrayAttr:$type_parameters,
      TypeAttrOf<FunctionType>:$function_type, StrArrayAttr:$input_names,
      StrArrayAttr:$output_names);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_RuleOp : ACIR_Op<"rule", [IsolatedFromAbove, SingleBlock, Pure]> {
  let summary = "Define a combinational calculation within module Work";
  let description = [{
    Captures and results are hardware wires, including whole structs. The
    body computes results from explicit captures and yields them in order.
    It owns no storage or child instances, and has no implicit writeback or
    source-order visibility of another rule's work. Fields needed from a
    captured struct are ordinary combinational projections.

    A rule is a module-local calculation with explicit arguments and results,
    suitable for a helper function invoked from the module's Work method.
    Its captures bind those arguments and its SSA results connect its users.
    It is not a module instance or an independently scheduled runtime object.
    Dependencies determine its placement within Work; its source name and
    textual order do not introduce priority or a clock boundary.
  }];
  let arguments = (ins Variadic<ACIR_HardwareType>:$captures, StrAttr:$name,
      DictionaryAttr:$occurrence);
  let results = (outs Variadic<ACIR_HardwareType>:$results);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}

def ACIR_SourceObserveOp : ACIR_Op<"observe", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary = "Record one source observation without runtime side effects";
  let arguments = (ins ACIR_BitsType:$path, Variadic<AnyType>:$values, StrAttr:$kind,
      DictionaryAttr:$spec);
  let hasVerifier = 1;
}

def ACIR_SourceExpectOp : ACIR_Op<"expect", [
    DeclareOpInterfaceMethods<MemoryEffectsOpInterface>]> {
  let summary =
      "Record one source assertion check without runtime side effects";
  let arguments = (ins ACIR_BitsType:$condition, ACIR_BitsType:$path, StrAttr:$kind,
      DictionaryAttr:$location);
  let hasVerifier = 1;
}

def ACIR_InstanceOp : ACIR_Op<"instance", [
    DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
  let summary = "Instantiate one module with typed SSA input and output wires";
  let description = [{
    callee resolves to ac.module or ac.module.import. Input operands bind
    the callee's input positions; results are its output wires. After static
    and type substitution, arity and payload types must match exactly.
    Whole structs connect directly, preserving their field layout and X/Z.

    parameters supplies ordered StaticExprAttr integer actuals, with defaults
    resolved against the callee declaration. type_arguments supplies one
    TypeAttr per type_parameters entry, in declaration order. A type actual
    is finite bits/struct data or a forwarded enclosing type parameter.
    These bindings reuse the callee definition rather than cloning it.

    Every instance denotes distinct hardware, including independent state
    owned by its leaf descendants. An instance is not a Pure software call
    and must not be merged by value CSE. instance_name is unique within the
    parent; occurrence records source identity, not admission criteria.
    Ordinary and storage leaf modules use this same operation.
  }];
  let arguments = (ins Variadic<ACIR_HardwareType>:$inputs, StrAttr:$instance_name,
      FlatSymbolRefAttr:$callee,
      TypedArrayAttrBase<ACIR_StaticExprAttr,
          "ordered static integer arguments">:$parameters,
      TypedArrayAttrBase<TypeAttrOf<ACIR_ScalarHardwareType>,
          "ordered hardware type arguments">:$type_arguments,
      DictionaryAttr:$occurrence);
  let results = (outs Variadic<ACIR_HardwareType>:$outputs);
  let hasVerifier = 1;
}

def ACIR_YieldOp : ACIR_Op<"yield", [Terminator, MemoryEffects<[]>]> {
  let summary = "Return module outputs or combinational rule results";
  let description = [{
    Binds output wires in signature order. Values must exactly match the
    enclosing module or rule results, including aggregate types. Yield is
    an output connection, not a clock boundary or a copy into storage.
  }];
  let arguments = (ins Variadic<ACIR_HardwareType>:$values);
  let hasVerifier = 1;
}

def ACIR_CollectionOp : ACIR_Op<"collection", [
    DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
  let summary = "Compact family of independent instances with table wires";
  let arguments = (ins Variadic<ACIR_HardwareType>:$inputs,
      StrAttr:$instance_name, FlatSymbolRefAttr:$callee,
      TypedArrayAttrBase<ACIR_StaticExprAttr, "static integer actuals">:$parameters,
      TypedArrayAttrBase<TypeAttrOf<ACIR_ScalarHardwareType>, "scalar type actuals">:$type_arguments,
      ArrayAttr:$shape, OptionalAttr<DictArrayAttr>:$layout,
      DictionaryAttr:$occurrence);
  let results = (outs Variadic<ACIR_HardwareType>:$outputs);
  let hasVerifier = 1;
}

def ACIR_QueueOp : ACIR_Op<"queue"> {
  let summary = "Own one bounded FIFO of complete hardware tokens";
  let arguments = (ins
      ACIR_BitsType:$clk, ACIR_BitsType:$rst,
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

def ACIR_TableCreateOp : ACIR_Op<"table.create", [Pure]> {
  let arguments = (ins Variadic<ACIR_ScalarHardwareType>:$inputs);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def ACIR_TableSplatOp : ACIR_Op<"table.splat", [Pure]> {
  let arguments = (ins ACIR_HardwareType:$input, ArrayAttr:$shape);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def ACIR_TableMapOp : ACIR_Op<"table.map", [Pure, IsolatedFromAbove,
    SingleBlock, AttrSizedOperandSegments]> {
  let arguments = (ins Variadic<ACIR_TableType>:$tables,
      Variadic<ACIR_ScalarHardwareType>:$captures, ArrayAttr:$shape);
  let results = (outs Variadic<ACIR_TableType>:$results);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}
def ACIR_TableViewOp : ACIR_Op<"table.view", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, StrAttr:$kind,
      DictionaryAttr:$parameters);
  let results = (outs ACIR_TableType:$result);
  let hasVerifier = 1;
}
def ACIR_TableIndexOp : ACIR_Op<"table.index", [Pure]> {
  let arguments = (ins Variadic<ACIR_BitsType>:$coords, ArrayAttr:$shape);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}
def ACIR_TableGetOp : ACIR_Op<"table.get", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, ACIR_BitsType:$index);
  let results = (outs ACIR_ScalarHardwareType:$value, ACIR_BitsType:$in_range);
  let hasVerifier = 1;
}
def ACIR_TableMatchOp : ACIR_Op<"table.match", [Pure, IsolatedFromAbove,
    SingleBlock]> {
  let arguments = (ins ACIR_TableType:$input,
      Variadic<ACIR_ScalarHardwareType>:$captures);
  let results = (outs ACIR_BitsType:$mask);
  let regions = (region AnyRegion:$body);
  let hasVerifier = 1;
  let hasRegionVerifier = 1;
}
def ACIR_TableChooseOp : ACIR_Op<"table.choose", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, ACIR_BitsType:$mask,
      I64Attr:$count, StrAttr:$policy, StrAttr:$order);
  let results = (outs Variadic<ACIR_BitsType>:$results);
  let hasVerifier = 1;
}
def ACIR_TableFoldOp : ACIR_Op<"table.fold", [Pure]> {
  let arguments = (ins ACIR_TableType:$input, StrAttr:$kind);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}
def ACIR_ValueMergeOp : ACIR_Op<"value.merge", [Pure,
    AttrSizedOperandSegments]> {
  let arguments = (ins ACIR_ScalarHardwareType:$base,
      Variadic<ACIR_BitsType>:$guards,
      Variadic<ACIR_ScalarHardwareType>:$values, ArrayAttr:$paths);
  let results = (outs ACIR_ScalarHardwareType:$next, ACIR_BitsType:$en);
  let hasVerifier = 1;
}

def ACIR_EnumOp : ACIR_Op<"enum", [Symbol]> {
  let summary = "Declare a nominal enum with ordered physical member codes";
  let description = [{
    Width is a positive closed mathematical integer. Each ordered member is
    exactly {name: StringAttr, code: MathIntAttr}. Names and nonnegative codes
    are unique, and codes fit the width. Derived encodings are verified against
    source-order ordinals; explicit encoding preserves the declared codes.
  }];
  let arguments = (ins StrAttr:$sym_name, ACIR_MathIntAttr:$width,
      StrAttr:$encoding, DictArrayAttr:$members);
  let hasVerifier = 1;
}

def ACIR_EnumCreateOp : ACIR_Op<"enum.create", [Pure]> {
  let summary = "Create the declared code of one nominal enum member";
  let arguments = (ins StrAttr:$member);
  let results = (outs ACIR_EnumType:$result);
  let hasVerifier = 1;
}

def ACIR_EnumToBitsOp : ACIR_Op<"enum.to_bits", [Pure]> {
  let summary = "Expose an enum carrier without changing its four-state bits";
  let arguments = (ins ACIR_EnumType:$input);
  let results = (outs ACIR_BitsType:$result);
  let hasVerifier = 1;
}

def ACIR_EnumFromBitsOp : ACIR_Op<"enum.from_bits", [Pure]> {
  let summary = "Preserve raw bits as an enum and compute four-state membership";
  let description = [{
    Input width equals the declaration width. The value preserves every input
    bit, including invalid codes and X/Z. The one-bit member result is the OR
    of existing four-state equality comparisons with the declared codes.
    Ignoring membership neither changes the carrier nor creates an assertion.
  }];
  let arguments = (ins ACIR_BitsType:$input);
  let results = (outs ACIR_EnumType:$value, ACIR_BitsType:$member);
  let hasVerifier = 1;
}

def ACIR_StructOp : ACIR_Op<"struct", [Symbol]> {
  let summary = "Declare the ordered fields of a packed hardware struct";
  let description = [{
    Each fields entry is exactly {name: StringAttr, type: TypeAttr}. Names
    are nonempty and unique. Field types resolve to finite bits, enums or other
    declared structs; recursive layouts and unresolved type parameters are
    invalid. The array preserves declaration order. There are no field
    directions, initialization values, storage handles or source constructors.

    The first field occupies the most significant bits. Later fields follow
    contiguously toward bit zero, with no padding. Nested structs apply the
    same rule recursively, and total width is the sum of field widths. Both
    backends use this layout independently of host struct alignment.
  }];
  let arguments = (ins StrAttr:$sym_name, DictArrayAttr:$fields);
  let hasVerifier = 1;
  let assemblyFormat =
      "$sym_name `fields` $fields attr-dict";
}

def ACIR_StructCreateOp : ACIR_Op<"struct.create", [Pure]> {
  let summary = "Assemble a complete packed struct wire from field wires";
  let description = [{
    Operands supply every declared field exactly once, in declaration order
    and with the declared field type. This is combinational assembly, not a
    constructor call, allocation or register write. Missing fields do not
    imply initialization, a previous value or storage hold.
  }];
  let arguments = (ins Variadic<ACIR_ScalarHardwareType>:$values);
  let results = (outs ACIR_StructType:$result);
  let hasVerifier = 1;
  let assemblyFormat =
      "`(` $values `)` attr-dict `:` functional-type($values, results)";
}

def ACIR_StructGetOp : ACIR_Op<"struct.get", [Pure]> {
  let summary = "Project one field wire from a packed struct wire";
  let description = [{
    field names a declared field and the result has exactly its type. The
    projection depends only on that field's bits and preserves their X/Z
    states. Nested projections follow the corresponding field path; reading
    one field creates no dependency on the other fields of the struct.
  }];
  let arguments = (ins ACIR_StructType:$value, StrAttr:$field);
  let results = (outs ACIR_ScalarHardwareType:$result);
  let hasVerifier = 1;
  let assemblyFormat =
      "$value `[` $field `]` attr-dict `:` functional-type(operands, results)";
}

#endif // ACIR_DIALECT_ACIR_ACIROPS_TD

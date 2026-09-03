#ifndef ACIR_DIALECT_ACSIM_ACSIMOPS_TD
#define ACIR_DIALECT_ACSIM_ACSIMOPS_TD

include "acir/Dialect/ACSim/ACSimTypes.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class ACSim_Op<string mnemonic, list<Trait> traits = []>
    : Op<ACSim_Dialect, mnemonic, traits>;

def ACSim_ModelOp : ACSim_Op<"model", [
    Symbol, SymbolTable, IsolatedFromAbove, SingleBlock, NoTerminator,
    RecursiveMemoryEffects
  ]> {
  let arguments = (ins SymbolNameAttr:$sym_name, StrAttr:$contract_epoch,
                       FlatSymbolRefAttr:$root,
                       ArrayAttr:$construction_order,
                       ArrayAttr:$destruction_order,
                       DictionaryAttr:$fingerprints);
  let regions = (region AnyRegion:$body);
  let assemblyFormat = [{
    $sym_name `epoch` $contract_epoch `root` $root
    `construction` $construction_order `destruction` $destruction_order
    `fingerprints` $fingerprints attr-dict-with-keyword $body
  }];
  let hasVerifier = 1;
}

def ACSim_TypeOp : ACSim_Op<"type", [Symbol, Pure]> {
  let arguments = (ins SymbolNameAttr:$sym_name, StrAttr:$cpp_name,
                       StrAttr:$kind, StrAttr:$fingerprint,
                       OptionalAttr<I64Attr>:$period,
                       OptionalAttr<I64Attr>:$phase,
                       OptionalAttr<I64Attr>:$tick_scale,
                       OptionalAttr<FlatSymbolRefAttr>:$parent,
                       OptionalAttr<DictionaryAttr>:$bridge);
  let assemblyFormat = [{
    $sym_name `cpp` $cpp_name `kind` $kind `fingerprint` $fingerprint attr-dict
  }];
  let hasVerifier = 1;
}

def ACSim_BindingOp : ACSim_Op<"binding", [Symbol, Pure]> {
  let arguments = (ins SymbolNameAttr:$sym_name, DictionaryAttr:$record);
  let assemblyFormat = "$sym_name `record` $record attr-dict";
  let extraClassDeclaration = [{
    ::mlir::StringAttr getBindingSchemaAttr() {
      return getRecord().getAs<::mlir::StringAttr>("binding_schema");
    }
    ::llvm::StringRef getBindingSchema() {
      auto value = getBindingSchemaAttr();
      return value ? value.getValue() : ::llvm::StringRef();
    }
    ::mlir::FlatSymbolRefAttr getCppTypeAttr() {
      return getRecord().getAs<::mlir::FlatSymbolRefAttr>("cpp_type");
    }
    ::mlir::FlatSymbolRefAttr getSchemaAttr() {
      return getRecord().getAs<::mlir::FlatSymbolRefAttr>("component_schema");
    }
    ::mlir::FlatSymbolRefAttr getProviderAttr() {
      return getRecord().getAs<::mlir::FlatSymbolRefAttr>("provider");
    }
    ::mlir::FlatSymbolRefAttr getImplementationAttr() {
      return getRecord().getAs<::mlir::FlatSymbolRefAttr>("implementation");
    }
    ::mlir::StringAttr getEffectAttr() {
      return getRecord().getAs<::mlir::StringAttr>("effect");
    }
    ::llvm::StringRef getEffect() {
      auto value = getEffectAttr();
      return value ? value.getValue() : ::llvm::StringRef();
    }
    ::mlir::StringAttr getFingerprintAttr() {
      return getRecord().getAs<::mlir::StringAttr>("fingerprint");
    }
    ::mlir::DictionaryAttr getCppRecord() {
      return getRecord().getAs<::mlir::DictionaryAttr>("cpp");
    }
  }];
  let hasVerifier = 1;
}

def ACSim_ModuleOp : ACSim_Op<"module", [
    Symbol, SingleBlock, RecursiveMemoryEffects
  ]> {
  let arguments = (ins SymbolNameAttr:$sym_name, DictionaryAttr:$interface,
                       ArrayAttr:$static_params,
                       StrAttr:$specialization_fingerprint,
                       ArrayAttr:$exports);
  let regions = (region SizedRegion<1>:$body);
  let assemblyFormat = [{
    $sym_name `interface` $interface `static` $static_params
    `specialization` $specialization_fingerprint `exports` $exports
    attr-dict-with-keyword $body
  }];
  let hasVerifier = 1;
}

def ACSim_InstanceOp : ACSim_Op<"instance"> {
  let arguments = (ins SymbolNameAttr:$sym_name, SymbolRefAttr:$target,
                       ArrayAttr:$static_args,
                       StrAttr:$specialization_fingerprint);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $sym_name `target` $target `args` $static_args
    `specialization` $specialization_fingerprint attr-dict `:` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_ArrayOp : ACSim_Op<"array"> {
  let arguments = (ins SymbolNameAttr:$sym_name, SymbolRefAttr:$target,
                       ArrayAttr:$static_args,
                       StrAttr:$specialization_fingerprint,
                       DenseI64ArrayAttr:$shape);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $sym_name `target` $target `args` $static_args
    `specialization` $specialization_fingerprint
    `shape` $shape attr-dict `:` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_ElementOp : ACSim_Op<"element", [Pure]> {
  let arguments = (ins AnyType:$array, DenseI64ArrayAttr:$indices);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $array `indices` $indices attr-dict `:` type($array) `->` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_PortOp : ACSim_Op<"port", [Pure]> {
  let arguments = (ins AnyType:$base, FlatSymbolRefAttr:$accessor);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $base `accessor` $accessor attr-dict `:` type($base) `->` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_ResourceOp : ACSim_Op<"resource", [Pure]> {
  let arguments = (ins AnyType:$base, FlatSymbolRefAttr:$accessor);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $base `accessor` $accessor attr-dict `:` type($base) `->` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_BindOp : ACSim_Op<"bind"> {
  let arguments = (ins AnyType:$source, AnyType:$target, StrAttr:$kind);
  let assemblyFormat = [{
    $source `to` $target `kind` $kind attr-dict `:` type($source) `to`
    type($target)
  }];
  let hasVerifier = 1;
}

def ACSim_InlineOp : ACSim_Op<"inline", [Pure]> {
  let arguments = (ins Variadic<AnyType>:$args, FlatSymbolRefAttr:$callee);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $callee `(` $args `)` attr-dict `:` functional-type($args, $result)
  }];
  let hasVerifier = 1;
}

def ACSim_ProcessOp : ACSim_Op<"process", [
    IsolatedFromAbove, RecursiveMemoryEffects
  ]> {
  let arguments = (ins Variadic<AnyType>:$captures,
                       SymbolNameAttr:$sym_name,
                       ArrayAttr:$capture_names,
                       FlatSymbolRefAttr:$entry_pc, ArrayAttr:$pcs,
                       ArrayAttr:$live_slots, I64Attr:$fairness_cap,
                       StrAttr:$specialization_fingerprint);
  let regions = (region VariadicRegion<AnyRegion>:$states);
  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;
}

def ACSim_LiveLoadOp : ACSim_Op<"live.load"> {
  let arguments = (ins FlatSymbolRefAttr:$process, StrAttr:$slot);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $process `slot` $slot attr-dict `:` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_LiveStoreOp : ACSim_Op<"live.store"> {
  let arguments = (ins AnyType:$value, FlatSymbolRefAttr:$process,
                       StrAttr:$slot);
  let assemblyFormat = [{
    $value `in` $process `slot` $slot attr-dict `:` type($value)
  }];
  let hasVerifier = 1;
}

def ACSim_InvokeOp : ACSim_Op<"invoke"> {
  let arguments = (ins Variadic<AnyType>:$args, FlatSymbolRefAttr:$callee);
  let results = (outs Variadic<AnyType>:$results);
  let assemblyFormat = [{
    $callee `(` $args `)` attr-dict `:` functional-type($args, $results)
  }];
  let hasVerifier = 1;
}

def ACSim_ContinueOp : ACSim_Op<"continue", [Terminator]> {
  let arguments = (ins FlatSymbolRefAttr:$target_pc);
  let assemblyFormat = "$target_pc attr-dict";
  let hasVerifier = 1;
}

def ACSim_SuspendOp : ACSim_Op<"suspend", [Terminator]> {
  let arguments = (ins AnyType:$wake, FlatSymbolRefAttr:$target_pc);
  let assemblyFormat = "$target_pc `on` $wake attr-dict `:` type($wake)";
  let hasVerifier = 1;
}

def ACSim_TerminateOp : ACSim_Op<"terminate", [Terminator]> {
  let arguments = (ins StrAttr:$status);
  let assemblyFormat = "$status attr-dict";
  let hasVerifier = 1;
}

def ACSim_ExportOp : ACSim_Op<"export", [Pure]> {
  let arguments = (ins AnyType:$value, SymbolNameAttr:$sym_name,
                       FlatSymbolRefAttr:$role);
  let results = (outs AnyType:$result);
  let assemblyFormat = [{
    $sym_name $value `role` $role attr-dict `:` type($value) `->` type($result)
  }];
  let hasVerifier = 1;
}

def ACSim_DispatchOp : ACSim_Op<"dispatch"> {
  let arguments = (ins SymbolRefAttr:$target, StrAttr:$path,
                       DenseI64ArrayAttr:$indices,
                       I64Attr:$object_id, I64Attr:$activation_id,
                       StrAttr:$work, StrAttr:$xfer, StrAttr:$reset,
                       StrAttr:$validate);
  let results = (outs ACSim_ObjectIdType:$object,
                      ACSim_ActivationIdType:$activation);
  let assemblyFormat = [{
    $target `path` $path `indices` $indices
    `object` $object_id `activation` $activation_id
    `work` $work `xfer` $xfer `reset` $reset `validate` $validate attr-dict
    `:` type($object) `,` type($activation)
  }];
  let hasVerifier = 1;
}

def ACSim_ActivateOp : ACSim_Op<"activate"> {
  let arguments = (ins ACSim_ActivationIdType:$source,
                       ACSim_ObjectIdType:$target);
  let assemblyFormat = [{
    $source `to` $target attr-dict `:` type($source) `to` type($target)
  }];
  let hasVerifier = 1;
}

def ACSim_ReturnOp : ACSim_Op<"return", [Terminator]> {
  let arguments = (ins Variadic<AnyType>:$values);
  let assemblyFormat = "($values^ `:` type($values))? attr-dict";
  let hasVerifier = 1;
}

#endif // ACIR_DIALECT_ACSIM_ACSIMOPS_TD

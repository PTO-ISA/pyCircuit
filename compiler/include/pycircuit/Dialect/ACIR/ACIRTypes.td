#ifndef ACIR_DIALECT_ACIR_ACIRTYPES_TD
#define ACIR_DIALECT_ACIR_ACIRTYPES_TD

include "pycircuit/Dialect/ACIR/ACIRAttributes.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/CommonTypeConstraints.td"

class ACIR_Type<string name, string typeMnemonic>
    : TypeDef<ACIR_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def ACIR_MathIntType : ACIR_Type<"MathInt", "math_int"> {
  let summary = "Temporary arbitrary-precision source mathematical integer";
  let hasCustomAssemblyFormat = 1;
}

def ACIR_BitsType : ACIR_Type<"Bits", "bits"> {
  let summary = "Parameter-width four-state hardware bit vector";
  let description = [{
    A wire payload with a positive, finite, compile-time width. Width may
    reference the declaring module's integer parameters; each instance binds
    those parameters without changing the module definition. Each bit retains
    its value and X/Z state across connections and storage.
  }];
  let parameters = (ins "::acir::ac::StaticExprAttr":$width);
  let assemblyFormat = "`<` $width `>`";
  let genVerifyDecl = 1;
}

def ACIR_StructType : ACIR_Type<"Struct", "struct"> {
  let summary = "Qualified nominal packed hardware struct";
  let description = [{
    The qualified name resolves to an ac.struct declaration. Its ordered
    fields define a finite packed payload, recursively composed of bits, enums
    and other structs. Field order and widths determine the layout, never host
    object size, alignment or padding. Field names carry no port direction.

    An SSA value of this type is a wire carrying the complete struct. Field
    extraction and construction are combinational; neither the type nor a
    struct value allocates storage. All fields preserve four-state semantics.
  }];
  let parameters = (ins "::mlir::StringAttr":$name);
  let assemblyFormat = "`<` $name `>`";
  let genVerifyDecl = 1;
}

def ACIR_EnumType : ACIR_Type<"Enum", "enum"> {
  let summary = "Qualified nominal encoded hardware enum";
  let description = [{
    The name resolves to one ac.enum declaration. Its declared width defines
    a four-state carrier; the member table does not imply that every carrier
    contains a valid member code. Equal widths or codes do not equate names.
  }];
  let parameters = (ins "::mlir::StringAttr":$name);
  let assemblyFormat = "`<` $name `>`";
  let genVerifyDecl = 1;
}

def ACIR_TypeParamType : ACIR_Type<"TypeParam", "type_param"> {
  let summary = "Module-owned compile-time hardware type parameter";
  let description = [{
    Names one entry in the referenced module or module.import's
    type_parameters. The owner and name identify the formal independently
    of instance names. This is a compile-time placeholder, not a reference
    to mutable storage and not an additional runtime payload kind.

    An instance substitutes its type_arguments into the callee signature.
    Arguments may forward parameters of the enclosing module. Following the
    instance bindings from an executable root must resolve every formal to
    finite bits, enum or struct data; missing, foreign or cyclic bindings are errors.
    Generic definitions retain their formals and are shared by their instances.
  }];
  let parameters = (ins "::mlir::FlatSymbolRefAttr":$owner,
      "::mlir::StringAttr":$name);
  let assemblyFormat = "`<` $owner `,` $name `>`";
  let genVerifyDecl = 1;
}

// This is an ODS constraint, not another IR type or wire wrapper. A hardware
// SSA value already denotes the wire carrying its declared payload.
def ACIR_ScalarHardwareType : AnyTypeOf<[
    ACIR_BitsType, ACIR_EnumType, ACIR_StructType, ACIR_TypeParamType],
    "bits, nominal enum, packed struct, or declared hardware type parameter">;

def ACIR_TableType : ACIR_Type<"Table", "table"> {
  let summary = "Static row-major table of four-state hardware payloads";
  let parameters = (ins "::mlir::ArrayAttr":$shape, "::mlir::Type":$elementType);
  let assemblyFormat = "`<` $shape `,` $elementType `>`";
  let builders = [TypeBuilder<(ins "::mlir::ArrayAttr":$shape,
      "::mlir::Type":$elementType), [{
    ::llvm::SmallVector<::mlir::Attribute> dimensions(shape.begin(), shape.end());
    while (auto nested = ::mlir::dyn_cast<TableType>(elementType)) {
      for (auto dimension : nested.getShape()) dimensions.push_back(dimension);
      elementType = nested.getElementType();
    }
    return $_get($_ctxt, ::mlir::ArrayAttr::get($_ctxt, dimensions), elementType);
  }]>];
  let skipDefaultBuilders = 1;
  let genVerifyDecl = 1;
}

def ACIR_HardwareType : AnyTypeOf<[
    ACIR_BitsType, ACIR_EnumType, ACIR_StructType, ACIR_TypeParamType, ACIR_TableType],
    "scalar hardware payload or static table">;

#endif // ACIR_DIALECT_ACIR_ACIRTYPES_TD

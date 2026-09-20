include "mlir/IR/AttrTypeBase.td"
include "pyc/Dialect/PYC/PYCDialect.td"
include "acir/Dialect/ACIR/ACIRAttributes.td"

class PYC_Attr<string name> : AttrDef<PYCDialect, name>;

def PYC_ProjectionStepAttr : PYC_Attr<"ProjectionStep"> {
  let mnemonic = "projection_step";
  let parameters = (ins "::mlir::Attribute":$value);
  let assemblyFormat = "`<` $value `>`";
  let genVerifyDecl = 1;
}
def PYC_ProjectionFieldAttr : PYC_Attr<"ProjectionField"> {
  let mnemonic = "projection_field";
  let parameters = (ins "::mlir::StringAttr":$name);
  let assemblyFormat = "`<` $name `>`";
  let genVerifyDecl = 1;
}
def PYC_ProjectionTupleElementAttr : PYC_Attr<"ProjectionTupleElement"> {
  let mnemonic = "projection_tuple_element";
  let parameters = (ins "uint64_t":$index);
  let assemblyFormat = "`<` $index `>`";
}
def PYC_ProjectionArrayElementAttr : PYC_Attr<"ProjectionArrayElement"> {
  let mnemonic = "projection_array_element";
  let parameters = (ins "uint64_t":$index);
  let assemblyFormat = "`<` $index `>`";
}
def PYC_ProjectionPathAttr : PYC_Attr<"ProjectionPath"> {
  let mnemonic = "projection_path";
  let parameters = (ins "::mlir::ArrayAttr":$steps);
  let assemblyFormat = "`<` $steps `>`";
  let genVerifyDecl = 1;
}
def PYC_PackedLeafAttr : PYC_Attr<"PackedLeaf"> {
  let mnemonic = "packed_leaf";
  let parameters = (ins "::pyc::ProjectionPathAttr":$path,
                         "::acir::ac::TypeExprAttr":$logicalType,
                         APIntParameter<"arbitrary-precision mathematical integer">:$lsb,
                         APIntParameter<"arbitrary-precision mathematical integer">:$width);
  let assemblyFormat = "`<` $path `,` $logicalType `,` $lsb `,` $width `>`";
  let genVerifyDecl = 1;
}
def PYC_LayoutAttr : PYC_Attr<"Layout"> {
  let mnemonic = "layout";
  let parameters = (ins APIntParameter<"arbitrary-precision mathematical integer">:$width,
                         "::mlir::ArrayAttr":$leaves);
  let assemblyFormat = "`<` $width `,` $leaves `>`";
  let genVerifyDecl = 1;
}
def PYC_PhysicalPortAttr : PYC_Attr<"PhysicalPort"> {
  let mnemonic = "physical_port";
  let parameters = (ins "::mlir::StringAttr":$direction, "uint64_t":$index,
                         "::mlir::TypeAttr":$type, "::mlir::StringAttr":$role,
                         OptionalParameter<"::mlir::IntegerAttr">:$lane,
                         OptionalParameter<"::pyc::LayoutAttr">:$layout);
  let assemblyFormat = "`<` $direction `,` $index `,` $type `,` $role (`lane` $lane^)? (`layout` $layout^)? `>`";
  let genVerifyDecl = 1;
}
def PYC_LogicalPortMappingAttr : PYC_Attr<"LogicalPortMapping"> {
  let mnemonic = "logical_port_mapping";
  let parameters = (ins "::mlir::StringAttr":$direction, "uint64_t":$index,
                         "::mlir::StringAttr":$name,
                         "::acir::ac::TypeExprAttr":$logicalType,
                         "::mlir::ArrayAttr":$carriers,
                         "::acir::ac::SourceProvenanceAttr":$provenance);
  let assemblyFormat = "`<` $direction `,` $index `,` $name `,` $logicalType `,` $carriers `,` $provenance `>`";
  let genVerifyDecl = 1;
}
def PYC_ImplicitControlOriginAttr : PYC_Attr<"ImplicitControlOrigin"> {
  let mnemonic = "implicit_control_origin";
  let parameters = (ins "::mlir::StringAttr":$kind,
                         "::mlir::StringAttr":$source,
                         "::acir::ac::SourceProvenanceAttr":$provenance);
  let assemblyFormat = "`<` $kind `,` $source `,` $provenance `>`";
  let genVerifyDecl = 1;
}
def PYC_ControlPortMappingAttr : PYC_Attr<"ControlPortMapping"> {
  let mnemonic = "control_port_mapping";
  let parameters = (ins "::mlir::StringAttr":$kind,
                         "uint64_t":$physicalInputIndex,
                         "::mlir::TypeAttr":$type,
                         "::pyc::ImplicitControlOriginAttr":$origin);
  let assemblyFormat = "`<` $kind `,` $physicalInputIndex `,` $type `,` $origin `>`";
  let genVerifyDecl = 1;
}
def PYC_ModulePortMappingAttr : PYC_Attr<"ModulePortMapping"> {
  let mnemonic = "module_port_mapping";
  let parameters = (ins "::mlir::ArrayAttr":$controls,
                         "::mlir::ArrayAttr":$logicalPorts,
                         "::mlir::ArrayAttr":$physicalInputs,
                         "::mlir::ArrayAttr":$physicalResults);
  let assemblyFormat = "`<` $controls `,` $logicalPorts `,` $physicalInputs `,` $physicalResults `>`";
  let genVerifyDecl = 1;
}
def PYC_ModuleCaseSignatureAttr : PYC_Attr<"ModuleCaseSignature"> {
  let mnemonic = "module_case_signature";
  let parameters = (ins "::acir::ac::DependentArgumentsAttr":$arguments,
                         "::acir::ac::ModuleInterfaceAttr":$logical,
                         "::mlir::TypeAttr":$physical,
                         "::pyc::ModulePortMappingAttr":$mapping);
  let assemblyFormat = "`<` $arguments `,` $logical `,` $physical `,` $mapping `>`";
  let genVerifyDecl = 1;
}

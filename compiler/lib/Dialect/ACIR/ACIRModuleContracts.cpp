#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/APSInt.h"
using namespace mlir;
namespace acir::ac {
DictionaryAttr getStaticParameterAttr(Builder &builder, StringRef name,
                                      StaticExprAttr defaultValue) {
  NamedAttrList fields;
  fields.append("name", builder.getStringAttr(name));
  fields.append("type", TypeAttr::get(MathIntType::get(builder.getContext())));
  if (defaultValue)
    fields.append("default", defaultValue);
  return fields.getDictionary(builder.getContext());
}
DictionaryAttr getOutputDependencyAttr(Builder &builder, unsigned output,
                                       ArrayRef<unsigned> inputs) {
  auto endpoint = [&](unsigned port) {
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("port", builder.getI64IntegerAttr(port)),
         builder.getNamedAttr("path", builder.getArrayAttr({}))});
  };
  SmallVector<Attribute> deps;
  for (unsigned port : inputs)
    deps.push_back(endpoint(port));
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("output", endpoint(output)),
       builder.getNamedAttr("inputs", builder.getArrayAttr(deps))});
}
} // namespace acir::ac

namespace acir::ac::detail {
LogicalResult verifySourceCallHardwareConstraint(
    Attribute raw, Type physical, const HardwareAnalysis &analysis,
    Operation *lexicalModule, Operation *diagnosticOperation,
    StringRef description) {
  auto error = [&] { return diagnosticOperation->emitOpError(); };
  HardwareBindings bindings;
  bindings.owner = lexicalModule;
  auto value = dyn_cast_or_null<DictionaryAttr>(raw);
  auto kind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto type = value ? value.getAs<TypeAttr>("type") : TypeAttr();
  auto sourceKind =
      value ? value.getAs<StringAttr>("source_kind") : StringAttr();
  if (!kind || kind.getValue() != "hardware" || !type || !sourceKind ||
      !areEquivalentHardwareTypes(type.getValue(), physical))
    return error() << description
                   << " hardware constraint disagrees "
                      "with its physical type";
  StringRef source = sourceKind.getValue();
  bool logical = source == "boolean" || source == "integer";
  if (value.size() != (logical ? 4 : 3) ||
      failed(verifyHardwareTypeScope(type.getValue(), lexicalModule)))
    return error() << description
                   << " hardware constraint fields or "
                      "type scope are invalid";
  if (logical || source == "fixed_bits") {
    if (!isa<BitsType>(physical))
      return error() << description
                     << " bit or logical constraint "
                        "requires BitsType";
  } else if (source == "nominal") {
    if (!isa<StructType, EnumType>(physical))
      return error() << description
                     << " nominal constraint requires "
                        "a struct or enum";
  } else if (source == "table") {
    auto table = dyn_cast<TableType>(physical);
    if (!table || table.getShape().size() != 1 ||
        !isa<BitsType, StructType, EnumType>(table.getElementType()))
      return error() << description
                     << " table constraint requires one "
                        "dimension of bits or nominal elements";
  } else {
    return error() << "unsupported " << description << " source kind";
  }
  // Reuse canonical nominal resolution, static widths and checked layout.
  auto width = analysis.getPackedWidth(physical, bindings, diagnosticOperation);
  if (failed(width))
    return failure();
  if (!*width)
    return error() << description << " payload must have positive width";
  if (!logical)
    return success();
  auto sourceDomain = value.getAs<SourceDomainAttr>("domain");
  if (!sourceDomain ||
      failed(SourceDomainAttr::verify(error, sourceDomain.getValue())))
    return error() << description
                   << " logical constraint requires a "
                      "valid SourceDomain";
  auto fields = sourceDomain.getValue();
  StringRef domainKind = fields.getAs<StringAttr>("kind").getValue();
  if (source == "boolean") {
    if (domainKind != "bool" || *width != 1)
      return error() << description
                     << " Boolean requires a bool domain "
                        "and one-bit physical type";
    return success();
  }
  if (domainKind != "integer")
    return error() << description << " Integer requires an integer domain";
  llvm::APSInt lower(fields.getAs<MathIntAttr>("lower").getCanonicalValue());
  llvm::APSInt upper(fields.getAs<MathIntAttr>("upper").getCanonicalValue());
  // Compare bit lengths instead of constructing a potentially huge 2**W.
  if (!lower.isZero() || upper.isNegative() || !upper.isPowerOf2() ||
      uint64_t(upper.getActiveBits() - 1) != *width)
    return error() << description << " Integer domain must be [0, 2**W)";
  return success();
}

LogicalResult verifyRequiredSourceParameter(Attribute raw,
                                            StringAttr expectedName,
                                            Operation *lexicalModule,
                                            Operation *diagnosticOperation,
                                            StringRef description) {
  auto error = [&] { return diagnosticOperation->emitOpError(); };
  auto parameter = dyn_cast_or_null<DictionaryAttr>(raw);
  auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
  auto binding =
      parameter ? parameter.getAs<StringAttr>("binding") : StringAttr();
  auto defaultValue =
      parameter ? parameter.getAs<DictionaryAttr>("default") : DictionaryAttr();
  auto origin =
      parameter ? parameter.getAs<DictionaryAttr>("origin") : DictionaryAttr();
  auto location = parameter ? parameter.getAs<DictionaryAttr>("location")
                            : DictionaryAttr();
  if (!parameter || parameter.size() != 6 || !name || !expectedName ||
      name != expectedName || !binding ||
      binding.getValue() != "positional_or_keyword" ||
      failed(verifyDefaultStructure(defaultValue, error)) ||
      defaultValue.getAs<BoolAttr>("present").getValue())
    return error() << description
                   << " parameters require matching names, "
                      "positional-or-keyword binding and absent defaults";
  auto sourceOwner =
      lexicalModule->getAttrOfType<DictionaryAttr>("source_owner");
  auto symbol = lexicalModule->getAttrOfType<StringAttr>(
      SymbolTable::getSymbolAttrName());
  if (!symbol || failed(verifySourceOwner(sourceOwner, error)))
    return error() << description
                   << " requires a named lexical module and "
                      "valid SourceOwner";
  auto definition =
      FlatSymbolRefAttr::get(lexicalModule->getContext(), symbol.getValue());
  if (failed(verifyOccurrence(origin, error)) ||
      failed(verifyOriginDefinition(origin, definition, "module parameter",
                                    error)) ||
      failed(verifySourceSpan(location, error)))
    return failure();
  if (location.getAs<StringAttr>("path") !=
      sourceOwner.getAs<StringAttr>("path"))
    return error() << description
                   << " parameter path differs from "
                      "its original SourceOwner";
  return success();
}

LogicalResult verifyModuleSourceCallContract(Operation *owner) {
  unsigned present = 0;
  for (StringRef name : {"ac.return_form", "ac.parameters",
                         "ac.result_constraints", "ac.domain_inputs"})
    present += owner->hasAttr(name);
  if (!present)
    return success();
  auto error = [&] { return owner->emitOpError(); };
  auto form = owner->getAttrOfType<StringAttr>("ac.return_form");
  auto parameters = owner->getAttrOfType<ArrayAttr>("ac.parameters");
  auto results = owner->getAttrOfType<ArrayAttr>("ac.result_constraints");
  auto domain = owner->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto integers = owner->getAttrOfType<ArrayAttr>("parameters");
  auto types = owner->getAttrOfType<ArrayAttr>("type_parameters");
  auto rawType = owner->getAttrOfType<TypeAttr>("function_type");
  auto signature =
      rawType ? dyn_cast<FunctionType>(rawType.getValue()) : FunctionType();
  auto names = owner->getAttrOfType<ArrayAttr>("input_names");
  auto sourceOwner = owner->getAttrOfType<DictionaryAttr>("source_owner");
  auto symbol =
      owner->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  auto package = dyn_cast_or_null<mlir::ModuleOp>(owner->getParentOp());
  auto rootKind = owner->getAttrOfType<StringAttr>("ac.root_kind");
  bool system = rootKind && rootKind.getValue() == "system";
  if (!domain || !integers || !types || !signature || !names ||
      names.size() != signature.getNumInputs() || !symbol || !package ||
      failed(verifySourceOwner(sourceOwner, error)))
    return error() << "source module call contract requires complete physical "
                      "signature metadata";
  HardwareAnalysis analysis(package);
  HardwareBindings bindings;
  bindings.owner = owner;
  auto verifyDomain = [&](size_t count) -> LogicalResult {
    auto clock = domain.getAs<IntegerAttr>("clock");
    auto reset = domain.getAs<IntegerAttr>("reset");
    if (domain.size() != 2 || !clock || !reset ||
        !clock.getType().isSignlessInteger(64) ||
        !reset.getType().isSignlessInteger(64) ||
        clock.getValue().isNegative() || reset.getValue().isNegative() ||
        clock.getValue().getZExtValue() != count ||
        reset.getValue().getZExtValue() != count + 1 ||
        signature.getNumInputs() != count + 2)
      return error() << "source module call domain must explicitly map clock "
                        "and reset after the data inputs";
    StringRef domainNames[] = {"pyc_clk", "pyc_rst"};
    for (auto [index, expected] : llvm::enumerate(domainNames)) {
      auto name = dyn_cast<StringAttr>(names[count + index]);
      if (!name || name.getValue() != expected ||
          !isa<BitsType>(signature.getInput(count + index)))
        return error() << "source module call domain disagrees with physical "
                          "port names or types";
    }
    for (size_t index : {count, count + 1}) {
      auto width =
          analysis.getPackedWidth(signature.getInput(index), bindings, owner);
      if (failed(width))
        return failure();
      if (*width != 1)
        return error() << "source module call domain inputs must be one bit";
    }
    return success();
  };

  if (present == 1) {
    if (domain.empty() || signature.getNumInputs() < 2)
      return error() << "inferred module domain metadata requires clock and "
                        "reset inputs";
    size_t count = signature.getNumInputs() - 2;
    if (failed(verifyDomain(count)))
      return failure();
    if (system &&
        (!integers.empty() || !types.empty() || count != 0 ||
         signature.getNumResults() != 0))
      return error() << "source system requires no authored parameters, "
                        "inputs, or results";
    return success();
  }
  if (present != 4 || system || !form || form.getValue() != "single" ||
      !parameters || !results || results.size() != 1 ||
      !integers.empty() || !types.empty() || signature.getNumResults() != 1)
    return error() << "source module call contract requires a concrete single "
                      "struct result and all four attributes";

  size_t count = parameters.size();
  if (domain.empty()) {
    if (signature.getNumInputs() != count)
      return error() << "source module call input count differs from its "
                        "domain-free physical signature";
  } else if (failed(verifyDomain(count))) {
    return failure();
  }

  for (auto [index, raw] : llvm::enumerate(parameters)) {
    if (failed(verifyRequiredSourceParameter(
            raw, dyn_cast<StringAttr>(names[index]), owner, owner,
            "source module call")))
      return failure();
    auto parameter = cast<DictionaryAttr>(raw);
    if (failed(verifySourceCallHardwareConstraint(
            parameter.get("constraint"), signature.getInput(index), analysis,
            owner, owner, "source module call")))
      return failure();
  }
  if (failed(verifySourceCallHardwareConstraint(
          results[0], signature.getResult(0), analysis, owner, owner,
          "source module call")))
    return failure();
  auto result = cast<DictionaryAttr>(results[0]);
  if (result.getAs<StringAttr>("source_kind").getValue() != "nominal" ||
      !isa<StructType>(signature.getResult(0)))
    return error() << "source module call result must be a nominal struct";
  return success();
}
} // namespace acir::ac::detail

#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
using namespace mlir;
namespace acir::ac {
FailureOr<ModulePortLayout> getModulePortLayout(Operation *declaration) {
  auto error = [&] { return declaration->emitOpError(); };
  auto rawType = declaration->getAttrOfType<TypeAttr>("function_type");
  auto signature =
      rawType ? dyn_cast<FunctionType>(rawType.getValue()) : FunctionType();
  if (!signature)
    return error() << "module port layout requires a physical signature";
  ModulePortLayout layout{signature.getNumInputs(), signature.getNumResults(),
                          ArrayAttr::get(declaration->getContext(), {}), {}, {}};
  Attribute raw = declaration->getAttr("ac.internal_ports");
  if (!raw) {
    if (declaration->hasAttr("ac.input_validity") ||
        declaration->hasAttr("ac.output_validity") ||
        declaration->hasAttr("ac.routed_output_validity"))
      return error()
             << "availability companions require routed or final internal ports";
    layout.inputValidity.assign(layout.sourceInputCount, -1);
    layout.outputValidity.assign(layout.sourceOutputCount, -1);
    return layout;
  }
  auto package = dyn_cast_or_null<mlir::ModuleOp>(declaration->getParentOp());
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  if (!isa<ModuleOp>(declaration) || !stage ||
      (stage.getValue() != "routed" && stage.getValue() != "final") ||
      package->hasAttr("ac.unit_kind"))
    return error() << "ac.internal_ports is permitted only on a real module "
                      "in a routed or final package";
  auto fields = dyn_cast<DictionaryAttr>(raw);
  auto inputs =
      fields ? fields.getAs<IntegerAttr>("source_inputs") : IntegerAttr();
  auto outputs =
      fields ? fields.getAs<IntegerAttr>("source_outputs") : IntegerAttr();
  auto defaults = fields ? fields.getAs<ArrayAttr>("root_inputs") : ArrayAttr();
  auto validCount = [](IntegerAttr value, unsigned bound) {
    return value && value.getType().isSignlessInteger(64) &&
           !value.getValue().isNegative() &&
           value.getValue().getZExtValue() <= bound;
  };
  if (!fields || fields.size() != 3 ||
      !validCount(inputs, signature.getNumInputs()) ||
      !validCount(outputs, signature.getNumResults()) || !defaults)
    return error() << "ac.internal_ports requires exactly source_inputs and "
                      "source_outputs as bounded nonnegative i64 counts and "
                      "root_inputs as an array";
  layout.sourceInputCount = inputs.getValue().getZExtValue();
  layout.sourceOutputCount = outputs.getValue().getZExtValue();
  layout.rootInputDefaults = defaults;
  if (layout.sourceInputCount == signature.getNumInputs() &&
      layout.sourceOutputCount == signature.getNumResults())
    return error() << "ac.internal_ports requires an appended physical port";
  if (defaults.size() != signature.getNumInputs() - layout.sourceInputCount)
    return error() << "ac.internal_ports root_inputs must cover the exact "
                      "physical input suffix";
  HardwareAnalysis analysis(package);
  HardwareBindings bindings;
  bindings.owner = declaration;
  auto validity = [&](StringRef name, unsigned sourceCount, TypeRange types,
                      SmallVectorImpl<int64_t> &ordinals,
                      bool input) -> LogicalResult {
    ordinals.assign(sourceCount, -1);
    auto raw = declaration->getAttr(name);
    if (!raw)
      return success();
    auto array = dyn_cast<ArrayAttr>(raw);
    if (!array || array.size() != sourceCount)
      return error() << name << " must cover the exact source port prefix";
    llvm::DenseSet<int64_t> seen;
    auto domain = declaration->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    auto wiring = declaration->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto incoming = wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
    for (auto [source, element] : llvm::enumerate(array)) {
      auto ordinal = dyn_cast<IntegerAttr>(element);
      if (!ordinal || !ordinal.getType().isSignlessInteger(64) ||
          ordinal.getInt() < -1)
        return error() << name << " entries require i64 -1 or a suffix port ordinal";
      int64_t index = ordinal.getInt();
      if (index == -1)
        continue;
      if (index < sourceCount || static_cast<uint64_t>(index) >= types.size() ||
          !seen.insert(index).second || !isa<BitsType>(types[index]))
        return error() << name << " companions require distinct Bits1 suffix ports";
      auto width = analysis.getPackedWidth(types[index], bindings, declaration);
      if (failed(width))
        return failure();
      if (*width != 1)
        return error() << name << " companions must be one bit";
      if (input && domain)
        for (StringRef pin : {"clock", "reset"})
          if (auto port = domain.getAs<IntegerAttr>(pin);
              port && port.getInt() == static_cast<int64_t>(source))
            return error() << "clock/reset ports cannot acquire availability companions";
      if (input && incoming)
        for (Attribute item : incoming) {
          auto route = dyn_cast<DictionaryAttr>(item);
          if (!route)
            continue; // The wiring verifier diagnoses malformed route records.
          for (StringRef pin : {"data", "enable"})
            if (auto port = route.getAs<IntegerAttr>(pin);
                port && port.getInt() == index)
              return error() << "availability companions cannot alias proposal ports";
        }
      if (input) {
        auto queueWiring = declaration->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
        auto queueIncoming = queueWiring ? queueWiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
        if (queueIncoming) for (Attribute raw : queueIncoming) {
          auto route = dyn_cast<DictionaryAttr>(raw);
          if (!route) continue;
          for (StringRef pin : {"path", "data"})
            if (auto port = route.getAs<IntegerAttr>(pin); port && port.getInt() == index)
              return error() << "availability companions cannot alias queue proposal ports";
        }
      }
      ordinals[source] = index;
    }
    return success();
  };
  if (failed(validity("ac.input_validity", layout.sourceInputCount,
                      signature.getInputs(), layout.inputValidity, true)) ||
      failed(validity("ac.output_validity", layout.sourceOutputCount,
                      signature.getResults(), layout.outputValidity, false)))
    return failure();
  if (auto rawRoutes = declaration->getAttr("ac.routed_output_validity")) {
    auto routes = dyn_cast<ArrayAttr>(rawRoutes);
    if (!routes || routes.empty())
      return error() << "routed output availability requires a nonempty array";
    llvm::DenseSet<int64_t> controls;
    for (int64_t index : layout.outputValidity)
      if (index >= 0)
        controls.insert(index);
    int64_t previous = -1;
    for (Attribute rawRoute : routes) {
      auto route = dyn_cast<DictionaryAttr>(rawRoute);
      auto payload = route ? route.getAs<IntegerAttr>("payload") : IntegerAttr();
      auto control = route ? route.getAs<IntegerAttr>("validity") : IntegerAttr();
      if (!route || route.size() != 2 || !payload || !control ||
          !payload.getType().isSignlessInteger(64) ||
          !control.getType().isSignlessInteger(64) ||
          payload.getInt() < layout.sourceOutputCount ||
          control.getInt() < layout.sourceOutputCount ||
          uint64_t(payload.getInt()) >= signature.getNumResults() ||
          uint64_t(control.getInt()) >= signature.getNumResults() ||
          payload.getInt() <= previous ||
          !controls.insert(control.getInt()).second ||
          !isa<BitsType>(signature.getResult(control.getInt())))
        return error() << "routed availability requires ordered distinct payload "
                          "suffixes and distinct Bits1 companions";
      auto width = analysis.getPackedWidth(signature.getResult(control.getInt()),
                                           bindings, declaration);
      if (failed(width))
        return failure();
      if (*width != 1)
        return error() << "routed availability companions must be one bit";
      previous = payload.getInt();
      layout.routedOutputValidity.push_back({payload.getInt(), control.getInt()});
    }
    for (auto [payload, control] : layout.routedOutputValidity)
      if (controls.contains(payload))
        return error() << "routed payload cannot alias an availability companion";
  }
  for (auto [index, rawDefault] : llvm::enumerate(defaults)) {
    auto value = dyn_cast<IntegerAttr>(rawDefault);
    Type type = signature.getInput(layout.sourceInputCount + index);
    bool available = llvm::is_contained(
        layout.inputValidity, static_cast<int64_t>(layout.sourceInputCount + index));
    if (!value || !value.getType().isSignlessInteger(64) ||
        value.getInt() != (available ? 1 : 0) ||
        !isa<BitsType, EnumType, StructType, TableType>(type))
      return error() << "ac.internal_ports root inputs require finite payloads "
                        "and i64 zero defaults, except declared availability "
                        "companions which require one";
    auto width = analysis.getPackedWidth(type, bindings, declaration);
    if (failed(width))
      return failure();
    if (!*width)
      return error() << "ac.internal_ports root input width must be positive";
  }
  return layout;
}

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
LogicalResult verifyModuleImportSummarySelection(ModuleImportOp imported) {
  auto kind = imported->getAttrOfType<StringAttr>("semantic_kind");
  if (!kind || (kind.getValue() != "hardware" &&
                kind.getValue() != "transactions"))
    return imported.emitOpError()
           << "semantic_kind must be hardware or transactions";
  if (Attribute raw = imported->getAttr("ac.external_contract")) {
    auto contract = dyn_cast<DictionaryAttr>(raw);
    auto matches = [&](StringRef key, StringRef expected) {
      auto value = contract ? contract.getAs<StringAttr>(key) : StringAttr();
      return value && value.getValue() == expected;
    };
    auto effects = contract ? contract.getAs<ArrayAttr>("effects") : ArrayAttr();
    auto latency = contract ? contract.getAs<IntegerAttr>("latency")
                            : IntegerAttr();
    if (kind.getValue() != "hardware" ||
        imported->hasAttr("primitive_kind") ||
        imported->hasAttr("transaction_summary") ||
        imported.getSymName().starts_with("pycircuit.__builtins__."))
      return imported.emitOpError()
             << "ac.external_contract requires an ordinary hardware import "
                "without primitive_kind or transaction_summary";
    if (!contract || contract.size() != 6 || !matches("state", "none") ||
        !effects || !effects.empty() || !latency ||
        !latency.getType().isSignlessInteger(64) ||
        !latency.getValue().isZero() ||
        !matches("availability", "all_inputs") ||
        !matches("value_domain", "four_state") || !matches("failure", "none"))
      return imported.emitOpError()
             << "ac.external_contract requires exactly state = none, effects = "
                "[], latency = 0 : i64, availability = all_inputs, value_domain "
                "= four_state and failure = none";
  }
  if (kind.getValue() == "hardware") {
    if (!imported->getAttrOfType<ArrayAttr>("dependency_summary") ||
        imported->hasAttr("transaction_summary"))
      return imported.emitOpError()
             << "hardware import requires dependency_summary and forbids "
                "transaction_summary";
  } else if (!imported->getAttrOfType<DictionaryAttr>("transaction_summary") ||
             imported->hasAttr("dependency_summary") ||
             imported->hasAttr("primitive_kind")) {
    return imported.emitOpError()
           << "transactions import requires transaction_summary and forbids "
              "dependency_summary and primitive_kind";
  }
  return success();
}

LogicalResult verifySourceCallHardwareConstraint(
    Attribute raw, Type physical, const HardwareAnalysis &analysis,
    Operation *lexicalModule, Operation *diagnosticOperation,
    StringRef description, SourceCallWidthMode widthMode) {
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
  // Generic Module contracts preserve scoped symbolic fixed widths until an
  // actual instance supplies parameters. Closed widths still take the normal
  // positive-width check, including malformed closed expressions.
  if (widthMode == SourceCallWidthMode::SymbolicFixedBits &&
      source == "fixed_bits" &&
      !analysis.isStaticEvaluable(cast<BitsType>(physical).getWidth(), bindings))
    return success();
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

LogicalResult verifyNominalStructField(
    Attribute raw, const HardwareAnalysis &analysis, Operation *lexicalModule,
    Operation *diagnosticOperation) {
  auto field = dyn_cast<DictionaryAttr>(raw);
  auto name = field ? field.getAs<StringAttr>("name") : StringAttr();
  auto type = field ? field.getAs<TypeAttr>("type") : TypeAttr();
  Attribute constraint = field ? field.get("source_constraint") : Attribute();
  if (!field || field.size() != (constraint ? 3 : 2) || !name ||
      name.getValue().empty() || !type ||
      !isa<BitsType, StructType, EnumType, TableType>(type.getValue()))
    return diagnosticOperation->emitOpError()
           << "fields require unique names and finite bits/struct/enum/table "
              "types, with optional Boolean source_constraint";
  if (!constraint)
    return success();
  auto descriptor = dyn_cast<DictionaryAttr>(constraint);
  auto kind = descriptor ? descriptor.getAs<StringAttr>("source_kind")
                         : StringAttr();
  if (!kind || kind.getValue() != "boolean")
    return diagnosticOperation->emitOpError()
           << "Struct field source_constraint requires Boolean source kind";
  return verifySourceCallHardwareConstraint(
      constraint, type.getValue(), analysis, lexicalModule, diagnosticOperation,
      "Struct field");
}

LogicalResult verifyRequiredSourceParameter(Attribute raw,
                                            StringAttr expectedName,
                                            DictionaryAttr sourceOwner,
                                            FlatSymbolRefAttr definition,
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
  if (!definition || failed(verifySourceOwner(sourceOwner, error)))
    return error() << description
                   << " requires a canonical definition and valid SourceOwner";
  if (failed(verifyOccurrence(origin, error)) ||
      failed(verifyOriginDefinition(origin, definition, description,
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

LogicalResult verifyRequiredSourceParameter(Attribute raw,
                                            StringAttr expectedName,
                                            Operation *lexicalModule,
                                            Operation *diagnosticOperation,
                                            StringRef description) {
  auto symbol = lexicalModule->getAttrOfType<StringAttr>(
      SymbolTable::getSymbolAttrName());
  return verifyRequiredSourceParameter(
      raw, expectedName,
      lexicalModule->getAttrOfType<DictionaryAttr>("source_owner"),
      symbol ? FlatSymbolRefAttr::get(lexicalModule->getContext(),
                                     symbol.getValue())
             : FlatSymbolRefAttr(),
      diagnosticOperation, description);
}

LogicalResult verifyModuleSourceCallContract(Operation *owner) {
  auto layout = getModulePortLayout(owner);
  if (failed(layout))
    return failure();
  if (auto raw = owner->getAttr("ac.result_availability")) {
    auto policy = dyn_cast<ArrayAttr>(raw);
    if (!policy || policy.size() != layout->sourceOutputCount)
      return owner->emitOpError()
             << "ac.result_availability must cover the exact source results";
    for (Attribute entry : policy) {
      auto kind = dyn_cast<StringAttr>(entry);
      if (!kind || kind.getValue() != "compiler")
        return owner->emitOpError()
               << "source result availability must be compiler managed";
    }
  }
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
        layout->sourceInputCount != count + 2)
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
    if (domain.empty() || layout->sourceInputCount < 2)
      return error() << "inferred module domain metadata requires clock and "
                        "reset inputs";
    size_t count = layout->sourceInputCount - 2;
    if (failed(verifyDomain(count)))
      return failure();
    if (system && (!integers.empty() || !types.empty() || count != 0 ||
                   layout->sourceOutputCount != 0))
      return error() << "source system requires no authored parameters, "
                        "inputs, or results";
    return success();
  }
  bool single = form && form.getValue() == "single";
  bool tuple = form && form.getValue() == "tuple";
  bool none = form && form.getValue() == "none";
  if (present != 4 || system || (!single && !tuple && !none) || !parameters ||
      !results || !types.empty() ||
      layout->sourceOutputCount != results.size() ||
      (none && !results.empty()) || (tuple && results.empty()) ||
      (single && results.size() != 1))
    return error() << "source module call contract requires matching none, single or "
                      "nonempty tuple results and all four attributes";

  size_t count = parameters.size();
  if (domain.empty()) {
    if (layout->sourceInputCount != count)
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
            owner, owner, "source module call",
            SourceCallWidthMode::SymbolicFixedBits)))
      return failure();
  }
  for (auto [index, result] : llvm::enumerate(results))
    if (failed(verifySourceCallHardwareConstraint(
            result, signature.getResult(index), analysis, owner, owner,
            "source module call", SourceCallWidthMode::SymbolicFixedBits)))
      return failure();
  return success();
}
} // namespace acir::ac::detail

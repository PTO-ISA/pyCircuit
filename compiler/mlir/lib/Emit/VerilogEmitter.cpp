#include "pyc/Emit/VerilogEmitter.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "pyc/Dialect/PYC/PYCAttributes.h"
#include "pyc/Dialect/PYC/PYCOps.h"
#include "pyc/Dialect/PYC/PYCTypes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <optional>
#include <vector>

using namespace mlir;

namespace pyc {
namespace {

static IntegerType leafIntType(Type ty) { return dyn_cast<IntegerType>(ty); }

static std::string vRange(Type ty) {
  // Clocks/resets are treated as 1-bit scalar ports/nets in Verilog.
  if (isa<pyc::ClockType>(ty) || isa<pyc::ResetType>(ty))
    return "";
  auto intTy = dyn_cast<IntegerType>(ty);
  if (!intTy)
    return "";
  if (intTy.getWidth() <= 1)
    return "";
  return "[" + std::to_string(intTy.getWidth() - 1) + ":0]";
}

static std::optional<unsigned> leafWidth(Type ty) {
  auto intTy = leafIntType(ty);
  if (!intTy)
    return std::nullopt;
  return intTy.getWidth();
}

static std::optional<int64_t> flatBitWidth(Type ty) {
  auto width = leafWidth(ty);
  if (!width)
    return std::nullopt;
  return static_cast<int64_t>(*width);
}

static std::string vPortRange(Type ty) {
  auto bits = flatBitWidth(ty);
  if (!bits || *bits <= 1)
    return "";
  return "[" + std::to_string(*bits - 1) + ":0]";
}

/// Build a balanced binary-tree expression from a list of term strings.
/// e.g. {"v[0]","v[1]","v[2]","v[3]"} with "|" → "((v[0] | v[1]) | (v[2] | v[3]))"
static std::string treeReduceExpr(llvm::SmallVectorImpl<std::string> &terms,
                                  const std::string &op) {
  while (terms.size() > 1) {
    llvm::SmallVector<std::string> next;
    for (size_t i = 0; i < terms.size(); i += 2) {
      if (i + 1 < terms.size())
        next.push_back("(" + terms[i] + " " + op + " " + terms[i + 1] + ")");
      else
        next.push_back(terms[i]);
    }
    terms = std::move(next);
  }
  return terms.empty() ? "" : terms[0];
}

struct ZeroCountExpr {
  std::string allZero;
  std::string count;
  unsigned width;
};

static std::string zeroCountExpr(llvm::StringRef input, unsigned inputWidth,
                                 unsigned outputWidth, bool directionLow) {
  llvm::SmallVector<ZeroCountExpr> terms;
  terms.reserve(inputWidth);
  for (unsigned offset = 0; offset < inputWidth; ++offset) {
    const unsigned bit = directionLow ? offset : inputWidth - 1u - offset;
    const std::string bitExpr = input.str() + "[" + std::to_string(bit) + "]";
    terms.push_back({"(~" + bitExpr + ")",
                     "(" + bitExpr + " ? " + std::to_string(outputWidth) +
                         "'d0 : " + std::to_string(outputWidth) + "'d1)",
                     1});
  }
  while (terms.size() > 1) {
    llvm::SmallVector<ZeroCountExpr> next;
    for (size_t index = 0; index < terms.size(); index += 2) {
      if (index + 1 == terms.size()) {
        next.push_back(std::move(terms[index]));
        continue;
      }
      ZeroCountExpr &left = terms[index];
      ZeroCountExpr &right = terms[index + 1];
      next.push_back({"(" + left.allZero + " & " + right.allZero + ")",
                      "(" + left.allZero + " ? (" +
                          std::to_string(outputWidth) + "'d" +
                          std::to_string(left.width) + " + " + right.count +
                          ") : " + left.count + ")",
                      left.width + right.width});
    }
    terms = std::move(next);
  }
  return terms.front().count;
}

static std::string vLiteral(IntegerAttr a, Type dstTy) {
  auto intTy = dyn_cast<IntegerType>(dstTy);
  if (!intTy)
    return "0";
  unsigned w = intTy.getWidth();
  const llvm::APInt v = a.getValue();
  // Decimal formatting is fine for small constants; wide values may exceed 64b
  // and cannot use APInt::getZExtValue().
  if (v.getActiveBits() <= 64) {
    return std::to_string(w) + "'d" + std::to_string(v.getZExtValue());
  }
  llvm::SmallString<256> hex;
  v.toStringUnsigned(hex, /*Radix=*/16);
  if (hex.empty())
    hex = "0";
  return std::to_string(w) + "'h" + hex.str().str();
}

static std::string vZero(Type dstTy) {
  auto intTy = leafIntType(dstTy);
  if (!intTy)
    return "0";
  unsigned w = intTy.getWidth();
  return std::to_string(w) + "'d0";
}

static std::string sanitizeId(llvm::StringRef s) {
  std::string out;
  out.reserve(s.size() + 1);
  auto isAlpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
  auto isDigit = [](char c) { return (c >= '0' && c <= '9'); };
  auto isOk = [&](char c) { return isAlpha(c) || isDigit(c) || c == '_'; };

  for (char c : s)
    out.push_back(isOk(c) ? c : '_');
  if (out.empty() || isDigit(out.front()))
    out.insert(out.begin(), '_');
  return out;
}

static std::string lowerSnakeModuleName(llvm::StringRef source) {
  std::string result;
  bool separator = false;
  bool priorLowerOrDigit = false;
  for (char character : source) {
    const unsigned byte = static_cast<unsigned char>(character);
    if (std::isupper(byte)) {
      if ((!result.empty() && priorLowerOrDigit) || separator)
        if (result.back() != '_')
          result.push_back('_');
      result.push_back(static_cast<char>(std::tolower(byte)));
      separator = false;
      priorLowerOrDigit = false;
    } else if (std::islower(byte) || std::isdigit(byte)) {
      if (separator && !result.empty() && result.back() != '_')
        result.push_back('_');
      result.push_back(character);
      separator = false;
      priorLowerOrDigit = true;
    } else {
      separator = true;
      priorLowerOrDigit = false;
    }
  }
  while (!result.empty() && result.back() == '_')
    result.pop_back();
  if (result.empty())
    return {};
  if (std::isdigit(static_cast<unsigned char>(result.front())))
    result.insert(0, "module_");
  return result;
}

static LogicalResult buildRtlModuleNames(
    ModuleOp module, llvm::StringMap<std::string> &rtlNames) {
  llvm::StringMap<std::string> sourceByRtlName;
  auto add = [&](Operation *operation,
                 llvm::StringRef source) -> LogicalResult {
    std::string rtl = lowerSnakeModuleName(source);
    if (rtl.empty())
      return operation->emitError(
          "RTL module name has no readable lower-snake spelling");
    auto prior = sourceByRtlName.find(rtl);
    if (prior != sourceByRtlName.end() && prior->getValue() != source)
      return operation->emitError()
             << "RTL module name '" << rtl << "' collides between '"
             << prior->getValue() << "' and '" << source << "'";
    sourceByRtlName[rtl] = source.str();
    rtlNames[source] = std::move(rtl);
    return success();
  };
  for (func::FuncOp function : module.getOps<func::FuncOp>())
    if (failed(add(function, function.getSymName())))
      return failure();
  for (pyc::FamilyOp family : module.getOps<pyc::FamilyOp>())
    if (failed(add(family, family.getSymName())))
      return failure();
  return success();
}

static std::string obligationAssertionLabel(llvm::StringRef id) {
  std::string label = "obligation";
  bool separator = true;
  for (char character : id) {
    const char lowered = static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
    if ((lowered >= 'a' && lowered <= 'z') ||
        (lowered >= '0' && lowered <= '9')) {
      if (separator && label.back() != '_')
        label.push_back('_');
      label.push_back(lowered);
      separator = false;
    } else {
      separator = true;
    }
  }
  return label;
}

static std::string assertionDiagnostic(pyc::AssertOp assertion) {
  std::string message = "pyc.assert failed";
  if (auto attr = assertion.getMsgAttr())
    message = attr.getValue().str();
  auto id = assertion.getObligationIdAttr();
  if (!id)
    return message;
  std::string diagnostic =
      "architecture_obligation id=" + id.getValue().str() + " kind=" +
      assertion.getObligationKindAttr().getValue().str() + " severity=" +
      assertion.getSeverityAttr().getValue().str() + " sampling=" +
      assertion.getSamplingKindAttr().getValue().str() + "/" +
      assertion.getSamplingEdgeAttr().getValue().str() + "@" +
      assertion.getSampleAnchorAttr().getValue().str() + " source=" +
      assertion.getSourceAttr().getValue().str();
  if (auto identifiers = assertion.getNdfIdsAttr(); identifiers && !identifiers.empty()) {
    diagnostic += " ndf=[";
    for (auto [index, raw] : llvm::enumerate(identifiers)) {
      if (index)
        diagnostic += ',';
      diagnostic += cast<StringAttr>(raw).getValue();
    }
    diagnostic += ']';
  }
  return diagnostic + ": " + message;
}

struct NameTable {
  DenseMap<Value, std::string> names;
  llvm::StringMap<unsigned> used;
  int next = 0;

  std::string unique(std::string base) {
    unsigned &n = used[base];
    n++;
    if (n == 1)
      return base;
    return base + "_" + std::to_string(n);
  }

  std::string get(Value v) {
    if (auto it = names.find(v); it != names.end())
      return it->second;
    if (Operation *def = v.getDefiningOp()) {
      if (auto nAttr = def->getAttrOfType<StringAttr>("pyc.name")) {
        std::string cand = unique(sanitizeId(nAttr.getValue()));
        names.try_emplace(v, cand);
        return cand;
      }
      // Fall back to op-based names for readability (instead of v1/v2/...).
      std::string base = sanitizeId(def->getName().getStringRef());
      if (base.empty())
        base = "v";
      base += "_" + std::to_string(++next);
      std::string cand = unique(base);
      names.try_emplace(v, cand);
      return cand;
    }
    std::string n = unique("arg_" + std::to_string(++next));
    names.try_emplace(v, n);
    return n;
  }
};

static std::string getPortName(func::FuncOp f, unsigned idx, bool isResult) {
  std::string raw;
  if (!isResult) {
    if (auto names = f->getAttrOfType<ArrayAttr>("arg_names")) {
      if (idx < names.size())
        if (auto s = dyn_cast<StringAttr>(names[idx]))
          raw = s.getValue().str();
    }
    if (raw.empty())
      raw = "arg" + std::to_string(idx);
    return sanitizeId(raw);
  }
  if (auto names = f->getAttrOfType<ArrayAttr>("result_names")) {
    if (idx < names.size())
      if (auto s = dyn_cast<StringAttr>(names[idx]))
        raw = s.getValue().str();
  }
  if (raw.empty())
    raw = "out" + std::to_string(idx);
  return sanitizeId(raw);
}

static void computeUniquePortNames(func::FuncOp f, std::vector<std::string> &inNames, std::vector<std::string> &outNames) {
  NameTable nt;
  inNames.clear();
  outNames.clear();
  auto fTy = f.getFunctionType();
  unsigned numInputs = fTy.getNumInputs();
  unsigned numResults = fTy.getNumResults();
  inNames.reserve(numInputs);
  outNames.reserve(numResults);

  for (unsigned i = 0; i < numInputs; ++i) {
    inNames.push_back(nt.unique(getPortName(f, static_cast<unsigned>(i), /*isResult=*/false)));
  }
  for (unsigned i = 0; i < numResults; ++i) {
    outNames.push_back(nt.unique(getPortName(f, i, /*isResult=*/true)));
  }
}

static LogicalResult computeCasePortNames(pyc::ModuleCaseOp moduleCase,
                                          std::vector<std::string> &inNames,
                                          std::vector<std::string> &outNames) {
  auto signature = moduleCase.getSignature();
  auto functionType = cast<FunctionType>(signature.getPhysical().getValue());
  inNames.assign(functionType.getNumInputs(), std::string());
  outNames.assign(functionType.getNumResults(), std::string());
  NameTable names;
  for (ControlPortMappingAttr control :
       signature.getMapping().getControls().getAsRange<ControlPortMappingAttr>()) {
    std::string name = control.getKind().getValue() == "clock" ? "clk" : "rst";
    inNames[control.getPhysicalInputIndex()] = names.unique(name);
  }
  for (LogicalPortMappingAttr logical : signature.getMapping()
                                                   .getLogicalPorts()
                                                   .getAsRange<LogicalPortMappingAttr>()) {
    uint64_t lanes = 0;
    for (PhysicalPortAttr carrier :
         logical.getCarriers().getAsRange<PhysicalPortAttr>())
      if (carrier.getLane())
        lanes = std::max(lanes,
                         carrier.getLane().getValue().getZExtValue() + 1);
    for (PhysicalPortAttr carrier :
         logical.getCarriers().getAsRange<PhysicalPortAttr>()) {
      std::string name = sanitizeId(logical.getName().getValue());
      StringRef role = carrier.getRole().getValue();
      if (role == "queue_valid")
        name += "_valid";
      else if (role == "queue_data")
        name += "_data";
      else if (role == "queue_ready")
        name += "_ready";
      if (carrier.getLane() && lanes > 1)
        name += "_" + std::to_string(carrier.getLane().getValue().getZExtValue());
      auto &ports = carrier.getDirection().getValue() == "input" ? inNames
                                                                  : outNames;
      ports[carrier.getIndex()] = names.unique(name);
    }
  }
  if (llvm::any_of(inNames, [](const std::string &name) { return name.empty(); }) ||
      llvm::any_of(outNames, [](const std::string &name) { return name.empty(); }))
    return moduleCase.emitError("verified case mapping has an unnamed physical carrier");
  return success();
}

static std::optional<std::string> verilogStaticRawValue(Attribute value) {
  if (auto boolean = dyn_cast<acir::ac::StaticBoolValueAttr>(value))
    return std::string(boolean.getValue() ? "1'b1" : "1'b0");
  if (auto integer = dyn_cast<acir::ac::StaticIntValueAttr>(value)) {
    llvm::SmallString<64> spelling;
    integer.getValue().getValue().toString(
        spelling, 10, integer.getType().getIsSigned());
    return spelling.str().str();
  }
  if (auto enumeration = dyn_cast<acir::ac::StaticEnumValueAttr>(value))
    return sanitizeId(enumeration.getDeclaration().getValue()) + "_" +
           sanitizeId(enumeration.getMember().getValue());
  if (auto config = dyn_cast<acir::ac::StaticConfigValueAttr>(value)) {
    std::string result =
        sanitizeId(config.getDeclaration().getValue()) + "'{";
    bool first = true;
    for (acir::ac::StaticConfigFieldValueAttr field :
         config.getFields().getFields().getAsRange<
             acir::ac::StaticConfigFieldValueAttr>()) {
      auto fieldValue = verilogStaticRawValue(field.getValue());
      if (!fieldValue)
        return std::nullopt;
      if (!first)
        result += ", ";
      first = false;
      result += *fieldValue;
    }
    result += "}";
    return result;
  }
  return std::nullopt;
}

static std::optional<std::string>
verilogStaticValue(acir::ac::StaticValueAttr wrapped) {
  return verilogStaticRawValue(wrapped.getValue());
}

static std::optional<std::string>
verilogCaseCondition(acir::ac::StaticArgumentsAttr arguments) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  bool first = true;
  for (acir::ac::StaticArgumentAttr argument :
       arguments.getArguments().getAsRange<acir::ac::StaticArgumentAttr>()) {
    auto value = verilogStaticValue(argument.getValue());
    if (!value)
      return std::nullopt;
    if (!first)
      stream << " && ";
    first = false;
    stream << "(" << sanitizeId(argument.getName().getValue()) << " == "
           << *value << ")";
  }
  return first ? std::optional<std::string>("1'b1")
               : std::optional<std::string>(stream.str());
}

static std::optional<std::string>
verilogInstanceParameters(acir::ac::StaticArgumentsAttr arguments) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  bool first = true;
  for (acir::ac::StaticArgumentAttr argument :
       arguments.getArguments().getAsRange<acir::ac::StaticArgumentAttr>()) {
    auto value = verilogStaticValue(argument.getValue());
    if (!value)
      return std::nullopt;
    if (!first)
      stream << ", ";
    first = false;
    stream << "." << sanitizeId(argument.getName().getValue()) << "(" << *value
           << ")";
  }
  return stream.str();
}

static std::optional<std::string> verilogStaticType(Attribute type) {
  if (isa<acir::ac::StaticBoolTypeAttr>(type))
    return std::string("bit");
  if (auto integer = dyn_cast<acir::ac::StaticIntTypeAttr>(type)) {
    std::string result = "logic ";
    if (integer.getIsSigned())
      result += "signed ";
    result += "[" + std::to_string(integer.getWidth() - 1) + ":0]";
    return result;
  }
  if (auto enumeration = dyn_cast<acir::ac::StaticEnumTypeAttr>(type))
    return sanitizeId(enumeration.getDeclaration().getValue());
  if (auto config = dyn_cast<acir::ac::StaticConfigTypeAttr>(type))
    return sanitizeId(config.getDeclaration().getValue());
  return std::nullopt;
}

static LogicalResult emitVerilogNominalDeclarations(ModuleOp module,
                                                     raw_ostream &os) {
  llvm::StringMap<Attribute> emittedTypes;
  for (pyc::FamilyOp family : module.getOps<pyc::FamilyOp>()) {
    for (acir::ac::StaticParameterAttr parameter :
         family.getSchema()
             .getParameters()
             .getParameters()
             .getAsRange<acir::ac::StaticParameterAttr>()) {
      Attribute type = parameter.getType().getValue();
      if (auto enumeration = dyn_cast<acir::ac::StaticEnumTypeAttr>(type)) {
        StringRef name = enumeration.getDeclaration().getValue();
        if (auto previous = emittedTypes.find(name);
            previous != emittedTypes.end()) {
          if (previous->second != type)
            return family.emitError(
                "nominal enum parameter definitions disagree");
          continue;
        }
        acir::ac::EnumOp declaration;
        for (acir::ac::TypeScopeOp scope :
             module.getOps<acir::ac::TypeScopeOp>())
          for (acir::ac::EnumOp candidate :
               scope.getBody().front().getOps<acir::ac::EnumOp>())
            if (candidate.getSymName() == name)
              declaration = candidate;
        if (!declaration)
          return family.emitError("nominal enum definition is unavailable: ")
                 << name;
        emittedTypes.try_emplace(name, type);
        uint64_t width = declaration.getEncodingWidth().value_or(32);
        os << "typedef enum logic [" << (width - 1) << ":0] {\n";
        ArrayAttr values = declaration.getValuesAttr();
        for (auto [index, rawMember] :
             llvm::enumerate(declaration.getEnumerants())) {
          os << "  " << sanitizeId(name) << "_"
             << sanitizeId(cast<StringAttr>(rawMember).getValue()) << " = "
             << width << "'d";
          if (values) {
            llvm::SmallString<64> spelling;
            cast<IntegerAttr>(values[index]).getValue().toStringUnsigned(
                spelling, 10);
            os << spelling;
          } else {
            os << index;
          }
          os << (index + 1 == declaration.getEnumerants().size() ? "\n"
                                                                  : ",\n");
        }
        os << "} " << sanitizeId(name) << ";\n\n";
        continue;
      }
      auto config = dyn_cast<acir::ac::StaticConfigTypeAttr>(type);
      if (!config)
        continue;
      StringRef name = config.getDeclaration().getValue();
      if (auto previous = emittedTypes.find(name);
          previous != emittedTypes.end()) {
        if (previous->second != type)
          return family.emitError(
              "nominal config parameter definitions disagree");
        continue;
      }
      emittedTypes.try_emplace(name, type);
      os << "typedef struct packed {\n";
      for (acir::ac::StaticConfigFieldAttr field :
           config.getFields().getFields().getAsRange<
               acir::ac::StaticConfigFieldAttr>()) {
        auto fieldType = verilogStaticType(field.getType());
        if (!fieldType)
          return family.emitError(
              "Verilog config field uses an unsupported static type");
        os << "  " << *fieldType << " "
           << sanitizeId(field.getName().getValue()) << ";\n";
      }
      os << "} " << sanitizeId(name) << ";\n\n";
    }
  }
  return success();
}

// Emit a single combinational assignment for the common scalar/elementwise op
// set shared by the pyc.comb region path and the top-level netlist path.
// Returns std::nullopt when `op` is not one of these ops (the caller handles
// container-specific ops such as pyc.assert / pyc.assign / pyc.comb).
static void emitConnectAssign(llvm::StringRef lhs, llvm::StringRef rhs, Type ty, raw_ostream &os);
static std::optional<LogicalResult> emitScalarOpAssign(Operation &op, raw_ostream &os, NameTable &nt) {
  if (auto c = dyn_cast<pyc::ConstantOp>(op)) {
    os << "assign " << nt.get(c.getResult()) << " = " << vLiteral(c.getValueAttr(), c.getType()) << ";\n";
    return success();
  }
  if (auto a = dyn_cast<pyc::AliasOp>(op)) {
    os << "assign " << nt.get(a.getResult()) << " = " << nt.get(a.getIn()) << ";\n";
    return success();
  }
  if (auto ra = dyn_cast<pyc::ResetActiveOp>(op)) {
    os << "assign " << nt.get(ra.getActive()) << " = " << nt.get(ra.getRst()) << ";\n";
    return success();
  }
  if (auto a = dyn_cast<pyc::AddOp>(op)) {
    os << "assign " << nt.get(a.getResult()) << " = (" << nt.get(a.getLhs()) << " + " << nt.get(a.getRhs()) << ");\n";
    return success();
  }
  if (auto s = dyn_cast<pyc::SubOp>(op)) {
    os << "assign " << nt.get(s.getResult()) << " = (" << nt.get(s.getLhs()) << " - " << nt.get(s.getRhs()) << ");\n";
    return success();
  }
  if (auto m = dyn_cast<pyc::MulOp>(op)) {
    os << "assign " << nt.get(m.getResult()) << " = (" << nt.get(m.getLhs()) << " * " << nt.get(m.getRhs()) << ");\n";
    return success();
  }
  if (auto d = dyn_cast<pyc::UdivOp>(op)) {
    os << "assign " << nt.get(d.getResult()) << " = (" << nt.get(d.getRhs()) << " == " << vZero(d.getRhs().getType())
       << " ? " << vZero(d.getResult().getType()) << " : (" << nt.get(d.getLhs()) << " / " << nt.get(d.getRhs())
       << "));\n";
    return success();
  }
  if (auto r = dyn_cast<pyc::UremOp>(op)) {
    os << "assign " << nt.get(r.getResult()) << " = (" << nt.get(r.getRhs()) << " == " << vZero(r.getRhs().getType())
       << " ? " << vZero(r.getResult().getType()) << " : (" << nt.get(r.getLhs()) << " % " << nt.get(r.getRhs())
       << "));\n";
    return success();
  }
  if (auto d = dyn_cast<pyc::SdivOp>(op)) {
    os << "assign " << nt.get(d.getResult()) << " = (" << nt.get(d.getRhs()) << " == " << vZero(d.getRhs().getType())
       << " ? $signed(" << vZero(d.getResult().getType()) << ") : ($signed(" << nt.get(d.getLhs()) << ") / $signed("
       << nt.get(d.getRhs()) << ")));\n";
    return success();
  }
  if (auto r = dyn_cast<pyc::SremOp>(op)) {
    os << "assign " << nt.get(r.getResult()) << " = (" << nt.get(r.getRhs()) << " == " << vZero(r.getRhs().getType())
       << " ? $signed(" << vZero(r.getResult().getType()) << ") : ($signed(" << nt.get(r.getLhs()) << ") % $signed("
       << nt.get(r.getRhs()) << ")));\n";
    return success();
  }
  if (auto m = dyn_cast<pyc::SelectOp>(op)) {
    os << "assign " << nt.get(m.getResult()) << " = (" << nt.get(m.getSel()) << " ? " << nt.get(m.getA()) << " : "
       << nt.get(m.getB()) << ");\n";
    return success();
  }
  if (auto s = dyn_cast<arith::SelectOp>(op)) {
    if (!s.getCondition().getType().isInteger(1))
      return {s.emitError("verilog emitter only supports arith.select with i1 condition")};
    os << "assign " << nt.get(s.getResult()) << " = (" << nt.get(s.getCondition()) << " ? "
       << nt.get(s.getTrueValue()) << " : " << nt.get(s.getFalseValue()) << ");\n";
    return success();
  }
  if (auto a = dyn_cast<pyc::AndOp>(op)) {
    os << "assign " << nt.get(a.getResult()) << " = (" << nt.get(a.getLhs()) << " & " << nt.get(a.getRhs()) << ");\n";
    return success();
  }
  if (auto o = dyn_cast<pyc::OrOp>(op)) {
    os << "assign " << nt.get(o.getResult()) << " = (" << nt.get(o.getLhs()) << " | " << nt.get(o.getRhs()) << ");\n";
    return success();
  }
  if (auto x = dyn_cast<pyc::XorOp>(op)) {
    os << "assign " << nt.get(x.getResult()) << " = (" << nt.get(x.getLhs()) << " ^ " << nt.get(x.getRhs()) << ");\n";
    return success();
  }
  if (auto n = dyn_cast<pyc::NotOp>(op)) {
    os << "assign " << nt.get(n.getResult()) << " = (~" << nt.get(n.getIn()) << ");\n";
    return success();
  }
  if (auto cmp = dyn_cast<pyc::CmpOp>(op)) {
    os << "assign " << nt.get(cmp.getResult()) << " = (";
    if (cmp.getPredicate() == "slt")
      os << "$signed(" << nt.get(cmp.getLhs()) << ") < $signed(" << nt.get(cmp.getRhs()) << ")";
    else
      os << nt.get(cmp.getLhs()) << (cmp.getPredicate() == "eq" ? " == " : " < ")
         << nt.get(cmp.getRhs());
    os << ");\n";
    return success();
  }
  if (auto t = dyn_cast<pyc::TruncOp>(op)) {
    auto outTy = leafIntType(t.getResult().getType());
    if (!outTy)
      return {t.emitError("verilog emitter only supports integer trunc")};
    unsigned w = outTy.getWidth();
    if (w == 1)
      os << "assign " << nt.get(t.getResult()) << " = " << nt.get(t.getIn()) << "[0];\n";
    else
      os << "assign " << nt.get(t.getResult()) << " = " << nt.get(t.getIn()) << "[" << (w - 1) << ":0];\n";
    return success();
  }
  if (auto z = dyn_cast<pyc::ZextOp>(op)) {
    auto inTy = leafIntType(z.getIn().getType());
    auto outTy = leafIntType(z.getResult().getType());
    if (!inTy || !outTy)
      return {z.emitError("verilog emitter only supports integer zext")};
    unsigned iw = inTy.getWidth();
    unsigned ow = outTy.getWidth();
    if (ow == iw)
      os << "assign " << nt.get(z.getResult()) << " = " << nt.get(z.getIn()) << ";\n";
    else
      os << "assign " << nt.get(z.getResult()) << " = {{" << (ow - iw) << "{1'b0}}, " << nt.get(z.getIn()) << "};\n";
    return success();
  }
  if (auto s = dyn_cast<pyc::SextOp>(op)) {
    auto inTy = leafIntType(s.getIn().getType());
    auto outTy = leafIntType(s.getResult().getType());
    if (!inTy || !outTy)
      return {s.emitError("verilog emitter only supports integer sext")};
    unsigned iw = inTy.getWidth();
    unsigned ow = outTy.getWidth();
    if (ow == iw)
      os << "assign " << nt.get(s.getResult()) << " = " << nt.get(s.getIn()) << ";\n";
    else
      os << "assign " << nt.get(s.getResult()) << " = {{" << (ow - iw) << "{" << nt.get(s.getIn()) << "["
         << (iw - 1) << "]}}, " << nt.get(s.getIn()) << "};\n";
    return success();
  }
  if (auto ex = dyn_cast<pyc::ExtractOp>(op)) {
    auto inTy = leafIntType(ex.getIn().getType());
    auto outTy = leafIntType(ex.getResult().getType());
    if (!inTy || !outTy)
      return {ex.emitError("verilog emitter only supports integer extract")};
    unsigned ow = outTy.getWidth();
    std::int64_t lsb = ex.getLsbAttr().getInt();
    if (ow == 1)
      os << "assign " << nt.get(ex.getResult()) << " = " << nt.get(ex.getIn()) << "[" << lsb << "];\n";
    else
      os << "assign " << nt.get(ex.getResult()) << " = " << nt.get(ex.getIn()) << "[" << (lsb + ow - 1) << ":"
         << lsb << "];\n";
    return success();
  }
  if (auto sh = dyn_cast<pyc::ShlOp>(op)) {
    os << "assign " << nt.get(sh.getResult()) << " = (" << nt.get(sh.getIn()) << " << " << nt.get(sh.getAmount())
       << ");\n";
    return success();
  }
  if (auto sh = dyn_cast<pyc::LshrOp>(op)) {
    os << "assign " << nt.get(sh.getResult()) << " = (" << nt.get(sh.getIn()) << " >> " << nt.get(sh.getAmount())
       << ");\n";
    return success();
  }
  if (auto sh = dyn_cast<pyc::AshrOp>(op)) {
    os << "assign " << nt.get(sh.getResult()) << " = ($signed(" << nt.get(sh.getIn()) << ") >>> "
       << nt.get(sh.getAmount()) << ");\n";
    return success();
  }
  if (auto c = dyn_cast<pyc::ConcatOp>(op)) {
    os << "assign " << nt.get(c.getResult()) << " = {";
    for (auto [i, v] : llvm::enumerate(c.getInputs())) {
      if (i)
        os << ", ";
      os << nt.get(v);
    }
    os << "};\n";
    return success();
  }
  if (auto priority = dyn_cast<pyc::PriorityEncodeOp>(op)) {
    auto inputType = leafIntType(priority.getIn().getType());
    auto indexType = leafIntType(priority.getIndex().getType());
    if (!inputType || !indexType)
      return {priority.emitError(
          "verilog emitter requires integer priority_encode types")};
    const unsigned inputWidth = inputType.getWidth();
    const unsigned indexWidth = indexType.getWidth();
    const std::string input = nt.get(priority.getIn());
    std::string indexExpression = std::to_string(indexWidth) + "'d0";
    if (priority.getOrder() == "low") {
      for (unsigned offset = 0; offset < inputWidth; ++offset) {
        const unsigned bit = inputWidth - 1u - offset;
        indexExpression = input + "[" + std::to_string(bit) + "] ? " +
                          std::to_string(indexWidth) + "'d" +
                          std::to_string(bit) + " : (" + indexExpression + ")";
      }
    } else {
      for (unsigned bit = 0; bit < inputWidth; ++bit) {
        indexExpression = input + "[" + std::to_string(bit) + "] ? " +
                          std::to_string(indexWidth) + "'d" +
                          std::to_string(bit) + " : (" + indexExpression + ")";
      }
    }
    os << "assign " << nt.get(priority.getIndex()) << " = " << indexExpression
       << ";\n";
    os << "assign " << nt.get(priority.getValid()) << " = |" << input << ";\n";
    return success();
  }
  if (auto popcount = dyn_cast<pyc::PopcountOp>(op)) {
    auto inputType = leafIntType(popcount.getIn().getType());
    auto outputType = leafIntType(popcount.getCount().getType());
    if (!inputType || !outputType)
      return {popcount.emitError(
          "verilog emitter requires integer popcount types")};
    const unsigned inputWidth = inputType.getWidth();
    const unsigned outputWidth = outputType.getWidth();
    const std::string input = nt.get(popcount.getIn());
    llvm::SmallVector<std::string> terms;
    for (unsigned bit = 0; bit < inputWidth; ++bit) {
      terms.push_back("{{" + std::to_string(outputWidth - 1) + "{1'b0}}, " +
                      input + '[' + std::to_string(bit) + "]}");
    }
    os << "assign " << nt.get(popcount.getCount()) << " = "
       << treeReduceExpr(terms, "+") << ";\n";
    return success();
  }
  if (auto countZeros = dyn_cast<pyc::CountZerosOp>(op)) {
    auto inputType = leafIntType(countZeros.getIn().getType());
    auto outputType = leafIntType(countZeros.getCount().getType());
    if (!inputType || !outputType)
      return {countZeros.emitError(
          "verilog emitter requires integer count_zeros types")};
    os << "assign " << nt.get(countZeros.getCount()) << " = "
       << zeroCountExpr(nt.get(countZeros.getIn()), inputType.getWidth(),
                        outputType.getWidth(),
                        countZeros.getDirection() == "trailing")
       << ";\n";
    return success();
  }
  if (auto selected = dyn_cast<pyc::RtlCombOp>(op)) {
    auto module = selected->getAttrOfType<StringAttr>("module");
    auto parameters = selected->getAttrOfType<DictionaryAttr>("parameters");
    auto inputPorts = selected->getAttrOfType<ArrayAttr>("input_ports");
    auto outputPorts = selected->getAttrOfType<ArrayAttr>("output_ports");
    if (!module || !parameters || !inputPorts || !outputPorts)
      return {selected.emitError("selected RTL metadata is incomplete")};
    os << module.getValue() << " #(";
    for (auto [index, parameter] : llvm::enumerate(parameters)) {
      if (index)
        os << ", ";
      auto value = dyn_cast<IntegerAttr>(parameter.getValue());
      if (!value)
        return {selected.emitError("selected RTL parameters must be integer")};
      os << "." << parameter.getName().strref() << "(" << value.getInt() << ")";
    }
    os << ") rtl_" << nt.get(selected.getOutputs().front()) << " (";
    bool first = true;
    for (auto [port, value] : llvm::zip(inputPorts, selected.getInputs())) {
      if (!first)
        os << ", ";
      first = false;
      os << "." << cast<StringAttr>(port).getValue() << "(" << nt.get(value)
         << ")";
    }
    for (auto [port, value] : llvm::zip(outputPorts, selected.getOutputs())) {
      if (!first)
        os << ", ";
      first = false;
      os << "." << cast<StringAttr>(port).getValue() << "(" << nt.get(value)
         << ")";
    }
    os << ");\n";
    return success();
  }
  return std::nullopt;
}

static LogicalResult emitNetlistOp(Operation &op, raw_ostream &os, NameTable &nt) {
  std::optional<LogicalResult> handled = emitScalarOpAssign(op, os, nt);
  if (!handled)
    return op.emitError("internal error: missing verilog emission handler");
  return *handled;
}

static void emitConnectAssign(llvm::StringRef lhs, llvm::StringRef rhs, Type, raw_ostream &os) {
  os << "assign " << lhs << " = " << rhs << ";\n";
}

static LogicalResult emitComb(pyc::CombOp comb, raw_ostream &os, NameTable &nt) {
  if (comb.getBody().empty())
    return comb.emitError("pyc.comb must have a non-empty region");
  Block &b = comb.getBody().front();
  if (b.getNumArguments() != comb.getNumOperands())
    return comb.emitError("pyc.comb region block args must match input operand count");

  for (auto [i, arg] : llvm::enumerate(b.getArguments()))
    nt.names.try_emplace(arg, nt.get(comb.getInputs()[i]));

  for (Operation &op : b) {
    if (isa<pyc::YieldOp>(op))
      break;
    if (failed(emitNetlistOp(op, os, nt)))
      return failure();
  }

  auto yield = dyn_cast_or_null<pyc::YieldOp>(b.getTerminator());
  if (!yield)
    return comb.emitError("pyc.comb must terminate with pyc.yield");
  if (yield.getNumOperands() != comb.getNumResults())
    return comb.emitError("pyc.yield operand count must match pyc.comb results");

  for (auto [i, v] : llvm::enumerate(yield.getOperands()))
    emitConnectAssign(nt.get(comb.getResult(i)), nt.get(v), comb.getResult(i).getType(), os);

  return success();
}

struct NetDecl {
  std::string name;
  Type ty;
  std::string comment;
};

static std::string opSortKey(Operation *op, NameTable &nt) {
  if (auto assertion = dyn_cast<pyc::AssertOp>(op)) {
    if (auto id = assertion.getObligationIdAttr())
      return "assert:" + id.getValue().str();
    if (auto message = assertion.getMsgAttr())
      return "assert:" + message.getValue().str();
    return "assert";
  }
  if (auto a = dyn_cast<pyc::AssignOp>(op))
    return nt.get(a.getDst());
  if (auto mem = dyn_cast<pyc::ByteMemOp>(op)) {
    if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
      return sanitizeId(nameAttr.getValue());
    return nt.get(mem.getRdata());
  }
  if (auto mem = dyn_cast<pyc::SyncMemOp>(op)) {
    if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
      return sanitizeId(nameAttr.getValue());
    return nt.get(mem.getRdata());
  }
  if (auto mem = dyn_cast<pyc::SyncMemDPOp>(op)) {
    if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
      return sanitizeId(nameAttr.getValue());
    return nt.get(mem.getRdata0());
  }
  if (!op->getResults().empty())
    return nt.get(op->getResult(0));
  return "";
}

static bool topoSortCombOps(ArrayRef<Operation *> ops, NameTable &nt, llvm::SmallVectorImpl<Operation *> &ordered) {
  ordered.clear();
  if (ops.empty())
    return true;

  llvm::SmallVector<std::string> nodeKey;
  nodeKey.reserve(ops.size());
  llvm::DenseMap<Operation *, unsigned> nodeIndex;
  nodeIndex.reserve(ops.size());
  for (auto [i, op] : llvm::enumerate(ops)) {
    nodeIndex.try_emplace(op, static_cast<unsigned>(i));
    nodeKey.push_back(opSortKey(op, nt));
  }

  llvm::DenseMap<Value, unsigned> valueProducer;
  llvm::DenseMap<Value, unsigned> wireAssign;
  llvm::DenseMap<Value, unsigned> wireAssignCount;

  for (auto [idx, op] : llvm::enumerate(ops)) {
    for (Value r : op->getResults())
      valueProducer.try_emplace(r, static_cast<unsigned>(idx));

    if (auto a = dyn_cast<pyc::AssignOp>(*op)) {
      Value dst = a.getDst();
      unsigned &cnt = wireAssignCount[dst];
      cnt++;
      if (cnt == 1)
        wireAssign[dst] = static_cast<unsigned>(idx);
    }
  }

  // Verilog does not support multiple continuous drivers for one net.
  for (auto &it : wireAssignCount) {
    if (it.second > 1)
      return false;
  }
  for (auto &it : wireAssign)
    valueProducer[it.first] = it.second;

  llvm::SmallVector<llvm::SmallVector<unsigned>> succ(ops.size());
  llvm::SmallVector<unsigned> indeg(ops.size(), 0);

  for (auto it : llvm::enumerate(ops)) {
    unsigned idx = it.index();
    Operation *op = it.value();

    llvm::SmallDenseSet<unsigned, 8> deps;
    auto addDep = [&](Value v) {
      auto it = valueProducer.find(v);
      if (it == valueProducer.end())
        return;
      unsigned p = it->second;
      if (p == idx)
        return;
      deps.insert(p);
    };

    if (auto a = dyn_cast<pyc::AssignOp>(*op)) {
      addDep(a.getSrc());
    } else {
      for (Value v : op->getOperands())
        addDep(v);
    }

    indeg[idx] = static_cast<unsigned>(deps.size());
    for (unsigned p : deps)
      succ[p].push_back(static_cast<unsigned>(idx));
  }

  auto cmp = [&](unsigned a, unsigned b) { return nodeKey[a] > nodeKey[b]; };
  std::vector<unsigned> heap;
  heap.reserve(ops.size());
  for (unsigned i = 0; i < ops.size(); ++i)
    if (indeg[i] == 0)
      heap.push_back(i);
  std::make_heap(heap.begin(), heap.end(), cmp);

  llvm::SmallVector<unsigned> out;
  out.reserve(ops.size());
  while (!heap.empty()) {
    std::pop_heap(heap.begin(), heap.end(), cmp);
    unsigned n = heap.back();
    heap.pop_back();
    out.push_back(n);
    for (unsigned s : succ[n]) {
      if (--indeg[s] == 0) {
        heap.push_back(s);
        std::push_heap(heap.begin(), heap.end(), cmp);
      }
    }
  }

  if (out.size() != ops.size())
    return false;

  for (unsigned idx : out)
    ordered.push_back(ops[idx]);
  return true;
}

static LogicalResult emitBlockModule(
    Operation *owner, StringRef symbol, Block &top, FunctionType functionType,
    ArrayRef<std::string> inputPortNames,
    ArrayRef<std::string> outputPortNames, ValueRange returnValues,
    raw_ostream &os, const VerilogEmitterOptions &opts,
    const llvm::StringMap<std::string> &rtlModuleNames,
    StringRef parameterClause = {}, ArrayRef<std::string> inputRanges = {},
    ArrayRef<std::string> outputRanges = {}, StringRef bodyPrefix = {},
    StringRef bodySuffix = {}, bool emitHeader = true, bool emitFooter = true) {
  (void)opts;
  NameTable nt;
  std::vector<std::string> outNames;
  outNames.reserve(functionType.getNumResults());
  if (emitHeader) {
    os << "// Generated by pycc (pyCircuit)\n";
    os << "// Module: " << symbol << "\n\n";
    os << "module " << symbol;
    if (!parameterClause.empty())
      os << " #(\n" << parameterClause << "\n)";
    os << " (\n";
  }
  for (auto [i, arg] : llvm::enumerate(top.getArguments())) {
    std::string portName = nt.unique(inputPortNames[i]);
    if (emitHeader) {
      std::string range = inputRanges.empty() ? vPortRange(arg.getType())
                                               : inputRanges[i];
      os << "  input ";
      if (!range.empty())
        os << range << " ";
      os << portName;
      os << ((i + 1 == top.getNumArguments() &&
              functionType.getNumResults() == 0)
                 ? "\n"
                 : ",\n");
    }
    nt.names.try_emplace(arg, portName);
  }
  for (unsigned i = 0; i < functionType.getNumResults(); ++i) {
    std::string portName = nt.unique(outputPortNames[i]);
    outNames.push_back(portName);
    if (emitHeader) {
      std::string range = outputRanges.empty()
                              ? vPortRange(functionType.getResult(i))
                              : outputRanges[i];
      os << "  output ";
      if (!range.empty())
        os << range << " ";
      os << portName;
      os << ((i + 1 == functionType.getNumResults()) ? "\n" : ",\n");
    }
  }
  if (emitHeader)
    os << ");\n\n";
  os << bodyPrefix;

  // Declare internal nets for op results (including results inside pyc.comb regions).
  std::vector<NetDecl> decls;
  decls.reserve(256);
  owner->walk([&](Operation *op) {
    for (Value r : op->getResults()) {
      NetDecl d;
      d.name = nt.get(r);
      d.ty = r.getType();
      if (auto nAttr = op->getAttrOfType<StringAttr>("pyc.name"))
        d.comment = "pyc.name=\"" + nAttr.getValue().str() + "\"";
      else
        d.comment = "op=" + op->getName().getStringRef().str();
      decls.push_back(std::move(d));
    }
  });
  std::sort(decls.begin(), decls.end(), [](const NetDecl &a, const NetDecl &b) { return a.name < b.name; });
  for (const NetDecl &d : decls) {
    std::string range = vRange(d.ty);
    os << "wire ";
    if (!range.empty())
      os << range << " ";
    os << d.name << ";";
    if (!d.comment.empty())
      os << " // " << d.comment;
    os << "\n";
  }
  os << "\n";

  // Collect top-level ops for netlist-friendly emission.
  llvm::SmallVector<Operation *> combAssignOps;
  llvm::SmallVector<Operation *> instOps;
  llvm::SmallVector<Operation *> seqInstOps;

  for (Operation &op : top) {
      if (isa<func::ReturnOp, pyc::ReturnOp>(op))
        continue;
      if (isa<pyc::WireOp>(op))
        continue;

      if (isa<pyc::ConstantOp, pyc::AliasOp, pyc::ResetActiveOp, pyc::AddOp,
              pyc::SubOp, pyc::MulOp, pyc::UdivOp, pyc::UremOp, pyc::SdivOp,
              pyc::SremOp, pyc::SelectOp, pyc::AndOp, pyc::OrOp, pyc::XorOp,
              pyc::NotOp, pyc::AssertOp, pyc::AssignOp, pyc::CombOp,
              arith::SelectOp, pyc::CmpOp, pyc::TruncOp,
              pyc::ZextOp, pyc::SextOp, pyc::ExtractOp,
              pyc::ShlOp, pyc::LshrOp, pyc::AshrOp,
              pyc::ConcatOp, pyc::PriorityEncodeOp, pyc::PopcountOp,
              pyc::CountZerosOp, pyc::RtlCombOp>(op)) {
        combAssignOps.push_back(&op);
        continue;
      }
      if (isa<pyc::InstanceOp>(op)) {
        instOps.push_back(&op);
        continue;
      }
      if (isa<pyc::RegOp, pyc::FifoOp, pyc::ByteMemOp>(op)) {
        seqInstOps.push_back(&op);
        continue;
      }
      if (isa<pyc::SyncMemOp, pyc::SyncMemDPOp, pyc::AsyncFifoOp, pyc::CdcSyncOp>(op)) {
        seqInstOps.push_back(&op);
        continue;
      }
      return op.emitError("unsupported op for verilog emission: ") << op.getName();
  }

  auto cmp = [&](Operation *a, Operation *b) { return opSortKey(a, nt) < opSortKey(b, nt); };
  std::sort(instOps.begin(), instOps.end(), cmp);
  std::sort(seqInstOps.begin(), seqInstOps.end(), cmp);
  llvm::SmallVector<Operation *> orderedComb;
  if (!topoSortCombOps(combAssignOps, nt, orderedComb))
    std::sort(combAssignOps.begin(), combAssignOps.end(), cmp);
  else
    combAssignOps.assign(orderedComb.begin(), orderedComb.end());

  if (!combAssignOps.empty()) {
    std::optional<std::string> assertionClock;
    for (BlockArgument argument : top.getArguments())
      if (isa<pyc::ClockType>(argument.getType())) {
        if (assertionClock)
          return owner->emitError(
              "architecture SVA requires one unambiguous module clock");
        assertionClock = nt.get(argument);
      }
    llvm::StringSet<> assertionLabels;
    os << "// --- Combinational (netlist)\n";
    for (Operation *op : combAssignOps) {
      if (auto a = dyn_cast<pyc::AssertOp>(op)) {
        std::string msg = assertionDiagnostic(a);
        std::string esc;
        esc.reserve(msg.size());
        for (char c : msg) {
          if (c == '"' || c == '\\')
            esc.push_back('\\');
          esc.push_back(c);
        }
        os << "`ifndef SYNTHESIS\n";
        if (auto id = a.getObligationIdAttr()) {
          if (!assertionClock)
            return a.emitError(
                "architecture obligation SVA requires a module clock");
          std::string label = obligationAssertionLabel(id.getValue());
          if (!assertionLabels.insert(label).second)
            return a.emitError(
                "architecture obligation IDs collide after readable SVA "
                "label normalization");
          os << label << ": assert property (@(posedge " << *assertionClock
             << ") (" << nt.get(a.getCond()) << "))\n"
             << "  else $fatal(1, \"" << esc << "\");\n";
          os << label << "_coverage: cover property (@(posedge "
             << *assertionClock << ") (" << nt.get(a.getCond()) << "));\n";
        } else {
          os << "always @(*) begin\n";
          os << "  if (!(" << nt.get(a.getCond()) << ")) $fatal(1, \"" << esc
             << "\");\n";
          os << "end\n";
        }
        os << "`endif\n";
        continue;
      }
      if (auto a = dyn_cast<pyc::AssignOp>(op)) {
        emitConnectAssign(nt.get(a.getDst()), nt.get(a.getSrc()), a.getDst().getType(), os);
        continue;
      }
      if (auto comb = dyn_cast<pyc::CombOp>(op)) {
        if (failed(emitComb(comb, os, nt)))
          return failure();
        continue;
      }
      if (failed(emitNetlistOp(*op, os, nt)))
        return failure();
    }
    os << "\n";
  }

  if (!instOps.empty()) {
    os << "// --- Submodules\n";
    ModuleOp mod = owner->getParentOfType<ModuleOp>();
    if (!mod)
      return owner->emitError("verilog emitter: missing parent module for instance resolution");
    for (Operation *op : instOps) {
      auto inst = dyn_cast<pyc::InstanceOp>(op);
      if (!inst)
        continue;

      auto calleeAttr = op->getAttrOfType<FlatSymbolRefAttr>("callee");
      if (!calleeAttr)
        return inst.emitError("missing required FlatSymbolRefAttr `callee`");
      std::vector<std::string> inPorts;
      std::vector<std::string> outPorts;
      std::string calleeName;
      std::string calleeParameters;
      if (auto callee = mod.lookupSymbol<func::FuncOp>(calleeAttr.getValue())) {
        computeUniquePortNames(callee, inPorts, outPorts);
        calleeName = rtlModuleNames.lookup(callee.getSymName());
      } else if (auto family =
                     mod.lookupSymbol<pyc::FamilyOp>(calleeAttr.getValue())) {
        pyc::ModuleCaseOp selected;
        for (pyc::ModuleCaseOp candidate :
             family.getBody().front().getOps<pyc::ModuleCaseOp>())
          if (candidate.getSignature().getArguments() == inst.getStaticArgs()) {
            selected = candidate;
            break;
          }
        if (!selected)
          return inst.emitError("static arguments do not select a family case");
        if (failed(computeCasePortNames(selected, inPorts, outPorts)))
          return failure();
        calleeName = rtlModuleNames.lookup(family.getSymName());
        if (!family.getSchema().getParameters().getParameters().empty()) {
          auto concreteArguments = pyc::staticArgumentsFromDependent(
              inst.getStaticArgs());
          if (failed(concreteArguments))
            return inst.emitError(
                "Verilog instance arguments must be concrete typed values");
          auto parameters = verilogInstanceParameters(*concreteArguments);
          if (!parameters)
            return inst.emitError(
                "Verilog family emission does not support this static value type");
          calleeParameters = *parameters;
        }
      } else {
        return inst.emitError("callee symbol not found: ") << calleeAttr.getValue();
      }
      if (inPorts.size() != inst.getNumOperands())
        return inst.emitError("operand count does not match callee signature");
      if (outPorts.size() != inst.getNumResults())
        return inst.emitError("result count does not match callee signature");

      std::string instName = "inst";
      if (auto nameAttr = op->getAttrOfType<StringAttr>("name"))
        instName = sanitizeId(nameAttr.getValue());
      if (auto shortAttr = op->getAttrOfType<StringAttr>("short_name"))
        instName = sanitizeId(shortAttr.getValue());
      instName = nt.unique(instName);

      std::vector<std::string> inConn;
      std::vector<std::string> outConn;
      inConn.reserve(inPorts.size());
      outConn.reserve(outPorts.size());

      for (unsigned i = 0; i < inPorts.size(); ++i)
        inConn.push_back(nt.get(inst.getOperand(i)));
      for (unsigned i = 0; i < outPorts.size(); ++i)
        outConn.push_back(nt.get(inst.getResult(i)));

      os << calleeName;
      if (!calleeParameters.empty())
        os << " #(" << calleeParameters << ")";
      os << " " << instName << " (\n";
      unsigned totalPorts = static_cast<unsigned>(inPorts.size() + outPorts.size());
      unsigned emitted = 0;

      for (unsigned i = 0; i < inPorts.size(); ++i) {
        os << "  ." << inPorts[i] << "(" << inConn[i] << ")";
        emitted++;
        os << ((emitted == totalPorts) ? "\n" : ",\n");
      }
      for (unsigned i = 0; i < outPorts.size(); ++i) {
        os << "  ." << outPorts[i] << "(" << outConn[i] << ")";
        emitted++;
        os << ((emitted == totalPorts) ? "\n" : ",\n");
      }
      os << ");\n";
    }
    os << "\n";
  }

  if (!seqInstOps.empty()) {
    os << "// --- Sequential primitives\n";
    for (Operation *op : seqInstOps) {
      if (auto r = dyn_cast<pyc::RegOp>(op)) {
        auto qTy = r.getQ().getType();
        auto width = leafWidth(qTy);
        if (!width)
          return r.emitError("verilog emitter only supports integer reg data type");

        os << "pyc_reg #(.WIDTH(" << *width << ")) " << nt.get(r.getQ()) << "_inst (\n";
        os << "  .clk(" << nt.get(r.getClk()) << "),\n";
        os << "  .rst(" << nt.get(r.getRst()) << "),\n";
        os << "  .en(" << nt.get(r.getEn()) << "),\n";
        os << "  .d(" << nt.get(r.getNext()) << "),\n";
        os << "  .init(" << nt.get(r.getInit()) << "),\n";
        os << "  .q(" << nt.get(r.getQ()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto fifo = dyn_cast<pyc::FifoOp>(op)) {
        auto inDataTy = dyn_cast<IntegerType>(fifo.getInData().getType());
        if (!inDataTy)
          return fifo.emitError("verilog emitter only supports integer fifo data type");
        auto depth = fifo->getAttrOfType<IntegerAttr>("depth").getValue().getZExtValue();
        os << "pyc_fifo #(.WIDTH(" << inDataTy.getWidth() << "), .DEPTH(" << depth << ")) "
           << nt.get(fifo.getInReady()) << "_inst (\n";
        os << "  .clk(" << nt.get(fifo.getClk()) << "),\n";
        os << "  .rst(" << nt.get(fifo.getRst()) << "),\n";
        os << "  .in_valid(" << nt.get(fifo.getInValid()) << "),\n";
        os << "  .in_ready(" << nt.get(fifo.getInReady()) << "),\n";
        os << "  .in_data(" << nt.get(fifo.getInData()) << "),\n";
        os << "  .out_valid(" << nt.get(fifo.getOutValid()) << "),\n";
        os << "  .out_ready(" << nt.get(fifo.getOutReady()) << "),\n";
        os << "  .out_data(" << nt.get(fifo.getOutData()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto mem = dyn_cast<pyc::ByteMemOp>(op)) {
        auto addrTy = dyn_cast<IntegerType>(mem.getRaddr().getType());
        auto dataTy = dyn_cast<IntegerType>(mem.getRdata().getType());
        if (!addrTy || !dataTy)
          return mem.emitError("verilog emitter only supports integer byte_mem types");

        auto depthAttr = mem->getAttrOfType<IntegerAttr>("depth");
        if (!depthAttr)
          return mem.emitError("missing integer attribute `depth`");
        auto depth = depthAttr.getValue().getZExtValue();

        std::string inst = nt.get(mem.getRdata()) + "_inst";
        if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
          inst = sanitizeId(nameAttr.getValue());

        os << "pyc_byte_mem #(.ADDR_WIDTH(" << addrTy.getWidth() << "), .DATA_WIDTH(" << dataTy.getWidth() << "), .DEPTH("
           << depth << ")) " << inst << " (\n";
        os << "  .clk(" << nt.get(mem.getClk()) << "),\n";
        os << "  .rst(" << nt.get(mem.getRst()) << "),\n";
        os << "  .raddr(" << nt.get(mem.getRaddr()) << "),\n";
        os << "  .rdata(" << nt.get(mem.getRdata()) << "),\n";
        os << "  .wvalid(" << nt.get(mem.getWvalid()) << "),\n";
        os << "  .waddr(" << nt.get(mem.getWaddr()) << "),\n";
        os << "  .wdata(" << nt.get(mem.getWdata()) << "),\n";
        os << "  .wstrb(" << nt.get(mem.getWstrb()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto mem = dyn_cast<pyc::SyncMemOp>(op)) {
        auto addrTy = dyn_cast<IntegerType>(mem.getRaddr().getType());
        auto dataTy = dyn_cast<IntegerType>(mem.getRdata().getType());
        if (!addrTy || !dataTy)
          return mem.emitError("verilog emitter only supports integer sync_mem types");

        auto depthAttr = mem->getAttrOfType<IntegerAttr>("depth");
        if (!depthAttr)
          return mem.emitError("missing integer attribute `depth`");
        auto depth = depthAttr.getValue().getZExtValue();

        std::string inst = nt.get(mem.getRdata()) + "_inst";
        if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
          inst = sanitizeId(nameAttr.getValue());

        os << "pyc_sync_mem #(.ADDR_WIDTH(" << addrTy.getWidth() << "), .DATA_WIDTH(" << dataTy.getWidth()
           << "), .DEPTH(" << depth << ")) " << inst << " (\n";
        os << "  .clk(" << nt.get(mem.getClk()) << "),\n";
        os << "  .rst(" << nt.get(mem.getRst()) << "),\n";
        os << "  .ren(" << nt.get(mem.getRen()) << "),\n";
        os << "  .raddr(" << nt.get(mem.getRaddr()) << "),\n";
        os << "  .rdata(" << nt.get(mem.getRdata()) << "),\n";
        os << "  .wvalid(" << nt.get(mem.getWvalid()) << "),\n";
        os << "  .waddr(" << nt.get(mem.getWaddr()) << "),\n";
        os << "  .wdata(" << nt.get(mem.getWdata()) << "),\n";
        os << "  .wstrb(" << nt.get(mem.getWstrb()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto mem = dyn_cast<pyc::SyncMemDPOp>(op)) {
        auto addrTy = dyn_cast<IntegerType>(mem.getRaddr0().getType());
        auto dataTy = dyn_cast<IntegerType>(mem.getRdata0().getType());
        if (!addrTy || !dataTy)
          return mem.emitError("verilog emitter only supports integer sync_mem_dp types");

        auto depthAttr = mem->getAttrOfType<IntegerAttr>("depth");
        if (!depthAttr)
          return mem.emitError("missing integer attribute `depth`");
        auto depth = depthAttr.getValue().getZExtValue();

        std::string inst = nt.get(mem.getRdata0()) + "_inst";
        if (auto nameAttr = mem->getAttrOfType<StringAttr>("name"))
          inst = sanitizeId(nameAttr.getValue());

        os << "pyc_sync_mem_dp #(.ADDR_WIDTH(" << addrTy.getWidth() << "), .DATA_WIDTH(" << dataTy.getWidth()
           << "), .DEPTH(" << depth << ")) " << inst << " (\n";
        os << "  .clk(" << nt.get(mem.getClk()) << "),\n";
        os << "  .rst(" << nt.get(mem.getRst()) << "),\n";
        os << "  .ren0(" << nt.get(mem.getRen0()) << "),\n";
        os << "  .raddr0(" << nt.get(mem.getRaddr0()) << "),\n";
        os << "  .rdata0(" << nt.get(mem.getRdata0()) << "),\n";
        os << "  .ren1(" << nt.get(mem.getRen1()) << "),\n";
        os << "  .raddr1(" << nt.get(mem.getRaddr1()) << "),\n";
        os << "  .rdata1(" << nt.get(mem.getRdata1()) << "),\n";
        os << "  .wvalid(" << nt.get(mem.getWvalid()) << "),\n";
        os << "  .waddr(" << nt.get(mem.getWaddr()) << "),\n";
        os << "  .wdata(" << nt.get(mem.getWdata()) << "),\n";
        os << "  .wstrb(" << nt.get(mem.getWstrb()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto fifo = dyn_cast<pyc::AsyncFifoOp>(op)) {
        auto inDataTy = dyn_cast<IntegerType>(fifo.getInData().getType());
        if (!inDataTy)
          return fifo.emitError("verilog emitter only supports integer async_fifo data type");
        auto depth = fifo->getAttrOfType<IntegerAttr>("depth").getValue().getZExtValue();
        os << "pyc_async_fifo #(.WIDTH(" << inDataTy.getWidth() << "), .DEPTH(" << depth << ")) "
           << nt.get(fifo.getInReady()) << "_inst (\n";
        os << "  .in_clk(" << nt.get(fifo.getInClk()) << "),\n";
        os << "  .in_rst(" << nt.get(fifo.getInRst()) << "),\n";
        os << "  .in_valid(" << nt.get(fifo.getInValid()) << "),\n";
        os << "  .in_ready(" << nt.get(fifo.getInReady()) << "),\n";
        os << "  .in_data(" << nt.get(fifo.getInData()) << "),\n";
        os << "  .out_clk(" << nt.get(fifo.getOutClk()) << "),\n";
        os << "  .out_rst(" << nt.get(fifo.getOutRst()) << "),\n";
        os << "  .out_valid(" << nt.get(fifo.getOutValid()) << "),\n";
        os << "  .out_ready(" << nt.get(fifo.getOutReady()) << "),\n";
        os << "  .out_data(" << nt.get(fifo.getOutData()) << ")\n";
        os << ");\n";
        continue;
      }
      if (auto s = dyn_cast<pyc::CdcSyncOp>(op)) {
        auto ty = dyn_cast<IntegerType>(s.getIn().getType());
        if (!ty)
          return s.emitError("verilog emitter only supports integer cdc_sync types");
        std::uint64_t stages = 2;
        if (auto st = s->getAttrOfType<IntegerAttr>("stages"))
          stages = st.getValue().getZExtValue();
        os << "pyc_cdc_sync #(.WIDTH(" << ty.getWidth() << "), .STAGES(" << stages << ")) " << nt.get(s.getOut())
           << "_inst (\n";
        os << "  .clk(" << nt.get(s.getClk()) << "),\n";
        os << "  .rst(" << nt.get(s.getRst()) << "),\n";
        os << "  .in(" << nt.get(s.getIn()) << "),\n";
        os << "  .out(" << nt.get(s.getOut()) << ")\n";
        os << ");\n";
        continue;
      }
      return op->emitError("internal error: missing verilog sequential primitive emission handler");
    }
    os << "\n";
  }

  // Connect outputs from return.
  for (auto [i, v] : llvm::enumerate(returnValues)) {
    if (nt.get(v) == outNames[i])
      continue;
    emitConnectAssign(outNames[i], nt.get(v), functionType.getResult(i), os);
  }

  os << bodySuffix;
  if (emitFooter)
    os << "\nendmodule\n\n";
  return success();
}

static LogicalResult emitFunc(
    func::FuncOp f, llvm::StringRef rtlName, raw_ostream &os,
    const VerilogEmitterOptions &opts,
    const llvm::StringMap<std::string> &rtlModuleNames) {
  if (!llvm::hasSingleElement(f.getBody()))
    return f.emitError("verilog emitter currently supports single-block functions only");
  std::vector<std::string> inputNames;
  std::vector<std::string> outputNames;
  computeUniquePortNames(f, inputNames, outputNames);
  auto ret = dyn_cast_or_null<func::ReturnOp>(f.getBody().front().getTerminator());
  if (!ret)
    return f.emitError("missing return");
  return emitBlockModule(f, rtlName, f.getBody().front(),
                         f.getFunctionType(), inputNames, outputNames,
                         ret.getOperands(), os, opts, rtlModuleNames);
}

static LogicalResult emitFamily(
    pyc::FamilyOp family, llvm::StringRef rtlName, raw_ostream &os,
    const VerilogEmitterOptions &opts,
    const llvm::StringMap<std::string> &rtlModuleNames) {
  auto cases = family.getBody().front().getOps<pyc::ModuleCaseOp>();
  auto parameters = family.getSchema().getParameters().getParameters();
  if (parameters.empty()) {
    if (!llvm::hasSingleElement(cases))
      return family.emitError("an unparameterized family must have one case");
    pyc::ModuleCaseOp moduleCase = *cases.begin();
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    if (failed(computeCasePortNames(moduleCase, inputNames, outputNames)))
      return failure();
    auto functionType = cast<FunctionType>(
        moduleCase.getSignature().getPhysical().getValue());
    auto ret = dyn_cast_or_null<pyc::ReturnOp>(
        moduleCase.getBody().front().getTerminator());
    if (!ret)
      return moduleCase.emitError("missing pyc.return");
    return emitBlockModule(moduleCase, rtlName,
                           moduleCase.getBody().front(), functionType,
                           inputNames, outputNames, ret.getValues(), os, opts,
                           rtlModuleNames);
  }

  llvm::SmallVector<pyc::ModuleCaseOp> orderedCases(cases.begin(), cases.end());
  std::vector<std::string> inputNames;
  std::vector<std::string> outputNames;
  if (failed(computeCasePortNames(orderedCases.front(), inputNames,
                                  outputNames)))
    return failure();
  auto firstType = cast<FunctionType>(
      orderedCases.front().getSignature().getPhysical().getValue());
  std::vector<std::string> conditions;
  conditions.reserve(orderedCases.size());
  for (pyc::ModuleCaseOp moduleCase : orderedCases) {
    std::vector<std::string> caseInputs;
    std::vector<std::string> caseOutputs;
    if (failed(computeCasePortNames(moduleCase, caseInputs, caseOutputs)))
      return failure();
    auto functionType = cast<FunctionType>(
        moduleCase.getSignature().getPhysical().getValue());
    if (caseInputs != inputNames || caseOutputs != outputNames ||
        functionType.getNumInputs() != firstType.getNumInputs() ||
        functionType.getNumResults() != firstType.getNumResults())
      return family.emitError(
          "admitted cases do not share one representable RTL carrier shape");
    auto arguments = pyc::staticArgumentsFromDependent(
        moduleCase.getSignature().getArguments());
    if (failed(arguments))
      return moduleCase.emitError(
          "Verilog family case arguments must be concrete typed values");
    auto condition = verilogCaseCondition(*arguments);
    if (!condition)
      return moduleCase.emitError(
          "Verilog family emission does not support this static value type");
    conditions.push_back(*condition);
  }

  auto physicalWidth = [&](Type type) -> std::optional<unsigned> {
    if (isa<pyc::ClockType, pyc::ResetType>(type))
      return 1;
    return leafWidth(type);
  };
  auto makeRanges = [&](bool inputs) -> std::optional<std::vector<std::string>> {
    unsigned count = inputs ? firstType.getNumInputs() : firstType.getNumResults();
    std::vector<std::string> ranges(count);
    for (unsigned index = 0; index < count; ++index) {
      llvm::SmallVector<unsigned> widths;
      for (pyc::ModuleCaseOp moduleCase : orderedCases) {
        auto type = cast<FunctionType>(
            moduleCase.getSignature().getPhysical().getValue());
        auto width = physicalWidth(inputs ? type.getInput(index)
                                          : type.getResult(index));
        if (!width)
          return std::nullopt;
        widths.push_back(*width);
      }
      if (llvm::all_of(widths, [](unsigned width) { return width == 1; }))
        continue;
      std::string expression = std::to_string(widths.back());
      for (int caseIndex = static_cast<int>(widths.size()) - 2; caseIndex >= 0;
           --caseIndex)
        expression = "(" + conditions[caseIndex] + " ? " +
                     std::to_string(widths[caseIndex]) + " : " + expression +
                     ")";
      ranges[index] = "[(" + expression + ")-1:0]";
    }
    return ranges;
  };
  auto inputRanges = makeRanges(true);
  auto outputRanges = makeRanges(false);
  if (!inputRanges || !outputRanges)
    return family.emitError(
        "admitted family case has a non-integer physical RTL carrier");

  auto firstStaticArguments = pyc::staticArgumentsFromDependent(
      orderedCases.front().getSignature().getArguments());
  if (failed(firstStaticArguments))
    return family.emitError(
        "default family case arguments must be concrete typed values");
  auto firstArguments = firstStaticArguments->getArguments()
                            .getAsRange<acir::ac::StaticArgumentAttr>();
  auto firstArgument = firstArguments.begin();
  std::string parameterClause;
  llvm::raw_string_ostream parameterStream(parameterClause);
  bool first = true;
  for (acir::ac::StaticParameterAttr parameter :
       parameters.getAsRange<acir::ac::StaticParameterAttr>()) {
    if (firstArgument == firstArguments.end())
      return family.emitError("default case arguments are incomplete");
    auto defaultValue = verilogStaticValue((*firstArgument++).getValue());
    if (!defaultValue)
      return family.emitError(
          "Verilog family emission does not support this static parameter type");
    if (!first)
      parameterStream << ",\n";
    first = false;
    Attribute type = parameter.getType().getValue();
    auto typeSpelling = verilogStaticType(type);
    if (!typeSpelling)
      return family.emitError(
          "Verilog family emission does not support this static parameter type");
    parameterStream << "  parameter " << *typeSpelling << " "
                    << sanitizeId(parameter.getName().getValue()) << " = "
                    << *defaultValue;
  }
  parameterStream.flush();

  for (auto [caseIndex, moduleCase] : llvm::enumerate(orderedCases)) {
    auto functionType = cast<FunctionType>(
        moduleCase.getSignature().getPhysical().getValue());
    auto ret = dyn_cast_or_null<pyc::ReturnOp>(
        moduleCase.getBody().front().getTerminator());
    if (!ret)
      return moduleCase.emitError("missing pyc.return");
    std::string prefix = caseIndex == 0 ? "generate\n  if (" : "  else if (";
    prefix += conditions[caseIndex] + ") begin : admitted_case\n";
    std::string suffix = "  end\n";
    const bool last = caseIndex + 1 == orderedCases.size();
    if (last)
      suffix += "  else begin : rejected_parameters\n"
                "    initial $fatal(1, \"unadmitted static family arguments\");\n"
                "  end\nendgenerate\n";
    if (failed(emitBlockModule(
            moduleCase, rtlName, moduleCase.getBody().front(),
            functionType, inputNames, outputNames, ret.getValues(), os, opts,
            rtlModuleNames,
            parameterClause, *inputRanges, *outputRanges, prefix, suffix,
            /*emitHeader=*/caseIndex == 0, /*emitFooter=*/last)))
      return failure();
  }
  return success();
}

} // namespace

LogicalResult emitVerilog(ModuleOp module, llvm::raw_ostream &os, const VerilogEmitterOptions &opts) {
  llvm::StringMap<std::string> rtlModuleNames;
  if (failed(buildRtlModuleNames(module, rtlModuleNames)))
    return failure();
  if (opts.targetFpga) {
    os << "`define PYC_TARGET_FPGA 1\n\n";
  }
  if (opts.includePrimitives) {
    os << "`include \"pyc_reg.v\"\n";
    os << "`include \"pyc_fifo.v\"\n\n";
    os << "`include \"pyc_byte_mem.v\"\n\n";
    os << "`include \"pyc_sync_mem.v\"\n";
    os << "`include \"pyc_sync_mem_dp.v\"\n";
    os << "`include \"pyc_async_fifo.v\"\n";
    os << "`include \"pyc_cdc_sync.v\"\n\n";
    llvm::StringSet<> included;
    module.walk([&](pyc::RtlCombOp selected) {
      auto sources = selected->getAttrOfType<ArrayAttr>("sources");
      if (!sources)
        return;
      for (Attribute raw : sources) {
        auto source = dyn_cast<DictionaryAttr>(raw);
        auto path = source ? source.getAs<StringAttr>("path") : StringAttr();
        if (path && included.insert(path.getValue()).second)
          os << "`include \"" << path.getValue() << "\"\n";
      }
    });
    if (!included.empty())
      os << "\n";
  }

  if (failed(emitVerilogNominalDeclarations(module, os)))
    return failure();

  llvm::SmallVector<func::FuncOp> functions(module.getOps<func::FuncOp>());
  llvm::sort(functions, [&](func::FuncOp left, func::FuncOp right) {
    return rtlModuleNames.lookup(left.getSymName()) <
           rtlModuleNames.lookup(right.getSymName());
  });
  for (func::FuncOp f : functions) {
    if (failed(emitFunc(f, rtlModuleNames.lookup(f.getSymName()), os, opts,
                        rtlModuleNames)))
      return failure();
  }
  llvm::SmallVector<pyc::FamilyOp> families(module.getOps<pyc::FamilyOp>());
  llvm::sort(families, [&](pyc::FamilyOp left, pyc::FamilyOp right) {
    return rtlModuleNames.lookup(left.getSymName()) <
           rtlModuleNames.lookup(right.getSymName());
  });
  for (pyc::FamilyOp family : families)
    if (failed(emitFamily(family, rtlModuleNames.lookup(family.getSymName()),
                          os, opts, rtlModuleNames)))
      return failure();
  return success();
}

LogicalResult emitVerilogFunc(ModuleOp module, func::FuncOp f, llvm::raw_ostream &os, const VerilogEmitterOptions &opts) {
  llvm::StringMap<std::string> rtlModuleNames;
  if (failed(buildRtlModuleNames(module, rtlModuleNames)))
    return failure();
  return emitFunc(f, rtlModuleNames.lookup(f.getSymName()), os, opts,
                  rtlModuleNames);
}

} // namespace pyc

#include "CppEmitter.h"
#include "CppLowering.h"
#include "CppRuntime.h"
#include "Dialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/JSON.h"
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
using namespace mlir;
namespace {
std::string attr(Operation *op, StringRef key) {
  auto a = op->getAttrOfType<StringAttr>(key);
  return a ? a.getValue().str() : "";
}
int64_t num(Operation *op, StringRef key) {
  return op->getAttrOfType<IntegerAttr>(key).getInt();
}
std::string quote(StringRef s) {
  std::string out;
  llvm::raw_string_ostream stream(out);
  stream << llvm::json::Value(s);
  return out;
}
struct Emitter {
  mlir::ModuleOp module;
  acir::CircuitOp circuit;
  DenseMap<Type, std::string> types;
  acir::CppPlan plan;
  std::map<std::string, std::string> paths, scopeTypes;
  std::map<std::string, std::ostringstream> scopeFields, scopeMethods;
  std::map<std::string, std::map<std::string, std::string>> children;
  std::map<std::string, std::set<std::string>> occupied;
  std::map<std::string, std::string> nativeArrays;
  std::vector<std::string> blockNames;
  std::string context;
  SmallVector<std::string> operands;
  std::set<std::string> typeNames{"Model", "json", "Cell",
                                  "Queue", "EHR",  "Signal"};
  std::map<std::string, std::string> states;
  std::ostringstream definitions, fields, methods, reset, begin, commit, trace;
  explicit Emitter(mlir::ModuleOp m)
      : module(m), circuit(*m.getOps<acir::CircuitOp>().begin()), plan(m) {}
  Type child(Type t, unsigned i) {
    if (auto a = dyn_cast<acir::ArrayType>(t))
      return a.getShape().size() == 1
                 ? a.getElementType()
                 : acir::ArrayType::get(t.getContext(), a.getElementType(),
                                        a.getShape().drop_front());
    if (auto a = dyn_cast<acir::MaskType>(t))
      return a.getShape().size() == 1
                 ? Type(IntegerType::get(t.getContext(), 1))
                 : acir::MaskType::get(t.getContext(),
                                       a.getShape().drop_front());
    if (auto a = dyn_cast<TupleType>(t))
      return a.getType(i);
    if (auto a = dyn_cast<acir::SelectedType>(t)) {
      auto array = cast<acir::ArrayType>(a.getElementType());
      return i == 0 ? Type(array)
                    : acir::MaskType::get(t.getContext(), array.getShape());
    }
    auto s = cast<acir::StructType>(t);
    auto fs = module->getAttrOfType<DictionaryAttr>("acir.model")
                  .getAs<DictionaryAttr>("types")
                  .getAs<ArrayAttr>(s.getName());
    return cast<DictionaryAttr>(fs[i])
        .getAs<TypeAttr>("payload_type")
        .getValue();
  }
  unsigned fieldIndex(Type t, StringRef field) {
    auto s = cast<acir::StructType>(t);
    auto fs = module->getAttrOfType<DictionaryAttr>("acir.model")
                  .getAs<DictionaryAttr>("types")
                  .getAs<ArrayAttr>(s.getName());
    for (auto [i, f] : llvm::enumerate(fs))
      if (cast<DictionaryAttr>(f).getAs<StringAttr>("name").getValue() == field)
        return i;
    throw std::runtime_error("C++ emitter: unknown struct field");
  }
  std::string type(Type t) {
    if (auto it = types.find(t); it != types.end())
      return it->second;
    std::string result;
    if (auto i = dyn_cast<IntegerType>(t)) {
      unsigned w = i.getWidth();
      if (w != 1 && w != 8 && w != 16 && w != 32 && w != 64)
        throw std::runtime_error("C++ emitter: unsupported integer width");
      result = w == 1 ? "bool" : "uint" + std::to_string(w) + "_t";
    } else if (auto a = dyn_cast<acir::ArrayType>(t))
      result = "std::array<" + type(child(t, 0)) + "," +
               std::to_string(a.getShape().front()) + ">";
    else if (auto a = dyn_cast<acir::MaskType>(t))
      result = "std::array<" + type(child(t, 0)) + "," +
               std::to_string(a.getShape().front()) + ">";
    else if (auto a = dyn_cast<TupleType>(t)) {
      result = "std::tuple<";
      for (auto [i, c] : llvm::enumerate(a.getTypes()))
        result += (i ? "," : "") + type(c);
      result += ">";
    } else if (isa<acir::SelectedType>(t))
      result =
          "std::tuple<" + type(child(t, 0)) + "," + type(child(t, 1)) + ">";
    else if (auto s = dyn_cast<acir::StructType>(t)) {
      auto fs = module->getAttrOfType<DictionaryAttr>("acir.model")
                    .getAs<DictionaryAttr>("types")
                    .getAs<ArrayAttr>(s.getName());
      SmallVector<std::string> members;
      for (unsigned i = 0; i < fs.size(); ++i)
        members.push_back(type(child(t, i)));
      result = uniqueType(s.getName().contains('.')
                              ? s.getName().rsplit('.').second.str()
                              : s.getName().str());
      definitions << "// ACPy payload " << s.getName().str() << "\nstruct "
                  << result << " {\n";
      for (auto [i, c] : llvm::enumerate(members))
        definitions << c << " " << fieldName(t, i) << "{};\n";
      definitions
          << "bool operator==(const " << result
          << "&) const = default;\n};\ninline void json(std::ostream &o,const "
          << result << " &v) { o<<'[';\n";
      for (unsigned i = 0; i < fs.size(); ++i)
        definitions << (i ? "o<<','; " : "") << "json(o,v." << fieldName(t, i)
                    << ");\n";
      definitions << "o<<']'; }\n";
    } else
      throw std::runtime_error("C++ emitter: unsupported payload type");
    types[t] = result;
    return result;
  }
  std::string access(std::string x, Type t, unsigned i) {
    if (isa<acir::StructType>(t)) {
      bool simple = llvm::all_of(x, [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '.';
      });
      return (simple ? x : "(" + x + ")") + "." + fieldName(t, i);
    }
    if (isa<TupleType, acir::SelectedType>(t))
      return "std::get<" + std::to_string(i) + ">(" + x + ")";
    return "(" + x + ").at(" + std::to_string(i) + ")";
  }
  std::string literal(Attribute a, Type t) {
    if (auto n = dyn_cast<IntegerAttr>(a)) {
      auto width = cast<IntegerType>(t).getWidth();
      auto value = std::to_string(n.getValue().getZExtValue());
      if (width == 1)
        return n.getValue().isZero() ? "false" : "true";
      if (width == 32)
        return value + "u";
      if (width == 64)
        return "uint64_t(" + value + "ULL)";
      return type(t) + "(" + value + ")";
    }
    auto array = cast<ArrayAttr>(a);
    std::string s = type(t) + "{";
    for (auto [i, v] : llvm::enumerate(array))
      s += (i ? "," : "") + literal(v, child(t, i));
    return s + "}";
  }
  std::string uniqueType(std::string base) {
    base = acir::cppIdentifier(base);
    auto result = base;
    for (unsigned i = 2; !typeNames.insert(result).second; ++i)
      result = base + "_" + std::to_string(i);
    return result;
  }
  std::string fieldName(Type t, unsigned index) {
    auto structure = cast<acir::StructType>(t);
    auto fs = module->getAttrOfType<DictionaryAttr>("acir.model")
                  .getAs<DictionaryAttr>("types")
                  .getAs<ArrayAttr>(structure.getName());
    std::set<std::string> used;
    std::string result;
    for (unsigned i = 0; i <= index; ++i) {
      auto base = acir::cppIdentifier(
          cast<DictionaryAttr>(fs[i]).getAs<StringAttr>("name").getValue());
      result = base;
      for (unsigned n = 2; !used.insert(result).second; ++n)
        result = base + "_" + std::to_string(n);
    }
    return result;
  }
  std::string expr(Value v) {
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    auto t = v.getType();
    auto a = [&](unsigned i) { return operands[i]; };
    auto src = [&](unsigned i) { return op->getOperand(i).getType(); };
    if (auto read = dyn_cast<acir::ReadOp>(op))
      return readExpression(read, v);
    if (n == "arith.constant")
      return literal(op->getAttr("value"), t);
    if (n == "acir.export" || n == "acir.bind" || n == "acir.fire")
      return a(0);
    if (n == "arith.select") {
      if (a(0) == "true" || a(1) == a(2))
        return a(1);
      if (a(0) == "false")
        return a(2);
      return "(" + a(0) + " ? " + a(1) + " : " + a(2) + ")";
    }
    if (n == "acir.aggregate") {
      std::string s = type(t) + "{";
      for (unsigned i = 0; i < op->getNumOperands(); ++i)
        s += (i ? "," : "") + a(i);
      return s + "}";
    }
    if (n == "acir.length")
      return type(t) + "(" + a(0) + ".size())";
    if (n == "acir.extract") {
      if (op->hasAttr("field"))
        return access(a(0), src(0), fieldIndex(src(0), attr(op, "field")));
      if (isa<TupleType, acir::SelectedType>(src(0))) {
        auto c = op->getOperand(1).getDefiningOp<arith::ConstantOp>();
        if (!c)
          throw std::runtime_error(
              "C++ emitter: dynamic heterogeneous tuple index");
        return access(a(0), src(0), cast<IntegerAttr>(c.getValue()).getInt());
      }
      return a(0) + ".at(" + a(1) + ")";
    }
    if (n == "acir.array.domain") {
      if (isa<acir::SelectedType>(src(0)))
        return access(a(0), src(0), 1);
      return "[&]{ " + type(t) + " r{}; acir_sim::full_mask(r); return r; }()";
    }
    if (n == "acir.array.select") {
      if (isa<acir::SelectedType>(src(0)))
        return type(t) + "{" + access(a(0), src(0), 0) +
               ",acir_sim::intersect(" + access(a(0), src(0), 1) + "," + a(1) +
               ")}";
      return type(t) + "{" + a(0) + "," + a(1) + "}";
    }
    if (n == "acir.array.index") {
      if (!isa<acir::SelectedType>(src(0)))
        return a(0) + ".at(" + a(1) + ")";
      auto data = access(a(0), src(0), 0) + ".at(" + a(1) + ")";
      if (isa<acir::SelectedType>(t))
        return type(t) + "{" + data + "," + access(a(0), src(0), 1) + ".at(" +
               a(1) + ")}";
      return data;
    }
    if (n == "acir.update") {
      std::string target = "r";
      Type current = src(0);
      unsigned dynamic = 2;
      for (auto step : op->getAttrOfType<ArrayAttr>("path")) {
        if (auto f = dyn_cast<StringAttr>(step)) {
          auto i = fieldIndex(current, f.getValue());
          target = access(target, current, i);
          current = child(current, i);
        } else {
          target += ".at(" + a(dynamic++) + ")";
          current = child(current, 0);
        }
      }
      return "[&]{ auto r=" + a(0) + "; " + target + "=" + a(1) +
             "; return r; }()";
    }
    if (n == "acir.compare") {
      auto comparison = attr(op, "operator_name");
      if (comparison != "==" && comparison != "!=")
        throw std::runtime_error(
            "C++ emitter: unsupported aggregate comparison");
      return "(" + a(0) + comparison + a(1) + ")";
    }
    if (n == "acir.assert")
      return "acir_sim::check(" + a(0) + ")";
    if (n == "acir.unary") {
      auto oper = attr(op, "operator_name");
      if (oper == "not")
        return "(!" + a(0) + ")";
      if (oper == "-")
        return "acir_sim::neg(" + a(0) + ")";
      if (oper == "~")
        return "acir_sim::narrow<" + type(t) + ">(~acir_sim::bits(" + a(0) +
               "))";
      if (oper == "+")
        return a(0);
    }
    if (n == "arith.extui" || n == "arith.trunci")
      return t.isInteger(1) ? "acir_sim::narrow<bool>(" + a(0) + ")"
                            : type(t) + "(" + a(0) + ")";
    if (n == "arith.extsi")
      return "acir_sim::sext<" + type(t) + ">(" + a(0) + ")";
    if (n == "arith.cmpi") {
      int p = num(op, "predicate");
      if (p == 0 || p == 1)
        return "(" + a(0) + (p == 0 ? "==" : "!=") + a(1) + ")";
      if (p >= 6) {
        const char *ops[] = {"<", "<=", ">", ">="};
        return "(" + a(0) + ops[p - 6] + a(1) + ")";
      }
      auto l = "acir_sim::less_signed(" + a(p == 3 || p == 4 ? 1 : 0) + "," +
               a(p == 3 || p == 4 ? 0 : 1) + ")";
      return p == 3 || p == 5 ? "(!" + l + ")" : l;
    }
    if ((n == "arith.andi" || n == "arith.ori") && t.isInteger(1)) {
      if (a(0) == a(1))
        return a(0);
      if (n == "arith.andi" && (a(0) == "false" || a(1) == "false"))
        return "false";
      if (n == "arith.ori" && (a(0) == "true" || a(1) == "true"))
        return "true";
      if (a(0) == "true" && n == "arith.andi")
        return a(1);
      if (a(1) == "true" && n == "arith.andi")
        return a(0);
      if (a(0) == "false" && n == "arith.ori")
        return a(1);
      if (a(1) == "false" && n == "arith.ori")
        return a(0);
      return "(" + a(0) + (n == "arith.andi" ? " && " : " || ") + a(1) + ")";
    }
    if (n == "arith.xori" && t.isInteger(1)) {
      if (a(1) == "true")
        return "(!" + a(0) + ")";
      return "(" + a(0) + " != " + a(1) + ")";
    }
    std::map<std::string, std::string> binary = {
        {"arith.addi", "+"}, {"arith.subi", "-"}, {"arith.muli", "*"},
        {"arith.andi", "&"}, {"arith.ori", "|"},  {"arith.xori", "^"}};
    if (binary.count(n.str())) {
      auto width = cast<IntegerType>(t).getWidth();
      if (width >= 32)
        return "(" + a(0) + " " + binary[n.str()] + " " + a(1) + ")";
      // Explicit unsigned promotion avoids signed int overflow for u16*u16.
      return "acir_sim::narrow<" + type(t) + ">(uint32_t(" + a(0) + ") " +
             binary[n.str()] + " uint32_t(" + a(1) + "))";
    }
    if (n == "arith.shli" || n == "arith.shrui")
      if (auto count = op->getOperand(1).getDefiningOp<arith::ConstantOp>()) {
        auto amount = cast<IntegerAttr>(count.getValue()).getValue();
        auto width = cast<IntegerType>(t).getWidth();
        if (amount.ult(width)) {
          auto input = width < 32 ? "uint32_t(" + a(0) + ")" : a(0);
          auto shift = "(" + input + (n == "arith.shli" ? " << " : " >> ") +
                       std::to_string(amount.getZExtValue()) + "u)";
          return width < 32 ? "acir_sim::narrow<" + type(t) + ">(" + shift + ")"
                            : shift;
        }
      }
    if (n == "arith.shli" || n == "arith.shrui" || n == "arith.shrsi")
      return "acir_sim::shift(" + a(0) + "," + a(1) + "," +
             (n == "arith.shli"    ? "0"
              : n == "arith.shrui" ? "1"
                                   : "2") +
             ")";
    if (n == "arith.divui" || n == "arith.divsi" || n == "arith.floordivsi" ||
        n == "arith.remui" || n == "arith.remsi")
      return "acir_sim::divide(" + a(0) + "," + a(1) + "," +
             (n.ends_with("si") ? "true" : "false") + "," +
             (n.starts_with("arith.rem") ? "true" : "false") + "," +
             (n == "arith.floordivsi" ? "true" : "false") + ")";
    throw std::runtime_error("C++ emitter: unsupported operation " + n.str());
  }
  std::string occupy(const std::string &scope, std::string base) {
    base = acir::cppIdentifier(base);
    auto result = base;
    for (unsigned i = 2; !occupied[scope].insert(result).second; ++i)
      result = base + "_" + std::to_string(i);
    return result;
  }
  std::string addScope(std::string scope) {
    if (paths.count(scope))
      return paths.at(scope);
    auto split = llvm::StringRef(scope).rsplit('.');
    auto parent = scope.find('.') == std::string::npos ? "" : split.first.str();
    auto base = parent.empty() ? scope : split.second.str();
    auto path = parent.empty() ? "" : addScope(parent);
    auto field = occupy(parent, base);
    paths[scope] = path.empty() ? field : path + "." + field;
    children[parent][scope] = field;
    std::string definition = base;
    if (auto instances = module->getAttrOfType<ArrayAttr>("acir.instances"))
      for (auto item : instances) {
        auto d = cast<DictionaryAttr>(item);
        if (d.getAs<StringAttr>("name").getValue() == scope)
          definition = d.getAs<StringAttr>("definition").getValue().str();
      }
    scopeTypes[scope] =
        uniqueType(llvm::StringRef(definition).contains('.')
                       ? llvm::StringRef(definition).rsplit('.').second.str()
                       : definition);
    return paths[scope];
  }
  std::string resourceType(acir::StateOp s, unsigned ports) {
    auto cpp = type(s.getPayloadType()), kind = attr(s, "kind");
    if (kind == "queue")
      return "Queue<" + cpp + "," + std::to_string(num(s, "capacity")) + ">";
    if (kind == "ehr")
      return "EHR<" + cpp + "," + std::to_string(ports) + ">";
    if (kind == "cell" || kind == "signal")
      return (kind == "cell" ? "Cell<" : "Signal<") + cpp + ">";
    throw std::runtime_error("C++ emitter: unsupported state " + kind);
  }
  std::string resourceAction(Operation *op, std::string index,
                             std::string action) {
    if (!op->hasAttr("resources"))
      return context + states.at(attr(op, "resource")) + action;
    auto resources = op->getAttrOfType<ArrayAttr>("resources");
    auto key = attr(op, "array");
    if (nativeArrays.count(key)) {
      auto port = op->getAttrOfType<ArrayAttr>("ports");
      // A flat view uses the original multidimensional storage by coordinate.
      auto access = nativeArrays.at(key); // expression has INDEX placeholders
      size_t start = 0;
      while ((start = access.find("INDEX", start)) != std::string::npos) {
        access.replace(start, 5, index);
        start += index.size();
      }
      return context + access + action;
    }
    std::string tuple = "std::tie(";
    for (auto [i, res] : llvm::enumerate(resources))
      tuple += (i ? "," : "") + context +
               states.at(cast<StringAttr>(res).getValue().str());
    return "visit_resource(" + tuple + ")," + index +
           ",[&](auto &resource) { return resource" + action + "; })";
  }
  std::string portExpression(Operation *op, const std::string &index) {
    if (!op->hasAttr("resources"))
      return std::to_string(num(op, "port"));
    std::string result =
        "std::array<size_t," +
        std::to_string(op->getAttrOfType<ArrayAttr>("ports").size()) + ">{";
    for (auto [i, p] : llvm::enumerate(op->getAttrOfType<ArrayAttr>("ports")))
      result += (i ? "," : "") + std::to_string(cast<IntegerAttr>(p).getInt());
    return result + "}.at(" + index + ")";
  }
  std::string readExpression(acir::ReadOp op, Value v) {
    auto method = attr(op, "method");
    bool ready = v == op.getReady();
    auto index = op->hasAttr("resources") ? operands[0] : "";
    std::string action;
    if (ready) {
      if (method == "peek")
        action = ".empty()";
      else if (method == "get")
        action = ".valid";
      else
        action = "";
    } else if (method == "read" || method == "get" || method == "peek")
      action = ".read()";
    else if (method == "ehr.read")
      action = ".read(" + portExpression(op, index) + ")";
    else if (method == "empty" || method == "full" || method == "size")
      action = "." + method + "()";
    else
      throw std::runtime_error("C++ emitter: unsupported read " + method);
    auto result =
        ready && action.empty() ? "true" : resourceAction(op, index, action);
    if (ready && method == "peek")
      result = "(!" + result + ")";
    if (op->hasAttr("resources")) {
      auto valid =
          "(" + index + " < " +
          std::to_string(op->getAttrOfType<ArrayAttr>("resources").size());
      if (operands.size() > 1)
        valid += " && " + operands[1] + ".at(" + index + ")";
      valid += ")";
      result = "(" + valid + " ? " + result + " : " + type(v.getType()) + "{})";
    }
    return result;
  }
  acir::CppStatements statements(std::string prefix) {
    return acir::CppStatements(
        plan, [&](Type t) { return type(t); },
        [&, prefix](Value v, llvm::ArrayRef<std::string> args) {
          context = prefix;
          operands.assign(args.begin(), args.end());
          return expr(v);
        },
        [&, prefix](Value v) {
          auto id = plan.boundary.at(v);
          auto &outputs = plan.blocks[id];
          auto i =
              std::find(outputs.begin(), outputs.end(), v) - outputs.begin();
          return "std::get<" + std::to_string(i) + ">(" +
                 (prefix.empty() ? "this->" : prefix) + blockNames[id] + "())";
        });
  }
  std::string scopeDefinition(const std::string &scope) {
    std::ostringstream out;
    out << "struct " << scopeTypes.at(scope) << " {\n";
    for (auto &[child, field] : children[scope])
      out << scopeDefinition(child) << " " << field << ";\n";
    out << scopeFields[scope].str() << scopeMethods[scope].str() << "}";
    return out.str();
  }
  std::string generate() {
    std::map<std::string, acir::StateOp> declarations;
    unsigned ports = 1;
    for (auto s : circuit.getBody().getOps<acir::StateOp>())
      declarations[s.getSymName().str()] = s;
    module.walk([&](Operation *op) {
      if (op->hasAttr("port"))
        ports = std::max(ports, unsigned(num(op, "port") + 1));
      if (auto ps = op->getAttrOfType<ArrayAttr>("ports"))
        for (auto p : ps)
          ports = std::max(ports, unsigned(cast<IntegerAttr>(p).getInt() + 1));
      auto n = op->getName().getStringRef();
      if (!op->getNumResults() && n != "builtin.module" &&
          n != "acir.circuit" && n != "acir.path" &&
          n != "acir.resource.array" && n != "acir.state" && n != "acir.effect")
        throw std::runtime_error("C++ emitter: unsupported operation " +
                                 n.str());
    });
    addScope("top");
    if (auto instances = module->getAttrOfType<ArrayAttr>("acir.instances"))
      for (auto i : instances)
        addScope(
            cast<DictionaryAttr>(i).getAs<StringAttr>("name").getValue().str());
    // Own each complete frontend component array once. Aliases retain identity.
    std::vector<acir::ResourceArrayOp> arrays;
    for (auto a : circuit.getBody().getOps<acir::ResourceArrayOp>())
      arrays.push_back(a);
    std::stable_sort(arrays.begin(), arrays.end(), [](auto a, auto b) {
      return a.getResources().size() > b.getResources().size();
    });
    for (auto a : arrays) {
      auto name = a.getName().str();
      if (nativeArrays.count(name) || a.getResources().empty())
        continue;
      auto first = cast<StringAttr>(a.getResources()[0]).getValue().str();
      if (!declarations.count(first))
        continue;
      auto cpp = resourceType(declarations.at(first), ports);
      bool owns = true;
      for (auto r : a.getResources()) {
        auto res = cast<StringAttr>(r).getValue().str();
        if (!llvm::StringRef(res).starts_with(name + ".") ||
            states.count(res) ||
            resourceType(declarations.at(res), ports) != cpp)
          owns = false;
      }
      if (!owns)
        continue;
      auto split = llvm::StringRef(name).rsplit('.');
      auto owner = split.first.str();
      auto path = addScope(owner) + "." + occupy(owner, split.second.str());
      auto shape = a.getShape();
      for (auto d : llvm::reverse(shape))
        cpp = "std::array<" + cpp + "," +
              std::to_string(cast<IntegerAttr>(d).getInt()) + ">";
      scopeFields[owner] << cpp << " " << path.substr(path.rfind('.') + 1)
                         << "{};\n";
      std::string indexed = path;
      for (unsigned axis = 0; axis < shape.size(); ++axis) {
        int64_t stride = 1;
        for (unsigned j = axis + 1; j < shape.size(); ++j)
          stride *= cast<IntegerAttr>(shape[j]).getInt();
        auto size = cast<IntegerAttr>(shape[axis]).getInt();
        if (shape.size() == 1)
          indexed += ".at(INDEX)";
        else
          indexed += ".at((INDEX / " + std::to_string(stride) + ") % " +
                     std::to_string(size) + ")";
      }
      nativeArrays[name] = indexed;
      for (auto [i, r] : llvm::enumerate(a.getResources())) {
        std::string access = path;
        int64_t flat = i;
        for (unsigned axis = 0; axis < shape.size(); ++axis) {
          int64_t stride = 1;
          for (unsigned j = axis + 1; j < shape.size(); ++j)
            stride *= cast<IntegerAttr>(shape[j]).getInt();
          access += "[" + std::to_string(flat / stride) + "]";
          flat %= stride;
        }
        states[cast<StringAttr>(r).getValue().str()] = access;
      }
    }
    for (auto &[name, s] : declarations) {
      if (!states.count(name)) {
        auto split = llvm::StringRef(name).rsplit('.');
        auto owner = split.first.str();
        auto field = occupy(owner, split.second.str());
        states[name] = addScope(owner) + "." + field;
        scopeFields[owner] << resourceType(s, ports) << " " << field << "{};\n";
      }
      auto id = states.at(name), kind = attr(s, "kind");
      auto t = s.getPayloadType();
      auto initial = s->getAttr("initial");
      reset << id << "={};\n";
      if (kind == "queue") {
        for (auto [i, item] : llvm::enumerate(cast<ArrayAttr>(initial)))
          reset << id << ".data[" << i << "]=" << literal(item, t) << ";\n";
        reset << id << ".count=" << cast<ArrayAttr>(initial).size() << ";\n";
      } else
        reset << id << ".value=" << literal(initial, t) << ";\n";
      begin << id << ".begin();\n";
      commit << id << ".commit();\n";
      if (kind != "signal") {
        if (trace.tellp() > 0)
          trace << "o<<',';\n";
        trace << "o<<" << quote(quote(name) + ":") << "; json(o," << id
              << (kind == "queue" ? "" : ".value") << ");\n";
      }
    }
    std::set<std::string> blockUsed;
    for (auto &outputs : plan.blocks) {
      auto base = acir::cppSourceName(outputs[0]);
      auto primary = outputs[0];
      while (primary.getDefiningOp()->getName().getStringRef() == "acir.bind" ||
             primary.getDefiningOp()->getName().getStringRef() ==
                 "acir.export") {
        auto name = acir::cppSourceName(primary);
        if (!name.empty())
          base = name;
        primary = primary.getDefiningOp()->getOperand(0);
      }
      if (base.empty()) {
        auto *op = outputs[0].getDefiningOp();
        base = attr(op, "resource");
        if (base.empty())
          base = "shared_computation";
        else
          base = llvm::StringRef(base).rsplit('.').second.str() + "_view";
      }
      base = "compute_" + acir::cppIdentifier(base);
      auto name = base;
      for (unsigned i = 2; !blockUsed.insert(name).second; ++i)
        name = base + "_" + std::to_string(i);
      blockNames.push_back(name);
    }
    for (auto [id, outputs] : llvm::enumerate(plan.blocks)) {
      auto fn = blockNames[id], cache = fn + "_cache";
      std::string result = "std::tuple<";
      for (auto [i, v] : llvm::enumerate(outputs))
        result += (i ? "," : "") + type(v.getType());
      result += ">";
      fields << "std::optional<" << result << "> " << cache << ";\n";
      begin << cache << ".reset();\n";
      auto stmt = statements("");
      for (auto v : outputs)
        stmt.ignored.insert(v);
      std::string tuple = result + "{";
      for (auto [i, v] : llvm::enumerate(outputs))
        tuple += (i ? "," : "") + stmt.value(v);
      tuple += "}";
      methods << "const " << result << " &" << fn << "() {\nif (!" << cache
              << ") {\n"
              << stmt.render() << cache << ".emplace(" << tuple
              << ");\n}\nreturn *" << cache << ";\n}\n";
    }
    std::ostringstream tick, fireTrace;
    std::map<std::string, unsigned> ruleNames;
    unsigned fireIndex = 0;
    for (auto id : circuit->getAttrOfType<ArrayAttr>("schedule")) {
      auto txn = cast<IntegerAttr>(id).getInt();
      acir::FireOp fire;
      for (auto f : circuit.getBody().getOps<acir::FireOp>())
        if (num(f, "txn") == txn)
          fire = f;
      if (!fire)
        throw std::runtime_error(
            "C++ emitter: schedule references missing transaction");
      auto full = fire.getName().split('#').first;
      auto split = full.rsplit('.');
      auto owner = split.first.str();
      auto path = addScope(owner);
      auto fn = occupy(owner, "try_" + split.second.str());
      auto fired = occupy("", "fired_" + acir::cppIdentifier(full));
      fields << "bool " << fired << "=false;\n";
      begin << fired << "=false;\n";
      tick << path << "." << fn << "(*this);\n";
      if (fireIndex++)
        fireTrace << "o<<',';\n";
      fireTrace << "o<<" << quote(quote(fire.getName()) + ":") << "; json(o,"
                << fired << ");\n";
      auto stmt = statements("ctx.");
      for (auto check : fire.getChecks()) {
        auto selection = check.getDefiningOp<arith::SelectOp>();
        auto assertion =
            selection ? selection.getTrueValue().getDefiningOp<acir::AssertOp>()
                      : acir::AssertOp();
        auto alternative =
            selection
                ? selection.getFalseValue().getDefiningOp<arith::ConstantOp>()
                : arith::ConstantOp();
        if (assertion && alternative &&
            cast<IntegerAttr>(alternative.getValue()).getValue().isOne()) {
          auto enabled = stmt.value(selection.getCondition());
          stmt.conditional(enabled, [&] {
            auto condition = stmt.value(assertion.getCondition());
            stmt.line("acir_sim::check(" + condition + ");");
          });
        } else {
          auto value = stmt.value(check);
          stmt.line("(void)(" + value + ");");
        }
        stmt.assume(check);
      }
      auto condition = stmt.value(fire.getCondition());
      stmt.line("if (!(" + condition + ")) return;");
      stmt.assume(fire.getCondition());
      stmt.available[fire.getFire()] = "true";
      std::ostringstream publish;
      stmt.line("// Prepare every effect before publishing state operations.");
      bool pops = false;
      std::set<std::string> popResources;
      for (auto e : circuit.getBody().getOps<acir::EffectOp>())
        if (num(e, "txn") == txn && attr(e, "method") == "pop") {
          if (auto resources = e->getAttrOfType<ArrayAttr>("resources")) {
            for (auto r : resources)
              pops |= !popResources.insert(cast<StringAttr>(r).getValue().str())
                           .second;
          } else
            pops |= !popResources.insert(attr(e, "resource")).second;
        }
      auto popped = stmt.name("popped_resources");
      if (pops)
        stmt.line("std::set<const void *> " + popped + ";");
      for (auto e : circuit.getBody().getOps<acir::EffectOp>())
        if (num(e, "txn") == txn) {
          auto method = attr(e, "method");
          auto enabled = stmt.value(e.getEnable());
          if (enabled == "false")
            continue;
          auto local = enabled;
          if (enabled != "true") {
            local = stmt.name(method + "_fire");
            stmt.line("const bool " + local + " = " + enabled + ";");
          }
          std::string input, index;
          if (method != "pop" && method != "check") {
            input = stmt.name(method + "_value");
            if (enabled != "true")
              stmt.line(type(e.getInputs()[0].getType()) + " " + input + "{};");
          }
          if (e->hasAttr("resources")) {
            index = stmt.name("resource_index");
            if (enabled != "true")
              stmt.line("uint64_t " + index + "{};");
          }
          auto prepare = [&] {
            if (method == "check") {
              auto value = stmt.value(e.getInputs()[0]);
              if (value != "true")
                stmt.line("acir_sim::check(" + value + ");");
              return;
            }
            if (!input.empty()) {
              auto value = stmt.value(e.getInputs()[0]);
              stmt.line((enabled == "true"
                             ? "const " + type(e.getInputs()[0].getType()) + " "
                             : "") +
                        input + " = " + value + ";");
            }
            if (!index.empty()) {
              auto value = stmt.value(e->getOperand(method == "pop" ? 1 : 2));
              stmt.line((enabled == "true" ? "const uint64_t " : "") + index +
                        " = " + value + ";");
            }
          };
          if (enabled == "true")
            prepare();
          else if (method != "pop" || !index.empty())
            stmt.conditional(local, prepare);
          if (method == "check")
            continue;
          context = "ctx.";
          std::string action;
          if (method == "pop") {
            // Deduplicate by runtime component identity, including dynamic
            // aliases.
            action = "";
            auto resource = resourceAction(e, index, "");
            if (e->hasAttr("resources") &&
                !nativeArrays.count(attr(e, "array"))) {
              action = resourceAction(
                  e, index, ".pop()"); // handled below through generic callback
              auto resources = e->getAttrOfType<ArrayAttr>("resources");
              std::string tuple = "std::tie(";
              for (auto [i, r] : llvm::enumerate(resources))
                tuple += (i ? "," : "") + context +
                         states.at(cast<StringAttr>(r).getValue().str());
              action =
                  "visit_resource(" + tuple + ")," + index +
                  ",[&](auto &resource) { " +
                  (pops ? "if(" + popped + ".insert(&resource).second) " : "") +
                  "resource.pop(); })";
            } else
              action = (pops ? "if (" + popped + ".insert(&" + resource +
                                   ").second) "
                             : "") +
                       resource + ".pop()";
          } else if (method == "push")
            action = resourceAction(e, index, ".push(" + input + ")");
          else if (method == "write" || method == "set" ||
                   method == "ehr.write")
            action = resourceAction(e, index,
                                    ".write(" +
                                        (method == "ehr.write"
                                             ? portExpression(e, index) + ","
                                             : "") +
                                        input + ")");
          else
            throw std::runtime_error("C++ emitter: unsupported effect " +
                                     method);
          if (enabled == "true")
            publish << action << ";\n";
          else
            publish << "if (" << local << ") { " << action << "; }\n";
        }
      scopeMethods[owner] << "// Rule " << fire.getName().str()
                          << "\ntemplate<class Context> void " << fn
                          << "(Context &ctx) {\n"
                          << stmt.render() << publish.str() << "ctx." << fired
                          << "=true;\n}\n";
    }
    std::ostringstream out;
    out << "#pragma once\n#include \"runtime.hpp\"\n#include <set>\nnamespace "
           "acir_generated {\nusing namespace acir_sim;\nusing "
           "acir_sim::json;\n";
    auto hierarchy = scopeDefinition("top");
    out << definitions.str() << "class Model {\npublic:\n"
        << hierarchy << " top;\n"
        << fields.str() << "Model() { reset(); }\nvoid reset() {\n"
        << reset.str() << "begin_tick();\n}\nvoid begin_tick() {\n"
        << begin.str() << "}\nvoid commit() {\n"
        << commit.str() << "}\nvoid tick() {\nbegin_tick();\n"
        << tick.str()
        << "commit();\n}\nvoid trace(std::ostream &o,uint64_t tick) const { "
           "o<<\"{\\\"tick\\\":\"<<tick<<\",\\\"fire\\\":{\";\n"
        << fireTrace.str() << "o<<\"},\\\"state\\\":{\";\n"
        << trace.str() << "o<<\"}}\"; }\n"
        << methods.str() << "};\n} // namespace acir_generated\n";
    return out.str();
  }
};
const char *driver = R"CPP(#include "model.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
int main(int argc,char **argv) {
  try {
    uint64_t cycles=0; std::string path;
    for(int i=1;i<argc;++i) {
      std::string_view option=argv[i];
      if(option=="--cycles" && i+1<argc) {
        std::string_view value=argv[++i];
        auto [end,ec]=std::from_chars(value.data(),value.data()+value.size(),cycles);
        if(ec!=std::errc{} || end!=value.data()+value.size()) throw std::runtime_error("invalid cycle count");
      } else if(option=="--trace" && i+1<argc) path=argv[++i];
      else throw std::runtime_error("usage: sim --cycles N [--trace FILE]");
    }
    std::ofstream trace;
    if(!path.empty()) { trace.open(path); if(!trace) throw std::runtime_error("cannot open trace"); trace<<"{\"trace\":["; }
    auto model=std::make_unique<acir_generated::Model>();
    for(uint64_t i=0;i<cycles;++i) { model->tick(); if(trace.is_open()) { if(i)trace<<','; model->trace(trace,i); } }
    if(trace.is_open()) { trace<<"]}\n"; trace.flush(); if(!trace) throw std::runtime_error("cannot write trace"); }
    return 0;
  } catch(const std::exception &e) { std::cerr<<"sim: "<<e.what()<<'\n'; return 1; }
}
)CPP";
// The emitter already chooses statement and expression boundaries. Indent
// those lines without depending on an installed formatter in standalone use.
std::string indentCpp(StringRef input) {
  std::string output;
  SmallVector<StringRef> lines;
  input.split(lines, '\n');
  int depth = 0;
  for (auto line : lines) {
    line = line.trim();
    if (line.empty())
      continue;
    auto leading = line.starts_with("}") || line == "public:" ? 1 : 0;
    if (!line.starts_with("#"))
      output.append(2 * std::max(0, depth - leading), ' ');
    output += line.str() + '\n';
    char quote = 0;
    for (size_t i = 0; i < line.size(); ++i) {
      char c = line[i];
      if (quote) {
        if (c == '\\')
          ++i;
        else if (c == quote)
          quote = 0;
      } else if (c == '\'' || c == '"')
        quote = c;
      else if (c == '/' && i + 1 < line.size() && line[i + 1] == '/')
        break;
      else if (c == '{')
        ++depth;
      else if (c == '}')
        --depth;
    }
  }
  return output;
}
void save(const std::filesystem::path &path, const std::string &content) {
  std::ofstream out(path);
  out << content;
  if (!out)
    throw std::runtime_error("C++ emitter: cannot write " + path.string());
}
} // namespace
void acir::emitCpp(mlir::ModuleOp module,
                   const std::filesystem::path &directory) {
  auto stage = module->getAttrOfType<StringAttr>("acir.stage");
  if (!stage || stage.getValue() != "transaction")
    throw std::runtime_error("C++ emitter requires transaction MLIR");
  Emitter emitter(module);
  auto model = emitter.generate(); // Validate before creating artifacts.
  std::filesystem::create_directories(directory);
  save(directory / "model.hpp", indentCpp(model));
  save(directory / "runtime.hpp", indentCpp(cppRuntime));
  save(directory / "sim.cpp", indentCpp(driver));
  auto &plan = emitter.plan;
  llvm::json::Object stats{{"conditional_cycles", plan.conditionalCycles},
                           {"demand_limits", plan.demandLimits},
                           {"local_lazy_blocks", plan.lazyBlocks},
                           {"local_memoized_blocks", plan.memoizedBlocks}};
  std::string statistics;
  llvm::raw_string_ostream stream(statistics);
  stream << llvm::json::Value(std::move(stats));
  save(directory / "lowering.json", statistics + "\n");
}

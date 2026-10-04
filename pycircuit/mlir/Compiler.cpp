#include "Dialect.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/Cpp/CppEmitter.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
using namespace mlir;
using std::string;
using Dict = DictionaryAttr;
using Strings = std::set<string>;
static string str(Attribute a) { return cast<StringAttr>(a).getValue().str(); }
static string get(Dict a, StringRef k) {
  if (!a)
    return "";
  auto v = a.getAs<StringAttr>(k);
  return v ? v.getValue().str() : "";
}
static string get(Operation *o, StringRef k) {
  auto v = o->getAttrOfType<StringAttr>(k);
  return v ? v.getValue().str() : "";
}
static ArrayAttr arr(Dict a, StringRef k) {
  auto v = a.getAs<ArrayAttr>(k);
  return v ? v : ArrayAttr::get(a.getContext(), {});
}
static string join(const std::vector<string> &v, const string &sep = ", ") {
  string s;
  for (auto &x : v) {
    if (!s.empty())
      s += sep;
    s += x;
  }
  return s;
}
static string ident(StringRef name) {
  static const Strings reserved = {
      "alignas",       "alignof",     "and",
      "and_eq",        "asm",         "auto",
      "bitand",        "bitor",       "bool",
      "break",         "case",        "catch",
      "char",          "char8_t",     "char16_t",
      "char32_t",      "class",       "compl",
      "concept",       "const",       "consteval",
      "constexpr",     "constinit",   "const_cast",
      "continue",      "co_await",    "co_return",
      "co_yield",      "decltype",    "default",
      "delete",        "do",          "double",
      "dynamic_cast",  "else",        "enum",
      "explicit",      "export",      "extern",
      "false",         "float",       "for",
      "friend",        "goto",        "if",
      "inline",        "int",         "long",
      "mutable",       "namespace",   "new",
      "noexcept",      "not",         "not_eq",
      "nullptr",       "operator",    "or",
      "or_eq",         "private",     "protected",
      "public",        "register",    "reinterpret_cast",
      "requires",      "return",      "short",
      "signed",        "sizeof",      "static",
      "static_assert", "static_cast", "struct",
      "switch",        "template",    "this",
      "thread_local",  "throw",       "true",
      "try",           "typedef",     "typeid",
      "typename",      "union",       "unsigned",
      "using",         "virtual",     "void",
      "volatile",      "wchar_t",     "while",
      "xor",           "xor_eq"};
  if (!reserved.count(name.str()) && !name.starts_with("ac_py_") &&
      !name.starts_with("__"))
    return name.str();
  string out = "ac_py_";
  for (unsigned char c : name) {
    const char *hex = "0123456789abcdef";
    out += hex[c >> 4];
    out += hex[c & 15];
  }
  return out;
}
static string elem(string t) {
  auto p = t.find('<');
  return t.substr(p + 1, t.size() - p - 2);
}
static string cpp(string t) {
  if (t == "bool" || t == "void")
    return t;
  if (t.size() > 1 && (t[0] == 'u' || t[0] == 'i') && isdigit(t[1]))
    return "std::" + string(t[0] == 'u' ? "u" : "") + "int" + t.substr(1) +
           "_t";
  if (t.starts_with("array<")) {
    string e = elem(t);
    auto pos = e.rfind(',');
    return "std::array<" + cpp(e.substr(0, pos)) + ", " + e.substr(pos + 1) +
           ">";
  }
  if (t.starts_with("queue<"))
    return "gfsim::Queue<" + cpp(elem(t)) + ">*";
  if (t.starts_with("signal<"))
    return "gfsim::Signal<" + cpp(elem(t)) + ">*";
  if (t.starts_with("qarray<"))
    return "ac_detail::QueueRefs<" + cpp(elem(t)) + ">";
  if (t.starts_with("vector<"))
    return "std::vector<" + cpp(elem(t)) + ">";
  return ident(t);
}
static string sourceType(Type t) {
  if (auto i = dyn_cast<IntegerType>(t))
    return i.getWidth() == 1 ? "bool" : "u" + std::to_string(i.getWidth());
  if (auto v = dyn_cast<acir::StructType>(t))
    return v.getName().str();
  if (auto v = dyn_cast<acir::QueueType>(t))
    return "queue<" + sourceType(v.getElementType()) + ">";
  if (auto v = dyn_cast<acir::SignalType>(t))
    return "signal<" + sourceType(v.getElementType()) + ">";
  if (auto v = dyn_cast<acir::QueueArrayType>(t))
    return "qarray<" + sourceType(v.getElementType()) + ">";
  if (auto v = dyn_cast<acir::VectorType>(t))
    return "vector<" + sourceType(v.getElementType()) + ">";
  if (auto v = dyn_cast<acir::ArrayType>(t))
    return "array<" + sourceType(v.getElementType()) + "," +
           std::to_string(v.getSize()) + ">";
  throw std::runtime_error("unsupported ACIR type");
}
static bool resource(Value v) {
  return isa<acir::QueueType, acir::SignalType, acir::QueueArrayType>(
      v.getType());
}
static string valueSourceType(Value value) {
  if (auto *op = value.getDefiningOp()) {
    auto t = get(op, "acir.source_type");
    if (!t.empty())
      return t;
  }
  if (auto loc = dyn_cast<NameLoc>(value.getLoc())) {
    if (loc.getName().getValue().starts_with("acir.type:"))
      return loc.getName().getValue().drop_front(10).str();
  }
  if (auto arg = dyn_cast<BlockArgument>(value)) {
    auto f = dyn_cast<func::FuncOp>(arg.getOwner()->getParentOp());
    if (f && arg.getOwner() == &f.getBody().front()) {
      auto meta = f->getAttrOfType<Dict>("acir.function");
      if (meta)
        return get(cast<Dict>(arr(meta, "params")[arg.getArgNumber()]), "type");
    }
  }
  return sourceType(value.getType());
}
struct Analysis {
  std::map<string, Strings> accesses, signals;
  std::map<string, std::map<string, Strings>> effects;
  DenseMap<Value, Strings> origins;
  std::map<string, func::FuncOp> rules;
  explicit Analysis(ModuleOp m) {
    for (auto f : m.getOps<func::FuncOp>()) {
      auto a = f->getAttrOfType<Dict>("acir.function");
      if (get(a, "kind") == "rule")
        rules[get(a, "owner") + "." + get(a, "rule")] = f;
    }
    bool changed;
    do {
      changed = false;
      auto merge = [&](Value v, Strings s) {
        if (!resource(v))
          return;
        auto &d = origins[v];
        auto n = d.size();
        d.insert(s.begin(), s.end());
        changed |= d.size() != n;
      };
      m.walk([&](Operation *o) {
        if (isa<acir::GetOp>(o))
          merge(o->getResult(0), {get(o, "name")});
        else if (auto b = dyn_cast<cf::BranchOp>(o)) {
          for (auto [v, a] :
               llvm::zip(b.getDestOperands(), b.getDest()->getArguments()))
            merge(a, origins[v]);
        } else if (auto b = dyn_cast<cf::CondBranchOp>(o)) {
          for (auto [v, a] : llvm::zip(b.getTrueDestOperands(),
                                       b.getTrueDest()->getArguments()))
            merge(a, origins[v]);
          for (auto [v, a] : llvm::zip(b.getFalseDestOperands(),
                                       b.getFalseDest()->getArguments()))
            merge(a, origins[v]);
        } else if (isa<acir::ExtractOp>(o) && o->getNumOperands() == 2 &&
                   isa<acir::QueueArrayType>(o->getOperand(0).getType()) &&
                   o->getOperand(0).getDefiningOp<acir::GetOp>() &&
                   o->getOperand(1).getDefiningOp<arith::ConstantOp>()) {
          auto index = cast<IntegerAttr>(o->getOperand(1)
                                             .getDefiningOp<arith::ConstantOp>()
                                             .getValue())
                           .getInt();
          auto table = o->getOperand(0).getDefiningOp<acir::GetOp>();
          merge(o->getResult(0),
                {get(table, "name") + "[" + std::to_string(index) + "]"});
        } else if (isa<acir::InvokeOp>(o)) {
          auto owner =
              get(o->getParentOfType<func::FuncOp>()->getAttrOfType<Dict>(
                      "acir.function"),
                  "owner");
          auto f = rules.at(owner + "." + get(o, "name"));
          for (auto [v, a] : llvm::zip(o->getOperands(), f.getArguments()))
            merge(a, origins[v]);
        } else {
          Strings s;
          for (auto a : o->getOperands())
            s.insert(origins[a].begin(), origins[a].end());
          for (auto r : o->getResults())
            merge(r, s);
        }
      });
    } while (changed);
    m.walk([&](Operation *o) {
      if (!isa<acir::ReadOp, acir::QueryOp, acir::PopOp, acir::PushOp,
               acir::ReviseOp>(o))
        return;
      auto f = o->getParentOfType<func::FuncOp>();
      auto name = f.getSymName().str();
      auto s = origins[o->getOperand(0)];
      accesses[name].insert(s.begin(), s.end());
      if (sourceType(o->getOperand(0).getType()).starts_with("signal<"))
        signals[name].insert(s.begin(), s.end());
      string e = isa<acir::PopOp>(o)      ? "Pop"
                 : isa<acir::PushOp>(o)   ? "Push"
                 : isa<acir::ReviseOp>(o) ? "Revise"
                                          : "";
      if (!e.empty())
        for (auto &q : s)
          effects[name][q].insert(e);
    });
    // Signals subscribe to their complete static port bindings, including
    // unused inputs and every element of a bound resource array.
    for (auto r : m.getOps<acir::ResourceOp>()) {
      if (get(r, "kind") != "signal")
        continue;
      auto &inputs = accesses[get(r, "evaluate_fn")];
      if (auto ports = r->getAttrOfType<ArrayAttr>("inputs"))
        for (auto port : ports)
          inputs.insert(str(port));
    }
    // A whole-table declaration subsumes its element declarations. Avoid
    // duplicate registration when a function mixes static and dynamic indices.
    for (auto &[function, reads] : accesses) {
      auto snapshot = reads;
      for (auto &q : snapshot) {
        auto bracket = q.find('[');
        if (bracket != string::npos && reads.count(q.substr(0, bracket)))
          reads.erase(q);
      }
    }
    for (auto &[function, proposals] : effects) {
      auto snapshot = proposals;
      for (auto &[q, operations] : snapshot) {
        auto bracket = q.find('[');
        if (bracket == string::npos)
          continue;
        auto whole = proposals.find(q.substr(0, bracket));
        if (whole == proposals.end())
          continue;
        for (auto &e : whole->second)
          proposals[q].erase(e);
        if (proposals[q].empty())
          proposals.erase(q);
      }
    }
  }
};
namespace acir {
#define GEN_PASS_DEF_ANALYZERESOURCES
#define GEN_PASS_DEF_LOWERGFSIM
#define GEN_PASS_DEF_CONVERTTOEMITC
#include "ACIRPasses.h.inc"
} // namespace acir
struct AnalyzeResourcesPass
    : acir::impl::AnalyzeResourcesBase<AnalyzeResourcesPass> {
  void runOnOperation() override {
    auto module = getOperation();
    auto model = module->getAttrOfType<Dict>("acir.model");
    if (!model || !model.getAs<IntegerAttr>("format_version") ||
        model.getAs<IntegerAttr>("format_version").getInt() != 2) {
      module.emitError("expected ACIR model format_version = 2");
      signalPassFailure();
      return;
    }
    for (auto f : module.getOps<func::FuncOp>())
      if (!f->hasAttr("acir.function") && !f->hasAttr("acir.runtime")) {
        f.emitError("missing acir.function metadata");
        signalPassFailure();
        return;
      }
    Analysis analysis(module);
    OpBuilder b(module.getContext());
    auto list = [&](const Strings &values) {
      SmallVector<Attribute> a;
      for (auto &s : values)
        a.push_back(b.getStringAttr(s));
      return b.getArrayAttr(a);
    };
    for (auto f : module.getOps<func::FuncOp>()) {
      string name = f.getSymName().str();
      f->setAttr("acir.accesses", list(analysis.accesses[name]));
      f->setAttr("acir.signals", list(analysis.signals[name]));
      SmallVector<NamedAttribute> effects;
      for (auto &[q, es] : analysis.effects[name])
        effects.push_back(b.getNamedAttr(q, list(es)));
      f->setAttr("acir.effects", b.getDictionaryAttr(effects));
    }
  }
};
// Expand transaction boundaries before type conversion. Every required read
// splits its original block; no check can move to an untaken branch.
struct LowerGFSimPass : acir::impl::LowerGFSimBase<LowerGFSimPass> {
  void runOnOperation() override {
    ModuleOp module = getOperation();
    OpBuilder b(module.getContext());
    SmallVector<func::FuncOp> functions(module.getOps<func::FuncOp>());
    unsigned serial = 0;
    for (auto f : functions) {
      if (f.isExternal())
        continue;
      auto meta = f->getAttrOfType<Dict>("acir.function");
      auto role = get(meta, "kind");
      if (role != "rule" && role != "work")
        continue;
      auto runtime = [&](StringRef action, TypeRange result,
                         ValueRange operands, string source = "void") {
        string name = "ac_runtime_" + std::to_string(++serial);
        auto decl = func::FuncOp::create(
            f.getLoc(), name, b.getFunctionType(operands.getTypes(), result));
        decl.setPrivate();
        decl->setAttr("acir.runtime", b.getStringAttr(action));
        module.getBody()->push_back(decl);
        auto call = func::CallOp::create(b, f.getLoc(), decl, operands);
        call->setAttr("acir.source_type", b.getStringAttr(source));
        return call;
      };
      SmallVector<func::ReturnOp> exits;
      f.walk([&](func::ReturnOp r) { exits.push_back(r); });
      if (role == "rule")
        for (auto ret : exits) {
          b.setInsertionPoint(ret);
          runtime("complete", {}, {});
        }
      auto split = [&](Operation *before,
                       const std::function<Value()> &condition, bool abort) {
        Block *parent = before->getBlock();
        Block *resume = parent->splitBlock(before);
        auto *missing = new Block;
        f.getBody().push_back(missing);
        b.setInsertionPointToEnd(parent);
        Value ready = condition();
        cf::CondBranchOp::create(b, before->getLoc(), ready, resume, missing);
        b.setInsertionPointToEnd(missing);
        if (abort && role == "rule")
          runtime("abort", {}, {});
        func::ReturnOp::create(b, before->getLoc());
        b.setInsertionPointToStart(resume);
      };
      if (role == "rule") {
        auto *first = &f.getBody().front().front();
        split(
            first,
            [&] {
              runtime("start", {}, {});
              return runtime("begin", b.getI1Type(), f.getArguments(), "bool")
                  .getResult(0);
            },
            false);
      }
      SmallVector<Operation *> required;
      f.walk([&](Operation *op) {
        if (isa<acir::PopOp, acir::ReviseOp>(op) ||
            (isa<acir::ReadOp>(op) &&
             isa<acir::QueueType>(op->getOperand(0).getType())))
          required.push_back(op);
      });
      for (auto *op : required) {
        Value queue = op->getOperand(0), pointer;
        bool read = isa<acir::ReadOp>(op);
        string payload = read ? get(op, "acir.source_type") : "";
        split(
            op,
            [&] {
              if (!read)
                return runtime("nonempty", b.getI1Type(), queue, "bool")
                    .getResult(0);
              string pt = "const " + cpp(payload) + "*";
              pointer = runtime("tryPeek",
                                emitc::OpaqueType::get(module.getContext(), pt),
                                queue, pt)
                            .getResult(0);
              return runtime("present", b.getI1Type(), pointer, "bool")
                  .getResult(0);
            },
            true);
        if (read) {
          auto value = runtime("load", op->getResultTypes(), pointer, payload)
                           .getResult(0);
          op->getResult(0).replaceAllUsesWith(value);
          op->erase();
        }
      }
      f->setAttr("acir.lifecycle", b.getUnitAttr());
    }
  }
};
// All generated declarations and registration use the same static MLIR
// metadata.
struct Layout {
  ModuleOp module;
  Dict model;
  std::map<string, Dict> functions, resources;
  Analysis analysis;
  string top;
  explicit Layout(ModuleOp m)
      : module(m), model(m->getAttrOfType<Dict>("acir.model")), analysis(m),
        top(ident(get(model, "top"))) {
    for (auto f : m.getOps<func::FuncOp>())
      functions[f.getSymName().str()] = f->getAttrOfType<Dict>("acir.function");
    for (auto r : m.getOps<acir::ResourceOp>())
      resources[get(r, "name")] = r->getAttrDictionary();
  }
  string signature(Dict fn, bool names = true) {
    std::vector<string> p;
    for (auto a : arr(fn, "params")) {
      auto d = cast<Dict>(a);
      p.push_back(cpp(get(d, "type")) +
                  (names ? " " + ident(get(d, "name")) : ""));
    }
    return join(p);
  }
  string functionName(Dict fn) {
    auto kind = get(fn, "kind");
    if (kind == "rule")
      return "Module_" + ident(get(fn, "owner")) + "::work_" +
             ident(get(fn, "rule"));
    if (kind == "work")
      return "Module_" + ident(get(fn, "owner")) + "::Work";
    if (kind == "signal")
      return top + "::evaluate_" + ident(get(fn, "owner"));
    return ident(get(fn, "name"));
  }
  string configArgs() {
    std::vector<string> v;
    for (auto a : arr(model, "config"))
      v.push_back(ident(get(cast<Dict>(a), "name")));
    return join(v);
  }
  string init(Dict r, StringRef key) {
    return ident(get(r, key)) + "(" + configArgs() + ")";
  }
  string each(string name, const std::function<string(string)> &action) {
    auto bracket = name.find('[');
    if (bracket != string::npos) {
      auto index = name.substr(bracket + 1, name.size() - bracket - 2);
      return action("*" + ident(name.substr(0, bracket)) + ".refs.at(" + index +
                    ")");
    }
    return get(resources.at(name), "kind") == "qarray"
               ? "for (auto* q : " + ident(name) + ".refs) " + action("*q")
               : action(ident(name));
  }
  void header(std::ostream &h) {
    h << "// Generated from ACIR MLIR.\n#pragma once\n#include "
         "\"ac_support.hpp\"\nnamespace ac_generated {\n";
    for (auto a : model.getAs<Dict>("constants"))
      h << "inline constexpr std::uint32_t " << ident(a.getName()) << " = "
        << cast<IntegerAttr>(a.getValue()).getInt() << "U;\n";
    for (auto a : arr(model, "type_order")) {
      auto rawName = str(a);
      auto fields = model.getAs<Dict>("types").getAs<ArrayAttr>(rawName);
      auto name = ident(rawName);
      h << "struct " << name << " {\n";
      for (auto field : fields) {
        auto d = cast<Dict>(field);
        h << "  " << cpp(get(d, "type")) << " " << ident(get(d, "name"))
          << "{};\n";
      }
      h << "  bool operator==(const " << name << "&) const = default;\n};\n";
    }
    for (auto &[name, f] : functions)
      if (get(f, "kind") == "helper" || get(f, "kind") == "initializer")
        h << cpp(get(f, "result")) << " " << ident(name) << "(" << signature(f)
          << ");\n";
    h << "struct " << top << ";\n";
    for (auto ma : arr(model, "modules")) {
      auto m = cast<Dict>(ma);
      auto name = ident(get(m, "name"));
      h << "struct Module_" << name << " {\n  " << top
        << "& model;\n  gfsim::ModuleId mid{};\n  void Work();\n";
      for (auto ra : arr(m, "rules")) {
        auto r = cast<Dict>(ra), f = functions.at(get(r, "function"));
        auto rn = ident(get(r, "name"));
        h << "  gfsim::RuleId rid_" << rn << "{};\n";
        if (!arr(f, "params").empty()) {
          h << "  struct Args_" << rn << " {\n";
          for (auto pa : arr(f, "params")) {
            auto p = cast<Dict>(pa);
            h << "    " << cpp(get(p, "type")) << " " << ident(get(p, "name"))
              << ";\n";
          }
          h << "    bool operator==(const Args_" << rn
            << "&) const = default;\n  };\n  gfsim::ParameterCache<Args_" << rn
            << "> cache_" << rn << ";\n";
        }
        h << "  void work_" << rn << "(" << signature(f)
          << ");\n  bool arbitrate_" << rn << "();\n";
      }
      h << "};\n";
    }
    h << "struct " << top << " {\n";
    for (auto pa : arr(model, "config")) {
      auto p = cast<Dict>(pa);
      h << "  const " << cpp(get(p, "type")) << " " << ident(get(p, "name"))
        << ";\n";
    }
    // Preserve declaration order for construction dependencies and
    // deterministic IDs.
    for (auto op : module.getOps<acir::ResourceOp>()) {
      auto r = op->getAttrDictionary();
      auto name = ident(get(r, "name")), t = cpp(get(r, "type")),
           k = get(r, "kind");
      if (k == "queue")
        h << "  gfsim::Queue<" << t << "> " << name << "{"
          << init(r, "capacity_fn") << ", " << init(r, "initial_fn") << "};\n";
      else if (k == "qarray")
        h << "  ac_detail::QueueArray<" << t << "> " << name << "{"
          << init(r, "iterable_fn") << ", " << init(r, "capacity_fn") << ", "
          << ident(get(r, "initializer_fn"))
          << (r.getAs<BoolAttr>("empty_initial") &&
                      r.getAs<BoolAttr>("empty_initial").getValue()
                  ? ", false"
                  : "")
          << "};\n";
      else
        h << "  " << t << " evaluate_" << name << "();\n  gfsim::Signal<" << t
          << "> " << name << "{this, [](void* p) { return static_cast<" << top
          << "*>(p)->evaluate_" << name << "(); }};\n";
    }
    Strings members;
    for (auto ma : arr(model, "modules"))
      members.insert(get(cast<Dict>(ma), "name"));
    if (auto exports = model.getAs<Dict>("exports"))
      for (auto alias : exports) {
        string n = alias.getName().str(), target = str(alias.getValue());
        if (n == target || members.count(n) || resources.count(n))
          continue;
        auto r = resources.at(target);
        auto k = get(r, "kind");
        h << "  "
          << (k == "queue"    ? "gfsim::Queue<"
              : k == "signal" ? "gfsim::Signal<"
                              : "ac_detail::QueueArray<")
          << cpp(get(r, "type")) << ">& " << ident(n) << "{" << ident(target)
          << "};\n";
      }
    h << "  gfsim::Simulator sim;\n";
    for (auto ma : arr(model, "modules")) {
      auto n = ident(get(cast<Dict>(ma), "name"));
      h << "  Module_" << n << " " << n << "{*this};\n";
    }
    h << "  " << top << "(" << constructorSignature(true) << ");\n};\n}\n";
  }
  string constructorSignature(bool defaults) {
    std::vector<string> ps;
    for (auto pa : arr(model, "config")) {
      auto p = cast<Dict>(pa);
      auto t = get(p, "type");
      ps.push_back((t.starts_with("vector<")
                        ? "std::span<const " + cpp(elem(t)) + ">"
                        : cpp(t)) +
                   " arg_" + ident(get(p, "name")));
    }
    ps.push_back(defaults ? "bool cache = true" : "bool cache");
    ps.push_back(defaults ? "bool reverse = false" : "bool reverse");
    return join(ps);
  }
  void constructor(std::ostream &s) {
    for (auto ma : arr(model, "modules")) {
      auto m = cast<Dict>(ma);
      auto n = ident(get(m, "name"));
      for (auto ra : arr(m, "rules")) {
        auto rn = ident(get(cast<Dict>(ra), "name"));
        s << "bool Module_" << n << "::arbitrate_" << rn
          << "() { return model.sim.arbitrateRule(rid_" << rn << "); }\n";
      }
    }
    std::vector<string> inits;
    for (auto pa : arr(model, "config")) {
      auto p = cast<Dict>(pa);
      auto n = ident(get(p, "name"));
      inits.push_back(n + "(arg_" + n +
                      (get(p, "type").starts_with("vector<")
                           ? ".begin(), arg_" + n + ".end()"
                           : "") +
                      ")");
    }
    inits.push_back("sim(cache)");
    s << top << "::" << top << "(" << constructorSignature(false)
      << ") : " << join(inits) << " {\n";
    std::vector<string> setup;
    for (auto ma : arr(model, "modules")) {
      auto n = ident(get(cast<Dict>(ma), "name"));
      setup.push_back(n + ".mid = sim.addModule<&Module_" + n + "::Work>(" + n +
                      ");");
    }
    s << "  if (reverse) {\n";
    for (auto i = setup.rbegin(); i != setup.rend(); ++i)
      s << "    " << *i << '\n';
    s << "  } else {\n";
    for (auto &x : setup)
      s << "    " << x << '\n';
    s << "  }\n";
    for (auto ma : arr(model, "modules")) {
      auto m = cast<Dict>(ma);
      auto n = ident(get(m, "name"));
      for (auto ra : arr(m, "rules")) {
        auto rn = ident(get(cast<Dict>(ra), "name"));
        s << "  " << n << ".rid_" << rn << " = sim.addRule(" << n
          << ".mid, [](void* p, auto&, auto) { return static_cast<Module_" << n
          << "*>(p)->arbitrate_" << rn << "(); });\n";
      }
    }
    for (auto op : module.getOps<acir::ResourceOp>()) {
      auto r = op->getAttrDictionary();
      s << "  " << each(get(r, "name"), [&](string q) {
        return "sim." +
               string(get(r, "kind") == "signal" ? "addSignal" : "addQueue") +
               "(" + q + ");";
      }) << '\n';
    }
    for (auto ma : arr(model, "modules")) {
      auto m = cast<Dict>(ma);
      auto n = ident(get(m, "name"));
      auto accesses = analysis.accesses[get(m, "work")];
      for (auto ra : arr(m, "rules")) {
        auto r = cast<Dict>(ra);
        auto f = get(r, "function");
        auto &set = analysis.accesses[f];
        accesses.insert(set.begin(), set.end());
      }
      for (auto &q : accesses)
        s << "  " << each(q, [&](string q) {
          return "sim.declareResource(" + n + ".mid, " + q + ");";
        }) << '\n';
      for (auto ra : arr(m, "rules")) {
        auto r = cast<Dict>(ra);
        auto f = get(r, "function"), rid = n + ".rid_" + ident(get(r, "name"));
        for (auto &q : analysis.signals[f])
          s << "  sim.declareInput(" << rid << ", " << ident(q) << ");\n";
        for (auto &[q, effects] : analysis.effects[f]) {
          std::vector<string> es;
          for (auto &e : effects)
            es.push_back("gfsim::" + e);
          s << "  " << each(q, [&](string q) {
            return "sim.bind(" + rid + ", " + q + ", " + join(es, " | ") + ");";
          }) << '\n';
        }
      }
    }
    for (auto &[n, r] : resources)
      if (get(r, "kind") == "signal")
        for (auto &q : analysis.accesses[get(r, "evaluate_fn")])
          s << "  " << each(q, [&](string q) {
            return "sim.declareInput(" + ident(n) + ", " + q + ");";
          }) << '\n';
    s << "  sim.freeze();\n}\n";
  }
};
struct Lowering {
  Layout &layout;
  OpBuilder b;
  OwningOpRef<ModuleOp> output;
  DenseMap<Value, Value> values;
  DenseMap<Block *, Block *> blocks;
  DenseMap<Value, string> valueTypes;
  func::FuncOp current;
  Dict meta;
  string role, rid, owner;
  Value pops;
  explicit Lowering(Layout &l)
      : layout(l), b(l.module.getContext()),
        output(ModuleOp::create(l.module.getLoc())) {}
  Type type(string t) {
    return t == "bool" ? Type(b.getI1Type())
                       : Type(emitc::OpaqueType::get(b.getContext(), cpp(t)));
  }
  Operation *make(StringRef name, ValueRange args = {}, TypeRange results = {},
                  ArrayRef<NamedAttribute> attrs = {}) {
    OperationState st(current ? current.getLoc() : layout.module.getLoc(),
                      name);
    st.addOperands(args);
    st.addTypes(results);
    st.addAttributes(attrs);
    return b.create(st);
  }
  Value literal(string t, string text) {
    return call(t, "[&]() { return " + text + "; }");
  }
  Value call(string t, string callee, ValueRange args = {},
             std::vector<string> suffix = {}) {
    SmallVector<NamedAttribute> attrs{
        b.getNamedAttr("callee", b.getStringAttr(callee))};
    if (!suffix.empty()) {
      SmallVector<Attribute> order;
      for (unsigned i = 0; i < args.size(); ++i)
        order.push_back(b.getIndexAttr(i));
      for (auto &s : suffix)
        order.push_back(emitc::OpaqueAttr::get(b.getContext(), s));
      attrs.push_back(b.getNamedAttr("args", b.getArrayAttr(order)));
    }
    SmallVector<Type> result;
    if (t != "void")
      result.push_back(type(t));
    auto *op = make("emitc.call_opaque", args, result, attrs);
    return result.empty() ? Value{} : op->getResult(0);
  }
  void verbatim(string text, ValueRange args = {}) {
    make("emitc.verbatim", args, {},
         {b.getNamedAttr("value", b.getStringAttr(text))});
  }
  SmallVector<Value> mapped(ValueRange args) {
    SmallVector<Value> v;
    for (auto a : args)
      v.push_back(values.lookup(a));
    return v;
  }
  SmallVector<Value> adapt(ValueRange args, TypeRange types) {
    SmallVector<Value> out;
    for (auto [value, target] : llvm::zip(args, types)) {
      if (value.getType() != target) {
        string ct = target.isInteger(1)
                        ? "bool"
                        : cast<emitc::OpaqueType>(target).getValue().str();
        value = call(ct, "ac_detail::cast<" + ct + ">", value);
      }
      out.push_back(value);
    }
    return out;
  }
  string path(Operation &op, string object, unsigned indexBase = 0) {
    if (auto a = op.getAttrOfType<ArrayAttr>("path"))
      for (auto p : a) {
        if (auto s = dyn_cast<StringAttr>(p))
          object += "." + ident(s.getValue());
        else
          object += ".at(i" + std::to_string(indexBase++) + ")";
      }
    return object;
  }
  void lower(func::FuncOp f) {
    values.clear();
    blocks.clear();
    valueTypes.clear();
    meta = f->getAttrOfType<Dict>("acir.function");
    role = get(meta, "kind");
    rid = "rid_" + ident(get(meta, "rule"));
    owner = role == "signal" ? "" : "model.";
    SmallVector<Type> params, results;
    auto ps = arr(meta, "params");
    for (auto p : ps)
      params.push_back(type(get(cast<Dict>(p), "type")));
    auto result = get(meta, "result");
    if (result != "void")
      results.push_back(type(result));
    current = func::FuncOp::create(f.getLoc(), layout.functionName(meta),
                                   b.getFunctionType(params, results));
    output->getBody()->push_back(current);
    unsigned bi = 0;
    for (auto &old : f.getBody()) {
      auto *block = new Block;
      current.getBody().push_back(block);
      blocks[&old] = block;
      unsigned ai = 0;
      for (auto a : old.getArguments()) {
        string t = valueSourceType(a);
        values[a] = block->addArgument(type(t), a.getLoc());
        valueTypes[a] = t;
        ++ai;
      }
      ++bi;
    }
    for (auto &old : f.getBody()) {
      b.setInsertionPointToEnd(blocks[&old]);
      for (auto &op : old) {
        SmallVector<Value> args = mapped(op.getOperands());
        auto name = op.getName().getStringRef();
        string t = get(&op, "acir.source_type");
        if (t.empty() && op.getNumResults())
          t = sourceType(op.getResult(0).getType());
        Value v;
        auto binary = [&](string expression) {
          return call(t, "[](auto a, auto b) { return " + expression + "; }",
                      args);
        };
        if (auto br = dyn_cast<cf::BranchOp>(op))
          cf::BranchOp::create(
              b, op.getLoc(), blocks[br.getDest()],
              adapt(args, blocks[br.getDest()]->getArgumentTypes()));
        else if (auto br = dyn_cast<cf::CondBranchOp>(op))
          cf::CondBranchOp::create(
              b, op.getLoc(), args[0], blocks[br.getTrueDest()],
              adapt(mapped(br.getTrueDestOperands()),
                    blocks[br.getTrueDest()]->getArgumentTypes()),
              blocks[br.getFalseDest()],
              adapt(mapped(br.getFalseDestOperands()),
                    blocks[br.getFalseDest()]->getArgumentTypes()));
        else if (isa<func::ReturnOp>(op)) {
          func::ReturnOp::create(b, op.getLoc(),
                                 adapt(args, current.getResultTypes()));
        } else if (auto c = dyn_cast<arith::ConstantOp>(op)) {
          auto value = cast<IntegerAttr>(c.getValue()).getValue();
          v = literal(t, "ac_detail::cast<" + cpp(t) + ">(" +
                             std::to_string(value.getZExtValue()) + "ULL)");
        } else if (isa<arith::CmpIOp>(op)) {
          static const char *comparisons[] = {"==", "!=", "<",  "<=", ">",
                                              ">=", "<",  "<=", ">",  ">="};
          auto predicate =
              static_cast<unsigned>(cast<arith::CmpIOp>(op).getPredicate());
          auto width = cast<IntegerType>(op.getOperand(0).getType()).getWidth();
          string ct =
              width == 1
                  ? "bool"
                  : "std::" +
                        string(predicate >= 2 && predicate <= 5 ? "" : "u") +
                        "int" + std::to_string(width) + "_t";
          v = binary("ac_detail::cast<" + ct + ">(a) " +
                     comparisons[predicate] + " ac_detail::cast<" + ct +
                     ">(b)");
        } else if (isa<arith::SelectOp>(op)) {
          v = call(t,
                   "[](auto condition, auto yes, auto no) { return "
                   "ac_detail::cast<" +
                       cpp(t) + ">(condition ? yes : no); }",
                   args);
        } else if (isa<arith::ExtUIOp, arith::ExtSIOp, arith::TruncIOp>(op)) {
          auto width = cast<IntegerType>(op.getOperand(0).getType()).getWidth();
          bool sign = isa<arith::ExtSIOp>(op);
          string operand =
              width == 1 ? (sign ? "(a ? -1 : 0)" : "a")
                         : "ac_detail::cast<std::" + string(sign ? "" : "u") +
                               "int" + std::to_string(width) + "_t>(a)";
          v = call(t,
                   "[](auto a) { return ac_detail::cast<" + cpp(t) + ">(" +
                       operand + "); }",
                   args);
        } else if (name.starts_with("arith.")) {
          static const std::map<string, string> helpers = {
              {"arith.addi", "add"},       {"arith.subi", "sub"},
              {"arith.muli", "mul"},       {"arith.shli", "shl"},
              {"arith.shrsi", "shr"},      {"arith.shrui", "shr"},
              {"arith.floordivsi", "div"}, {"arith.divui", "div"},
              {"arith.remsi", "mod"},      {"arith.remui", "mod"}};
          auto it = helpers.find(name.str());
          if (it != helpers.end())
            v = call(t, "ac_detail::" + it->second + "<" + cpp(t) + ">", args);
          else if (isa<arith::AndIOp, arith::OrIOp, arith::XOrIOp>(op)) {
            string symbol = name == "arith.andi"  ? "&"
                            : name == "arith.ori" ? "|"
                                                  : "^";
            v = binary("ac_detail::cast<" + cpp(t) + ">(a " + symbol + " b)");
          } else
            throw std::runtime_error("unsupported arithmetic operation: " +
                                     name.str());
        } else if (auto c = dyn_cast<func::CallOp>(op)) {
          auto decl = layout.module.lookupSymbol<func::FuncOp>(c.getCallee());
          auto action = get(decl, "acir.runtime");
          if (action == "start")
            verbatim("ac_detail::Pops pops;");
          else if (action == "begin") {
            if (args.empty())
              v = call("bool", "model.sim.beginRule", {}, {rid});
            else {
              std::vector<string> fields, parameters;
              for (unsigned i = 0; i < args.size(); ++i) {
                fields.push_back("a" + std::to_string(i));
                parameters.push_back("auto " + fields.back());
              }
              auto rn = ident(get(meta, "rule"));
              v = call("bool",
                       "[&](" + join(parameters) +
                           ") { return model.sim.beginRule(" + rid +
                           ", cache_" + rn + ", Args_" + rn + "{" +
                           join(fields) + "}); }",
                       args);
            }
          } else if (action == "abort" || action == "complete")
            call("void", "model.sim." + action + "Rule", {}, {rid});
          else if (!action.empty())
            v = call(t, "ac_detail::" + action, args);
          else {
            auto fn = layout.functions.at(c.getCallee().str());
            auto intrinsic = get(fn, "intrinsic");
            v = call(t,
                     intrinsic.empty()
                         ? ident(c.getCallee())
                         : "ac_detail::" + intrinsic + "<" + cpp(t) + ">",
                     args);
          }
        } else if (isa<acir::GetOp>(op)) {
          auto n = ident(get(&op, "name"));
          v = literal(t, t.starts_with("qarray<")
                             ? cpp(t) + "(" + owner + n + ".refs)"
                             : "&" + owner + n);
        } else if (isa<acir::ConfigOp>(op))
          v = literal(t, owner + ident(get(&op, "name")));
        else if (isa<acir::ReadOp>(op)) {
          auto input = valueTypes.lookup(op.getOperand(0));
          if (input.starts_with("signal<"))
            v = call(t, "ac_detail::signalValue", args);
          else
            v = call(t, "ac_detail::peek", args);
        } else if (isa<acir::QueryOp>(op))
          v = call(t, "ac_detail::" + get(&op, "kind"), args);
        else if (isa<acir::PopOp>(op)) {
          call("void", "pops.add", args, {rid});
        } else if (isa<acir::PushOp>(op))
          call("void", "ac_detail::push", args, {rid});
        else if (isa<acir::ReviseOp>(op)) {
          unsigned n = args.size() - 2;
          std::vector<string> indices;
          for (unsigned i = 0; i < n; ++i)
            indices.push_back("auto i" + std::to_string(i));
          string accessor = "[](auto& target" +
                            (n ? ", " + join(indices) : "") +
                            ") -> auto& { return " + path(op, "target") + "; }";
          call("void",
               "[&](auto q, auto value, auto... indices) { "
               "ac_detail::revise(q, value, " +
                   rid + ", " + accessor + ", indices...); }",
               args);
        } else if (isa<acir::InvokeOp>(op))
          call("void", "work_" + ident(get(&op, "name")), args);
        else if (isa<acir::EventOp>(op))
          call("void",
               "[&](auto delay) { model.sim.requestWakeup(" + rid +
                   ", mid, delay); }",
               args);
        else if (isa<acir::AggregateOp>(op))
          v = call(t, "ac_detail::make<" + cpp(t) + ">", args);
        else if (isa<acir::ExtractOp>(op))
          v = call(t,
                   args.size() == 2 ? "ac_detail::index"
                                    : "[](auto value) { return value." +
                                          ident(get(&op, "field")) + "; }",
                   args);
        else if (isa<acir::LengthOp>(op))
          v = call(t, "ac_detail::length", args);
        else if (isa<acir::RangeOp>(op))
          v = call(t, "ac_detail::range", args);
        else if (isa<acir::CastOp>(op))
          v = call(t, "ac_detail::cast<" + cpp(t) + ">", args);
        else if (isa<acir::UnaryOp>(op)) {
          auto optr = get(&op, "operator_name");
          v = optr == "-"
                  ? call(t, "ac_detail::neg<" + cpp(t) + ">", args)
                  : call(t,
                         "[](auto a) { return ac_detail::cast<" + cpp(t) +
                             ">(" + (optr == "not" ? "!" : optr) + "a); }",
                         args);
        } else if (isa<acir::CompareOp>(op))
          v = binary("a " + get(&op, "operator_name") + " b");
        else if (isa<acir::UpdateOp>(op)) {
          if (op.hasAttr("indexed"))
            v = call(t,
                     "[](auto value, auto index, auto item) { value.at(index) "
                     "= item; return value; }",
                     args);
          else {
            std::vector<string> indices;
            for (unsigned i = 0; i + 2 < args.size(); ++i)
              indices.push_back("auto i" + std::to_string(i));
            v = call(t,
                     "[](auto value, auto item" +
                         (indices.empty() ? "" : ", " + join(indices)) +
                         ") { " + path(op, "value") +
                         " = item; return value; }",
                     args);
          }
        } else if (isa<acir::CheckOp>(op))
          call("void", "ac_detail::check", args);
        else if (isa<acir::UnreachableOp>(op))
          call("void", "ac_detail::unreachable");
        else
          throw std::runtime_error("unhandled operation: " + name.str());
        if (op.getNumResults()) {
          values[op.getResult(0)] = v;
          valueTypes[op.getResult(0)] = t;
        }
      }
    }
  }
  LogicalResult run() {
    for (auto f : layout.module.getOps<func::FuncOp>())
      if (!f.isExternal())
        lower(f);
    return verify(*output);
  }
};
struct ConvertToEmitCPass : acir::impl::ConvertToEmitCBase<ConvertToEmitCPass> {
  Layout *layout = nullptr;
  ConvertToEmitCPass() = default;
  explicit ConvertToEmitCPass(Layout &l) : layout(&l) {}
  void runOnOperation() override {
    std::optional<Layout> local;
    auto *activeLayout = layout;
    if (!activeLayout) {
      local.emplace(getOperation());
      activeLayout = &*local;
    }
    Lowering lower(*activeLayout);
    if (failed(lower.run())) {
      signalPassFailure();
      return;
    }
    getOperation().getBodyRegion().takeBody(lower.output->getBodyRegion());
    getOperation()->setAttrs(ArrayRef<NamedAttribute>{});
  }
};
std::unique_ptr<Pass> acir::createAnalyzeResourcesPass() {
  return std::make_unique<AnalyzeResourcesPass>();
}
std::unique_ptr<Pass> acir::createLowerGFSimPass() {
  return std::make_unique<LowerGFSimPass>();
}
std::unique_ptr<Pass> acir::createConvertToEmitCPass() {
  return std::make_unique<ConvertToEmitCPass>();
}
void acir::registerPasses() {
  registerPass([] { return createAnalyzeResourcesPass(); });
  registerPass([] { return createLowerGFSimPass(); });
  registerPass([] { return createConvertToEmitCPass(); });
}
int acir::compileMain(int argc, char **argv) {
  llvm::InitLLVM init(argc, argv);
  try {
    if (argc < 3) {
      llvm::errs() << "usage: acir-compile INPUT.mlir OUTPUT_DIR [--no-opt]\n";
      return 2;
    }
    DialectRegistry registry;
    registry
        .insert<acir::ACIRDialect, arith::ArithDialect, cf::ControlFlowDialect,
                emitc::EmitCDialect, func::FuncDialect>();
    MLIRContext context(registry);
    context.loadAllAvailableDialects();
    auto module = parseSourceFile<ModuleOp>(argv[1], &context);
    if (!module || failed(verify(*module)))
      return 1;
    if (!(*module)->hasAttr("acir.model"))
      throw std::runtime_error("missing acir.model metadata");
    acir::registerPasses();
    PassManager analysis(&context);
    analysis.addPass(acir::createAnalyzeResourcesPass());
    if (failed(analysis.run(*module)))
      return 1;
    Layout layout(*module);
    std::filesystem::create_directories(argv[2]);
    std::filesystem::path dir(argv[2]);
    std::ofstream header(dir / "model.hpp");
    layout.header(header);
    std::ostringstream registration;
    layout.constructor(registration);
    if (argc < 4 || string(argv[3]) != "--no-opt") {
      PassManager pm(&context);
      pm.addPass(createCanonicalizerPass());
      pm.addPass(createCSEPass());
      if (failed(pm.run(*module)))
        return 1;
    }
    PassManager lowering(&context);
    lowering.addPass(acir::createLowerGFSimPass());
    lowering.addPass(std::make_unique<ConvertToEmitCPass>(layout));
    if (failed(lowering.run(*module)))
      return 1;
    std::string code;
    llvm::raw_string_ostream stream(code);
    if (failed(emitc::translateToCpp(*module, stream, true)))
      return 1;
    std::ofstream source(dir / "model.cpp");
    source << "// Generated via MLIR EmitC.\n#include \"model.hpp\"\nnamespace "
              "ac_generated {\n"
           << code << '\n';
    source << registration.str();
    source << "}\n";
    std::error_code ec;
    llvm::raw_fd_ostream ir((dir / "model.emitc.mlir").string(), ec);
    if (ec)
      throw std::runtime_error(ec.message());
    module->print(ir);
    return 0;
  } catch (const std::exception &e) {
    llvm::errs() << "acir-compile: " << e.what() << "\n";
    return 1;
  }
}

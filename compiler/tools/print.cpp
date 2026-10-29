// Human ACIR views deliberately do not form a second serialization format.
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"

using namespace mlir;
using namespace acir::ac;

namespace {
llvm::cl::opt<std::string> input(llvm::cl::Positional, llvm::cl::Required,
                                 llvm::cl::desc("ACIR input"));
llvm::cl::opt<std::string> profile("profile", llvm::cl::init("readable"));
llvm::cl::opt<std::string> configPath("config", llvm::cl::Required);
llvm::cl::opt<std::string> output("o", llvm::cl::init("-"));

// All names, recursive attributes/types and regions share this printer. No
// attributes or SSA values in the parsed module are rewritten for display.
class DisplayPrinter {
public:
  DisplayPrinter(llvm::raw_ostream &os, const llvm::json::Object &config)
      : os(os), config(config) {
    auto *globals = config.getObject("globals");
    locations = globals->getBoolean("locations").value_or(false);
    sourceInfo = globals->getBoolean("source_info").value_or(false);
    emptyAttributes = globals->getBoolean("empty_attributes").value_or(false);
    compactTypes = globals->getBoolean("compact_types").value_or(true);
    if (auto *fields = globals->getArray("metadata_fields"))
      for (const auto &field : *fields)
        if (auto name = field.getAsString())
          metadata.insert(*name);
  }

  void print(Operation *op) {
    os << "// ACIR reading view (阅读视图); not compiler input.\n";
    operation(op, 0);
  }

private:
  llvm::raw_ostream &os;
  const llvm::json::Object &config;
  bool locations, sourceInfo, emptyAttributes, compactTypes;
  llvm::StringSet<> metadata;
  llvm::DenseMap<Value, unsigned> values;
  llvm::DenseMap<Block *, unsigned> blocks;

  void value(Value v) {
    auto entry = values.try_emplace(v, values.size());
    os << '%' << entry.first->second;
  }
  void blockName(Block *block) {
    auto entry = blocks.try_emplace(block, blocks.size());
    os << "^bb" << entry.first->second;
  }
  bool visible(StringRef key) const {
    return sourceInfo || !metadata.contains(key);
  }
  static bool empty(Attribute attr) {
    if (auto dict = dyn_cast<DictionaryAttr>(attr))
      return dict.empty();
    if (auto array = dyn_cast<ArrayAttr>(attr))
      return array.empty();
    if (auto str = dyn_cast<StringAttr>(attr))
      return str.getValue().empty();
    return false;
  }
  template <typename Range, typename Fn> void list(Range range, Fn fn) {
    llvm::interleaveComma(range, os, fn);
  }

  void type(Type t) {
    if (auto bits = dyn_cast<BitsType>(t)) {
      os << "!ac.bits<";
      if (compactTypes && !sourceInfo)
        expression(bits.getWidth());
      else
        attribute(bits.getWidth());
      os << '>';
    } else if (auto state = dyn_cast<StateType>(t)) {
      os << "!ac.state<";
      type(state.getPayload());
      os << '>';
    } else if (auto queue = dyn_cast<QueueType>(t)) {
      os << "!ac.queue<";
      type(queue.getPayload());
      os << '>';
    } else if (auto primitive = dyn_cast<PrimitiveType>(t)) {
      os << "!ac.primitive<";
      type(primitive.getPayload());
      os << '>';
    } else if (auto table = dyn_cast<TableType>(t)) {
      os << "!ac.table<";
      attribute(table.getShape());
      os << ", ";
      type(table.getElementType());
      os << '>';
    } else if (auto function = dyn_cast<FunctionType>(t)) {
      os << '(';
      list(function.getInputs(), [&](Type input) { type(input); });
      os << ") -> (";
      list(function.getResults(), [&](Type result) { type(result); });
      os << ')';
    } else if (auto tuple = dyn_cast<TupleType>(t)) {
      os << "tuple<";
      list(tuple.getTypes(), [&](Type element) { type(element); });
      os << '>';
    } else {
      os << t;
    }
  }

  // The tree is displayed, never evaluated. An unrecognised expression keeps
  // its complete semantic dictionary rather than guessing its spelling.
  void expression(StaticExprAttr expr) {
    auto tree = expr.getTree();
    auto kind = tree.getAs<StringAttr>("kind");
    if (kind && kind.getValue() == "literal") {
      auto literal = tree.getAs<DictionaryAttr>("value");
      if (literal) {
        if (auto integer = literal.getAs<MathIntAttr>("value")) {
          os << integer.getCanonicalValue();
          return;
        }
      }
    }
    if (kind && kind.getValue() == "reference") {
      auto ref = tree.getAs<DictionaryAttr>("ref");
      if (ref) {
        auto owner = ref.getAs<FlatSymbolRefAttr>("owner");
        auto name = ref.getAs<StringAttr>("name");
        if (owner && name) {
          os << owner << "::" << name.getValue();
          return;
        }
        if (auto symbol = ref.getAs<FlatSymbolRefAttr>("symbol")) {
          os << symbol;
          return;
        }
      }
    }
    if (kind && (kind.getValue() == "binary" || kind.getValue() == "unary")) {
      auto op = tree.getAs<StringAttr>("operator");
      auto rhs = tree.getAs<StaticExprAttr>(
          kind.getValue() == "unary" ? "operand" : "rhs");
      auto lhs = tree.getAs<StaticExprAttr>("lhs");
      if (op && rhs && (kind.getValue() == "unary" || lhs)) {
        os << '(';
        if (lhs) {
          expression(lhs);
          os << ' ';
        }
        os << op.getValue() << ' ';
        expression(rhs);
        os << ')';
        return;
      }
    }
    attribute(tree);
  }

  void attribute(Attribute attr) {
    if (auto dict = dyn_cast<DictionaryAttr>(attr)) {
      os << '{';
      bool first = true;
      for (auto field : dict) {
        if (!visible(field.getName().getValue()))
          continue;
        if (!first)
          os << ", ";
        first = false;
        os << field.getName().getValue() << " = ";
        attribute(field.getValue());
      }
      os << '}';
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      os << '[';
      list(array, [&](Attribute a) { attribute(a); });
      os << ']';
    } else if (auto t = dyn_cast<TypeAttr>(attr)) {
      type(t.getValue());
    } else if (auto expr = dyn_cast<StaticExprAttr>(attr)) {
      os << "#ac.static_expr<";
      attribute(expr.getTree());
      os << '>';
    } else if (auto domain = dyn_cast<SourceDomainAttr>(attr)) {
      os << "#ac.source_domain<";
      attribute(domain.getValue());
      os << '>';
    } else if (auto expr = dyn_cast<SourceTypeExprAttr>(attr)) {
      os << "#ac.source_type_expr<";
      attribute(expr.getValue());
      os << '>';
    } else {
      os << attr;
    }
  }

  static bool contains(const llvm::json::Array *array, StringRef key) {
    return array && llvm::any_of(*array, [&](const auto &item) {
             return item.getAsString() == key;
           });
  }

  static bool headerField(StringRef op, StringRef key) {
    if (op == "ac.module" || op == "ac.module.import" || op == "ac.rule" ||
        op == "ac.method")
      return key == "sym_name" || key == "function_type";
    if (op == "ac.instance")
      return key == "instance_name" || key == "callee";
    if (op == "ac.rule.call" || op == "ac.method.call" ||
        op == "ac.method.pending")
      return key == "callee";
    if (op == "ac.resource.alloc")
      return key == "instance_name";
    if (op == "ac.instance.ref")
      return key == "instance_path";
    return false;
  }

  void attributes(Operation *op) {
    const llvm::json::Object *rule = nullptr;
    if (auto *ops = config.getObject("ops"))
      rule = ops->getObject(op->getName().getStringRef());
    auto *fields = rule ? rule->getArray("fields") : nullptr;
    auto *omit = rule ? rule->getArray("omit_empty") : nullptr;
    llvm::StringSet<> printed;
    bool first = true;
    auto emit = [&](NamedAttribute field) {
      StringRef name = field.getName().getValue();
      if (!visible(name) || printed.contains(name))
        return;
      printed.insert(name);
      if (empty(field.getValue()) && (!emptyAttributes || contains(omit, name)))
        return;
      if (headerField(op->getName().getStringRef(), name)) {
        if (!first) {
          os << '}';
          first = true;
        }
        os << ' ';
        if (name == "sym_name" && isa<StringAttr>(field.getValue()))
          os << FlatSymbolRefAttr::get(cast<StringAttr>(field.getValue()));
        else
          attribute(field.getValue());
        return;
      }
      if (first)
        os << " {";
      else
        os << ", ";
      first = false;
      // Symbol definitions and callable signatures use dedicated typography.
      if (name == "sym_name" && isa<StringAttr>(field.getValue())) {
        os << "symbol = ";
        os << FlatSymbolRefAttr::get(cast<StringAttr>(field.getValue()));
      } else {
        os << name << " = ";
        attribute(field.getValue());
      }
    };
    if (fields) {
      for (const auto &item : *fields) {
        auto name = item.getAsString();
        if (!name)
          continue;
        if (*name == "*") {
          for (auto field : op->getAttrs())
            emit(field);
        } else if (auto attr = op->getAttr(*name)) {
          emit(NamedAttribute(StringAttr::get(op->getContext(), *name), attr));
        }
      }
    } else {
      for (auto field : op->getAttrs())
        emit(field);
    }
    if (!first)
      os << '}';
  }

  void operation(Operation *op, unsigned indent) {
    os.indent(indent);
    if (op->getNumResults()) {
      list(op->getResults(), [&](Value v) { value(v); });
      os << " = ";
    }
    StringRef name = op->getName().getStringRef();
    os << name;
    bool dedicated = name == "ac.module" || name == "ac.module.import" ||
                     name == "ac.rule" || name == "ac.method" ||
                     name == "ac.instance" || name == "ac.instance.ref" ||
                     name == "ac.rule.call" || name == "ac.method.call" ||
                     name == "ac.method.pending" || name == "ac.resource.alloc";
    if (dedicated)
      attributes(op);
    if (name == "ac.rule" || name == "ac.method")
      os << (op->getNumOperands() ? " captures" : " captures()");
    else if (name == "scf.if")
      os << " condition";
    else if (name == "ac.instance")
      os << " inputs";
    else if (name == "ac.method.call" || name == "ac.method.pending")
      os << " receiver, arguments";
    else if (name == "ac.rule.call")
      os << " interfaces";
    else if (name == "ac.resource.alloc")
      os << " initial";
    else if (name == "ac.state" || name == "ac.queue")
      os << " resource";
    if (op->getNumOperands() || op->getNumResults()) {
      os << '(';
      list(op->getOperands(), [&](Value v) { value(v); });
      os << ')';
      // Operand types matter for uses of unresolved resources as well as
      // results.
      os << " : (";
      list(op->getOperandTypes(), [&](Type t) { type(t); });
      os << ") -> (";
      list(op->getResultTypes(), [&](Type t) { type(t); });
      os << ')';
    }
    if (!dedicated)
      attributes(op);
    // Unregistered operations may carry opaque properties outside getAttrs().
    // Keep them in the generic view just as the full MLIR printer does.
    if (!op->getName().isRegistered()) {
      if (auto properties = op->getPropertiesAsAttribute()) {
        os << " properties = ";
        attribute(properties);
      }
    }
    if (op->getNumSuccessors()) {
      os << " successors(";
      list(op->getSuccessors(), [&](Block *b) { blockName(b); });
      os << ')';
    }
    if (locations) {
      os << " at ";
      op->getLoc().print(os);
    }
    if (!op->getNumRegions()) {
      os << '\n';
      return;
    }
    os << " {\n";
    for (auto [index, region] : llvm::enumerate(op->getRegions())) {
      os.indent(indent + 2);
      if ((name == "ac.rule" || name == "ac.method") &&
          op->getNumRegions() == 2)
        os << (index == 0 ? "guard" : "body");
      else if (name == "scf.if")
        os << (index == 0 ? "then" : "else");
      else if (name == "ac.module")
        os << "body";
      else
        os << "region " << index;
      os << " {\n";
      for (auto &block : region) {
        os.indent(indent + 4);
        blockName(&block);
        os << '(';
        list(block.getArguments(), [&](BlockArgument arg) {
          value(arg);
          os << ": ";
          type(arg.getType());
          if (locations) {
            os << " at ";
            arg.getLoc().print(os);
          }
        });
        os << "):\n";
        for (auto &nested : block)
          operation(&nested, indent + 6);
      }
      os.indent(indent + 2) << "}\n";
    }
    os.indent(indent) << "}\n";
  }
};
} // namespace

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "ACIR display (no lowering)\n");
  if (profile != "readable" && profile != "debug" && profile != "full") {
    llvm::errs() << "unknown display profile\n";
    return 1;
  }
  auto buffer = llvm::MemoryBuffer::getFile(configPath);
  if (!buffer) {
    llvm::errs() << buffer.getError().message() << '\n';
    return 1;
  }
  auto config = llvm::json::parse((*buffer)->getBuffer());
  if (!config) {
    llvm::errs() << llvm::toString(config.takeError()) << '\n';
    return 1;
  }
  auto *object = config->getAsObject();
  if (!object || !object->getObject("globals") || !object->getObject("ops")) {
    llvm::errs() << "display config requires globals and ops objects\n";
    return 1;
  }
  DialectRegistry dialects;
  dialects.insert<ACIRDialect, arith::ArithDialect, func::FuncDialect,
                  scf::SCFDialect>();
  MLIRContext context(dialects);
  context.allowUnregisteredDialects();
  // Unlinked units can contain constraints that only linking resolves. Parsing
  // must not trigger semantic verification, passes or type constraint solving.
  ParserConfig parserConfig(&context, /*verifyAfterParse=*/false);
  auto module = parseSourceFile<mlir::ModuleOp>(input, parserConfig);
  if (!module)
    return 1;
  // Protect the input even for direct native callers (including hard links).
  llvm::sys::fs::UniqueID inputID, outputID;
  if (!llvm::sys::fs::getUniqueID(input, inputID) &&
      !llvm::sys::fs::getUniqueID(output, outputID) && inputID == outputID) {
    llvm::errs() << "display output must differ from input\n";
    return 1;
  }
  std::error_code error;
  llvm::ToolOutputFile file(output, error, llvm::sys::fs::OF_Text);
  if (error) {
    llvm::errs() << error.message() << '\n';
    return 1;
  }
  if (profile == "full") {
    module->print(file.os(), OpPrintingFlags().enableDebugInfo());
    file.os() << '\n';
  } else {
    DisplayPrinter(file.os(), *object).print(*module);
  }
  file.keep();
  return 0;
}

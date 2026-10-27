// Independent canonical FIFO Instance/schema/source-unit dependency tests.
#include "Compiler/HardwareEmitCommon.h"
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <array>
#include <cstddef>
#include <limits>
#include <optional>

#include <algorithm>
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;
using namespace mlir;

class QueueContractsTest : public ::testing::Test {
protected:
  MLIRContext context;
  std::string diagnostics;
  void SetUp() override { context.loadDialect<ac::ACIRDialect>(); }
  DictionaryAttr owner(StringRef path = "__init__.py") {
    Builder b(&context);
    return b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("")),
                                b.getNamedAttr("path", b.getStringAttr(path))});
  }
  DictionaryAttr occurrence(StringRef symbol) {
    Builder b(&context);
    return b.getDictionaryAttr(
        {b.getNamedAttr(
             "site", b.getDictionaryAttr(
                         {b.getNamedAttr("definition", FlatSymbolRefAttr::get(
                                                           &context, symbol)),
                          b.getNamedAttr("ast_path", b.getArrayAttr({}))})),
         b.getNamedAttr("expansion", b.getArrayAttr({}))});
  }
  DictionaryAttr exportSite(StringRef path) {
    Builder b(&context);
    NamedAttrList span;
    span.append("path", b.getStringAttr(path));
    for (StringRef field : {"line", "column", "end_line", "end_column"})
      span.append(field, b.getI64IntegerAttr(field == "end_column" ? 2 : 1));
    return b.getDictionaryAttr(
        {b.getNamedAttr("ast_path", b.getArrayAttr({})),
         b.getNamedAttr("location", span.getDictionary(&context))});
  }
  std::string literal(unsigned value) {
    return "#ac.static_expr<{kind = \"literal\", location = {path = "
           "\"__init__.py\", line = 1 : i64, column = 1 : i64, end_line = 1 : "
           "i64, end_column = 2 : i64}, origin = {site = {definition = "
           "@Provider, "
           "ast_path = []}, expansion = []}, value = {kind = \"integer\", "
           "value "
           "= #ac.math_int<" +
           std::to_string(value) + ">}}>";
  }
  std::string providerText(unsigned width, unsigned depth, StringRef policy) {
    return "#one = " + literal(1) + "\n#zero = " + literal(0) +
           "\n#width = " + literal(width) + "\n#depth = " + literal(depth) +
           R"mlir(
!control = !ac.bits<#one>
!data = !ac.bits<#width>
module {
  "ac.module"() ({
  ^bb0(%valid: !control, %data: !data, %take: !control, %clk: !control, %rst: !control):
    %ready, %available, %head, %head_capacity = "ac.instance"(%clk, %rst, %valid, %data, %take) {
      instance_name = "fifo", callee = @pycircuit.__builtins__.)mlir" +
           (policy == "local_occupancy" ? "fifo_local" : "fifo_downstream_pop") + R"mlir(,
      parameters = [#depth, #one], type_arguments = [!data],
      occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}
    } : (!control, !control, !control, !data, !control) -> (!control, !control, !data, !control)
    "ac.yield"(%ready, %available, %head) : (!control, !control, !data) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"},
      parameters = [], type_parameters = [],
      function_type = (!control, !data, !control, !control, !control) -> (!control, !control, !data),
      input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"],
      output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
}
)mlir";
  }
  OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp original) {
    return OwningOpRef<mlir::ModuleOp>(cast<mlir::ModuleOp>(original->clone()));
  }
  // The existing source-unit envelope is stamped onto real registered IR.
  // This is test provenance, never a second source compiler or semantic route.
  void envelope(mlir::ModuleOp unit, StringRef path) {
    Builder b(&context);
    auto sourceOwner = owner(path);
    unit->setAttr("ac.stage", b.getStringAttr("source"));
    unit->setAttr("ac.unit_kind", b.getStringAttr("implementation"));
    unit->setAttr("ac.source_owner", sourceOwner);
    unit->setAttr("ac.import_bindings", b.getArrayAttr({}));
    SmallVector<Attribute> owners{sourceOwner}, exports;
    std::vector<std::pair<std::string, Attribute>> namedExports;
    for (Operation &op : unit.getBody()->getOperations()) {
      auto name = SymbolTable::getSymbolName(&op);
      if (!name)
        continue;
      if (auto imported = dyn_cast<ac::ModuleImportOp>(op)) {
        imported->setAttr("ac.declaration_role",
                          b.getStringAttr("import_snapshot"));
        auto importedOwner = imported.getSourceOwner();
        bool builtin = importedOwner.getAs<StringAttr>("package").getValue() ==
                           "pycircuit" &&
                       importedOwner.getAs<StringAttr>("path").getValue() ==
                           "__builtins__.py";
        if (!builtin &&
            !llvm::is_contained(owners, Attribute(importedOwner)))
          owners.push_back(imported.getSourceOwner());
        continue;
      }
      op.setAttr("ac.origin", occurrence(name.getValue()));
      op.setAttr("ac.declaration_role", b.getStringAttr("definition"));
      if (isa<ac::StructOp>(op))
        op.setAttr("ac.source_owner", sourceOwner);
      auto local = name.getValue().contains('.')
                       ? name.getValue().rsplit('.').second
                       : name.getValue();
      namedExports.emplace_back(
          local.str(),
          b.getDictionaryAttr(
              {b.getNamedAttr("name", b.getStringAttr(local)),
               b.getNamedAttr(
                   "target", FlatSymbolRefAttr::get(&context, name.getValue())),
               b.getNamedAttr("site", exportSite(path))}));
    }
    std::sort(namedExports.begin(), namedExports.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &item : namedExports)
      exports.push_back(item.second);
    unit->setAttr("ac.interfaces", b.getArrayAttr(owners));
    unit->setAttr("ac.exports", b.getArrayAttr(exports));
  }
  OwningOpRef<mlir::ModuleOp> provider(unsigned width, unsigned depth,
                                       StringRef policy) {
    auto unit = parseSourceString<mlir::ModuleOp>(
        providerText(width, depth, policy), ParserConfig(&context, false));
    if (unit) {
      addQueueBuiltins(*unit);
      envelope(*unit, "__init__.py");
      EXPECT_TRUE(succeeded(mlir::verify(*unit)));
    }
    return unit;
  }
  LogicalResult extract(mlir::ModuleOp unit) {
    PassManager manager(&context);
    manager.addPass(acir::createExtractSourceInterfacePass());
    return manager.run(unit);
  }
  ac::ModuleOp definition(mlir::ModuleOp unit) {
    return cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(unit, "Provider"));
  }
  ac::ModuleImportOp declaration(mlir::ModuleOp unit) {
    return cast<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(unit, "Provider"));
  }
  std::string print(mlir::ModuleOp unit) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    unit.print(stream, OpPrintingFlags().enableDebugInfo());
    return text;
  }
  ac::StaticExprAttr expression(StringRef kind,
                                ArrayRef<NamedAttribute> fields) {
    Builder b(&context);
    NamedAttrList tree;
    tree.append("kind", b.getStringAttr(kind));
    tree.append("origin", occurrence("Provider"));
    tree.append("location",
                exportSite("__init__.py").getAs<DictionaryAttr>("location"));
    for (auto field : fields)
      tree.append(field);
    return ac::StaticExprAttr::get(&context, tree.getDictionary(&context));
  }
  ac::StaticExprAttr integer(unsigned value) {
    return cast<ac::StaticExprAttr>(parseAttribute(literal(value), &context));
  }
  ac::StaticExprAttr boolean(bool value) {
    Builder b(&context);
    return expression(
        "literal",
        {b.getNamedAttr("value",
                        b.getDictionaryAttr(
                            {b.getNamedAttr("kind", b.getStringAttr("bool")),
                             b.getNamedAttr("value", b.getBoolAttr(value))}))});
  }
  ac::StaticExprAttr binary(StringRef op, ac::StaticExprAttr lhs,
                            ac::StaticExprAttr rhs) {
    Builder b(&context);
    return expression("binary",
                      {b.getNamedAttr("operator", b.getStringAttr(op)),
                       b.getNamedAttr("lhs", lhs), b.getNamedAttr("rhs", rhs)});
  }
  ac::StaticExprAttr select(ac::StaticExprAttr condition,
                            ac::StaticExprAttr yes, ac::StaticExprAttr no) {
    Builder b(&context);
    return expression("select",
                      {b.getNamedAttr("condition", condition),
                       b.getNamedAttr("yes", yes), b.getNamedAttr("no", no)});
  }
  ac::StaticExprAttr declaredUnbound(ac::ModuleOp module) {
    Builder b(&context);
    module->setAttr(
        "parameters",
        b.getArrayAttr({b.getDictionaryAttr(
            {b.getNamedAttr("name", b.getStringAttr("D")),
             b.getNamedAttr("type",
                            TypeAttr::get(ac::MathIntType::get(&context)))})}));
    return expression(
        "reference",
        {b.getNamedAttr(
            "ref", b.getDictionaryAttr(
                       {b.getNamedAttr("kind", b.getStringAttr("parameter")),
                        b.getNamedAttr("owner", FlatSymbolRefAttr::get(
                                                    &context, "Provider")),
                        b.getNamedAttr("name", b.getStringAttr("D"))}))});
  }

  static void replace(std::string &text, StringRef from, StringRef to) {
    auto at = text.find(from.str());
    ASSERT_NE(at, std::string::npos);
    text.replace(at, from.size(), to.str());
  }
  ac::StaticExprAttr exactInteger(StringRef value) {
    Builder b(&context);
    return expression(
        "literal",
        {b.getNamedAttr(
            "value", b.getDictionaryAttr(
                         {b.getNamedAttr("kind", b.getStringAttr("integer")),
                          b.getNamedAttr(
                              "value", ac::MathIntAttr::get(
                                           &context, llvm::APSInt(value)))}))});
  }
  void addIntegerFormals(ac::ModuleOp module, ArrayRef<StringRef> names) {
    Builder b(&context);
    SmallVector<Attribute> formals(module.getParameters().begin(),
                                   module.getParameters().end());
    for (StringRef name : names)
      formals.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr(name)),
           b.getNamedAttr("type",
                          TypeAttr::get(ac::MathIntType::get(&context)))}));
    module->setAttr("parameters", b.getArrayAttr(formals));
  }
  ac::StaticExprAttr formal(StringRef ownerName, StringRef name) {
    Builder b(&context);
    return expression(
        "reference",
        {b.getNamedAttr(
            "ref", b.getDictionaryAttr(
                       {b.getNamedAttr("kind", b.getStringAttr("parameter")),
                        b.getNamedAttr("owner", FlatSymbolRefAttr::get(
                                                    &context, ownerName)),
                        b.getNamedAttr("name", b.getStringAttr(name))}))});
  }
  OwningOpRef<mlir::ModuleOp> finalForEmission(mlir::ModuleOp body,
                                               StringRef top = "Provider") {
    auto header = clone(body);
    if (failed(extract(*header)))
      return {};
    auto linked = compiler::linkSourceUnits({{body, *header}}, top, [&] {
      return emitError(UnknownLoc::get(&context));
    });
    return succeeded(linked) ? std::move(*linked)
                             : OwningOpRef<mlir::ModuleOp>{};
  }
  void addQueueFamilyRoot(mlir::ModuleOp body, unsigned lanes) {
    Builder attrs(&context);
    auto leaf = definition(body);
    auto root = cast<ac::ModuleOp>(leaf->clone());
    root->setAttr("sym_name", attrs.getStringAttr("FamilyRoot"));
    auto shape = attrs.getArrayAttr({integer(lanes)});
    SmallVector<Type> inputs, outputs;
    for (BlockArgument argument : root.getBody().front().getArguments()) {
      auto table = ac::TableType::get(&context, shape, argument.getType());
      argument.setType(table);
      inputs.push_back(table);
    }
    for (Type type : leaf.getFunctionType().getResults())
      outputs.push_back(ac::TableType::get(&context, shape, type));
    auto queue = onlyQueue(root);
    OpBuilder b(queue);
    OperationState collection(queue.getLoc(), "ac.collection");
    collection.addOperands(root.getBody().front().getArguments());
    collection.addTypes(outputs);
    collection.addAttribute("instance_name", attrs.getStringAttr("lanes"));
    collection.addAttribute("callee",
                            FlatSymbolRefAttr::get(&context, "Provider"));
    collection.addAttribute("shape", shape);
    collection.addAttribute("parameters", attrs.getArrayAttr({}));
    collection.addAttribute("type_arguments", attrs.getArrayAttr({}));
    collection.addAttribute("occurrence", occurrence("FamilyRoot"));
    auto family = b.create(collection);
    cast<ac::YieldOp>(root.getBody().front().back())
        ->setOperands(family->getResults());
    queue.erase();
    root->setAttr("function_type",
                  TypeAttr::get(FunctionType::get(&context, inputs, outputs)));
    body.getBody()->push_back(root);
    envelope(body, "__init__.py");
  }
  OwningOpRef<mlir::ModuleOp> payloadProvider(StringRef payload, unsigned depth,
                                              StringRef policy) {
    auto text = providerText(130, depth, policy);
    replace(text, "!control =",
            "#tag = " + literal(13) + "\n#lane = " + literal(65) +
                "\n#three = " + literal(3) + "\n!control =");
    replace(text, "!data = !ac.bits<#width>", "!data = " + payload.str());
    replace(text, "module {", R"mlir(module {
      ac.struct "HeaderA" fields [{name = "tag", type = !ac.bits<#tag>}, {name = "flag", type = !control}]
      ac.struct "RecordA" fields [{name = "header", type = !ac.struct<"HeaderA">}, {name = "data", type = !ac.bits<#lane>}]
      ac.struct "HeaderB" fields [{name = "flag", type = !control}, {name = "length", type = !ac.bits<#tag>}]
      ac.struct "RecordB" fields [{name = "metadata", type = !ac.struct<"HeaderB">}, {name = "payload", type = !ac.bits<#lane>}]
    )mlir");
    auto unit = parseSourceString<mlir::ModuleOp>(
        text, ParserConfig(&context, false));
    if (unit) {
      addQueueBuiltins(*unit);
      envelope(*unit, "__init__.py");
      EXPECT_TRUE(succeeded(mlir::verify(*unit)));
    }
    return unit;
  }
  OwningOpRef<mlir::ModuleOp> fixture(StringRef name) {
    auto path = std::string(PYCIRCUIT_TEST_REPO_ROOT) +
                "/tests/compiler/lit/IR/" + name.str();
    auto contents = llvm::MemoryBuffer::getFile(path);
    if (!contents)
      return {};
    auto unit = parseSourceString<mlir::ModuleOp>(
        contents.get()->getBuffer().split("// -----").first, &context);
    if (unit)
      for (auto system :
           llvm::make_early_inc_range(unit->getOps<ac::SystemOp>()))
        system.erase();
    if (unit)
      envelope(*unit, "__init__.py");
    return unit;
  }
  OwningOpRef<mlir::ModuleOp> genericBody() {
    auto body = fixture("queue-contracts.mlir");
    if (!body)
      return {};
    auto records =
        payloadProvider("!ac.struct<\"RecordA\">", 3, "local_occupancy");
    if (!records)
      return {};
    for (auto record : records->getOps<ac::StructOp>())
      body->getBody()->push_front(record->clone());
    for (auto imported :
         llvm::make_early_inc_range(body->getOps<ac::ModuleImportOp>()))
      if (imported.getSymName().starts_with("pycircuit.__builtins__.fifo_"))
        imported.erase();
    addQueueBuiltins(*body);
    envelope(*body, "__init__.py");
    return body;
  }
  void addQueueBuiltins(mlir::ModuleOp unit) {
    auto registry = compiler::SourceHeaderRegistry::create(
        &context, {}, [&] { return emitError(UnknownLoc::get(&context)); });
    ASSERT_TRUE(succeeded(registry));
    OpBuilder insertion = OpBuilder::atBlockBegin(unit.getBody());
    for (StringRef kind : {"fifo_local", "fifo_downstream_pop"}) {
      auto builtin = registry->lookupBuiltin(kind);
      ASSERT_TRUE(builtin);
      if (!SymbolTable::lookupSymbolIn(unit, builtin.symbol))
        insertion.insert(builtin.declaration->clone());
    }
  }
  ac::InstanceOp onlyQueue(ac::ModuleOp module) {
    SmallVector<ac::InstanceOp> queues;
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>())
      if (ac::isQueueInstance(instance))
        queues.push_back(instance);
    EXPECT_TRUE(llvm::hasSingleElement(queues));
    return queues.empty() ? ac::InstanceOp() : queues.front();
  }
  void setQueueDepth(ac::InstanceOp queue, ac::StaticExprAttr depth) {
    Builder b(&context);
    queue->setAttr("parameters",
                   b.getArrayAttr({depth, queue.getParameters()[1]}));
  }
  void setQueueLatency(ac::InstanceOp queue, ac::StaticExprAttr latency) {
    Builder b(&context);
    queue->setAttr("parameters",
                   b.getArrayAttr({queue.getParameters()[0], latency}));
  }
  void setQueuePolicy(ac::InstanceOp queue, StringRef policy) {
    queue->setAttr(
        "callee",
        FlatSymbolRefAttr::get(
            &context,
            policy == "local_occupancy"
                ? "pycircuit.__builtins__.fifo_local"
                : "pycircuit.__builtins__.fifo_downstream_pop"));
  }
  Type queuePayload(ac::InstanceOp queue) {
    return queue.getInputs()[3].getType();
  }
  void updateResultSignature(ac::ModuleOp module) {
    SmallVector<Type> outputTypes;
    auto yield = cast<ac::YieldOp>(module.getBody().front().back());
    for (Value value : yield.getValues())
      outputTypes.push_back(value.getType());
    module->setAttr(
        "function_type",
        TypeAttr::get(FunctionType::get(
            &context, module.getFunctionType().getInputs(), outputTypes)));
  }
  OwningOpRef<mlir::ModuleOp> consumer(mlir::ModuleOp providerHeader) {
    auto unit = OwningOpRef<mlir::ModuleOp>(
        mlir::ModuleOp::create(UnknownLoc::get(&context)));
    unit->getBody()->push_back(declaration(providerHeader)->clone());
    auto signature = declaration(providerHeader).getFunctionType();
    OpBuilder b(&context);
    b.setInsertionPointToEnd(unit->getBody());
    OperationState module(UnknownLoc::get(&context), "ac.module");
    module.addAttribute("sym_name", b.getStringAttr("consumer.Consumer"));
    module.addAttribute("source_owner", owner("consumer.py"));
    module.addAttribute("parameters", b.getArrayAttr({}));
    module.addAttribute("type_parameters", b.getArrayAttr({}));
    module.addAttribute("function_type", TypeAttr::get(signature));
    module.addAttribute("input_names",
                        declaration(providerHeader).getInputNames());
    module.addAttribute("output_names",
                        declaration(providerHeader).getOutputNames());
    module.addRegion();
    auto op = cast<ac::ModuleOp>(b.create(module));
    auto block = new Block();
    op.getBody().push_back(block);
    for (Type type : signature.getInputs())
      block->addArgument(type, UnknownLoc::get(&context));
    b.setInsertionPointToEnd(block);
    OperationState instance(UnknownLoc::get(&context), "ac.instance");
    instance.addOperands(block->getArguments());
    instance.addTypes(signature.getResults());
    instance.addAttribute("instance_name", b.getStringAttr("provider"));
    instance.addAttribute("callee",
                          FlatSymbolRefAttr::get(&context, "Provider"));
    instance.addAttribute("parameters", b.getArrayAttr({}));
    instance.addAttribute("type_arguments", b.getArrayAttr({}));
    instance.addAttribute("occurrence", occurrence("consumer.Consumer"));
    auto child = b.create(instance);
    OperationState yield(UnknownLoc::get(&context), "ac.yield");
    yield.addOperands(child->getResults());
    b.create(yield);
    envelope(*unit, "consumer.py");
    return unit;
  }
  OwningOpRef<mlir::ModuleOp> forgedReady(mlir::ModuleOp original) {
    auto forged = clone(original);
    Builder b(&context);
    auto imported = declaration(*forged);
    auto summary = imported.getDependencySummaryAttr();
    SmallVector<Attribute> rows(summary.begin(), summary.end());
    NamedAttrList ready(cast<DictionaryAttr>(rows[0]));
    ready.set("inputs", b.getArrayAttr({}));
    rows[0] = ready.getDictionary(&context);
    imported->setAttr("dependency_summary", b.getArrayAttr(rows));
    return forged;
  }

  struct Scratch {
    llvm::SmallString<256> path;
    bool retained = false;
    Scratch() {
      const char *base = std::getenv("PYCIRCUIT_QUEUE_TEST_SCRATCH");
      if (base) {
        retained = true;
        llvm::sys::fs::create_directories(base);
        EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory(
            std::string(base) + "/queue-publication", path));
      } else
        EXPECT_FALSE(
            llvm::sys::fs::createUniqueDirectory("queue-publication", path));
      llvm::SmallString<256> real;
      EXPECT_FALSE(llvm::sys::fs::real_path(path, real));
      path = real;
    }
    ~Scratch() {
      if (!retained)
        llvm::sys::fs::remove_directories(path);
    }
    std::string file(StringRef name) const {
      llvm::SmallString<256> result(path);
      llvm::sys::path::append(result, name);
      return result.str().str();
    }
  };
  void write(StringRef path, StringRef contents) {
    std::error_code ec;
    llvm::raw_fd_ostream out(path, ec);
    ASSERT_FALSE(ec);
    out << contents;
  }
  std::string read(StringRef path) {
    auto result = llvm::MemoryBuffer::getFile(path);
    EXPECT_TRUE(bool(result));
    return result ? result.get()->getBuffer().str() : std::string{};
  }
  std::string publishedUnit(Scratch &scratch, StringRef name,
                            mlir::ModuleOp body, mlir::ModuleOp header,
                            StringRef sourcePath) {
    auto dir = scratch.file(name);
    EXPECT_FALSE(llvm::sys::fs::create_directory(dir));
    auto stem = sourcePath.drop_back(3).str();
    write(dir + "/" + stem + ".ac", print(body));
    write(dir + "/" + stem + ".interface.ac", print(header));
    write(dir + "/" + stem + ".d", stem + ".ac: " + sourcePath.str() + "\n");
    write(dir + "/unit.json",
          "{\"kind\":\"pycircuit-source-unit\",\"source\":{\"package\":\"\","
          "\"path\":\"" +
              sourcePath.str() + "\"},\"files\":{\"body\":\"" + stem +
              ".ac\",\"interface\":\"" + stem +
              ".interface.ac\",\"depfile\":\"" + stem + ".d\"}}\n");
    auto control = scratch.file("." + name.str() + ".pycircuit-publication");
    EXPECT_FALSE(llvm::sys::fs::create_directory(control));
    write(control + "/lock", "");
    write(control + "/owner.json",
          "{\"kind\":\"pycircuit-publication-control\",\"destination\":\"" +
              name.str() + "\"}\n");
    return dir;
  }
  int publicLink(Scratch &scratch, ArrayRef<std::string> units,
                 StringRef output, bool replaceOutput, StringRef label) {
    constexpr StringLiteral script = R"py(
import os,sys
repo,linker,*args=sys.argv[1:]
sys.path.insert(0,repo+'/python')
os.environ['PYCIRCUIT_LINKER']=linker
from pycircuit.cli import main
raise SystemExit(main(args))
)py";
    std::vector<std::string> owned{
        PYCIRCUIT_TEST_PYTHON, "-c",  script.str(), PYCIRCUIT_TEST_REPO_ROOT,
        PYCIRCUIT_TEST_LINKER, "link"};
    for (const auto &unit : units)
      owned.push_back(unit);
    owned.insert(owned.end(),
                 {"--top", "consumer.Consumer", "-o", output.str()});
    if (replaceOutput)
      owned.push_back("--replace");
    SmallVector<StringRef> args;
    for (auto &arg : owned)
      args.push_back(arg);
    auto log = scratch.file(label.str() + ".log");
    const std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log,
                                                            log};
    int status = llvm::sys::ExecuteAndWait(PYCIRCUIT_TEST_PYTHON, args,
                                           std::nullopt, redirects, 60);
    llvm::json::Array command;
    for (const auto &arg : owned)
      command.push_back(arg);
    std::string receipt;
    llvm::raw_string_ostream recorded(receipt);
    recorded << llvm::json::Value(
        llvm::json::Object{{"command", std::move(command)},
                           {"exit_status", status},
                           {"log", label.str() + ".log"}});
    recorded.flush();
    write(scratch.file(label.str() + ".command.json"), receipt + "\n");
    return status;
  }
  std::vector<std::pair<std::string, std::string>>
  snapshotUnit(Scratch &scratch, StringRef name, StringRef sourcePath) {
    std::vector<std::pair<std::string, std::string>> files;
    auto stem = sourcePath.drop_back(3).str();
    for (auto file : {stem + ".ac", stem + ".interface.ac", stem + ".d",
                      std::string("unit.json")}) {
      auto path = scratch.file(name.str() + "/" + file);
      files.emplace_back(path, read(path));
    }
    for (auto file : {"lock", "owner.json"}) {
      auto path =
          scratch.file("." + name.str() + ".pycircuit-publication/" + file);
      files.emplace_back(path, read(path));
    }
    return files;
  }

  auto error() {
    return [&] { return emitError(UnknownLoc::get(&context)); };
  }
  template <class Action> void rejects(Action action, StringRef expected) {
    diagnostics.clear();
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
      llvm::raw_string_ostream stream(diagnostics);
      diagnostic.print(stream);
      return success();
    });
    EXPECT_TRUE(failed(action()));
    EXPECT_NE(diagnostics.find(expected.str()), std::string::npos)
        << diagnostics;
  }
};

TEST_F(QueueContractsTest,
       ActualScalarProviderResolvedConfigurationAndReadDependencies) {
  // Independent metadata goldens for the four mandated nonzero depths.
  const unsigned depths[] = {1, 2, 3, 5}, pointers[] = {1, 1, 2, 3},
                 counts[] = {1, 2, 2, 3};
  for (unsigned width : {1u, 13u, 65u, 130u})
    for (unsigned i = 0; i != 4; ++i)
      for (StringRef policy : {"local_occupancy", "downstream_pop"}) {
        auto body = provider(width, depths[i], policy);
        ASSERT_TRUE(body);
        auto module = definition(*body);
        auto queue = onlyQueue(module);
        ac::HardwareAnalysis analysis(*body);
        ac::HardwareBindings bindings;
        bindings.owner = module;
        ASSERT_TRUE(
            succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
        auto config = analysis.resolveQueue(queue, bindings);
        ASSERT_TRUE(succeeded(config));
        EXPECT_EQ(config->packedWidth, width);
        EXPECT_EQ(config->depth, depths[i]);
        EXPECT_EQ(config->pointerWidth, pointers[i]);
        EXPECT_EQ(config->countWidth, counts[i]);
        EXPECT_EQ(config->tokenCardinality, 1u);
        EXPECT_TRUE(config->tokenShape.empty());
        EXPECT_EQ(config->storageBitCount,
                  static_cast<uint64_t>(width) * depths[i]);
        EXPECT_EQ(config->availabilityLatency, 1u);
        EXPECT_EQ(config->timestampWidth, 0u);
        EXPECT_EQ(config->timingStorageBitCount, 0u);
        EXPECT_EQ(config->headReadLatency, 0u);
        EXPECT_FALSE(config->emptyFlow);
        auto actual = analysis.analyzeModule(module, bindings);
        ASSERT_TRUE(succeeded(actual));
        ASSERT_EQ(actual->storageWork.size(), 1u);
        EXPECT_EQ(actual->storageWork[0], queue.getOperation());
        EXPECT_EQ(actual->dependencies.size(), 3u);
        const auto original = print(*body);
        auto header = clone(*body);
        ASSERT_TRUE(succeeded(extract(*header)));
        EXPECT_EQ(print(*body), original);
        auto published =
            analysis.getImportDependencies(declaration(*header), bindings);
        ASSERT_TRUE(succeeded(published));
        ASSERT_EQ(published->size(), 3u);
        ASSERT_EQ((*published)[0].inputs.size(),
                  policy == "local_occupancy" ? 0u : 1u);
        if (policy == "downstream_pop")
          EXPECT_EQ((*published)[0].inputs[0].port, 2u);
        EXPECT_TRUE((*published)[1].inputs.empty());
        EXPECT_TRUE((*published)[2].inputs.empty());
        ASSERT_TRUE(succeeded(
            compiler::admitSourceLinkUnits({{*body, *header}}, error())));
        ASSERT_TRUE(succeeded(compiler::linkSourceUnits(
            {{*body, *header}}, "Provider", error())));
      }
}

TEST_F(QueueContractsTest,
       LocalCapacityIsFourthIndependentCommittedStateEndpoint) {
  auto body = provider(13, 1, "downstream_pop");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ASSERT_EQ(queue->getNumResults(), 4u);
  EXPECT_EQ(queue.getOutputs()[0], queue->getResult(0));
  EXPECT_EQ(queue.getOutputs()[1], queue->getResult(1));
  EXPECT_EQ(queue.getOutputs()[2], queue->getResult(2));
  EXPECT_EQ(queue.getOutputs()[3], queue->getResult(3));
  setQueueLatency(queue, integer(3));
  auto yielded = cast<ac::YieldOp>(module.getBody().front().back());
  SmallVector<Value> results(yielded.getValues());
  results.push_back(queue.getOutputs()[3]);
  yielded->setOperands(results);
  Builder b(&context);
  module->setAttr("output_names", b.getStrArrayAttr({"in_ready", "out_valid", "out_data", "local_capacity"}));
  updateResultSignature(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  auto facts = analysis.analyzeModule(module, bindings);
  ASSERT_TRUE(succeeded(facts));
  ASSERT_EQ(facts->dependencies.size(), 4u);
  ASSERT_EQ(facts->dependencies[0].inputs.size(), 1u);
  EXPECT_EQ(facts->dependencies[0].inputs[0].port, 2u);
  EXPECT_TRUE(facts->dependencies[3].inputs.empty());
  ASSERT_EQ(facts->storageWork.size(), 1u);
  EXPECT_EQ(facts->storageWork[0], queue.getOperation());
  auto config = analysis.resolveQueue(queue, bindings);
  ASSERT_TRUE(succeeded(config));
  EXPECT_EQ(config->depth, 1u);
  EXPECT_EQ(config->storageBitCount, 13u);
  EXPECT_EQ(config->timingStorageBitCount, 6u);
  auto header = clone(*body);
  ASSERT_TRUE(succeeded(extract(*header)));
  auto published = analysis.getImportDependencies(declaration(*header), bindings);
  ASSERT_TRUE(succeeded(published));
  ASSERT_EQ(published->size(), 4u);
  EXPECT_TRUE((*published)[3].inputs.empty());
}

TEST_F(QueueContractsTest, RetiredQueueOpFailsAtOwningContract) {
  auto body = provider(13, 1, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto original = onlyQueue(module);
  OpBuilder b(original);
  OperationState state(original.getLoc(), ac::QueueOp::getOperationName());
  state.addOperands(original.getInputs());
  SmallVector<Type> legacyTypes;
  for (unsigned i = 0; i < 3; ++i)
    legacyTypes.push_back(original->getResult(i).getType());
  state.addTypes(legacyTypes);
  auto legacy = cast<ac::QueueOp>(b.create(state));
  rejects([&] { return legacy.verify(); },
          "retired ac.queue is not admitted");
  legacy.erase();
}

TEST_F(QueueContractsTest, ActualUnchangedProviderRejectsReadyHeaderTamper) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  const auto original = print(*body);
  auto actualHeader = clone(*body);
  ASSERT_TRUE(succeeded(extract(*actualHeader)));
  auto forgedHeader = clone(*actualHeader);
  auto imported = declaration(*forgedHeader);
  Builder b(&context);
  auto old = imported.getDependencySummaryAttr();
  ASSERT_TRUE(old);
  EXPECT_EQ(imported->getAttr("semantic_kind"), b.getStringAttr("hardware"));
  EXPECT_FALSE(imported->hasAttr("transaction_summary"));
  SmallVector<Attribute> rows(old.begin(), old.end());
  NamedAttrList ready(cast<DictionaryAttr>(rows[0]));
  ready.set("inputs", b.getArrayAttr({}));
  rows[0] = ready.getDictionary(&context);
  imported->setAttr("dependency_summary", b.getArrayAttr(rows));
  // Intrinsic and link-wide revalidation must recompute the original SSA.
  rejects(
      [&] {
        return compiler::admitSourceLinkUnits({{*body, *forgedHeader}},
                                              error());
      },
      "published dependency summary differs from module SSA: @Provider");
  EXPECT_EQ(print(*body), original);
  // Matching forged consumer snapshot and public fresh/replacement protection
  // are next completions after the seam, using current APIs in fixture-plan.
}

TEST_F(QueueContractsTest,
       PositiveU64AvailabilityKeepsPayloadGeometryAndPublishedDependencies) {
  // Width goldens come from bit_width(L - 1), including both sides of powers
  // of two. Latency never adds payload slots or combinational dependencies.
  const std::pair<StringRef, unsigned> latencies[] = {
      {"2", 1},
      {"3", 2},
      {"4", 2},
      {"5", 3},
      {"8", 3},
      {"9", 4},
      {"9223372036854775808", 63},
      {"18446744073709551615", 64}};
  const unsigned depths[] = {1, 2, 3, 5}, pointers[] = {1, 1, 2, 3},
                 counts[] = {1, 2, 2, 3};
  for (auto [latency, timestampWidth] : latencies)
    for (unsigned i = 0; i != 4; ++i)
      for (StringRef policy : {"local_occupancy", "downstream_pop"}) {
        SCOPED_TRACE(latency.str() + "/D" + std::to_string(depths[i]) + "/" +
                     policy.str());
        auto body = provider(13, depths[i], policy);
        ASSERT_TRUE(body);
        auto module = definition(*body);
        auto queue = onlyQueue(module);
        setQueueLatency(queue, exactInteger(latency));
        ac::HardwareAnalysis analysis(*body);
        ac::HardwareBindings bindings;
        bindings.owner = module;
        ASSERT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings)));
        auto config = analysis.resolveQueue(queue, bindings);
        ASSERT_TRUE(succeeded(config));
        EXPECT_EQ(config->availabilityLatency,
                  llvm::APSInt(latency).getZExtValue());
        EXPECT_EQ(config->timestampWidth, timestampWidth);
        EXPECT_EQ(config->pointerWidth, pointers[i]);
        EXPECT_EQ(config->countWidth, counts[i]);
        EXPECT_EQ(config->storageBitCount, 13u * depths[i]);
        EXPECT_EQ(config->timingStorageBitCount,
                  uint64_t(depths[i]) * timestampWidth + timestampWidth +
                      pointers[i] + counts[i]);
        auto header = clone(*body);
        ASSERT_TRUE(succeeded(extract(*header)));
        auto dependencies =
            analysis.getImportDependencies(declaration(*header), bindings);
        ASSERT_TRUE(succeeded(dependencies));
        ASSERT_EQ(dependencies->size(), 3u);
        EXPECT_EQ((*dependencies)[0].inputs.size(),
                  policy == "downstream_pop" ? 1u : 0u);
        if (policy == "downstream_pop")
          EXPECT_EQ((*dependencies)[0].inputs[0].port, 2u);
        EXPECT_TRUE((*dependencies)[1].inputs.empty());
        EXPECT_TRUE((*dependencies)[2].inputs.empty());
        ASSERT_TRUE(succeeded(compiler::linkSourceUnits(
            {{*body, *header}}, "Provider", error())));
      }
}

TEST_F(QueueContractsTest,
       AvailabilityRejectsNonPositiveNonIntegerAndBeyondU64) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  for (StringRef value : {"0", "-1", "18446744073709551616"}) {
    setQueueLatency(queue, exactInteger(value));
    rejects(
        [&] { return analysis.verifyQueueOperation(queue, bindings, false); },
        "availability_latency must be positive and fit u64");
    rejects([&] { return analysis.resolveQueue(queue, bindings); },
            "availability_latency must be positive and fit u64");
  }
  for (bool value : {false, true}) {
    setQueueLatency(queue, boolean(value));
    rejects(
        [&] { return analysis.verifyQueueOperation(queue, bindings, false); },
        "integer formal requires mathematical integer");
    rejects([&] { return analysis.resolveQueue(queue, bindings); },
            "integer formal requires mathematical integer");
  }
}

TEST_F(QueueContractsTest,
       TimingGeometryRejectsBeforeUnresolvedPayloadReturns) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  auto module = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto queue = onlyQueue(module);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  // T stays unbound: known timing must still reject both product overflow and
  // a representable deadline array whose metadata addition overflows.
  setQueueDepth(queue, exactInteger("18446744073709551615"));
  for (unsigned latency : {3u, 2u}) {
    setQueueLatency(queue, integer(latency));
    rejects(
        [&] { return analysis.verifyQueueOperation(queue, bindings, false); },
        "logical timing storage bit count overflows u64");
  }
  // This exactly fits the timing dimension with symbolic T; payload size is
  // deliberately not inferred from an unrelated occurrence.
  setQueueDepth(queue, exactInteger("18446744073709551486"));
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
  setQueueDepth(queue, exactInteger("9223372036854775807"));
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
  bindings.types["T"] = ac::BitsType::get(&context, integer(1));
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings, false); },
          "combined logical storage bit count overflows u64");
  rejects([&] { return analysis.resolveQueue(queue, bindings); },
          "combined logical storage bit count overflows u64");
  setQueueDepth(queue, integer(3));
  auto valid = analysis.resolveQueue(queue, bindings);
  ASSERT_TRUE(succeeded(valid));
  EXPECT_EQ(valid->storageBitCount, 3u);
  EXPECT_EQ(valid->timingStorageBitCount, 8u);
}

TEST_F(QueueContractsTest, SharedUnsignedRendererPreservesExactClosedLatency) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  compiler::HardwareEmitContext emitter(*body, analysis);
  for (StringRef decimal :
       {"1", "2", "9223372036854775808", "18446744073709551615"}) {
    // Exercise exact evaluation too, rather than only literal recognition.
    auto value = binary("add", exactInteger(decimal), integer(0));
    auto cpp = emitter.unsignedStaticValue(value, queue);
    auto rtl = emitter.unsignedStaticValue(value, queue, true);
    ASSERT_TRUE(succeeded(cpp));
    ASSERT_TRUE(succeeded(rtl));
    EXPECT_EQ(*cpp, decimal.str() + "ULL");
    EXPECT_EQ(*rtl, "64'd" + decimal.str());
    setQueueLatency(queue, value);
    auto final = finalForEmission(*body);
    ASSERT_TRUE(final);
    ac::HardwareAnalysis finalAnalysis(*final);
    compiler::HardwareEmitContext finalEmitter(*final, finalAnalysis);
    EXPECT_TRUE(succeeded(finalEmitter.prepare()));
  }
  for (StringRef decimal : {"0", "-1", "18446744073709551616"})
    for (bool rtl : {false, true})
      rejects(
          [&] {
            return emitter.unsignedStaticValue(exactInteger(decimal), queue,
                                               rtl);
          },
          "positive integer fitting u64");
  for (bool rtl : {false, true})
    rejects(
        [&] { return emitter.unsignedStaticValue(boolean(true), queue, rtl); },
        "positive integer fitting u64");
  addIntegerFormals(module, {"L"});
  auto latency = formal("Provider", "L");
  ac::HardwareBindings bindings;
  bindings.owner = module;
  EXPECT_FALSE(emitter.isClosedStatic(latency, bindings));
  EXPECT_TRUE(
      succeeded(emitter.unsignedStaticValue(latency, queue, false, bindings)));
  EXPECT_TRUE(
      succeeded(emitter.unsignedStaticValue(latency, queue, true, bindings)));
  bindings.integers["L"] = exactInteger("18446744073709551615");
  EXPECT_TRUE(emitter.isClosedStatic(latency, bindings));
  auto closed = emitter.unsignedStaticValue(latency, queue, false, bindings);
  ASSERT_TRUE(succeeded(closed));
  EXPECT_EQ(*closed, "18446744073709551615ULL");
}

TEST_F(QueueContractsTest,
       CapacityWireStorageIsCountedBeforeAnyRuntimeAllocation) {
  const uint64_t maximum = std::numeric_limits<std::ptrdiff_t>::max();
  const uint64_t depth = (maximum - 128 - 192) / 24 - 5;
  // One Bits1 token occupies three64-bit planes,24bytes.
  // Independently derive the exact old/new metadata boundary; do not allocate.
  ASSERT_EQ(24 * (depth + 5) + 192 + 128, maximum - 23);
  ASSERT_EQ(24 * (depth + 5) + 216 + 128, maximum + 1);
  auto body = provider(1, 1, "local_occupancy");
  ASSERT_TRUE(body);
  setQueueDepth(onlyQueue(definition(*body)),
                exactInteger(std::to_string(depth)));
  auto final = finalForEmission(*body);
  ASSERT_TRUE(final);
  auto before = print(*final);
  ac::HardwareAnalysis analysis(*final);
  compiler::HardwareEmitContext emitter(*final, analysis);
  rejects([&] { return emitter.prepare(); }, "Runtime object capacity");
  EXPECT_EQ(print(*final), before);
}

TEST_F(QueueContractsTest,
       EmissionChecksCombinedTimingAndFamilyCapacityBeforeAllocation) {
  // These broad capacity witnesses do not freeze padding or an exact emitted
  // storage recipe: L1 fits, but the D delayed timestamps cannot fit with it.
  const uint64_t capacity = std::numeric_limits<std::ptrdiff_t>::max();
  for (bool family : {false, true})
    for (unsigned latency : {1u, 2u}) {
      auto body = provider(1, 3, "local_occupancy");
      ASSERT_TRUE(body);
      auto queue = onlyQueue(definition(*body));
      setQueueDepth(queue, exactInteger(std::to_string(
                               capacity / (family ? 150 : 30))));
      setQueueLatency(queue, integer(latency));
      if (family)
        addQueueFamilyRoot(*body, 5);
      auto final = finalForEmission(*body, family ? "FamilyRoot" : "Provider");
      ASSERT_TRUE(final);
      ac::HardwareAnalysis analysis(*final);
      compiler::HardwareEmitContext emitter(*final, analysis);
      if (latency == 1)
        EXPECT_TRUE(succeeded(emitter.prepare()));
      else
        rejects([&] { return emitter.prepare(); }, "Runtime object capacity");
    }
  auto body = provider(1, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto queue = onlyQueue(definition(*body));
  setQueueDepth(queue, exactInteger(std::to_string(capacity / 4)));
  setQueueLatency(queue, integer(2));
  auto final = finalForEmission(*body);
  ASSERT_TRUE(final);
  ac::HardwareAnalysis analysis(*final);
  compiler::HardwareEmitContext emitter(*final, analysis);
  rejects([&] { return emitter.prepare(); }, "Runtime object capacity");
}

TEST_F(QueueContractsTest, UnusedConcreteQueuesStillReceiveTimingPreflight) {
  auto body = provider(1, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto unused = cast<ac::ModuleOp>(definition(*body)->clone());
  unused->setAttr("sym_name", Builder(&context).getStringAttr("Unused"));
  auto queue = onlyQueue(unused);
  queue->setAttr("occurrence", occurrence("Unused"));
  setQueueLatency(queue, integer(2));
  body->getBody()->push_back(unused);
  envelope(*body, "__init__.py");
  auto safe = finalForEmission(*body);
  ASSERT_TRUE(safe);
  ac::HardwareAnalysis safeAnalysis(*safe);
  compiler::HardwareEmitContext safeEmitter(*safe, safeAnalysis);
  ASSERT_TRUE(succeeded(safeEmitter.prepare()));
  setQueueDepth(queue, exactInteger(std::to_string(
                           std::numeric_limits<std::ptrdiff_t>::max() / 30)));
  auto oversized = finalForEmission(*body);
  ASSERT_TRUE(oversized);
  ac::HardwareAnalysis analysis(*oversized);
  compiler::HardwareEmitContext emitter(*oversized, analysis);
  rejects([&] { return emitter.prepare(); }, "Runtime object capacity");
}

TEST_F(QueueContractsTest,
       CommonStaticTableWidthAndSelectedDepthIgnoreUndemandedFormal) {
  auto body = provider(130, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto unbound = declaredUnbound(module);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  Builder b(&context);
  auto table = ac::TableType::get(&context, b.getArrayAttr({integer(3)}),
                                  queuePayload(queue));
  auto width =
      expression("type_width", {b.getNamedAttr("type", TypeAttr::get(table))});
  EXPECT_TRUE(analysis.isStaticEvaluable(width, bindings));
  auto evaluated = analysis.evaluateStatic(width, bindings, queue);
  ASSERT_TRUE(succeeded(evaluated));
  EXPECT_EQ(cast<ac::MathIntAttr>(*evaluated).getCanonicalValue(), "390");
  for (bool chosen : {false, true}) {
    auto depth = chosen ? select(boolean(true), integer(3), unbound)
                        : select(boolean(false), unbound, integer(3));
    EXPECT_TRUE(analysis.isStaticEvaluable(depth, bindings));
    setQueueDepth(queue, depth);
    auto resolved = analysis.resolveQueue(queue, bindings);
    ASSERT_TRUE(succeeded(resolved));
    EXPECT_EQ(resolved->depth, 3u);
    auto zeroDepth = chosen ? select(boolean(true), integer(0), unbound)
                            : select(boolean(false), unbound, integer(0));
    EXPECT_TRUE(analysis.isStaticEvaluable(zeroDepth, bindings));
    setQueueDepth(queue, zeroDepth);
    // Only the owning FIFO depth guard may reject this closed selected zero.
    rejects([&] { return analysis.resolveQueue(queue, bindings); }, "depth");
  }
}

TEST_F(QueueContractsTest,
       CommonStaticInvalidClosedConditionHasOneOwningDiagnostic) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto unbound = declaredUnbound(module);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  auto invalid = select(integer(1), integer(3), unbound);
  unsigned count = 0;
  ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
    ++count;
    llvm::raw_string_ostream out(diagnostics);
    diagnostic.print(out);
    return success();
  });
  EXPECT_TRUE(analysis.isStaticEvaluable(invalid, bindings));
  EXPECT_EQ(count, 0u);
  EXPECT_TRUE(failed(analysis.evaluateStatic(invalid, bindings, queue)));
  EXPECT_EQ(count, 1u);
  EXPECT_NE(diagnostics.find("static select requires bool condition"),
            std::string::npos);
}

TEST_F(QueueContractsTest,
       CommonStaticBooleanShortCircuitDemandsOnlyActiveArithmeticFault) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  auto fault =
      binary("eq", binary("floordiv", integer(7), integer(0)), integer(0));
  for (StringRef op : {"and_bool", "or_bool"}) {
    const bool undemanded = op == "or_bool";
    auto inactive = binary(op, boolean(undemanded), fault);
    EXPECT_TRUE(analysis.isStaticEvaluable(inactive, bindings));
    auto value = analysis.evaluateStatic(inactive, bindings, queue);
    ASSERT_TRUE(succeeded(value));
    EXPECT_EQ(cast<BoolAttr>(*value).getValue(), undemanded);
    auto active = binary(op, boolean(!undemanded), fault);
    EXPECT_TRUE(analysis.isStaticEvaluable(active, bindings));
    unsigned count = 0;
    ScopedDiagnosticHandler handler(&context, [&](Diagnostic &diagnostic) {
      ++count;
      llvm::raw_string_ostream out(diagnostics);
      diagnostic.print(out);
      return success();
    });
    EXPECT_TRUE(failed(analysis.evaluateStatic(active, bindings, queue)));
    EXPECT_EQ(count, 1u);
  }
}

TEST_F(QueueContractsTest, NominalAndTablePacketsKeepTokenGeometryAndIdentity) {
  for (StringRef policy : {"local_occupancy", "downstream_pop"})
    for (unsigned depth : {1u, 2u, 3u, 5u})
      for (StringRef payload :
           {"!ac.struct<\"RecordA\">", "!ac.struct<\"RecordB\">",
            "!ac.table<[#three], !ac.bits<#width>>",
            "!ac.table<[#three], !ac.struct<\"RecordA\">>"}) {
        auto body = payloadProvider(payload, depth, policy);
        ASSERT_TRUE(body);
        auto module = definition(*body);
        auto queue = onlyQueue(module);
        ac::HardwareAnalysis analysis(*body);
        ac::HardwareBindings bindings;
        bindings.owner = module;
        auto config = analysis.resolveQueue(queue, bindings);
        ASSERT_TRUE(succeeded(config));
        const bool table = payload.starts_with("!ac.table");
        const bool bits = payload.contains("!ac.bits");
        const uint64_t width = table ? (bits ? 390u : 237u) : 79u;
        EXPECT_EQ(config->packedWidth, width);
        EXPECT_EQ(config->tokenCardinality, table ? 3u : 1u);
        EXPECT_EQ(config->storageBitCount, width * depth);
        EXPECT_EQ(config->tokenShape.size(), table ? 1u : 0u);
        if (table)
          EXPECT_EQ(config->tokenShape[0], 3u);
        EXPECT_TRUE(ac::areEquivalentHardwareTypes(
            config->payloadType, queuePayload(queue)));
        setQueueLatency(queue, integer(3));
        auto delayed = analysis.resolveQueue(queue, bindings);
        ASSERT_TRUE(succeeded(delayed));
        EXPECT_EQ(delayed->packedWidth, width);
        EXPECT_EQ(delayed->storageBitCount, width * depth);
        EXPECT_EQ(delayed->tokenCardinality, table ? 3u : 1u);
        EXPECT_EQ(delayed->timestampWidth, 2u);
        EXPECT_EQ(delayed->timingStorageBitCount, depth == 1   ? 6u
                                                  : depth == 2 ? 9u
                                                  : depth == 3 ? 12u
                                                               : 18u);
        auto header = clone(*body);
        ASSERT_TRUE(succeeded(extract(*header)));
        auto published =
            analysis.getImportDependencies(declaration(*header), bindings);
        ASSERT_TRUE(succeeded(published));
        for (auto &row : *published)
          if (row.output.port != 0)
            EXPECT_TRUE(row.inputs.empty());
        ASSERT_TRUE(succeeded(compiler::linkSourceUnits(
            {{*body, *header}}, "Provider", error())));
      }
}

TEST_F(QueueContractsTest,
       GenericAndTableTokenBindingsResolveWithoutCloningDefinitions) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  Builder b(&context);
  auto generic = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto table = cast<ac::ModuleOp>(analysis.lookupDefinition("Token3"));
  auto scalarQueue = onlyQueue(generic), tableQueue = onlyQueue(table);
  ac::HardwareBindings open;
  open.owner = generic;
  ASSERT_TRUE(
      succeeded(analysis.verifyQueueOperation(scalarQueue, open, false)));
  auto header = clone(*body);
  ASSERT_TRUE(succeeded(extract(*header)));
  const auto definitionBefore = print(*body);
  SmallVector<Type> payloads;
  for (unsigned width : {1u, 13u, 65u, 130u})
    payloads.push_back(ac::BitsType::get(&context, integer(width)));
  payloads.push_back(ac::StructType::get(&context, b.getStringAttr("RecordA")));
  payloads.push_back(ac::StructType::get(&context, b.getStringAttr("RecordB")));
  const uint64_t widths[] = {1, 13, 65, 130, 79, 79};
  for (auto [index, payload] : llvm::enumerate(payloads))
    for (unsigned depth : {1u, 2u, 3u, 5u})
      for (StringRef policy : {"local_occupancy", "downstream_pop"})
        for (bool tokenTable : {false, true}) {
          auto module = tokenTable ? table : generic;
          auto queue = tokenTable ? tableQueue : scalarQueue;
          setQueuePolicy(queue, policy);
          ac::HardwareBindings bound;
          bound.owner = module;
          bound.types["T"] = payload;
          bound.integers["D"] = ac::MathIntAttr::get(
              &context, llvm::APSInt(std::to_string(depth)));
          auto config = analysis.resolveQueue(queue, bound);
          ASSERT_TRUE(succeeded(config));
          EXPECT_EQ(config->packedWidth,
                    widths[index] * (tokenTable ? 3u : 1u));
          EXPECT_EQ(config->tokenCardinality, tokenTable ? 3u : 1u);
          EXPECT_TRUE(ac::areEquivalentHardwareTypes(config->tokenElementType,
                                                     payload));
          EXPECT_EQ(config->depth, depth);
          EXPECT_EQ(config->storageBitCount, config->packedWidth * depth);
        }
  // Policy mutations only; generic declarations/signatures are not cloned or
  // specialized into width/shape recipes. Reset policy before byte comparison.
  setQueuePolicy(scalarQueue, "local_occupancy");
  setQueuePolicy(tableQueue, "local_occupancy");
  EXPECT_EQ(print(*body), definitionBefore);
  EXPECT_FALSE(ac::areEquivalentHardwareTypes(payloads[4], payloads[5]));
}

TEST_F(QueueContractsTest, OrdinaryGenericModuleAcceptsWholeTableTypeActual) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  auto root = cast<ac::ModuleOp>(analysis.lookupDefinition("Root"));
  ac::InstanceOp tableInstance;
  for (auto instance : root.getBody().front().getOps<ac::InstanceOp>())
    if (instance.getInstanceName() == "table")
      tableInstance = instance;
  ASSERT_TRUE(tableInstance);
  auto tableType = tableInstance.getInputs()[1].getType();
  ASSERT_TRUE(isa<ac::TableType>(tableType));
  Builder b(&context);
  tableInstance->setAttr("callee",
                         FlatSymbolRefAttr::get(&context, "Generic"));
  tableInstance->setAttr("type_arguments",
                         b.getArrayAttr({TypeAttr::get(tableType)}));
  ASSERT_TRUE(succeeded(analysis.verifyInstance(tableInstance)));
  auto bindings = analysis.bindInstance(tableInstance);
  ASSERT_TRUE(succeeded(bindings));
  EXPECT_EQ(bindings->types.lookup("T"), tableType);
  auto generic = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto config = analysis.resolveQueue(onlyQueue(generic), *bindings);
  ASSERT_TRUE(succeeded(config));
  EXPECT_EQ(config->packedWidth, 390u);
  EXPECT_EQ(config->tokenCardinality, 3u);
}

TEST_F(QueueContractsTest,
       OrdinaryStoragePrimitiveStillRejectsWholeTableTypeActual) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  auto registry = compiler::SourceHeaderRegistry::create(
      &context, {}, [&] { return emitError(UnknownLoc::get(&context)); });
  ASSERT_TRUE(succeeded(registry));
  auto dffe = registry->lookupBuiltin("dffe");
  ASSERT_TRUE(dffe);
  OpBuilder::atBlockBegin(body->getBody()).insert(dffe.declaration->clone());
  ac::HardwareAnalysis analysis(*body);
  auto root = cast<ac::ModuleOp>(analysis.lookupDefinition("Root"));
  auto &rootBody = root.getBody().front();
  auto tableType = rootBody.getArgument(2).getType();
  ASSERT_TRUE(isa<ac::TableType>(tableType));
  OpBuilder insertion(rootBody.getTerminator());
  OperationState state(root.getLoc(), ac::InstanceOp::getOperationName());
  state.addOperands({rootBody.getArgument(4), rootBody.getArgument(5),
                     rootBody.getArgument(0), rootBody.getArgument(2),
                     rootBody.getArgument(2)});
  state.addTypes(tableType);
  state.addAttribute("instance_name", insertion.getStringAttr("table_state"));
  state.addAttribute("callee", dffe.symbol);
  state.addAttribute("parameters", insertion.getArrayAttr({}));
  state.addAttribute("type_arguments",
                     insertion.getArrayAttr({TypeAttr::get(tableType)}));
  state.addAttribute("occurrence", occurrence("Root"));
  auto storage = cast<ac::InstanceOp>(insertion.create(state));
  rejects([&] { return analysis.verifyInstance(storage); },
          "storage and memory primitives require a scalar payload binding");
}

TEST_F(QueueContractsTest, DirectBindingScalarContractCyclesAndAliasChain) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  Builder b(&context);
  auto module = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto queue = onlyQueue(module);
  module->setAttr("type_parameters",
                  b.getArrayAttr({b.getStringAttr("T"), b.getStringAttr("U")}));
  auto t = ac::TypeParamType::get(&context,
                                  FlatSymbolRefAttr::get(&context, "Generic"),
                                  b.getStringAttr("T"));
  auto u = ac::TypeParamType::get(&context,
                                  FlatSymbolRefAttr::get(&context, "Generic"),
                                  b.getStringAttr("U"));
  ac::HardwareBindings bound;
  bound.owner = module;
  bound.integers["D"] = ac::MathIntAttr::get(&context, llvm::APSInt("3"));
  bound.types["T"] =
      ac::TableType::get(&context, b.getArrayAttr({integer(3)}), t);
  rejects([&] { return analysis.verifyQueueOperation(queue, bound, false); },
          "cyclic");
  rejects([&] { return analysis.resolveQueue(queue, bound); }, "cyclic");
  bound.types["T"] = u;
  bound.types["U"] = t;
  rejects([&] { return analysis.verifyQueueOperation(queue, bound, false); },
          "cyclic");
  rejects([&] { return analysis.resolveQueue(queue, bound); }, "cyclic");
  bound.types["U"] = ac::BitsType::get(&context, integer(13));
  ASSERT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bound, false)));
  auto valid = analysis.resolveQueue(queue, bound);
  ASSERT_TRUE(succeeded(valid));
  EXPECT_EQ(valid->packedWidth, 13u);
  bound.types.clear();
  rejects([&] { return analysis.resolveQueue(queue, bound); },
          "resolved finite");
  bound.owner = cast<ac::ModuleOp>(analysis.lookupDefinition("Token3"));
  rejects([&] { return analysis.resolveQueue(queue, bound); }, "owning module");
}

TEST_F(QueueContractsTest,
       LogicalU64GeometryAndNominalPayloadMismatchFailClosed) {
  auto body = provider(1, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bound;
  bound.owner = module;
  setQueueDepth(queue, exactInteger("18446744073709551615"));
  auto max = analysis.resolveQueue(queue, bound);
  ASSERT_TRUE(succeeded(max));
  EXPECT_EQ(max->depth, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(max->pointerWidth, 64u);
  EXPECT_EQ(max->countWidth, 64u);
  EXPECT_EQ(max->storageBitCount, std::numeric_limits<uint64_t>::max());
  for (StringRef value : {"0", "-1", "18446744073709551616"}) {
    setQueueDepth(queue, exactInteger(value));
    rejects([&] { return analysis.resolveQueue(queue, bound); },
            "depth must be positive and fit u64");
  }
  auto overflow = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(overflow);
  ac::HardwareAnalysis overflowAnalysis(*overflow);
  auto overflowModule = definition(*overflow);
  auto overflowQueue = onlyQueue(overflowModule);
  bound.owner = overflowModule;
  setQueueDepth(overflowQueue, exactInteger("18446744073709551615"));
  rejects([&] { return overflowAnalysis.resolveQueue(overflowQueue, bound); },
          "logical storage bit count overflows u64");
  auto nominal =
      payloadProvider("!ac.struct<\"RecordA\">", 3, "local_occupancy");
  ASSERT_TRUE(nominal);
  ac::HardwareAnalysis nominalAnalysis(*nominal);
  auto nominalModule = definition(*nominal);
  auto nominalQueue = onlyQueue(nominalModule);
  nominalQueue.getOutputs()[2].setType(ac::StructType::get(
      &context, Builder(&context).getStringAttr("RecordB")));
  updateResultSignature(nominalModule);
  bound.owner = nominalModule;
  rejects([&] { return nominalAnalysis.resolveQueue(nominalQueue, bound); },
          "payload types must match");
}

TEST_F(QueueContractsTest,
       NativeProfileTamperingAndEveryControlWidthReject) {
  Builder b(&context);
  for (auto [name, expected] :
       {std::pair<StringRef, StringRef>{"instance_name",
                                        "nonempty instance_name"},
        {"parameters", "instance parameter arity mismatch"},
        {"occurrence", "occurrence requires a dictionary"}}) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    queue->removeAttr(name);
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            expected);
  }
  for (StringRef name : {"depth", "ready_policy", "availability_latency",
                         "head_read_latency", "empty_flow",
                         "read_during_write", "reset_policy", "empty_data"}) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    queue->setAttr(name, b.getBoolAttr(true));
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            "cannot duplicate retired");
  }
  {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    queue->setAttr("callee", FlatSymbolRefAttr::get(&context, "Provider"));
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            "canonical FIFO instance");
  }
  {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    queue->setAttr("parameters", b.getArrayAttr({integer(3)}));
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            "canonical FIFO instance requires explicit DEPTH and "
            "AVAILABILITY_LATENCY arguments");
  }
  for (unsigned position = 0; position != 7; ++position) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    auto wide = ac::BitsType::get(&context, integer(13));
    if (position < 4) {
      OpBuilder builder(queue);
      OperationState constant(queue.getLoc(), "ac.bits.constant");
      constant.addTypes(wide);
      constant.addAttribute("value", integer(0));
      queue->setOperand(position == 3 ? 4 : position,
                        builder.create(constant)->getResult(0));
    } else {
      queue->getResult(position == 6 ? 3 : position - 4).setType(wide);
      updateResultSignature(module);
    }
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            "controls must have width one");
  }
}

TEST_F(QueueContractsTest, CanonicalFifoRequiresMatchingTrustedImportProfile) {
  for (unsigned mutation = 0; mutation != 2; ++mutation) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    auto imported = cast<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(*body, queue.getCalleeAttr()));
    if (mutation == 0)
      imported->removeAttr("primitive_kind");
    else
      imported->setAttr("primitive_kind",
                        Builder(&context).getStringAttr("fifo_downstream_pop"));
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bindings;
    bindings.owner = module;
    rejects([&] { return analysis.resolveQueue(queue, bindings); },
            "canonical FIFO requires its trusted builtin import");
    (*body)->setAttr("ac.stage", Builder(&context).getStringAttr("final"));
    rejects([&] { return mlir::verify(*body); }, "trusted");
  }
}

TEST_F(QueueContractsTest, ActualMatchingForgedConsumerCannotHideProviderBody) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto actual = clone(*body);
  ASSERT_TRUE(succeeded(extract(*actual)));
  auto forged = forgedReady(*actual);
  auto used = consumer(*forged);
  auto consumerHeader = clone(*used);
  ASSERT_TRUE(succeeded(extract(*consumerHeader)));
  auto before = print(*body);
  rejects(
      [&] {
        return compiler::admitSourceLinkUnits(
            {{*used, *consumerHeader}, {*body, *forged}}, error());
      },
      "published dependency summary differs from module SSA: @Provider");
  EXPECT_EQ(print(*body), before);
}

TEST_F(QueueContractsTest,
       ChangedLocalBodyRejectsStaleHeaderAndFreshExtractionLinks) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto stale = clone(*body);
  ASSERT_TRUE(succeeded(extract(*stale)));
  auto local = clone(*body);
  auto module = definition(*local);
  auto queue = onlyQueue(module);
  setQueuePolicy(queue, "local_occupancy");
  rejects(
      [&] {
        return compiler::admitSourceLinkUnits({{*local, *stale}}, error());
      },
      "published dependency summary differs from module SSA: @Provider");
  auto fresh = clone(*local);
  ASSERT_TRUE(succeeded(extract(*fresh)));
  ASSERT_TRUE(succeeded(
      compiler::linkSourceUnits({{*local, *fresh}}, "Provider", error())));
}

TEST_F(QueueContractsTest, QueueOwnsStateAcrossCseAndAnalysis) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto first = onlyQueue(module);
  auto second = cast<ac::InstanceOp>(first->clone());
  second->setAttr("instance_name", Builder(&context).getStringAttr("other"));
  OpBuilder insertion(&module.getBody().front().back());
  insertion.insert(second);
  EXPECT_FALSE(isMemoryEffectFree(first));
  EXPECT_FALSE(isMemoryEffectFree(second));
  PassManager passes(&context);
  passes.addPass(createCSEPass());
  ASSERT_TRUE(succeeded(passes.run(*body)));
  EXPECT_EQ(
      std::distance(module.getBody().front().getOps<ac::InstanceOp>().begin(),
                    module.getBody().front().getOps<ac::InstanceOp>().end()),
      2);
  ac::HardwareAnalysis analysis(*body);
  auto work = analysis.analyzeModule(module);
  ASSERT_TRUE(succeeded(work));
  ASSERT_EQ(work->storageWork.size(), 2u);
  EXPECT_EQ(llvm::count(work->storageWork, first.getOperation()), 1);
  EXPECT_EQ(llvm::count(work->storageWork, second.getOperation()), 1);
}

TEST_F(QueueContractsTest,
       RealPublicLinkRecomputesBodyAndProtectsFreshAndReplacementOutputs) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto header = clone(*body);
  ASSERT_TRUE(succeeded(extract(*header)));
  auto consumerBody = consumer(*header);
  auto consumerHeader = clone(*consumerBody);
  ASSERT_TRUE(succeeded(extract(*consumerHeader)));
  Scratch scratch;
  auto goodProvider =
      publishedUnit(scratch, "provider_good", *body, *header, "__init__.py");
  auto goodConsumer = publishedUnit(scratch, "consumer_good", *consumerBody,
                                    *consumerHeader, "consumer.py");
  auto prior = scratch.file("prior.ac");
  ASSERT_EQ(publicLink(scratch, {goodProvider, goodConsumer}, prior, false,
                       "positive"),
            0)
      << read(scratch.file("positive.log"));
  auto protectedFiles = snapshotUnit(scratch, "provider_good", "__init__.py");
  auto consumerFiles = snapshotUnit(scratch, "consumer_good", "consumer.py");
  protectedFiles.insert(protectedFiles.end(), consumerFiles.begin(),
                        consumerFiles.end());
  protectedFiles.emplace_back(prior, read(prior));
  // A successful publication's owner/control bytes are part of the protection
  // oracle; a rejection cannot publish a new success generation.
  auto control = scratch.file(".prior.ac.pycircuit-publication");
  for (auto file : {"lock", "owner.json"})
    protectedFiles.emplace_back(control + "/" + file,
                                read(control + "/" + file));
  auto forged = forgedReady(*header);
  auto changed = clone(*body);
  setQueuePolicy(onlyQueue(definition(*changed)), "local_occupancy");
  auto missing = clone(*body), overrideBody = clone(*body),
       cyclic = clone(*body);
  onlyQueue(definition(*missing))->removeAttr("parameters");
  onlyQueue(definition(*overrideBody))
      ->setAttr("ready_policy", Builder(&context).getStringAttr("local_occupancy"));
  auto cyclicModule = definition(*cyclic);
  auto cyclicQueue = onlyQueue(cyclicModule);
  OpBuilder cycleBuilder(cyclicQueue);
  OperationState cycle(cyclicQueue.getLoc(), "ac.bits.binary");
  cycle.addOperands({cyclicModule.getBody().front().getArgument(0),
                     cyclicModule.getBody().front().getArgument(0)});
  cycle.addTypes(cyclicQueue.getInputs()[2].getType());
  cycle.addAttribute("opcode", cycleBuilder.getStringAttr("xor"));
  auto feedback = cycleBuilder.create(cycle);
  feedback->setOperand(0, feedback->getResult(0));
  cyclicQueue->setOperand(2, feedback->getResult(0));
  const StringRef guards[] = {
      "published dependency summary differs from module SSA: @Provider",
      "published dependency summary differs from module SSA: @Provider",
      "published dependency summary differs from module SSA: @Provider",
      "requires attribute 'parameters'",
      "cannot duplicate retired 'ready_policy'",
      "combinational cycle in hardware field dependencies"};
  for (unsigned mutation = 0; mutation != 6; ++mutation) {
    auto badBody = mutation == 2   ? *changed
                   : mutation == 3 ? *missing
                   : mutation == 4 ? *overrideBody
                   : mutation == 5 ? *cyclic
                                   : *body;
    auto badHeader = mutation < 2 ? *forged : *header;
    auto matching = consumer(badHeader);
    auto matchingHeader = clone(*matching);
    ASSERT_TRUE(succeeded(extract(*matchingHeader)));
    auto name = "provider_bad" + std::to_string(mutation);
    auto badProvider =
        publishedUnit(scratch, name, badBody, badHeader, "__init__.py");
    auto consumerName = "consumer_bad" + std::to_string(mutation);
    auto badConsumer = mutation == 1
                           ? publishedUnit(scratch, consumerName, *matching,
                                           *matchingHeader, "consumer.py")
                           : goodConsumer;
    auto inputs = snapshotUnit(scratch, name, "__init__.py");
    if (mutation == 1) {
      auto other = snapshotUnit(scratch, consumerName, "consumer.py");
      inputs.insert(inputs.end(), other.begin(), other.end());
    }
    for (bool replacing : {false, true}) {
      auto label = "negative" + std::to_string(mutation) +
                   (replacing ? "-replacement" : "-fresh");
      auto output = replacing ? prior : scratch.file(label + ".ac");
      EXPECT_EQ(publicLink(scratch, {badProvider, badConsumer}, output,
                           replacing, label),
                1);
      EXPECT_NE(read(scratch.file(label + ".log")).find(guards[mutation].str()),
                std::string::npos)
          << read(scratch.file(label + ".log"));
      if (!replacing) {
        EXPECT_FALSE(llvm::sys::fs::exists(output));
        auto idle = scratch.file("." + label + ".ac.pycircuit-publication");
        std::error_code ec;
        std::vector<std::string> names;
        for (llvm::sys::fs::directory_iterator entry(idle, ec), end;
             entry != end && !ec; entry.increment(ec))
          names.push_back(llvm::sys::path::filename(entry->path()).str());
        EXPECT_FALSE(ec);
        std::sort(names.begin(), names.end());
        EXPECT_EQ(names, (std::vector<std::string>{"lock", "owner.json"}));
        auto marker = llvm::json::parse(read(idle + "/owner.json"));
        ASSERT_TRUE(bool(marker));
        auto object = marker->getAsObject();
        ASSERT_TRUE(object);
        EXPECT_EQ(object->size(), 2u);
        EXPECT_EQ(object->getString("kind"), "pycircuit-publication-control");
        EXPECT_EQ(object->getString("destination"), label + ".ac");
      }
      EXPECT_FALSE(llvm::sys::fs::exists(control + "/journal.json"));
      EXPECT_FALSE(llvm::sys::fs::exists(control + "/stage"));
      EXPECT_FALSE(llvm::sys::fs::exists(control + "/previous"));
      for (auto &file : protectedFiles)
        EXPECT_EQ(read(file.first), file.second) << file.first;
      for (auto &file : inputs)
        EXPECT_EQ(read(file.first), file.second) << file.first;
    }
  }
}

TEST_F(QueueContractsTest, NumericActualAliasesPreserveDemandAndOwningFailure) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  auto module =
      cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(*body, "Generic"));
  auto queue = onlyQueue(module);
  Builder b(&context);
  module->setAttr(
      "parameters",
      b.getArrayAttr(
          {b.getDictionaryAttr(
               {b.getNamedAttr("name", b.getStringAttr("D")),
                b.getNamedAttr("type",
                               TypeAttr::get(ac::MathIntType::get(&context)))}),
           b.getDictionaryAttr(
               {b.getNamedAttr("name", b.getStringAttr("E")),
                b.getNamedAttr(
                    "type", TypeAttr::get(ac::MathIntType::get(&context)))})}));
  auto reference = [&](StringRef name, StringRef ownerName = "Generic") {
    return expression(
        "reference",
        {b.getNamedAttr(
            "ref", b.getDictionaryAttr(
                       {b.getNamedAttr("kind", b.getStringAttr("parameter")),
                        b.getNamedAttr("owner", FlatSymbolRefAttr::get(
                                                    &context, ownerName)),
                        b.getNamedAttr("name", b.getStringAttr(name))}))});
  };
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bound;
  bound.owner = module;
  bound.types["T"] = ac::BitsType::get(&context, integer(13));
  bound.integers["D"] = reference("E");
  bound.integers["E"] = ac::MathIntAttr::get(&context, llvm::APSInt("3"));
  auto depth = reference("D");
  setQueueDepth(queue, depth);
  EXPECT_TRUE(analysis.isStaticEvaluable(depth, bound));
  auto valid = analysis.resolveQueue(queue, bound);
  ASSERT_TRUE(succeeded(valid));
  EXPECT_EQ(valid->depth, 3u);
  bound.integers["E"] = ac::MathIntAttr::get(&context, llvm::APSInt("-1"));
  auto invalid =
      select(binary("lt", depth, integer(0)), integer(0), integer(2));
  setQueueDepth(queue, invalid);
  EXPECT_TRUE(analysis.isStaticEvaluable(invalid, bound));
  rejects([&] { return analysis.verifyQueueOperation(queue, bound, false); },
          "depth must be positive and fit u64");
  rejects([&] { return analysis.resolveQueue(queue, bound); },
          "depth must be positive and fit u64");
  bound.integers["E"] = reference("D");
  setQueueDepth(queue, depth);
  EXPECT_TRUE(analysis.isStaticEvaluable(depth, bound));
  rejects([&] { return analysis.verifyQueueOperation(queue, bound, false); },
          "cyclic");
  rejects([&] { return analysis.resolveQueue(queue, bound); }, "cyclic");
  bound.integers["D"] = reference("N", "Forward");
  bound.integers.erase("E");
  EXPECT_FALSE(analysis.isStaticEvaluable(depth, bound));
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bound, false)));
  rejects([&] { return analysis.resolveQueue(queue, bound); }, "unresolved");
}

TEST_F(QueueContractsTest,
       ActualForwardedGenericOccurrenceUsesExistingBindings) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  Builder b(&context);
  auto generic = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto forward = cast<ac::ModuleOp>(analysis.lookupDefinition("Forward"));
  addIntegerFormals(generic, {"L"});
  addIntegerFormals(forward, {"A"});
  setQueueLatency(onlyQueue(generic), formal("Generic", "L"));
  auto inner = *forward.getBody().front().getOps<ac::InstanceOp>().begin();
  inner->setAttr("parameters", b.getArrayAttr({formal("Forward", "N"),
                                               formal("Forward", "A")}));
  auto root = cast<ac::ModuleOp>(analysis.lookupDefinition("Root"));
  auto outer = *root.getBody().front().getOps<ac::InstanceOp>().begin();
  OpBuilder insertion(&root.getBody().front().back());
  auto sibling = cast<ac::InstanceOp>(insertion.insert(outer->clone()));
  sibling->setAttr("instance_name", b.getStringAttr("other_latency"));
  // Cloning an operation creates a new source allocation site in this fixture.
  auto clonedSite = outer->getAttrOfType<DictionaryAttr>("occurrence");
  NamedAttrList site(clonedSite.getAs<DictionaryAttr>("site"));
  site.set("ast_path", b.getArrayAttr({
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("field")),
                           b.getNamedAttr("name", b.getStringAttr("body"))}),
      b.getDictionaryAttr({b.getNamedAttr("kind", b.getStringAttr("index")),
                           b.getNamedAttr("value", b.getI64IntegerAttr(
                               std::distance(root.getBody().front().begin(),
                                             sibling->getIterator())))})}));
  NamedAttrList allocation(clonedSite);
  allocation.set("site", site.getDictionary(&context));
  sibling->setAttr("occurrence", allocation.getDictionary(&context));
  sibling->setAttr("parameters", b.getArrayAttr({integer(3), integer(9)}));
  for (StringRef latency : {"2", "3", "5", "18446744073709551615"}) {
    outer->setAttr("parameters",
                   b.getArrayAttr({integer(3), exactInteger(latency)}));
    const auto before = print(*body);
    auto forwarding = analysis.bindInstance(outer);
    ASSERT_TRUE(succeeded(forwarding));
    EXPECT_EQ(forwarding->owner, forward.getOperation());
    auto bound = analysis.bindInstance(inner, *forwarding);
    ASSERT_TRUE(succeeded(bound));
    EXPECT_EQ(bound->owner, generic.getOperation());
    auto resolved = analysis.resolveQueue(onlyQueue(generic), *bound);
    ASSERT_TRUE(succeeded(resolved));
    EXPECT_EQ(resolved->packedWidth, 130u);
    EXPECT_EQ(resolved->depth, 3u);
    EXPECT_EQ(resolved->tokenCardinality, 1u);
    EXPECT_EQ(resolved->availabilityLatency,
              llvm::APSInt(latency).getZExtValue());
    auto siblingForwarding = analysis.bindInstance(sibling);
    ASSERT_TRUE(succeeded(siblingForwarding));
    auto siblingBindings = analysis.bindInstance(inner, *siblingForwarding);
    ASSERT_TRUE(succeeded(siblingBindings));
    EXPECT_EQ(siblingBindings->owner, generic.getOperation());
    auto siblingConfig =
        analysis.resolveQueue(onlyQueue(generic), *siblingBindings);
    ASSERT_TRUE(succeeded(siblingConfig));
    EXPECT_EQ(siblingConfig->availabilityLatency, 9u);
    EXPECT_EQ(siblingConfig->timestampWidth, 4u);
    EXPECT_EQ(siblingConfig->timingStorageBitCount, 20u);
    EXPECT_EQ(print(*body), before);
    auto final = finalForEmission(*body, "Root");
    ASSERT_TRUE(final);
    ac::HardwareAnalysis finalAnalysis(*final);
    compiler::HardwareEmitContext emitter(*final, finalAnalysis);
    if (latency == "18446744073709551615")
      rejects([&] { return emitter.prepare(); },
              "emitted signed 64-bit domain");
    else
      EXPECT_TRUE(succeeded(emitter.prepare()));
  }
}

TEST_F(QueueContractsTest, LatencyBindingDemandScopeAndCyclesStayAtQueueOwner) {
  auto body = genericBody();
  ASSERT_TRUE(body);
  ac::HardwareAnalysis analysis(*body);
  auto module = cast<ac::ModuleOp>(analysis.lookupDefinition("Generic"));
  auto queue = onlyQueue(module);
  addIntegerFormals(module, {"L", "E"});
  auto latency = formal("Generic", "L");
  setQueueLatency(queue, latency);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  bindings.types["T"] = ac::BitsType::get(&context, integer(13));
  bindings.integers["D"] = exactInteger("3");
  EXPECT_FALSE(analysis.isStaticEvaluable(latency, bindings));
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
  rejects([&] { return analysis.resolveQueue(queue, bindings); }, "unresolved");
  bindings.integers["L"] = formal("Generic", "E");
  bindings.integers["E"] = exactInteger("5");
  auto valid = analysis.resolveQueue(queue, bindings);
  ASSERT_TRUE(succeeded(valid));
  EXPECT_EQ(valid->availabilityLatency, 5u);
  EXPECT_EQ(valid->timestampWidth, 3u);
  EXPECT_EQ(valid->timingStorageBitCount, 16u);
  bindings.integers["E"] = formal("Generic", "L");
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings, false); },
          "cyclic");
  rejects([&] { return analysis.resolveQueue(queue, bindings); }, "cyclic");
  bindings.integers["L"] = formal("Forward", "N");
  bindings.integers.erase("E");
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
  rejects([&] { return analysis.resolveQueue(queue, bindings); }, "unresolved");
  // A forwarded actual may be unresolved, but original queue syntax cannot
  // name a foreign formal, even in an unselected branch.
  for (bool selected : {false, true}) {
    auto foreign = formal("Forward", "N");
    setQueueLatency(queue, selected ? select(boolean(true), integer(3), foreign)
                                    : foreign);
    rejects(
        [&] { return analysis.verifyQueueOperation(queue, bindings, false); },
        "formal of its owning module");
  }
  setQueueLatency(queue, latency);
  bindings.integers["L"] = exactInteger("3");
  bindings.integers.erase("D");
  EXPECT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings, false)));
  rejects([&] { return analysis.resolveQueue(queue, bindings); }, "unresolved");
  bindings.owner = cast<ac::ModuleOp>(analysis.lookupDefinition("Forward"));
  rejects([&] { return analysis.resolveQueue(queue, bindings); },
          "owning module");
}

TEST_F(QueueContractsTest, RuleAndMapPlacementRejectAtQueueOwner) {
  for (bool mapped : {false, true}) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto original = onlyQueue(module);
    OpBuilder b(original);
    OperationState region(original.getLoc(),
                          mapped ? "ac.table.map" : "ac.comb");
    auto inputTypes = module.getFunctionType().getInputs();
    SmallVector<Type> blockTypes;
    if (mapped) {
      auto shape = b.getArrayAttr({integer(1)});
      auto table = ac::TableType::get(&context, shape, inputTypes[1]);
      OperationState splat(original.getLoc(), "ac.table.splat");
      splat.addOperands(module.getBody().front().getArgument(1));
      splat.addTypes(table);
      splat.addAttribute("shape", shape);
      auto data = b.create(splat)->getResult(0);
      region.addOperands(data);
      for (unsigned i : {0u, 2u, 3u, 4u})
        region.addOperands(module.getBody().front().getArgument(i));
      region.addTypes(table);
      region.addAttribute("shape", shape);
      region.addAttribute("operandSegmentSizes",
                          b.getDenseI32ArrayAttr({1, 4}));
      blockTypes = {inputTypes[0], inputTypes[1], inputTypes[0],
                    inputTypes[2], inputTypes[3], inputTypes[4]};
    } else {
      region.addOperands(module.getBody().front().getArguments());
      region.addTypes(module.getFunctionType().getResults());
      region.addAttribute("name", b.getStringAttr("nested"));
      region.addAttribute("occurrence", occurrence("Provider"));
      llvm::append_range(blockTypes, inputTypes);
    }
    region.addRegion();
    auto parent = b.create(region);
    auto block = new Block();
    parent->getRegion(0).push_back(block);
    for (Type type : blockTypes)
      block->addArgument(type, original.getLoc());
    b.setInsertionPointToEnd(block);
    auto nested = cast<ac::InstanceOp>(b.insert(original->clone()));
    if (mapped)
      nested->setOperands({block->getArgument(4), block->getArgument(5),
                           block->getArgument(2), block->getArgument(1),
                           block->getArgument(3)});
    else
      nested->setOperands({block->getArgument(3), block->getArgument(4),
                           block->getArgument(0), block->getArgument(1),
                           block->getArgument(2)});
    OperationState yield(original.getLoc(), "ac.yield");
    if (mapped)
      yield.addOperands(nested.getOutputs()[2]);
    else
      yield.addOperands(nested->getResults());
    b.create(yield);
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(nested, bound); },
            "requires direct module placement");
  }
}

TEST_F(QueueContractsTest, QueueInstanceAndCollectionNamesShareOneNamespace) {
  for (unsigned kind = 0; kind != 3; ++kind) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    Builder attrs(&context);
    Operation *other = nullptr;
    if (kind == 0) {
      auto copy = queue->clone();
      copy->setAttr("instance_name", attrs.getStringAttr("other"));
      OpBuilder insertion(queue);
      other = insertion.insert(copy);
    } else {
      auto leaf = cast<ac::ModuleOp>(module->clone());
      leaf->setAttr("sym_name", attrs.getStringAttr("Leaf"));
      auto leafQueue = onlyQueue(leaf);
      auto term = cast<ac::YieldOp>(leaf.getBody().front().back());
      term->setOperands({leaf.getBody().front().getArgument(2),
                         leaf.getBody().front().getArgument(0),
                         leaf.getBody().front().getArgument(1)});
      leafQueue.erase();
      body->getBody()->push_back(leaf);
      OpBuilder b(queue);
      OperationState state(queue.getLoc(),
                           kind == 1 ? "ac.instance" : "ac.collection");
      auto shape = b.getArrayAttr({integer(1)});
      if (kind == 1) {
        state.addOperands(module.getBody().front().getArguments());
        state.addTypes(module.getFunctionType().getResults());
      } else {
        for (Value arg : module.getBody().front().getArguments()) {
          auto table = ac::TableType::get(&context, shape, arg.getType());
          OperationState splat(queue.getLoc(), "ac.table.splat");
          splat.addOperands(arg);
          splat.addTypes(table);
          splat.addAttribute("shape", shape);
          state.addOperands(b.create(splat)->getResults());
        }
        for (Type result : module.getFunctionType().getResults())
          state.addTypes(ac::TableType::get(&context, shape, result));
        state.addAttribute("shape", shape);
      }
      state.addAttribute("instance_name", attrs.getStringAttr("other"));
      state.addAttribute("callee", FlatSymbolRefAttr::get(&context, "Leaf"));
      state.addAttribute("parameters", attrs.getArrayAttr({}));
      state.addAttribute("type_arguments", attrs.getArrayAttr({}));
      state.addAttribute("occurrence", occurrence("Provider"));
      other = b.create(state);
      envelope(*body, "__init__.py");
    }
    ASSERT_TRUE(succeeded(mlir::verify(*body)));
    other->setAttr("instance_name", attrs.getStringAttr("fifo"));
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            "instance_name must be unique within parent");
  }
}

TEST_F(QueueContractsTest,
       OccurrenceStaticScopeKindsAndPartialTableGeometryReject) {
  Builder b(&context);
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    auto body = provider(13, 3, "local_occupancy");
    ASSERT_TRUE(body);
    auto module = definition(*body);
    auto queue = onlyQueue(module);
    if (mutation == 0)
      queue->setAttr("occurrence", b.getDictionaryAttr({}));
    if (mutation == 1)
      queue->setAttr("empty_flow", b.getBoolAttr(true));
    if (mutation == 2)
      setQueueDepth(queue, boolean(true));
    if (mutation == 3 || mutation == 4)
      setQueueDepth(
          queue,
          expression(
              "reference",
              {b.getNamedAttr(
                  "ref",
                  b.getDictionaryAttr(
                      {b.getNamedAttr("kind", b.getStringAttr("parameter")),
                       b.getNamedAttr("owner", FlatSymbolRefAttr::get(
                                                   &context, "Foreign")),
                       b.getNamedAttr("name", b.getStringAttr("D"))}))}));
    if (mutation == 4) {
      auto foreign = cast<ac::StaticExprAttr>(queue.getParameters()[0]);
      setQueueDepth(queue, select(boolean(true), integer(3), foreign));
    }
    ac::HardwareAnalysis analysis(*body);
    ac::HardwareBindings bound;
    bound.owner = module;
    const StringRef guards[] = {"Occurrence must contain exactly 2 fields",
                                "cannot duplicate retired",
                                "integer formal requires mathematical integer",
                                "formal of its owning module",
                                "formal of its owning module"};
    rejects([&] { return analysis.verifyQueueOperation(queue, bound); },
            guards[mutation]);
  }
  for (StringRef axis : {"0", "-1", "9223372036854775808"}) {
    auto body = genericBody();
    ASSERT_TRUE(body);
    ac::HardwareAnalysis analysis(*body);
    auto module = cast<ac::ModuleOp>(analysis.lookupDefinition("Token3"));
    auto queue = onlyQueue(module);
    auto original = cast<ac::TableType>(queuePayload(queue));
    auto malformed =
        ac::TableType::get(&context, b.getArrayAttr({exactInteger(axis)}),
                           original.getElementType());
    queue.getInputs()[3].setType(malformed);
    queue.getOutputs()[2].setType(malformed);
    SmallVector<Type> inputs;
    for (Value arg : module.getBody().front().getArguments())
      inputs.push_back(arg.getType());
    SmallVector<Type> outputs;
    for (Value result :
         cast<ac::YieldOp>(module.getBody().front().back()).getValues())
      outputs.push_back(result.getType());
    module->setAttr("function_type", TypeAttr::get(FunctionType::get(
                                         &context, inputs, outputs)));
    ac::HardwareBindings bound;
    bound.owner = module;
    rejects([&] { return analysis.verifyQueueOperation(queue, bound, false); },
            axis == "9223372036854775808" ? "signed 64" : "positive");
  }
}


TEST_F(QueueContractsTest, SourceQueueOwnerPreservesIdentityAndRejectsHeaderTamper) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::SourceMethodSummariesAnalysis summaries(*body);
  ASSERT_TRUE(summaries.isValid());
  auto published = summaries.getPublicationSummary(module);
  ASSERT_TRUE(succeeded(published));
  ASSERT_TRUE(succeeded(ac::verifySourceTransactionSummary(module, *published)));
  auto owners = published->getAs<ArrayAttr>("owners");
  ASSERT_EQ(owners.size(), 1u);
  auto row = cast<DictionaryAttr>(owners[0]);
  EXPECT_EQ(row.getAs<StringAttr>("kind").getValue(), "instance");
  EXPECT_EQ(row.getAs<DictionaryAttr>("occurrence"), queue.getOccurrence());
  EXPECT_EQ(row.getAs<FlatSymbolRefAttr>("callee"), queue.getCalleeAttr());
  EXPECT_EQ(row.getAs<ArrayAttr>("parameters"), queue.getParameters());
  EXPECT_EQ(row.getAs<ArrayAttr>("type_arguments"), queue.getTypeArguments());
  EXPECT_EQ(cast<TypeAttr>(row.getAs<ArrayAttr>("result_types")[2]).getValue(),
            queue.getOutputs()[2].getType());
  ASSERT_EQ(row.getAs<ArrayAttr>("arguments").size(), 5u);
  auto resolved = ac::lookupSourceQueue(module, "fifo", queue);
  ASSERT_TRUE(succeeded(resolved));
  EXPECT_EQ(resolved->allocation, queue);
  EXPECT_FALSE(resolved->isPushUnbound());
  EXPECT_FALSE(resolved->isPopUnbound());
  Builder b(&context);
  NamedAttrList damaged(row);
  damaged.set("parameters",
              b.getArrayAttr({queue.getParameters()[0], exactInteger("0")}));
  NamedAttrList summary(*published);
  summary.set("owners", b.getArrayAttr({damaged.getDictionary(&context)}));
  rejects([&] { return ac::verifySourceTransactionSummary(
                         module, summary.getDictionary(&context)); },
          "availability_latency must be positive");
  damaged = NamedAttrList(row);
  damaged.set("unbound_pop", b.getBoolAttr(true));
  summary.set("owners", b.getArrayAttr({damaged.getDictionary(&context)}));
  rejects([&] { return ac::verifySourceTransactionSummary(
                         module, summary.getDictionary(&context)); },
          "payload constraint");
}

TEST_F(QueueContractsTest, SourceQueueDirectionsRequireZeroPinsAndLogicalContract) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  ac::HardwareAnalysis analysis(*body);
  ac::HardwareBindings bindings;
  bindings.owner = module;
  Builder b(&context);
  queue->setAttr("ac.source_unbound_pop", b.getBoolAttr(false));
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings); },
          "requires BoolAttr true");
  queue->setAttr("ac.source_unbound_pop", b.getBoolAttr(true));
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings); },
          "requires a zero placeholder");
  OpBuilder at(queue);
  OperationState zero(queue.getLoc(), ac::BitsConstantOp::getOperationName());
  zero.addTypes(queue.getInputs()[4].getType());
  zero.addAttribute("value", exactInteger("0"));
  queue->setOperand(4, at.create(zero)->getResult(0));
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings); },
          "declared payload constraint");
  queue->setAttr("ac.payload_constraint", b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("hardware")),
      b.getNamedAttr("type", TypeAttr::get(queue.getOutputs()[2].getType())),
      b.getNamedAttr("source_kind", b.getStringAttr("fixed_bits"))}));
  ASSERT_TRUE(succeeded(analysis.verifyQueueOperation(queue, bindings)));
  auto resolved = ac::lookupSourceQueue(module, "fifo", queue);
  ASSERT_TRUE(succeeded(resolved));
  EXPECT_TRUE(resolved->isPopUnbound());
  EXPECT_FALSE(resolved->isPushUnbound());
  auto constraint = queue->getAttrOfType<DictionaryAttr>("ac.payload_constraint");
  NamedAttrList logical(constraint);
  logical.set("source_kind", b.getStringAttr("boolean"));
  queue->setAttr("ac.payload_constraint", logical.getDictionary(&context));
  rejects([&] { return analysis.verifyQueueOperation(queue, bindings); },
          "fields or type scope are invalid");
}

TEST_F(QueueContractsTest, ImportedSourceQueueOwnerUsesOnlyPublishedTypeAuthority) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  Builder b(&context);
  queue->setAttr("ac.payload_constraint", b.getDictionaryAttr({
      b.getNamedAttr("kind", b.getStringAttr("hardware")),
      b.getNamedAttr("type", TypeAttr::get(queue.getOutputs()[2].getType())),
      b.getNamedAttr("source_kind", b.getStringAttr("fixed_bits"))}));
  auto header = clone(*body);
  ASSERT_TRUE(succeeded(extract(*header)));
  auto imported = declaration(*header);
  ASSERT_TRUE(imported.getTransactionSummaryAttr());
  auto resolved = ac::lookupSourceQueue(imported, "fifo", imported);
  ASSERT_TRUE(succeeded(resolved));
  EXPECT_FALSE(resolved->allocation);
  EXPECT_EQ(resolved->getPayloadType(), queue.getOutputs()[2].getType());
  EXPECT_EQ(resolved->getOccurrence(), queue.getOccurrence());
  EXPECT_EQ(resolved->getPayloadConstraint(), queue->getAttr("ac.payload_constraint"));
  // The header package has no provider body; changing the unrelated original
  // cannot become an alternate authority for imported queue types.
  setQueueDepth(queue, exactInteger("0"));
  EXPECT_TRUE(succeeded(ac::lookupSourceQueue(imported, "fifo", imported)));
}

TEST_F(QueueContractsTest,
       ImportedQueueOwnerRejectsMalformedTypesAndUnboundFactsWithoutCrash) {
  auto body = provider(13, 3, "downstream_pop");
  ASSERT_TRUE(body);
  auto queue = onlyQueue(definition(*body));
  Builder sourceBuilder(&context);
  queue->setAttr("ac.payload_constraint", sourceBuilder.getDictionaryAttr({
      sourceBuilder.getNamedAttr("kind",
                                 sourceBuilder.getStringAttr("hardware")),
      sourceBuilder.getNamedAttr(
          "type", TypeAttr::get(queue.getOutputs()[2].getType())),
      sourceBuilder.getNamedAttr(
          "source_kind", sourceBuilder.getStringAttr("fixed_bits"))}));
  auto header = clone(*body);
  ASSERT_TRUE(succeeded(extract(*header)));
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    auto damaged = clone(*header);
    auto imported = declaration(*damaged);
    auto summary = imported.getTransactionSummaryAttr();
    ASSERT_TRUE(summary);
    auto owners = summary.getAs<ArrayAttr>("owners");
    ASSERT_EQ(owners.size(), 1u);
    NamedAttrList row(cast<DictionaryAttr>(owners[0]));
    Builder b(&context);
    if (mutation == 0) {
      auto resultTypes = cast<ArrayAttr>(row.get("result_types"));
      SmallVector<Attribute> results(resultTypes.begin(), resultTypes.end());
      results[2] = b.getStringAttr("not-a-type");
      row.set("result_types", b.getArrayAttr(results));
    } else {
      row.set(mutation == 1 ? "unbound_push" : "unbound_pop",
              b.getStringAttr("not-a-bool"));
    }
    NamedAttrList changed(summary);
    changed.set("owners", b.getArrayAttr({row.getDictionary(&context)}));
    imported->setAttr("transaction_summary",
                      changed.getDictionary(&context));
    rejects(
        [&] {
          return success(
              succeeded(ac::lookupSourceQueue(imported, "fifo", imported)));
        },
        mutation == 0
            ? "transaction summary type must be a hardware TypeAttr"
            : "queue unbound-direction facts require BoolAttr");
  }
}

TEST_F(QueueContractsTest, SourceQueueOwnerRejectsAmbiguousProvenance) {
  auto body = provider(13, 3, "local_occupancy");
  ASSERT_TRUE(body);
  auto module = definition(*body);
  auto queue = onlyQueue(module);
  Builder b(&context);
  auto *copy = queue->clone();
  copy->setAttr("instance_name", b.getStringAttr("other"));
  OpBuilder(queue).insert(copy);
  ASSERT_TRUE(succeeded(mlir::verify(*body)));
  rejects([&] { return success(succeeded(ac::lookupSourceQueue(
                         module, "fifo", queue))); },
          "source queue allocations require distinct occurrence identities");
}

} // namespace

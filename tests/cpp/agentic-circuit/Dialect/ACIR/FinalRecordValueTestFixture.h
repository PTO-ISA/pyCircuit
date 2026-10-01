#pragma once
#include "Compiler/FinalProgram.h"
#include "Dialect/ACIR/ACIRFinalContracts.h"
#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace testing {

using namespace mlir;

class FinalRecordValueContractsTest : public ::testing::Test {
protected:
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> path(directory);
    llvm::sys::path::append(path, name);
    return path.str().str();
  }

  auto emitError(MLIRContext &ctx) {
    return [&]() -> InFlightDiagnostic {
      return mlir::emitError(UnknownLoc::get(&ctx));
    };
  }

  void SetUp() override {
    registry.insert<ac::ACIRDialect, arith::ArithDialect, func::FuncDialect>();
    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "final-record-value-contracts", directory));
    compileAndMaterialize();
  }

  void TearDown() override {
    if (!directory.empty())
      llvm::sys::fs::remove_directories(directory);
  }

  void compileAndMaterialize() {
    static constexpr llvm::StringLiteral script = R"py(
import subprocess
import sys
from pathlib import Path
repo, output, compiler = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
sys.path[:0] = [str(repo / "python/semantic-core/src"),
                str(repo / "python/pycircuit/src"), str(repo)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
root = output / "source"
root.mkdir()
sources = {
    "provider.py": "from typing import Annotated\n"
                  "from pycircuit import module\n"
                  "Word = Annotated[int, range(18446744073709551616)]\n"
                  "class Pair:\n    lo: Word\n    hi: bool\n"
                  "    def __init__(self, lo: Word = 3, hi: bool = False):\n"
                  "        self.lo = lo\n        self.hi = hi\n"
                  "class Twin:\n    left: Word\n    right: Word\n"
                  "    def __init__(self, left: Word = 3, right: Word = 17):\n"
                  "        self.left = left\n        self.right = right\n"
                  "@module\n"
                  "def Provider():\n    provider_state: Word = 1\n",
    "empty.py": '"""Empty declaration source."""\n',
    "root.py": "from .provider import Pair, Twin, Provider, Word\n"
               "from pycircuit import module, rule\n"
               "@module\ndef Root():\n    state: Word = 1\n"
               "    child = Provider()\n"
               "    @rule\n    def transfer():\n"
               "        nonlocal state\n        state = state\n"
               "        return\n    transfer()\n",
}
interfaces = {}
for relative, text in sources.items():
    source = root / relative
    source.write_text(text, encoding="utf-8")
    capture = _capture_source_file(source, source_root=root)
    stem = source.stem
    transport = output / (stem + ".transport.mlir")
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [compiler, "--capture", str(transport), "--package", "decl",
               "--path", relative]
    if relative == "root.py":
        command += ["--header", interfaces["provider.py"]]
    command += ["--body-out", str(output / (stem + ".ac")),
                "--interface-out", str(output / (stem + ".interface.ac"))]
    subprocess.run(command, check=True)
    interfaces[relative] = str(output / (stem + ".interface.ac"))
)py";
    std::vector<std::string> arguments = {
        ACIR_TEST_PYTHON,      "-c",
        script.str(),          ACIR_TEST_REPO_ROOT,
        directory.str().str(), ACIR_TEST_SOURCE_UNIT_HARNESS};
    llvm::SmallVector<llvm::StringRef> refs;
    for (const std::string &argument : arguments)
      refs.push_back(argument);
    const std::string log = child("source.log");
    const std::array<std::optional<llvm::StringRef>, 3> redirects = {
        std::nullopt, log, log};
    int status = llvm::sys::ExecuteAndWait(ACIR_TEST_PYTHON, refs, std::nullopt,
                                           redirects);
    auto capturedLog = llvm::MemoryBuffer::getFile(log);
    ASSERT_EQ(status, 0) << (capturedLog ? capturedLog.get()->getBuffer().str()
                                         : log);
    auto load = [&](llvm::StringRef file) {
      return parseSourceFile<ModuleOp>(child(file), &context);
    };
    auto root = load("root.ac"), rootHeader = load("root.interface.ac");
    auto provider = load("provider.ac"),
         providerHeader = load("provider.interface.ac");
    auto empty = load("empty.ac"), emptyHeader = load("empty.interface.ac");
    ASSERT_TRUE(root && rootHeader && provider && providerHeader && empty &&
                emptyHeader);
    llvm::SmallVector<SourceLinkUnit> units = {{*provider, *providerHeader},
                                               {*empty, *emptyHeader},
                                               {*root, *rootHeader}};
    auto analysis = buildFinalProgram(units, emitError(context));
    ASSERT_TRUE(succeeded(analysis));
    auto ready =
        materializeFinalProgram(std::move(*analysis), emitError(context));
    ASSERT_TRUE(succeeded(ready));
    package = ready->hardware().clone();
    ASSERT_TRUE(package);
    Builder builder(&context);
    package->walk([&](ac::StructOp record) {
      if (record.getSymName() != "decl.provider.Pair" &&
          record.getSymName() != "decl.provider.Twin")
        return;
      auto fields = record.getFields();
      auto first = cast<DictionaryAttr>(fields[0]);
      auto logical = cast<DictionaryAttr>(first.get("type"));
      logical =
          replace(logical, "lower",
                  ac::MathIntAttr::get(&context, llvm::APSInt("1")), builder);
      first = replace(first, "type", logical, builder);
      SmallVector<Attribute> updated(fields.begin(), fields.end());
      updated[0] = first;
      record->setAttr("fields", builder.getArrayAttr(updated));
    });
    ASSERT_TRUE(succeeded(mlir::verify(*package)));
    ASSERT_TRUE(succeeded(ac::verifyFinalHardware(*package)));
    baseline = OwningOpRef<ModuleOp>(cast<ModuleOp>(package->clone()));
  }

  ac::RuleOp transfer() {
    ac::RuleOp found;
    package->walk([&](ac::RuleOp candidate) {
      if (candidate.getName() == "transfer")
        found = candidate;
    });
    return found;
  }

  ac::RegOp state() {
    ac::RegOp found;
    package->walk([&](ac::RegOp candidate) {
      if (candidate.getName() == "state")
        found = candidate;
    });
    return found;
  }

  DictionaryAttr replace(DictionaryAttr source, llvm::StringRef name,
                         Attribute value, Builder &builder) {
    SmallVector<NamedAttribute> attrs(source.begin(), source.end());
    for (NamedAttribute &attribute : attrs)
      if (attribute.getName() == name) {
        attribute = builder.getNamedAttr(name, value);
        return builder.getDictionaryAttr(attrs);
      }
    ADD_FAILURE() << "missing attribute " << name.str();
    return source;
  }

  DictionaryAttr occurrence(Builder &builder, llvm::StringRef definition,
                            unsigned index) {
    auto pathElement = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("index")),
         builder.getNamedAttr("value", builder.getI64IntegerAttr(index))});
    auto fieldElement = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("field")),
         builder.getNamedAttr("name", builder.getStringAttr("body"))});
    auto site = builder.getDictionaryAttr(
        {builder.getNamedAttr("definition",
                              FlatSymbolRefAttr::get(&context, definition)),
         builder.getNamedAttr(
             "ast_path", builder.getArrayAttr({fieldElement, pathElement}))});
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("site", site),
         builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
  }

  DictionaryAttr valueID(Builder &builder, unsigned index) {
    auto rootModule = transfer()->getParentOfType<ac::ModuleOp>();
    auto symbol = rootModule->getAttrOfType<StringAttr>("sym_name").getValue();
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("origin", occurrence(builder, symbol, index)),
         builder.getNamedAttr("slot", builder.getI32IntegerAttr(0))});
  }

  void installRecordPacket(bool homogeneousIntegers = false) {
    ac::RuleOp rule = transfer();
    ac::RegOp reg = state();
    ASSERT_TRUE(rule && reg);
    Builder builder(&context);
    OpBuilder ops(&context);
    Block &body = rule.getBody().front();
    llvm::StringRef recordName =
        homogeneousIntegers ? "decl.provider.Twin" : "decl.provider.Pair";
    llvm::StringRef firstName = homogeneousIntegers ? "left" : "lo";
    llvm::StringRef secondName = homogeneousIntegers ? "right" : "hi";
    auto providerSymbol = FlatSymbolRefAttr::get(&context, recordName);
    auto recordType =
        ac::StructType::get(&context, builder.getStringAttr(recordName));
    auto recordLogical = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("record")),
         builder.getNamedAttr("symbol", providerSymbol)});

    reg.getResult().setType(ac::RegType::get(&context, recordType));
    reg->setAttr("ac.logical_type", recordLogical);
    rule->setAttr("ac.input_types", builder.getArrayAttr({recordLogical}));
    rule->setAttr("ac.output_types", builder.getArrayAttr({recordLogical}));
    body.getArgument(0).setType(recordType);
    ASSERT_EQ(rule.getNextValues().size(), 2u);
    rule.getNextValues()[0].setType(recordType);

    auto requiredUses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
    ASSERT_EQ(requiredUses.size(), 1u);
    auto requiredUse = cast<DictionaryAttr>(requiredUses[0]);
    auto useId = requiredUse.getAs<DictionaryAttr>("id");
    ASSERT_TRUE(useId && requiredUse.getAs<DictionaryAttr>("target"));

    auto yield = cast<ac::YieldOp>(body.getTerminator());
    yield->dropAllReferences();
    for (Operation &operation : body) {
      if (isa<ac::YieldOp>(operation))
        continue;
      operation.dropAllReferences();
    }
    for (Operation &operation : llvm::make_early_inc_range(body)) {
      if (isa<ac::YieldOp>(operation))
        continue;
      operation.erase();
    }
    ops.setInsertionPoint(yield);
    Location loc = rule.getLoc();
    auto trueValue = arith::ConstantOp::create(ops, loc, builder.getI1Type(),
                                               builder.getBoolAttr(true));
    auto recordID = valueID(builder, 20);
    auto loID = valueID(builder, 21);
    auto hiID = valueID(builder, 22);
    auto createID = valueID(builder, 23);
    auto origin = [&](DictionaryAttr id) {
      return id.getAs<DictionaryAttr>("origin");
    };
    auto sourceGet = [&](llvm::StringRef field, Type resultType,
                         DictionaryAttr id) {
      OperationState state(loc, ac::StructGetOp::getOperationName());
      state.addOperands(body.getArgument(0));
      state.addTypes(resultType);
      state.addAttribute("field", builder.getStringAttr(field));
      state.addAttribute("ac.origin", origin(id));
      return cast<ac::StructGetOp>(ops.create(state));
    };
    Type firstType = builder.getI64Type();
    Type secondType = homogeneousIntegers ? Type(builder.getI64Type())
                                          : Type(builder.getI1Type());
    auto lo = sourceGet(firstName, firstType, loID);
    auto hi = sourceGet(secondName, secondType, hiID);
    auto bind = [&](Value value, DictionaryAttr id, DictionaryAttr domain,
                    Value valid = Value(), Value path = Value()) {
      if (!valid)
        valid = trueValue;
      if (!path)
        path = trueValue;
      OperationState state(loc, ac::ValueBindingOp::getOperationName());
      state.addOperands({value, valid, path});
      state.addAttribute("id", id);
      state.addAttribute("domain", domain);
      return cast<ac::ValueBindingOp>(ops.create(state));
    };
    bind(body.getArgument(0), recordID, recordLogical);
    auto recordDecl = [&]() {
      ac::StructOp found;
      package->walk([&](ac::StructOp candidate) {
        if (candidate.getSymName() == recordName)
          found = candidate;
      });
      return found;
    }();
    ASSERT_TRUE(recordDecl);
    auto loType = cast<DictionaryAttr>(
        cast<DictionaryAttr>(recordDecl.getFields()[0]).get("type"));
    auto hiType = cast<DictionaryAttr>(
        cast<DictionaryAttr>(recordDecl.getFields()[1]).get("type"));
    bind(lo.getResult(), loID, loType);
    bind(hi.getResult(), hiID, hiType);
    OperationState createState(loc, ac::StructCreateOp::getOperationName());
    createState.addOperands({lo.getResult(), hi.getResult()});
    createState.addTypes(recordType);
    createState.addAttribute("ac.origin", origin(createID));
    auto created = cast<ac::StructCreateOp>(ops.create(createState));
    auto aggregateValid = arith::AndIOp::create(ops, loc, trueValue, trueValue);
    auto createdBinding =
        bind(created.getResult(), createID, recordLogical, aggregateValid);

    requiredUse = replace(requiredUse, "value", createID, builder);
    rule->setAttr("ac.required_uses", builder.getArrayAttr({requiredUse}));
    SmallVector<NamedAttribute> useAttrs = {
        builder.getNamedAttr("id", useId),
        builder.getNamedAttr("source", createID)};
    OperationState useState(loc, ac::ValueUseOp::getOperationName());
    useState.addOperands({created.getResult(), aggregateValid, trueValue});
    useState.addAttributes(useAttrs);
    auto use = cast<ac::ValueUseOp>(ops.create(useState));
    auto enable =
        arith::AndIOp::create(ops, loc, use.getPath(), use.getValid());

    auto makeRequired = [&](llvm::StringRef kind, DictionaryAttr id) {
      SmallVector<NamedAttribute> attrs = {
          builder.getNamedAttr("kind", builder.getStringAttr(kind)),
          builder.getNamedAttr("id", id),
          builder.getNamedAttr("record", providerSymbol)};
      return attrs;
    };
    auto read = makeRequired("read", recordID);
    read.push_back(builder.getNamedAttr(
        "state", cast<DictionaryAttr>(
                     rule->getAttrOfType<ArrayAttr>("ac.input_bindings")[0])));
    auto getLo = makeRequired("get", loID);
    getLo.push_back(builder.getNamedAttr("base", recordID));
    getLo.push_back(
        builder.getNamedAttr("field", builder.getI32IntegerAttr(0)));
    auto getHi = makeRequired("get", hiID);
    getHi.push_back(builder.getNamedAttr("base", recordID));
    getHi.push_back(
        builder.getNamedAttr("field", builder.getI32IntegerAttr(1)));
    auto create = makeRequired("create", createID);
    create.push_back(
        builder.getNamedAttr("fields", builder.getArrayAttr({loID, hiID})));
    rule->setAttr("ac.required_records",
                  builder.getArrayAttr({builder.getDictionaryAttr(read),
                                        builder.getDictionaryAttr(getLo),
                                        builder.getDictionaryAttr(getHi),
                                        builder.getDictionaryAttr(create)}));

    ops.setInsertionPoint(yield);
    auto selectLeaf = [&](Type type, llvm::StringRef field) {
      OperationState state(loc, ac::StructGetOp::getOperationName());
      state.addOperands(created.getResult());
      state.addTypes(type);
      state.addAttribute("field", builder.getStringAttr(field));
      return cast<ac::StructGetOp>(ops.create(state));
    };
    auto selectedLoInput = selectLeaf(builder.getI64Type(), firstName);
    auto selectedHiInput = selectLeaf(secondType, secondName);
    auto zeroLo = arith::ConstantOp::create(
        ops, loc, builder.getI64Type(),
        builder.getIntegerAttr(builder.getI64Type(), 0));
    arith::ConstantOp zeroHi;
    if (homogeneousIntegers)
      zeroHi = arith::ConstantOp::create(ops, loc, secondType,
                                         builder.getIntegerAttr(secondType, 0));
    else
      zeroHi = arith::ConstantOp::create(ops, loc, secondType,
                                         builder.getBoolAttr(false));
    auto selectedLo = arith::SelectOp::create(
        ops, loc, builder.getI64Type(), enable, selectedLoInput.getResult(),
        zeroLo.getResult());
    auto selectedHi = arith::SelectOp::create(ops, loc, secondType, enable,
                                              selectedHiInput.getResult(),
                                              zeroHi.getResult());
    OperationState selectedState(loc, ac::StructCreateOp::getOperationName());
    selectedState.addOperands({selectedLo, selectedHi});
    selectedState.addTypes(recordType);
    auto selected = cast<ac::StructCreateOp>(ops.create(selectedState));
    yield->setOperands({selected.getResult(), enable});
    (void)createdBinding;
  }

  void restoreBaseline() {
    package = OwningOpRef<ModuleOp>(cast<ModuleOp>(baseline->clone()));
  }

  ac::StructCreateOp sourceCreate(ac::RuleOp rule) {
    ac::StructCreateOp result;
    rule.walk([&](ac::StructCreateOp candidate) {
      if (candidate->hasAttr("ac.origin"))
        result = candidate;
    });
    return result;
  }

  ac::StructGetOp sourceGet(ac::RuleOp rule) {
    ac::StructGetOp result;
    rule.walk([&](ac::StructGetOp candidate) {
      if (candidate->hasAttr("ac.origin") && !result)
        result = candidate;
    });
    return result;
  }

  DialectRegistry registry;
  MLIRContext context;
  llvm::SmallString<256> directory;
  OwningOpRef<ModuleOp> package;
  OwningOpRef<ModuleOp> baseline;
};

} // namespace testing
} // namespace acir::compiler

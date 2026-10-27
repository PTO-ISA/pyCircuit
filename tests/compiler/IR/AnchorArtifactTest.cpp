#include "Compiler/AnchorArtifact.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {
namespace ac = acir::ac;
namespace json = llvm::json;
namespace fs = std::filesystem;

// Contract-owned canonical JSON, independent of the artifact implementation.
// LLVM supplies JSON string escaping and SHA256, not manifest construction.
std::string canonical(const json::Value &value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  if (auto object = value.getAsObject()) {
    std::vector<std::string> names;
    for (const auto &field : *object)
      names.push_back(field.first.str());
    std::sort(names.begin(), names.end());
    out << '{';
    bool comma = false;
    for (const auto &name : names) {
      if (comma)
        out << ',';
      comma = true;
      out << json::Value(name) << ':' << canonical(*object->get(name));
    }
    out << '}';
  } else if (auto array = value.getAsArray()) {
    out << '[';
    bool comma = false;
    for (const auto &item : *array) {
      if (comma)
        out << ',';
      comma = true;
      out << canonical(item);
    }
    out << ']';
  } else {
    out << value;
  }
  return text;
}

std::string digest(llvm::StringRef text) {
  return llvm::toHex(llvm::SHA256::hash(llvm::arrayRefFromStringRef(text)),
                     true);
}

std::string read(const fs::path &path) {
  auto buffer = llvm::MemoryBuffer::getFile(path.string());
  EXPECT_TRUE(buffer) << path.string();
  return buffer ? (*buffer)->getBuffer().str() : std::string();
}

void write(const fs::path &path, llvm::StringRef text) {
  fs::create_directories(path.parent_path());
  std::error_code error;
  llvm::raw_fd_ostream out(path.string(), error);
  ASSERT_FALSE(error) << error.message();
  out << text;
}

json::Object manifest(const fs::path &directory) {
  auto value = json::parse(read(directory / "anchor.json"));
  EXPECT_TRUE(static_cast<bool>(value));
  if (!value) {
    llvm::consumeError(value.takeError());
    return {};
  }
  EXPECT_NE(value->getAsObject(), nullptr);
  return value->getAsObject() ? json::Object(*value->getAsObject())
                              : json::Object();
}

void saveManifest(const fs::path &directory, json::Object &object) {
  object.erase("content_identity");
  object["content_identity"] =
      digest(canonical(json::Value(json::Object(object))));
  write(directory / "anchor.json",
        canonical(json::Value(json::Object(object))) + '\n');
}

std::string field(const json::Object &object, llvm::StringRef name) {
  auto value = object.getString(name);
  EXPECT_TRUE(value.has_value()) << name.str();
  return value ? value->str() : std::string();
}

// Re-seal a deliberately modified payload under the specified v1 identity
// rules. This lets negatives reach native spec/closure/binding checks instead
// of merely detecting an obsolete hash. It provides no expected DUT behavior.
void reseal(const fs::path &directory, json::Object &object) {
  std::map<std::string, std::string> files;
  for (const auto &raw : *object.getArray("files")) {
    auto &row = *raw.getAsObject();
    files[field(row, "path")] = read(directory / field(row, "path"));
  }
  json::Array configurationDefinitions;
  auto &configuration = *object.getObject("configuration");
  for (auto &raw : *object.getArray("definitions")) {
    auto &row = *raw.getAsObject();
    auto oldSpec = field(row, "spec"), oldBody = field(row, "body");
    auto specText = files.at(oldSpec), bodyText = files.at(oldBody);
    auto specHash = digest(specText), bodyHash = digest(bodyText);
    auto name = field(row, "definition");
    json::Object identity{{"source", json::Object(*row.getObject("source"))},
                          {"definition", name},
                          {"spec_sha256", specHash},
                          {"body_sha256", bodyHash}};
    auto variant = digest(canonical(json::Value(std::move(identity))));
    auto specPath = "definitions/" + variant + "/spec.ac";
    auto bodyPath = "definitions/" + variant + "/body.ac";
    files.erase(oldSpec);
    files.erase(oldBody);
    files[specPath] = std::move(specText);
    files[bodyPath] = std::move(bodyText);
    row["variant_id"] = variant;
    row["spec"] = specPath;
    row["body"] = bodyPath;
    row["spec_sha256"] = specHash;
    row["body_sha256"] = bodyHash;
    configurationDefinitions.push_back(json::Object{{"definition", name},
                                                    {"variant_id", variant},
                                                    {"spec_sha256", specHash},
                                                    {"body_sha256", bodyHash}});
    if (name == field(object, "root"))
      (*configuration.getObject("root"))["variant_id"] = variant;
  }
  configuration["definitions"] = std::move(configurationDefinitions);
  configuration["graph_sha256"] = digest(files.at("graph.ac"));
  object["configuration_identity"] =
      digest(canonical(json::Value(json::Object(configuration))));
  json::Array inventory;
  for (const auto &[path, text] : files) {
    std::string role = path == "graph.ac" ? "graph"
                       : llvm::StringRef(path).starts_with("sources/")
                           ? "source-interface"
                       : llvm::StringRef(path).ends_with("/spec.ac") ? "spec"
                                                                     : "body";
    inventory.push_back(
        json::Object{{"path", path}, {"role", role}, {"sha256", digest(text)}});
  }
  object["files"] = std::move(inventory);
  fs::remove_all(directory);
  fs::create_directories(directory);
  for (const auto &[path, text] : files)
    write(directory / path, text);
  saveManifest(directory, object);
}

const char *sourceIR = R"ir(
#owner = {package = "δοκιμή", path = "模块.py"}
#child = {site = {definition = @"δοκιμή.模块.Child", ast_path = []}, expansion = []}
#root = {site = {definition = @"δοκιμή.模块.Root", ast_path = []}, expansion = []}
#left = {site = {definition = @"δοκιμή.模块.Root", ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#right = {site = {definition = @"δοκιμή.模块.Root", ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#unused = {site = {definition = @"δοκιμή.模块.Unused", ast_path = []}, expansion = []}
#span = {path = "模块.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = #child, location = #span}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = #child, location = #span}>
#w7 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<7>}, origin = #child, location = #span}>
!b1 = !ac.bits<#w1>
!b7 = !ac.bits<#w7>
module attributes {ac.stage = "source", ac.unit_kind = "implementation", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {
  "ac.module"() ({
  ^bb0(%clock: !b1, %reset: !b1):
    %initial = "ac.bits.constant"() {value = #w2} : () -> !b7
    %q = "ac.state"(%clock, %reset, %initial) {instance_name = "held", occurrence = #child} : (!b1, !b1, !b7) -> !b7
    "ac.yield"(%q) : (!b7) -> ()
  }) {sym_name = "δοκιμή.模块.Child", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> !b7, input_names = ["pyc_clk", "pyc_rst"], output_names = ["q"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.origin = #child, ac.declaration_role = "definition"} : () -> ()
  "ac.module"() ({
  ^bb0(%clock: !b1, %reset: !b1):
    %left = "ac.instance"(%clock, %reset) {instance_name = "left", callee = @"δοκιμή.模块.Child", parameters = [], type_arguments = [], occurrence = #left} : (!b1, !b1) -> !b7
    %right = "ac.instance"(%clock, %reset) {instance_name = "right", callee = @"δοκιμή.模块.Child", parameters = [], type_arguments = [], occurrence = #right} : (!b1, !b1) -> !b7
    "ac.yield"(%left, %right) : (!b7, !b7) -> ()
  }) {sym_name = "δοκιμή.模块.Root", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b1, !b1) -> (!b7, !b7), input_names = ["pyc_clk", "pyc_rst"], output_names = ["left_value", "right_value"], ac.domain_inputs = {clock = 0 : i64, reset = 1 : i64}, ac.origin = #root, ac.declaration_role = "definition"} : () -> ()
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "δοκιμή.模块.Unused", source_owner = #owner, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], ac.origin = #unused, ac.declaration_role = "definition"} : () -> ()
}
)ir";

std::string collectionSource(uint64_t lanes) {
  // A native Collection fixture, not a claim about Python array authoring.
  // The callee is stateless so verification need not expand stateful lanes.
  return R"ir(
#owner = {package = "δοκιμή", path = "模块.py"}
#child = {site = {definition = @"δοκιμή.模块.Child", ast_path = []}, expansion = []}
#root = {site = {definition = @"δοκιμή.模块.Root", ast_path = []}, expansion = []}
#collection = {site = {definition = @"δοκιμή.模块.Root", ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#span = {path = "模块.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#w7 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<7>}, origin = #root, location = #span}>
#extent = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<)ir" +
         std::to_string(lanes) + R"ir(>}, origin = #root, location = #span}>
!b7 = !ac.bits<#w7>
!lanes = !ac.table<[#extent], !b7>
module attributes {ac.stage = "source", ac.unit_kind = "implementation", ac.source_owner = #owner, ac.interfaces = [#owner], ac.exports = [], ac.import_bindings = []} {
  "ac.module"() ({
  ^bb0(%value: !b7):
    "ac.yield"(%value) : (!b7) -> ()
  }) {sym_name = "δοκιμή.模块.Child", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!b7) -> !b7, input_names = ["value"], output_names = ["forwarded"], ac.origin = #child, ac.declaration_role = "definition"} : () -> ()
  "ac.module"() ({
  ^bb0(%values: !lanes):
    %result = "ac.collection"(%values) {instance_name = "family", callee = @"δοκιμή.模块.Child", parameters = [], type_arguments = [], shape = [#extent], occurrence = #collection} : (!lanes) -> !lanes
    "ac.yield"(%result) : (!lanes) -> ()
  }) {sym_name = "δοκιμή.模块.Root", source_owner = #owner, parameters = [], type_parameters = [], function_type = (!lanes) -> !lanes, input_names = ["values"], output_names = ["forwarded_values"], ac.origin = #root, ac.declaration_role = "definition"} : () -> ()
}
)ir";
}

class AnchorArtifactTest : public ::testing::Test {
protected:
  AnchorArtifactTest() { context.loadDialect<ac::ACIRDialect>(); }
  ~AnchorArtifactTest() override {
    if (!temporary.empty())
      fs::remove_all(temporary);
  }
  void SetUp() override {
    llvm::SmallString<256> path;
    ASSERT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-anchor-native", path));
    // macOS's temporary-directory spelling may traverse /var -> /private/var.
    // The native loader correctly rejects symlink ancestors, so the fixture
    // supplies its actual ordinary-directory path rather than that alias.
    temporary = fs::canonical(path.str().str());
  }

  struct Inputs {
    mlir::OwningOpRef<mlir::ModuleOp> body, header, routed;
    std::string originalBody, originalHeader;
  };
  mlir::MLIRContext context;
  fs::path temporary;
  std::string diagnostics;

  auto error() {
    return [&] { return mlir::emitError(mlir::UnknownLoc::get(&context)); };
  }
  template <typename Function> auto capture(Function function) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream out(diagnostics);
      d.print(out);
      return mlir::success();
    });
    return function();
  }
  std::string print(mlir::Operation *operation) {
    std::string text;
    llvm::raw_string_ostream out(text);
    operation->print(out, mlir::OpPrintingFlags().enableDebugInfo());
    return text + '\n';
  }
  std::string print(mlir::Attribute attribute) {
    std::string text;
    llvm::raw_string_ostream out(text);
    out << attribute;
    return text + '\n';
  }

  std::unique_ptr<Inputs> inputs(unsigned initial = 2,
                                 llvm::StringRef source = sourceIR) {
    auto result = std::make_unique<Inputs>();
    std::string text(source);
    if (initial != 2) {
      auto literal = text.find("#ac.math_int<2>");
      EXPECT_NE(literal, std::string::npos);
      text.replace(literal, std::string("#ac.math_int<2>").size(),
                   "#ac.math_int<" + std::to_string(initial) + ">");
    }
    result->body = mlir::parseSourceString<mlir::ModuleOp>(text, &context);
    if (!result->body)
      return {};
    auto builtins = capture(
        [&] { return SourceHeaderRegistry::create(&context, {}, error()); });
    if (mlir::failed(builtins)) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    auto primitive = builtins->lookupBuiltin("dffe");
    if (!primitive) {
      ADD_FAILURE() << "missing canonical DFFE";
      return {};
    }
    auto *imported = primitive.declaration->clone();
    imported->setAttr("ac.declaration_role",
                      mlir::StringAttr::get(&context, "import_snapshot"));
    result->body->getBody()->push_front(imported);
    result->header =
        mlir::cast<mlir::ModuleOp>(result->body->getOperation()->clone());
    auto extracted = capture([&] {
      mlir::PassManager manager(&context);
      manager.addPass(acir::createExtractSourceInterfacePass());
      return manager.run(*result->header);
    });
    if (mlir::failed(extracted)) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    result->originalBody = print(result->body->getOperation());
    result->originalHeader = print(result->header->getOperation());
    auto routed = capture([&] {
      return linkSourceUnits({{*result->body, *result->header}},
                             "δοκιμή.模块.Root", error(),
                             SourceLinkTarget::Routed);
    });
    if (mlir::failed(routed)) {
      ADD_FAILURE() << diagnostics;
      return {};
    }
    result->routed = std::move(*routed);
    return result;
  }

  fs::path publish(Inputs &input, llvm::StringRef name = "anchor") {
    auto directory = temporary / name.str();
    fs::create_directory(directory);
    AnchorSourceProvenance source{{*input.body, *input.header},
                                  digest(input.originalBody),
                                  input.originalHeader};
    auto status = capture([&] {
      return writeAnchorArtifacts(*input.routed, {source}, directory.string(),
                                  error());
    });
    EXPECT_TRUE(mlir::succeeded(status)) << diagnostics;
    auto baseline = load(directory);
    EXPECT_TRUE(mlir::succeeded(baseline)) << diagnostics;
    return directory;
  }
  mlir::FailureOr<LoadedAnchor> load(const fs::path &directory) {
    return capture([&] {
      return loadVerifiedAnchor(directory.string(), context, error());
    });
  }
  void reject(const fs::path &directory, llvm::StringRef reason) {
    auto loaded = load(directory);
    EXPECT_TRUE(mlir::failed(loaded));
    EXPECT_NE(diagnostics.find(reason.str()), std::string::npos) << diagnostics;
  }
};

TEST_F(AnchorArtifactTest,
       TransactionalResourceStageCannotPublishAnAnchor) {
  auto package = mlir::parseSourceString<mlir::ModuleOp>(
      "module attributes {ac.transactional = 2 : i64, "
      "ac.stage = \"linked\"} {}", &context);
  ASSERT_TRUE(package);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*package)));
  const std::string reason =
      "transactional resource packages cannot enter legacy routed or final "
      "hardware analysis";
  EXPECT_TRUE(mlir::failed(capture(
      [&] { return ac::verifyHardwarePackage(*package); })));
  EXPECT_NE(diagnostics.find(reason), std::string::npos) << diagnostics;
  EXPECT_TRUE(mlir::failed(capture(
      [&] { return ac::verifyRoutedPackage(*package); })));
  EXPECT_NE(diagnostics.find(reason), std::string::npos) << diagnostics;
  auto directory = temporary / "transactional-resource-anchor";
  fs::create_directory(directory);
  EXPECT_TRUE(mlir::failed(capture([&] {
    return writeAnchorArtifacts(*package, {}, directory.string(), error());
  })));
  EXPECT_NE(diagnostics.find(reason), std::string::npos) << diagnostics;
  EXPECT_TRUE(fs::is_empty(directory));
}

TEST_F(AnchorArtifactTest,
       WriteLoadAndExplicitRefineRetainOneSharedDefinition) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto original = print(input->routed->getOperation());
  auto directory = publish(*input);
  auto object = manifest(directory);
  ASSERT_EQ(object.getArray("definitions")->size(), 2u);
  EXPECT_EQ(field(object, "root"), "@\"δοκιμή.模块.Root\"");
  EXPECT_EQ(object.getArray("sources")->size(), 1u);
  EXPECT_EQ(field(*(*object.getArray("sources"))[0].getAsObject(), "owner_id"),
            "8b78f562e4b6abe3e8af1b4d56c4cafbcf424fcc45af02903780612f59418559");
  auto loaded = load(directory);
  ASSERT_TRUE(mlir::succeeded(loaded)) << diagnostics;
  unsigned logical = 0, updates = 0, modules = 0, allocations = 0;
  loaded->package->walk([&](ac::SourceStateOp) { ++logical; });
  loaded->package->walk([&](ac::StateUpdateOp) { ++updates; });
  loaded->package->walk([&](ac::ModuleOp) { ++modules; });
  loaded->package->walk([&](ac::InstanceOp) { ++allocations; });
  EXPECT_EQ(logical, 1u);
  EXPECT_EQ(updates, 1u);
  EXPECT_EQ(modules, 2u);
  EXPECT_EQ(allocations, 2u);
  auto held = print(loaded->package->getOperation());
  auto final = capture([&] { return refineVerifiedAnchor(*loaded, error()); });
  ASSERT_TRUE(mlir::succeeded(final)) << diagnostics;
  EXPECT_TRUE(mlir::succeeded(ac::verifyHardwarePackage(**final)));
  auto provenance =
      (*final)->getOperation()->getAttrOfType<mlir::DictionaryAttr>(
          "ac.anchor_provenance");
  ASSERT_TRUE(provenance);
  EXPECT_EQ(provenance.getAs<mlir::StringAttr>("content_identity").getValue(),
            loaded->contentIdentity);
  EXPECT_EQ(
      provenance.getAs<mlir::StringAttr>("configuration_identity").getValue(),
      loaded->configurationIdentity);
  EXPECT_EQ(print(loaded->package->getOperation()), held);
  EXPECT_EQ(print(input->routed->getOperation()), original);
  EXPECT_EQ(print(input->body->getOperation()), input->originalBody);
  EXPECT_EQ(print(input->header->getOperation()), input->originalHeader);
}

TEST_F(AnchorArtifactTest,
       SavedSpecsMatchBothNativePropertiesAndDiscardableAttributesExactly) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto directory = publish(*input);
  auto object = manifest(directory);
  auto graph = mlir::parseSourceString<mlir::ModuleOp>(
      read(directory / "graph.ac"), mlir::ParserConfig(&context, false));
  ASSERT_TRUE(graph);
  EXPECT_TRUE(graph->getOps<ac::ModuleOp>().empty());
  EXPECT_EQ(std::distance(graph->getOps<ac::ModuleImportOp>().begin(),
                          graph->getOps<ac::ModuleImportOp>().end()),
            1);
  for (const auto &raw : *object.getArray("definitions")) {
    auto &row = *raw.getAsObject();
    auto body = mlir::parseSourceString<mlir::ModuleOp>(
        read(directory / field(row, "body")),
        mlir::ParserConfig(&context, false));
    ASSERT_TRUE(body);
    ASSERT_TRUE(llvm::hasSingleElement(body->getBody()->getOperations()));
    auto definition = mlir::cast<ac::ModuleOp>(body->getBody()->front());
    auto spec = mlir::dyn_cast<mlir::DictionaryAttr>(
        mlir::parseAttribute(read(directory / field(row, "spec")), &context));
    ASSERT_TRUE(spec);
    EXPECT_EQ(spec.size(), 2u);
    EXPECT_EQ(spec.get("properties"), definition->getPropertiesAsAttribute());
    EXPECT_EQ(spec.get("attributes"),
              definition->getDiscardableAttrDictionary());
    auto dependencies = row.getArray("dependencies");
    ASSERT_TRUE(dependencies);
    EXPECT_EQ(dependencies->size(),
              field(row, "definition").ends_with("Root\"") ? 1u : 0u);
  }
}

TEST_F(AnchorArtifactTest,
       ReloadNeedsNoOriginalSourceUnitsOrPreparationObjects) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto directory = publish(*input);
  input.reset();
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect>();
  auto loaded = loadVerifiedAnchor(directory.string(), fresh, [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&fresh));
  });
  ASSERT_TRUE(mlir::succeeded(loaded));
  EXPECT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*loaded->package)));
}

TEST_F(AnchorArtifactTest,
       DeterministicPublicationAndChangedProviderProduceDifferentIdentity) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto first = publish(*input, "first");
  auto second = publish(*input, "second");
  EXPECT_EQ(read(first / "anchor.json"), read(second / "anchor.json"));
  // Change the actual provider initializer while keeping its interface and
  // original source publication intact. No generated value supplies the oracle.
  auto changedInput = inputs(1);
  ASSERT_TRUE(changedInput);
  EXPECT_EQ(changedInput->originalHeader, input->originalHeader);
  auto changed = publish(*changedInput, "changed");
  auto previous = manifest(first), current = manifest(changed);
  EXPECT_NE(field(previous, "content_identity"),
            field(current, "content_identity"));
  EXPECT_NE(field(previous, "configuration_identity"),
            field(current, "configuration_identity"));
  EXPECT_EQ(print(input->body->getOperation()), input->originalBody);
}

TEST_F(AnchorArtifactTest,
       ExactSpecProofRejectsResealedPropertyAndAttributeSubstitutions) {
  for (llvm::StringRef compartment : {"properties", "attributes"}) {
    SCOPED_TRACE(compartment.str());
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto object = manifest(directory);
    auto &row = *object.getArray("definitions")->front().getAsObject();
    auto path = directory / field(row, "spec");
    auto spec = mlir::cast<mlir::DictionaryAttr>(
        mlir::parseAttribute(read(path), &context));
    mlir::NamedAttrList fields(
        mlir::cast<mlir::DictionaryAttr>(spec.get(compartment)));
    fields.set("substituted", mlir::UnitAttr::get(&context));
    mlir::NamedAttrList outer(spec);
    outer.set(compartment, fields.getDictionary(&context));
    write(path, print(outer.getDictionary(&context)));
    reseal(directory, object);
    reject(directory, "exact saved spec mismatch");
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest, PinnedBodiesAndInterfacesRejectByteSubstitution) {
  for (llvm::StringRef target : {"body", "interface", "missing-body"}) {
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto object = manifest(directory);
    auto &row = target != "interface"
                    ? *object.getArray("definitions")->front().getAsObject()
                    : *object.getArray("sources")->front().getAsObject();
    auto path =
        directory / field(row, target == "interface" ? "interface" : "body");
    if (target == "missing-body") {
      fs::remove(path);
      reject(directory, "inventory");
    } else {
      write(path, read(path) + "\n// changed payload\n");
      reject(directory, "digest mismatch");
    }
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest,
       ConfigurationAndRootSubstitutionsFailAfterIdentitiesAreRecomputed) {
  for (llvm::StringRef mutation :
       {"root", "reset", "owner", "parameter", "type"}) {
    SCOPED_TRACE(mutation.str());
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto object = manifest(directory);
    auto &config = *object.getObject("configuration");
    if (mutation == "root")
      (*config.getObject("root"))["definition"] = "@\"δοκιμή.模块.Child\"";
    if (mutation == "reset")
      config["reset_semantics"] = "if_one";
    if (mutation == "owner")
      (*object.getObject("owner")->getObject("source"))["path"] =
          "different.py";
    if (mutation == "parameter")
      config.getArray("integer_arguments")
          ->push_back(json::Object{{"name", "missing"}, {"value", "1"}});
    if (mutation == "type")
      config.getArray("type_arguments")
          ->push_back(json::Object{{"name", "T"}, {"type", "!ac.bits<1>"}});
    object["configuration_identity"] =
        digest(canonical(json::Value(json::Object(config))));
    saveManifest(directory, object);
    auto loaded = load(directory);
    EXPECT_TRUE(mlir::failed(loaded)) << mutation.str();
    EXPECT_FALSE(diagnostics.empty());
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest,
       DependencyAndClosedSchemaMutationsFailWithoutProviderSearch) {
  for (llvm::StringRef mutation :
       {"dependency", "duplicate", "schema", "extra", "path"}) {
    SCOPED_TRACE(mutation.str());
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto object = manifest(directory);
    if (mutation == "dependency")
      object.getArray("definitions")
          ->front()
          .getAsObject()
          ->getArray("dependencies")
          ->push_back("@\"absent.Provider\"");
    if (mutation == "duplicate")
      object.getArray("definitions")
          ->push_back(json::Object(
              *object.getArray("definitions")->front().getAsObject()));
    if (mutation == "schema")
      object["schema"] = 2;
    if (mutation == "extra")
      object["fallback_provider_directory"] = "../providers";
    if (mutation == "path")
      (*object.getArray("files")->front().getAsObject())["path"] =
          "../graph.ac";
    saveManifest(directory, object);
    auto loaded = load(directory);
    EXPECT_TRUE(mlir::failed(loaded));
    EXPECT_FALSE(diagnostics.empty());
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest, MalformedAssembledStateIsRejectedBeforeBinding) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto directory = publish(*input);
  auto object = manifest(directory);
  for (const auto &raw : *object.getArray("definitions")) {
    auto &row = *raw.getAsObject();
    if (!field(row, "definition").ends_with("Child\""))
      continue;
    auto path = directory / field(row, "body");
    auto fragment = mlir::parseSourceString<mlir::ModuleOp>(
        read(path), mlir::ParserConfig(&context, false));
    ASSERT_TRUE(fragment);
    ac::StateUpdateOp update;
    fragment->walk([&](ac::StateUpdateOp op) { update = op; });
    ASSERT_TRUE(update);
    update->setOperands(update.getState());
    write(path, print(&fragment->getBody()->front()));
  }
  reseal(directory, object);
  auto loaded = load(directory);
  EXPECT_TRUE(mlir::failed(loaded));
  EXPECT_FALSE(diagnostics.empty());
}

TEST_F(AnchorArtifactTest,
       MissingAndIllTypedInstanceCalleeRejectBeforeBinding) {
  for (bool missing : {true, false}) {
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto object = manifest(directory);
    for (const auto &raw : *object.getArray("definitions")) {
      auto &row = *raw.getAsObject();
      if (!field(row, "definition").ends_with("Root\""))
        continue;
      auto path = directory / field(row, "body");
      auto fragment = mlir::parseSourceString<mlir::ModuleOp>(
          read(path), mlir::ParserConfig(&context, false));
      ASSERT_TRUE(fragment);
      ac::InstanceOp instance;
      fragment->walk([&](ac::InstanceOp op) { instance = op; });
      ASSERT_TRUE(instance);
      if (missing)
        instance->removeAttr("callee");
      else
        instance->setAttr("callee",
                          mlir::StringAttr::get(&context, "not-a-symbol"));
      write(path, print(&fragment->getBody()->front()));
    }
    reseal(directory, object);
    auto loaded = load(directory);
    EXPECT_TRUE(mlir::failed(loaded));
    EXPECT_FALSE(diagnostics.empty());
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest,
       RefinementRejectsMutationOfPreviouslyVerifiedLoadedPackage) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto directory = publish(*input);
  auto loaded = load(directory);
  ASSERT_TRUE(mlir::succeeded(loaded));
  (*loaded->package)
      ->setAttr("ac.changed_after_verification", mlir::UnitAttr::get(&context));
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*loaded->package)));
  auto final = capture([&] { return refineVerifiedAnchor(*loaded, error()); });
  EXPECT_TRUE(mlir::failed(final));
  EXPECT_NE(diagnostics.find("changed after native verification"),
            std::string::npos)
      << diagnostics;
}

TEST_F(AnchorArtifactTest,
       DeclaredExternReloadsButExplicitRefinementRequiresImplementation) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto root = mlir::cast<ac::ModuleOp>(
      mlir::SymbolTable::lookupSymbolIn(*input->routed, "δοκιμή.模块.Root"));
  auto child = mlir::cast<ac::ModuleOp>(
      mlir::SymbolTable::lookupSymbolIn(*input->routed, "δοκιμή.模块.Child"));
  mlir::OpBuilder builder(root);
  mlir::OperationState external(root.getLoc(),
                                ac::ModuleImportOp::getOperationName());
  external.addAttribute("sym_name",
                        builder.getStringAttr("δοκιμή.模块.External"));
  external.addAttribute("source_owner", child.getSourceOwner());
  external.addAttribute("semantic_kind", builder.getStringAttr("hardware"));
  external.addAttribute("parameters", builder.getArrayAttr({}));
  external.addAttribute("type_parameters", builder.getArrayAttr({}));
  external.addAttribute("function_type",
                        mlir::TypeAttr::get(child.getFunctionType()));
  external.addAttribute("input_names", child.getInputNames());
  external.addAttribute("output_names", child.getOutputNames());
  external.addAttribute(
      "dependency_summary",
      builder.getArrayAttr({ac::getOutputDependencyAttr(builder, 0, {})}));
  external.addAttribute(
      "ac.external_contract",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("state", builder.getStringAttr("none")),
           builder.getNamedAttr("effects", builder.getArrayAttr({})),
           builder.getNamedAttr("latency", builder.getI64IntegerAttr(0)),
           builder.getNamedAttr("availability",
                                builder.getStringAttr("all_inputs")),
           builder.getNamedAttr("value_domain",
                                builder.getStringAttr("four_state")),
           builder.getNamedAttr("failure", builder.getStringAttr("none"))}));
  builder.create(external);
  for (auto instance : root.getBody().front().getOps<ac::InstanceOp>())
    instance->setAttr("callee", mlir::FlatSymbolRefAttr::get(
                                    &context, "δοκιμή.模块.External"));
  // This directly exercises the existing standalone native extern profile.
  // It neither admits a new source import nor claims frontend extern syntax.
  ASSERT_TRUE(mlir::succeeded(ac::verifyRoutedPackage(*input->routed)));
  auto directory = publish(*input);
  auto loaded = load(directory);
  ASSERT_TRUE(mlir::succeeded(loaded)) << diagnostics;
  auto before = print(loaded->package->getOperation());
  auto final = capture([&] { return refineVerifiedAnchor(*loaded, error()); });
  EXPECT_TRUE(mlir::failed(final));
  EXPECT_NE(diagnostics.find(
                "requires an implementation for an ordinary external module"),
            std::string::npos)
      << diagnostics;
  EXPECT_EQ(print(loaded->package->getOperation()), before);
}

TEST_F(AnchorArtifactTest, NativeTransportBoundsPrecedeJSONAndMLIRParsing) {
  for (llvm::StringRef mutation :
       {"manifest", "file", "total", "depth", "extra-directory"}) {
    SCOPED_TRACE(mutation.str());
    auto directory = temporary / mutation.str();
    fs::create_directory(directory);
    write(directory / "anchor.json", "{}\n");
    if (mutation == "manifest")
      fs::resize_file(directory / "anchor.json",
                      AnchorFormatLimits::manifestBytes + 1);
    if (mutation == "file") {
      write(directory / "graph.ac", "");
      fs::resize_file(directory / "graph.ac",
                      AnchorFormatLimits::fileBytes + 1);
    }
    if (mutation == "total")
      for (unsigned i = 0; i < 5; ++i) {
        auto path = directory / ("payload" + std::to_string(i));
        write(path, "");
        fs::resize_file(path, i < 4 ? AnchorFormatLimits::fileBytes : 1);
      }
    if (mutation == "depth") {
      auto path = directory;
      for (unsigned i = 0; i <= AnchorFormatLimits::depth; ++i)
        path /= "d";
      fs::create_directories(path);
    }
    if (mutation == "extra-directory")
      fs::create_directory(directory / "unexpected");
    auto loaded = load(directory);
    EXPECT_TRUE(mlir::failed(loaded));
    EXPECT_NE(diagnostics.find("anchor"), std::string::npos) << diagnostics;
    EXPECT_EQ(diagnostics.find("JSON"), std::string::npos) << diagnostics;
  }
}

TEST_F(AnchorArtifactTest,
       FinalAnchorProvenanceRequiresClosedDigestSchemaAndFinalStage) {
  auto input = inputs();
  ASSERT_TRUE(input);
  auto directory = publish(*input);
  auto loaded = load(directory);
  ASSERT_TRUE(mlir::succeeded(loaded));
  auto final = capture([&] { return refineVerifiedAnchor(*loaded, error()); });
  ASSERT_TRUE(mlir::succeeded(final)) << diagnostics;
  auto valid = (*final)->getOperation()->getAttrOfType<mlir::DictionaryAttr>(
      "ac.anchor_provenance");
  ASSERT_TRUE(valid);
  for (unsigned mutation = 0; mutation < 7; ++mutation) {
    SCOPED_TRACE(mutation);
    mlir::OwningOpRef<mlir::ModuleOp> changed(
        mlir::cast<mlir::ModuleOp>((*final)->getOperation()->clone()));
    mlir::Builder builder(&context);
    mlir::NamedAttrList fields(valid);
    if (mutation == 0)
      fields.erase("content_identity");
    if (mutation == 1)
      fields.set("extra", builder.getUnitAttr());
    if (mutation == 2)
      fields.set("content_identity",
                 builder.getStringAttr(std::string(64, 'A')));
    if (mutation == 3)
      fields.set("configuration_identity", builder.getStringAttr("short"));
    if (mutation == 4)
      fields.set("content_identity", builder.getI64IntegerAttr(1));
    (*changed)->setAttr("ac.anchor_provenance", fields.getDictionary(&context));
    if (mutation == 5)
      (*changed)->setAttr("ac.stage", builder.getStringAttr("routed"));
    if (mutation == 6)
      (*changed)->setAttr("ac.unit_kind",
                          builder.getStringAttr("implementation"));
    EXPECT_TRUE(mlir::failed(capture([&] { return mlir::verify(*changed); })));
    EXPECT_NE(diagnostics.find("ac.anchor_provenance"), std::string::npos)
        << diagnostics;
  }
}

TEST_F(AnchorArtifactTest,
       FinalAnchorProvenanceCannotAppearOnNestedPackageOrDefinition) {
  for (bool builtinModule : {false, true}) {
    auto input = inputs();
    ASSERT_TRUE(input);
    auto directory = publish(*input);
    auto loaded = load(directory);
    ASSERT_TRUE(mlir::succeeded(loaded));
    auto final =
        capture([&] { return refineVerifiedAnchor(*loaded, error()); });
    ASSERT_TRUE(mlir::succeeded(final)) << diagnostics;
    auto provenance = (*final)->getOperation()->getAttr("ac.anchor_provenance");
    if (builtinModule) {
      auto nested = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
      nested->setAttr("ac.stage", mlir::StringAttr::get(&context, "final"));
      nested->setAttr("ac.anchor_provenance", provenance);
      (*final)->getBody()->push_back(nested);
    } else {
      auto definition = *(*final)->getOps<ac::ModuleOp>().begin();
      definition->setAttr("ac.anchor_provenance", provenance);
    }
    EXPECT_TRUE(mlir::failed(
        capture([&] { return ac::verifyHardwarePackage(**final); })));
    EXPECT_NE(diagnostics.find("top-level final package"), std::string::npos)
        << diagnostics;
    fs::remove_all(directory);
  }
}

TEST_F(AnchorArtifactTest,
       LiteralCollectionShapeSurvivesSplitLoadAndRefineWithoutLaneExpansion) {
  for (uint64_t lanes : {3ULL, 65537ULL}) {
    SCOPED_TRACE(lanes);
    auto input = inputs(2, collectionSource(lanes));
    ASSERT_TRUE(input);
    auto sourceRoot = mlir::cast<ac::ModuleOp>(
        mlir::SymbolTable::lookupSymbolIn(*input->routed, "δοκιμή.模块.Root"));
    auto sourceCollections =
        sourceRoot.getBody().front().getOps<ac::CollectionOp>();
    ASSERT_TRUE(llvm::hasSingleElement(sourceCollections));
    auto original = *sourceCollections.begin();
    auto directory = publish(*input, "collection-" + std::to_string(lanes));
    auto loaded = load(directory);
    ASSERT_TRUE(mlir::succeeded(loaded)) << diagnostics;
    auto root = mlir::cast<ac::ModuleOp>(mlir::SymbolTable::lookupSymbolIn(
        *loaded->package, "δοκιμή.模块.Root"));
    auto savedCollections = root.getBody().front().getOps<ac::CollectionOp>();
    ASSERT_TRUE(llvm::hasSingleElement(savedCollections));
    auto saved = *savedCollections.begin();
    EXPECT_EQ(saved->getAttrDictionary(), original->getAttrDictionary());
    EXPECT_EQ(saved.getCallee(), "δοκιμή.模块.Child");
    EXPECT_EQ(saved.getOccurrence(), original.getOccurrence());
    EXPECT_TRUE(
        llvm::equal(saved.getOperandTypes(), original.getOperandTypes()));
    EXPECT_TRUE(llvm::equal(saved.getResultTypes(), original.getResultTypes()));
    EXPECT_EQ(root.getFunctionType(), sourceRoot.getFunctionType());
    ASSERT_EQ(saved.getShape().size(), 1u);
    ac::HardwareBindings bindings;
    bindings.owner = root;
    ac::HardwareAnalysis analysis(*loaded->package);
    auto extent = analysis.evaluateStatic(
        mlir::cast<ac::StaticExprAttr>(saved.getShape()[0]), bindings, saved);
    ASSERT_TRUE(mlir::succeeded(extent));
    EXPECT_EQ(mlir::cast<ac::MathIntAttr>(*extent).getCanonicalValue(),
              std::to_string(lanes));
    auto packedWidth =
        analysis.getPackedWidth(saved.getResult(0).getType(), bindings, saved);
    ASSERT_TRUE(mlir::succeeded(packedWidth));
    EXPECT_EQ(*packedWidth, lanes * 7);
    size_t operationCount = 0;
    loaded->package->walk([&](mlir::Operation *) { ++operationCount; });
    auto final =
        capture([&] { return refineVerifiedAnchor(*loaded, error()); });
    ASSERT_TRUE(mlir::succeeded(final)) << diagnostics;
    unsigned collectionCount = 0, instanceCount = 0, definitionCount = 0;
    size_t finalOperationCount = 0;
    (*final)->walk([&](mlir::Operation *op) {
      ++finalOperationCount;
      collectionCount += mlir::isa<ac::CollectionOp>(op);
      instanceCount += mlir::isa<ac::InstanceOp>(op);
      definitionCount += mlir::isa<ac::ModuleOp>(op);
      if (auto collection = mlir::dyn_cast<ac::CollectionOp>(op)) {
        EXPECT_EQ(collection->getAttrDictionary(), saved->getAttrDictionary());
        EXPECT_TRUE(
            llvm::equal(collection.getResultTypes(), saved.getResultTypes()));
      }
    });
    EXPECT_EQ(collectionCount, 1u);
    EXPECT_EQ(instanceCount, 0u);
    EXPECT_EQ(definitionCount, 2u);
    EXPECT_EQ(finalOperationCount, operationCount);
    EXPECT_EQ(print(input->body->getOperation()), input->originalBody);
    EXPECT_EQ(print(input->header->getOperation()), input->originalHeader);
  }
}

TEST_F(AnchorArtifactTest,
       NativePayloadCountCeilingRejectsBeforeManifestParsing) {
  auto directory = temporary / "payload-count";
  fs::create_directory(directory);
  write(directory / "anchor.json", "not JSON or MLIR\n");
  for (uint64_t i = 0; i <= AnchorFormatLimits::payloadFiles; ++i)
    write(directory / ("payload-" + std::to_string(i)), "");
  reject(directory, "anchor file/count/aggregate bytes exceed format limit");
  EXPECT_EQ(diagnostics.find("JSON"), std::string::npos) << diagnostics;
}

TEST_F(AnchorArtifactTest,
       NativeDirectoryCountCeilingRejectsBeforeManifestParsing) {
  auto directory = temporary / "directory-count";
  fs::create_directory(directory);
  write(directory / "anchor.json", "not JSON or MLIR\n");
  for (uint64_t i = 0; i <= AnchorFormatLimits::directories; ++i)
    fs::create_directory(directory / ("directory-" + std::to_string(i)));
  reject(directory, "anchor directory count exceeds format limit");
  EXPECT_EQ(diagnostics.find("JSON"), std::string::npos) << diagnostics;
}

} // namespace
} // namespace acir::compiler

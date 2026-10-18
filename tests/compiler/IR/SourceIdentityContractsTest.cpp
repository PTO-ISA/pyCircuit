#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;
using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

llvm::APSInt signedInteger(unsigned width, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  llvm::APInt bits(width, spelling, 10);
  if (negative)
    bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/false);
}

MathIntAttr math(mlir::MLIRContext &context, llvm::StringRef spelling) {
  return MathIntAttr::get(
      &context,
      signedInteger(static_cast<unsigned>(spelling.size() * 4 + 2), spelling));
}

mlir::IntegerAttr integer(mlir::OpBuilder &builder, unsigned width,
                          const llvm::APInt &value,
                          mlir::IntegerType::SignednessSemantics signedness =
                              mlir::IntegerType::Signless) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), width, signedness), value);
}

mlir::IntegerAttr u64(mlir::OpBuilder &builder, uint64_t value) {
  return integer(builder, 64, llvm::APInt(64, value));
}

mlir::IntegerAttr u32(mlir::OpBuilder &builder, uint32_t value) {
  return integer(builder, 32, llvm::APInt(32, value));
}

mlir::FlatSymbolRefAttr symbol(mlir::OpBuilder &builder,
                               llvm::StringRef value) {
  return mlir::FlatSymbolRefAttr::get(builder.getContext(), value);
}

mlir::DictionaryAttr pathField(mlir::OpBuilder &builder,
                               llvm::StringRef name = "body") {
  return dictionary(builder, {{"kind", builder.getStringAttr("field")},
                              {"name", builder.getStringAttr(name)}});
}

mlir::DictionaryAttr site(mlir::OpBuilder &builder,
                          llvm::StringRef definition = "demo.Model") {
  return dictionary(builder,
                    {{"definition", symbol(builder, definition)},
                     {"ast_path", builder.getArrayAttr({pathField(builder)})}});
}

mlir::DictionaryAttr boolValue(mlir::OpBuilder &builder, bool value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"value", builder.getBoolAttr(value)}});
}

mlir::DictionaryAttr integerValue(mlir::OpBuilder &builder,
                                  llvm::StringRef value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"value", math(*builder.getContext(), value)}});
}

mlir::DictionaryAttr callFrame(mlir::OpBuilder &builder,
                               llvm::StringRef callee = "demo.Helper") {
  return dictionary(builder, {{"kind", builder.getStringAttr("call")},
                              {"site", site(builder)},
                              {"callee", symbol(builder, callee)}});
}

mlir::DictionaryAttr iterationFrame(mlir::OpBuilder &builder,
                                    mlir::Attribute ordinal,
                                    mlir::DictionaryAttr value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                              {"site", site(builder)},
                              {"ordinal", ordinal},
                              {"value", value}});
}

mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder,
                                mlir::ArrayAttr expansion = {}) {
  if (!expansion)
    expansion = builder.getArrayAttr({});
  return dictionary(builder,
                    {{"site", site(builder)}, {"expansion", expansion}});
}

mlir::DictionaryAttr specKey(mlir::OpBuilder &builder,
                             mlir::ArrayAttr arguments,
                             llvm::StringRef definition = "demo.Model") {
  return dictionary(builder, {{"definition", symbol(builder, definition)},
                              {"arguments", arguments}});
}

mlir::DictionaryAttr valueID(mlir::OpBuilder &builder, mlir::Attribute slot) {
  return dictionary(builder, {{"origin", occurrence(builder)}, {"slot", slot}});
}

mlir::DictionaryAttr checkID(mlir::OpBuilder &builder,
                             mlir::Attribute obligation) {
  return dictionary(builder, {{"registration", occurrence(builder)},
                              {"check", occurrence(builder)},
                              {"obligation", obligation}});
}

mlir::DictionaryAttr ownerRef(mlir::OpBuilder &builder, mlir::ArrayAttr path) {
  return dictionary(builder, {{"instance_path", path}});
}

struct CheckResult {
  bool passed;
  std::string diagnostic;
};

template <typename Callback>
CheckResult check(mlir::MLIRContext &context, Callback callback) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };
  return {mlir::succeeded(callback(emitError)), diagnostic};
}

void expectRejected(const CheckResult &result) {
  EXPECT_FALSE(result.passed);
  EXPECT_FALSE(result.diagnostic.empty());
}

class SourceIdentityContractsTest : public ::testing::Test {
protected:
  SourceIdentityContractsTest() : builder(&context) {
    context.loadDialect<ACIRDialect>();
  }

  template <typename Verifier>
  CheckResult verify(mlir::DictionaryAttr value, Verifier verifier) {
    return check(context,
                 [&](EmitError emit) { return verifier(value, emit); });
  }

  mlir::MLIRContext context;
  mlir::OpBuilder builder;
};

TEST_F(SourceIdentityContractsTest,
       SourceOwnerEnforcesOnlyApprovedPosixPathRules) {
  auto verifyOwner = [&](llvm::StringRef package, llvm::StringRef path) {
    return verify(
        dictionary(builder, {{"package", builder.getStringAttr(package)},
                             {"path", builder.getStringAttr(path)}}),
        detail::verifySourceOwner);
  };

  EXPECT_TRUE(verifyOwner("", "model.py").passed);
  EXPECT_TRUE(verifyOwner("demo.pkg", "nested/model.py").passed);
  EXPECT_TRUE(verifyOwner("demo", "pkg/__init__.py").passed);
  EXPECT_TRUE(verifyOwner("demo", "pkg:variant/model.py").passed);
  for (llvm::StringRef path :
       {"", "/model.py", "./model.py", "pkg/./model.py", "../model.py",
        "pkg/../model.py", "pkg//model.py", "pkg\\model.py", "C:/model.py",
        "C:model.py", "model.txt", "model.PY", "pkg/"})
    expectRejected(verifyOwner("demo", path));
  std::string nulPath("pkg/model\0.py", 13);
  expectRejected(verifyOwner("demo", nulPath));

  expectRejected(
      verify(dictionary(builder, {{"package", builder.getStringAttr("demo")}}),
             detail::verifySourceOwner));
  expectRejected(
      verify(dictionary(builder, {{"package", builder.getStringAttr("demo")},
                                  {"path", builder.getStringAttr("model.py")},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifySourceOwner));
  expectRejected(
      verify(dictionary(builder, {{"package", u64(builder, 0)},
                                  {"path", builder.getStringAttr("model.py")}}),
             detail::verifySourceOwner));
}

TEST_F(SourceIdentityContractsTest,
       SourceSpanGrammarMemoRejectsChangedMalformedSpans) {
  auto span = dictionary(builder, {{"path", builder.getStringAttr("model.py")},
      {"line", u64(builder, 7)}, {"column", u64(builder, 3)},
      {"end_line", u64(builder, 7)}, {"end_column", u64(builder, 9)}});
  EXPECT_FALSE(detail::isVerifiedSourceSpanGrammar(span));
  ASSERT_TRUE(verify(span, detail::verifySourceSpan).passed);
  ASSERT_TRUE(detail::isVerifiedSourceSpanGrammar(span));
  mlir::NamedAttrList updated(span);
  updated.set("end_column", u64(builder, 10));
  auto changed = updated.getDictionary(&context);
  EXPECT_FALSE(detail::isVerifiedSourceSpanGrammar(changed));
  EXPECT_TRUE(verify(changed, detail::verifySourceSpan).passed);
  EXPECT_TRUE(detail::isVerifiedSourceSpanGrammar(changed));
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    mlir::NamedAttrList fields(span);
    if (mutation == 0)
      fields.set("end_line", u64(builder, 6));
    else if (mutation == 1)
      fields.set("end_column", u64(builder, 2));
    else
      fields.set("extra", mlir::TypeAttr::get(builder.getI1Type()));
    auto malformed = fields.getDictionary(&context);
    expectRejected(verify(malformed, detail::verifySourceSpan));
    EXPECT_FALSE(detail::isVerifiedSourceSpanGrammar(malformed));
    EXPECT_TRUE(detail::isVerifiedSourceSpanGrammar(span));
  }
}

TEST_F(SourceIdentityContractsTest,
       WarmSourceSpanGrammarStillChecksCurrentSourceOwnerPath) {
  auto span = dictionary(builder, {{"path", builder.getStringAttr("model.py")},
      {"line", u64(builder, 2)}, {"column", u64(builder, 1)},
      {"end_line", u64(builder, 2)}, {"end_column", u64(builder, 5)}});
  ASSERT_TRUE(verify(span, detail::verifySourceSpan).passed);
  ASSERT_TRUE(detail::isVerifiedSourceSpanGrammar(span));
  auto makeOwner = [&](llvm::StringRef path) {
    mlir::OperationState state(mlir::UnknownLoc::get(&context), ModuleOp::getOperationName());
    state.addAttribute("sym_name", builder.getStringAttr("demo.Model"));
    state.addAttribute("source_owner", dictionary(builder, {
        {"package", builder.getStringAttr("demo")}, {"path", builder.getStringAttr(path)}}));
    return mlir::OwningOpRef<ModuleOp>(mlir::cast<ModuleOp>(mlir::Operation::create(state)));
  };
  auto owner = makeOwner("model.py"), foreignSource = makeOwner("other.py");
  auto origin = occurrence(builder);
  EXPECT_TRUE(check(context, [&](EmitError emit) {
    return detail::verifySourceEffectMetadata(origin, span, owner->getOperation(), emit);
  }).passed);
  expectRejected(check(context, [&](EmitError emit) {
    return detail::verifySourceEffectMetadata(origin, span, foreignSource->getOperation(), emit);
  }));
  EXPECT_TRUE(detail::isVerifiedSourceSpanGrammar(span));
}

TEST_F(SourceIdentityContractsTest,
       SourceSpanGrammarMemoDoesNotCrossContexts) {
  auto makeSpan = [](mlir::OpBuilder &b) {
    return dictionary(b, {{"path", b.getStringAttr("model.py")},
        {"line", u64(b, 7)}, {"column", u64(b, 3)},
        {"end_line", u64(b, 7)}, {"end_column", u64(b, 9)}});
  };
  auto span = makeSpan(builder);
  ASSERT_TRUE(verify(span, detail::verifySourceSpan).passed);
  mlir::MLIRContext otherContext;
  otherContext.loadDialect<ACIRDialect>();
  mlir::OpBuilder other(&otherContext);
  auto sameSpelling = makeSpan(other);
  EXPECT_FALSE(detail::isVerifiedSourceSpanGrammar(sameSpelling));
  EXPECT_EQ(detail::sourceSpanGrammarCacheShape(&otherContext).first, 0u);
  EXPECT_TRUE(check(otherContext, [&](EmitError emit) {
    return detail::verifySourceSpan(sameSpelling, emit);
  }).passed);
  EXPECT_TRUE(detail::isVerifiedSourceSpanGrammar(sameSpelling));
  EXPECT_TRUE(detail::isVerifiedSourceSpanGrammar(span));
}

TEST_F(SourceIdentityContractsTest, ExpansionFramesAcceptBothClosedVariants) {
  EXPECT_TRUE(
      verify(callFrame(builder, ""), detail::verifyExpansionFrame).passed);
  EXPECT_TRUE(verify(iterationFrame(builder, u64(builder, 0),
                                    integerValue(builder, "-1")),
                     detail::verifyExpansionFrame)
                  .passed);
  for (auto invalid : {
           dictionary(builder, {{"kind", builder.getStringAttr("other")},
                                {"site", site(builder)},
                                {"callee", symbol(builder, "demo.Helper")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", site(builder)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", builder.getStringAttr("not-a-site")},
                                {"callee", symbol(builder, "demo.Helper")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", site(builder)},
                                {"callee", symbol(builder, "demo.Helper")},
                                {"unknown", builder.getUnitAttr()}}),
           dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                                {"site", site(builder)},
                                {"ordinal", builder.getBoolAttr(true)},
                                {"value", boolValue(builder, true)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                                {"site", site(builder)},
                                {"ordinal", u64(builder, 0)},
                                {"value", builder.getStringAttr("bad")}}),
       })
    expectRejected(verify(invalid, detail::verifyExpansionFrame));
}

TEST_F(SourceIdentityContractsTest,
       OccurrencePreservesEmptyAndOrderedDuplicateExpansionFrames) {
  auto first = callFrame(builder, "demo.A");
  auto second = callFrame(builder, "demo.B");
  EXPECT_TRUE(verify(occurrence(builder), detail::verifyOccurrence).passed);
  EXPECT_TRUE(
      verify(occurrence(builder, builder.getArrayAttr({second, first, second})),
             detail::verifyOccurrence)
          .passed);
  expectRejected(verify(dictionary(builder, {{"site", site(builder)}}),
                        detail::verifyOccurrence));
  expectRejected(
      verify(dictionary(builder, {{"site", site(builder)},
                                  {"expansion", builder.getStringAttr("bad")}}),
             detail::verifyOccurrence));
  expectRejected(
      verify(dictionary(builder, {{"site", site(builder)},
                                  {"expansion", builder.getArrayAttr({})},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifyOccurrence));
  auto nested = verify(
      occurrence(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifyOccurrence);
  expectRejected(nested);
  EXPECT_NE(nested.diagnostic.find("expansion[0]"), std::string::npos)
      << nested.diagnostic;
}

TEST_F(SourceIdentityContractsTest,
       WarmOccurrenceGrammarDoesNotAuthorizeForeignOwnerOrSourcePath) {
  auto origin = occurrence(builder);
  ASSERT_TRUE(verify(origin, detail::verifyOccurrence).passed);
  ASSERT_TRUE(detail::isVerifiedOccurrenceGrammar(origin));
  auto makeOwner = [&](llvm::StringRef name, llvm::StringRef path) {
    mlir::OperationState state(mlir::UnknownLoc::get(&context),
                               ModuleOp::getOperationName());
    state.addAttribute("sym_name", builder.getStringAttr(name));
    state.addAttribute(
        "source_owner",
        dictionary(builder, {{"package", builder.getStringAttr("demo")},
                             {"path", builder.getStringAttr(path)}}));
    return mlir::OwningOpRef<ModuleOp>(
        mlir::cast<ModuleOp>(mlir::Operation::create(state)));
  };
  auto location =
      dictionary(builder, {{"path", builder.getStringAttr("model.py")},
                           {"line", u64(builder, 1)},
                           {"column", u64(builder, 1)},
                           {"end_line", u64(builder, 1)},
                           {"end_column", u64(builder, 2)}});
  auto owner = makeOwner("demo.Model", "model.py");
  auto foreign = makeOwner("demo.Other", "model.py");
  auto wrongSource = makeOwner("demo.Model", "other.py");
  auto metadata = [&](mlir::Operation *module) {
    return check(context, [&](EmitError emit) {
      return detail::verifySourceEffectMetadata(origin, location, module, emit);
    });
  };
  EXPECT_TRUE(metadata(owner->getOperation()).passed);
  expectRejected(metadata(foreign->getOperation()));
  expectRejected(metadata(wrongSource->getOperation()));
  // The scope shortcut cannot admit a dictionary containing a hardware type.
  auto changed = dictionary(
      builder, {{"site", site(builder)},
                {"expansion", builder.getArrayAttr({})},
                {"extra", mlir::TypeAttr::get(builder.getI1Type())}});
  expectRejected(verify(changed, detail::verifyOccurrence));
  EXPECT_FALSE(detail::isVerifiedOccurrenceGrammar(changed));
  EXPECT_TRUE(detail::isVerifiedOccurrenceGrammar(origin));
}

TEST_F(SourceIdentityContractsTest,
       OccurrenceGrammarMemoSeparatesChangedAttributesAndContexts) {
  auto good = occurrence(
      builder, builder.getArrayAttr({iterationFrame(
                   builder, u64(builder, 2), integerValue(builder, "5"))}));
  ASSERT_TRUE(verify(good, detail::verifyOccurrence).passed);
  auto changed = occurrence(
      builder,
      builder.getArrayAttr({iterationFrame(builder, builder.getBoolAttr(true),
                                           integerValue(builder, "5"))}));
  expectRejected(verify(changed, detail::verifyOccurrence));
  EXPECT_FALSE(detail::isVerifiedOccurrenceGrammar(changed));
  mlir::MLIRContext second;
  second.loadDialect<ACIRDialect>();
  mlir::OpBuilder other(&second);
  auto separate = occurrence(other);
  EXPECT_FALSE(detail::isVerifiedOccurrenceGrammar(separate));
  EXPECT_TRUE(check(second, [&](EmitError emit) {
                return detail::verifyOccurrence(separate, emit);
              }).passed);
  EXPECT_TRUE(detail::isVerifiedOccurrenceGrammar(separate));
}

TEST_F(SourceIdentityContractsTest,
       StaticLiteralGrammarMemoRejectsChangedAndMalformedTrees) {
  auto location = dictionary(
      builder, {{"path", builder.getStringAttr("literal.py")},
                {"line", u64(builder, 1)}, {"column", u64(builder, 1)},
                {"end_line", u64(builder, 1)}, {"end_column", u64(builder, 2)}});
  auto literal = [&](llvm::StringRef value) {
    return dictionary(builder, {{"kind", builder.getStringAttr("literal")},
                                {"value", integerValue(builder, value)},
                                {"origin", occurrence(builder)},
                                {"location", location}});
  };
  auto verifyTree = [&](mlir::DictionaryAttr tree) {
    return check(context, [&](EmitError emit) {
      return StaticExprAttr::verify(emit, tree);
    });
  };
  auto original = literal("7");
  EXPECT_FALSE(detail::isVerifiedStaticExpressionGrammar(original));
  ASSERT_TRUE(verifyTree(original).passed);
  ASSERT_TRUE(detail::isVerifiedStaticExpressionGrammar(original));
  auto changed = literal("8");
  EXPECT_NE(changed, original);
  EXPECT_FALSE(detail::isVerifiedStaticExpressionGrammar(changed));
  EXPECT_TRUE(verifyTree(changed).passed);
  EXPECT_TRUE(detail::isVerifiedStaticExpressionGrammar(changed));
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    SCOPED_TRACE(mutation);
    mlir::NamedAttrList fields(original);
    if (mutation == 0)
      fields.set("value", dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                                              {"value", builder.getI64IntegerAttr(7)}}));
    else if (mutation == 1)
      fields.set("extra", mlir::TypeAttr::get(builder.getI1Type()));
    else
      fields.erase("location");
    auto malformed = fields.getDictionary(&context);
    expectRejected(verifyTree(malformed));
    EXPECT_FALSE(detail::isVerifiedStaticExpressionGrammar(malformed));
    EXPECT_TRUE(detail::isVerifiedStaticExpressionGrammar(original));
    EXPECT_TRUE(verifyTree(original).passed);
  }
}

TEST_F(SourceIdentityContractsTest,
       StaticExpressionGrammarMemoDoesNotCrossContexts) {
  auto tree = [](mlir::OpBuilder &b) {
    return dictionary(b, {{"kind", b.getStringAttr("literal")},
                          {"value", integerValue(b, "19")},
                          {"origin", occurrence(b)},
                          {"location", dictionary(b, {
                              {"path", b.getStringAttr("literal.py")},
                              {"line", u64(b, 1)}, {"column", u64(b, 1)},
                              {"end_line", u64(b, 1)}, {"end_column", u64(b, 2)}})}});
  };
  auto first = tree(builder);
  ASSERT_TRUE(check(context, [&](EmitError emit) {
    return StaticExprAttr::verify(emit, first);
  }).passed);
  ASSERT_TRUE(detail::isVerifiedStaticExpressionGrammar(first));
  mlir::MLIRContext otherContext;
  otherContext.loadDialect<ACIRDialect>();
  mlir::OpBuilder other(&otherContext);
  auto separate = tree(other);
  EXPECT_FALSE(detail::isVerifiedStaticExpressionGrammar(separate));
  EXPECT_EQ(detail::staticExpressionGrammarCacheShape(&otherContext).first, 0u);
  ASSERT_TRUE(check(otherContext, [&](EmitError emit) {
    return StaticExprAttr::verify(emit, separate);
  }).passed);
  EXPECT_TRUE(detail::isVerifiedStaticExpressionGrammar(separate));
  EXPECT_TRUE(detail::isVerifiedStaticExpressionGrammar(first));
}

TEST_F(SourceIdentityContractsTest,
       WarmStaticReferenceAndTypeWidthGrammarKeepLexicalOwnerChecks) {
  auto makeOwner = [&](llvm::StringRef name) {
    mlir::OperationState state(mlir::UnknownLoc::get(&context), ModuleOp::getOperationName());
    state.addAttribute("sym_name", builder.getStringAttr(name));
    state.addAttribute("parameters", builder.getArrayAttr({
        dictionary(builder, {{"name", builder.getStringAttr("WIDTH")}})}));
    state.addAttribute("type_parameters", builder.getArrayAttr({builder.getStringAttr("T")}));
    return mlir::OwningOpRef<ModuleOp>(mlir::cast<ModuleOp>(mlir::Operation::create(state)));
  };
  auto owner = makeOwner("demo.Model");
  auto foreign = makeOwner("demo.Other");
  auto location = dictionary(builder, {{"path", builder.getStringAttr("literal.py")},
      {"line", u64(builder, 1)}, {"column", u64(builder, 1)},
      {"end_line", u64(builder, 1)}, {"end_column", u64(builder, 2)}});
  for (bool typeWidth : {false, true}) {
    SCOPED_TRACE(typeWidth);
    mlir::NamedAttrList fields;
    fields.set("kind", builder.getStringAttr(typeWidth ? "type_width" : "reference"));
    fields.set("origin", occurrence(builder));
    fields.set("location", location);
    if (typeWidth)
      fields.set("type", mlir::TypeAttr::get(TypeParamType::get(
          &context, symbol(builder, "demo.Model"), builder.getStringAttr("T"))));
    else
      fields.set("ref", dictionary(builder, {{"kind", builder.getStringAttr("parameter")},
          {"owner", symbol(builder, "demo.Model")}, {"name", builder.getStringAttr("WIDTH")}}));
    auto tree = fields.getDictionary(&context);
    ASSERT_TRUE(check(context, [&](EmitError emit) {
      return StaticExprAttr::verify(emit, tree);
    }).passed);
    ASSERT_TRUE(detail::isVerifiedStaticExpressionGrammar(tree));
    auto expression = StaticExprAttr::get(&context, tree);
    EXPECT_TRUE(check(context, [&](EmitError) {
      return detail::verifyHardwareAttributeScope(expression, owner->getOperation());
    }).passed);
    expectRejected(check(context, [&](EmitError) {
      return detail::verifyHardwareAttributeScope(expression, foreign->getOperation());
    }));
    EXPECT_TRUE(detail::isVerifiedStaticExpressionGrammar(tree));
  }
}

TEST_F(SourceIdentityContractsTest,
       ConcurrentOccurrenceGrammarMemoRecordsOnlyUniqueSuccesses) {
  std::vector<mlir::DictionaryAttr> roots;
  for (unsigned i = 0; i < 32; ++i)
    roots.push_back(occurrence(
        builder, builder.getArrayAttr({iterationFrame(
                     builder, u64(builder, i), integerValue(builder, "7"))})));
  auto before = detail::occurrenceGrammarCacheShape(&context).first;
  std::atomic<bool> accepted{true};
  auto location = mlir::UnknownLoc::get(&context);
  std::vector<std::thread> threads;
  for (unsigned i = 0; i < 8; ++i)
    threads.emplace_back([&] {
      for (unsigned repeat = 0; repeat < 16; ++repeat)
        for (auto root : roots)
          if (mlir::failed(detail::verifyOccurrence(
                  root, [&] { return mlir::emitError(location); })))
            accepted.store(false, std::memory_order_relaxed);
    });
  for (auto &thread : threads)
    thread.join();
  EXPECT_TRUE(accepted.load(std::memory_order_relaxed));
  EXPECT_EQ(detail::occurrenceGrammarCacheShape(&context).first,
            before + roots.size());
  for (auto root : roots)
    EXPECT_TRUE(detail::isVerifiedOccurrenceGrammar(root));
}

TEST_F(SourceIdentityContractsTest,
       SpecKeyAcceptsEmptySymbolsEmptyArgumentsAndOrderedDuplicates) {
  auto zero = integerValue(builder, "0");
  auto one = integerValue(builder, "1");
  EXPECT_TRUE(verify(specKey(builder, builder.getArrayAttr({}), ""),
                     detail::verifySpecKey)
                  .passed);
  EXPECT_TRUE(verify(specKey(builder, builder.getArrayAttr({one, zero, one})),
                     detail::verifySpecKey)
                  .passed);
  expectRejected(verify(
      dictionary(builder, {{"definition", symbol(builder, "demo.Model")}}),
      detail::verifySpecKey));
  expectRejected(
      verify(dictionary(builder, {{"definition", builder.getStringAttr("bad")},
                                  {"arguments", builder.getArrayAttr({})}}),
             detail::verifySpecKey));
  expectRejected(verify(
      specKey(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifySpecKey));
  expectRejected(
      verify(dictionary(builder, {{"definition", symbol(builder, "demo.Model")},
                                  {"arguments", builder.getArrayAttr({})},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifySpecKey));
}

TEST_F(SourceIdentityContractsTest,
       U32AndU64FieldsHonorMathematicalValueRange) {
  auto wideSmall = integer(builder, 128, llvm::APInt(128, 7));
  auto maximumU32 = integer(builder, 32, llvm::APInt::getAllOnes(32),
                            mlir::IntegerType::Unsigned);
  auto maximumU64 = integer(builder, 64, llvm::APInt::getAllOnes(64),
                            mlir::IntegerType::Unsigned);
  EXPECT_TRUE(
      verify(valueID(builder, maximumU32), detail::verifyValueID).passed);
  EXPECT_TRUE(
      verify(valueID(builder, wideSmall), detail::verifyValueID).passed);
  EXPECT_TRUE(
      verify(checkID(builder, maximumU64), detail::verifyCheckID).passed);
  EXPECT_TRUE(
      verify(checkID(builder, wideSmall), detail::verifyCheckID).passed);

  for (mlir::Attribute invalid : {
           mlir::Attribute(builder.getBoolAttr(true)),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64))),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64),
                                   mlir::IntegerType::Signed)),
           mlir::Attribute(
               integer(builder, 128, llvm::APInt::getOneBitSet(128, 32))),
       })
    expectRejected(verify(valueID(builder, invalid), detail::verifyValueID));
  expectRejected(
      verify(checkID(builder,
                     integer(builder, 128, llvm::APInt::getOneBitSet(128, 64))),
             detail::verifyCheckID));
  for (mlir::Attribute invalid : {
           mlir::Attribute(builder.getBoolAttr(true)),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64))),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64),
                                   mlir::IntegerType::Signed)),
       })
    expectRejected(verify(checkID(builder, invalid), detail::verifyCheckID));
}

TEST_F(SourceIdentityContractsTest,
       CheckValueAndProofIdentitiesRejectMalformedNestedChildren) {
  auto value = valueID(builder, u32(builder, 0));
  auto checkIdentity = checkID(builder, u64(builder, 0));
  auto key = specKey(builder, builder.getArrayAttr({}));
  auto scope = dictionary(builder, {{"specialization", key},
                                    {"registration", occurrence(builder)}});
  EXPECT_TRUE(verify(value, detail::verifyValueID).passed);
  EXPECT_TRUE(verify(checkIdentity, detail::verifyCheckID).passed);
  EXPECT_TRUE(verify(scope, detail::verifyProofScope).passed);

  expectRejected(verify(dictionary(builder, {{"origin", occurrence(builder)}}),
                        detail::verifyValueID));
  expectRejected(
      verify(dictionary(builder, {{"origin", occurrence(builder)},
                                  {"slot", u32(builder, 0)},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifyValueID));
  expectRejected(
      verify(dictionary(builder, {{"registration", occurrence(builder)},
                                  {"check", builder.getStringAttr("bad")},
                                  {"obligation", u64(builder, 0)}}),
             detail::verifyCheckID));
  expectRejected(
      verify(dictionary(builder, {{"registration", occurrence(builder)},
                                  {"check", occurrence(builder)}}),
             detail::verifyCheckID));
  auto nested = verify(
      dictionary(builder, {{"specialization", builder.getStringAttr("bad")},
                           {"registration", occurrence(builder)}}),
      detail::verifyProofScope);
  expectRejected(nested);
  EXPECT_NE(nested.diagnostic.find("specialization"), std::string::npos)
      << nested.diagnostic;
  expectRejected(verify(dictionary(builder, {{"specialization", key}}),
                        detail::verifyProofScope));
}

TEST_F(SourceIdentityContractsTest,
       OwnerAndStateIdentitiesAllowRootAndValueEmptyPaths) {
  auto declaration = occurrence(builder);
  auto root = ownerRef(builder, builder.getArrayAttr({}));
  auto repeatedPath = ownerRef(
      builder,
      builder.getArrayAttr({occurrence(builder), occurrence(builder)}));
  auto state = dictionary(builder, {{"owner", repeatedPath},
                                    {"declaration", declaration},
                                    {"element", builder.getArrayAttr({})}});
  EXPECT_TRUE(verify(root, detail::verifyOwnerRef).passed);
  EXPECT_TRUE(verify(repeatedPath, detail::verifyOwnerRef).passed);
  EXPECT_TRUE(verify(state, detail::verifyStateID).passed);
  EXPECT_TRUE(
      verify(dictionary(builder,
                        {{"owner", root},
                         {"declaration", declaration},
                         {"element", builder.getArrayAttr(
                                         {u64(builder, 2), u64(builder, 0)})}}),
             detail::verifyStateID)
          .passed);
  expectRejected(verify(
      ownerRef(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifyOwnerRef));
  expectRejected(verify(
      dictionary(builder, {{"owner", root}, {"declaration", declaration}}),
      detail::verifyStateID));
  expectRejected(
      verify(dictionary(builder, {{"owner", builder.getStringAttr("bad")},
                                  {"declaration", declaration},
                                  {"element", builder.getArrayAttr({})}}),
             detail::verifyStateID));
}

TEST_F(SourceIdentityContractsTest,
       StateRefDistinguishesOwnedAndFormalOrdinals) {
  auto declaration = occurrence(builder);
  auto ownedValue =
      dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                           {"declaration", declaration},
                           {"element", builder.getArrayAttr({})}});
  auto formalValue =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("")},
                           {"ordinal", builder.getUnitAttr()}});
  auto formalElement =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("items")},
                           {"ordinal", u64(builder, 0)}});
  EXPECT_TRUE(verify(ownedValue, detail::verifyStateRef).passed);
  EXPECT_TRUE(verify(formalValue, detail::verifyStateRef).passed);
  EXPECT_TRUE(verify(formalElement, detail::verifyStateRef).passed);

  for (auto invalid : {
           dictionary(builder, {{"kind", builder.getStringAttr("other")},
                                {"parameter", builder.getStringAttr("items")},
                                {"ordinal", u64(builder, 0)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                                {"declaration", declaration},
                                {"element", builder.getArrayAttr(
                                                {builder.getUnitAttr()})}}),
           dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                                {"parameter", builder.getStringAttr("items")},
                                {"ordinal", builder.getBoolAttr(true)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                                {"parameter", builder.getStringAttr("items")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                                {"declaration", declaration},
                                {"element", builder.getArrayAttr({})},
                                {"unknown", builder.getUnitAttr()}}),
       })
    expectRejected(verify(invalid, detail::verifyStateRef));
}

} // namespace
} // namespace acir::ac

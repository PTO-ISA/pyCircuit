#include "TransactionalEmitGfsim.h"
#include "HardwareEmitCommon.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include <map>
#include <set>
using namespace mlir;
namespace acir::compiler {
namespace {
using Step = ac::TransactionalStep;
using Sequence = ac::TransactionalSequence;
using Kind = ac::TransactionalStepKind;
constexpr unsigned NoID = ac::TransactionalNoID;
std::optional<llvm::APSInt> literal(ac::StaticExprAttr attr) {
  auto tree = attr ? attr.getTree() : DictionaryAttr();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
  auto integer =
      value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !integer)
    return std::nullopt;
  return llvm::APSInt(integer.getCanonicalValue());
}
std::string quote(StringRef value) {
  std::string result = "\"";
  for (unsigned char c : value.bytes()) {
    if (c == '"' || c == '\\') {
      result += '\\';
      result += c;
    } else if (c >= 32 && c < 127)
      result += c;
    else {
      result += '\\';
      result += static_cast<char>('0' + ((c >> 6) & 7));
      result += static_cast<char>('0' + ((c >> 3) & 7));
      result += static_cast<char>('0' + (c & 7));
    }
  }
  return result + "\"";
}

class Emitter {
public:
  Emitter(const ac::TransactionalAnalysis &plan, raw_ostream &output)
      : p(plan), out(output) {}
  LogicalResult run();

private:
  const ac::TransactionalAnalysis &p;
  raw_ostream &out;
  llvm::DenseMap<unsigned, const Step *> producers;
  SmallVector<const Step *> steps;
  unsigned indentation = 0;
  void line(const Twine &text = {}) { out.indent(indentation) << text << '\n'; }
  unsigned width(Type type) const { return *p.getPayloadWidth(type); }
  std::string valueType(unsigned id) const {
    return "::gfsim::FourState<" +
           std::to_string(width(p.getValues()[id].type)) + ">";
  }
  std::string slot(unsigned id) const {
    const auto &value = p.getValues()[id];
    if (value.rule == NoID)
      return "model.m" + std::to_string(value.instance) + ".c.v" +
             std::to_string(id);
    return "f.v" + std::to_string(id);
  }
  std::string resource(unsigned id) const {
    return "model.m" + std::to_string(p.getResources()[id].instance) + ".r" +
           std::to_string(id);
  }
  std::string available(ArrayRef<unsigned> operands) const {
    std::string condition = "true";
    for (auto id : operands)
      condition += " && " + slot(id) + ".has_value()";
    return condition;
  }
  std::string path(ArrayRef<ac::TransactionalPathTerm> terms) const {
    std::string result = "true";
    for (auto term : terms)
      result += " && " + slot(term.value) +
                " && (::pycircuit::transactional_gfsim::CheckedControl(*" + slot(term.value) +
                ") == " + (term.positive ? "true" : "false") + ")";
    return result;
  }
  std::string bits(const llvm::APInt &value, unsigned w) {
    auto data = value.zextOrTrunc(w);
    std::string result = "::gfsim::Bits<" + std::to_string(w) + ">{";
    for (unsigned i = 0; i < data.getNumWords(); ++i) {
      if (i)
        result += ", ";
      result += "0x" + llvm::utohexstr(data.getRawData()[i]) + "ULL";
    }
    return result + "}";
  }
  std::string known(const llvm::APInt &value, unsigned w) {
    return "::gfsim::FourState<" + std::to_string(w) + ">::known(" +
           bits(value, w) + ")";
  }
  FailureOr<std::string> expression(const Step &, unsigned,
                                    ArrayRef<std::string>);
  FailureOr<std::string> initial(unsigned value) {
    auto found = producers.find(value);
    if (found == producers.end() ||
        (found->second->kind != Kind::Pure && found->second->kind != Kind::Merge))
      return failure();
    auto &step = *found->second;
    SmallVector<std::string> args;
    for (auto operand : step.operands) {
      auto text = initial(operand);
      if (failed(text))
        return failure();
      args.push_back(std::move(*text));
    }
    return expression(
        step, llvm::find(step.results, value) - step.results.begin(), args);
  }
  void frame(unsigned rule, unsigned instance) {
    for (auto [id, value] : llvm::enumerate(p.getValues()))
      if (value.rule == rule && (rule != NoID || value.instance == instance))
        line("std::optional<" + valueType(id) + "> v" + std::to_string(id) +
             ";");
  }
  LogicalResult prepare(const Step &step);
  LogicalResult schedule(ArrayRef<const Step *> sequence, unsigned owner,
                         bool effects);
  LogicalResult execute(const Step &step);
  SmallVector<const Step *> sequence(const Sequence &s) {
    SmallVector<const Step *> result;
    for (auto &step : s.steps)
      result.push_back(step.get());
    return result;
  }
};
FailureOr<std::string> Emitter::expression(const Step &step, unsigned result,
                                           ArrayRef<std::string> args) {
  Operation *op = step.operation;
  unsigned w = width(p.getValues()[step.results[result]].type);
  if (step.kind == Kind::Merge)
    return "::gfsim::select(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto value = literal(constant.getValue());
    if (value)
      return known(*value, w);
    op->emitError("t-gfsim requires closed literal constant values");
    return failure();
  }
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op)) {
    llvm::APInt value(w, 0), mask(w, 0), z(w, 0);
    for (auto [i, c] : llvm::enumerate(constant.getPattern())) {
      unsigned bit = w - 1 - i;
      if (c == '1')
        value.setBit(bit);
      if (c == '0' || c == '1')
        mask.setBit(bit);
      if (c == 'z')
        z.setBit(bit);
    }
    return "::gfsim::FourState<" + std::to_string(w) + ">::fromMasks(" +
           bits(value, w) + ", " + bits(mask, w) + ", " + bits(z, w) + ")";
  }
  if (isa<ac::BitsUnaryOp>(op))
    return "::gfsim::bit_not(" + args[0] + ")";
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
    auto opcode = llvm::StringSwitch<StringRef>(binary.getOpcode())
                      .Case("and", "bit_and")
                      .Case("or", "bit_or")
                      .Case("xor", "bit_xor")
                      .Default(binary.getOpcode());
    return "::gfsim::" + opcode.str() + "(" + args[0] + ", " + args[1] + ")";
  }
  if (auto compare = dyn_cast<ac::BitsCompareOp>(op)) {
    auto predicate = llvm::StringSwitch<StringRef>(compare.getPredicate())
                         .Case("eq", "EQ")
                         .Case("ne", "NE")
                         .Case("case_eq", "CASE_EQ")
                         .Case("ult", "ULT")
                         .Case("ule", "ULE")
                         .Case("ugt", "UGT")
                         .Case("uge", "UGE")
                         .Case("slt", "SLT")
                         .Case("sle", "SLE")
                         .Case("sgt", "SGT")
                         .Case("sge", "SGE")
                         .Default("");
    return "::gfsim::compare<::gfsim::ComparePredicate::" + predicate.str() +
           ">(" + args[0] + ", " + args[1] + ")";
  }
  if (isa<ac::BitsSelectOp>(op))
    return "::gfsim::select(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
  if (isa<ac::BitsConcatOp>(op)) {
    std::string text = args[0];
    for (unsigned i = 1; i < args.size(); ++i)
      text = "::gfsim::concat(" + text + ", " + args[i] + ")";
    return text;
  }
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = literal(extract.getLow());
    if (!low || low->isNegative() || low->getActiveBits() > 32) {
      op->emitError("t-gfsim requires a closed finite slice offset");
      return failure();
    }
    return "::gfsim::extract<" + std::to_string(w) + ">(" + args[0] + ", " +
           std::to_string(low->getZExtValue()) + ")";
  }
  if (auto resize = dyn_cast<ac::BitsResizeOp>(op)) {
    auto mode = llvm::StringSwitch<StringRef>(resize.getMode())
                    .Case("trunc", "truncate")
                    .Case("zext", "zero_extend")
                    .Case("sext", "sign_extend")
                    .Default("");
    return "::gfsim::" + mode.str() + "<" + std::to_string(w) + ">(" + args[0] +
           ")";
  }
  op->emitError("unsupported pure operation in t-gfsim emitter");
  return failure();
}

LogicalResult Emitter::prepare(const Step &step) {
  if (step.kind == Kind::Merge) {
    auto condition = slot(step.operands[0]);
    line("if (" + path(step.path) + " && " + condition + ") {");
    indentation += 2;
    // Copy only the selected optional: the inactive branch need not have run.
    line(slot(step.results.front()) +
         " = ::pycircuit::transactional_gfsim::CheckedControl(*" + condition +
         ") ? " + slot(step.operands[1]) + " : " + slot(step.operands[2]) + ";");
    indentation -= 2;
    line("}");
    return success();
  }
  bool queueRead = step.kind == Kind::QueueHead || step.kind == Kind::QueuePop;
  if (step.kind != Kind::Pure && step.kind != Kind::StateRead && !queueRead)
    return success();
  if (step.results.empty())
    return success();
  std::string condition = path(step.path) + " && " + available(step.operands);
  if (queueRead)
    condition += " && " + resource(step.resource) + ".CanPop()";
  line("if (" + condition + ") {");
  indentation += 2;
  if (step.kind == Kind::StateRead)
    line(slot(step.results[0]) + " = " + resource(step.resource) + ".Q();");
  else if (queueRead)
    line(slot(step.results[0]) + " = " + resource(step.resource) + ".Head();");
  else {
    SmallVector<std::string> args;
    for (unsigned operand : step.operands)
      args.push_back("*" + slot(operand));
    for (auto [index, result] : llvm::enumerate(step.results)) {
      auto text = expression(step, index, args);
      if (failed(text))
        return failure();
      line(slot(result) + " = " + *text + ";");
    }
  }
  indentation -= 2;
  line("}");
  return success();
}
LogicalResult Emitter::execute(const Step &step) {
  if (step.kind == Kind::Pure || step.kind == Kind::Merge ||
      step.kind == Kind::StateRead || step.kind == Kind::QueueHead ||
      (step.kind == Kind::QueuePop && step.ruleParameter != NoID))
    return success();
  std::string condition = path(step.path) + " && " + available(step.operands);
  if (step.activation != NoID)
    condition += " && " + slot(step.activation) +
                 " && ::pycircuit::transactional_gfsim::CheckedControl(*" +
                 slot(step.activation) + ")";
  line("if (" + condition + ") {");
  indentation += 2;
  if (step.kind == Kind::StateWrite)
    line("static_cast<void>(" + resource(step.resource) +
         ".Propose(context, *" + slot(step.operands[0]) + "));");
  else if (step.kind == Kind::QueuePush)
    line("static_cast<void>(" + resource(step.resource) +
         ".ProposePush(context, *" + slot(step.operands[0]) + "));");
  else if (step.kind == Kind::QueuePop)
    line("static_cast<void>(" + resource(step.resource) +
         ".ProposePop(context));");
  else if (step.kind == Kind::Assert)
    line("if (model.checkAssertions) context.Assert(*" +
         slot(step.operands[0]) + ", \"transactional assertion failed\");");
  else if (step.kind == Kind::Observe) {
    auto observe = cast<ac::SourceObserveOp>(step.operation);
    if (observe.getKind() != "log")
      return observe.emitError(
          "t-gfsim currently supports log observations only");
    auto spec = emissionMetadataJson(observe.getSpec(), observe);
    auto registration = emissionMetadataJson(
        p.getRules()[step.rule].registration->getAttr("ac.origin"), observe);
    auto site = emissionMetadataJson(observe->getAttr("ac.origin"), observe);
    if (failed(spec) || failed(registration) || failed(site))
      return failure();
    std::string instance = "root";
    SmallVector<unsigned> ancestry;
    for (unsigned id = step.instance; p.getInstances()[id].parent != NoID;
         id = p.getInstances()[id].parent)
      ancestry.push_back(id);
    for (unsigned id : llvm::reverse(ancestry))
      instance +=
          "/" + p.getInstances()[id]
                    .declaration->getAttrOfType<StringAttr>("instance_name")
                    .getValue()
                    .str();
    std::string prefix =
        "{\"kind\":" +
        llvm::formatv("{0}", llvm::json::Value(observe.getKind())).str() +
        ",\"instance\":" +
        llvm::formatv("{0}", llvm::json::Value(instance)).str() +
        ",\"registration\":" +
        llvm::formatv("{0}", llvm::json::Value(*registration)).str() +
        ",\"site\":" + llvm::formatv("{0}", llvm::json::Value(*site)).str();
    std::string text =
        "std::string(" + quote(prefix + ",\"evaluation_epoch\":\"") +
        ") + std::to_string(model.epoch) + " + quote("\",\"commit_epoch\":\"") +
        " + std::to_string(model.epoch + 1) + " +
        quote("\",\"spec\":" + *spec + ",\"values\":[");
    for (auto [index, operand] : llvm::enumerate(step.operands)) {
      if (index)
        text += " + \",\"";
      text += " + PayloadJson(*" + slot(operand) + ")";
    }
    line("context.Log(" + text + " + \"]}\");");
  } else
    return step.operation->emitError("unsupported t-gfsim effect");
  indentation -= 2;
  line("}");
  return success();
}
LogicalResult Emitter::schedule(ArrayRef<const Step *> sequence, unsigned owner,
                                bool effects) {
  std::set<unsigned> invoked;
  for (auto *step : sequence) {
    unsigned method = step->methodInvocation;
    if (method != owner) {
      while (method != NoID && p.getMethodInvocations()[method].parent != owner)
        method = p.getMethodInvocations()[method].parent;
      if (method == NoID)
        return step->operation->emitError("inconsistent Method step ownership");
      if (invoked.insert(method).second) {
        const auto &call = p.getMethodInvocations()[method];
        line("model.m" + Twine(call.instance) +
             (effects ? ".Effect_" : ".Method_") + Twine(method) +
             "(context, f);");
      }
    } else if (failed(effects ? execute(*step) : prepare(*step)))
      return failure();
  }
  return success();
}
LogicalResult Emitter::run() {
  for (const auto &resource : p.getResources())
    if (!isa<ac::BitsType>(resource.getPayload()))
      return resource.allocation->emitError(
          "t-gfsim currently supports Bits State and Queue only");
  for (const auto &value : p.getValues())
    if (!isa<ac::BitsType>(value.type))
      return p.getTop().emitError(
          "t-gfsim currently supports Bits payloads only");
  if (!p.getExports().empty())
    return p.getTop().emitError(
        "t-gfsim requires a closed System without exported resources");
  for (const auto &instance : p.getInstances())
    for (const auto &step : instance.comb.steps)
      steps.push_back(step.get());
  for (const auto &rule : p.getRules()) {
    for (const auto &step : rule.guard.steps)
      steps.push_back(step.get());
    for (const auto &step : rule.body.steps)
      steps.push_back(step.get());
  }
  for (const auto *step : steps)
    for (unsigned result : step->results)
      producers[result] = step;
  llvm::sort(steps, [](const Step *a, const Step *b) { return a->id < b->id; });
  line("// Generated directly from verified Linked transactional ACIR.");
  line("#pragma once\n#include <pycircuit/transactional_gfsim/runtime.h>\n#include "
       "<optional>\n#include <string>");
  line("namespace pycircuit_generated {");
  indentation = 2;
  line("struct Model;");
  line(
      R"cpp(template <unsigned W> std::string PayloadJson(const ::gfsim::FourState<W> &value) {
    constexpr char digits[] = "0123456789abcdef";
    auto plane = [&](const auto &bits) {
      std::string text((W + 3) / 4, '0');
      for (unsigned nibble = 0; nibble < text.size(); ++nibble) {
        unsigned v = 0;
        for (unsigned bit = 0; bit < 4 && nibble * 4 + bit < W; ++bit)
          if (bits.bit(nibble * 4 + bit)) v |= 1u << bit;
        text[text.size() - 1 - nibble] = digits[v];
      }
      return text;
    };
    return "{\"kind\":\"bits4\",\"width\":" + std::to_string(W) + ",\"value\":\"" + plane(value.value()) + "\",\"known\":\"" + plane(value.knownMask()) + "\",\"z\":\"" + plane(value.zMask()) + "\"}";
  })cpp");
  for (auto [id, rule] : llvm::enumerate(p.getRules())) {
    line("struct Frame_" + Twine(id) + " {");
    indentation += 2;
    frame(id, rule.instance);
    indentation -= 2;
    line("};");
  }
  for (auto [id, instance] : llvm::enumerate(p.getInstances())) {
    line("struct Module_" + Twine(id) +
         " final : ::pycircuit::transactional_gfsim::Module {");
    indentation += 2;
    line("Model &model;");
    line("struct CombFrame {");
    indentation += 2;
    frame(NoID, id);
    indentation -= 2;
    line("} c;");
    for (unsigned resourceID : instance.resources) {
      const auto &r = p.getResources()[resourceID];
      if (r.isQueue()) {
        line("::pycircuit::transactional_gfsim::Queue<::gfsim::FourState<" +
             Twine(width(r.getPayload())) + ">, " + Twine(r.depth) + "> r" +
             Twine(resourceID) + ";");
        continue;
      }
      auto init = initial(r.initial);
      if (failed(init))
        return r.allocation->emitError("unsupported direct State initializer");
      line("::pycircuit::transactional_gfsim::State<::gfsim::FourState<" +
           Twine(width(r.getPayload())) + ">> r" + Twine(resourceID) + "{" +
           *init + "};");
    }
    line("explicit Module_" + Twine(id) + "(Model &owner) : model(owner) {}");
    line("void CombWork(::pycircuit::transactional_gfsim::Context &) override;");
    line("void Work(::pycircuit::transactional_gfsim::Context &) override;");
    for (auto [methodID, method] : llvm::enumerate(p.getMethodInvocations()))
      if (method.instance == id) {
        line("void Method_" + Twine(methodID) +
             "(::pycircuit::transactional_gfsim::Context &, Frame_" + Twine(method.rule) +
             " &);");
        line("void Effect_" + Twine(methodID) +
             "(::pycircuit::transactional_gfsim::Context &, Frame_" + Twine(method.rule) +
             " &);");
      }
    line("private:");
    for (unsigned rule : instance.rules)
      line("void Rule_" + Twine(rule) + "(::pycircuit::transactional_gfsim::Context &);");
    indentation -= 2;
    line("};");
  }
  line("struct Model {");
  indentation += 2;
  for (auto [id, instance] : llvm::enumerate(p.getInstances()))
    line("Module_" + Twine(id) + " m" + Twine(id) + "{*this};");
  line("::pycircuit::transactional_gfsim::System system;");
  line("std::uint64_t epoch = 0; bool checkAssertions = true;");
  line("Model() {");
  indentation += 2;
  for (auto [id, instance] : llvm::enumerate(p.getInstances())) {
    line("system.Register(m" + Twine(id) + ");");
    for (unsigned resource : instance.resources)
      line("system.Register(m" + Twine(id) + ".r" + Twine(resource) + ");");
  }
  line("system.Reset();");
  indentation -= 2;
  line("}");
  indentation -= 2;
  line("};");
  for (auto [id, instance] : llvm::enumerate(p.getInstances())) {
    line("inline void Module_" + Twine(id) +
         "::CombWork(::pycircuit::transactional_gfsim::Context &context) {");
    indentation += 2;
    line("(void)context; c = {}; auto &f = c; (void)f;");
    if (failed(schedule(sequence(instance.comb), NoID, false)))
      return failure();
    indentation -= 2;
    line("}");
    line("inline void Module_" + Twine(id) +
         "::Work(::pycircuit::transactional_gfsim::Context &context) {");
    indentation += 2;
    line("(void)context;");
    for (unsigned rule : instance.rules)
      line("Rule_" + Twine(rule) + "(context);");
    indentation -= 2;
    line("}");
    for (unsigned ruleID : instance.rules) {
      const auto &rule = p.getRules()[ruleID];
      line("inline void Module_" + Twine(id) + "::Rule_" + Twine(ruleID) +
           "(::pycircuit::transactional_gfsim::Context &context) {");
      indentation += 2;
      line("(void)context; Frame_" + Twine(ruleID) + " f;");
      if (failed(schedule(sequence(rule.guard), NoID, false)) ||
          failed(schedule(sequence(rule.body), NoID, false)))
        return failure();
      std::string fire = path(rule.path);
      for (unsigned guard : rule.guard.yields)
        fire += " && " + slot(guard) +
                " && ::pycircuit::transactional_gfsim::CheckedControl(*" + slot(guard) + ")";
      for (const auto &requirement : rule.requirements)
        fire += " && (!(" + path(requirement.path) + ") || (" +
                slot(requirement.value) +
                " && ::pycircuit::transactional_gfsim::CheckedControl(*" +
                slot(requirement.value) + ")))";
      for (const auto &control : rule.controls)
        line("if (" + path(control.path) + ") { if (!" + slot(control.value) +
             ") return; (void)::pycircuit::transactional_gfsim::CheckedControl(*" +
             slot(control.value) + "); }");
      for (bool reads : {true, false})
        for (const auto &requirement : reads ? rule.queueReads : rule.queueWrites)
          fire += " && (!(" + path(requirement.path) + ") || " +
                  resource(requirement.value) +
                  (reads ? ".CanPop())" : ".CanPush())");
      line("if (" + fire + ") {");
      indentation += 2;
      if (failed(schedule(sequence(rule.body), NoID, true)))
        return failure();
      // Common analysis gives repeated formal reads one logical access.
      // Union their selected paths here, leaving explicit pops distinct and
      // the Runtime's duplicate-proposal checks intact.
      std::map<unsigned, SmallVector<const Step *>> consumptions;
      for (const auto &step : rule.body.steps)
        if (step->kind == Kind::QueuePop && step->ruleParameter != NoID)
          consumptions[step->ruleParameter].push_back(step.get());
      for (const auto &[parameter, demands] : consumptions) {
        std::string selected = "false";
        for (const auto *demand : demands)
          selected += " || (" + path(demand->path) + ")";
        line("if (" + selected + ") static_cast<void>(" +
             resource(demands.front()->resource) + ".ProposePop(context));");
      }
      indentation -= 2;
      line("}");
      indentation -= 2;
      line("}");
    }
  }
  for (auto [id, method] : llvm::enumerate(p.getMethodInvocations())) {
    SmallVector<const Step *> methodSteps;
    for (const Step *step : steps)
      if (step->id >= method.beginStep && step->id < method.endStep)
        methodSteps.push_back(step);
    for (bool effects : {false, true}) {
      line("inline void Module_" + Twine(method.instance) +
           (effects ? "::Effect_" : "::Method_") + Twine(id) +
           "(::pycircuit::transactional_gfsim::Context &context, Frame_" +
           Twine(method.rule) + " &f) {");
      indentation += 2;
      line("(void)context; (void)f;");
      if (failed(schedule(methodSteps, id, effects)))
        return failure();
      indentation -= 2;
      line("}");
    }
  }
  indentation = 0;
  line("} // namespace pycircuit_generated");
  return success();
}
} // namespace
LogicalResult emitTransactionalGfsim(const ac::TransactionalAnalysis &analysis,
                                     raw_ostream &out) {
  std::string text;
  llvm::raw_string_ostream buffer(text);
  if (failed(Emitter(analysis, buffer).run()))
    return failure();
  out << text;
  return success();
}
void emitTransactionalGfsimMain(raw_ostream &out, ArrayAttr samples,
                                bool resetIfOne) {
  out << R"cpp(#include "pycircuit_model.hpp"
#include <charconv>
#include <cstdio>
#include <memory>
#include <iostream>
#include <limits>
#include <string_view>
int main(int argc, char **argv) {
  std::uint64_t cycles = 10;
  bool hasCycles = false, hasWorkers = false, hasEvents = false;
  std::string events = "-";
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 == argc) { std::cerr << "missing simulation option value\n"; return 2; }
    std::string_view key(argv[i]), value(argv[i + 1]);
    if (key == "--cycles" && !hasCycles) {
      auto result = std::from_chars(value.data(), value.data() + value.size(), cycles);
      if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || !cycles || cycles > 4611686018427387903ULL) return 2;
      hasCycles = true;
    } else if (key == "--workers" && !hasWorkers && value == "1") hasWorkers = true;
    else if (key == "--events" && !hasEvents) { events = value; hasEvents = true; }
    else { std::cerr << "unknown, duplicate, or unsupported simulation option\n"; return 2; }
  }
)cpp";
  if (!samples.empty()) {
    out << "  if (hasCycles) { std::cerr << \"explicit --cycles conflicts with "
           "samples\\n\"; return 2; }\n";
    out << "  constexpr std::string_view samples[] = {";
    for (Attribute raw : samples)
      out << quote(cast<StringAttr>(raw).getValue()) << ",";
    out << "};\n  const std::uint64_t epochs = " << samples.size() << ";\n";
  } else
    out << "  const std::uint64_t epochs = cycles * 2;\n";
  out << R"cpp(  try {
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file(nullptr, &std::fclose);
    std::FILE *sink = stdout;
    if (events != "-") {
      file.reset(std::fopen(events.c_str(), "wx"));
      if (!file) { std::cerr << "cannot create event output exclusively\n"; return 2; }
      sink = file.get();
    }
    auto publish = [&](std::string_view message) {
      if (std::fwrite(message.data(), 1, message.size(), sink) != message.size() ||
          std::fputc('\n', sink) == EOF || std::fflush(sink) != 0)
        throw std::runtime_error("event publication failed");
    };
    pycircuit_generated::Model model;
    model.system.SetLogSink(publish);
    bool previousClock = false;
    for (std::uint64_t epoch = 0; epoch < epochs; ++epoch) {
)cpp";
  if (!samples.empty())
    out << "      const bool clock = samples[epoch][0] == '1';\n      const "
           "bool reset = samples[epoch][1] == '1';\n";
  else
    out << "      const bool clock = (epoch & 1u) != 0;\n      const bool "
           "reset = false;\n";
  out << R"cpp(      const bool rising = !previousClock && clock;
      model.epoch = epoch;
      model.checkAssertions = !reset;
      if (!model.system.Sample(rising, reset)) {
        std::cerr << "sample " << epoch << " failed with zero commit: " << model.system.Failure() << '\n';
        publish("{\"kind\":\"result\",\"status\":\"FAILED\",\"epoch_time\":\"" + std::to_string(epoch) + "\",\"statistics\":{},\"error\":{\"code\":\"transaction_failed\",\"message\":\"transactional sample failed\",\"phase\":\"check\",\"instance\":null,\"source\":null,\"check_id\":null},\"reset_semantics\":\")cpp"
      << (resetIfOne ? "if_one" : "strict") << R"cpp(\"}");
        if (auto error = model.system.Exception()) std::rethrow_exception(error);
        return 1;
      }
      if (auto error = model.system.PublishException()) {
        std::cerr << "event publication failed after sample " << epoch << " committed\n";
        std::rethrow_exception(error);
      }
      previousClock = clock;
    }
    publish("{\"kind\":\"result\",\"status\":\"TERMINATED\",\"epoch_time\":\"" + std::to_string(epochs) + "\",\"statistics\":{},\"error\":null,\"reset_semantics\":\")cpp"
      << (resetIfOne ? "if_one" : "strict") << R"cpp(\"}");
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
  return 0;
}
)cpp";
}
} // namespace acir::compiler

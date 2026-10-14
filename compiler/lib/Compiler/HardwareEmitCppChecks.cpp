#include "HardwareEmitCppChecks.h"
#include "FinalCppNames.h"
#include "llvm/ADT/STLExtras.h"
#include <limits>

using namespace mlir;
namespace acir::compiler {
namespace {
std::string literal(StringRef value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << '"';
  out.write_escaped(value, false);
  out << '"';
  out.flush();
  return text;
}
std::string viewLiteral(StringRef value) {
  return "std::string_view{" + literal(value) + ", " +
         std::to_string(value.size()) + "}";
}
FailureOr<std::string> jsonLiteral(Attribute value, Operation *site) {
  auto converted = emissionMetadataJson(value, site);
  if (failed(converted))
    return failure();
  return viewLiteral(*converted);
}
bool resetSnapshot(const HardwareEmitContext &context,
                   ac::ModuleOp definition) {
  auto domain = definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  return context.sourceChecks().hasChecks() && domain && !domain.empty();
}
struct Address {
  std::string object = "pyc_implementation", suffix;
  uint64_t lane = 0;
};
FailureOr<Address> address(HardwareEmitContext &context, uint64_t ownerIndex) {
  const auto &plan = context.sourceChecks();
  if (ownerIndex >= plan.owners.size())
    return context.package.emitOpError()
           << "native check owner is outside plan";
  Address result;
  auto scope = plan.owners.front().bindings;
  for (const auto &step : plan.owners[ownerIndex].path) {
    auto raw = step.allocation->getAttrOfType<StringAttr>("instance_name");
    auto name = legalizeIdentifier(
        raw.getValue(), [&] { return step.allocation->emitOpError(); });
    if (failed(name))
      return failure();
    result.object += "->pyc_instance_" + *name;
    result.suffix += "." + raw.getValue().str();
    auto child = isa<ac::InstanceOp>(step.allocation)
                     ? context.analysis.bindInstance(
                           cast<ac::InstanceOp>(step.allocation), scope)
                     : context.analysis.bindInstance(
                           cast<ac::CollectionOp>(step.allocation), scope);
    if (failed(child))
      return failure();
    if (auto collection = dyn_cast<ac::CollectionOp>(step.allocation)) {
      auto shape = context.analysis.resolveShape(collection.getShape(), scope,
                                                 collection);
      if (failed(shape) || shape->size() != step.coordinates.size())
        return failure();
      uint64_t count = 1, coordinate = 0;
      for (auto [extent, index] : llvm::zip(*shape, step.coordinates)) {
        if (index >= extent ||
            count > std::numeric_limits<uint64_t>::max() / extent ||
            coordinate >
                (std::numeric_limits<uint64_t>::max() - index) / extent)
          return collection.emitOpError() << "native check lane index overflow";
        count *= extent;
        coordinate = coordinate * extent + index;
        result.suffix += "[" + std::to_string(index) + "]";
      }
      if (result.lane >
          (std::numeric_limits<uint64_t>::max() - coordinate) / count)
        return collection.emitOpError() << "native check lane index overflow";
      result.lane = result.lane * count + coordinate;
    }
    scope = std::move(*child);
  }
  return result;
}
FailureOr<size_t> localIndex(HardwareEmitContext &context, uint64_t global) {
  const auto &plan = context.sourceChecks();
  if (global >= plan.bindings.size())
    return context.package.emitOpError()
           << "native check binding is outside plan";
  const auto &binding = plan.bindings[global];
  for (auto site : cppDefinitionChecks(context, binding.definition))
    if (site.binding == &binding)
      return site.ordinal;
  return context.package.emitOpError()
         << "native check binding has no local site";
}
FailureOr<uint64_t> samplingIndex(HardwareEmitContext &context,
                                  uint64_t occurrenceIndex) {
  const auto &plan = context.sourceChecks();
  if (occurrenceIndex >= plan.samplingOccurrences.size())
    return context.package.emitOpError()
           << "native sampling occurrence is outside plan";
  auto bindingIndex = plan.samplingOccurrences[occurrenceIndex].bindingIndex;
  if (bindingIndex >= plan.samplingBindings.size())
    return context.package.emitOpError()
           << "native sampling binding is outside plan";
  return plan.samplingBindings[bindingIndex].localOrdinal;
}
} // namespace
SmallVector<CppSourceCheckSite>
cppDefinitionChecks(const HardwareEmitContext &context,
                    ac::ModuleOp definition) {
  SmallVector<CppSourceCheckSite> result;
  for (const auto &binding : context.sourceChecks().bindings)
    if (binding.definition == definition)
      result.push_back({&binding, result.size()});
  return result;
}
void emitCppCheckStorage(HardwareEmitContext &context, ac::ModuleOp definition,
                         raw_ostream &out) {
  auto sites = cppDefinitionChecks(context, definition);
  bool reset = resetSnapshot(context, definition);
  bool samples = llvm::any_of(
      context.sourceChecks().samplingBindings,
      [&](const auto &binding) { return binding.definition == definition; });
  if (sites.empty() && !reset && !samples)
    return;
  out << "private:\n";
  for (auto site : sites) {
    for (StringRef operand : {"condition", "path"})
      out << "  gfsim::wire<gfsim::table<gfsim::Bits<1>, pyc_count>> "
          << "pyc_check_" << operand << "_" << site.ordinal << ";\n";
    out << "  std::array<bool, pyc_count> pyc_check_valid_" << site.ordinal
        << "{};\n";
  }
  if (reset)
    out << "  gfsim::wire<gfsim::table<gfsim::Bits<1>, pyc_count>> "
           "pyc_check_reset_;\n"
           "  std::array<bool, pyc_count> pyc_check_reset_valid_{};\n";
  for (const auto &binding : context.sourceChecks().samplingBindings) {
    if (binding.definition != definition)
      continue;
    auto ordinal = binding.localOrdinal;
    for (StringRef field : {"clock", "pending_clock", "pending_valid", "valid"})
      out << "  std::array<bool, pyc_count> pyc_sampling_" << field << "_"
          << ordinal << "{};\n";
    out << "  std::array<gfsim::SampledClockEvaluation, pyc_count> "
           "pyc_sampling_evaluation_"
        << ordinal << "{};\n";
  }
  out << "public:\n";
  for (const auto &binding : context.sourceChecks().samplingBindings) {
    if (binding.definition != definition)
      continue;
    auto ordinal = binding.localOrdinal;
    out << "  void __pyc_capture_sampling_" << ordinal
        << "(std::size_t lane, const gfsim::wire<gfsim::Bits<1>> &clock, "
           "const gfsim::wire<gfsim::Bits<1>> &reset) noexcept {\n"
        << "    const auto evaluation = gfsim::evaluateSampledClock("
           "pyc_sampling_clock_"
        << ordinal << "[lane], clock, reset);\n"
        << "    pyc_sampling_evaluation_" << ordinal << "[lane] = evaluation;\n"
        << "    pyc_sampling_pending_clock_" << ordinal
        << "[lane] = evaluation.nextClock;\n"
        << "    pyc_sampling_pending_valid_" << ordinal
        << "[lane] = evaluation.fault == gfsim::SampledClockFault::None;\n"
        << "    pyc_sampling_valid_" << ordinal << "[lane] = true;\n  }\n"
        << "  bool __pyc_sampling_valid_" << ordinal
        << "(std::size_t lane) const noexcept { return pyc_sampling_valid_"
        << ordinal << "[lane]; }\n"
        << "  const gfsim::SampledClockEvaluation &__pyc_sampling_evaluation_"
        << ordinal
        << "(std::size_t lane) const noexcept { return "
           "pyc_sampling_evaluation_"
        << ordinal << "[lane]; }\n";
  }
  for (auto site : sites) {
    for (StringRef operand : {"condition", "path"})
      out << "  const gfsim::wire<gfsim::Bits<1>> &__pyc_check_" << operand
          << "_" << site.ordinal
          << "(std::size_t lane) const noexcept { return pyc_check_" << operand
          << "_" << site.ordinal << ".element(lane); }\n";
    out << "  void __pyc_capture_check_" << site.ordinal
        << "(std::size_t lane, const gfsim::wire<gfsim::Bits<1>> &condition, "
           "const gfsim::wire<gfsim::Bits<1>> &path) noexcept { "
        << "pyc_check_condition_" << site.ordinal
        << ".element(lane) = condition; "
        << "pyc_check_path_" << site.ordinal << ".element(lane) = path; "
        << "pyc_check_valid_" << site.ordinal << "[lane] = true; }\n";
    out << "  bool __pyc_check_valid_" << site.ordinal
        << "(std::size_t lane) const noexcept { return pyc_check_valid_"
        << site.ordinal << "[lane]; }\n";
  }
  if (reset)
    out << "  const gfsim::wire<gfsim::Bits<1>> &__pyc_check_reset("
           "std::size_t lane) const noexcept { return "
           "pyc_check_reset_.element(lane); }\n"
           "  void __pyc_capture_reset(std::size_t lane, const "
           "gfsim::wire<gfsim::Bits<1>> &reset) noexcept { "
           "pyc_check_reset_.element(lane) = reset; "
           "pyc_check_reset_valid_[lane] = true; }\n"
           "  bool __pyc_check_reset_valid(std::size_t lane) const noexcept { "
           "return pyc_check_reset_valid_[lane]; }\n";
}
void emitCppCheckClear(HardwareEmitContext &context, ac::ModuleOp definition,
                       raw_ostream &out) {
  for (auto site : cppDefinitionChecks(context, definition))
    out << "    pyc_check_valid_" << site.ordinal << ".fill(false);\n";
  if (resetSnapshot(context, definition))
    out << "    pyc_check_reset_valid_.fill(false);\n";
  for (const auto &binding : context.sourceChecks().samplingBindings)
    if (binding.definition == definition)
      out << "    pyc_sampling_valid_" << binding.localOrdinal
          << ".fill(false);\n    pyc_sampling_pending_valid_"
          << binding.localOrdinal << ".fill(false);\n";
}
void emitCppSamplingXfer(HardwareEmitContext &context, ac::ModuleOp definition,
                         raw_ostream &out) {
  for (const auto &binding : context.sourceChecks().samplingBindings) {
    if (binding.definition != definition)
      continue;
    auto ordinal = binding.localOrdinal;
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < pyc_count; "
           "++pyc_lane)\n"
        << "      if (pyc_sampling_pending_valid_" << ordinal
        << "[pyc_lane]) pyc_sampling_clock_" << ordinal
        << "[pyc_lane] = pyc_sampling_pending_clock_" << ordinal
        << "[pyc_lane];\n";
  }
}
void emitCppSamplingReset(HardwareEmitContext &context, ac::ModuleOp definition,
                          raw_ostream &out) {
  for (const auto &binding : context.sourceChecks().samplingBindings)
    if (binding.definition == definition)
      out << "    pyc_sampling_pending_clock_" << binding.localOrdinal
          << ".fill(false);\n    pyc_sampling_pending_valid_"
          << binding.localOrdinal << ".fill(true);\n";
}
LogicalResult
emitCppCheckCapture(HardwareEmitContext &context, ac::ModuleOp definition,
                    StringRef object, StringRef count,
                    const std::function<FailureOr<std::string>(Value)> &value,
                    raw_ostream &out) {
  for (const auto &binding : context.sourceChecks().samplingBindings) {
    if (binding.definition != definition)
      continue;
    auto clock = value(binding.clock);
    auto reset = value(binding.reset);
    if (failed(clock) || failed(reset))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << count
        << "; ++pyc_lane) " << object << "__pyc_capture_sampling_"
        << binding.localOrdinal << "(pyc_lane, " << *clock
        << ".element(pyc_lane), " << *reset << ".element(pyc_lane));\n";
  }
  if (resetSnapshot(context, definition)) {
    auto domain = definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    auto ordinal = domain.getAs<IntegerAttr>("reset").getValue().getZExtValue();
    auto reset = value(definition.getBody().front().getArgument(ordinal));
    if (failed(reset))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << count
        << "; ++pyc_lane) " << object << "__pyc_capture_reset(pyc_lane, "
        << *reset << ".element(pyc_lane));\n";
  }
  for (auto site : cppDefinitionChecks(context, definition)) {
    auto condition = value(site.binding->condition);
    auto path = value(site.binding->path);
    if (failed(condition) || failed(path))
      return failure();
    out << "    for (std::size_t pyc_lane = 0; pyc_lane < " << count
        << "; ++pyc_lane) " << object << "__pyc_capture_check_" << site.ordinal
        << "(pyc_lane, " << *condition << ".element(pyc_lane), " << *path
        << ".element(pyc_lane));\n";
  }
  return success();
}
bool cppHasRootChecks(const HardwareEmitContext &context,
                      ac::ModuleOp definition) {
  const auto &plan = context.sourceChecks();
  return (plan.hasChecks() || !plan.samplingOccurrences.empty()) &&
         plan.owners.front().definition == definition;
}
LogicalResult emitCppCheckInstanceNames(HardwareEmitContext &context,
                                        raw_ostream &out) {
  for (const auto &check : context.sourceChecks().checks) {
    auto owner = address(context, check.ownerOccurrence);
    if (failed(owner))
      return failure();
    out << "    pyc_check_instances_[" << check.ordinal
        << "] = std::string(this->name()) + " << literal(owner->suffix)
        << ";\n";
  }
  return success();
}
LogicalResult emitCppRootCheckValidator(HardwareEmitContext &context,
                                        raw_ostream &out) {
  const auto &plan = context.sourceChecks();
  out << "  bool __pyc_validate_checks(gfsim::SimFailureInfo &failure) const "
         "noexcept {\n    failure = {};\n";
  // Missing Work/context completion is a model Check-phase failure, not a
  // source predicate failure. Validate the whole closure before selecting any
  // SourceExpect result, so an earlier false condition cannot hide a missing
  // later snapshot.
  for (const auto &check : plan.checks) {
    auto owner = address(context, check.ownerOccurrence);
    auto local = localIndex(context, check.bindingIndex);
    if (failed(owner) || failed(local))
      return failure();
    std::string complete = owner->object + "->__pyc_check_valid_" +
                           std::to_string(*local) + "(" +
                           std::to_string(owner->lane) + ")";
    if (!check.samplingOccurrenceIndex &&
        check.reset.kind == ac::HardwareCheckResetKind::PhysicalReset) {
      auto resetOwner = address(context, *check.reset.ownerOccurrence);
      if (failed(resetOwner))
        return failure();
      complete += " && " + resetOwner->object + "->__pyc_check_reset_valid(" +
                  std::to_string(resetOwner->lane) + ")";
    }
    out << "    if (!(" << complete << ")) {\n"
        << "      failure = {gfsim::SimFailurePhase::Check, "
        << viewLiteral("runtime_failure") << ", "
        << viewLiteral("model check failed")
        << ", {}, {}, {}};\n      return false;\n    }\n";
  }
  for (const auto &sampling : plan.samplingOccurrences) {
    auto owner = address(context, sampling.ownerOccurrence);
    auto local = samplingIndex(context, sampling.ordinal);
    if (failed(owner) || failed(local))
      return failure();
    out << "    if (!(" << owner->object << "->__pyc_sampling_valid_" << *local
        << "(" << owner->lane << "))) {\n"
        << "      failure = {gfsim::SimFailurePhase::Check, "
        << viewLiteral("runtime_failure") << ", "
        << viewLiteral("model check failed")
        << ", {}, {}, {}};\n      return false;\n    }\n";
  }
  // Physical faults precede every source guard. All evaluations are frozen by
  // Work, including check-only transactions without any storage leaf.
  for (const auto &sampling : plan.samplingOccurrences) {
    auto owner = address(context, sampling.ownerOccurrence);
    auto local = samplingIndex(context, sampling.ordinal);
    if (failed(owner) || failed(local))
      return failure();
    out << "    {\n      const auto &pyc_sampling = " << owner->object
        << "->__pyc_sampling_evaluation_" << *local << "(" << owner->lane
        << ");\n"
        << "      if (pyc_sampling.fault != gfsim::SampledClockFault::None) "
           "{\n"
        << "        failure = {gfsim::SimFailurePhase::Check, "
        << viewLiteral("runtime_failure")
        << ", pyc_sampling.fault == gfsim::SampledClockFault::UnknownClock ? "
        << viewLiteral("sampled clock is unknown") << " : "
        << viewLiteral("sampled reset is unknown")
        << ", {}, {}, {}};\n        return false;\n      }\n    }\n";
  }
  // Guard faults precede ordinary and body assertions. Preserve the
  // validated occurrence/registration order within each phase.
  for (bool guardPhase : {true, false}) {
    for (const auto &check : plan.checks) {
      const auto &binding = plan.bindings[check.bindingIndex];
      if ((binding.kind.getValue() == "guard_known") != guardPhase)
        continue;
      auto owner = address(context, check.ownerOccurrence);
      auto local = localIndex(context, check.bindingIndex);
      if (failed(owner) || failed(local))
        return failure();
      auto location = jsonLiteral(binding.location, binding.expect);
      auto id = jsonLiteral(binding.checkID, binding.expect);
      if (failed(location) || failed(id))
        return failure();
      std::string message =
          binding.message
              ? binding.message.getValue().str()
              : "source " + binding.kind.getValue().str() + " check failed";
      auto lane = std::to_string(owner->lane);
      auto site = std::to_string(*local);
      if (binding.kind.getValue() == "guard_known") {
        if (!check.samplingOccurrenceIndex)
          return binding.expect->emitOpError()
                 << "native guard check has no sampling occurrence";
        auto samplingLocal =
            samplingIndex(context, *check.samplingOccurrenceIndex);
        if (failed(samplingLocal))
          return failure();
        const auto &sampling =
            plan.samplingOccurrences[*check.samplingOccurrenceIndex];
        auto samplingOwner = address(context, sampling.ownerOccurrence);
        if (failed(samplingOwner))
          return failure();
        out << "    {\n      const auto &pyc_reached = " << owner->object
            << "->__pyc_check_path_" << site << "(" << lane << ");\n"
            << "      if (!pyc_reached.isFullyKnown()) {\n"
            << "        failure = {gfsim::SimFailurePhase::Check, "
            << viewLiteral("runtime_failure") << ", "
            << viewLiteral("sampled guard reachability is unknown")
            << ", {}, {}, {}};\n        return false;\n      }\n"
            << "      const auto &pyc_sampling = " << samplingOwner->object
            << "->__pyc_sampling_evaluation_" << *samplingLocal << "("
            << samplingOwner->lane << ");\n"
            << "      if (pyc_sampling.rising && !pyc_sampling.resetActive && "
               "pyc_reached.value().toBool() && !"
            << owner->object << "->__pyc_check_condition_" << site << "(" << lane
            << ").isFullyKnown()) {\n"
            << "        failure = {gfsim::SimFailurePhase::Check, "
            << viewLiteral("source_check_failed") << ", " << viewLiteral(message)
            << ", pyc_check_instances_[" << check.ordinal << "], " << *location
            << ", " << *id << "};\n        return false;\n      }\n    }\n";
        continue;
      }
      std::string reset = "gfsim::FourState<1>::known(gfsim::Bits<1>{0})";
      if (check.reset.kind == ac::HardwareCheckResetKind::PhysicalReset) {
        auto resetOwner = address(context, *check.reset.ownerOccurrence);
        if (failed(resetOwner))
          return failure();
        auto resetLane = std::to_string(resetOwner->lane);
        reset = resetOwner->object + "->__pyc_check_reset(" + resetLane +
                ").packed()";
      }
      out << "    {\n      const auto pyc_error = gfsim::bit_and("
             "gfsim::bit_and("
          << owner->object << "->__pyc_check_path_" << site << "(" << lane
          << ").packed(), gfsim::bit_not(" << reset << ")), gfsim::bit_not("
          << owner->object << "->__pyc_check_condition_" << site << "(" << lane
          << ").packed()));\n"
          << "      if (!pyc_error.isFullyKnown() || "
             "pyc_error.value() != gfsim::Bits<1>{0}) {\n"
          << "        failure = {gfsim::SimFailurePhase::Check, "
          << viewLiteral("source_check_failed") << ", " << viewLiteral(message)
          << ", pyc_check_instances_[" << check.ordinal << "], " << *location
          << ", " << *id << "};\n        return false;\n      }\n    }\n";
    }
  }
  out << "    return true;\n  }\n";
  return success();
}
} // namespace acir::compiler

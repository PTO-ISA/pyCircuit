#include "ComposedObservations.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"
using namespace mlir;
namespace {
std::string identity(Attribute value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  value.print(out);
  return text;
}
llvm::json::Value json(Attribute attr) {
  if (auto value = dyn_cast<StringAttr>(attr))
    return value.getValue();
  if (auto value = dyn_cast<IntegerAttr>(attr))
    return value.getValue().getZExtValue();
  if (auto value = dyn_cast<ArrayAttr>(attr)) {
    llvm::json::Array result;
    for (auto item : value)
      result.push_back(json(item));
    return result;
  }
  if (auto value = dyn_cast<DictionaryAttr>(attr)) {
    llvm::json::Object result;
    for (auto item : value)
      result[item.getName().strref()] = json(item.getValue());
    return result;
  }
  return nullptr;
}
} // namespace
FailureOr<llvm::json::Object>
composedObservation(const acir::compiler::FinalProgram &program,
                    llvm::StringRef tag, uint64_t ordinal, uint64_t epoch,
                    uint64_t valueKind, llvm::StringRef bits, uint64_t owner,
                    uint64_t registration, uint64_t site,
                    acir::ac::detail::EmitError error) {
  const acir::compiler::ObservationBinding *binding = nullptr;
  for (const auto &candidate : program.observations().bindings)
    if (candidate.stableOrdinal == ordinal) {
      if (binding)
        return error() << "duplicate observation metadata ordinal";
      binding = &candidate;
    }
  if (!binding || !epoch || binding->values.size() != 1 ||
      binding->valueConstraints.size() != 1)
    return error() << "runtime observation has no exact source metadata";
  uint64_t expectedOwner = UINT64_MAX, expectedRegistration = 0;
  for (const auto &instance : program.instances())
    if (instance.view == binding->owner)
      expectedOwner = instance.ordinal;
  for (auto rule :
       binding->owner->module.getBody().front().getOps<acir::ac::RuleOp>()) {
    if (rule == binding->rule)
      break;
    ++expectedRegistration;
  }
  bool gauge = binding->kind.getValue() == "report";
  if (owner != expectedOwner || registration != expectedRegistration ||
      site != binding->requiredIndex || (tag == "G") != gauge)
    return error() << "runtime observation descriptor identity was redirected";
  auto constraint = cast<DictionaryAttr>(binding->valueConstraints[0]);
  auto logical = constraint.getAs<DictionaryAttr>("type");
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  uint64_t value;
  if (!kind || bits.getAsInteger(10, value))
    return error() << "runtime observation value is malformed";
  llvm::json::Object scalar;
  if (kind.getValue() == "bool") {
    if (valueKind != 0 || value > 1)
      return error() << "runtime bool observation is invalid";
    scalar["kind"] = "bool";
    scalar["value"] = value != 0;
  } else if (kind.getValue() == "integer") {
    auto interpretation = logical.getAs<StringAttr>("interpretation");
    if (!interpretation ||
        (interpretation.getValue() == "unsigned" ? valueKind != 2
                                                 : valueKind != 1))
      return error() << "runtime integer interpretation changed";
    scalar["kind"] = "integer";
    scalar["value"] = valueKind == 1
                          ? std::to_string(static_cast<int64_t>(value))
                          : bits.str();
  } else
    return error() << "unsupported observation value type";
  return llvm::json::Object{
      {"kind", binding->kind.getValue()},
      {"instance", identity(binding->ownerRef)},
      {"registration", identity(binding->registration)},
      {"site", identity(binding->observationID.getAs<DictionaryAttr>("site"))},
      {"evaluation_epoch", std::to_string(epoch - 1)},
      {"commit_epoch", std::to_string(epoch)},
      {"spec", json(binding->spec)},
      {"values", llvm::json::Array{std::move(scalar)}}};
}
LogicalResult
verifyComposedCompletion(const acir::compiler::FinalProgram &program,
                         const llvm::json::Object &run,
                         acir::ac::detail::EmitError error) {
  auto observations = run.getArray("observations");
  auto events = run.getArray("events"), gauges = run.getArray("gauges");
  if (!observations || !events || !gauges ||
      observations->size() != program.observations().bindings.size())
    return error() << "fixture completion observation inventory is missing";
  llvm::DenseSet<uint64_t> ordinals;
  for (const auto *rows : {events, gauges})
    for (const auto &raw : *rows) {
      const auto *row = raw.getAsObject();
      auto ordinal = row ? row->getInteger("ordinal") : std::nullopt;
      if (!ordinal || !ordinals.insert(*ordinal).second)
        return error() << "fixture completion observation is duplicated";
    }
  bool completed = false;
  for (const auto &raw : *observations) {
    auto row = raw.getAsObject();
    auto kind = row ? row->getString("kind") : std::nullopt;
    auto spec = row ? row->getObject("spec") : nullptr;
    auto name = spec ? spec->getString("name") : std::nullopt;
    if (kind && *kind == "report" && name && *name == "completed") {
      auto values = row->getArray("values");
      auto value =
          values && values->size() == 1 ? (*values)[0].getAsObject() : nullptr;
      completed = value && value->getString("value") == llvm::StringRef("1");
    }
  }
  return completed ? success()
                   : error() << "fixture completion gauge is absent or false";
}

mlir::FailureOr<llvm::json::Array>
composedRtlStatistics(const acir::compiler::FinalProgram &program,
                      uint64_t epoch, const llvm::json::Array &gauges,
                      acir::ac::detail::EmitError error) {
  auto row = [](llvm::StringRef owner, llvm::StringRef name,
                llvm::StringRef kind, uint64_t value, uint64_t time) {
    return llvm::json::Object{
        {"buckets", llvm::json::Array{}},
        {"count", 0},
        {"kind", kind},
        {"last_update", llvm::json::Object{{"delta", 0}, {"time", time}}},
        {"maximum", 0},
        {"minimum", 0},
        {"name", name},
        {"object_path", owner},
        {"sum", 0},
        {"value", value}};
  };
  llvm::json::Array result;
  result.push_back(row("@runtime", "cycles", "counter", epoch, epoch));
  result.push_back(row("@runtime", "stop_reason", "gauge", 1, epoch));
  for (const auto &observation : program.observations().bindings) {
    if (observation.kind.getValue() != "report")
      continue;
    uint64_t value = 0, time = 0;
    for (const auto &raw : gauges) {
      auto item = raw.getAsObject();
      auto ordinal = item ? item->getInteger("ordinal") : std::nullopt;
      if (!ordinal || uint64_t(*ordinal) != observation.stableOrdinal)
        continue;
      auto text = item->getString("value");
      auto update = item->getInteger("epoch");
      if (!text || !update || text->getAsInteger(10, value) || *update < 0 ||
          uint64_t(*update) > epoch)
        return error() << "RTL gauge snapshot is invalid";
      time = *update;
    }
    result.push_back(
        row(identity(observation.ownerRef),
            observation.spec.getAs<mlir::StringAttr>("name").getValue(),
            "gauge", value, time));
  }
  llvm::sort(
      result, [](const llvm::json::Value &a, const llvm::json::Value &b) {
        auto x = a.getAsObject(), y = b.getAsObject();
        return std::pair(*x->getString("object_path"), *x->getString("name")) <
               std::pair(*y->getString("object_path"), *y->getString("name"));
      });
  return std::move(result);
}

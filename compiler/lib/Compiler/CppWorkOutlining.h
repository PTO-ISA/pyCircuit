#ifndef PYCIRCUIT_CPP_WORK_OUTLINING_H
#define PYCIRCUIT_CPP_WORK_OUTLINING_H
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace acir::compiler {
// This is a code-size target between complete statement groups, not a limit
// on an indivisible scalar expression or region evaluator.
class CppWorkOutlining {
public:
  static constexpr std::size_t outlineTriggerBytes = 256 * 1024;
  static constexpr std::size_t partBytes = 8 * 1024;
  static constexpr std::size_t pageFields = 32;
  static constexpr std::size_t dispatchFanout = 32;
  CppWorkOutlining() : stream(pending) {}

  struct PlaneReference {
    std::string storage, value;
  };
  PlaneReference plane(llvm::StringRef type, llvm::StringRef name,
                       bool borrowedConstant = false) {
    auto page = planes.size() / pageFields;
    planes.push_back({type.str(), name.str(), borrowedConstant});
    std::string value =
        "pyc_values.page_" + std::to_string(page) + "." + name.str();
    return {value, borrowedConstant ? "(*" + value + ")" : value};
  }
  // Called only at emission-owned statement boundaries, never by inspecting
  // generated C++ syntax. Region-local temporaries remain in one group.
  void finishStatement() {
    stream.flush();
    if (pending.empty())
      return;
    maxStatementBytes = std::max(maxStatementBytes, pending.size());
    if (parts.empty() || (!parts.back().empty() &&
                          parts.back().size() + pending.size() > partBytes))
      parts.emplace_back();
    parts.back() += pending;
    pending.clear();
  }
  void emit(llvm::raw_ostream &out, bool known = false,
            llvm::StringRef guard = {}) {
    std::string prefix = known ? "pyc_known_work" : "pyc_work";
    std::string frame = prefix + "_frame";
    out << "private:\n  // Work statement target: " << partBytes
        << " bytes; largest indivisible group: " << maxStatementBytes
        << " bytes.\n";
    for (std::size_t start = 0; start < planes.size(); start += pageFields) {
      auto page = start / pageFields;
      out << "  struct " << prefix << "_page_" << page << " {\n";
      for (std::size_t i = start;
           i < std::min(start + pageFields, planes.size()); ++i) {
        auto &plane = planes[i];
        out << "    " << (plane.constant ? "const " : "") << plane.type
            << (plane.constant ? " *" : " ") << plane.name
            << (plane.constant ? " = nullptr" : "") << ";\n";
      }
      // Each constructor initializes at most one page. A single large frame
      // constructor must not become the new unbounded generated function.
      out << "    pyc_work_noinline " << prefix << "_page_" << page
          << "() {}\n  };\n";
    }
    out << "  struct " << frame << " {\n";
    for (std::size_t start = 0; start < planes.size(); start += pageFields) {
      auto page = start / pageFields;
      out << "    " << prefix << "_page_" << page << " page_" << page << ";\n";
    }
    out << "  };\n";
    std::vector<std::string> calls;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      auto name = prefix + "_part_" + std::to_string(i);
      out << "  pyc_work_noinline void " << name << "(" << frame
          << " &pyc_values) {\n"
          << parts[i] << "  }\n";
      calls.push_back(name);
    }
    // Preserve order, while also bounding call-only dispatch functions.
    for (std::size_t level = 0; calls.size() > dispatchFanout; ++level) {
      std::vector<std::string> next;
      for (std::size_t start = 0; start < calls.size();
           start += dispatchFanout) {
        auto name = prefix + "_dispatch_" + std::to_string(level) + "_" +
                    std::to_string(start / dispatchFanout);
        out << "  pyc_work_noinline void " << name << "(" << frame
            << " &pyc_values) {\n";
        for (std::size_t i = start;
             i < std::min(start + dispatchFanout, calls.size()); ++i)
          out << "    " << calls[i] << "(pyc_values);\n";
        out << "  }\n";
        next.push_back(name);
      }
      calls = std::move(next);
    }
    if (known) {
      out << "  void pyc_run_known_work() {\n";
    } else {
      out << "public:\n  void Work() {\n    pyc_clear_check_snapshots();\n"
             "    try {\n";
      if (!guard.empty())
        out << "      if (" << guard << ") return pyc_run_known_work();\n";
    }
    out << "      auto pyc_values = std::make_unique_for_overwrite<" << frame
        << ">();\n";
    for (auto &name : calls)
      out << "      " << name << "(*pyc_values);\n";
    if (!known)
      out << "    } catch (...) { DiscardNext(); throw; }\n";
    out << "  }\n";
  }

private:
  std::string pending;

public:
  llvm::raw_string_ostream stream;

private:
  struct Plane {
    std::string type, name;
    bool constant;
  };
  std::vector<std::string> parts;
  std::vector<Plane> planes;
  std::size_t maxStatementBytes = 0;
};

inline llvm::StringRef cppWorkRuntimeSupport() {
  return R"cpp(
#if defined(_MSC_VER)
#define pyc_work_noinline __declspec(noinline)
#else
#define pyc_work_noinline __attribute__((noinline))
#endif

template <std::size_t OutputLanes, std::size_t InputLanes, class Plane>
struct pyc_work_broadcast_view {
  const Plane &source;
  decltype(auto) element(std::size_t lane) const noexcept {
    return source.element(lane / OutputLanes * InputLanes + lane % InputLanes);
  }
};

template <std::size_t OutputLanes, std::size_t InputLanes, class Plane>
auto pyc_work_broadcast(const Plane &source) noexcept {
  return pyc_work_broadcast_view<OutputLanes, InputLanes, Plane>{source};
}

template <class Expression>
struct pyc_work_expression_view {
  Expression expression;
  auto element(std::size_t lane) const noexcept { return expression(lane); }
};

template <class Condition, class True, class False>
auto pyc_work_select_lane(const Condition &condition, const True &whenTrue,
                          const False &whenFalse, std::size_t lane) noexcept {
  const auto guard = condition.element(lane).packed();
  if (guard.isFullyKnown())
    return guard.value().toBool() ? whenTrue.element(lane)
                                  : whenFalse.element(lane);
  using Wire = std::remove_cvref_t<decltype(whenTrue.element(lane))>;
  return Wire::fromPacked(select(guard, whenTrue.element(lane).packed(),
                                        whenFalse.element(lane).packed()));
}

template <class Output, class Condition, class True, class False>
void pyc_work_select(Output &out, const Condition &condition,
                     const True &whenTrue, const False &whenFalse) noexcept {
  for (std::size_t lane = 0; lane < Output::size; ++lane)
    out.element(lane) = pyc_work_select_lane(condition, whenTrue, whenFalse, lane);
}

template <std::size_t Group, class Output, class Condition, class True,
          class False>
void pyc_work_select(Output &out,
                     const pyc_work_broadcast_view<Group, 1, Condition> &condition,
                     const True &whenTrue, const False &whenFalse) noexcept {
  static_assert(Output::size % Group == 0);
  using Wire = std::remove_cvref_t<decltype(out.element(0))>;
  for (std::size_t first = 0; first < Output::size; first += Group) {
    const auto guard = condition.source.element(first / Group).packed();
    if (guard.isFullyKnown()) {
      const auto copy = [&](const auto &source) {
        for (std::size_t lane = first; lane < first + Group; ++lane)
          out.element(lane) = source.element(lane);
      };
      if (guard.value().toBool())
        copy(whenTrue);
      else
        copy(whenFalse);
    } else {
      for (std::size_t lane = first; lane < first + Group; ++lane)
        out.element(lane) = Wire::fromPacked(
            select(guard, whenTrue.element(lane).packed(),
                         whenFalse.element(lane).packed()));
    }
  }
}
)cpp";
}
} // namespace acir::compiler
#endif

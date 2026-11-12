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
    std::string value = "pyc_values.page<pyc_work_page_" +
                        std::to_string(page) + ">(" + std::to_string(page) +
                        ")." + name.str();
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
  void emit(llvm::raw_ostream &out) {
    out << "private:\n  // Work statement target: " << partBytes
        << " bytes; largest indivisible group: " << maxStatementBytes
        << " bytes.\n";
    for (std::size_t start = 0; start < planes.size(); start += pageFields) {
      auto page = start / pageFields;
      out << "  struct pyc_work_page_" << page << " {\n";
      for (std::size_t i = start;
           i < std::min(start + pageFields, planes.size()); ++i) {
        auto &plane = planes[i];
        out << "    " << (plane.constant ? "const " : "") << plane.type
            << (plane.constant ? " *" : " ") << plane.name
            << (plane.constant ? " = nullptr" : "") << ";\n";
      }
      // Each constructor initializes at most one page. A single large frame
      // constructor must not become the new unbounded generated function.
      out << "    pyc_work_noinline pyc_work_page_" << page << "() {}\n  };\n";
    }
    std::vector<std::string> calls;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      auto name = "pyc_work_part_" + std::to_string(i);
      out << "  pyc_work_noinline void " << name
          << "(pyc_work_frame &pyc_values) {\n"
          << parts[i] << "  }\n";
      calls.push_back(name);
    }
    // Preserve order, while also bounding call-only dispatch functions.
    for (std::size_t level = 0; calls.size() > dispatchFanout; ++level) {
      std::vector<std::string> next;
      for (std::size_t start = 0; start < calls.size();
           start += dispatchFanout) {
        auto name = "pyc_work_dispatch_" + std::to_string(level) + "_" +
                    std::to_string(start / dispatchFanout);
        out << "  pyc_work_noinline void " << name
            << "(pyc_work_frame &pyc_values) {\n";
        for (std::size_t i = start;
             i < std::min(start + dispatchFanout, calls.size()); ++i)
          out << "    " << calls[i] << "(pyc_values);\n";
        out << "  }\n";
        next.push_back(name);
      }
      calls = std::move(next);
    }
    out << "public:\n  void Work() {\n    pyc_clear_check_snapshots();\n    "
           "try {\n"
        << "      pyc_work_frame pyc_values("
        << (planes.size() + pageFields - 1) / pageFields << ");\n";
    for (auto &name : calls)
      out << "      " << name << "(pyc_values);\n";
    out << "    } catch (...) { DiscardNext(); throw; }\n  }\n";
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
#include <vector>
#if defined(_MSC_VER)
#define pyc_work_noinline __declspec(noinline)
#else
#define pyc_work_noinline __attribute__((noinline))
#endif
// Per-evaluation, normally constructed typed storage. The fixed page sizes
// avoid a giant implicit constructor; the erased owners only manage lifetime.
class pyc_work_frame {
  struct slot {
    void *value = nullptr;
    void (*destroy)(void *) noexcept = nullptr;
  };
  std::vector<slot> slots_;
public:
  explicit pyc_work_frame(std::size_t count) : slots_(count) {}
  pyc_work_frame(const pyc_work_frame &) = delete;
  pyc_work_frame &operator=(const pyc_work_frame &) = delete;
  ~pyc_work_frame() noexcept {
    for (auto &entry : slots_)
      if (entry.value) entry.destroy(entry.value);
  }
  template <class Page> Page &page(std::size_t index) {
    auto &entry = slots_[index];
    if (!entry.value) {
      auto *value = new Page;
      entry.value = value;
      entry.destroy = +[](void *object) noexcept { delete static_cast<Page *>(object); };
    }
    return *static_cast<Page *>(entry.value);
  }
};
)cpp";
}
} // namespace acir::compiler
#endif

// Access-only generated-header variant. Existing capture methods set bounded
// partial completion; source checks, reset logic, validator and IR stay intact.
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <gfsim/WorkExecutor.h>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

void require(bool value,
             std::source_location at = std::source_location::current()) {
  if (!value) {
    std::cerr << "completion precedence oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
auto known(bool value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value ? 1u : 0u});
}
std::string hex(std::string_view value) {
  if (value.empty())
    return "-";
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (unsigned char byte : value) {
    result += digits[byte >> 4];
    result += digits[byte & 15];
  }
  return result;
}
void report(pyc_root &root, std::string_view label, bool accepted) {
  gfsim::SimFailureInfo error;
  require(root.__pyc_validate_checks(error) == accepted);
  std::cout << "COMPLETION " << label << ' ' << accepted << ' '
            << static_cast<unsigned>(error.phase) << ' ' << hex(error.code)
            << ' ' << hex(error.message) << ' ' << hex(error.instance) << ' '
            << hex(error.sourceJson) << ' ' << hex(error.checkIdJson) << '\n';
}
int main(int argc, char **argv) {
  require(argc == 2);
  auto workers = static_cast<unsigned>(std::stoul(argv[1]));
  require(workers == 1 || workers == 2);
  gfsim::WorkExecutor executor(workers);
  pyc_root root("root", &executor);
  auto family = root.pyc_implementation;
#if T3_COMPLETION == 1
  family->__pyc_clear_check_snapshots();
  family->__pyc_capture_reset(0, known(false));
  family->__pyc_capture_check_0(0, known(true), known(true));
  family->__pyc_capture_check_1(0, known(true), known(true));
  report(root, "snapshots-later-check-missing", false);
  family->__pyc_capture_check_0(0, known(false), known(true));
  report(root, "snapshots-earlier-false-later-missing", false);
  family->__pyc_capture_check_2(0, known(true), known(true));
  report(root, "snapshots-restored-check", false);
  family->__pyc_clear_check_snapshots();
  family->__pyc_capture_check_0(0, known(true), known(true));
  family->__pyc_capture_check_1(0, known(true), known(true));
  family->__pyc_capture_check_2(0, known(true), known(true));
  report(root, "snapshots-reset-only-missing", false);
  family->__pyc_capture_reset(0, known(false));
  report(root, "snapshots-restored-reset", true);
#elif T3_COMPLETION == 5
  auto a = family->pyc_instance_zzz;
  auto b = family->pyc_instance_aaa;
  auto aPure = a->pyc_instance_pyc_70757265;
  auto bPure = b->pyc_instance_pyc_70757265;
  family->__pyc_clear_check_snapshots();
  a->__pyc_capture_check_0(0, known(false), known(true));
  aPure->__pyc_capture_check_0(0, known(true), known(true));
  b->__pyc_capture_check_0(0, known(true), known(true));
  bPure->__pyc_capture_check_0(0, known(true), known(true));
  a->__pyc_capture_reset(0, known(false));
  report(root, "domains-earlier-false-later-reset-missing", false);
  b->__pyc_capture_reset(0, known(false));
  report(root, "domains-restored-later-reset", false);
  family->__pyc_clear_check_snapshots();
  a->__pyc_capture_check_0(0, known(true), known(true));
  aPure->__pyc_capture_check_0(0, known(true), known(true));
  b->__pyc_capture_check_0(0, known(true), known(true));
  bPure->__pyc_capture_check_0(0, known(false), known(true));
  a->__pyc_capture_reset(0, known(false));
  report(root, "domains-grandchild-authoritative-reset-missing", false);
  b->__pyc_capture_reset(0, known(false));
  report(root, "domains-restored-grandchild-reset", false);
#else
#error Unsupported completion test fixture
#endif
}

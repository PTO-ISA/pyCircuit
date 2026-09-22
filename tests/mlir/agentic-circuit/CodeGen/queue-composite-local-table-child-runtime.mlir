// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python %t/lower.py %t/local_table_child.py %t/raw.mlir
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,ac-inline-pure-helpers,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %t/raw.mlir -o %t/frozen.mlir
// RUN: %acir_queue_cxxgen --output-root=%t/bundle %t/frozen.mlir | %FileCheck %s --check-prefix=EMIT
// RUN: %FileCheck %s --check-prefix=TABLE < %t/bundle/include/generated/modules/stage.hpp
// RUN: cmake -S %t/bundle -B %t/build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/build --parallel 2
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/harness
// RUN: %t/harness | %FileCheck %s --check-prefix=RUNTIME

// Issue #223 acceptance criterion 9 for a module-local TABLE rather than a
// scalar Var. `stage` owns a two-entry `ac.table` whose rule updates the entry
// selected by the incoming tid, and that entry feeds a stateful child. The
// runtime stream proves that both entries persist across ticks, that the local
// rule updates the addressed entry only, and that the child observes the
// committed value:
//
//   entries[0]: 1, 3      child bank total: 1, 11, 14, 44
//   entries[1]: 10, 30
//
// A shared or reset Table would produce different numbers, and the child cannot
// produce 11 or 44 without the local Table's committed values.

// EMIT: emitted model bundle v1

// TABLE: gfsim::QueueTableTransition<{{.*}}> block_;
// TABLE: gfsim::SimTable<Entry> state_entries_;

// RUNTIME: results=1,11,14,44
// RUNTIME: PASS local table

//--- lower.py
from pathlib import Path
import sys

from agentic_circuit._queue_frontend import lower_queue_source


source = Path(sys.argv[1]).read_text()
Path(sys.argv[2]).write_text(
    lower_queue_source(source, "composite", source_path="scen/entry.py")
)

//--- local_table_child.py
import agentic_circuit as ac


@ac.struct
class Entry:
    value: ac.bits[8]
    tid: ac.bits[1]
    valid: ac.bits[1]


@ac.rule
def accumulate(total, packet):
    total = total + packet.value
    return Entry(value=total, tid=packet.tid, valid=packet.valid)


@ac.module_decl(source="scen/entry.py")
def acc_bank(packet: Entry) -> Entry:
    ...


acc_bank_decl = acc_bank


@ac.module(declaration=acc_bank_decl)
def acc_bank(packet: Entry) -> Entry:
    total: ac.bits[8] = 0
    result = accumulate(total, packet)
    return result


@ac.rule
def install(entries, incoming):
    old = entries[incoming.tid]
    updated = Entry(
        value=old.value + incoming.value, tid=incoming.tid, valid=incoming.valid
    )
    entries[incoming.tid] = updated
    return updated


@ac.module_decl(source="scen/entry.py")
def stage(request: Entry) -> Entry:
    ...


stage_decl = stage


@ac.module(declaration=stage_decl)
def stage(request: Entry) -> Entry:
    entries = ac.table[2, Entry](init=0)
    tagged = install(entries, request)
    result = acc_bank(tagged)
    return result


@ac.system
def composite(request: Entry) -> Entry:
    return stage(request)

//--- harness.cpp
// Runtime harness for issue #223 acceptance criterion 9 with a Table.

#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace {

using ac_generated::Composite;
using ac_generated::Entry;

Entry makeEntry(uint8_t value, bool tid) {
  Entry entry;
  entry.value = gfsim::UInt<8>{value};
  entry.tid = gfsim::UInt<1>{tid};
  entry.valid = gfsim::UInt<1>{true};
  return entry;
}

} // namespace

int main() {
  gfsim::SimSystem system{"table9"};
  Composite model;
  model.set_sink_retention_limit(0);
  std::array<gfsim::TimeDomainRuntime, 1> timeDomains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(timeDomains) ||
      !model.configure_activation_scheduler(system)) {
    std::printf("FAIL: model did not wire\n");
    return 1;
  }

  const std::vector<Entry> stimulus{makeEntry(1, /*tid=*/false),
                                    makeEntry(10, /*tid=*/true),
                                    makeEntry(2, /*tid=*/false),
                                    makeEntry(20, /*tid=*/true)};
  std::vector<unsigned> values;
  size_t next = 0;
  for (size_t tick = 0; tick < 64; ++tick) {
    if (next < stimulus.size() && model.offer_request(system, stimulus[next]))
      ++next;
    const bool advanced = system.step();
    while (std::optional<Entry> value = model.try_take_result_0(system))
      values.push_back(static_cast<unsigned>(value->value.value()));
    if (!advanced)
      break;
  }

  std::string text;
  for (unsigned value : values) {
    if (!text.empty())
      text += ",";
    text += std::to_string(value);
  }
  std::printf("results=%s\n", text.c_str());

  const std::vector<unsigned> expected{1, 11, 14, 44};
  if (values != expected) {
    std::printf("FAIL: local Table state stream unexpected\n");
    return 1;
  }

  std::printf("PASS local table\n");
  return 0;
}

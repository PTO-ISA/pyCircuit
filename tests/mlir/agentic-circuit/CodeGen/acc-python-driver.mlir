// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/architecture.py -o %t/model.ac --quiet
// RUN: %FileCheck %s --check-prefix=AC < %t/model.ac
// RUN: %acc -c %t/model.ac -emit-cpp -o %t/model.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t/model.cpp
// RUN: %acc -c %t/model.ac -emit-verilog -o %t/model.v
// RUN: %FileCheck %s --check-prefix=VERILOG < %t/model.v
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include %t/harness.cpp -o %t/model
// RUN: %t/model | %FileCheck %s --check-prefix=EXEC
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/structured.toml -c %t/structured.py -o %t/structured.ac --quiet
// RUN: %not %acc -c %t/structured.ac -emit-verilog -o %t/structured.v 2>&1 | %FileCheck %s --check-prefix=STRUCTURED-VERILOG-ERROR
// RUN: test ! -e %t/structured.v

// AC: ac.system = "rob"
// AC: ac.ndf_ids = ["DAV-ACC-RULE-0001"]
// AC-SAME: ac.ndf_requires = ["DAV-ACC-CONTRACT-0001"]
// CPP: // rule: complete
// CPP-NEXT: // ndf: DAV-ACC-RULE-0001
// CPP-NEXT: // ndf-requires: DAV-ACC-CONTRACT-0001
// CPP-NEXT: // source: architecture.py:
// CPP: // ndf: DAV-ACC-SYSTEM-0001
// CPP-NEXT: // source: architecture.py:
// CPP-NEXT: class Rob final : public gfsim::Module
// VERILOG: module rob (
// VERILOG: endmodule
// EXEC: acc generated DUT: PASS
// STRUCTURED-VERILOG-ERROR: structured input requires a directory-backed AC package

//--- architecture.py
import agentic_circuit as ac


@ac.struct
class Entry:
    sequence: ac.u4
    value: ac.u16
    done: bool


# ndf: DAV-ACC-RULE-0001
# ndf:requires DAV-ACC-CONTRACT-0001
@ac.rule
def complete(entry):
    return entry.with_fields(done=True)


# ndf: DAV-ACC-SYSTEM-0001
@ac.system
def rob() -> None:
    issued = ac.source(Entry, depth=4, latency=1)
    completed = complete(issued)
    retired = ac.reorder(completed, by=Entry.sequence, entries=8, start=0)
    ac.sink(retired)


//--- harness.cpp
#include "model.cpp"

#include <cstdint>
#include <iostream>

namespace {

void cycle(ac_generated::Rob &model, std::uint64_t tick) {
  auto rows = model.dispatch_rows();
  const gfsim::Epoch epoch{tick, 0};
  for (auto &row : rows)
    row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
}

} // namespace

int main() {
  ac_generated::Rob model;
  model.reset();

  const ac_generated::Entry later{
      gfsim::UInt<4>{1}, gfsim::UInt<16>{0x1234}, gfsim::UInt<1>{0}};
  const ac_generated::Entry first{
      gfsim::UInt<4>{0}, gfsim::UInt<16>{0xabcd}, gfsim::UInt<1>{0}};
  if (!model.issued().proposePush(later))
    return 1;
  model.issued().doXfer({0, 0});
  if (!model.issued().proposePush(first))
    return 2;
  model.issued().doXfer({0, 1});

  for (std::uint64_t tick = 1; tick <= 12; ++tick)
    cycle(model, tick);

  const auto &values = model.sink_0_values();
  if (values.size() != 2)
    return 3;
  if (values[0].sequence.value() != 0 || values[0].value.value() != 0xabcd ||
      values[0].done.value() != 1)
    return 4;
  if (values[1].sequence.value() != 1 || values[1].value.value() != 0x1234 ||
      values[1].done.value() != 1)
    return 5;

  std::cout << "acc generated DUT: PASS\n";
  return 0;
}

//--- agentic-circuit.toml

[project]
name = "acc-python-smoke"
version = "0.1.0"
architecture = "architecture.py"
system = "rob"

[providers]
standard_library = ["ac"]

[build]
profile = "fast"
compiler = "c++"
standard_library = "libc++"
component_roots = []
protocol_roots = []
build_root = "build"
instrumentation_layers = []

[run]
trace_roots = []
inputs = {}

[diagnostics]
format = "text"

//--- structured.py
import agentic_circuit as ac


@ac.rule
def frontend_marker(value):
    return value


@ac.module_decl(source="structured.py")
def increment(value: ac.u8) -> ac.u8:
    ...


increment_decl = increment


@ac.module(declaration=increment_decl)
def increment(value: ac.u8) -> ac.u8:
    return value + 1


@ac.system
def structured(value: ac.u8) -> ac.u8:
    result = increment(value)
    return result

//--- structured.toml

[project]
name = "acc-structured-smoke"
version = "0.1.0"
architecture = "structured.py"
system = "structured"

[providers]
standard_library = ["ac"]

[build]
profile = "fast"
compiler = "c++"
standard_library = "libc++"
component_roots = []
protocol_roots = []
build_root = "build-structured"
instrumentation_layers = []

[run]
trace_roots = []
inputs = {}

[diagnostics]
format = "text"

// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/core.py --unit core -o %t/package/core.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/copy.py --specializations-json %t/copy.json --header-output %t/package/interfaces/pkg/copy/module.ac -o %t/package/h3/copy/copy.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/split.py --specializations-json %t/split.json --header-output %t/package/interfaces/pkg/split/module.ac -o %t/package/h3/split/split.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/join.py --specializations-json %t/join.json --header-output %t/package/interfaces/pkg/join/module.ac -o %t/package/h3/join/join.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/assembly.py --specializations-json %t/assembly.json --header-output %t/package/interfaces/pkg/assembly/module.ac -o %t/package/h2/assembly/assembly.ac --quiet
// RUN: %acc -c %t/package -verify
// RUN: %acc -c %t/package -emit-cpp-bundle -o %t/bundle
// RUN: cmake -S %t/bundle -B %t/bundle-build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/bundle-build --parallel 4
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/bundle-build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/fanout
// RUN: %t/fanout

//--- pkg/__init__.py

//--- pkg/copy_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/copy.py")
def copy_value(value: ac.u8) -> ac.u8:
    ...

//--- pkg/assembly_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/assembly.py")
def assembly(value: ac.u8) -> ac.u8:
    ...

//--- pkg/split_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/split.py")
def split_value(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    ...

//--- pkg/join_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/join.py")
def join(left: ac.u8, right: ac.u8, third: ac.u8) -> ac.u8:
    ...

//--- pkg/copy.py
import agentic_circuit as ac


@ac.module
def copy_value(value: ac.u8) -> ac.u8:
    return value

//--- pkg/split.py
import agentic_circuit as ac


@ac.rule
def duplicate(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    left = value + 0
    right = value + 0
    return left, right


@ac.module
def split_value(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    left, right = duplicate(value)
    return left, right

//--- pkg/join.py
import agentic_circuit as ac


@ac.rule
def choose(left, right, third):
    return left


@ac.module
def join(left: ac.u8, right: ac.u8, third: ac.u8) -> ac.u8:
    result = choose(left, right, third)
    return result

//--- pkg/assembly.py
import agentic_circuit as ac

from pkg.copy_module import copy_value
from pkg.join_module import join
from pkg.split_module import split_value


@ac.module
def assembly(value: ac.u8) -> ac.u8:
    left, right = split_value(value)
    third = copy_value(value)
    result = join(left, right, third)
    return result

//--- pkg/core.py
import agentic_circuit as ac

from pkg.assembly_module import assembly


@ac.system
def core(value: ac.u8) -> ac.u8:
    return assembly(value)

//--- copy.json
{"schema":"agentic-circuit-specializations","version":"0.1","specializations":[{"module":"copy_value","static":{}}]}

//--- assembly.json
{"schema":"agentic-circuit-specializations","version":"0.1","specializations":[{"module":"assembly","static":{}}]}

//--- split.json
{"schema":"agentic-circuit-specializations","version":"0.1","specializations":[{"module":"split_value","static":{}}]}

//--- join.json
{"schema":"agentic-circuit-specializations","version":"0.1","specializations":[{"module":"join","static":{}}]}

//--- harness.cpp
#include "generated/dut.h"

#include <array>
#include <cstdint>

int main() {
  gfsim::SimSystem system{"composite-fanout"};
  ac_generated::Core model;
  std::array<gfsim::TimeDomainRuntime, 1> time_domains{{
      {"cycle", 1, 0, 1},
  }};
  if (!system.root().attachChild(model) ||
      !system.setTimeDomains(time_domains) ||
      !model.configure_activation_scheduler(system) ||
      !model.offer_value(system, gfsim::UInt<8>{9}))
    return 1;
  while (system.step()) {}
  const auto &values = model.sink_0_values();
  return values.size() == 1 && values[0].value() == 9 ? 0 : 2;
}

//--- agentic-circuit.toml
[project]
name = "acc-composite-fanout-package"
version = "0.1.0"
architecture = "pkg/core.py"
system = "core"

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

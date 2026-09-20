// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/core.py --unit core -o %t/package/core.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/copy.py --header-output %t/package/interfaces/pkg/copy.ac -o %t/package/h3/copy/copy.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/split.py --header-output %t/package/interfaces/pkg/split.ac -o %t/package/h3/split/split.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/join.py --header-output %t/package/interfaces/pkg/join.ac -o %t/package/h3/join/join.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/fanout_pipeline.py --header-output %t/package/interfaces/pkg/fanout_pipeline.ac -o %t/package/h2/fanout_pipeline/fanout_pipeline.ac --quiet
// RUN: %acc -c %t/package -verify
// RUN: %acc -c %t/package -emit-cpp-bundle -o %t/bundle
// RUN: cmake -S %t/bundle -B %t/bundle-build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/bundle-build --parallel 4
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/bundle-build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/fanout
// RUN: %t/fanout

//--- pkg/_init__.py

//--- pkg/copy_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/copy.py")
def copy_value(value: ac.u8) -> ac.u8:
    ...

//--- pkg/fanout_pipeline_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/fanout_pipeline.py")
def fanout_pipeline(value: ac.u8) -> ac.u8:
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
from pkg.copy_module import copy_value


@ac.module(declaration=copy_value)
def copy_value(value: ac.u8) -> ac.u8:
    return value

//--- pkg/split.py
import agentic_circuit as ac
from pkg.split_module import split_value


@ac.rule
def duplicate(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    left = value + 0
    right = value + 0
    return left, right


@ac.module(declaration=split_value)
def split_value(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    left, right = duplicate(value)
    return left, right

//--- pkg/join.py
import agentic_circuit as ac
from pkg.join_module import join


@ac.rule
def choose(left, right, third):
    return left


@ac.module(declaration=join)
def join(left: ac.u8, right: ac.u8, third: ac.u8) -> ac.u8:
    result = choose(left, right, third)
    return result

//--- pkg/fanout_pipeline.py
import agentic_circuit as ac

from pkg.fanout_pipeline_module import fanout_pipeline
from pkg.copy_module import copy_value
from pkg.join_module import join
from pkg.split_module import split_value


@ac.module(declaration=fanout_pipeline)
def fanout_pipeline(value: ac.u8) -> ac.u8:
    left, right = split_value(value)
    third = copy_value(value)
    result = join(left, right, third)
    return result

//--- pkg/core.py
import agentic_circuit as ac

from pkg.fanout_pipeline_module import fanout_pipeline


@ac.system
def core(value: ac.u8) -> ac.u8:
    return fanout_pipeline(value)





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
  std::optional<gfsim::UInt<8>> value;
  while (!value && system.step())
    value = model.try_take_result_0(system);
  if (!value)
    return 2;
  while (system.step()) {}
  return value->value() == 9 ? 0 : 3;
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

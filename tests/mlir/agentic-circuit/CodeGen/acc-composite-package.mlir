// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/core.py --unit core -o %t/package/core.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/decode.py --header-output %t/package/interfaces/pkg/decode.ac -o %t/package/h3/decode/decode.ac --quiet
// RUN: %FileCheck %s --check-prefix=SOURCE < %t/package/h3/decode/decode.ac
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/execute.py --header-output %t/package/interfaces/pkg/execute.ac -o %t/package/h3/execute/execute.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/pipeline.py --header-output %t/package/interfaces/pkg/pipeline.ac -o %t/package/h2/pipeline/pipeline.ac --quiet
// RUN: %acc -c %t/package -verify
// RUN: %acc -c %t/package -emit-cpp-bundle -o %t/bundle
// RUN: cmake -S %t/bundle -B %t/bundle-build -G Ninja -DAC_GFSIM_INCLUDE_DIR=%source_root/simulator/gfsim/include
// RUN: cmake --build %t/bundle-build --parallel 4
// RUN: %cxx -std=c++20 -I%t/bundle/include -I%source_root/simulator/gfsim/include %t/harness.cpp %t/bundle-build/libac_generated_model.a %binary_root/gfsim/libgfsim.a -o %t/composite
// RUN: %t/composite

// SOURCE: ac.module @decode
// SOURCE-NOT: func.call @widen
// SOURCE-NOT: func.func private @widen

//--- pkg/_init__.py

//--- pkg/decode_module.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/decode.py")
def decode(value: ac.u8) -> ac.u16:
    ...

//--- pkg/execute_module.py
import agentic_circuit as ac

@ac.module_decl(source="pkg/execute.py")
def execute(value: ac.u16) -> ac.u32:
    ...

//--- pkg/pipeline_module.py
import agentic_circuit as ac

@ac.module_decl(source="pkg/pipeline.py")
def pipeline(value: ac.u8) -> ac.u32:
    ...

//--- pkg/decode.py
import agentic_circuit as ac

from pkg.decode_module import decode


@ac.inline
def widen(value: ac.u8) -> ac.u16:
    return ac.zext(value, ac.u16)


@ac.module(declaration=decode)
def decode(value: ac.u8) -> ac.u16:
    return widen(value)

//--- pkg/helpers.py
import agentic_circuit as ac


@ac.inline
def widen(value: ac.u8) -> ac.u16:
    return ac.zext(value, ac.u16)

//--- pkg/execute.py
import agentic_circuit as ac
from pkg.execute_module import execute


@ac.module(declaration=execute)
def execute(value: ac.u16) -> ac.u32:
    return ac.zext(value, ac.u32)

//--- pkg/pipeline.py
import agentic_circuit as ac

from pkg.decode_module import decode
from pkg.execute_module import execute
from pkg.pipeline_module import pipeline


@ac.module(declaration=pipeline)
def pipeline(value: ac.u8) -> ac.u32:
    decoded = decode(value)
    result = execute(decoded)
    return result

//--- pkg/core.py
import agentic_circuit as ac

from pkg.pipeline_module import pipeline


@ac.system
def core(value: ac.u8) -> ac.u32:
    return pipeline(value)




//--- harness.cpp
#include "generated/dut.h"

#include <array>
#include <cstdint>

int main() {
  gfsim::SimSystem system{"composite"};
  ac_generated::Core model;
  std::array<gfsim::TimeDomainRuntime, 1> time_domains{{
      {"cycle", 1, 0, 1},
  }};
  if (!system.root().attachChild(model) ||
      !system.setTimeDomains(time_domains) ||
      !model.configure_activation_scheduler(system) ||
      !model.offer_value(system, gfsim::UInt<8>{7}))
    return 1;
  std::optional<gfsim::UInt<32>> value;
  while (!value && system.step())
    value = model.try_take_result_0(system);
  if (!value)
    return 2;
  while (system.step()) {}
  return value->value() == 7 ? 0 : 3;
}

//--- agentic-circuit.toml
[project]
name = "acc-composite-package"
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

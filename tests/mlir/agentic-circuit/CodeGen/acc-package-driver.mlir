// RUN: rm -rf %t
// RUN: %split_file %s %t
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/core.py --unit core -o %t/package/core.ac --quiet
// RUN: env PYTHONPATH=%source_root/python/semantic-core/src:%source_root/python/agentic-circuit/src:%binary_root/python %python -m agentic_circuit._acc_py --project %t/agentic-circuit.toml -c %t/pkg/child.py --specializations-json %t/child.json --header-output %t/package/interfaces/pkg/child/module.ac -o %t/package/h1/child/child.ac --quiet
// RUN: %acc -c %t/package -verify
// RUN: %acc -c %t/package -emit-cpp-bundle -o %t/bundle
// RUN: test -s %t/bundle/include/generated/dut.h
// RUN: cp -R %t/package %t/missing-header
// RUN: rm %t/missing-header/interfaces/pkg/child/module.ac
// RUN: %not %acc -c %t/missing-header -verify 2>&1 | %FileCheck %s --check-prefix=MISSING-HEADER

// MISSING-HEADER: source AC unit requires one matching module interface header for 'child'

//--- pkg/__init__.py

//--- pkg/interface.py
import agentic_circuit as ac


@ac.module_decl(source="pkg/child.py")
def child() -> None:
    ...

//--- pkg/child.py
import agentic_circuit as ac


@ac.module
def child() -> None:
    pass

//--- pkg/core.py
import agentic_circuit as ac

from pkg.interface import child


@ac.system
def core() -> None:
    child()

//--- child.json
{
  "schema": "agentic-circuit-specializations",
  "version": "0.1",
  "specializations": [
    {"module": "child", "static": {}}
  ]
}

//--- agentic-circuit.toml
[project]
name = "acc-package-smoke"
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

# Issue #180: the Python bundle API

## Change

`agentic_circuit.bundle.lower_sources(<architecture>, output=<bundle directory>)`
drives the whole multi-unit flow for a Python caller:

1. `capture_source_closure()` enumerates the workspace sources reachable from the
   architecture; each source that declares exactly one public `@ac.module` is
   compiled into `sources/<relative>.ac` with its interface header at
   `interfaces/<relative>.ac`;
2. the architecture is compiled into `core.ac` (rejected if it declares a module
   itself, since a bundle needs a system);
3. `--unit interfaces` supplies the compiler interface units
   (`interfaces/_compiler/*.ac`);
4. the native `acc` tool links the directory and emits the structured bundle.

It returns a `ModuleBundle` with the bundle path, the sorted file inventory, and
the parsed `module-manifest.json`; `package=<path>` retains the package directory
(the default removes it). The native tool is resolved from the `acc` argument,
then `AGENTIC_CIRCUIT_ACC`, then `PATH`, and a missing tool, an existing output
path, or a module-unit architecture raises `BundleError`.

The submodule is exposed as `agentic_circuit.bundle` and deliberately does not
join `RUNTIME_API`/`__all__`, which stay the capture-only authoring inventory.

## Evidence

`test_multi_unit_package.py` (7 passed) calls the API for the linked three-file
hierarchy and asserts the 15-file bundle inventory, the parsed manifest
(`definition`, placement definitions), and the retained package layout
(`core.ac`, `sources/source/<module>.ac`, `interfaces/source/<module>.ac`,
`interfaces/_compiler/layouts.ac`). Error paths were exercised directly: an
existing bundle destination, a module-unit architecture, and a missing tool each
raise `BundleError` with the exact reason.

| lane | result |
| --- | --- |
| `tests/python/agentic-circuit` (pytest, ignore tools/) | 496 passed, 1 skipped, 5 pre-existing failures |
| `tests/unit -m unit` | 217 passed |
| `check-contracts.py`, `check-ir-coverage.py`, diagnostic catalog | OK |

No C++ or lit input changed in this run.

## Scope

The driver API only. The link and bundle steps still shell out to the native
`acc` tool because the Python extension exposes only `run_compiler` and
`capabilities`; moving them in-process needs a binding entry point, and the
issue's `ac.jit(...).lower_cpp()` frame still has no JIT-level method.

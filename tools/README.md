# Repository tools

Build, validate and maintain the checkout from one directory. Run these commands
from the repository root; generated outputs belong under `.pycircuit_out/`.

```sh
bash tools/pyc build
bash tools/run_api_tests.sh --tier gate
bash tools/run_examples.sh --tier gate
```

| Tool | Responsibility |
| --- | --- |
| [pyc](pyc), [pyc.ps1](pyc.ps1) | Configure, build and install the toolchain on POSIX or Windows. |
| [install_llvm_and_build.sh](install_llvm_and_build.sh) | Discover a Homebrew LLVM installation and invoke the build helper. |
| [lib.sh](lib.sh) | Shared shell discovery, configuration and gate receipts. |
| [run_api_tests.sh](run_api_tests.sh), [run_examples.sh](run_examples.sh) | Select the existing gate or nightly test suites. |
| [check_api_hygiene.py](check_api_hygiene.py), [check_frontend_retirement.py](check_frontend_retirement.py) | Check the active Python surface and reject retired compilation routes. |
| [check_generated_rtl.py](check_generated_rtl.py) | Audit generated RTL. |
| [materialize_source_preview.py](materialize_source_preview.py) | Materialize source-owned preview artifacts. |
| [measure_source_build.py](measure_source_build.py), [measure_build_resources.py](measure_build_resources.py), [process_usage.py](process_usage.py) | Measure build costs and process resource use. |
| [summarize_gate_run.py](summarize_gate_run.py) | Summarize gate evidence. |
| [example_catalog.py](example_catalog.py) | Render example navigation and publish verified generated-output excerpts. |
| [generate_source_identifier_unicode.py](generate_source_identifier_unicode.py) | Regenerate pinned Unicode source-identifier tables and provenance. |

Installed build helpers live in [`cmake/`](../cmake/); SDK and wheel assembly
live in [`packaging/`](../packaging/). The public compiler commands remain
`pycircuit compile`, `link`, `emit` and `run`.
See the [repository map](../docs/development/repository-layout.md) for source owners
and [testing guide](../docs/development/testing-and-gates.md) for gate selection.

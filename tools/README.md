# Repository maintenance tools

| Tool | Responsibility |
| --- | --- |
| [example_catalog.py](example_catalog.py) | Render example navigation and publish verified generated-output excerpts. |
| [generate_source_identifier_unicode.py](generate_source_identifier_unicode.py) | Regenerate pinned Unicode source-identifier tables and provenance. |

Build, validation and measurement orchestration lives in [`flows/`](../flows/),
installed CMake helpers in [`cmake/`](../cmake/), and SDK/wheel assembly in
[`packaging/`](../packaging/). These generators do not implement another compiler.
The public commands are `pycircuit compile`, `link`, `emit` and `run`.
See the [repository map](../docs/development/repository-layout.md) for owners.
Generated outputs belong under `.pycircuit_out/`.

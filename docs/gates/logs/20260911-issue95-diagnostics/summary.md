# Issue 95 structured diagnostic evidence

Decision 0242 separates stable diagnostic identity from rendered message text.
Agentic frontend and CLI boundaries now consume structured code/message/source
fields, the explicit registry records one owner/stage/meaning and exact source
set for every implementation code, and repository checks reject unregistered
or stale catalog output. Scalar value primitives share the `ACPY-VAR-003`
family. A fresh universal wheel contains the generated registry/catalog and
resolves `explain` from its installed package resources.

pyCircuit public authoring failures carry a `Diagnostic` through the existing
`PyCircuitError` hierarchy while preserving builtin exception compatibility.
The native PYC helper attaches `diagnostic.code` metadata, the ACIR driver reads
that metadata rather than scanning text, and unstructured third-party failures
use registered stage fallbacks. Former PYC970-975/PYC982 meaning conflicts use
distinct probe and frontend codes.

Python, native, catalog, CLI, documentation, inventory, and changed-file gates
all pass from the current checkout. See `commands.txt`, adjacent stdout/stderr
records, `summary.json`, and `decision_status_report.json` for bounded evidence.

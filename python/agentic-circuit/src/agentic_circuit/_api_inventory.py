"""Public authoring inventories shared by the package root and the frontend.

``agentic_circuit`` separates three disjoint inventories:

* ``RUNTIME_API`` (package root): names that can author a Queue model.
* ``CAPTURE_ONLY_API`` (``markers``): capture-time-only syntax markers.
* ``RESERVED_API`` (this module): declaration spellings the package accepts so
  authored source can import them, but that the ACPy queue frontend has no
  implementation for.

The ACPy queue frontend imports this module so a reserved declaration fails
fast with a targeted diagnostic instead of an unrelated payload or collection
error further down the pipeline.
"""

from __future__ import annotations

RESERVED_API = (
    "extern_module",
    "interface",
    "packet",
    "process",
    "protocol",
    "transaction",
)

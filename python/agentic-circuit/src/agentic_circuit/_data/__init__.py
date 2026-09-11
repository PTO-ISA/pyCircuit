"""Installed Agentic Circuit data resources.

Wheel and CMake installation populate ``_data/schemas`` from the canonical
repository schemas. Editable source checkouts resolve that canonical tree via
``agentic_circuit._package_data`` instead of duplicating schema files here.
"""

from pkgutil import extend_path


__path__ = extend_path(__path__, __name__)

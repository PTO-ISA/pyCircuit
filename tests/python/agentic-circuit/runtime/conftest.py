"""Make the repository-local runtime tool package importable in isolation."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
for path in (ROOT, ROOT / "tools", ROOT / "tools" / "runtime"):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

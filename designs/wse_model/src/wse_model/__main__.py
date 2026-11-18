"""Allow ``python -m wse_model`` to run the command line interface."""

from __future__ import annotations

from wse_model.cli import main

if __name__ == "__main__":
    raise SystemExit(main())

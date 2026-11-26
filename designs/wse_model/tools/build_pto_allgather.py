"""Compatibility entry: build the generic WSE machine, without an AllGather workload."""

from build_wse_machine import main

if __name__ == "__main__":
    raise SystemExit(main())

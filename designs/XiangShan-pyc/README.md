# XiangShan-pyc: XiangShan KunMingHu in PyCircuit V6

PyCircuit V6 cycle-aware reimplementation of the XiangShan (香山) KunMingHu
micro-architecture.  All RTL is written from scratch using `CycleAwareCircuit`
/ `CycleAwareDomain` APIs; the original Chisel implementation under
`designs/XiangShan/` serves only as a **specification reference** for port
widths, parameter defaults, and behavioral details.

## Programming style

- **Authoring mode:** PyCircuit V6 cycle-aware (`cas`, `domain.next()`,
  `domain.signal`, `mux`)
- **Style reference:** `designs/RegisterFile/regfile.py` (explicit `cas` +
  `domain.next()`)
- **Build function signature:** `build_<module>(m: CycleAwareCircuit, domain:
  CycleAwareDomain, **params)`
- **Compilation:** `compile_cycle_aware(build_<module>, name="<module>",
  eager=True).emit_mlir()`
- **Testbench:** `@testbench` + `CycleAwareTb` (`tb.drive` / `tb.expect` /
  `tb.next()`)

## Directory layout

```text
lib/            Common primitives, protocol definitions
frontend/       BPU, FTQ, IFU, ICache, IBuffer, Decode
backend/        Rename, Dispatch, Issue, ExeUnits, ROB, RegFile, CtrlBlock
mem/            Load/Store pipelines, LSQ, SBuffer, Prefetcher
cache/          DCache, MMU/TLB, WPU
l2/             L2 Cache (CoupledL2)
top/            XSCore, XSTile, XSTop, parameters
docs/           Specifications, port lists, feature lists, traceability
```

## Input sources

| Source | Path | Purpose |
|--------|------|---------|
| XiangShan micro-arch docs | `designs/XiangShan-doc/docs/` | Per-subsystem design specs |
| XiangShan arch diagrams | `designs/XiangShan-doc/docs/figs/` | Module relationships, pipelines |
| XiangShan reference impl | `designs/XiangShan/` | Port widths, parameter defaults, behavioral details |

## Ten-step workflow

Every module follows `docs/pycircuit_implementation_method.md`:

1. Read PyCircuit V6 API docs and examples
2. Read block-specific requirement documents
3. Top-level ports, buses, widths, and feature list
4. Sequential behavior description (pseudocode)
5. Pipeline mapping (`domain.next()` planning)
6. Full cycle-aware PyCircuit V6 implementation
7. Specification traceability check
8. Itemized test plan
9. Incremental implementation
10. System test

## License

Copyright (C) 2024-2026 PyCircuit Contributors

# LinxCore DSE Guide

This guide defines a migration direction for a parameterized LinxCore frontend.

## Objectives

- Move structural config derivation into `@const` code.
- Represent interface/stage contracts with `spec` objects.
- Reduce manual port/instance wiring with `new` + spec-driven helpers.
- Keep deterministic parameter-space expansion for reproducible sweeps.

## Recommended structure

1. `common/config.py`
- Provide template-derived normalized parameter sets.
- Validate constraints once at compile time.

2. `common/meta_specs.py`
- Build `BundleSpec`/`StructSpec` from canonical stage contracts.
- Export helpers consumed by top/stage modules.

3. top-level composition
- Use spec declarations (`inputs/outputs`) for external ports.
- Use `new` for repeated interface wiring paths.

4. sweep scripts
- Use `spec.dse.product/grid/filter` to generate variant sets.
- Emit deterministic names and manifests for each variant.

## Migration checkpoints

- Wave 1: top composition + frontend shell modules use spec-driven wiring.
- Wave 2: decode tables authored from `DecodeRule` sets.
- Wave 3: backend internal generators parameterized for ROB/IQ/issue/store structures.

## Candidate hotspots from current LinxCore sources

1. Interface and port boilerplate
- `/Users/zhoubot/LinxCore/src/top/top.py`
- `/Users/zhoubot/LinxCore/src/bcc/backend/engine.py`
- `/Users/zhoubot/LinxCore/src/bcc/backend/rob.py`
- Primary migration: `spec.BundleSpec` + `m.inputs/m.outputs` + `m.new`.

2. Repeated slot/lane logic
- `/Users/zhoubot/LinxCore/src/bcc/backend/engine.py`
- `/Users/zhoubot/LinxCore/src/common/exec_uop.py`
- `/Users/zhoubot/LinxCore/src/bcc/backend/issue.py`
- Primary migration: `@const` loops that emit immutable lane/index plans and parameterized `ParamSet`-driven unroll factors.

3. Decode/control rule density
- `/Users/zhoubot/LinxCore/src/common/decode.py`
- `/Users/zhoubot/LinxCore/src/common/decode_f4.py`
- Primary migration: `spec.DecodeRule` rulesets with template-generated mask/match/update tables.

4. Config derivation and DSE sweeps
- `/Users/zhoubot/LinxCore/src/common/config.py`
- `/Users/zhoubot/LinxCore/tools/perf/dse_frontend_icache.py`
- Primary migration: `spec.params()` + `spec.dse.product/grid/filter` for deterministic variant manifests.

## Frontend capabilities required by these hotspots

- Templates can return immutable spec objects and nested containers.
- Templates are hard-pure: any hardware emission or module mutation raises `JitError`.
- Strict JIT now supports chained comparisons (`a <= x <= b`) and template/spec subscripting (`param_set["k"]`) in elaboration code.

## Hygiene gates

- No old inline-helper decorator usage in migrated paths.
- No removed method-style frontend APIs in migrated paths.
- Deterministic compile outputs for identical parameter sets.

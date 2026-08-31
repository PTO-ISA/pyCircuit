# Frontend API

pyCircuit 6 uses CycleAwareSignal as its primary scalar authoring model. The
cycle-aware surface and structural library surface lower to the same verified
`pyc` MLIR.

## Cycle-aware imports

```python
from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    CycleAwareTb,
    ForwardSignal,
    Tb,
    cas,
    compile_cycle_aware,
    mux,
    submodule_input,
    testbench,
    wire_of,
)
```

Use `domain.signal()` plus `<<=` or `.assign()` to infer state. Use
`domain.next()` to advance the logical cycle. See the
[V6 specification](v6_PyCircuit_Specification.md) for the normative API and
cycle-balancing rules.

## Structural decorators and library API

The structural API remains supported for explicit hierarchy, static
metaprogramming, and reusable libraries. It does not replace the V6
CycleAwareSignal timing model.

### Core decorators

- `@module`: hierarchy-preserving boundary (materializes `pyc.instance`)
  - boundary-dynamic value ports: `@module(value_params={"gain": "i8", "sel": "i1"})`
  - `value_params` are runtime module IO values (not specialization params)
- `@function`: inline helper (inlined into the caller)
- `@const`: compile-time helper (pure; may not emit IR or mutate the module)
- `@testbench`: host-side cycle test program lowered via a `.pyc` payload

### Structural imports

```python
from pycircuit import Circuit, compile, module, function, const, testbench
from pycircuit import ct, spec, wiring, logic, lib
```

### Circuit authoring API

Declarations:

- `m.clock(name)`, `m.reset(name)`
- `m.input(name, width=..., signed=False)`
- `m.output(name, value)`
- `m.inputs(spec, prefix=...)` / `m.outputs(spec, values, prefix=...)`
- `m.io(signature, prefix=...)` (directioned signature IO)

State and pipeline:

- `m.out(name, clk=..., rst=..., width=..., init=...)` (register)
- `m.state(spec, clk=..., rst=..., init=..., en=..., prefix=...)`
- `m.pipe(spec, src_values, clk=..., rst=..., en=..., flush=..., init=..., prefix=...)`

Instantiation:

- `m.new(fn, name=..., params=..., bind=...)`
- `m.array(fn_or_collection, name=..., keys=..., per=..., params=..., bind=...)`

Wiring:

- `m.connect(dst, src, when=...)`
- `wiring.bind(spec_or_sig, connector_bundle_or_struct)`
- `wiring.ports(m, bind)`
- `wiring.unbind(...)`, `wiring.unflatten(...)` (debug/inspection helpers)

### `spec`, `logic`, and `lib`

`spec` (compile-time shapes):

- `spec.struct("name").field("a.b", width=...).build()`
- `spec.bundle("name").field("x", width=...).build()`
- `spec.signature(...)` for directioned IO leaves
- `@spec.valueclass` for canonical compile-time config objects

`logic`:

- `logic.onehot_mux(sel, vals)`
- `logic.priority_pick(bits, n=...)`
- `logic.match_any(key, keys, valids=None)`

`lib`:

- `lib.StreamSig(...)` (ready/valid signature builder)
- plus structural blocks under `pycircuit.lib.*`

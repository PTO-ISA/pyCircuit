# Spec collections

`pycircuit.spec` supports compile-time module collections that elaborate into fixed hardware instance graphs.

## Types (selected)

- `ModuleFamilySpec(name, module, params=None)`
- `ModuleListSpec`, `ModuleVectorSpec`
- `ModuleMapSpec`, `ModuleDictSpec`

## Builder shape

```python
family = spec.module_family("lane", module=build_lane, params={"width": 32})
lanes  = family.list(8)
vec    = family.vector(8)
mset   = family.map(["alu", "bru", "lsu"])
dct    = family.dict({"alu": {"gain": 1}, "bru": {"gain": 2}})
```

## Elaboration API

Use `Circuit.array(...)` to elaborate a collection into instances.

## Determinism

- Collection key ordering is canonicalized.
- Per-instance names are deterministic.

## Binding policy

Binding is strict exact-match:

- missing keys: error
- extra keys: error
- width/signed mismatch: error

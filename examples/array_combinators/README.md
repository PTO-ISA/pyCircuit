# Array combinators

`ArrayCombinators` preserves the map-oriented `array_combinators` root from
historical commit `8887e6de` (`examples/agentic-circuit/blocks/array_combinators.py`).
The original three-element `values` and `narrow` arrays remain
`table[3, u8]` and `table[3, u4]`. Its two-by-three nested pure array is
represented as one row-major `table[6, u8]`:
`(first, second, third, third, second, first)`. This keeps the nested six-element
transformations within the current one-dimensional Table contract without
expanding the scalar maps.

The design covers an expression lambda whose parameter shadows an outer name,
the named `bump` callback, a three-lane multi-input map producing `NamedPair`, a
three-lane map producing `NextPair`, and a three-lane `Item` record map. One
`bump` map transforms all six nested lanes. Immutable first-column replacement
uses one six-lane multi-input map with a column-index Table. The second row's
last and first projections are row-major indices five and three.
`decode_checked` is a named scalar callback that returns the input below five
and zero otherwise; the three reported fields explicitly slice those mapped
`u8` values to three bits.

The eleven result fields retain their original order and widths for a total of
62 packed bits. The module transports requests and complete results through two
depth-one queues using `ready_policy="downstream_pop"`, so stalls preserve one
complete token at each stage.

`ArrayCombinatorsSystem` supplies the compact requests `(0,1,4)`, `(3,5,7)`,
and `(255,2,254)`. Its same-source expected records contain independent literal
values for every result field. The closed system checks all three accepted
requests, all three outputs, and a final drained cycle.

This example uses the existing one-dimensional Table map surface. It does not
claim general nested Table callbacks, historical tuple/zip/checked/index APIs,
or arbitrary callback results. The closed system has compiled and run through
generated C++ and Verilator from one linked artifact. Four-state behavior and
wider nightly coverage are not claimed.

The independent native and RTL drivers check 76 unique known-value vectors
across 191 Work samples. They retain the three historical packed goldens, cover
checked-conversion boundaries before truncation, and exercise stalls, held
clocks, full replacement and reset while full. Native workers one/two and
Verilator agree on ready/valid and every result bit.

```sh
cmake -S examples/array_combinators -B .pycircuit_out/array-combinators -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build .pycircuit_out/array-combinators --parallel 4
ctest --test-dir .pycircuit_out/array-combinators --output-on-failure --no-tests=error
pycircuit run examples/array_combinators --target cpp --cycles 12
pycircuit run examples/array_combinators --target verilog --cycles 12
```

[Generated excerpts](GENERATED.md) and the [artifact receipt](GENERATED.json)
bind the module verification. The DUT is 153 lines and the compact system bench
is 172 lines; neither expands the stimulus into per-cycle hardware rules.

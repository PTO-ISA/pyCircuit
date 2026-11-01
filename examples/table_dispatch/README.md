# Table dispatch

This System distributes the values 1 through 8 across four independent
accumulators. `lanes: ac.table[4, Accumulator]` creates four Module instances;
the two-bit `selected` State chooses which existing instance receives each
Method call. The selector wraps after lane 3.

The dispatch Rule updates the selected accumulator, selector and input counter
as one transaction. `add` returns its computed new total as an ordinary scalar.
The separate observation Rule reads committed state, so it sees each update on
the following cycle. Neither Rule exposes storage pins or admission plumbing.

After eight successful rising edges, the lane totals are `1 + 5 = 6`,
`2 + 6 = 8`, `3 + 7 = 10` and `4 + 8 = 12`. The ninth cycle observes these
totals while the dispatch guard holds further updates.
[expected.json](expected.json) records the independently calculated dispatch
results and all four totals at every cycle. The shared example verifier checks
both low and high Work samples through C++ workers 1/2 and Verilog.

With an installed toolchain:

```sh
pycircuit run examples/table_dispatch --cycles 9
pycircuit run examples/table_dispatch --target verilog --cycles 9
cmake -S examples/table_dispatch -B /absolute/build/table_dispatch -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/table_dispatch --parallel 4
ctest --test-dir /absolute/build/table_dispatch --output-on-failure --no-tests=error
```

This example covers a known, in-range selector and scalar Method arguments and
results. Reset, partial data-Table updates, guarded selected calls and narrower
selectors have separate compiler tests; this scenario does not establish those
behaviors or replace the retained `table_rule` Queue and four-state oracles.

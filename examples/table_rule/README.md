# Atomic table replacement

`TableRule` owns two `Entry` records and two queues. Its `transfer` Rule pops a
request, calls the local `install` Rule, and pushes the old entry as the response.
The table update and both Queue operations commit together. A blocked response
holds the entire transaction; the frontend does not repeat Queue readiness checks.

`install` captures the Module's table directly. Its `old` local is a value
snapshot, so writing the selected entry does not change the returned old value.
The ordinary Python call adds no extra cycle.

The 16-cycle System sends six replacements across both slots, stalls one output,
checks every expected response and requires all six completions. The standalone
C++/RTL oracle and four-state regression retain their full independent coverage.
The old 1,343-cycle literal-vector System is preserved under
`tests/fixtures/historical_examples/system_benches/table_rule/`; its original
trace still has a separate unit-test owner. It is not the current tutorial bench.

```sh
pycircuit run examples/table_rule --target cpp --cycles 16
pycircuit run examples/table_rule --target verilog --cycles 16
cmake -S examples/table_rule -B /absolute/build/table_rule -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/table_rule --parallel 4
ctest --test-dir /absolute/build/table_rule --output-on-failure --no-tests=error
```

[Verified generated excerpts](GENERATED.md).

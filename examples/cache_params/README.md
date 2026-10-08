# Fixed cache address fields

[Generated MLIR, C++ and RTL](GENERATED.md)

The design fixed ways=4, sets=64, line_bytes=64, addr_width=40 and
data_width=64. Its ordinary compile-time helper derived six offset bits and
six index bits. This design preserves the unsigned slice `tag=addr[12:40]`, plus
the observable nine-bit constants `line_words=8` and `tag_bits=28`.
The original root had no public configuration parameters, state or clock.

```sh
cmake -S examples/cache_params -B /absolute/build/cache-params -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/cache-params --parallel 4
ctest --test-dir /absolute/build/cache-params --output-on-failure --no-tests=error
```

The source uses `addr: ac.u40` and returns a typed `CacheResult`. Its nine-bit
constants are declared struct defaults, so the constructor supplies only the
tag. Both backends consume the same existing bit-extract operation. Low
twelve X/Z bits are discarded; retained X and Z remain at their tag positions.
The original zero-address case, both sides of the bit12 boundary, high bit39,
maximum address, consecutive tags and all three outputs are checked by
independent native/RTL oracles. No cache-specific compiler rule or source
helper execution is needed. Acceptance is recorded in the historical inventory.

## Generated system usage

`bench.py` exports `example_cache_params.bench.ExerciseCacheParams`. Compile sources in
order `cache_params.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/cache_params --target cpp --cycles 14` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 14 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.

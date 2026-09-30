# Benchmarks

The old builder/QueueGraph performance drivers have been retired. Current
M5 evidence measures correctness of the serial source-owned route. Scale,
incremental compilation and parallel scheduling benchmarks belong to M6 and
must invoke the public compile/link/emit driver against a pinned candidate.
Historical benchmark implementations remain in Git, outside the active route.

The [M6-02 packet](../docs/work-items/m6-incremental-scale.md) measures shared
definitions at 1/16/64 instances and distinct leaves at 1/8/32 sources. Generate
fixtures with `benchmarks/pycircuit/m6-source-units/generate.py`, or run the full
public-driver measurement from the checkout:

```sh
python flows/tools/measure_m6_build.py --prefix <installed-prefix> --output-dir .pycircuit_out/m6-measure
```

The output directory must be absent or empty. Prefix metadata must match the
checkout revision; dirty candidate bytes are separately bound in gate evidence.
The report records executed Ninja commands, source and model invalidation,
artifact sizes, wall times including startup, and literal simulation oracles.
It does not measure RSS or promise a performance threshold or parallel simulation.

# External FIFO boundary

`FifoLoopback` exposes a depth-two, latency-one FIFO through typed input pins and
one `FifoResult`. This example specifically demonstrates an external ready/valid
protocol. Ordinary internal module connections should use the typed Rule calls
shown in `queue_rule_pipeline`.

The FIFO preserves all eight payload bits. A full pop/push edge removes the old
head, appends the new token, and keeps occupancy unchanged. There is no empty
bypass. `out_valid` describes an available head even while `out_ready` is zero;
it is not a commit signal.

Reset empties the FIFO. Held/falling clocks do not transfer data. The generated
compiler flow owns clocks and state storage; no additional authored Rule or
register is needed around the native Queue module.

The 344-cycle System exercises the boundary protocol. The separate host and
four-state RTL tests retain independent capacity, replacement, held-clock,
reset, X/Z payload and failure/Discard checks.

```sh
cmake -S examples/fifo_loopback -B /absolute/build/fifo_loopback -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fifo_loopback --parallel 4
ctest --test-dir /absolute/build/fifo_loopback --output-on-failure --no-tests=error
```

[Generated excerpts and verification inputs](GENERATED.md).

# Two isolated clock-domain records

`CounterLane` owns one eight-bit DFF and returns its count as a typed bounded
Integer scalar. `MulticlockDomainIsolation` creates two lanes with direct typed
calls and returns their independent counts as a two-element tuple.

The `DomainPins` Struct preserves each lane's physical `clk` and `rst` identity
and packed field order. The generated output ABI exposes lane A as `result_0`
and lane B as `result_1`; the C++ and RTL harnesses map those positions to the
existing `a_count`/`b_count` oracle names.

The original native and RTL stimuli remain authoritative. They independently
exercise both clock/reset domains, per-lane reset and hold behavior, 255-to-zero
wrap, failed unknown controls, X/Z reset isolation and recovery. No automatic
clock-domain scheduler or CDC behavior is implied.

```sh
cmake -S examples/multiclock_domain_isolation \
  -B /absolute/build/multiclock-domain-isolation -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/multiclock-domain-isolation --parallel 4
ctest --test-dir /absolute/build/multiclock-domain-isolation \
  --output-on-failure --no-tests=error
```

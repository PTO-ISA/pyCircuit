# Independent clock and reset pins

Two standard eight-bit DFFs increment modulo 256 on their respective rising
edges. `clk_a/rst_a` control only `a_count`; `clk_b/rst_b` control only `b_count`.
Reset is synchronous and active high, with value zero. Each leaf owns its own
clock history. The host drives both clock levels explicitly; a Work sample
observes old Q, and the following Work sees the edge's committed result.

```sh
cmake -S examples/multiclock_regs -B /absolute/build/multiclock-regs -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/multiclock-regs --parallel 4
ctest --test-dir /absolute/build/multiclock-regs --output-on-failure --no-tests=error
```

The original testbench declares identical default clocks and resets only A, without output
expectations. It remains separate from the stronger parity oracle, which
explicitly resets both lanes before checking independent/simultaneous edges,
hold, reset priority and wrap. Native host Reset initialization and four-state
RTL startup are different: unreset RTL B is X, and Verilator's startup zero
does not prove B was reset.
The old SV testbench generator emitted only the first declared clock, so that
generated smoke also did not prove B edge behavior. The new independent-edge
oracle exercises both clock pins of the original hardware design.

This example uses existing module binding, exact addition/masking and DFF
semantics. It demonstrates explicit pin sampling, without introducing a
clock-domain scheduler or CDC behavior. Known-output RTL is checked with
Verilator and Icarus. `PYC_MULTICLOCK_FOUR_STATE` additionally proves unreset B
stays X until its own reset edge, and X/Z reset pins are ignored without that
lane's edge. Unknown-clock RTL parity is not claimed. The original standard DFF
is compiled directly. Independent native and RTL tests cover this behavior.

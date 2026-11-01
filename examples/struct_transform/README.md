# Registered packet transform

Current migration status and remaining work are tracked in the
[catalog](../catalog.json) and [migration inventory](../README.md#remaining-migration-work).
The transactional System adapter preserves the packet's original capture timing;
the direct physical root still requires an export boundary.

`StructTransformStorage` owns one initialized `Packet` State containing
`op: ac.u4`, `dst: ac.u6`, `word: ac.u32`, and `valid: ac.u1`. Its
`capture_and_transform(packet)` Method captures the incoming packet and returns
an arithmetic projection of committed old Q in the same transaction:
`op`, `dst`, and `valid` are unchanged, while `word` is
`(old.word + old.op + 1) mod 2^32`.

The reset image is recursively zero. The first Work therefore reports
`op=0`, `dst=0`, `word=1`, and `valid=0`. An input captured on a rising edge
becomes the old state observed by a later Work. No extra result State or Queue
is introduced. The typed local `op_wide: ac.u32 = state.op` zero-extends the
opcode before addition; overflow wraps at 32 bits. `valid` remains packet data,
including when zero; it never gates capture.

The `StructTransform` wrapper retains the original four scalar inputs and
explicitly registers its calling Rule. It currently fails linking with
`transactional link requires a closed concrete root`. Its input-bearing physical
interface and same-Work projected output remain required migration work.

## System scenario

`bench.py` owns one storage instance and explicitly registers one Rule. The
Rule selects a scenario, passes its input fields together as one `Packet`,
checks the Method's same-cycle result, and advances the phase. All 72 original
scenario rows and their four independent expectations are retained unchanged.

The first row expects the reset projection. Row 71 and the known tail thereafter
retain input `(1, 2, 3, 1)` and expected output `(1, 2, 5, 1)`. The u64 phase
counter wraps normally; assertions are enabled below phase 72 and the four
observations are unconditional. The scenario Table's State is initialized once
and never written.

Run the generated System using the current toolchain:

```sh
pycircuit run examples/struct_transform --target cpp --workers 1 --cycles 72
pycircuit run examples/struct_transform --target cpp --workers 2 --cycles 72
pycircuit run examples/struct_transform --target verilog --cycles 72
```

The existing standalone CMake entry registers separate Module and System tests.
The System test can run while the physical Module boundary remains unfinished:

```sh
cmake -S examples/struct_transform -B /absolute/build/struct-transform -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
ctest --test-dir /absolute/build/struct-transform -R struct_transform_system \
  --output-on-failure --no-tests=error
```

## Retained physical verification

The original C++ and RTL drivers, 149 known Work samples, discard checks,
valid X/Z and unknown word/op probes remain unchanged. They are **NOT RUN** for
this migration because the direct Module root fails linking. The separate RTL
four-state branch also remains **NOT RUN**. System checks do not replace these
required physical-control and four-state cases.

[Generated excerpts and receipts](GENERATED.md) describe the superseded frontend
and remain historical evidence only. Once the physical export is supported,
build the standalone example and run both CTests, then its existing Icarus branch:

```sh
cmake --build /absolute/build/struct-transform --parallel 4
ctest --test-dir /absolute/build/struct-transform --output-on-failure --no-tests=error
iverilog -g2012 -DPYC_STRUCT_TRANSFORM_FOUR_STATE -s tb \
  -o /absolute/build/struct-transform/four-state.vvp \
  /absolute/pycircuit/install/include/verilog/*.v \
  /absolute/build/struct-transform/verilog/design_top.sv \
  /absolute/build/struct-transform/verilog/sources/example_struct_transform/struct_transform.v \
  examples/struct_transform/rtl_tb.sv
vvp /absolute/build/struct-transform/four-state.vvp
```

# Reproduction tools and library snapshot

The candidate evidence fixes these versions; this is not a latest-version claim.
All dependencies remain outside tracked source. No service or credentials are
changed by this example.

| Tool/data | Actual version/source | License and boundary |
| --- | --- | --- |
| ISA authority | [RV32I2.1, ISA manual v20260120](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html), accessed2026-11-02 | Ratified unprivileged specification; the demo EEI is separately declared. |
| Compiler | LLVM/clang/lld22.1.8, `--target=riscv32-unknown-elf -march=rv32i -mabi=ilp32 -mno-relax -O2 -ffreestanding -fno-builtin -nostdlib` | Apache-2.0 with LLVM exceptions. Existing toolchain reused; exact commands/disassembly are saved per program. |
| Spike | [riscv-isa-sim4c6a9bc79dab80616932949707e7c0471d8e9e54](https://github.com/riscv-software-src/riscv-isa-sim/tree/4c6a9bc79dab80616932949707e7c0471d8e9e54), version1.1.1-dev | BSD-3-Clause; independent verification only. SoftFloat submodule497c81c1f5016dc6ef5b3219ece2f3c261d4d8f1 retains its upstream license. |
| Icarus | [iverilog/vvp12.0](https://steveicarus.github.io/iverilog/) | GPL-2.0-or-later; actual physical RTL and mapped-netlist simulation. |
| SV conversion | [sv2v v0.0.12](https://github.com/zachjs/sv2v/releases/tag/v0.0.12), Linux release binary | BSD-3-Clause; standard source conversion, no architecture rewrite. |
| Synthesis | [Yosys0.33,2584903a060](https://github.com/YosysHQ/yosys/tree/2584903a060) and its ABC | Yosys ISC; ABC's included license retained. NoSTA/P&R claim. |
| Cell distribution | Existing open_pdks/Volare sky130A distribution `c6d73a35f524070e85faff4a6a9eef49553ebc2b` | [SKY130 documentation](https://skywater-pdk.readthedocs.io/en/main/contents.html), Apache-2.0 standard-cell models. No full-PDK rebuild/download was needed. |
| Cell library/corner | `sky130_fd_sc_hd__tt_025C_1v80.lib`, HD, typical process,25°C,1.8V; time1ns, capacitance1pf | Real standard cells, not generic gate estimates. Liberty area is checked against the owning distribution's LEF geometry. |
| Standard example gate | Repository pin Verilator5.048 | Distinct from this new model's Icarus evidence. |

Selected Liberty SHA256 is
`8e78e14442062dba34d414fca6490b2f6b96038d4510d1438ca44fee31487135`.
The distribution's revision and actual file are known. A separate producing
upstream HD library commit is **UNKNOWN**; a later repository HEAD is not used
as a substitute provenance claim. Standard-cell simulation models are
`sky130_fd_sc_hd.v` and `primitives.v` from that same distribution; their license
headers remain intact. Supply the actual Liberty, LEF and model paths explicitly.
No private fixture bundle is an input.

## Bounded local dependency installation

The normal Spike build is reproducible from the fixed upstream source and its
pinned SoftFloat dependency. Device-tree-compiler/libfdt and Boost regex/system
are host dependencies. In this run, Ubuntu packages were downloaded and extracted
into a local `deps/host` prefix with `apt download` and `dpkg-deb -x`; no service
or system install was changed. The package versions were device-tree-compiler/
libfdt1.7.0-2build1 and Boost1.83.0-2.1ubuntu3.2.

```sh
git clone https://github.com/riscv-software-src/riscv-isa-sim.git "$DEPS/spike-source"
git -C "$DEPS/spike-source" checkout 4c6a9bc79dab80616932949707e7c0471d8e9e54
git -C "$DEPS/spike-source" submodule update --init berkeley-softfloat-3
mkdir -p "$DEPS/spike-build"
cd "$DEPS/spike-build"
"$DEPS/spike-source/configure" --prefix="$SPIKE_PREFIX" --enable-commitlog
make -j4
make install
```

Use the host dependency prefix's `bin`, includes, library search path and runtime
library path when dependencies are not installed globally. The original run's
configure/build logs record those paths. Spike's debug command file stops on the
first trap, before a handler, and captures mcause/mepc/mtval plus all GPRs.
`--pcs=0:0x80000000` is necessary to bypass Spike's boot ROM; merely `--pc` does
not provide this execution contract. Privileged instructions remain unavailable
to the DUT; Spike's machine-mode trap bookkeeping supplies the selected EEI's
reference termination record.

Icarus and Yosys were already installed and reused. sv2v's fixed Linux release
archive was extracted into a local dependency prefix with its LICENSE/NOTICE.
The synthesis script records complete commands, SV conversion output, Yosys/ABC
logs, selected Liberty identity, mapped cell counts, cell areas and netlists.
Its normal cleanup splits mapped physical ports into scalar nets and checks exact
cell/type/parameter and logical-port/pin connectivity against the pre-cleanup
netlist. The metadata-driven adapter restores logical packing without CPU logic.
SKY130 functional simulation defines `FUNCTIONAL` and `UNIT_DELAY=#1`, with an
explicit1ns/1ps testbench timescale and a conservative functional-only clock.
It rejects unknown/unmapped cells, latches, failed checks and an empty sequential
cone. Missing tools or a library are failures/NOT_RUN, not synthesis passes.

## Interpretation limits

The fixed Spike build accepts FENCE.I under `--isa=RV32I`. Keep its raw log and
mark that reference disagreement. The test-owned specification expectation
requires an illegal-encoding trap for this closed RV32I2.1 demo without Zifencei;
it does not rewrite Spike or learn a golden from the DUT.

Architecture-test certification, full formal equivalence, OpenSTA and P&R are
NOT_RUN. Mapped program replay proves the exercised generated/mapped behavior;
it is not a frequency, signoff or universal equivalence proof. SOTA remains
NOT_ESTABLISHED without matched external measurements.

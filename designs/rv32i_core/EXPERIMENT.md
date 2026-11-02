# A reproducible five-stage RISC-V core experiment

This project shows how one pyCircuit hardware description becomes an executable
software model and a physical hardware description, and how those outputs can
be checked independently. The processor implements the 32-bit RISC-V base
integer instruction set, RV32I. Its five stages fetch an instruction, decode it,
execute arithmetic or a branch, access memory, and write back the oldest result.
Instructions overlap in these stages rather than running through a serial
instruction-at-a-time state machine.

The generated C++ and register-transfer-level (RTL) sources are included in this
project, together with a real standard-cell mapped netlist. A reader can inspect
or download them, compile them without regenerating the processor, and repeat
the C-program and dependency checks described here. The [artifact manifest](generated/artifacts.json)
records source revisions, generation settings, file sizes and SHA-256 checksums.
The [generated-file guide](generated/README.md) maps the emitted files to the
processor hierarchy. Build products, executable files and third-party libraries
remain outside the source tree.

## The design and its specification

The [design input](spec/design-input.md) sets the architectural and platform
requirements. The [core specification](spec/core-spec.md) refines these into
stage responsibilities, memory handshakes, state ownership, dependency handling,
reset and precise effects. The [acceptance document](spec/acceptance.md) identifies
independent checks. These documents use the pinned NDF 0.2 format, which gives
requirements stable identities and explicit refinement and verification links.
They remain draft specifications; a successful graph check does not approve a
design or prove instruction semantics.

The hardware source reflects those requirements directly. `types.py:Machine`
owns the register file, program counters and pipeline records. `core.py:Core`
connects pure computation modules, and `Core.step` commits one complete next
state. The `implements` annotations on these definitions refer to the Spec;
`verifies` annotations identify the independent test entrypoints. The existing
[NDF workflow](spec/README.md) checks those links separately from execution.

The processor has 32 integer registers, with x0 fixed at zero. Forwarding
supplies recent results to dependent instructions, a load-use dependency can
insert a bubble, and memory backpressure holds live instructions. A branch or
jump redirects fetch and cancels younger work. Stores become externally visible
only at ordered retirement; a trap prevents younger register or memory effects.
An execution environment interface (EEI) defines the little-endian Harvard
instruction/data memories, the 64 KiB accessible window beginning at 0x80000000,
and precise halt-on-trap behavior. This is an unprivileged base-integer demo;
privileged execution, interrupts, caches and optional ISA extensions are not
part of its interface.

This delivery uses main revision `3ee29b077725638081b69dcb0b267979ade07f1f`.
That revision includes the held-operand repair: when an older memory operation
blocks the execute stage, operands forwarded from a retiring producer are saved
until the consumer can advance. The generated files and synthesis below were
created from this repaired design. Earlier processor netlists and area figures
are historical and are not used as these results.

## What functional, timed, C++ and RTL mean

Execution policy and output language are separate choices. Timed execution
visits every model cycle. Functional execution can omit only proven idle
fixed points while preserving architectural state, dependency and effect order.
C++ is a software execution backend; RTL is the clocked hardware backend. They
do not themselves define the two execution policies.

Both policies were generated from the same source and independently exercised
through the public SDK entrypoint. Within each policy, C++ and RTL consumed the
same verified intermediate representation (IR). All emitted computation files
are byte-identical between the two policies; only the execution-policy field in
the original generated descriptor differs. The repository therefore keeps one
C++ payload and one RTL payload, with the functional descriptors beside them.

The archival consumer deliberately samples every physical cycle for direct
comparison. It does not benchmark idle-cycle compression. In the fast-memory C
case there are no omitted wait cycles: all four views have 567 virtual cycles
and 567 executed ticks. The optimized SDK execution path remains available via
the source-generation commands below.

## Environment and installation

The experiment was run on Linux x86-64. The exact tool and cell-view identities
are also recorded in [the tool notes](verification/TOOLS.md) and the manifest.

| Component | Version used |
| --- | --- |
| pyCircuit source and SDK | main `3ee29b077725638081b69dcb0b267979ade07f1f` |
| SDK compiler dependency and firmware clang/lld | LLVM/MLIR 22.1.8 |
| Generated C++ consumer | GCC 13.3.0, C++20, Release build |
| Python | 3.12.3 for the C replay; 3.14.6/pytest 9.1.1 for held-operand checks |
| RTL simulation | Icarus Verilog/vvp 12.0 |
| RTL conversion and synthesis | sv2v 0.0.12; Yosys 0.33 (`2584903a060`) and ABC |
| Independent ISA reference | Spike `4c6a9bc79dab80616932949707e7c0471d8e9e54` |
| Cell views | Volare sky130A `c6d73a35f524070e85faff4a6a9eef49553ebc2b`, HD typical/25°C/1.8 V |

The SDK was built from the stated clean source revision using the normal
CompilerDev/Runtime configuration. LLVM was an existing matching external
dependency. The standalone C++ consumer asks CMake only for the SDK's Runtime
component; it does not discover LLVM or invoke the source compiler.

For full source generation, install the pinned SDK using the documented
[bootstrap](verification/TOOLS.md). Select paths through variables:

```sh
export PROJECT="$PWD/designs/rv32i_core"
export OUT="$PWD/.pycircuit_out/rv32i-artifact-experiment"
python3 "$PROJECT/verification/setup_sdk.py" \
  --llvm-root "$LLVM_ROOT" --output "$OUT/sdk"
export PYC_TOOLCHAIN_ROOT="$OUT/sdk/install"
```

Use the fixed Spike build and its device-tree-compiler dependency as documented
in the tool notes. Set `RV32_CLANG` and `SPIKE` to their executable paths; add the
host dependency prefix containing `dtc` to PATH when it is not installed globally.
The held-operand entrypoint additionally imports pytest and the existing
independent fixtures; a Python environment with pytest 9.1.1 is sufficient.

The standard-cell files are external dependencies, not bundled source. Their
fixed [Volare release](https://github.com/chipfoundry/volare/releases/tag/sky130-c6d73a35f524070e85faff4a6a9eef49553ebc2b)
provides the HD library; its model headers carry Apache-2.0 licensing. An existing
installation can be reused. Otherwise Volare 0.20.6 supports a library-scoped fetch:

```sh
volare enable --pdk-root "$PDK_ROOT" --pdk sky130 -l sky130_fd_sc_hd \
  c6d73a35f524070e85faff4a6a9eef49553ebc2b
export HD="$PDK_ROOT/sky130A/libs.ref/sky130_fd_sc_hd"
export SKY130_HD_LIBERTY="$HD/lib/sky130_fd_sc_hd__tt_025C_1v80.lib"
export SKY130_HD_LEF="$HD/lef/sky130_fd_sc_hd.lef"
export SKY130_HD_VERILOG="$HD/verilog/sky130_fd_sc_hd.v"
export SKY130_HD_PRIMITIVES="$HD/verilog/primitives.v"
```

The distribution and selected view checksums are known. A separate producing
upstream HD library commit is UNKNOWN; the report does not substitute a later
branch head. The reproduction checker rejects different selected Liberty or
cell-model bytes. The emitted project sources follow this repository's BSD
3-Clause license; tool and library licensing remains with those dependencies.

## The C application and independent reference

[The application](programs/c-array.c) fills 32 unsigned words, sums them and forms
a shift/XOR checksum. It writes four signature words, then executes EBREAK.
The firmware is compiled with `-march=rv32i -mabi=ilp32`, without a hosted C
runtime. This provides a real compiled program with loops, arithmetic, loads,
stores and control flow rather than a few manually driven arithmetic operations.

```sh
python3 "$PROJECT/verification/prepare.py" \
  --clang "$RV32_CLANG" --spike "$SPIKE" \
  --case c-array --output "$OUT/programs"
```

Preparation retains the ELF, disassembly, initial image and Spike trace in the
chosen output directory. Spike executes exactly that ELF and produces an
independent retirement and final-state expectation. The expected signature is
`[10208, 424583933, 309, 329]`, also calculated independently from the C algorithm.
No reference result is used to compute a DUT instruction result.

## Running the included generated files

```sh
python3 "$PROJECT/verification/run_generated.py" \
  --prefix "$PYC_TOOLCHAIN_ROOT" --program "$OUT/programs/c-array" \
  --output "$OUT/compiled-artifacts"
```

This command first verifies the committed payload checksums. It compiles the
included C++ through its CMake entry and the included RTL through Icarus, then
runs each descriptor with the same initial memory. The small test adapters only
supply clocks/reset and serve explicit memory requests. The processor alone
computes instruction results and its architectural state. Neither adapter is a
second CPU, and neither imports the pyCircuit compiler's private Python helpers.

The checker compares each retired PC/instruction, register write, memory effect
and trap to the independent reference. It checks every final register, PC,
control state, RAM byte and signature word. Store count, address, mask and data
must match independently retired stores exactly. Full packed state is then
compared across all four views.

| Included payload | Execution descriptor | Normal retirements | Expected traps | Virtual cycles |
| --- | --- | ---: | ---: | ---: |
| C++ | timing | 434 | 1 | 567 |
| C++ | functional | 434 | 1 | 567 |
| Physical RTL/Icarus | timing | 434 | 1 | 567 |
| Physical RTL/Icarus | functional | 434 | 1 | 567 |

The trap is an expected breakpoint termination, not another successful retired
instruction. The new runs exercise five simultaneous stage occupants, two
load-use bubbles and 64 redirects. Icarus retains two warnings about internal
`pyc_root_commit_ok` input-port coercion; they are not suppressed. The complete
program/state comparison and synthesis checks are separate evidence for the
exercised behavior, not a claim about every simulator or every possible input.

## Held operands under backpressure

The main-branch held-operand fixtures independently encode three short programs.
They place a consumer in execute while an older load waits for memory and its
producer retires. They check ALU input, Store data and Store address dependencies.
The data response is delayed by two cycles, so simply testing fast memory would
not cover the repaired behavior.

```sh
python3 "$PROJECT/verification/check_generated_held.py" \
  --compiled "$OUT/compiled-artifacts" --output "$OUT/held-operands"
```

This reuses the already compiled included DUTs, not a newly generated model.
It runs the three original fixtures across both descriptors and backends and
checks exact instruction/retirement order, all 32 registers, memory, Store count
and observed waiting cycles. In the ALU case x4 must be 5. The fixtures and
expected values remain owned by `test_held_operands.py`; they are not learned
from the generated processor output.

## Synthesis, area and mapped simulation

The [synthesis directory](results/synthesis/README.md) contains the new flattened,
standard-cell mapped Verilog netlist, converted synthesis input, original Yosys
command sequence with portable path placeholders, cell inventory, area/statistics
and the complete log with local path prefixes normalized. The source files and
netlist are preserved byte-for-byte. A compact port map supports simulation;
the large intermediate Yosys graph JSON is reproducible rather than duplicated.

The mapping reads the real Liberty library, checks hierarchy, lowers the design,
rejects latches and generic unmapped cells, maps sequential cells and invokes
ABC for combinational mapping. Checks report no undriven/multiple-driver problem
or combinational loop. Normalization preserves exact cells/types/parameters and
logical port/pin connectivity; it changes serialization and aliases only.

The new main-based result is **13,860 standard cells**: 2,076 sequential and
11,784 combinational. Liberty cell area is **109,284.8128 µm²**, comprising
41,559.8592 µm² sequential and 67,724.9536 µm² combinational area. Every used cell's
Liberty area is cross-checked against the selected LEF width times height. The
core boundary includes the register file, pipeline/control and architectural
observation logic. External instruction/data memories and the testbench are not
included. This is mapped cell area, not placed core or die area.

To reproduce mapping from the included physical RTL:

```sh
python3 "$PROJECT/verification/synthesize.py" \
  --generated "$PROJECT/generated/rtl" --toolchain "$PYC_TOOLCHAIN_ROOT" \
  --sv2v "$SV2V" --yosys "$YOSYS" --liberty "$SKY130_HD_LIBERTY" \
  --lef "$SKY130_HD_LEF" --output "$OUT/remapped"
```

To check the included netlist and repeat the mapped C application:

```sh
python3 "$PROJECT/verification/check_mapped_artifact.py" \
  --liberty "$SKY130_HD_LIBERTY" --cells "$SKY130_HD_VERILOG" \
  --primitives "$SKY130_HD_PRIMITIVES" --yosys "$YOSYS" \
  --trace "$OUT/compiled-artifacts/timing-cpp" --output "$OUT/mapped-check"
```

The latter reads the actual included netlist, recomputes its cell inventory and
area with the selected Liberty, and checks them against the published result.
It then simulates the mapped cells with inputs whose source-DUT retirement,
registers and memory have already passed independent reference checks. It
compares every output packet and the final complete state. Cell functional
models use `FUNCTIONAL` and `UNIT_DELAY=#1`; the generous test clock is for
functional verification, not frequency estimation.

## Re-generating instead of using the snapshot

For each execution policy and backend, the ordinary public command can rebuild
from the current source. Choose distinct build directories:

```sh
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" run "$PROJECT" \
  --model rv32i_core.core.Core --execution timing --target cpp \
  --memory "$OUT/programs/c-array/simulation.json" --build-dir "$OUT/from-source"
```

Select functional and/or verilog to obtain the other views. The provenance
manifest binds both verified IR hashes to the emitted snapshot. The SDK commands
preserve the source-capture, explicit compile/link, common-IR and backend route.

## What the experiment establishes

This delivery newly generates all views from the repaired main-based source,
newly compiles and executes the included C++ and RTL, exercises the held-operand
fixtures on those included files, and newly maps the resulting RTL to SKY130.
[Machine-readable reproduction results](results/reproduction.json) and retained
compact logs identify the actual outcomes. Older 160-view ISA campaigns and
older netlists/areas remain separately labelled history; they are not counted
as new runs here.

The C application and targeted dependencies are bounded verification, not an
official RISC-V certification or an exhaustive proof of the processor. Mapped
simulation is not formal equivalence. Static timing analysis and placement/
routing were not run, so no maximum-frequency or routed-area result is claimed.
The Spec remains draft and its independent approval is separate from these
execution results. These limitations do not prevent inspecting, compiling and
repeating the included artifacts with the declared dependencies.

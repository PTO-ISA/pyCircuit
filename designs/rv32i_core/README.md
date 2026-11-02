# Same-source five-stage RV32I core

[Complete experiment and reproduction](EXPERIMENT.md) · [Generated C++/RTL](generated/README.md) · [Mapped netlist and synthesis](results/synthesis/README.md)

A simple32-bit, single-issue, in-order IF/ID/EX/MEM/WB CPU demonstrates the shared
functional/timing tool extension. One source owns computation, architectural
state and precise effects; it generates executable native and physical RTL.
No separate functional CPU, imported RTL CPU or ISS numeric bypass is used.

The required public SDK revision is recorded in `pycircuit-dependency.json`.
Start with the [NDF design input, Spec and validation workflow](spec/README.md)
for the architectural requirements, source bindings and reproducible acceptance.
The specification is a draft for review; implementation results are recorded
separately.

## Directory map

The checked-in generated snapshot and mapped netlist are a deliberate part of
this project's reproducible deliverable. New build outputs remain under the
selected ignored output directory; SDKs, executables and cell libraries remain
external dependencies.

| Path | Responsibility |
| --- | --- |
| `spec/`, `ndf.yaml`, `ndf.lock` | Versioned design input, L1/L2 specification, L3 acceptance and exact NDF dependency |
| `generated/`, `results/synthesis/` | Browsable generated C++/physical RTL, mapped netlist and synthesis records |
| `programs/c-array.c`, `EXPERIMENT.md` | C application input and complete natural-language experiment |
| `types.py`, `decoder.py`, `arithmetic.py` | Shared records/reset values and pure RV32I decode/compute units |
| `writeback.py`, `memory.py`, `execute.py`, `frontend.py`, `core.py` | Actual phase hierarchy, one state owner and atomic commit |
| `CMakeLists.txt`, `simulation.json`, `link.ld` | Public source-unit closure, blocking platform roles, fixed EEI link layout |
| `verification/programs.py`, `prepare.py` | Finite assembly/random/C inputs, ELF/image/disassembly, independent Spike oracle |
| `verification/suite.py`, `verify.py`, `check_results.py`, `clean_consumer.py` | Four-view architecture/cycle/state checks and clean installed consumption |
| `verification/synthesize.py`, `mapped_check.py` | Unmodified physical RTL to real-cell mapping and metadata-driven test adapter |
| `verification/TOOLS.md` | Fixed tools/library sources, licenses, PVT, SHA and installation notes |
| `RESULTS.md`, `results-baseline.json` | Reviewable outcome summary, historical evidence scope and explicit limits |

## ISA, EEI and actual pipeline

The target is all40 instructions of RV32I2.1, ISA manual v20260120:32GPR/x0,
integer ALU/shift/compare, LUI/AUIPC, jumps, six branches, byte/half/word
loads/stores and FENCE. The little-endian demo EEI uses reset PC80000000 and a
64KiB window80000000..8000ffff. Blocking Harvard word ports expose completion
ready, access fault and byte enables. Naturally misaligned accesses/control
transfers trap. ECALL/EBREAK/illegal instructions halt precisely, never as NOPs.
M/A/C/F/D, CSR/privileged execution, interrupts, cache/MMU and Linux are outside
this demo. The repository's `docs/development/rv32i-core-contract.md` gives exact
trap values, ordering, reset and memory contracts.

Five stages have real state/valid bits and overlapping instructions. EX/MEM/WB
forwarding, ID write-through, a load-use bubble, retained memory responses and
backpressure prevent data loss. Redirects flush younger work and drain a pending
wrong-path fetch. Stores issue only in WB and commit exactly once at successful
completion/retirement. Oldest exception wins; younger register/store effects
cannot commit. Reset clears the pipeline and cancels pending platform requests.
Independent ALU instructions retire every cycle after fill; this is tested at
cycles4..67 with five occupied stages, separately from final-state equality.

Timing executes every model tick. Functional uses the same generated DUT and
folds only a complete-known-state fixed point with stable inputs and no event or
handshake. Virtual cycles, executed ticks and retirement steps are distinct.
Architectural timers cannot silently disappear. This bounded profile does not
claim an arbitrary ISA interpreter or arbitrary timing-IR projection.

## Environment and pinned SDK

Requirements: Python3.11+, CMake3.25+, Ninja, C++20 compiler; exact LLVM/MLIR22.1.8
for the tool SDK; RISC-V-capable clang/lld22.1.8 for firmware; Icarus12 for physical
RTL; fixed Spike for the independent oracle; sv2v0.0.12 and Yosys0.33/ABC for
mapping. Linux SDK runtime dependency bundling needs patchelf. Existing tools may
be reused; third-party source/build output is never committed here.

From a clean checkout of this repository, select local dependency/output paths:

```sh
export DEMO_OUT="$PWD/.pycircuit_out/rv32i-demo"
# LLVM_ROOT contains the externally installed LLVM/MLIR22.1.8 tree.
python3 designs/rv32i_core/verification/setup_sdk.py \
  --llvm-root "$LLVM_ROOT" --output "$DEMO_OUT/toolchain"
export PYC_TOOLCHAIN_ROOT="$DEMO_OUT/toolchain/install"
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" doctor --toolchain "$PYC_TOOLCHAIN_ROOT" --rtl
```

The bootstrap fetches the immutable merged upstream SDK commit, verifies it,
and performs a real standard build/install. It does not update credentials or
use a private SDK bundle. `TOOLS.md` documents fixed Spike/source dependencies
and standard open-tool installation. Set `RV32_CLANG`, `SPIKE`, `SV2V`, `YOSYS`
to those actual executable paths. The selected existing open_pdks/Volare sky130A
distribution is c6d73a35f524070e85faff4a6a9eef49553ebc2b, HD TT025C1v80. Supply
`SKY130_HD_LIBERTY`, `SKY130_HD_LEF`, `SKY130_HD_VERILOG`, `SKY130_HD_PRIMITIVES`
from that real distribution; exact selected SHA/license/PVT are in the results
and tool notes. Upstream producing HD commit remains UNKNOWN. No giant full-PDK
download is required when the selected standard-cell views already exist.

## Compile, execute and independently verify

```sh
python3 designs/rv32i_core/verification/prepare.py \
  --clang "$RV32_CLANG" --spike "$SPIKE" --output "$DEMO_OUT/programs"
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" run designs/rv32i_core \
  --model rv32i_core.core.Core --execution timing --target cpp \
  --memory "$DEMO_OUT/programs/c-array/simulation.json" \
  --build-dir "$DEMO_OUT/single-timing-cpp"
python3 designs/rv32i_core/verification/suite.py \
  --prefix "$PYC_TOOLCHAIN_ROOT" --programs "$DEMO_OUT/programs" \
  --output "$DEMO_OUT/validation"
python3 designs/rv32i_core/verification/check_results.py \
  --programs "$DEMO_OUT/programs" --results "$DEMO_OUT/validation"
```

Choose functional/timing independently of cpp/verilog, with separate build
directories. The suite compares each retirement PC/word/GPR/raw/memory/trap,
final32GPR/PC/RAM, exact external stores and C-source signature. Native/RTL cycle
checks and complete-state equality are separate. Fixed random seeds, waits,
redirects, x0/sign boundaries, illegal/alignment/access traps and waiting-store
reset retain failures/timeouts. The same ELF and initial image feed every view.
A successful execution receipt alone is not an independent numerical PASS.

The fixed Spike accepts FENCE.I even with its RV32I setting. Its raw log is
retained; four closed-RV32I2.1 admission views use a separately written illegal
expectation and are never counted as Spike agreement.

## Real-cell mapping and mapped C application

```sh
python3 designs/rv32i_core/verification/synthesize.py \
  --generated "$DEMO_OUT/validation/build/timing-verilog/verilog" \
  --toolchain "$PYC_TOOLCHAIN_ROOT" --sv2v "$SV2V" --yosys "$YOSYS" \
  --liberty "$SKY130_HD_LIBERTY" --lef "$SKY130_HD_LEF" \
  --output "$DEMO_OUT/mapping"
python3 designs/rv32i_core/verification/mapped_check.py \
  --generated "$DEMO_OUT/validation/build/timing-verilog/verilog" \
  --mapped "$DEMO_OUT/mapping/mapped.v" --netlist-json "$DEMO_OUT/mapping/mapped.json" \
  --trace "$DEMO_OUT/validation/runs/c-array/fast/timing-cpp" \
  --cells "$SKY130_HD_VERILOG" --primitives "$SKY130_HD_PRIMITIVES" \
  --output "$DEMO_OUT/mapped-check"
```

The flow uses ordinary synthesis/check, dfflibmap/ABC, alias cleanup and scalar
ports. Exact cell/type/parameter and logical-port/pin connectivity must be
unchanged by normalization. The test adapter restores packing and drives only
memory/reset/clock/observations; it implements no CPU. Real library functional
models use FUNCTIONAL and UNIT_DELAY=#1. ASIC observations require reset.

Reviewable complete results and historical/new-head boundaries are in RESULTS.md.
Area includes GPR/pipeline/control/observation logic, excluding external RAM and
testbench. Mapped cell area is not placed/die area. OpenSTA/P&R, official ISA
certification, full formal equivalence and independent review remain NOT_RUN.
SOTA is NOT_ESTABLISHED.

## Review regressions

The focused held-operand suite uses independently encoded RV32I words and
expected GPR, retirement and RAM/store results. It covers ALU, store-data and
store-address consumers stalled behind a load, under both execution policies
and both backends. It needs the selected installed SDK and Icarus, without
requiring an assembler or Spike:

```sh
PYC_TOOLCHAIN_ROOT=/path/to/installed/sdk python3 -m pytest \
  designs/rv32i_core/verification/test_held_operands.py -q
python3 -m unittest discover -s designs/rv32i_core/verification \
  -p test_check_results.py -v
```

The saved-results checker requires all four unique views for each selected
case/schedule. Empty, partial, duplicate or failed inventories cannot publish a
cross-view PASS. These focused regressions do not rerun the historical synthesis
campaign in RESULTS.md.

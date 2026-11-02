# RV32I five-stage demo and clocked fixed-point profile

Continuous task: tool mechanism → complete five-stage core → independent/native/
Icarus validation → real SKY130 mapping and mapped-demo verification. Intermediate
entries are not delivery completion. Starting HEAD e8340313 / core f7c7595 / main
base24fa8980. Local commits only; no remote writes, unrelated trees or SDK rebuild.

## Authority and EEI

Fixed authority: [RV32I2.1, ISA v20260120](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html),
accessed2026-11-02. Implement all 40 base instructions,32×32-bit GPR with x0 zero,
PC and little-endian byte/half/word memory. M/A/C/F/D, Zicsr, Zifencei, privilege,
interrupts, cache/MMU and Linux are outside this unprivileged demo. Legal HINTs
execute their ordinary x0-discard operation. FENCE conservatively orders all
memory; reserved fm/rs1/rd configurations remain FENCE as required. FENCE.I is
not base and traps illegal. Arithmetic wraps32bits; shifts use low5 count bits.

Reset PC80000000. Blocking Harvard word ports expose valid, byte address,
byte enables, write data, completion-ready/read data and access fault. A request
is held unchanged until completion; a write takes effect only at valid&ready
without fault, never while reset. RAM/image window80000000..8000ffff is used by
the demo platform. Instruction and data images start from the same ELF bytes;
code self-modification without instruction synchronization is not an exercised
program contract. Out-of-window requests fault. Fault signaling belongs to EEI,
not address special cases in the CPU. Naturally misaligned LH/LW/SH/SW trap;
LB/SB are byte aligned. Taken control targets require4-byte alignment after
JALR clears bit0; untaken branch target misalignment does not trap.

EEI halts on a precise trap, not a NOP/success instruction: causes0/1 instruction
alignment/access,2 illegal,3 breakpoint,4/5 load alignment/access,6/7 store
alignment/access,11 ECALL under the selected machine-like external environment.
No privileged CSR execution is added. tval is target/address, illegal bits,
breakpoint PC or0 for ECALL. Illegal bits are the low16 bits for a non32-bit
prefix and the low32 bits otherwise, matching this EEI's selected instruction
length reporting. Trapping instruction publishes one trap event, no
GPR or memory effect; no younger instruction can commit. Test PASS means matching
an explicitly expected EEI termination, not interpreting every trap as success.

## Five-stage microarchitecture

Real IF request/response state, IF/ID, ID/EX, EX/MEM, MEM/WB records with valid
bits; multiple instructions overlap. No serial all-instruction state machine.
Full-ready independent ALU stream sustains one retire per cycle after fill.
Operands forward from MEM-ready result then WB; ID has WB write-through. An
adjacent load-use inserts a bubble; memory waits hold dependent stages. Store
data gets the same bypass path. Pipeline ready propagation prevents overwrite.

Stores issue only from WB and retire on the same successful data-port handshake.
Loads issue in MEM; oldest WB store has data-port priority. Younger MEM accesses
cannot pass an older waiting store. A load response may be retained until WB is
free. These rules guarantee exactly-once stores and precise external effects.
FENCE needs no separate speculative memory mechanism under this stronger order.

Redirects are resolved on EX advancement, flushing younger ID/IF. An outstanding
wrong-path instruction fetch is drained and discarded rather than silently
cancelling the memory contract. Fetch buffers hold a completed response under
backpressure. Faults stop younger issue immediately and drain older instructions.
Priority is oldest WB trap, MEM fault/trap, then EX redirect/trap. Reset empties
all stages and cancels EEI outstanding requests; prior committed RAM writes are
not rolled back. Reset wins over external handshakes at the reset edge.

Retirement exports order, PC/instruction/nextPC, rd enable/index/raw value,
load/store effective address, mask/raw data and trap cause/tval. Trace observes
the pre-edge commit and architecture after edge; monitor code never implements
instruction arithmetic. Stage validity, stalls and redirect are independent
cycle evidence, distinct from architectural stream equivalence.

## Tool dependency and execution boundary

The example uses a pinned shared-model tool extension. The dependency and public
SDK bootstrap are recorded in `examples/rv32i_core/pycircuit-dependency.json`.
Its complete-state profile validates one state owner and pure hierarchy. Timing
executes every tick; functional folds only proved idle fixed points. CPU decode,
arithmetic, pipeline and trap semantics stay in this example's source units.
The example authors no second functional CPU and supplies no DUT results from
Spike or host arithmetic. The generic tool has its own non-CPU regressions.

## Historical baseline acceptance

These results describe pre-split candidate64730b9. Current publication validation
and artifact identity checks are recorded separately in the example's RESULTS.md
and machine-readable summaries. They are not inferred from these historical rows.

| Owner | Actual historical result |
| --- | --- |
| Generic compiler/profile | Non-CPU pure hierarchy/provider with complete Table/state witness; four execution/backend combinations; incomplete witness and hidden state reject. Native and physical RTL inactive/active unknown probes retain state. A visible timer executes all 28 ticks instead of folding waits. |
| RV32I computation and pipeline | All 40 declared instruction families plus EEI/illegal/alignment/access cases, x0 and signed/subword boundaries; independent ALU retires at cycles4..67 with five stages occupied. Smoke has exactly one load-use bubble. |
| Memory/effects | Zero, alternating and long fixed latency schedules; stable requests, exact external store count/bytes, wrong-path cancellation and reset during a waiting store. |
| Independent reference | Fixed official Spike, same ELF/image;27 programs,40 scenarios,160 passing views.156 views agree with Spike;4 FENCE.I admission views use the separately authored RV32I2.1/EEI expectation. The Spike disagreement remains in raw logs. |
| Native/RTL and state |80 native/Icarus cycle comparisons;40 complete-state groups agree across all four views. Same final IR bytes per selected strategy. C-source integer math independently checks signature `[10208, 424583933, 309, 329]`. |
| Functional reduction | C long-wait case:6481 virtual cycles,2782 executed ticks,3699 fixed-point idle cycles skipped. No architectural event or timer is erased. |
| Clean installed consumer | Public project copied into a clean temporary directory; C program passes functional/timing × C++/Icarus,434 normal retirements plus one expected trap. |
| Technology | SKY130 HD TT025C1v80:13596 cells,2076 sequential/11520 combinational;107784.624 µm², all used areas matched to LEF. Sequential area41559.8592 µm², combinational66224.7648 µm². Mapped Icarus C program passes567 cycles/434 normal retirements and complete final state. |
| Required local gates | API25/25; standard examples13/13 with Verilator5.048; Python/current profile842 passed,4 skipped. Later timer/failure-record scope:37/37. Formatting/docs evidence belongs to the final local candidate record. |

Source and mapped netlists have zero combinational SCCs. The normal mapping flow
uses `clean -purge`, `splitnets -ports` and standard Verilog serialization.
Exact cell/type/parameter and logical port/pin connectivity must remain identical
before/after normalization. This avoids Icarus strength/concat propagation costs
from wide alias buses; earlier180-second mapped simulation timeouts are retained.
No architectural cell or output bit is removed to obtain a simulation pass.
The test adapter restores bit packing using original and mapped port metadata.
Functional SKY130 models use `FUNCTIONAL` and `UNIT_DELAY=#1`; the generous
verification clock is not a PVT timing or frequency claim.

The four Python skips are three Windows-filesystem cases on Linux and optional
setuptools package-discovery coverage. Official architecture-test certification,
full formal equivalence, OpenSTA and placement/routing remain NOT_RUN. Independent
review is NOT_RUN after the configured Broker failed HTTP401 before an opinion.
SOTA is NOT_ESTABLISHED. This is a bounded local engineering candidate, not an
assertion that arbitrary timing IR, multi-owner scheduling or full SoCs work.

Independent review follows AGENTS/design-review skill. A requested unavailable
review is not rejection/approval. Preserve actual findings or explicit NOT_RUN.
The user's current local-candidate authorization permits implementation and
validation preparation to continue while a review mechanism is unavailable;
no independent-review result will be fabricated or release readiness inferred.

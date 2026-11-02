---
doc_id: RV32-DOC-SPEC
status: draft
authority: normative
owner: rv32i-demo
---

<!-- markdownlint-disable MD022 -->

# RV32I core specification

Revision 1, candidate. The clauses are design requirements for review, not an
approval or a claim that the implementation has passed them. Git records the
exact text; clause IDs persist across revisions. Implementation baseline:
`c00f45c1b023c89279c7ed7a9e519c83f8f456ba`.

## Instruction contract {#RV32-SPEC-ISA-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=architecture status=draft refines=RV32-REQ-ISA-001 -->

The decoder and execute path MUST implement this closed base set:

- ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND.
- ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI.
- LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU.
- LB, LH, LW, LBU, LHU, SB, SH, SW, FENCE, ECALL, EBREAK.

Arithmetic MUST wrap at 32 bits; shifts use five low count bits; signed
operations use two's-complement interpretation. JALR clears target bit 0.
Loads to x0 still perform their access and can fault; x0 discards only the
register write. Legal HINT encodings retain their ordinary x0-discard behavior.
FENCE MUST order this platform's memory effects; reserved FENCE fm/rs1/rd fields
are treated as specified by the selected base ISA. FENCE.I and CSR instructions
are outside this closed base and MUST trap under this EEI.

## EEI addresses and trap event {#RV32-SPEC-EEI-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=platform status=draft refines=RV32-REQ-EEI-001 -->

Reset PC MUST be 0x80000000. The platform memory window MUST be
0x80000000..0x8000ffff, initialized from the same ELF bytes for every view.
Out-of-window fetch/load/store requests MUST report access faults. Aligned
instruction targets require four bytes; a misaligned untaken branch MUST NOT
trap. LH/LHU/SH and LW/SW require natural alignment; LB/LBU/SB do not.

The precise halt event MUST report PC, instruction, cause and tval. Selected
cause values are 0/1 instruction alignment/access, 2 illegal, 3 breakpoint,
4/5 load alignment/access, 6/7 store alignment/access and 11 ECALL. These codes
are the external EEI convention, not an implementation of privileged CSRs.
Tval is the failing address/target, instruction bits for illegal, breakpoint
PC or zero for ECALL. For an unsupported non-32-bit prefix, illegal tval is the
low 16 instruction bits. A trapped instruction MUST have no GPR/store effect.
Self-modifying code without instruction synchronization is not admitted.

## Architectural and pipeline state ownership {#RV32-SPEC-STATE-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=pipeline status=draft refines=RV32-REQ-PIPE-001,RV32-REQ-EFFECT-001 -->

One finite Machine state owner MUST contain all GPRs, architectural PC,
fetch address/request/buffer state, IF/ID, ID/EX, EX/MEM and MEM/WB records,
valid bits, stopping/halted state and retirement order. The Core.step transaction
MUST atomically commit a single next Machine value. Pure helper modules MUST
not own hidden persistent state or commit independently.

| Stage | Responsibility and retained state |
| --- | --- |
| IF | Stable outstanding fetch and completed response buffer |
| ID | Instruction/PC validity, decode and operand selection |
| EX | Saved operands/control, ALU/address/branch calculation |
| MEM | Load request/response and completed value retention |
| WB | Oldest GPR/store/trap retirement and ordering |

External side effects MUST occur only at their specified handshake boundary;
the complete-state output is an observation witness, not a second state owner.

## Blocking memory and byte lanes {#RV32-SPEC-BUS-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=platform status=draft refines=RV32-REQ-EEI-001,RV32-REQ-EFFECT-001 -->

The word interfaces MUST expose request-valid, byte address, completion-ready,
read data and fault; the data interface also exposes write, four byte enables
and write data. Address/data/mask/write MUST hold while valid and not ready.
Subword addresses MUST select the correct little-endian lanes and signed or
unsigned load extension. The backing word request uses the aligned word address.
A store takes effect only on valid & ready & !fault & !reset. No response may be
consumed twice. A successful load response MUST be retained if WB is occupied.
An older waiting WB store MUST have data-port priority over a younger MEM load.

## Dependencies, flush and commit priority {#RV32-SPEC-HAZARD-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=pipeline status=draft refines=RV32-REQ-PIPE-001,RV32-REQ-EFFECT-001 -->

Operands MUST select the youngest available older producer: a ready MEM
result before WB, with WB write-through into ID. Store data uses the same
forwarding rule. An adjacent unavailable load result MUST insert a load-use
bubble; blocked stages retain live state while older work advances when possible.
A redirect is effective only when its EX instruction advances. It MUST invalidate
younger ID/IF work and drain/discard any outstanding wrong-path fetch while
preserving the external request contract. A completed fetch MUST survive
backpressure. Oldest WB terminal trap has priority over a MEM fault/trap and
then an EX redirect/trap; younger GPR/store effects MUST never escape that order.

## Retirement and reset {#RV32-SPEC-COMMIT-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=architecture status=draft refines=RV32-REQ-EFFECT-001 -->

Stores MUST issue only at WB and retire on their successful completion.
GPR writes and retirement order advance once per retiring instruction, with a
single terminal trap event and no younger commit. The trace MUST contain order,
PC/instruction/next-PC, rd/write/raw value, effective address/mask/raw memory data
and trap/cause/tval. Trace data MUST observe DUT results rather than recreate ISA
arithmetic. Reset MUST restore initial GPR/PC/control state, empty every stage,
cancel outstanding platform requests and suppress same-edge external effects;
already committed RAM writes are not rolled back.

## Functional and timed execution {#RV32-SPEC-VIEWS-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=pipeline status=draft refines=RV32-REQ-VERIFY-001 -->

Timing execution MUST evaluate every tick. Functional execution may compress
only a known complete-state fixed point with no event/handshake, unchanged inputs
and no reset/failure before the next explicit environment boundary. Visible
timers and time-dependent ordering MUST remain observable. Both backends MUST
consume the same verified final IR for each selected policy. Virtual cycles,
executed ticks and retirement count MUST be reported separately. Unknown reached
control MUST fail before improper state/external effects; inactive payload
retains the language's ordinary availability/four-state contract.

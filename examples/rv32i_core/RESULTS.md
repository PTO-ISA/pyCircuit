# RV32I evidence summary

The machine-readable `results-baseline.json` preserves the complete bounded
pre-split baseline from local candidate64730b9. It includes all40 scenario rows,
27 program identities,160 view outcomes, reference split, instruction coverage,
C signature, physical boundary and fixed library identity. This historical
candidate is not claimed as the test head of the publication branch.

| Historical measurement | Result |
| --- | --- |
| Architecture views |160 PASS:156 Spike agreement and4 separate FENCE.I admission checks |
| Trace comparisons |40 full timing native/RTL pairs and40 functional compressed pairs |
| Complete state |40 four-view groups; all32GPR/PC/RAM, exact stores and signatures checked |
| Instruction coverage |All40 RV32I families, including ECALL/EBREAK trap execution |
| C application |434 normal retirements plus expected trap,567 cycles; signature10208/424583933/309/329 |
| Long waits |6481 virtual cycles,2782 executed functional ticks,3699 idle ticks folded |
| Hazards/reset |Forwarding, one smoke load-use bubble, store dependencies, redirects/wrong-path cancellation, waiting-store reset |
| Open-cell mapping |13596 cells:2076 sequential,11520 combinational;107784.624 µm² |
| Area boundary |GPR file, pipeline/control and observation cones included; external imem/dmem and test adapter excluded |
| Mapped program |Actual Icarus SKY130 models,567 cycles/434 normal retirements; complete final state checked |

The fixed Spike build accepts FENCE.I with its RV32I setting. Those four views
use separately authored RV32I2.1/closed-EEI illegal-encoding expectations; they
are not counted as Spike agreement. Source/ELF execution coverage is distinct
from exhaustive ISA correctness or official certification.

The mapping flow uses standard Yosys/ABC and scalar-port serialization. Exact
cell/type/parameter and logical-port/pin connectivity is checked before/after
normalization. This avoids Icarus wide strength/concat propagation overhead;
no architecture cells are deleted to obtain a pass. Artificial functional cell
unit delays and the generous simulation clock are not PVT timing signoff.

Public reproduction starts with the pinned SDK bootstrap in README.md, then
`verification/prepare.py`, `suite.py`, `check_results.py`, `synthesize.py` and
`mapped_check.py`. Results are emitted in the selected output directory;
source commits contain summaries and reproducible commands, not private logs,
ELFs, SDKs, netlists or dependency bundles. Original raw logs remain local ignored
artifacts. No public raw-artifact URL is claimed for the historical run.

## Publication validation

Tested demo implementation:2f5e8c31602b4fa7517d11bda19d28311e728298.
Pinned tool SDK:2b8b8316ac533ef4773acc1e93a280301645c8ab, public upstream tool
PR370. The following result/documentation commit does not change DUT calculation.
`results-publication.json` records actual stage rc, artifact identities and scope.

The public `verification/setup_sdk.py` fetched that exact upstream PR commit and
performed a standard clean CompilerDev+Runtime SDK build/install. No private
SDK bundle was copied. Fresh `prepare.py` compilation/oracle generation for the
family and C inputs returned rc0. The C program then passed all four public
functional/timing × native/Icarus entries from a clean copied project, each rc0,
434 normal retirements plus one expected trap and567 cycles. Independent Spike,
all32GPR/PC/RAM, exact stores and C-source math were checked. Native/RTL cycle
and complete final-state comparisons passed.

Eight DUT files, all four finalIRs, physical RTLs and the converted synthesis
input are byte-identical to the historical baseline. The recorded25 identity
comparisons justify reusing the unchanged160-view/40-family baseline; the full
matrix was not mechanically rerun or called a fresh publication-head matrix.

Real SKY130 mapping and the mapped C simulation were newly executed, both rc0.
The result remains13596 cells/107784.624 µm²,2076 sequential/11520 combinational.
Before/after normalization cell/type/parameter and logical pin/port connectivity
is identical. Actual mapped Icarus completed567 cycles and434 normal retirements
with full packed-output and final-state checks. The PASS text's retirement count
comes from the independently checked source report; it is supported by the
cycle equality assertions, not a separate hardware performance counter.

Changed-file hooks and strict documentation checks are required on the final
publication head. GitHub's core jobs validate repository/Python metadata and
capture/publication; they do not claim to run the external ISS/cell-library flow.
Raw local logs remain ignored and are not presented as public download links.
The committed summaries, exact dependency/source/library identities, scenario
list, all40 instruction names and public reproduction scripts are the reviewable
record. Historical certification/STA/P&R/independent-review limitations remain.

Official architecture tests/certification, full formal equivalence, independent
review, OpenSTA and P&R remain NOT_RUN. Area is mapped standard-cell area, not
layout/die area. SOTA remains NOT_ESTABLISHED.

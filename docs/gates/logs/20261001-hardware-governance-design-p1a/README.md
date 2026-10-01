# Hardware governance and naming correction P1a

Baseline: 114b1ae1c79191817f82931825706e617234d9c0.
Authorized by the user: hardware-language naming cleanup; NO HARDCODE,
NO SHIM; actual architect and independent task decomposition before execution.
Architect: fresh architect preset, Astra/xhigh. Decomposition: planner preset,
Astra/medium. Implementation: ir_design_rename, 6.1 Sol/high. Independent review:
governance_design_review, separate 6.1 Sol/high instance.

Only the FinalHardwareProgram view family is renamed to FinalHardwareDesign;
FinalProgram and IR expression vocabulary are subsequent packets. Three
governance files require hardware semantics, no fixture-driven admission, no
compatibility/semantic shims, bounded independent decomposition and tests.

Fresh current-checkout build of ACIRFinalProgramTests and CTest passed.
Reviewer independently ran 69 tests from 10 suites, all passed. Strict docs
passed. The rename manifest proves exact before/after replacements.

Unaccepted dirty normalized-input acceptance, native test additions, pipeline
example, other names and full language readiness are explicitly excluded.
These files remain visible for repair; this packet does not hide or approve
them. Historical approved proposals and gate evidence retain their bytes.

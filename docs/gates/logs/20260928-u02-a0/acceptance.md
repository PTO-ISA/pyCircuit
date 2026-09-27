# U02-A0 extraction acceptance

Accepted isolated candidate: `b797eaf77f2bf5d1651e608bc3855d14fac92047`.
Implementation: u02_frontend, gpt-6-luna/high. Independent testing:
baseline_verification, gpt-5.6-sol/medium. Independent review:
governance_review, gpt-5.6-sol/high, PASS.

The PM verified all 14 reviewed source/oracle file hashes against the committed
candidate. The review manifest SHA-256 is
`1765df29971a243f75749e28b53ed93e16d303cdfe477f11a97e9cccb03d06f0`.

Fresh LLVM 22.1.8 build: 36 foundation + 14 header + 51 system tests passed,
zero skips; CTest 2/2. All six raw Packet/consumer transport/body/interface
artifacts match the accepted U01-E artifacts byte for byte. Archive previews
have only surplus trailing LF bytes removed where recorded in
[archive-binding.json](archive-binding.json); original hashes remain recoverable.

Eight compiler files extract shared context and signature parsing without
changing the entry point or schema. PythonImportRecords.cpp decreases from
621 to 498 lines, closing its temporary size exception. N1 implementation is a
subsequent dirty candidate and is not covered by this acceptance.

This proves behavior preservation for the extraction only. Source module/state,
link/final IR, C++/Verilog execution, public driver and SDK remain open.

# Release preflight independent architecture review

**Verdict:** APPROVE. **Unresolved findings:** 0. **Release readiness:** clear
for the v6.0.0 workflow.

The review confirmed that removing `Pure` from `ac.instance`, `ac.array`, and
`ac.instances` makes canonicalization and CSE conservatively retain owned
runtime objects. The focused lit covers generic `canonicalize,cse` and the real
`ac-lower-rules` pipeline.

Zero-delay analysis separately recognizes structural instances and recursively
classifies their callees. Pure self and multi-node cycles remain illegal, while
stateful callees remain valid cut points. The 26 ACIR model-analysis tests and
existing zero-delay lit pass.

The six workspace scenarios prove that zero-result external trace owners now
survive into Frozen ACIR, accept validated PTO traces, and reproduce their
checked-in results. The archived 192/192 lit and 20/20 native ctest runs have no
failures.

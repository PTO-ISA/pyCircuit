# C2 revision A independent review

Reviewer /root/architecture_review, gpt-6-astra/xhigh, independent from interface_design and PM authors. Verdict: revise. Exact SHA-256: 97a04a1d794d763f3d98225ac84e0cd339de23c8df35b0c701d578a292f732e5.

Four blocking groups: close StateID/OwnerRef/PortBinding and metadata/control/helper schemas; make StaticExpr cover admitted record/list/helper/static-name semantics; retain concrete specialization keys in final IR and define shared resolver; bind numerical witnesses to actual result and observable next/return with registration/specialization-scoped ValueIDs. Add minimal generic/interface/final Bank fragments and tamper negatives.

Direction, C1/C3 separation, header-only parent, MLIR math lowering, no bigint backend, and staged full-hardware obligations are sound. No user approval or product test claimed. Architect author is repairing the exact missing contracts before rereview.

# N1 scalar constant source/header acceptance

Accepted isolated candidate: `2cac172a` on top of `5923ce4e` and `856ff9cc`.
These commits add the approved `ac.constant` declaration, bool/integer literal
producer, canonical namespace export/import snapshots, and focused fidelity
tests. [Independent review A](review-a.md) found one test gap; [review B](review-b.md)
verified its repair with no remaining bounded-slice issue.

The independently rebuilt exact source passed 15/15 native tests, CTest 1/1,
and five system cases without skips. This accepts only scalar literal constant
source/header behavior. Other C1/C2 constant forms, module facade coverage,
real header/body link, removal before final IR, and dual C++/Verilog emit remain
open. The candidate is not published as the product route.

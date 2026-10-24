"""Sibling references share candidates only when they identify the same owner."""

import pycircuit as ac
from static_routes.counter import Counter


@ac.module
def Worker():
    peer = ac.instance_ref(Counter, "..", "peer")
    same = peer
    distinct = ac.instance_ref(Counter, "..", "distinct")

    @ac.rule
    def step() -> None:
        saved = peer.read()
        untouched = distinct.read()
        same.write(saved + 3)
        forwarded = peer.read()
        still_distinct = distinct.read()
        distinct.write(untouched + 5)
        other_forwarded = distinct.read()
        payload = same.read_payload()
        assert forwarded == saved + 3, "reference alias candidate"
        assert still_distinct == untouched, "distinct owner isolation"
        assert ac.case_eq(payload, ac.four_state("10xz0011")), "routed four-state payload"
        ac.log("info", "routing", saved, forwarded, untouched,
               still_distinct, other_forwarded, payload)

    step()

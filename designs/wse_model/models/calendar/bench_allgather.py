# ruff: noqa: N802, F823, F841 -- captured System state and static child instances.
"""Two complete FFN phases, serialized one source-row segment per test cycle.

All 32 member ReceiveEngine instances persist across the two phases. Eight
stateless ForwardHop instances serve the current group as a functional test
carrier; this is not a concurrent hardware network or a timing model.
Run 516 cycles: post, 256 row segments, retire, then repeat for the column phase.
"""

import pycircuit as ac
from wse_calendar.allgather import AllGatherMember, hop_rows, line_input, transfer_for
from wse_calendar.forward import ForwardHop


@ac.system
def AllGatherSystem():
    cycle: ac.u10 = 0
    transfer = transfer_for(cycle)
    hop0 = ForwardHop(line_input(transfer, 0))
    hop1 = ForwardHop(line_input(transfer, 1))
    hop2 = ForwardHop(line_input(transfer, 2))
    hop3 = ForwardHop(line_input(transfer, 3))
    hop4 = ForwardHop(line_input(transfer, 4))
    hop5 = ForwardHop(line_input(transfer, 5))
    hop6 = ForwardHop(line_input(transfer, 6))
    hop7 = ForwardHop(line_input(transfer, 7))
    hops = hop_rows(hop0, hop1, hop2, hop3, hop4, hop5, hop6, hop7)
    member0 = AllGatherMember(transfer, hops, 0)
    member1 = AllGatherMember(transfer, hops, 1)
    member2 = AllGatherMember(transfer, hops, 2)
    member3 = AllGatherMember(transfer, hops, 3)
    member4 = AllGatherMember(transfer, hops, 4)
    member5 = AllGatherMember(transfer, hops, 5)
    member6 = AllGatherMember(transfer, hops, 6)
    member7 = AllGatherMember(transfer, hops, 7)
    member8 = AllGatherMember(transfer, hops, 8)
    member9 = AllGatherMember(transfer, hops, 9)
    member10 = AllGatherMember(transfer, hops, 10)
    member11 = AllGatherMember(transfer, hops, 11)
    member12 = AllGatherMember(transfer, hops, 12)
    member13 = AllGatherMember(transfer, hops, 13)
    member14 = AllGatherMember(transfer, hops, 14)
    member15 = AllGatherMember(transfer, hops, 15)
    member16 = AllGatherMember(transfer, hops, 16)
    member17 = AllGatherMember(transfer, hops, 17)
    member18 = AllGatherMember(transfer, hops, 18)
    member19 = AllGatherMember(transfer, hops, 19)
    member20 = AllGatherMember(transfer, hops, 20)
    member21 = AllGatherMember(transfer, hops, 21)
    member22 = AllGatherMember(transfer, hops, 22)
    member23 = AllGatherMember(transfer, hops, 23)
    member24 = AllGatherMember(transfer, hops, 24)
    member25 = AllGatherMember(transfer, hops, 25)
    member26 = AllGatherMember(transfer, hops, 26)
    member27 = AllGatherMember(transfer, hops, 27)
    member28 = AllGatherMember(transfer, hops, 28)
    member29 = AllGatherMember(transfer, hops, 29)
    member30 = AllGatherMember(transfer, hops, 30)
    member31 = AllGatherMember(transfer, hops, 31)

    @ac.rule
    def advance() -> None:
        cycle = cycle + 1

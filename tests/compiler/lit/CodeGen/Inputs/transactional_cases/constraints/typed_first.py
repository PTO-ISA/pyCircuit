# ruff: noqa: F821, F823, F841, F842, N802 -- Compiler-owned instances, state and type constraints.
import pycircuit as ac


@ac.module
def TypedFirst(peer: ac.module):
    @ac.rule
    def step() -> None:
        value = peer.peek()
        peer.peek()
        alias1 = value
        alias2 = alias1
        alias3 = alias2
        alias4 = alias3
        alias5 = alias4
        snapshot: ac.u8 = alias5
        value = 7
        replacement: ac.u16 = value
        pair_first, pair_second = (peer.peek(), peer.peek())
        peer.advance()
        ac.log("info", "sample", snapshot, replacement, pair_first, pair_second)

    step()

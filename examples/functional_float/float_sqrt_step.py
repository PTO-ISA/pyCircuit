"""One reusable restoring-integer-square-root digit step."""
import pycircuit as ac


@ac.struct
class SqrtState:
    root: ac.u64
    remainder: ac.bits[128]
    radicand: ac.bits[128]


def digit_step(state: SqrtState) -> SqrtState:
    high: ac.bits[128] = state.radicand[126:128]
    remainder: ac.bits[128] = (state.remainder << 2) | high
    root_wide: ac.bits[128] = state.root
    trial: ac.bits[128] = (root_wide << 2) | 1
    accept: ac.u1 = remainder >= trial
    difference: ac.bits[128] = remainder - trial
    root: ac.u64 = ((state.root << 1) | 1) if accept else state.root << 1
    return SqrtState(root=root, remainder=difference if accept else remainder,
                     radicand=state.radicand << 2)


@ac.module
def SqrtStep(state: SqrtState) -> SqrtState:  # noqa: N802
    return digit_step(state)

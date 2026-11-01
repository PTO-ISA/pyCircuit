# ruff: noqa: F823, F841, N802, N803 -- Captured state and static parameters.
"""Static loops preserve sequential locals, old-Q reads and instance identity."""

import pycircuit as ac


def fold(value: ac.u16) -> ac.u16:
    result = value
    count = 3
    for outer in range(1, count + 1):
        stop = outer + 1
        for inner in range(stop):
            result += ac.u16(inner)
    for offset in range(1, 6, 2):
        result += ac.u16(offset)
    for _unused in range(3, 3):
        result += ac.u16(200)
    retained = ac.u16(7)
    for retained in range(0):
        retained = ac.u16(200)
    for fresh in range(3):
        fresh = ac.u16(9)
        result += fresh
    result += retained + fresh
    widened = ac.u16(14)
    for index in range(3):
        widened = widened[0:4] + ac.u4(1)
        escaped = ac.u16(index) + ac.u16(5)
    result += widened + escaped
    for selected in range(3):
        if selected == 0:
            only_branch = ac.u16(6)
    result += only_branch
    typed = ac.u8(0)
    for typed in range(255, 256):
        alias = typed + 1
        if alias == 0:
            result += ac.u16(2)
    return result


@ac.module
def Kernel(*, COUNT: int = 2):
    phase: ac.u8 = 0
    saved: ac.u8 = 5
    mask: ac.u8 = 0
    lanes: ac.u8 = ac.four_state("10xz01zx")
    accumulated: ac.u16 = 0

    total = ac.u16(0)
    running = ac.u16(3)
    for index in range(COUNT):
        if phase < 2:
            total += ac.u16(index) + ac.u16(phase)
        else:
            total += ac.u16(index) * ac.u16(2)
        running += total

    @ac.method
    def update() -> None:
        updated = saved
        for bit in range(COUNT):
            mask[bit] = ac.u1(1)
            lanes[bit] = ac.u1(1)
            updated = saved + 1
        saved = updated

    @ac.autorule
    def tick() -> None:
        local = ac.u16(0)
        for iteration in range(COUNT):
            local += ac.u16(saved) + ac.u16(iteration)
        update()
        accumulated = local
        ac.log(
            "info",
            "kernel",
            ac.u8(COUNT),
            phase,
            total,
            running,
            fold(total),
            saved,
            mask,
            accumulated,
            lanes,
        )
        phase += 1


@ac.module
def LoopState():
    for _iteration in range(2):
        last: ac.u8 = 0

    @ac.autorule
    def write_and_observe() -> None:
        last = ac.u8(5)
        ac.log("info", "loop-state", last)


@ac.module
def Stage(incoming: ac.u8, *, INDEX: int = 0) -> ac.u8:
    accepted: ac.u8 = 0

    @ac.rule
    def forward(value: ac.u8) -> ac.u8:
        ac.log("info", "stage", ac.u8(INDEX), accepted, value)
        accepted += 1
        return value + ac.u8(INDEX + 1)

    outgoing = forward(incoming)
    return outgoing


@ac.module
def Pipeline(incoming: ac.u8, *, STAGES: int = 3) -> ac.u8:
    stream = incoming
    for index in range(STAGES):
        stream = Stage(stream, INDEX=index)
    return stream

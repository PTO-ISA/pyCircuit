# ruff: noqa: F823 -- captured module-owned state proposal.
import pycircuit as ac


@ac.system
def ExerciseRawBoundQueue():
    phase: ac.u8 = 0
    take = phase >= 3
    tokens = ac.queue[ac.u8](depth=1, latency=1, out_ready=take)

    @ac.rule
    def produce() -> None:
        if phase < 6:
            tokens.push(phase + 10)
            ac.log("info", "queue raw-bound pushed", phase, phase + 10)

    @ac.rule
    def observe() -> None:
        if tokens.valid and take:
            ac.log("info", "queue raw-bound retired", phase, tokens.data)

    @ac.rule
    def tick() -> None:
        phase = phase + 1  # noqa: F841 - captured hardware state or child instance.

    produce()
    observe()
    tick()

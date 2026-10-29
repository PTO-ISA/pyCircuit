# ruff: noqa: N803 -- static formal convention.
import pycircuit as ac
from checks.static_parameter_provider import Counter, Required


@ac.system(
    samples=(
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
        (1, 0),
        (1, 0),
        (0, 0),
        (1, 1),
        (0, 0),
        (1, 0),
        (0, 0),
        (1, 0),
    )
)
def ExerciseStaticParameters(*, WIDTH: int = 3, CAPACITY: int = 2, STEP: int = 3):
    narrow = Counter(W=WIDTH, BIAS=1)
    default = Counter()
    forwarded = Required(WIDTH=4)
    wide_counter = Counter(W=80)
    pending = ac.queue[ac.bits[WIDTH]](depth=CAPACITY)

    @ac.rule
    def produce() -> None:
        old: ac.bits[WIDTH] = narrow.read()
        pending.push(old)

    @ac.rule
    def consume() -> None:
        value: ac.bits[WIDTH] = pending.pop()
        ac.log("info", "static queue", value)

    @ac.rule
    def advance() -> None:
        small: ac.bits[WIDTH] = narrow.advance(STEP)
        wide: ac.u5 = default.advance(5)
        nested: ac.u4 = forwarded.advance(2)
        small_old: ac.bits[WIDTH] = narrow.read()
        wide_old: ac.u5 = default.read()
        nested_old: ac.u4 = forwarded.read()
        wide_next: ac.bits[80] = wide_counter.advance(604462909807314587353089)
        wide_previous: ac.bits[80] = wide_counter.read()
        ac.log(
            "info",
            "static counters",
            small_old,
            small,
            wide_old,
            wide,
            nested_old,
            nested,
            wide_previous[:64],
            wide_previous[64:80],
            wide_next[:64],
            wide_next[64:80],
        )

    advance()
    produce()
    consume()

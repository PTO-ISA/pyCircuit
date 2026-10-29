# ruff: noqa: F823 -- Module state captured by Methods and Rules is compiler-owned.
import pycircuit as ac


@ac.system
def LocalMethodAccumulator():  # noqa: N802
    total: ac.bits[8] = 0
    accepted: ac.bits[8] = 0
    permit: ac.bits[1] = 0

    @ac.method
    def has_space() -> ac.bits[1]:
        return total < 9

    @ac.method(guard=lambda value: value < 5)
    def increment(value: ac.bits[8]) -> ac.bits[8]:
        value = value + 1
        return value

    @ac.method
    def count_sample() -> None:
        accepted = accepted + 1

    @ac.method
    def add(amount: ac.bits[8]) -> ac.bits[8]:
        next_total = total + amount
        total = next_total
        count_sample()
        return next_total

    @ac.method(guard=lambda allowed: allowed == 1)
    def require_permit(allowed: ac.bits[1]) -> None:
        pass

    @ac.method
    def finish(allowed: ac.bits[1]) -> None:
        require_permit(allowed)
        ac.log("info", "accepted", total)

    @ac.rule(guard=lambda: has_space())
    def receive() -> None:
        amount: ac.bits[8] = 2
        step = increment(amount)
        other = increment(4)
        next_total = add(step)
        ac.log("info", "add", amount, step, other, total, next_total)
        finish(permit)

    receive()

    @ac.autorule
    def toggle_permit() -> None:
        permit = permit ^ 1

    @ac.autorule
    def observe() -> None:
        ac.log("info", "state", total, accepted, permit)

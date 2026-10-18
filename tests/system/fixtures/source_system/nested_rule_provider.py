# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac


@ac.module
def RuleCell():
    count: ac.u8 = 5

    @ac.rule
    def add(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = count
        count = count + delta
        return before, count

    @ac.rule(guard=lambda allow: allow)
    def require(allow: ac.u1) -> None:
        return

    @ac.rule
    def read() -> ac.u8:
        return count


@ac.module
def RuleRelay():
    cell = RuleCell()

    @ac.rule
    def add(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before, after = cell.add(delta)
        return before, after

    @ac.rule
    def require(allow: ac.u1) -> None:
        cell.require(allow)

    @ac.rule
    def read() -> ac.u8:
        return cell.read()

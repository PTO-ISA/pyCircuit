# ruff: noqa: B007, F823, F841 -- capture-only loop binders and state proposals.
import pycircuit as ac


def add(value: ac.u8, amount: ac.u8) -> ac.u8:
    return value + amount


def nested_sum(value: ac.u8) -> ac.u8:
    for outer in range(2):
        for middle in range(2):
            for inner in range(2):
                amount: ac.u8 = outer + middle + inner + 1
                value = add(value, amount)
    return value


def bindings(value: ac.u8) -> ac.u8:
    outer: ac.u8 = 7
    inner: ac.u8 = 9
    for outer in range(0):
        for inner in range(2):
            value = value + 100
    value = value + outer + inner
    for outer in range(2):
        for inner in range(0):
            value = value + 100
    value = value + outer + inner
    for outer in range(2):
        for inner in range(2):
            fresh = outer * 2 + inner
    value = value + fresh + outer + inner
    index: ac.u8 = 0
    for index in range(2):
        for index in range(2):
            index = index + 4
        value = value + index
        index = index + 10
    return value + index


@ac.module
def NestedValues(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
    result = value
    for outer in range(2):
        for inner in range(3):
            result = add(result, outer * 3 + inner + 1)
    return nested_sum(value), result, bindings(value)


@ac.module
def NestedOwner():
    total: ac.u8 = 5

    @ac.method
    def advance(phase: ac.u2) -> ac.u8:
        for outer in range(3):
            for inner in range(3):
                if inner == phase:
                    break
                if inner == 1:
                    continue
                amount: ac.u8 = outer * 3 + inner + 1
                total = add(total, amount)
            else:
                for tail in range(2):
                    total = total + 1
            total = total + 10
        else:
            total = total + 20
        return total

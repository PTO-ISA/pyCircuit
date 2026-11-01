# ruff: noqa: F821, F823, F841, N802, N803 -- Captured instances and static parameters.
"""Static Table traversal calls the same independently compiled instances."""

import pycircuit as ac
from table_case.provider import Worker


@ac.module
def Bank(*, CAPACITY: int = 4):
    workers: ac.table[CAPACITY, Worker]
    first = workers[0]

    @ac.autorule
    def advance() -> None:
        total = ac.u8(0)
        for worker in workers:
            total += worker.read()
        for index, worker in enumerate(workers):
            worker.add(ac.u8(index))
            ac.log("info", "worker", ac.u8(len(workers)), ac.u8(index), worker.read())
        ac.log("info", "bank", ac.u8(len(workers)), total, first.read())


@ac.module
def NarrowBank():
    workers: ac.table[4, Worker]
    index: ac.u1 = 0

    @ac.autorule
    def advance() -> None:
        workers[index].add(ac.u8(1))
        ac.log(
            "info",
            "narrow",
            workers[0].read(),
            workers[1].read(),
            workers[2].read(),
            workers[3].read(),
        )
        index += 1


@ac.system
def Top():
    Bank()
    Bank(CAPACITY=2)
    NarrowBank()

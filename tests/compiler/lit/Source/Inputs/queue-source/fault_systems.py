"""Closed test systems for queue fault checking and whole-system discard."""
# ruff: noqa: F841, B011 -- hardware capture preserves unused effects and asserts.

from pycircuit import module, queue, rule, struct, system, u8
from q4_queue.pyc_credit_pipeline import CreditPipeline, CreditToken
from q4_queue.pyc_reorder_pipeline import ReorderPipeline, Token
from q4_queue.pyc_route_merge_pipeline import RouteMergePipeline


@rule
def AdvanceEpoch(epoch):
    epoch = epoch + 1


@struct
class SiblingResult:
    value: u8


@module
def Sibling() -> SiblingResult:
    value: u8 = 0
    ready, available, head = queue[u8](
        True, value, True, depth=4, latency=3, ready_policy="local_occupancy"
    )
    AdvanceEpoch(value)
    return SiblingResult(value=value)


@system
def RouteFault():
    epoch: u8 = 0
    pipeline = RouteMergePipeline(valid=epoch == 0, data=2, take=1)
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def RouteBlockedFault():
    epoch: u8 = 0
    pipeline = RouteMergePipeline(valid=epoch < 7, data=0 if epoch < 6 else 2, take=0)
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def CreditFault():
    epoch: u8 = 0
    pipeline = CreditPipeline(
        valid=epoch == 0,
        data=CreditToken(sequence=1, cycles=0, value=71),
        take=1,
    )
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def CreditDeferredFault():
    epoch: u8 = 0
    pipeline = CreditPipeline(
        valid=epoch < 3,
        data=CreditToken(sequence=1, cycles=15 if epoch < 2 else 0, value=71),
        take=1,
    )
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def ReorderStaleFault():
    epoch: u8 = 0
    pipeline = ReorderPipeline(
        valid=epoch == 0 or epoch == 4,
        data=Token(sequence=0, value=71),
        take=1,
    )
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def ReorderDuplicateFault():
    epoch: u8 = 0
    pipeline = ReorderPipeline(
        valid=epoch < 2, data=Token(sequence=2, value=71), take=1
    )
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def ReorderFullDefersFault():
    epoch: u8 = 0
    pipeline = ReorderPipeline(
        valid=epoch < 17,
        data=Token(sequence=epoch + 16 if epoch < 16 else 16, value=71),
        take=1,
    )
    sibling = Sibling()
    AdvanceEpoch(epoch)


@system
def AtomicSiblingFault():
    epoch: u8 = 0
    pipeline = RouteMergePipeline(valid=0, data=0, take=0)
    sibling = Sibling()

    @rule
    def reject():
        assert False, "queue_atomic_rejection"

    AdvanceEpoch(epoch)
    reject()

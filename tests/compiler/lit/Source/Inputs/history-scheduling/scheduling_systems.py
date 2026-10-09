"""Finite hardware stimulus; independent scheduler expectations stay in the host oracle."""

from pycircuit import log, rule, struct, system, u1, u8
from history_scheduling.pyc_dependency_pipeline import DependencyPipeline, Token
from history_scheduling.persistent_schedule import PersistentSchedule, ScheduleToken


@struct
class DependencyFrame:
    valid: u1
    take: u1
    data: Token


@rule
def stimulus_dependency(epoch: u8) -> DependencyFrame:
    valid: u1 = 0
    take: u1 = 1
    data: Token = Token(sequence=0, waits_for=0, resource=0, cycles=0, value=0)
    if epoch == 0:
        valid = 1
        take = 1
        data = Token(sequence=0, waits_for=15, resource=0, cycles=4, value=10)
    if epoch == 1:
        valid = 1
        take = 1
        data = Token(sequence=1, waits_for=15, resource=0, cycles=1, value=11)
    if epoch == 2:
        valid = 1
        take = 1
        data = Token(sequence=2, waits_for=15, resource=1, cycles=1, value=12)
    if epoch == 3:
        valid = 1
        take = 1
        data = Token(sequence=3, waits_for=0, resource=1, cycles=1, value=13)
    return DependencyFrame(valid=valid, take=take, data=data)


@rule
def observe_dependency(epoch, result):
    log("info", "epoch", epoch)
    log("info", "ready", result.ready)
    log("info", "available", result.available)
    log("info", "sequence", result.head.sequence)
    log("info", "waits_for", result.head.waits_for)
    log("info", "resource", result.head.resource)
    log("info", "cost", result.head.cycles)
    log("info", "value", result.head.value)
    epoch = epoch + 1


@system
def pyc_dependency_pipeline():
    epoch: u8 = 0
    frame = stimulus_dependency(epoch)
    result = DependencyPipeline(valid=frame.valid, data=frame.data, take=frame.take)
    observe_dependency(epoch, result)


@struct
class ScheduleFrame:
    valid: u1
    take: u1
    data: ScheduleToken


@rule
def stimulus_schedule(epoch: u8) -> ScheduleFrame:
    valid: u1 = 0
    take: u1 = 1
    data: ScheduleToken = ScheduleToken(
        sequence=0, waits_for=0, resource=0, cost=0, value=0
    )
    if epoch == 0:
        valid = 1
        take = 1
        data = ScheduleToken(sequence=0, waits_for=255, resource=0, cost=1, value=10)
    if epoch == 1:
        valid = 1
        take = 1
        data = ScheduleToken(sequence=1, waits_for=255, resource=1, cost=8, value=11)
    if epoch == 2:
        valid = 1
        take = 1
        data = ScheduleToken(sequence=2, waits_for=0, resource=1, cost=1, value=12)
    return ScheduleFrame(valid=valid, take=take, data=data)


@rule
def observe_schedule(epoch, result):
    log("info", "epoch", epoch)
    log("info", "ready", result.ready)
    log("info", "available", result.available)
    log("info", "sequence", result.head.sequence)
    log("info", "waits_for", result.head.waits_for)
    log("info", "resource", result.head.resource)
    log("info", "cost", result.head.cost)
    log("info", "value", result.head.value)
    epoch = epoch + 1


@system
def schedule_v2():
    epoch: u8 = 0
    frame = stimulus_schedule(epoch)
    result = PersistentSchedule(valid=frame.valid, data=frame.data, take=frame.take)
    observe_schedule(epoch, result)

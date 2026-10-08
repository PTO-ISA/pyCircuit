"""Finite expectation queue and source systems derived from the historical root."""

from pycircuit import log, module, queue, rule, struct, system, u1, u8, u16


@struct
class ExpectedToken:
    value: u16


@struct
class ExpectedResult:
    ready: u1
    available: u1
    head: ExpectedToken


@rule
def expect_positive(available: u1, head: ExpectedToken):
    if available:
        assert head.value > 0, "value must be positive"


@module
def ExpectPipeline(
    valid: u1, data: ExpectedToken, take: u1
) -> ExpectedResult:  # noqa: N802
    ready, available, head = queue[ExpectedToken](
        valid, data, take, depth=2, latency=1, ready_policy="local_occupancy"
    )
    expect_positive(available, head)
    return ExpectedResult(ready=ready, available=available, head=head)


@rule
def observe(epoch, result):
    log("info", "epoch", epoch)
    log("info", "ready", result.ready)
    log("info", "available", result.available)
    log("info", "value", result.head.value)
    epoch = epoch + 1


@system
def gfsim_expect_pipeline():
    epoch: u8 = 0
    result = ExpectPipeline(
        valid=epoch < 64,
        data=ExpectedToken(value=1 if (epoch & 1) == 0 else 65535),
        take=(epoch == 0) or (epoch >= 8),
    )
    observe(epoch, result)


@system
def gfsim_expect_invalid_idle():
    epoch: u8 = 0
    result = ExpectPipeline(valid=0, data=ExpectedToken(value=0), take=1)
    observe(epoch, result)


@system
def gfsim_expect_failure():
    epoch: u8 = 0
    result = ExpectPipeline(valid=epoch == 0, data=ExpectedToken(value=0), take=0)
    observe(epoch, result)

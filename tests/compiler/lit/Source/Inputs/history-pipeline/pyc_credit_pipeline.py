"""Regular-clock credit systems; the original nominal DUT comes from queue-source."""

from pycircuit import log, rule, struct, system, u1, u4, u8, u16
from q4_queue.pyc_credit_pipeline import CreditPipeline, CreditResult, CreditToken


@struct
class CreditFrame:
    valid: u1
    take: u1
    sequence: u4
    cycles: u4
    value: u16


@rule
def observe_credit(epoch, result: CreditResult):
    log("info", "epoch", epoch)
    log("info", "ready", result.ready)
    log("info", "available", result.available)
    log("info", "sequence", result.head.sequence)
    log("info", "cycles", result.head.cycles)
    log("info", "value", result.head.value)
    epoch = epoch + 1


@rule
def stimulus_c1(epoch: u8) -> CreditFrame:
    valid: u1 = 0
    take: u1 = 1
    sequence: u4 = 0
    cycles: u4 = 0
    value: u16 = 0
    if epoch == 0:
        valid = 1
        take = 1
        sequence = 0
        cycles = 5
        value = 2561
    elif epoch == 1:
        valid = 1
        take = 1
        sequence = 1
        cycles = 1
        value = 2562
    elif epoch == 2:
        valid = 1
        take = 1
        sequence = 2
        cycles = 4
        value = 2563
    elif epoch == 3:
        valid = 1
        take = 1
        sequence = 3
        cycles = 1
        value = 2564
    elif epoch == 4:
        valid = 1
        take = 1
        sequence = 4
        cycles = 2
        value = 2565
    elif epoch == 5:
        valid = 1
        take = 1
        sequence = 5
        cycles = 1
        value = 2566
    elif epoch == 6:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 7:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 8:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 9:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 10:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 11:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 12:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 13:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 14:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 15:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 16:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 17:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 18:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 19:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    return CreditFrame(
        valid=valid, take=take, sequence=sequence, cycles=cycles, value=value
    )


@system
def pyc_credit_pipeline():
    epoch: u8 = 0
    frame = stimulus_c1(epoch)
    result = CreditPipeline(
        valid=frame.valid,
        data=CreditToken(
            sequence=frame.sequence, cycles=frame.cycles, value=frame.value
        ),
        take=frame.take,
    )
    observe_credit(epoch, result)


@rule
def stimulus_c4(epoch: u8) -> CreditFrame:
    valid: u1 = 0
    take: u1 = 1
    sequence: u4 = 0
    cycles: u4 = 0
    value: u16 = 0
    if epoch == 0:
        valid = 1
        take = 1
        sequence = 0
        cycles = 1
        value = 3584
    elif epoch == 1:
        valid = 1
        take = 1
        sequence = 1
        cycles = 1
        value = 3585
    elif epoch == 2:
        valid = 1
        take = 1
        sequence = 2
        cycles = 1
        value = 3586
    elif epoch == 3:
        valid = 1
        take = 1
        sequence = 3
        cycles = 1
        value = 3587
    elif epoch == 4:
        valid = 1
        take = 0
        sequence = 4
        cycles = 1
        value = 3588
    elif epoch == 5:
        valid = 1
        take = 0
        sequence = 5
        cycles = 1
        value = 3589
    elif epoch == 6:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 7:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 8:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 9:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 10:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 11:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 12:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 13:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 14:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 15:
        valid = 0
        take = 0
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 16:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 17:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 18:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 19:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 20:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 21:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 22:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 23:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 24:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 25:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    return CreditFrame(
        valid=valid, take=take, sequence=sequence, cycles=cycles, value=value
    )


@system
def credit_c4():
    epoch: u8 = 0
    frame = stimulus_c4(epoch)
    result = CreditPipeline(
        valid=frame.valid,
        data=CreditToken(
            sequence=frame.sequence, cycles=frame.cycles, value=frame.value
        ),
        take=frame.take,
    )
    observe_credit(epoch, result)


@rule
def stimulus_c5(epoch: u8) -> CreditFrame:
    valid: u1 = 0
    take: u1 = 1
    sequence: u4 = 0
    cycles: u4 = 0
    value: u16 = 0
    if epoch == 0:
        valid = 1
        take = 1
        sequence = 0
        cycles = 2
        value = 3329
    elif epoch == 1:
        valid = 1
        take = 1
        sequence = 15
        cycles = 1
        value = 65535
    elif epoch == 2:
        valid = 1
        take = 1
        sequence = 2
        cycles = 15
        value = 3331
    elif epoch == 3:
        valid = 1
        take = 1
        sequence = 3
        cycles = 1
        value = 3332
    elif epoch == 4:
        valid = 1
        take = 1
        sequence = 4
        cycles = 1
        value = 3333
    elif epoch == 5:
        valid = 1
        take = 1
        sequence = 5
        cycles = 1
        value = 3334
    elif epoch == 6:
        valid = 1
        take = 1
        sequence = 6
        cycles = 1
        value = 3335
    elif epoch == 7:
        valid = 1
        take = 1
        sequence = 7
        cycles = 1
        value = 3336
    elif epoch == 8:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 9:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 10:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 11:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 12:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 13:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 14:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 15:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 16:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 17:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 18:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 19:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 20:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 21:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 22:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 23:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 24:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 25:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 26:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 27:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 28:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 29:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 30:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 31:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    return CreditFrame(
        valid=valid, take=take, sequence=sequence, cycles=cycles, value=value
    )


@system
def credit_c5():
    epoch: u8 = 0
    frame = stimulus_c5(epoch)
    result = CreditPipeline(
        valid=frame.valid,
        data=CreditToken(
            sequence=frame.sequence, cycles=frame.cycles, value=frame.value
        ),
        take=frame.take,
    )
    observe_credit(epoch, result)


@rule
def stimulus_c7(epoch: u8) -> CreditFrame:
    valid: u1 = 0
    take: u1 = 1
    sequence: u4 = 0
    cycles: u4 = 0
    value: u16 = 0
    if epoch == 0:
        valid = 1
        take = 1
        sequence = 0
        cycles = 3
        value = 4096
    elif epoch == 1:
        valid = 1
        take = 1
        sequence = 1
        cycles = 3
        value = 4097
    elif epoch == 2:
        valid = 1
        take = 1
        sequence = 2
        cycles = 3
        value = 4098
    elif epoch == 3:
        valid = 1
        take = 1
        sequence = 3
        cycles = 3
        value = 4099
    elif epoch == 4:
        valid = 1
        take = 1
        sequence = 4
        cycles = 3
        value = 4100
    elif epoch == 5:
        valid = 1
        take = 1
        sequence = 5
        cycles = 3
        value = 4101
    elif epoch == 6:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 7:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 8:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 9:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 10:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 11:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 12:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 13:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 14:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 15:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 16:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 17:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 18:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 19:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    return CreditFrame(
        valid=valid, take=take, sequence=sequence, cycles=cycles, value=value
    )


@system
def credit_c7():
    epoch: u8 = 0
    frame = stimulus_c7(epoch)
    result = CreditPipeline(
        valid=frame.valid,
        data=CreditToken(
            sequence=frame.sequence, cycles=frame.cycles, value=frame.value
        ),
        take=frame.take,
    )
    observe_credit(epoch, result)


@rule
def stimulus_c8(epoch: u8) -> CreditFrame:
    valid: u1 = 0
    take: u1 = 1
    sequence: u4 = 0
    cycles: u4 = 0
    value: u16 = 0
    if epoch == 0:
        valid = 1
        take = 1
        sequence = 0
        cycles = 1
        value = 4352
    elif epoch == 1:
        valid = 1
        take = 1
        sequence = 1
        cycles = 1
        value = 4353
    elif epoch == 2:
        valid = 1
        take = 1
        sequence = 2
        cycles = 1
        value = 4354
    elif epoch == 3:
        valid = 1
        take = 1
        sequence = 3
        cycles = 1
        value = 4355
    elif epoch == 4:
        valid = 1
        take = 0
        sequence = 4
        cycles = 1
        value = 4356
    elif epoch == 5:
        valid = 1
        take = 0
        sequence = 5
        cycles = 1
        value = 4357
    elif epoch == 6:
        valid = 1
        take = 0
        sequence = 6
        cycles = 1
        value = 4358
    elif epoch == 7:
        valid = 1
        take = 0
        sequence = 7
        cycles = 1
        value = 4359
    elif epoch == 8:
        valid = 1
        take = 0
        sequence = 8
        cycles = 1
        value = 4360
    elif epoch == 9:
        valid = 1
        take = 0
        sequence = 9
        cycles = 1
        value = 4361
    elif epoch == 10:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 11:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 12:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 13:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 14:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 15:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 16:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 17:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 18:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 19:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 20:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 21:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 22:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 23:
        valid = 1
        take = 0
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 24:
        valid = 1
        take = 1
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 25:
        valid = 1
        take = 1
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 26:
        valid = 1
        take = 1
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 27:
        valid = 1
        take = 1
        sequence = 10
        cycles = 1
        value = 4362
    elif epoch == 28:
        valid = 1
        take = 1
        sequence = 11
        cycles = 1
        value = 4363
    elif epoch == 29:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 30:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 31:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 32:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 33:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 34:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 35:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 36:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 37:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 38:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 39:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 40:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 41:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 42:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    elif epoch == 43:
        valid = 0
        take = 1
        sequence = 0
        cycles = 0
        value = 0
    return CreditFrame(
        valid=valid, take=take, sequence=sequence, cycles=cycles, value=value
    )


@system
def credit_c8():
    epoch: u8 = 0
    frame = stimulus_c8(epoch)
    result = CreditPipeline(
        valid=frame.valid,
        data=CreditToken(
            sequence=frame.sequence, cycles=frame.cycles, value=frame.value
        ),
        take=frame.take,
    )
    observe_credit(epoch, result)

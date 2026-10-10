"""Four ordered scans with complete immutable prefix snapshots."""

# ruff: noqa: F821, N802 -- forward stage-ready wire and hardware module name.

from pycircuit import module, queue, rule, struct, table, u1, u8


@struct
class Request:
    first: u8
    second: u8
    third: u8


@struct
class Values:
    values: table[3, u8]


@struct
class Pair:
    first: u8
    second: u8


@struct
class Prefixes:
    subtracted: table[3, u8]
    nonzero: table[3, u8]
    reset: table[3, u8]
    tupled: table[3, Pair]


@struct
class ScanResult:
    subtract_first: u8
    subtract_second: u8
    subtract_third: u8
    nonzero_last: u8
    reset_second: u8
    reset_third: u8
    tuple_first: u8
    tuple_third: u8


@struct
class Result:
    ready: u1
    valid: u1
    data: ScanResult


@rule
def subtract(accumulator: u8, value: u8) -> u8:
    return accumulator - value


@rule
def scan_arrays(request: Request) -> ScanResult:
    source = Values(values=(request.first, request.second, request.third))
    values = source.values
    zero = Prefixes()
    subtracted = zero.subtracted
    nonzero = zero.nonzero
    reset = zero.reset
    tupled = zero.tupled
    subtract_accumulator: u8 = 0
    sum_accumulator: u8 = 10
    reset_accumulator: u8 = 5
    pair = Pair(first=values[0], second=values[0])

    for index in range(3):
        value = values[index]
        subtract_accumulator = subtract(subtract_accumulator, value)
        sum_accumulator = sum_accumulator + value
        reset_accumulator = 0 if value == 0 else reset_accumulator + value
        pair = Pair(
            first=0 if value == 0 else pair.first + value,
            second=pair.second,
        )
        subtracted[index] = subtract_accumulator
        nonzero[index] = sum_accumulator
        reset[index] = reset_accumulator
        tupled[index] = pair

    return ScanResult(
        subtract_first=subtracted[0],
        subtract_second=subtracted[1],
        subtract_third=subtracted[2],
        nonzero_last=nonzero[2],
        reset_second=reset[1],
        reset_third=reset[2],
        tuple_first=tupled[0].first,
        tuple_third=tupled[2].first,
    )


@module
def ArrayScans(valid: u1, data: Request, take: u1) -> Result:
    ready, available, request = queue[Request](
        valid,
        data,
        stage_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    scanned = scan_arrays(request)
    stage_ready, output_valid, output_data = queue[ScanResult](
        available,
        scanned,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=output_valid, data=output_data)

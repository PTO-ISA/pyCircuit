"""Fixed-table reductions through the standard two-stage queue contract."""

# ruff: noqa: F821, N802 -- the first queue consumes the later stage-ready wire.
from pycircuit import module, queue, rule, struct, table, u1, u2, u8


@struct
class Request:
    first: u8
    second: u8
    third: u8


@struct
class Values:
    values: table[3, u8]


@struct
class Ranked:
    age: u8
    valid: u1


@struct
class ReductionResult:
    complete: u1
    present: u1
    count: u2
    helper_count: u2
    total: u8
    product: u8
    minimum: u8
    maximum: u8
    parity: u1
    first_index: u2
    first_valid: u1
    best_index: u2
    best_valid: u1
    range_best_index: u2


@struct
class Result:
    ready: u1
    valid: u1
    data: ReductionResult


@rule
def reduce_arrays(request: Request) -> ReductionResult:
    values = Values(values=(request.first, request.second, request.third)).values
    flags = values.map(lambda value: value != 0)
    ranked = values.map(
        lambda value, flag: Ranked(age=value, valid=flag),
        flags,
    )
    first_index, first_valid = ranked.first(where=lambda item: item.valid)
    best_index, best_valid = ranked.argmin(
        where=lambda item: item.valid,
        key=lambda item: item.age,
    )
    bounded = values.map(lambda value: value % 5)
    range_best_index, _range_best_valid = bounded.argmin(
        where=lambda value: True,
        key=lambda value: value,
    )
    shared_count = flags.count()
    return ReductionResult(
        complete=flags.all(),
        present=flags.any(),
        count=shared_count,
        helper_count=shared_count,
        total=values.fold(kind="add"),
        product=values.fold(kind="mul"),
        minimum=values.fold(kind="min"),
        maximum=values.fold(kind="max"),
        parity=flags.fold(kind="xor"),
        first_index=first_index,
        first_valid=first_valid,
        best_index=best_index,
        best_valid=best_valid,
        range_best_index=range_best_index,
    )


@module
def ArrayReductions(valid: u1, data: Request, take: u1) -> Result:
    ready, available, request = queue[Request](
        valid,
        data,
        stage_ready,
        depth=1,
        ready_policy="downstream_pop",
    )
    reduced = reduce_arrays(request)
    stage_ready, out_valid, out_data = queue[ReductionResult](
        available,
        reduced,
        take,
        depth=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)

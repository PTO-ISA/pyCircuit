"""Fixed-table reductions through the standard two-stage queue contract."""

# ruff: noqa: N802 -- hardware module name.
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
    ingress = queue[Request](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = queue[ReductionResult](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @rule
    def transfer() -> None:
        request = ingress.pop()
        reduced = reduce_arrays(request)
        egress.push(reduced)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)

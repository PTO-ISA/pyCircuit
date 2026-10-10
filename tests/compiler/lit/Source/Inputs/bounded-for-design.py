"""Bounded ordered loops over scalar, Table, Struct and wide carriers."""

# ruff: noqa: B007, F821, N802 -- binders and zero-trip name are intentional.

from pycircuit import bits, module, rule, struct, table, u1, u3, u8, u13


@struct
class Pair:
    first: u8
    second: u8


@struct
class Aggregate:
    lanes: table[3, u13]
    marker: u8


@struct
class SharedChild:
    first: u8
    second: u8


@struct
class Box32:
    left: SharedChild
    right: SharedChild


@struct
class ScanCase:
    extent_one: u3
    subtract_first: u8
    subtract_last: u8
    wide_last: u13
    pair_first: u8
    pair_second: u8
    snapshot_first: u8
    snapshot_last: u8


@struct
class BindingCase:
    reused_final: u8
    fresh_final: u8
    fixed_predicate: u8
    body_final: u8
    zero_preserved: u8
    conditional: u8


@struct
class AggregateCase:
    aggregate_lane: u13
    aggregate_marker: u8
    aggregate_snapshot: u13
    same_ssa: u8
    box_left: u8
    box_right: u8


@struct
class CarrierCase:
    wide_flag: u1
    signed_shifted: u8
    carrier: bits[65]


@struct
class BookkeepingCase:
    value: u8


@struct
class Result:
    extent_one: u3
    subtract_first: u8
    subtract_last: u8
    wide_last: u13
    pair_first: u8
    pair_second: u8
    snapshot_first: u8
    snapshot_last: u8
    reused_final: u8
    fresh_final: u8
    fixed_predicate: u8
    body_final: u8
    zero_preserved: u8
    conditional: u8
    aggregate_lane: u13
    aggregate_marker: u8
    aggregate_snapshot: u13
    same_ssa: u8
    box_left: u8
    box_right: u8
    wide_flag: u1
    signed_shifted: u8
    bookkeeping: u8
    carrier: bits[65]


@rule
def subtract(accumulator: u8, value: u8) -> u8:
    return accumulator - value


@rule
def scan_case(
    one_value: u3,
    first: u8,
    second: u8,
    third: u8,
    wide0: u13,
    wide1: u13,
    wide2: u13,
    wide3: u13,
    wide4: u13,
    seed: u8,
) -> ScanCase:
    one_values: table[1, u3] = (one_value,)
    values: table[3, u8] = (first, second, third)
    wide_values: table[5, u13] = (wide0, wide1, wide2, wide3, wide4)
    prefixes: table[3, u8] = (0, 0, 0)
    pair_prefixes: table[3, Pair] = (Pair(), Pair(), Pair())
    original_prefixes = prefixes
    extent_one: u3 = 0
    for index in range(1):
        extent_one = extent_one + one_values[index]
    accumulator: u8 = 0
    pair = Pair(first=seed, second=seed)
    for index in range(3):
        accumulator = subtract(accumulator, values[index])
        prefixes[index] = accumulator
        pair = Pair(first=pair.first + values[index], second=pair.second)
        pair_prefixes[index] = pair
    wide_accumulator: u13 = 0
    for index in range(5):
        wide_accumulator = wide_accumulator + wide_values[index]
    return ScanCase(
        extent_one=extent_one,
        subtract_first=prefixes[0],
        subtract_last=prefixes[2],
        wide_last=wide_accumulator,
        pair_first=pair_prefixes[2].first,
        pair_second=pair_prefixes[2].second,
        snapshot_first=original_prefixes[0],
        snapshot_last=original_prefixes[2],
    )


@rule
def binding_case(first: u8, second: u8, third: u8, seed: u8) -> BindingCase:
    values: table[3, u8] = (first, second, third)
    reused: u8 = 99
    for reused in range(3):
        reused = reused + 1
    for fresh in range(3):
        pass
    predicate: u1 = 0
    fixed_predicate: u8 = 0
    for predicate in range(2):
        if predicate:
            fixed_predicate = fixed_predicate + 1
    body_final: u8 = 0
    for changed in range(3):
        changed = changed + 10
        body_final = changed
    conditional: u8 = 0
    for index in range(3):
        if values[index] == 0:
            conditional = 1
        else:
            conditional = conditional + values[index]
    zero_preserved: u8 = seed
    for absent in range(0):
        zero_preserved = missing_name
    return BindingCase(
        reused_final=reused,
        fresh_final=fresh,
        fixed_predicate=fixed_predicate,
        body_final=body_final,
        zero_preserved=zero_preserved,
        conditional=conditional,
    )


@rule
def aggregate_case(
    first: u8,
    second: u8,
    third: u8,
    wide0: u13,
    wide1: u13,
    wide2: u13,
    wide3: u13,
    wide4: u13,
    seed: u8,
) -> AggregateCase:
    values: table[3, u8] = (first, second, third)
    aggregate = Aggregate(lanes=(wide0, wide1, wide2), marker=seed)
    aggregate_snapshot = aggregate
    alternative_lanes: table[3, u13] = (wide2, wide3, wide4)
    for index in range(3):
        contextual_zero: table[3, u13] = 0
        selected_lanes = contextual_zero if values[index] == 0 else alternative_lanes
        aggregate = Aggregate(
            lanes=selected_lanes,
            marker=aggregate.marker + values[index],
        )
    stable = seed
    stable_alias = stable
    for index in range(3):
        if values[index] == 0:
            pass
        else:
            pass
    shared_child = SharedChild(first=first, second=second)
    alternative_child = SharedChild(first=second, second=third)
    box = Box32(left=shared_child, right=shared_child)
    for index in range(3):
        box = (
            box
            if values[index] == 0
            else Box32(left=alternative_child, right=shared_child)
        )
    return AggregateCase(
        aggregate_lane=aggregate.lanes[1],
        aggregate_marker=aggregate.marker,
        aggregate_snapshot=aggregate_snapshot.lanes[1],
        same_ssa=stable_alias,
        box_left=box.left.first,
        box_right=box.right.second,
    )


@rule
def carrier_case(first: u8, carrier: bits[65]) -> CarrierCase:
    wide_flag: u1 = 0
    for index in range(1):
        wide_flag = carrier == 0
    selected = 0
    for index in range(1):
        if first == 0:
            selected = 0 - 1
        else:
            selected = 7
    signed_shifted: u8 = selected + 1
    carrier_result: bits[65] = 0
    for index in range(1):
        carrier_result = carrier
    return CarrierCase(
        wide_flag=wide_flag,
        signed_shifted=signed_shifted,
        carrier=carrier_result,
    )


@rule
def bookkeeping_case(first: u8, second: u8, third: u8, seed: u8) -> BookkeepingCase:
    values: table[3, u8] = (first, second, third)
    local0 = seed
    local1 = local0
    local2 = local1
    local3 = local2
    local4 = local3
    local5 = local4
    local6 = local5
    local7 = local6
    local8 = local7
    local9 = local8
    local10 = local9
    local11 = local10
    local12 = local11
    local13 = local12
    local14 = local13
    local15 = local14
    for index in range(3):
        if values[index] == 0:
            if seed == 0:
                pass
            else:
                pass
        else:
            pass
    return BookkeepingCase(value=local15)


@module
def Top(
    one_value: u3,
    first: u8,
    second: u8,
    third: u8,
    wide0: u13,
    wide1: u13,
    wide2: u13,
    wide3: u13,
    wide4: u13,
    seed: u8,
    carrier: bits[65],
) -> Result:
    scan = scan_case(
        one_value, first, second, third, wide0, wide1, wide2, wide3, wide4, seed
    )
    binding = binding_case(first, second, third, seed)
    aggregate = aggregate_case(
        first, second, third, wide0, wide1, wide2, wide3, wide4, seed
    )
    carrier_values = carrier_case(first, carrier)
    bookkeeping = bookkeeping_case(first, second, third, seed)
    return Result(
        extent_one=scan.extent_one,
        subtract_first=scan.subtract_first,
        subtract_last=scan.subtract_last,
        wide_last=scan.wide_last,
        pair_first=scan.pair_first,
        pair_second=scan.pair_second,
        snapshot_first=scan.snapshot_first,
        snapshot_last=scan.snapshot_last,
        reused_final=binding.reused_final,
        fresh_final=binding.fresh_final,
        fixed_predicate=binding.fixed_predicate,
        body_final=binding.body_final,
        zero_preserved=binding.zero_preserved,
        conditional=binding.conditional,
        aggregate_lane=aggregate.aggregate_lane,
        aggregate_marker=aggregate.aggregate_marker,
        aggregate_snapshot=aggregate.aggregate_snapshot,
        same_ssa=aggregate.same_ssa,
        box_left=aggregate.box_left,
        box_right=aggregate.box_right,
        wide_flag=carrier_values.wide_flag,
        signed_shifted=carrier_values.signed_shifted,
        bookkeeping=bookkeeping.value,
        carrier=carrier_values.carrier,
    )

# Hardware definition names are stable generated-symbol and mutation-test keys.
# ruff: noqa: N802
from enum import Enum

import pycircuit as ac
from pycircuit import rule as sampled


@ac.struct
class Pair:
    a: ac.u8
    b: ac.u8


@ac.struct
class State:
    pair: Pair
    c: ac.u8


@ac.struct
class Ack:
    value: ac.u8


@ac.struct
class Result:
    a: ac.u8
    b: ac.u8
    c: ac.u8
    side: ac.u8


@sampled
def WriteA(s, data, enable):
    if enable:
        s.pair.a = data
        s.pair.a = s.pair.b ^ data


@ac.rule
def WriteB(s, other, enable) -> None:
    if enable:
        s.pair.b = s.pair.a ^ other
    return


@ac.rule
def WriteSide(side, data) -> Ack:
    side = side ^ data
    return Ack(value=side)


@ac.rule
def UnannotatedValue(data):
    return Result(a=data, b=data, c=data, side=data)


@ac.rule
def WriteBoth(s, data, other, enable) -> Ack:
    if enable:
        s.pair.a = data
    s.pair.b = other
    return Ack(value=s.pair.a)


@ac.rule
def Unregistered(s, data) -> Ack:
    s = State(pair=Pair(a=data, b=data), c=data)  # noqa: F841 - inert write intent oracle
    return Ack(value=data)


@ac.rule
def Identity(s, data, enable) -> Ack:
    s.pair.a = s.pair.a
    return Ack(value=data)


@ac.rule
def Parent(s, data, enable) -> Ack:
    s.pair = Pair(a=data, b=data)
    return Ack(value=data)


@ac.rule
def Whole(s, data, enable) -> Ack:
    s = State(pair=Pair(a=data, b=data), c=data)  # noqa: F841 - whole-owner write intent
    return Ack(value=data)


@ac.rule
def Aliases(left, right, data) -> Ack:
    left.pair.a = data
    return Ack(value=right.pair.a)


@ac.rule
def TableWrite(entries, data) -> Ack:
    entries[0] = data
    return Ack(value=entries[1])


@ac.module
def Multi(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    s: State = State(pair=Pair(a=0, b=0), c=92)
    side: ac.u8 = 9
    copy = s
    copied = Whole(copy, data, en_a)  # noqa: F841 - copied value must not become an owner
    preserved_value = UnannotatedValue(data)  # noqa: F841 - value-return admission oracle
    WriteA(s, data, en_a)
    WriteB(s, other, en_b)
    WriteSide(side, data)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def Reverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    s: State = State(pair=Pair(a=0, b=0), c=92)
    side: ac.u8 = 9
    WriteSide(side, data)
    WriteB(s, other, en_b)
    WriteA(s, data, en_a)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def Solo(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    s: State = State(pair=Pair(a=0, b=0), c=92)
    side: ac.u8 = 9
    first = WriteBoth(s, data, other, en_a)  # noqa: F841 - ignored output retains writes
    third = WriteSide(side, data)  # noqa: F841 - ignored output retains writes
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def Tables(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    left = ac.table[2, ac.u8](init=0)
    right = ac.table[2, ac.u8](init=0)
    first = TableWrite(left, data)  # noqa: F841 - ignored output retains table writes
    second = TableWrite(right, other)  # noqa: F841 - ignored output retains table writes
    return Result(a=left[0], b=right[0], c=left[1], side=right[1])


@ac.struct
class NarrowPair:
    a: ac.u5
    b: ac.u9


@ac.struct
class NestedCell:
    sub: NarrowPair


@ac.struct
class TableResult:
    lane0: NarrowPair
    lane1: NarrowPair
    lane2: NarrowPair


@ac.struct
class ScalarTableResult:
    a: ac.u5
    b: ac.u5
    c: ac.u5


@ac.rule
def IndexedA(entries, data, enable):
    i = data % 3
    if enable:
        entries[i].a = entries[i].b[0:5] ^ data[0:5]


@ac.rule
def IndexedB(entries, other, enable):
    j = other % 3
    wide: ac.u9 = other
    wide = wide | 256
    old: ac.u9 = entries[j].a
    if enable:
        entries[j].b = old ^ wide


@ac.rule
def ScalarFirst(entries, data, enable):
    if enable:
        entries[0] = entries[2] ^ data[0:5]


@ac.rule
def ScalarLast(entries, other, enable):
    if enable:
        entries[2] = entries[0] ^ other[0:5]


@ac.rule
def NestedParent(entries, data, other, enable):
    wide: ac.u9 = other
    wide = wide | 256
    if enable:
        entries[0].sub = NarrowPair(a=data[0:5], b=wide)


@ac.rule
def NestedChild(entries, other, enable):
    if enable:
        entries[2].sub.a = other[0:5]


@ac.rule
def TableConditional(entries, data, other, enable):
    wide: ac.u9 = other
    wide = wide | 256
    if enable:
        entries[0].a = data[0:5]
    entries[0].b = wide


@ac.rule
def StaticLastField(entries, other, enable):
    if enable:
        entries[2].a = other[0:5]


@ac.module
def Indexed(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> TableResult:
    entries = ac.table[3, NarrowPair](init=0)
    IndexedA(entries, data, en_a)
    IndexedB(entries, other, en_b)
    return TableResult(lane0=entries[0], lane1=entries[1], lane2=entries[2])


@ac.module
def IndexedReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> TableResult:
    entries = ac.table[3, NarrowPair](init=0)
    IndexedB(entries, other, en_b)
    IndexedA(entries, data, en_a)
    return TableResult(lane0=entries[0], lane1=entries[1], lane2=entries[2])


@ac.module
def ScalarElements(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, ac.u5](init=0)
    ScalarFirst(entries, data, en_a)
    ScalarLast(entries, other, en_b)
    return ScalarTableResult(a=entries[0], b=entries[1], c=entries[2])


@ac.module
def NestedElements(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> TableResult:
    entries = ac.table[3, NestedCell](init=0)
    NestedParent(entries, data, other, en_a)
    NestedChild(entries, other, en_b)
    return TableResult(lane0=entries[0].sub, lane1=entries[1].sub, lane2=entries[2].sub)


@ac.module
def TableSolo(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> TableResult:
    entries = ac.table[3, NarrowPair](init=0)
    TableConditional(entries, data, other, en_a)
    StaticLastField(entries, other, en_b)
    return TableResult(lane0=entries[0], lane1=entries[1], lane2=entries[2])


@ac.rule
def LiteralFirst(entries, data):
    entries[0].a = data


@ac.rule
def LiteralSecond(entries, data):
    entries[1].b = data


@ac.module
def LiteralFields(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    entries = ac.table[2, Pair](init=0)
    LiteralFirst(entries, data)
    LiteralSecond(entries, data)
    return Result(a=entries[0].a, b=entries[1].b, c=entries[0].b, side=entries[1].a)


@ac.struct
class FiveResult:
    a: ac.u8
    b: ac.u8
    c: ac.u8
    d: ac.u8
    e: ac.u8


@ac.struct
class OneResult:
    value: ac.u8


@ac.rule
def PutIndex(entries, index, data, enable):
    if enable:
        entries[index] = data


@ac.module
def RangeFive(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FiveResult:
    entries = ac.table[5, ac.u8](init=0)
    index = data % 5
    alias = index
    PutIndex(entries, alias, data, en_a)
    return FiveResult(a=entries[0], b=entries[1], c=entries[2], d=entries[3], e=entries[4])


@ac.module
def RangeOne(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> OneResult:
    entries = ac.table[1, ac.u8](init=0)
    index = data % 1
    PutIndex(entries, index, data, en_a)
    return OneResult(value=entries[0])


# Nominal Enum payloads use the same scalar-leaf path; conversion preserves X/Z.
@ac.encoding(width=2)
class Choice(Enum):
    ZERO = 0
    ONE = 1
    TWO = 2
    THREE = 3


@ac.rule
def EnumFirst(entries, data, enable):
    value, valid = ac.enum_from_bits[Choice](data[0:2])  # noqa: F841 - data-only path
    if enable:
        entries[0] = value


@ac.rule
def EnumLast(entries, other, enable):
    value, valid = ac.enum_from_bits[Choice](other[0:2])  # noqa: F841 - data-only path
    if enable:
        entries[2] = value


@ac.module
def EnumElements(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, Choice](init=0)
    EnumFirst(entries, data, en_a)
    EnumLast(entries, other, en_b)
    return ScalarTableResult(a=ac.enum_to_bits(entries[0]), b=ac.enum_to_bits(entries[1]),
                             c=ac.enum_to_bits(entries[2]))


# Grant fixtures share one definition with distinct actual captures. Their
# result layout is the same as Phase A so native/RTL independent oracles agree.
@ac.rule
def GrantScalar(value, data, enable):
    if enable:
        value = value ^ data


@ac.rule
def GrantPrefix(s, data, enable):
    if enable:
        s.pair = Pair(a=s.pair.b ^ data, b=data)


@ac.rule
def GrantField(s, data, enable):
    if enable:
        s.pair.a = s.pair.b ^ data


@ac.rule
def GrantWhole(s, data, enable):
    if enable:
        s = State(pair=Pair(a=s.pair.b ^ data, b=s.pair.b), c=data)


@ac.rule
def GrantTable(entries, index, data, enable):
    if enable:
        entries[index].a = entries[index].b ^ data


@ac.module
def ScalarGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantScalar(value, data, ga)
    GrantScalar(value, other, gb)
    GrantScalar(value, data ^ other, gc)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def ScalarGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantScalar(value, data ^ other, gc)
    GrantScalar(value, other, gb)
    GrantScalar(value, data, ga)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def PrefixGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    s: State = State(pair=Pair(a=7, b=17), c=29)
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantPrefix(s, data, ga)
    GrantField(s, other, gb)
    GrantWhole(s, data ^ other, gc)
    WriteSide(side, data)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def PrefixGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    s: State = State(pair=Pair(a=7, b=17), c=29)
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantWhole(s, data ^ other, gc)
    GrantField(s, other, gb)
    GrantPrefix(s, data, ga)
    WriteSide(side, data)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def TableGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    entries = ac.table[2, Pair](init=0)
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantTable(entries, data % 2, data, ga)
    GrantTable(entries, other % 2, other, gb)
    GrantTable(entries, 0, data ^ other, gc)
    WriteSide(side, data)
    return Result(a=entries[0].a, b=entries[1].a, c=entries[0].b, side=side)


@ac.module
def TableGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    entries = ac.table[2, Pair](init=0)
    ga = en_a
    gb = en_b & ~en_a
    gc = ~en_a & ~en_b
    GrantTable(entries, 0, data ^ other, gc)
    GrantTable(entries, other % 2, other, gb)
    GrantTable(entries, data % 2, data, ga)
    WriteSide(side, data)
    return Result(a=entries[0].a, b=entries[1].a, c=entries[0].b, side=side)


@ac.module
def ComplementGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = ~en_a
    GrantScalar(value, data, ga)
    GrantScalar(value, other, gb)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def ComplementGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = ~en_a
    GrantScalar(value, other, gb)
    GrantScalar(value, data, ga)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def PoisonGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = (en_b & ~en_b) | ~en_a
    GrantScalar(value, data, ga)
    GrantScalar(value, other, gb)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def PoisonGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a
    gb = (en_b & ~en_b) | ~en_a
    GrantScalar(value, other, gb)
    GrantScalar(value, data, ga)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def NeverGrant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a & ~en_a
    gb = en_a
    GrantScalar(value, data, ga)
    GrantScalar(value, other, gb)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def NeverGrantReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    value: ac.u8 = 7
    ga = en_a & ~en_a
    gb = en_a
    GrantScalar(value, other, gb)
    GrantScalar(value, data, ga)
    WriteSide(side, data)
    return Result(a=value, b=17, c=29, side=side)


@ac.module
def PrefixComplement(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    s: State = State(pair=Pair(a=7, b=17), c=29)
    ga = en_a
    gb = ~en_a
    GrantPrefix(s, data, ga)
    GrantField(s, other, gb)
    WriteSide(side, data)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def PrefixComplementReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    s: State = State(pair=Pair(a=7, b=17), c=29)
    ga = en_a
    gb = ~en_a
    GrantField(s, other, gb)
    GrantPrefix(s, data, ga)
    WriteSide(side, data)
    return Result(a=s.pair.a, b=s.pair.b, c=s.c, side=side)


@ac.module
def TableComplement(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    entries = ac.table[2, Pair](init=0)
    ga = en_a
    gb = ~en_a
    GrantTable(entries, data % 2, data, ga)
    GrantTable(entries, other % 2, other, gb)
    WriteSide(side, data)
    return Result(a=entries[0].a, b=entries[1].a, c=entries[0].b, side=side)


@ac.module
def TableComplementReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> Result:
    side: ac.u8 = 9
    entries = ac.table[2, Pair](init=0)
    ga = en_a
    gb = ~en_a
    GrantTable(entries, other % 2, other, gb)
    GrantTable(entries, data % 2, data, ga)
    WriteSide(side, data)
    return Result(a=entries[0].a, b=entries[1].a, c=entries[0].b, side=side)


# P0 indexed-field footprints: all roots use the existing test harness interface.
@ac.encoding(width=2)
class FieldMark(Enum):
    ZERO = 0
    ONE = 1
    TWO = 2
    THREE = 3


@ac.struct
class FieldCell:
    a: ac.u1
    b: ac.u1
    keep: ac.u2
    mark: FieldMark


@ac.struct
class FieldPair:
    a: ac.u1
    b: ac.u1


@ac.struct
class FieldNestedCell:
    pair: FieldPair
    keep: ac.u2
    mark: FieldMark


@ac.struct
class FieldWideCell:
    a: ac.u2
    b: ac.u1
    keep: ac.u2
    mark: FieldMark


@ac.struct
class FieldViews:
    before: ac.u8
    between: ac.u8
    after: ac.u8
    read: ac.u8


@ac.struct
class FieldResult:
    lane0: ac.u8
    lane1: ac.u8
    lane2: ac.u8
    before: ac.u8
    between: ac.u8
    after: ac.u8
    read: ac.u8


@ac.rule
def FieldPut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].a = 0
            entries[index].a = 1
            middle = entries[0]
            index = j
            entries[index].b = other[0:1]
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldNestedPut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].pair.a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].pair.a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].pair.a = 0
            entries[index].pair.a = 1
            middle = entries[0]
            index = j
            entries[index].pair.b = other[0:1]
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.pair.a, saved.pair.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.pair.a, middle.pair.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.pair.a, current.pair.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.pair.a, selected.pair.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldSubtreePut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].pair.a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].pair.a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].pair.a = 0
            entries[index].pair.a = 1
            middle = entries[0]
            index = j
            entries[index].pair = FieldPair(a=other[0:1], b=data[1:2])
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.pair.a, saved.pair.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.pair.a, middle.pair.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.pair.a, current.pair.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.pair.a, selected.pair.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldRhsPut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].a = 0
            entries[index].a = 1
            middle = entries[0]
            index = j
            entries[index].b = entries[index].a
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldWholePut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].a = 0
            entries[index].a = 1
            middle = entries[0]
            index = j
            entries[index] = FieldCell(a=0, b=other[0:1], keep=0, mark=FieldMark.ZERO)
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldWholeReadPut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            middle = entries[0]
        else:
            index = i
            entries[index].a = 0
            entries[index].a = 1
            middle = entries[0]
            index = j
            entries[i] = entries[index]
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldBranchPut(entries, data, other, enable, seed) -> FieldViews:
    i = data[0:1]
    j = other[1:2]
    saved = entries[0]
    middle = entries[0]
    if seed:
        entries[0].a = 0
        entries[0].keep = other[0:2]
        entries[0].mark = FieldMark.TWO
        entries[1].a = 1
        entries[1].keep = other[2:4]
        entries[1].mark = FieldMark.ONE
        middle = entries[0]
    else:
        if enable:
            entries[i].a = 1
        middle = entries[0]
        entries[j].b = other[0:1]
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldRangePut(entries, data, other, enable, seed) -> FieldViews:
    i = data % 3
    j = other % 3
    saved = entries[0]
    middle = entries[0]
    if enable:
        if seed:
            entries[0].a = 0
            entries[0].keep = other[0:2]
            entries[0].mark = FieldMark.TWO
            entries[1].a = 1
            entries[1].keep = other[2:4]
            entries[1].mark = FieldMark.ONE
            entries[2].a = 0
            entries[2].keep = other[4:6]
            entries[2].mark = FieldMark.THREE
            middle = entries[0]
        else:
            index = i
            entries[index].a = 0
            entries[index].a = 1
            middle = entries[0]
            index = j
            entries[index].b = other[0:1]
    current = entries[0]
    selected = entries[j]
    return FieldViews(before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)), between=ac.concat(middle.a, middle.b, middle.keep, ac.enum_to_bits(middle.mark)),
                      after=ac.concat(current.a, current.b, current.keep, ac.enum_to_bits(current.mark)), read=ac.concat(selected.a, selected.b, selected.keep, ac.enum_to_bits(selected.mark)))


@ac.rule
def FieldOther(entries, data, enable):
    if enable:
        entries[data[0:1]].a = 0


@ac.rule
def FieldWideSeed(entries, other, seed):
    if seed:
        entries[0].a = 0
        entries[1].a = 1
        entries[0].keep = other[0:2]
        entries[1].keep = other[2:4]
        entries[0].mark = FieldMark.TWO
        entries[1].mark = FieldMark.ONE


@ac.module
def FieldSingle(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldMulti(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldPut(entries, data, other, en_a, en_b)
    FieldOther(entries, data, ~en_a)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldMultiReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    FieldOther(entries, data, ~en_a)
    views = FieldPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldNestedLeaf(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldNestedCell](init=0)
    views = FieldNestedPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].pair.a, entries[0].pair.b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].pair.a, entries[1].pair.b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldNestedSubtree(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldNestedCell](init=0)
    views = FieldSubtreePut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].pair.a, entries[0].pair.b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].pair.a, entries[1].pair.b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldExplicitRhs(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldRhsPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldWholeConstant(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldWholePut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldWholeRead(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldWholeReadPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldBranch(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldCell](init=0)
    views = FieldBranchPut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldRange(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[3, FieldCell](init=0)
    views = FieldRangePut(entries, data, other, en_a, en_b)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=views.before,
                       between=views.between, after=views.after, read=views.read)


@ac.module
def FieldLocal(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[2, FieldWideCell](init=0)
    FieldWideSeed(entries, other, en_b)
    snapshot = entries
    saved = snapshot[0]
    index = other[1:2]
    # A pure module-local Table candidate is not another persistent owner.
    # Binding u1 RHS to the declared u2 field must zero extend at its boundary.
    snapshot[index].a = data[0:1]
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)),
                       lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=0, before=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)),
                       between=ac.concat(saved.a, saved.b, saved.keep, ac.enum_to_bits(saved.mark)),
                       after=ac.concat(snapshot[0].a, snapshot[0].b, snapshot[0].keep, ac.enum_to_bits(snapshot[0].mark)),
                       read=ac.concat(snapshot[index].a, snapshot[index].b, snapshot[index].keep, ac.enum_to_bits(snapshot[index].mark)))


@ac.rule
def AddressPut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        entries[local].a = value
        entries[local].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=raw)


@ac.rule
def AddressSnapshotPut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        entries[local].a = value
        entries[local].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    local = later
    return Ack(value=local)


@ac.rule
def AddressBranchPut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        if raw[6:7]:
            local = index
            entries[local].a = value
            entries[local].b = raw[3:4]
        else:
            local = index
            entries[local].a = value
            entries[local].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=local)


@ac.rule
def AddressMatchPut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        match raw[6:7]:
            case 0:
                local = index
                entries[local].a = value
                entries[local].b = raw[3:4]
            case _:
                local = index
                entries[local].a = value
                entries[local].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=local)


@ac.rule
def AddressIdentityPut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        entries[local].a = entries[local].a
        entries[local].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=raw)


@ac.rule
def AddressSubtreePut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        entries[local].pair = FieldPair(a=value, b=raw[3:4])
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=raw)


@ac.rule
def AddressWholePut(entries, side, index, later, value, raw, enable) -> Ack:
    local = index
    if enable:
        entries[local] = FieldCell(a=value, b=raw[3:4], keep=raw[4:6], mark=FieldMark.ONE)
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=raw)


@ac.module
def AddressDynamic(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressDynamicReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressReverseOperands(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (j != i))
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressEqPolarity(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | ~(i == j))
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressSelect(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b if i != j else en_b & ~ga
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressAssignment(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressSnapshotPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressSnapshotPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressBranch(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressBranchPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressBranchPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressMatch(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressMatchPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressMatchPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressIdentity(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressIdentityPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressIdentityPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressSubtree(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldNestedCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressSubtreePut(entries, left, i, j, data[2:3], data, ga)
    b = AddressSubtreePut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].pair.a, entries[0].pair.b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].pair.a, entries[1].pair.b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].pair.a, entries[2].pair.b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].pair.a, entries[3].pair.b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressWhole(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressWholePut(entries, left, i, j, data[2:3], data, ga)
    b = AddressWholePut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)


@ac.module
def AddressRange(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[3, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data % 3
    j = other % 3
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=0,
                       between=a.value, after=b.value, read=left ^ right)


@ac.rule
def ScalarClosedLast(entries, other, enable):
    if enable:
        entries[0 + 2] = entries[0] ^ other[0:5]


@ac.rule
def ScalarAliasLast(entries, other, enable):
    if enable:
        index = 2
        entries[index] = entries[0] ^ other[0:5]


@ac.module
def ScalarClosedExpression(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, ac.u5](init=0)
    ScalarFirst(entries, data, en_a)
    ScalarClosedLast(entries, other, en_b)
    return ScalarTableResult(a=entries[0], b=entries[1], c=entries[2])


@ac.module
def ScalarClosedExpressionReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, ac.u5](init=0)
    ScalarClosedLast(entries, other, en_b)
    ScalarFirst(entries, data, en_a)
    return ScalarTableResult(a=entries[0], b=entries[1], c=entries[2])


@ac.module
def ScalarClosedAlias(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, ac.u5](init=0)
    ScalarFirst(entries, data, en_a)
    ScalarAliasLast(entries, other, en_b)
    return ScalarTableResult(a=entries[0], b=entries[1], c=entries[2])


@ac.module
def ScalarClosedAliasReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> ScalarTableResult:
    entries = ac.table[3, ac.u5](init=0)
    ScalarAliasLast(entries, other, en_b)
    ScalarFirst(entries, data, en_a)
    return ScalarTableResult(a=entries[0], b=entries[1], c=entries[2])


@ac.module
def AddressSelectReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b if i != j else en_b & ~ga
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)




@ac.module
def AddressSelectEq(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & ~ga if i == j else en_b
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)




@ac.module
def AddressSelectEqReverse(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & ~ga if i == j else en_b
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    a = AddressPut(entries, left, i, j, data[2:3], data, ga)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)




@ac.rule
def AddressLiteralPut(entries, side, index, later, value, raw, enable) -> Ack:
    if enable:
        entries[0].a = value
        entries[0].b = raw[3:4]
        side = raw  # noqa: F841 - persistent owner proposal observed by root
    return Ack(value=raw)




@ac.module
def AddressMixed(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = 0
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (0 != j))
    a = AddressLiteralPut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)




@ac.module
def AddressWholeField(en_a: ac.u1, en_b: ac.u1, data: ac.u8, other: ac.u8) -> FieldResult:
    entries = ac.table[4, FieldCell](init=0)
    left: ac.u8 = 0
    right: ac.u8 = 0
    i = data[0:2]
    j = other[0:2]
    ga = en_a
    gb = en_b & (~ga | (i != j))
    a = AddressWholePut(entries, left, i, j, data[2:3], data, ga)
    b = AddressPut(entries, right, j, i, other[2:3], other, gb)
    return FieldResult(lane0=ac.concat(entries[0].a, entries[0].b, entries[0].keep, ac.enum_to_bits(entries[0].mark)), lane1=ac.concat(entries[1].a, entries[1].b, entries[1].keep, ac.enum_to_bits(entries[1].mark)),
                       lane2=ac.concat(entries[2].a, entries[2].b, entries[2].keep, ac.enum_to_bits(entries[2].mark)), before=ac.concat(entries[3].a, entries[3].b, entries[3].keep, ac.enum_to_bits(entries[3].mark)),
                       between=a.value, after=b.value, read=left ^ right)

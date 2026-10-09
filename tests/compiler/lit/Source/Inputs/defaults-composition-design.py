"""Generic defaults, snapshots and persistent direct-call hierarchy fixture."""
import pycircuit as ac


@ac.struct
class Inner:
    ready: ac.u1 = 1
    payload: ac.u5 = (1 + 6)


@ac.struct
class Parcel:
    raw: Inner
    configured: Inner = Inner()
    mark: ac.u4 = (3 * 3)


@ac.struct
class ChildResult:
    prior: ac.u5
    proposed: ac.u5
    raw_ready: ac.u1
    configured_ready: ac.u1
    zero_payload: ac.u5
    default_payload: ac.u5
    untouched_payload: ac.u5
    snapshot_payload: ac.u5
    local_payload: ac.u5
    kept_mark: ac.u4
    owner_snapshot: ac.u5


@ac.struct
class Word:
    value: ac.u5
    mark: ac.u3


@ac.struct
class Box:
    items: ac.table[3, Word] = (Word(value=3, mark=1),
                              Word(value=5, mark=2),
                              Word(value=7, mark=4))
    flag: ac.u1 = 1


@ac.struct
class TableSnapshot:
    saved: Box
    flagged: Box
    replaced: Box
    selected: Box
    zero: Box
    defaulted: Box
    old_entry: Box
    proposed_entry: Box
    peer_entry: Box


@ac.struct
class Result:
    left: ChildResult
    right: ChildResult
    fanout: ac.u5
    sequential: ac.u5
    nested: ac.u5
    table_probe: TableSnapshot


@ac.rule
def table_snapshots(boxes, value, enable, index):
    local = Box()
    saved = local
    local.flag = enable
    flagged = local
    local.items = [Word(value=value, mark=6), Word(value=9, mark=1),
                   Word(value=21, mark=3)]
    replaced = local
    local.flag = 0
    selected = replaced if enable else saved
    old_entry = boxes[index]
    if enable:
        boxes[index] = Box(items=(Word(value=value, mark=6),
                                  Word(value=9, mark=1),
                                  Word(value=21, mark=3)), flag=0)
    return TableSnapshot(saved=saved, flagged=flagged, replaced=replaced,
                         selected=selected, zero=0, defaulted=Box(),
                         old_entry=old_entry, proposed_entry=boxes[index],
                         peer_entry=boxes[index ^ 1])


@ac.module
def TableProbe(value: ac.u5, enable: ac.u1, index: ac.u1) -> TableSnapshot:  # noqa: N802
    boxes = ac.table[2, Box](init=Box())
    return table_snapshots(boxes, value, enable, index)


@ac.rule
def revise(owner, zero, configured, enable, index, value):
    old_owner = owner
    old = configured[index]
    local = Parcel()
    saved = local
    local.raw.payload = value
    result = ChildResult()
    result.prior = owner.raw.payload
    if enable:
        owner.raw.payload = value
        configured[index] = Parcel(configured=Inner(ready=0, payload=value), mark=3)
    result.proposed = owner.raw.payload
    result.raw_ready = saved.raw.ready
    result.configured_ready = configured[index].configured.ready
    result.zero_payload = zero[index].configured.payload
    result.default_payload = old.configured.payload
    result.untouched_payload = configured[index ^ 1].configured.payload
    result.snapshot_payload = saved.raw.payload
    result.local_payload = local.raw.payload
    result.kept_mark = owner.mark
    result.owner_snapshot = old_owner.raw.payload
    return result


@ac.module
def Cell(enable: ac.u1, index: ac.u1, value: ac.u5) -> ChildResult:  # noqa: N802
    owner: Parcel = Parcel()
    zero = ac.table[2, Parcel](init=0)
    configured = ac.table[2, Parcel](init=Parcel())
    return revise(owner, zero, configured, enable, index, value)


@ac.module
def Middle(enable: ac.u1, index: ac.u1, value: ac.u5) -> ChildResult:  # noqa: N802
    child = Cell(enable, index, value)
    return child


@ac.struct
class Number:
    value: ac.u5


@ac.module
def Add(value: ac.u5) -> Number:  # noqa: N802
    return Number(value=value + 3)


@ac.module
def Top(left_enable: ac.u1, right_enable: ac.u1, index: ac.u1,  # noqa: N802
        value: ac.u5) -> Result:
    left = Middle(left_enable, index, value)
    right = Middle(right_enable, index, value)
    shared = Add(left.proposed)
    fanout = shared
    following = Add(shared.value)
    nested = Add(Add(right.proposed).value)
    table_probe = TableProbe(value, left_enable, index)
    return Result(left=left, right=right, fanout=fanout.value,
                  sequential=following.value, nested=nested.value,
                  table_probe=table_probe)


@ac.struct
class Pair:
    first: ChildResult
    second: ChildResult


@ac.module
def SameInputs(enable: ac.u1, index: ac.u1, value: ac.u5) -> Pair:  # noqa: N802
    first = Cell(enable, index, value)
    second = Cell(enable, index, value)
    return Pair(first=first, second=second)


@ac.module
def Collision(result: ac.u5) -> Number:  # noqa: N802
    return Number(value=result)


@ac.rule
def read_scalar(image, index):
    return Number(value=image[index])


@ac.module
def ScalarLiteral(index: ac.u1) -> Number:  # noqa: N802
    image = ac.table[2, ac.u5](init=7)
    return read_scalar(image, index)


@ac.module
def ScalarExpression(index: ac.u1) -> Number:  # noqa: N802
    image = ac.table[2, ac.u5](init=(3 + 4))
    return read_scalar(image, index)

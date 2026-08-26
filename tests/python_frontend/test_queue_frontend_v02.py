from __future__ import annotations

import unittest


SOURCE = """
from agentic_circuit import sink, source, system

@system
def pipeline() -> None:
    input_queue = source(int, depth=4, latency=1)
    output_queue = input_queue.apply(lambda item: item, depth=8, latency=2)
    sink(output_queue)
"""

POPCOUNT_SOURCE = """
from agentic_circuit import sink, source, struct, system

@struct
class Item:
    value: u8
    count: u4

@system
def pipeline() -> None:
    input_queue = source(Item)
    output_queue = input_queue.apply(
        lambda item: item.with_fields(count=popcount(item.value)),
        depth=2,
        latency=1,
    )
    sink(output_queue)
"""

STRUCT_SOURCE = """
from agentic_circuit import sink, source, struct, system

@struct
class WorkItem:
    value: int
    remaining: int

@system
def pipeline() -> None:
    input_queue = source(WorkItem, depth=4, latency=1)
    output_queue = input_queue.apply(
        lambda item: item.with_fields(
            value=(item.value + 1) * 2,
            remaining=item.remaining - 1,
        ),
        depth=8,
        latency=2,
    )
    sink(output_queue)
"""

SCOPE_SOURCE = """
from agentic_circuit import scope, sink, source, system

@system
def pipeline() -> None:
    input_queue = source(int, depth=4, latency=1)
    with scope("frontend"):
        adjusted = input_queue.apply(lambda item: item + 1)
        with scope("inner"):
            completed = adjusted.apply(lambda item: item * 2)
    sink(completed)
"""

BROADCAST_SOURCE = """
from agentic_circuit import sink, source, system

@system
def pipeline() -> None:
    input_queue = source(int)
    left = input_queue.apply(lambda item: item + 1)
    right = input_queue.apply(lambda item: item * 2)
    sink(left)
    sink(right)
"""

CROSS_SCOPE_BROADCAST_SOURCE = """
from agentic_circuit import scope, sink, source, system

@system
def pipeline() -> None:
    input_queue = source(int)
    with scope("left"):
        left = input_queue.apply(lambda item: item + 1)
    with scope("right"):
        right = input_queue.apply(lambda item: item * 2)
    sink(left)
    sink(right)
"""

ROUTE_SOURCE = """
from agentic_circuit import sink, source, struct, system

@struct
class Item:
    value: int
    route: int

@system
def pipeline() -> None:
    input_queue = source(Item)
    left, right = input_queue.route(
        outputs=2,
        key=lambda item: item.route,
        depth=2,
        latency=1,
    )
    merged = left.merge(right, policy="round_robin", depth=3, latency=1)
    sink(merged)
"""

COLLECTION_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    lanes = ac.array(2, lambda lane: ac.source(int, depth=lane + 1))
    named = ac.map({"right": lanes[1], "left": lanes[0]})
    active = ac.set({named["right"], named["left"]})
    for lane in active:
        ac.sink(lane)
"""

NESTED_COLLECTION_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    grid = ac.array(
        2,
        lambda row: ac.array(
            2,
            lambda column: ac.source(int, depth=row + column + 1),
        ),
    )
    ac.sink(grid[1][0])
"""

FEEDBACK_SOURCE = """
from agentic_circuit import sink, source, struct, system

@struct
class Item:
    value: int
    remaining: int

@system
def pipeline() -> None:
    current = source(Item)
    while current.remaining > 0:
        current = current.apply(
            lambda item: item.with_fields(
                value=item.value + 1,
                remaining=item.remaining - 1,
            ),
            depth=2,
            latency=1,
        )
    sink(current)
"""

OBSERVE_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    input_queue = ac.source(int)
    ac.observe(input_queue)
    ac.sink(input_queue)
"""

ATOMIC_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    left = ac.source(int)
    right = ac.source(int)
    with ac.atomic():
        left_next = left.apply(lambda item: item + 1)
        right_next = right.apply(lambda item: item * 2)
    ac.sink(left_next)
    ac.sink(right_next)
"""

STATIC_CONTROL_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    input_queue = ac.source(int)
    if True:
        selected = input_queue.apply(lambda item: item + 1)
    else:
        unreachable = input_queue.apply(lambda item: item + 99)
    lanes = ac.array(2, lambda index: ac.source(int))
    for index in range(2):
        ac.sink(lanes[index])
    ac.sink(selected)
"""

CONST_KEY_MAP_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    one = ac.source(int)
    two = ac.source(int)
    lanes = ac.map({2: two, 1: one})
    ac.sink(lanes[1])
    ac.sink(lanes[2])
"""

WIDTH_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Header:
    value: ac.u32
    route: ac.u2
    remaining: ac.u16
    valid: bool

@ac.system
def pipeline() -> None:
    input_queue = ac.source(Header)
    output_queue = input_queue.apply(
        lambda item: item.with_fields(
            value=item.value + 1,
            remaining=item.remaining - 1,
        )
    )
    ac.sink(output_queue)
"""

FORK_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    input_queue = ac.source(int)
    left, right = input_queue.fork(outputs=2, depth=2, latency=1)
    ac.sink(left)
    ac.sink(right)
"""

RUNTIME_IF_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Item:
    value: int
    route: int

@ac.system
def pipeline() -> None:
    input_queue = ac.source(Item)
    if input_queue.route == 0:
        output_queue = input_queue.apply(
            lambda item: item.with_fields(value=item.value + 10)
        )
    else:
        output_queue = input_queue.apply(
            lambda item: item.with_fields(value=item.value + 20)
        )
    ac.sink(output_queue)
"""

REORDER_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Token:
    sequence: ac.u64
    value: ac.u32

@ac.system
def pipeline() -> None:
    completed = ac.source(Token)
    retired = completed.reorder(
        key=lambda item: item.sequence,
        capacity=16,
        start=0,
        depth=4,
        latency=1,
    )
    ac.sink(retired)
"""

DEPENDENCY_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Token:
    sequence: ac.u8
    waits_for: ac.u8
    resource: ac.u2
    cycles: ac.u16

@ac.system
def pipeline() -> None:
    issued = ac.source(Token)
    completed = issued.depend(
        key=lambda item: item.sequence,
        waits_for=lambda item: item.waits_for,
        resource=lambda item: item.resource,
        cost=lambda item: item.cycles,
        capacity=16,
        resources=4,
        no_dependency=255,
        depth=8,
        latency=1,
    )
    ac.sink(completed)
"""

MEMORY_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Request:
    address: ac.u8
    write: bool
    data: ac.u16

@ac.system
def pipeline() -> None:
    requests = ac.source(Request)
    responses = requests.memory(
        address=lambda item: item.address,
        write=lambda item: item.write,
        data=lambda item: item.data,
        entries=16,
        init=0,
        result_field="data",
        depth=4,
        latency=1,
    )
    ac.sink(responses)
"""

CREDIT_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    issued = ac.source(int)
    completed = issued.credit(
        cost=lambda item: item,
        credits=4,
        depth=4,
        latency=1,
    )
    ac.sink(completed)
"""

BARRIER_SOURCE = """
import agentic_circuit as ac

@ac.system
def pipeline() -> None:
    left = ac.source(int)
    right = ac.source(int)
    left_ready, right_ready = left.barrier(right, depth=2, latency=1)
    ac.sink(left_ready)
    ac.sink(right_ready)
"""

SELECT_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Control:
    route: ac.u1

@ac.system
def pipeline() -> None:
    control = ac.source(Control)
    lanes = ac.array(2, lambda index: ac.source(int))
    selected = lanes.select(
        control,
        key=lambda item: item.route,
        depth=2,
        latency=1,
    )
    ac.sink(selected)
"""

FIRING_SOURCE = """
import agentic_circuit as ac

@ac.struct
class FiringItem:
    value: ac.u16

@ac.system
def pipeline() -> None:
    incoming = ac.source(FiringItem)
    outgoing = incoming.firing(
        lambda queue: queue.push(
            queue.pop().with_fields(value=queue.peek().value + 1)
        )
    )
    ac.sink(outgoing)
"""

LOOP_CONTROL_SOURCE = """
import agentic_circuit as ac

@ac.struct
class LoopItem:
    remaining: ac.u4
    stop: bool
    skip: bool

@ac.system
def pipeline() -> None:
    current = ac.source(LoopItem)
    while current.remaining > 0:
        if current.stop:
            break
        current = current.apply(
            lambda item: item.with_fields(remaining=item.remaining - 1)
        )
        if current.skip:
            continue
    ac.sink(current)
"""

RECURSION_SOURCE = """
import agentic_circuit as ac

def stages(queue, count):
    if count == 0:
        return queue
    return stages(
        queue.apply(lambda item: item + 1, depth=2, latency=1),
        count - 1,
    )

@ac.system
def pipeline() -> None:
    incoming = ac.source(int)
    outgoing = stages(incoming, 3)
    ac.sink(outgoing)
"""

EXPECT_SOURCE = """
import agentic_circuit as ac

@ac.struct
class ExpectedItem:
    value: ac.u16

@ac.system
def pipeline() -> None:
    incoming = ac.source(ExpectedItem)
    ac.expect(
        incoming,
        predicate=lambda item: item.value > 0,
        message="value must be positive",
    )
    ac.sink(incoming)
"""


class QueueFrontendV02Test(unittest.TestCase):
    def test_popcount_lowers_to_width_checked_var_operation(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(POPCOUNT_SOURCE, "pipeline")
        self.assertIn(
            "ac.var.popcount %v0 : !ac.var<i8> -> !ac.var<i4>",
            lowered,
        )

    def test_verification_expect_is_non_consuming_and_role_explicit(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(EXPECT_SOURCE, "pipeline")
        self.assertIn("ac.expect %incoming message", lowered)
        self.assertIn("ac.expect.yield", lowered)
        self.assertIn("ac.sink %incoming", lowered)
        with self.assertRaisesRegex(QueueFrontendError, "predicate must lower to bool"):
            lower_queue_source(
                EXPECT_SOURCE.replace("item.value > 0", "item.value"), "pipeline"
            )

    def test_compile_time_recursion_expands_to_frozen_queue_chain(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(RECURSION_SOURCE, "pipeline")
        self.assertEqual(3, lowered.count(" = ac.transform "))
        self.assertIn("%outgoing__rec0 = ac.transform %incoming", lowered)
        self.assertIn("%outgoing__rec1 = ac.transform %outgoing__rec0", lowered)
        self.assertIn("%outgoing = ac.transform %outgoing__rec1", lowered)
        self.assertEqual(lowered, lower_queue_source(RECURSION_SOURCE, "pipeline"))
        with self.assertRaisesRegex(QueueFrontendError, "recursion depth"):
            lower_queue_source(
                RECURSION_SOURCE.replace("stages(incoming, 3)", "stages(incoming, runtime)"),
                "pipeline",
            )

    def test_bounded_loop_break_and_tail_continue_lower_to_feedback_edges(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(LOOP_CONTROL_SOURCE, "pipeline")
        self.assertIn("ac.feedback", lowered)
        self.assertIn('field "stop"', lowered)
        self.assertIn('field "skip"', lowered)
        self.assertIn('ac.var.cmp "eq"', lowered)
        self.assertIn("ac.var.mul", lowered)
        self.assertIn("ac.feedback.yield", lowered)

    def test_python_firing_effects_normalize_to_standard_atomic_transform(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(FIRING_SOURCE, "pipeline")
        self.assertEqual(lowered, lower_queue_source(FIRING_SOURCE, "pipeline"))
        self.assertIn("%outgoing = ac.transform %incoming", lowered)
        self.assertIn('ac.var.get %item field "value"', lowered)
        self.assertIn("ac.transform.yield", lowered)
        with self.assertRaisesRegex(QueueFrontendError, "exactly one pop and one push"):
            lower_queue_source(
                FIRING_SOURCE.replace("queue.pop()", "queue.peek()"), "pipeline"
            )
        with self.assertRaisesRegex(QueueFrontendError, "queue effects require firing"):
            lower_queue_source(FIRING_SOURCE.replace(".firing(", ".apply("), "pipeline")

    def test_runtime_queue_collection_index_lowers_to_official_select(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(SELECT_SOURCE, "pipeline")
        self.assertIn(
            "%selected = ac.select %control, %lanes__0, %lanes__1 "
            "depth 2 latency 1 key",
            lowered,
        )
        self.assertIn('ac.var.get %item field "route"', lowered)
        self.assertIn("ac.select.yield", lowered)
        self.assertNotIn("dynamic", lowered)

    def test_barrier_lowers_multi_queue_atomic_synchronization(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(BARRIER_SOURCE, "pipeline")
        self.assertIn(
            "%left_ready, %right_ready = ac.barrier %left, %right "
            "depths [2, 2] latencies [1, 1]",
            lowered,
        )
        with self.assertRaisesRegex(QueueFrontendError, "inputs must be unique"):
            lower_queue_source(
                BARRIER_SOURCE.replace("left.barrier(right", "left.barrier(left"),
                "pipeline",
            )
        with self.assertRaisesRegex(QueueFrontendError, "matching input/output arity"):
            lower_queue_source(
                BARRIER_SOURCE.replace(
                    "left_ready, right_ready =", "left_ready, right_ready, extra ="
                ),
                "pipeline",
            )

    def test_credit_lowers_bounded_parallel_completion_window(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(CREDIT_SOURCE, "pipeline")
        self.assertIn(
            "%completed = ac.credit %issued credits 4 depth 4 latency 1 cost",
            lowered,
        )
        self.assertIn("ac.credit.yield %item : !ac.var<i64>", lowered)
        with self.assertRaisesRegex(QueueFrontendError, "credits must be positive"):
            lower_queue_source(
                CREDIT_SOURCE.replace("credits=4", "credits=0"), "pipeline"
            )

    def test_memory_lowers_old_data_request_response_contract(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(MEMORY_SOURCE, "pipeline")
        self.assertIn(
            "%responses = ac.memory %requests entries 16 init 0 "
            'result_field "data" depth 4 latency 1 address',
            lowered,
        )
        self.assertIn("} write {", lowered)
        self.assertIn("} data {", lowered)
        self.assertEqual(3, lowered.count("ac.memory.yield"))
        with self.assertRaisesRegex(QueueFrontendError, "memory init must be zero"):
            lower_queue_source(MEMORY_SOURCE.replace("init=0", "init=1"), "pipeline")

    def test_dependency_lowers_four_pure_policies(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(DEPENDENCY_SOURCE, "pipeline")
        self.assertIn(
            "%completed = ac.dependency %issued capacity 16 resources 4 "
            "no_dependency 255 depth 8 latency 1 key",
            lowered,
        )
        self.assertIn("} waits_for {", lowered)
        self.assertIn("} resource {", lowered)
        self.assertIn("} cost {", lowered)
        self.assertEqual(4, lowered.count("ac.dependency.yield"))
        with self.assertRaisesRegex(QueueFrontendError, "requires one cost lambda"):
            lower_queue_source(
                DEPENDENCY_SOURCE.replace("cost=lambda item: item.cycles,", ""),
                "pipeline",
            )

    def test_reorder_lowers_sequence_key_and_static_capacity(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(REORDER_SOURCE, "pipeline")
        self.assertIn(
            "%retired = ac.reorder %completed capacity 16 start 0 depth 4 latency 1",
            lowered,
        )
        self.assertIn('ac.var.get %item field "sequence"', lowered)
        self.assertIn("ac.reorder.yield", lowered)
        with self.assertRaisesRegex(QueueFrontendError, "capacity must be positive"):
            lower_queue_source(
                REORDER_SOURCE.replace("capacity=16", "capacity=0"), "pipeline"
            )

    def test_simple_serial_python_lowers_to_typed_queue_graph(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        self.assertEqual(
            """module attributes {ac.contract_epoch = "0.2", ac.system = "pipeline"} {
  %input_queue = ac.source depth 4 latency 1 {ac.name = "input_queue"} : !ac.queue<i64>
  %output_queue = ac.transform %input_queue depths [8] latencies [2] {
  ^transform(%item: !ac.var<i64>):
    ac.transform.yield %item : !ac.var<i64>
  } {ac.name = "output_queue"} : (!ac.queue<i64>) -> !ac.queue<i64>
  ac.sink %output_queue {ac.name = "sink_2"} : !ac.queue<i64>
}
""",
            lower_queue_source(SOURCE, "pipeline"),
        )

    def test_repeated_lowering_is_byte_identical(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        self.assertEqual(
            lower_queue_source(SOURCE, "pipeline"),
            lower_queue_source(SOURCE, "pipeline"),
        )

    def test_struct_and_immutable_lambda_lower_to_var_operations(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(STRUCT_SOURCE, "pipeline")
        self.assertIn("ac.type_scope @types", lowered)
        self.assertIn("ac.struct @WorkItem", lowered)
        self.assertIn("!ac.queue<!ac.struct<@types::@WorkItem>>", lowered)
        self.assertIn('ac.var.get %item field "value"', lowered)
        self.assertIn("ac.var.constant 1 : i64", lowered)
        self.assertIn("ac.var.add", lowered)
        self.assertIn("ac.var.mul", lowered)
        self.assertIn("ac.var.sub", lowered)
        self.assertIn("ac.var.with", lowered)
        self.assertIn('field "remaining"', lowered)

    def test_nested_scope_infers_borrowed_local_and_exported_queues(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(SCOPE_SOURCE, "pipeline")
        self.assertIn("%completed = ac.scope @frontend(%input_queue)", lowered)
        self.assertIn("^body(%input_queue__in: !ac.queue<i64>):", lowered)
        self.assertIn(
            "%completed__inner = ac.scope @inner(%adjusted__local)",
            lowered,
        )
        self.assertIn("ac.scope.yield %completed__local", lowered)
        self.assertIn(
            'ac.sink %completed {ac.name = "sink_5"} : !ac.queue<i64>', lowered
        )

    def test_multiple_consumers_insert_strict_atomic_broadcast(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(BROADCAST_SOURCE, "pipeline")
        self.assertIn(
            "%input_queue__fanout0, %input_queue__fanout1 = ac.broadcast "
            "%input_queue depths [1, 1] latencies [1, 1]",
            lowered,
        )
        self.assertIn("ac.transform %input_queue__fanout0", lowered)
        self.assertIn("ac.transform %input_queue__fanout1", lowered)

    def test_cross_scope_broadcast_is_placed_at_lexical_lca(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(CROSS_SCOPE_BROADCAST_SOURCE, "pipeline")
        broadcast = lowered.index("ac.broadcast %input_queue")
        left_scope = lowered.index("ac.scope @left(%input_queue__fanout0)")
        right_scope = lowered.index("ac.scope @right(%input_queue__fanout1)")
        self.assertLess(broadcast, left_scope)
        self.assertLess(broadcast, right_scope)
        self.assertIn("^body(%input_queue__fanout0__in: !ac.queue<i64>):", lowered)

    def test_tuple_route_lowers_selector_to_var_region(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(ROUTE_SOURCE, "pipeline")
        self.assertIn("%left, %right = ac.route %input_queue", lowered)
        self.assertIn("depths [2, 2] latencies [1, 1]", lowered)
        self.assertIn('ac.var.get %item field "route"', lowered)
        self.assertIn("ac.route.yield", lowered)
        self.assertIn('%merged = ac.merge %left, %right policy "round_robin"', lowered)
        self.assertIn("depth 3 latency 1", lowered)
        self.assertIn("ac.sink %merged", lowered)

    def test_static_queue_collections_flatten_in_canonical_order(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(COLLECTION_SOURCE, "pipeline")
        self.assertIn("%lanes__0 = ac.source depth 1", lowered)
        self.assertIn("%lanes__1 = ac.source depth 2", lowered)
        self.assertLess(
            lowered.index("ac.sink %lanes__0"),
            lowered.index("ac.sink %lanes__1"),
        )
        self.assertNotIn("dynamic", lowered)
        reordered = COLLECTION_SOURCE.replace(
            '{named["right"], named["left"]}',
            '{named["left"], named["right"]}',
        )
        self.assertEqual(lowered, lower_queue_source(reordered, "pipeline"))

    def test_dynamic_or_duplicate_collection_shape_is_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        with self.assertRaisesRegex(QueueFrontendError, "positive compile-time extent"):
            lower_queue_source(
                COLLECTION_SOURCE.replace("ac.array(2", "ac.array(runtime"),
                "pipeline",
            )
        with self.assertRaisesRegex(QueueFrontendError, "members must be unique"):
            lower_queue_source(
                COLLECTION_SOURCE.replace(
                    '{named["right"], named["left"]}',
                    '{named["left"], named["left"]}',
                ),
                "pipeline",
            )

    def test_nested_collection_shape_is_statically_flattened(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(NESTED_COLLECTION_SOURCE, "pipeline")
        self.assertIn("%grid__0__0 = ac.source depth 1", lowered)
        self.assertIn("%grid__0__1 = ac.source depth 2", lowered)
        self.assertIn("%grid__1__0 = ac.source depth 2", lowered)
        self.assertIn("%grid__1__1 = ac.source depth 3", lowered)
        self.assertIn("ac.sink %grid__1__0", lowered)

    def test_serial_while_lowers_to_bounded_feedback(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(FEEDBACK_SOURCE, "pipeline")
        self.assertIn("ac.feedback %current depth 2 latency 1", lowered)
        self.assertIn("max_iterations 1024", lowered)
        self.assertIn('ac.var.cmp "sgt"', lowered)
        self.assertIn("ac.feedback.yield", lowered)
        self.assertIn("ac.sink %current__feedback0", lowered)

    def test_observation_only_use_does_not_insert_broadcast(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(OBSERVE_SOURCE, "pipeline")
        self.assertIn('ac.observe %input_queue name "observe_1"', lowered)
        self.assertIn("ac.sink %input_queue", lowered)
        self.assertNotIn("ac.broadcast", lowered)

    def test_explicit_atomic_groups_multiple_queue_updates(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(ATOMIC_SOURCE, "pipeline")
        self.assertIn("%left_next, %right_next = ac.transform %left, %right", lowered)
        self.assertIn(
            "^transform(%item0: !ac.var<i64>, %item1: !ac.var<i64>):", lowered
        )
        self.assertIn("ac.transform.yield", lowered)
        self.assertIn('ac.output_names = ["left_next", "right_next"]', lowered)
        with self.assertRaisesRegex(QueueFrontendError, "inputs must be unique"):
            lower_queue_source(
                ATOMIC_SOURCE.replace(
                    "right_next = right.apply", "right_next = left.apply"
                ),
                "pipeline",
            )

    def test_static_if_and_range_are_fully_expanded(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        lowered = lower_queue_source(STATIC_CONTROL_SOURCE, "pipeline")
        self.assertIn('ac.name = "selected"', lowered)
        self.assertNotIn("unreachable", lowered)
        self.assertIn("ac.sink %lanes__0", lowered)
        self.assertIn("ac.sink %lanes__1", lowered)
        with self.assertRaisesRegex(QueueFrontendError, "one result name"):
            lower_queue_source(
                STATIC_CONTROL_SOURCE.replace("if True:", "if input_queue:"),
                "pipeline",
            )

    def test_user_opcode_definition_is_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        illegal = """
import agentic_circuit as ac

@ac.opcode
def private_block():
    pass

@ac.system
def pipeline() -> None:
    queue = ac.source(int)
    ac.sink(queue)
"""
        with self.assertRaisesRegex(QueueFrontendError, "providers are forbidden"):
            lower_queue_source(illegal, "pipeline")

    def test_static_map_accepts_canonical_integer_keys(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(CONST_KEY_MAP_SOURCE, "pipeline")
        self.assertLess(lowered.index("ac.sink %one"), lowered.index("ac.sink %two"))

    def test_explicit_integer_widths_freeze_payload_layout(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(WIDTH_SOURCE, "pipeline")
        self.assertIn('{name = "value", type = i32}', lowered)
        self.assertIn('{name = "route", type = i2}', lowered)
        self.assertIn('{name = "remaining", type = i16}', lowered)
        self.assertIn('{name = "valid", type = i1}', lowered)
        self.assertIn("size = 12 : i64", lowered)

    def test_explicit_fork_lowers_to_decoupled_fanout(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(FORK_SOURCE, "pipeline")
        self.assertIn(
            "%left, %right = ac.fork %input_queue depths [2, 2] latencies [1, 1]",
            lowered,
        )

    def test_runtime_if_infers_route_branch_transforms_and_merge(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(RUNTIME_IF_SOURCE, "pipeline")
        self.assertEqual(lowered, lower_queue_source(RUNTIME_IF_SOURCE, "pipeline"))
        self.assertIn(
            "%output_queue__if_false0_in, %output_queue__if_true0_in = "
            "ac.route %input_queue",
            lowered,
        )
        self.assertIn('ac.var.cmp "eq"', lowered)
        self.assertIn("ac.route.yield", lowered)
        self.assertIn("ac.transform %output_queue__if_false0_in", lowered)
        self.assertIn("ac.transform %output_queue__if_true0_in", lowered)
        self.assertIn(
            "%output_queue = ac.merge %output_queue__if_false0, "
            '%output_queue__if_true0 policy "priority"',
            lowered,
        )

    def test_runtime_if_requires_symmetric_queue_assignment(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        with self.assertRaisesRegex(
            QueueFrontendError, "requires one apply assignment in each branch"
        ):
            lower_queue_source(
                RUNTIME_IF_SOURCE.replace("    else:\n", "    else:\n        pass\n"),
                "pipeline",
            )
        with self.assertRaisesRegex(QueueFrontendError, "one result name"):
            lower_queue_source(
                RUNTIME_IF_SOURCE.replace(
                    "        output_queue = input_queue.apply(\n"
                    "            lambda item: item.with_fields(value=item.value + 20)",
                    "        other_queue = input_queue.apply(\n"
                    "            lambda item: item.with_fields(value=item.value + 20)",
                ),
                "pipeline",
            )
        with self.assertRaisesRegex(QueueFrontendError, "must lower to bool"):
            lower_queue_source(
                RUNTIME_IF_SOURCE.replace(
                    "if input_queue.route == 0:", "if input_queue.route:"
                ),
                "pipeline",
            )

    def test_latency_zero_and_unsupported_lambda_are_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        with self.assertRaisesRegex(QueueFrontendError, "latency must be positive"):
            lower_queue_source(SOURCE.replace("latency=2", "latency=0"), "pipeline")
        with self.assertRaisesRegex(QueueFrontendError, "unsupported lambda"):
            lower_queue_source(
                SOURCE.replace("lambda item: item", "lambda item: unknown(item)"),
                "pipeline",
            )


if __name__ == "__main__":
    unittest.main()

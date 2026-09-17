from __future__ import annotations

import unittest


STATEMENT_HANDLER_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Token:
    sequence: ac.u8
    waits_for: ac.u8
    resource: ac.u2
    cycles: ac.u8
    route: ac.u1

@ac.system
def pipeline() -> Token:
    incoming = ac.source(Token)
    left, right = incoming.fork(outputs=2)
    left_ready, right_ready = left.barrier(right)
    lane0, lane1 = left_ready.route(
        outputs=2,
        key=lambda item: item.route,
    )
    merged = lane0.merge(lane1, policy="round_robin")
    scheduled = merged.depend(
        key=lambda item: item.sequence,
        waits_for=lambda item: item.waits_for,
        resource=lambda item: item.resource,
        cost=lambda item: item.cycles,
        capacity=8,
        resources=4,
        no_dependency=255,
    )
    retired = scheduled.reorder(
        key=lambda item: item.sequence,
        capacity=8,
    )
    credited = right_ready.credit(
        cost=lambda item: item.cycles,
        credits=4,
    )
    ac.expect(
        retired,
        predicate=lambda item: item.cycles > 0,
        message="cycles must be positive",
    )
    ac.observe(credited)
    ac.sink(credited)
    return retired
"""


class QueueStatementHandlerTest(unittest.TestCase):
    def test_migrated_handlers_preserve_binding_and_source_order(self) -> None:
        from agentic_circuit._queue_frontend import parse_queue_program

        program = parse_queue_program(STATEMENT_HANDLER_SOURCE, "pipeline")
        bindings = (
            program.forks
            + program.barriers
            + program.routes
            + program.merges
            + program.dependencies
            + program.reorders
            + program.credits
            + program.expectations
            + program.observations
            + program.sinks
        )

        self.assertEqual(
            [
                "ForkBinding",
                "BarrierBinding",
                "RouteBinding",
                "MergeBinding",
                "DependencyBinding",
                "ReorderBinding",
                "CreditBinding",
                "ExpectBinding",
                "ObservationBinding",
                "SinkBinding",
                "SinkBinding",
            ],
            [type(binding).__name__ for binding in bindings],
        )
        self.assertEqual(list(range(1, 12)), [binding.order for binding in bindings])
        self.assertEqual("credited", program.sinks[0].queue)
        self.assertEqual("retired", program.sinks[1].queue)

    def test_migrated_handlers_generate_deterministic_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        first = lower_queue_source(STATEMENT_HANDLER_SOURCE, "pipeline")
        second = lower_queue_source(STATEMENT_HANDLER_SOURCE, "pipeline")

        self.assertEqual(first, second)
        for operation in (
            "ac.fork",
            "ac.barrier",
            "ac.route",
            "ac.merge",
            "ac.dependency",
            "ac.reorder",
            "ac.credit",
            "ac.expect",
            "ac.observe",
        ):
            with self.subTest(operation=operation):
                self.assertIn(operation, first)
        self.assertEqual(2, first.count("ac.sink"))


if __name__ == "__main__":
    unittest.main()

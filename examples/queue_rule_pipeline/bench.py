# ruff: noqa: F823 -- captured module-owned state proposals.
"""Four packets, output backpressure, and a complete drain in twelve cycles."""

import pycircuit as ac
from example_queue_rule_pipeline.queue_rule_pipeline import Packet, QueueRulePipeline


@ac.system
def QueueRulePipelineSystem():  # noqa: N802
    pipeline = QueueRulePipeline()
    cycle: ac.u4 = 0
    sent: ac.u4 = 0
    received: ac.u4 = 0

    @ac.rule
    def produce() -> None:
        if sent < 4:
            pipeline.put(Packet(tag=sent, value=sent + 10))
            sent = sent + 1

    @ac.rule(guard=lambda: cycle >= 6)
    def consume() -> Packet:
        packet = pipeline.get()
        received = received + 1
        return packet

    @ac.rule
    def check(packet: Packet) -> None:
        expected: ac.u8 = (
            12
            if received == 0
            else 25 if received == 1 else 39 if received == 2 else 54
        )
        assert packet.tag == received, "queue pipeline: packet order"
        assert packet.value == expected, "queue pipeline: accumulated payload"
        ac.log("info", "queue received", cycle, packet.tag, packet.value)

    @ac.rule
    def inspect() -> None:
        total, transferred = pipeline.snapshot()
        ac.log("info", "queue committed", cycle, sent, received, total, transferred)
        if cycle == 5:
            assert sent == 2, "queue pipeline: producer held by backpressure"
            assert received == 0, "queue pipeline: consumer paused"
            assert total == 12, "queue pipeline: blocked child state held"
            assert transferred == 1, "queue pipeline: blocked parent state held"
        if cycle == 10:
            assert sent == 4, "queue pipeline: all inputs accepted"
            assert received == 4, "queue pipeline: all outputs consumed"
            assert total == 54, "queue pipeline: final child state"
            assert transferred == 4, "queue pipeline: exactly four transfers"

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1

    produce()
    packet = consume()
    check(packet)
    inspect()
    tick()

"""Contract model: all transaction admission uses the previous cycle occupancy."""

from collections import Counter, deque


def expected_trace(mode, cycles):
    expected = Counter()
    queues = [deque() for _ in range(4)]
    sent = left_count = right_count = total = accepted = 0
    sent_right = 0
    stored = 100
    blocked = False
    independent_progress = False
    left_values, right_values = [], []
    for cycle in range(cycles):
        occupancy = [len(queue) for queue in queues]
        events = []
        if mode == "composition":
            send = sent < 4 and occupancy[0] < 2
            split = occupancy[0] > 0 and occupancy[1] < 2 and occupancy[2] < 2
            left = occupancy[1] > 0
            right = cycle >= 10 and occupancy[2] > 0
            events += [
                ("progress", (cycle, sent, left_count, right_count)),
                ("accepted", (accepted,)),
                ("counters", (total, 0)),
            ]
            if send:
                events.append(("send", (sent + 1,)))
            if split:
                value = queues[0][0]
                events.append(("split", (value, total + value)))
            if left:
                value_left = queues[1].popleft()
                left_values.append(value_left)
                events.append(("left", (value_left,)))
                left_count += 1
            if right:
                value_right = queues[2].popleft()
                right_values.append(value_right)
                events.append(("right", (value_right,)))
                right_count += 1
            blocked |= occupancy[0] > 0 and not split
            if split:
                value = queues[0].popleft()
                queues[1].append(value + 10)
                queues[2].append(value + 20)
                total += value
                accepted += 1
            if send:
                sent += 1
                queues[0].append(sent)
        elif mode == "mixed":
            value = cycle + 1
            first = occupancy[0] < 2
            second = occupancy[1] < 2 and occupancy[2] < 2
            events.append(("mixed", (cycle, value, stored, stored, stored)))
            if cycle >= 6 and occupancy[0]:
                events.append(("first", (queues[0].popleft(),)))
            if occupancy[1]:
                events.append(("other", (queues[1].popleft(),)))
            if occupancy[2]:
                events.append(("second", (queues[2].popleft(),)))
            blocked |= not first
            independent_progress |= not first and second
            if first:
                stored = value + 10
                queues[0].append(value + 20)
            if second:
                queues[1].append(value + 10)
                queues[2].append(value + 20)
        else:
            assert mode == "separate"
            send_left = sent < 4 and occupancy[0] < 2
            send_right = sent_right < 4 and occupancy[1] < 2
            fire_left = occupancy[0] > 0 and occupancy[2] < 2
            fire_right = occupancy[1] > 0 and occupancy[3] < 2
            events.append(("progress", (cycle, sent, sent_right)))
            if cycle >= 10 and occupancy[2]:
                value = queues[2].popleft()
                left_values.append(value)
                events.append(("left", (value,)))
            if occupancy[3]:
                value = queues[3].popleft()
                right_values.append(value)
                events.append(("right", (value,)))
            blocked |= occupancy[0] > 0 and not fire_left
            independent_progress |= occupancy[0] > 0 and not fire_left and fire_right
            if fire_left:
                value = queues[0].popleft()
                events.append(("first_fire", (value,)))
                queues[2].append(value + 10)
            if fire_right:
                value = queues[1].popleft()
                events.append(("second_fire", (value,)))
                queues[3].append(value + 20)
            if send_left:
                sent += 1
                queues[0].append(sent)
            if send_right:
                sent_right += 1
                queues[1].append(sent_right)
        for epoch in (2 * cycle, 2 * cycle + 1):
            for event, values in events:
                expected[(epoch, event, values)] += 1
    assert blocked
    if mode != "composition":
        assert independent_progress
    if mode != "mixed":
        assert left_values == [11, 12, 13, 14]
        assert right_values == [21, 22, 23, 24]
    if mode == "composition":
        assert total == 10 and accepted == 4
    return expected

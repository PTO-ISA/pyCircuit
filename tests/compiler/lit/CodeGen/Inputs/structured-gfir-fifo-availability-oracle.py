"""Independent two-slot FIFO, ordered admission and committed-state oracle."""

# clk rst offer data early late take parent-enable
samples = [
    (0, 0, 1, "1", "x", "0", 0, "1"),
    (1, 0, 1, "1", "x", "0", 0, "1"),  # early X before empty and later false
    (1, 0, 1, "1", "0", "0", 0, "1"),  # raw push retries the discarded edge
    (1, 0, 0, "0", "x", "x", 0, "x"),  # held edge preserves old head
    (0, 0, 1, "0", "1", "x", 0, "1"),
    (1, 0, 1, "0", "1", "x", 0, "1"),  # post-pop fault discards staged count
    (1, 0, 1, "0", "1", "1", 0, "1"),  # full ingress replaces old head
    (0, 0, 0, "0", "1", "1", 0, "1"),
    (1, 0, 0, "0", "1", "1", 0, "1"),  # full egress blocks the whole Rule
    (0, 0, 0, "0", "z", "0", 0, "1"),
    (1, 0, 0, "0", "z", "0", 0, "1"),  # reached Z before known false
    (1, 0, 0, "0", "1", "0", 0, "1"),
    (0, 0, 1, "x", "1", "1", 1, "1"),
    (1, 0, 1, "x", "1", "1", 1, "1"),  # full egress replacement observes old 1
    (1, 0, 0, "0", "1", "x", 0, "x"),
    (0, 0, 0, "0", "1", "1", 1, "1"),
    (1, 0, 0, "0", "1", "1", 1, "1"),  # parent rejects available X
    (1, 0, 0, "0", "1", "1", 1, "0"),  # first false allows X payload transport
    (0, 0, 1, "z", "0", "1", 0, "1"),
    (1, 0, 1, "z", "0", "1", 0, "1"),
    (0, 0, 0, "0", "1", "1", 1, "1"),
    (1, 0, 0, "0", "1", "1", 1, "1"),  # parent rejects available Z
    (1, 0, 0, "0", "1", "1", 1, "0"),
    (0, 0, 1, "1", "0", "1", 1, "1"),
    (1, 0, 1, "1", "0", "1", 1, "1"),
    (0, 0, 0, "0", "1", "1", 0, "x"),
    (1, 0, 0, "0", "1", "1", 0, "x"),  # parent failure rolls back both owners
    (1, 0, 0, "0", "1", "1", 0, "1"),
    (0, 1, 1, "x", "x", "x", 1, "x"),
    (1, 1, 1, "x", "x", "x", 1, "x"),  # reset masks sampled guards
    (1, 0, 0, "0", "x", "x", 0, "x"),
    (0, 0, 0, "0", "1", "1", 0, "1"),
    (1, 0, 0, "0", "1", "1", 0, "1"),
    (0, 0, 0, "0", "1", "1", 0, "1"),
]
ingress = []
egress = []
history = 0
moves = seen = 0
for clock, reset, offer, data, early, late, take, consume in samples:
    present = bool(ingress)
    head = ingress[0] if present else "0"
    output_valid = bool(egress)
    output = egress[0] if output_valid else "0"
    room = not egress or bool(take)
    transfer = early == late == "1" and present and room
    ready = not ingress or transfer
    derived = head if head in ("0", "1") else "x"
    rising = bool(clock and not history)
    fault = False
    if rising and not reset:
        fault = early in ("x", "z") or consume in ("x", "z")
        fault |= early == "1" and present and late in ("x", "z")
        fault |= consume == "1" and transfer and derived == "x"
    print(
        clock,
        reset,
        offer,
        data,
        early,
        late,
        take,
        consume,
        int(present),
        head,
        int(transfer),
        head,
        int(transfer),
        derived,
        int(ready),
        int(present),
        head,
        int(output_valid),
        output,
        f"{moves:x}",
        f"{seen:x}",
        int(not fault),
    )
    if fault:
        continue
    if rising:
        if reset:
            ingress.clear()
            egress.clear()
            moves = seen = 0
        else:
            # All values and admission above observe old committed queues.
            if output_valid and take:
                egress.pop(0)
            if transfer:
                ingress.pop(0)
                egress.append(head)
                moves = (moves + 1) % 256
                if consume == "1" and derived == "1":
                    seen = (seen + 1) % 256
            if offer and ready:
                ingress.append(data)
    history = clock

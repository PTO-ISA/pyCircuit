"""Independent masked/read-first/response-age model for the M1 public cases."""

SAMPLES = ((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0),
           (0, 0), (1, 0), (0, 0), (1, 0), (0, 0), (1, 0), (0, 0), (1, 0), (0, 0))


def expected(name):
    _, profile, topology = name.split("-")
    assert profile in ("byte", "sync", "nominal", "generic")
    assert topology in ("ancestor", "sibling", "crossbranch", "repeated")
    synchronous = profile in ("sync", "nominal")
    nominal = profile == "nominal"
    seeds = (0x1234, 0x5678) if topology == "repeated" else (0x1234,)
    rows = []
    for lane, seed in enumerate(seeds):
        phase = ram = response = idle = previous_clock = 0
        live = False
        for epoch, (clock, reset) in enumerate(SAMPLES):
            enabled = not (synchronous and phase >= 1 and not live)
            request = enabled and synchronous and phase <= 2
            old_ram = ram
            if enabled:
                if phase == 0:
                    rows.append((epoch, lane, "memory-write", [seed]))
                else:
                    value = response if synchronous else ram
                    parts = [value >> 8, value & 255] if nominal else [value]
                    rows.append((epoch, lane, "memory-before" if phase == 1 else "memory-after",
                                 parts * 2 if phase == 1 else parts))
            if clock and not previous_clock:
                if reset:
                    phase, live, idle = 0, False, 0
                else:
                    if enabled:
                        if phase == 0:
                            ram = (ram & ~255) | ((188 if nominal else seed) & 255)
                        elif phase == 1:
                            ram = (ram & 255) | ((21 if nominal else 0xAB) << 8)
                        phase = (phase + 1) & 7
                    if request:
                        response, live, idle = old_ram, True, 0
                    elif live:
                        idle += 1
                        if idle == 2:
                            live = False
            previous_clock = clock
    rows.sort(key=lambda row: (row[0], row[1]))
    return [(epoch, event, [{"kind": "integer", "value": str(value)} for value in values])
            for epoch, _, event, values in rows]

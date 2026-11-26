"""Test-owned integer RAM, response lifetime and sampled-clock reference."""

# Independent integer model: no generated DUT or gfsim helpers supply results.
memory = [0, 0, 0]
response = [0, 0]
live = [False, False]
idle = [0, 0]
previous_clock = 0
count = 0
# Earlier unknown must fail even when no response is live; retry the same high.
samples = [
    (0, 0, 0, 0, 0, 0, 0, 0, 0, 3, "1"),
    (1, 0, 0, 0, 0, 0, 0, 0, 0, 3, "x"),
    (1, 0, 0, 0, 0, 0, 0, 0, 0, 3, "0"),
]


def cycle(
    req0=0,
    req1=0,
    write=0,
    addr0=0,
    addr1=0,
    waddr=0,
    data=0,
    mask=3,
    reset=0,
    held=False,
    consume="1",
):
    arguments = (reset, req0, req1, write, addr0, addr1, waddr, data, mask, consume)
    samples.extend([(0, *arguments), (1, *arguments)])
    if held:
        samples.append((1, *arguments))


cycle(write=1, data=0xABCD)  # initialize row zero
cycle(req0=1, req1=1, write=1, addr1=1, data=0x1234)  # old-data collision
cycle(req0=1, req1=1, addr1=1, held=True)  # distinct responses
cycle(req0=1, write=1, data=0x55EE, mask=1)  # low byte only
cycle(req0=1, write=1, data=0x9900, mask=2)  # high byte only
cycle(req0=1, req1=1, addr1=3)  # read OOB -> live zero
cycle()  # first idle edge holds Q
cycle(held=True)  # second idle expires Q
cycle(req0=1, req1=1, write=1, addr1=1, waddr=3, data=0xFFFF)
cycle(req0=1, req1=1, addr1=1, reset=1)  # reset invalidates, retains RAM
cycle(req0=1, req1=1, addr1=1)  # re-read retained row
cycle(req1=1, addr1=0)  # independent port aging
cycle(req1=1, addr1=0)  # only port zero expires
cycle(req0=1, req1=1, addr0=2, addr1=0)  # both ports recover
cycle(write=1, data=0x1111, consume="0")
cycle(req0=1, consume="0")  # make an odd response live before the failing sample
for clock, consume in [(0, "x"), (1, "x"), (1, "1"), (1, "1")]:
    samples.append((clock, 0, 1, 1, 1, 0, 0, 0, 0x2222, 3, consume))
cycle()  # first successful idle edge retains the response
cycle(held=True)  # second idle edge expires it only after Work

for (
    clock,
    reset,
    req0,
    req1,
    write,
    addr0,
    addr1,
    waddr,
    data,
    mask,
    consume,
) in samples:
    rising = bool(clock and not previous_clock)
    accepted = not (rising and not reset and consume in ("x", "z"))
    values = [clock, reset, req0, req1, write, addr0, addr1, waddr, data, mask]
    expected = [
        int(live[0]),
        response[0],
        int(live[1]),
        response[1],
        int(live[0]),
        response[0] & 1,
        count,
        int(accepted),
    ]
    print(
        *(f"{value:x}" for value in values),
        consume,
        *(f"{value:x}" for value in expected),
    )
    if not accepted:
        continue
    if rising:
        if reset:
            live = [False, False]
            idle = [0, 0]
            count = 0
        else:
            if consume == "1" and live[0] and response[0] & 1:
                count = (count + 1) % 256
            # Sample both reads from old memory before changing any byte.
            for port, (enabled, address) in enumerate([(req0, addr0), (req1, addr1)]):
                if enabled:
                    response[port] = memory[address] if address < len(memory) else 0
                    live[port] = True
                    idle[port] = 0
                elif live[port]:
                    idle[port] += 1
                    if idle[port] >= 2:
                        live[port] = False
            if write and waddr < len(memory):
                for byte in range(2):
                    if mask & (1 << byte):
                        lane = 0xFF << (8 * byte)
                        memory[waddr] = (memory[waddr] & ~lane) | (data & lane)
    previous_clock = clock

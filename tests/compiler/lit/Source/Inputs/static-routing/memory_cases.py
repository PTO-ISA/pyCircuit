"""Pure-Memory LCA fixtures; expected values are derived from Memory semantics."""

SAMPLES = ((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0),
           (0, 0), (1, 0), (0, 0), (1, 0), (0, 0), (1, 0), (0, 0), (1, 0), (0, 0))


def source(shape=2, profile="byte", repeated=False):
    assert shape in (0, 1, 2)
    assert profile in ("byte", "sync", "nominal", "generic")
    nominal = profile == "nominal"
    synchronous = profile in ("sync", "nominal")
    payload = "Packet" if nominal else "ac.u16"
    generic = profile == "generic"
    memory_type = "ac.bits[WIDTH]" if generic else payload
    method_type = "ac.u24" if generic else memory_type
    text = "import pycircuit as ac\n"
    if nominal:
        text += "@ac.struct\nclass Packet:\n    tag: ac.u5\n    byte: ac.u8\n"
    text += "\n@ac.module\ndef Store(" + ("*, WIDTH: int = 16" if generic else "") + "):\n"
    text += "    memory = ac." + ("sync_mem" if synchronous else "byte_mem") + "[" + memory_type + "](depth=4)\n"
    if generic:
        text += "    alias = memory\n"
    text += '''
    @ac.method
    def write_low(value: PAYLOAD) -> None:
        memory.write(1, value, mask=1)

    @ac.method
    def write_high(value: PAYLOAD) -> None:
        memory.write(1, value, mask=2)
'''.replace("PAYLOAD", method_type)
    if synchronous:
        text += '''
    @ac.method
    def request() -> None:
        memory.request(1)

    @ac.method
    def result() -> PAYLOAD:
        return memory.result()
'''.replace("PAYLOAD", method_type)
    else:
        text += '''
    @ac.method
    def read() -> PAYLOAD:
        return memory.read(1)
'''.replace("PAYLOAD", method_type)
    if shape == 0:
        text += "\n    worker = Worker(0x1234)\n"
    worker_type = "ac.u24" if generic else "ac.u16"
    text += "\n\n@ac.module\ndef Worker(seed: " + worker_type + "):\n"
    path = '".."' if shape == 0 else '"..", "store"' if shape == 1 else '"..", "..", "remote", "store"'
    text += "    store = ac.instance_ref(Store, " + path + ")\n    phase: ac.u3 = 0\n"
    text += '''
    @ac.rule
    def step() -> None:
        if phase == 0:
            store.write_low(LOW)
            REQUEST
            ac.log("info", "memory-write", seed)
        elif phase == 1:
            first = store.READ()
            second = store.READ()
            store.write_high(HIGH)
            REQUEST
            ac.log("info", "memory-before", VALUES)
        else:
            current = store.READ()
            LATER_REQUEST
            ac.log("info", "memory-after", CURRENT)
        phase = phase + 1

    step()
'''.replace("LOW", "Packet(tag=26, byte=188)" if nominal else "seed").replace(
        "HIGH", "Packet(tag=21, byte=68)" if nominal else "0xAB00").replace(
        "LATER_REQUEST", "if phase == 2:\n                store.request()" if synchronous else "pass").replace(
        "REQUEST", "store.request()" if synchronous else "pass").replace(
        "READ", "result" if synchronous else "read").replace(
        "VALUES", "first.tag, first.byte, second.tag, second.byte" if nominal else "first, second").replace(
        "CURRENT", "current.tag, current.byte" if nominal else "current")
    if shape == 2:
        text += '''

@ac.module
def Remote():
    store = Store()


@ac.module
def Branch(seed: ac.u16):
    worker = Worker(seed)
'''
    if shape == 0:
        body = "    store = Store()\n"
    elif shape == 1:
        body = "    store = Store()\n    worker = Worker(seed)\n"
    else:
        body = "    remote = Remote()\n    branch = Branch(seed)\n"
    if repeated:
        text += "\n\n@ac.module\ndef Pair(seed: ac.u16):\n" + body
        top_body = "    left = Pair(0x1234)\n    right = Pair(0x5678)\n"
    else:
        top_body = body.replace("seed)", "0x1234)")
    text += "\n\n@ac.system(samples=" + repr(SAMPLES) + ")\ndef Top():\n" + top_body
    if generic:
        text = text.replace("Store()", "Store(WIDTH=24)")
        text = text.replace("seed: ac.u16", "seed: ac.u24")
        text = text.replace("memory.write(1, value, mask=2)", "alias.write(1, value, mask=2)")
        text = text.replace("return memory.read(1)", "return alias.read(1)")
    return text

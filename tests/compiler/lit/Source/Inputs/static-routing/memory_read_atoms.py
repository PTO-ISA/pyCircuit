"""Two ordered covered reads with genuine equal-definition/different-owner flags."""


def source(shape=2, profile="byte", repeated=False):
    text = '''import pycircuit as ac

@ac.module
def Flags(seed: ac.u1) -> ac.u1:
    flag: ac.u1 = 0
    return flag

@ac.module
def Store():
    memory = ac.byte_mem[ac.u16](depth=4)
    @ac.method
    def read() -> ac.u16:
        return memory.read(1)

@ac.module
def Worker(a: ac.u1, b: ac.u1, store: Store):
    @ac.rule
    def step() -> None:
        ac.log("info", "atom-inputs", a, b)
        if a:
            pass
        else:
            first = store.read()
            second = store.read()
            ac.log("info", "atom-reads", first, second)

@ac.system(samples=((0, 0), (1, 0)))
def Top():
    left_flag = Flags(0)
    right_flag = Flags(0)
    store = Store()
    worker = Worker(left_flag, right_flag, store)
'''
    if profile == "raw":
        text = text.replace(
            "        if a:\n            pass\n        else:\n"
            "            first = store.read()\n            second = store.read()\n"
            "            ac.log(\"info\", \"atom-reads\", first, second)\n",
            "        if a:\n            first = store.read()\n            second = store.read()\n"
            "            ac.log(\"info\", \"atom-reads\", first, second)\n        else:\n            pass\n")
        text = text.replace("left_flag = Flags(0)", "left_flag = Flags(1)")
    return text

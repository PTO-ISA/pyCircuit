"""Repeated crossed Queue owners reuse the independent external-pop oracle."""

# ruff: noqa: F823, F841 -- captured Module state and real hardware allocations.
import pycircuit as ac
@ac.module
def Store():
    tokens = ac.queue[ac.u8](depth=1, latency=1)
    @ac.method
    def push(value: ac.u8) -> None:
        tokens.push(value)
    @ac.method
    def pop() -> ac.u8:
        return tokens.pop()
@ac.module
def Producer(value: ac.u8, store: Remote):
    @ac.rule
    def produce() -> None:
        store.push(value)
        ac.log("info", "external pop pushed", value)
@ac.module
def Consumer(allow: ac.u1, take: ac.u1, store: Remote):
    @ac.rule(guard=lambda: allow)
    def consume() -> None:
        if take:
            value = store.pop()
            ac.log("info", "external pop retired", value)
@ac.module
def Remote():
    store = Store()
    @ac.method
    def push(value: ac.u8) -> None:
        store.push(value)
    @ac.method
    def pop() -> ac.u8:
        return store.pop()
@ac.module
def Branch(allow: ac.u1, take: ac.u1, value: ac.u8, store: Remote):
    producer = Producer(value, store)
    consumer = Consumer(allow, take, store)
@ac.module
def CreditPair(value: ac.u8):
    allow: ac.u1 = 1
    take: ac.u1 = 0
    count: ac.u8 = 0
    remote = Remote()
    branch = Branch(allow, take, value, remote)
    @ac.rule
    def tick() -> None:
        count = count + 1
@ac.system
def Top():
    left = CreditPair(23)
    right = CreditPair(47)

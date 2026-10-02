"""Compiler-shaped wiring: plain Modules, member Rules, fixed IDs and bindings."""
from typing import NamedTuple

from construction import assemble
from engine import Queue, Signal, RuleEntry


class Config(NamedTuple):
    bank: int = 0
    enabled: bool = True
    bias: int = 0


class Sample(NamedTuple):
    valid: bool = False
    bank: int = 0
    sequence: int = 0
    value: int = 0


class Combinational:
    def __init__(self, config, banks):
        self.config, self.banks = config, banks

    def enabled(self):
        return self.config.peek().enabled

    def selected(self):
        config = self.config.peek()
        if not config.enabled:
            return Sample()
        item = self.banks[config.bank].try_peek()
        if item is None:
            return Sample(False, config.bank)
        sequence, value = item
        return Sample(True, config.bank, sequence, value + config.bias)


class Source:
    def __init__(self, rid, words, position, output):
        self.rid, self.words, self.position, self.output = rid, words, position, output

    def Work(self):
        self.send()

    def send(self):
        e, r = self.engine, self.rid
        if not e.begin_rule(r):
            return
        index = self.position.peek()
        if index < len(self.words):
            self.output.propose_push(r, (index, self.words[index]))
            self.position.propose_revise(r, index + 1)
        e.complete_rule(r)

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)


class Controller:
    """A clocked configuration writer; the immutable script is the system input."""
    def __init__(self, rid, config, script, cycles):
        self.rid, self.config, self.script, self.cycles = rid, config, script, cycles

    def Work(self):
        self.configure(self.engine.tick)

    def configure(self, tick):
        e, r = self.engine, self.rid
        if not e.begin_rule(r, (tick,)):
            return
        for when, value in self.script:
            if when == tick:
                self.config.propose_revise(r, value)
                break
        if tick + 1 < self.cycles:
            e.request_wakeup(r, self.mid, 1)
        e.complete_rule(r)

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)


class Processor:
    def __init__(self, rid, enabled, selected, banks, output):
        self.rid, self.enabled, self.selected = rid, enabled, selected
        self.banks, self.output = banks, output

    def Work(self):
        if self.enabled.value:  # Module control subscription.
            self.transfer()

    def transfer(self):
        e, r = self.engine, self.rid
        if not e.begin_rule(r):
            return
        value = self.selected.value  # Rule subscription, no args required.
        if value.valid:
            self.banks[value.bank].propose_pop(r)
            self.output.propose_push(r, value)
        e.complete_rule(r)

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)


class Sink:
    def __init__(self, rid, timer_rid, input_queue, retired, cycles, interval):
        self.rid, self.timer_rid, self.input, self.retired = rid, timer_rid, input_queue, retired
        self.cycles, self.interval = cycles, interval

    def Work(self):
        tick = self.engine.tick
        if tick >= 5 and (tick - 5) % self.interval == 0:
            self.receive()
        self.timer(tick)

    def receive(self):
        e, r = self.engine, self.rid
        if not e.begin_rule(r):
            return
        value = self.input.try_peek()
        if value is not None:
            self.input.propose_pop(r)
            count, _ = self.retired.peek()
            self.retired.propose_revise(r, (count + 1, value))
        e.complete_rule(r)

    def timer(self, tick):
        e, r = self.engine, self.timer_rid
        if not e.begin_rule(r, (tick,)):
            return
        if tick + 1 < self.cycles:
            e.request_wakeup(r, self.mid, 1)
        e.complete_rule(r)

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)

    def arbitrate_timer(self):
        return self.engine.arbitrate_rule(self.timer_rid)


def workload(cycles=120, frequent=False):
    words = (tuple(range(10, 18)), tuple(range(30, 38)))
    script = [(2, Config(0, True, 100)), (3, Config(0, False, 100)),
              (4, Config(1, True, 0)), (6, Config(0, True, 0))]
    for tick in range(8, cycles - 10, 1 if frequent else 6):
        script.append((tick, Config((tick // 6) % 2, True, tick % 3)))
    return words, tuple(script)


def build(*, words=None, script=None, cycles=120, interval=4, cache=True, reverse=False):
    default_words, default_script = workload(cycles)
    words = default_words if words is None else words
    script = default_script if script is None else script
    banks = (Queue(), Queue())
    positions = (Queue(initial=(0,)), Queue(initial=(0,)))
    config, output, retired = Queue(initial=(Config(),)), Queue(), Queue(initial=((0, Sample()),))
    queues = list(banks + positions + (config, output, retired))
    for qid, q in enumerate(queues):
        q.qid = qid
    helpers = Combinational(config, banks)
    enabled, selected = Signal(helpers.enabled), Signal(helpers.selected)
    signals = [enabled, selected]
    source0, source1 = (Source(i + 1, words[i], positions[i], banks[i]) for i in range(2))
    controller = Controller(3, config, script, cycles)
    processor = Processor(4, enabled, selected, banks, output)
    sink = Sink(5, 6, output, retired, cycles, interval)
    ordered = [source0, source1, controller, processor, sink]
    modules = list(reversed(ordered)) if reverse else ordered
    for mid, module in enumerate(modules):
        module.mid = mid
    entries = [None,
        RuleEntry(source0.mid, source0.send, source0.arbitrate, pushes=(0,), revises=(2,)),
        RuleEntry(source1.mid, source1.send, source1.arbitrate, pushes=(1,), revises=(3,)),
        RuleEntry(controller.mid, controller.configure, controller.arbitrate, revises=(4,)),
        RuleEntry(processor.mid, processor.transfer, processor.arbitrate, pops=(0, 1), pushes=(5,)),
        RuleEntry(sink.mid, sink.receive, sink.arbitrate, pops=(5,), revises=(6,)),
        RuleEntry(sink.mid, sink.timer, sink.arbitrate_timer)]
    qbindings = [(0, 2), (1, 3), (4,), (0, 1, 5), (5, 6)]
    module_queues, module_signals = [None] * 5, [()] * 5
    for module, qids in zip(ordered, qbindings):
        module_queues[module.mid] = qids
    module_signals[processor.mid] = (0, 1)
    sim = assemble(queues, modules, entries, cache, module_queues=module_queues,
                   signals=signals, module_signals=module_signals, signal_queues=[(4,), (0, 1, 4)])
    return sim

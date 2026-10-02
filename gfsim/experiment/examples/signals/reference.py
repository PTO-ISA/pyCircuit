"""Independent clocked model: no runtime, proposals, cached Signals or subscriptions."""
from .model import Config, Sample


class Reference:
    def __init__(self, words, script, interval=4):
        self.words, self.script, self.interval = words, script, interval
        self.banks, self.positions, self.output = [[], []], [0, 0], []
        self.config, self.retired = Config(), (0, Sample())
        self.tick = 0

    def signals(self):
        c = self.config
        if not c.enabled:
            value = Sample()
        elif not self.banks[c.bank]:
            value = Sample(False, c.bank)
        else:
            sequence, payload = self.banks[c.bank][0]
            value = Sample(True, c.bank, sequence, payload + c.bias)
        return c.enabled, value

    def step(self):
        c, tick = self.config, self.tick
        enabled, value = self.signals()
        receive = bool(self.output) and tick >= 5 and (tick - 5) % self.interval == 0
        transfer = enabled and value.valid and (not self.output or receive)
        sends = [self.positions[i] < len(self.words[i]) and
                 (not self.banks[i] or (transfer and c.bank == i)) for i in range(2)]
        if receive:
            self.retired = (self.retired[0] + 1, self.output.pop(0))
        if transfer:
            self.output.append(value)
            self.banks[c.bank].pop(0)
        for i in range(2):
            if sends[i]:
                index = self.positions[i]
                self.banks[i].append((index, self.words[i][index]))
                self.positions[i] += 1
        for when, config in self.script:
            if when == tick:
                self.config = config
                break
        self.tick += 1
        return tuple(tuple(q) for q in self.banks) + tuple((p,) for p in self.positions) + (
            (self.config,), tuple(self.output), (self.retired,))

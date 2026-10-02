"""Fetch: explicit generated-style Rule and runtime calls."""

from .records import Fetched, u32


class Fetch:
    def __init__(self, mid, rid, rom, pc, control, redirect, output):
        self.mid, self.rid, self.engine = mid, rid, None
        self.rom = rom
        self.pc = pc
        self.control = control
        self.redirect = redirect
        self.output = output

    def Work(self):
        self.work_fetch()

    def work_fetch(self):
        e, rid = self.engine, self.rid
        if not e.begin_rule(rid):
            return

        control = self.control.peek()
        if control.stopped:
            e.complete_rule(rid)
            return
        redirect = self.redirect.try_peek()
        pc = self.pc.peek()
        if redirect is not None:
            self.redirect.propose_pop(rid)
            pc = redirect.pc
        # Invalid wrong-path fetches must survive until an older branch resolves.
        word = self.rom[pc // 4] if pc % 4 == 0 and 0 <= pc // 4 < len(self.rom) else 0
        self.output.propose_push(rid, Fetched(pc, word, control.epoch))
        self.pc.propose_revise(rid, u32(pc + 4))

        e.complete_rule(rid)

    def arbitrate_fetch(self):
        return self.engine.arbitrate_rule(self.rid)

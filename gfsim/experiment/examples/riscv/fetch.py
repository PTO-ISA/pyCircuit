"""IF: sequential fetch, EX redirects, and a fixed instruction ROM."""

from .stage import Stage
from .records import Fetched, u32


class Fetch(Stage):
    def __init__(self, mid, rid, rom, pc, control, redirect, output):
        super().__init__(mid, rid)
        self.rom, self.pc, self.control = rom, pc, control
        self.redirect, self.output = redirect, output

    def work_stage(self):
        control = self.observe(self.control)
        if control.stopped:
            return True
        redirect = self.observe(self.redirect)
        pc = self.observe(self.pc)
        if redirect is not None:
            pc = self.take(self.redirect).pc
        # Invalid fetches travel as illegal instructions; wrong-path fetches must
        # not fail before an older branch has resolved.
        word = self.rom[pc // 4] if pc % 4 == 0 and 0 <= pc // 4 < len(self.rom) else 0
        self.output.propose_push(self.rid, Fetched(pc, word, control.epoch))
        self.pc.propose_revise(self.rid, u32(pc + 4))
        return True

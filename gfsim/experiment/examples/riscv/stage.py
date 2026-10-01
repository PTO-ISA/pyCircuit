"""Only transaction plumbing is shared. Stage bodies express CPU behavior."""

class Stage:
    def __init__(self, mid, rid):
        self.mid, self.rid, self.engine = mid, rid, None

    def arguments(self):
        return ()

    def Work(self):
        args = self.arguments()
        if not self.engine.begin_rule(self.rid, args):
            return
        # The generated Work entry returns completion, not a payload or an
        # arbitration decision. Queue effects still require later acceptance.
        if self.work_stage(*args):
            self.engine.complete_rule(self.rid)
        else:
            self.engine.abort_rule(self.rid)

    def observe(self, queue):
        self.engine.record_read(self.mid, queue.qid, self.rid)
        return queue.try_peek()

    def take(self, queue):
        value = self.observe(queue)
        if value is not None:
            queue.propose_pop(self.rid)
        return value

    def arbitrate(self):
        return self.engine.arbitrate_rule(self.rid)

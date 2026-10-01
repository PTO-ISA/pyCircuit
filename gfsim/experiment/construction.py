"""Mechanical construction standing in for compiler-emitted static tables."""

from engine import Proposal, Simulator


def assemble(queues, modules, rules, cache=True):
    if not rules or rules[0] is not None:
        raise ValueError("RuleId zero is reserved")
    sources = [set((0,)) for _ in queues]
    for rid, entry in enumerate(rules[1:], 1):
        for qid in entry.pops:
            queues[qid].pop_rule = rid
        for qid in entry.pushes:
            queues[qid].push_rule = rid
        for qid in entry.pops + entry.pushes + entry.revises:
            sources[qid].add(rid)
    sim = Simulator(queues, modules, rules, cache)
    for qid, queue in enumerate(queues):
        queue.qid, queue.engine = qid, sim
        queue.sources = sorted(sources[qid])
        queue.slots = [Proposal() for _ in queue.sources]
        queue.readers = [0] * len(modules)
    for mid, module in enumerate(modules):
        if module.mid != mid:
            raise ValueError("ModuleId must match static table index")
        module.engine = sim
    return sim

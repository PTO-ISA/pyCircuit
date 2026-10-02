"""Mechanical construction standing in for compiler-emitted static tables."""

from engine import Proposal, Simulator


def assemble(queues, modules, rules, cache=True, *, module_queues=None):
    if not rules or rules[0] is not None:
        raise ValueError("RuleId zero is reserved")
    if module_queues is None:
        module_queues = [range(len(queues)) for _ in modules]
    if len(module_queues) != len(modules):
        raise ValueError("one resource declaration per Module required")
    resources = [tuple(sorted(set(qids))) for qids in module_queues]
    if any(qid < 0 or qid >= len(queues) for qids in resources for qid in qids):
        raise ValueError("unknown Queue in Module resources")
    local_rules = [[] for _ in modules]
    sources = [set((0,)) for _ in queues]
    for rid, entry in enumerate(rules[1:], 1):
        if not 0 <= entry.module_id < len(modules):
            raise ValueError("unknown Rule owner")
        local_rules[entry.module_id].append(rid)
        if any(qid not in resources[entry.module_id]
               for qid in entry.pops + entry.pushes + entry.revises):
            raise ValueError("Rule modifies undeclared Module Queue")
        for qid in entry.pops:
            queues[qid].pop_rule = rid
        for qid in entry.pushes:
            queues[qid].push_rule = rid
        for qid in entry.pops + entry.pushes + entry.revises:
            sources[qid].add(rid)
    sim = Simulator(queues, modules, rules, cache)
    for mid, module in enumerate(sim.modules):
        module.resource_qids = resources[mid]
        module.rule_ids = tuple(local_rules[mid])
        module.word_count = (len(module.rule_ids) + 63) // 64
        module.control_reads = [0] * len(module.resource_qids)
        module.rule_readers = [0] * (len(module.resource_qids) * module.word_count)
        module.dirty_words = [0] * module.word_count
        for local, rid in enumerate(module.rule_ids):
            sim.rules[rid].word_index, offset = divmod(local, 64)
            sim.rules[rid].bit = 1 << offset
    for qid, queue in enumerate(queues):
        queue.qid, queue.engine = qid, sim
        queue.sources = sorted(sources[qid])
        queue.slots = [Proposal() for _ in queue.sources]
        queue.readers, queue.reader_slots = [], []
        queue.module_slots = [-1] * len(modules)
    for mid, qids in enumerate(resources):
        for slot, qid in enumerate(qids):
            queues[qid].readers.append(mid)
            queues[qid].reader_slots.append(slot)
            queues[qid].module_slots[mid] = slot
    for mid, module in enumerate(modules):
        if module.mid != mid:
            raise ValueError("ModuleId must match static table index")
        module.engine = sim
    return sim

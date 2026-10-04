"""Mechanical construction standing in for compiler-emitted static tables."""

from engine import Proposal, Simulator


def assemble(queues, modules, rules, cache=True, *, module_queues=None,
             signals=(), module_signals=None, signal_queues=None, rule_signals=None):
    if not rules or rules[0] is not None:
        raise ValueError("RuleId zero is reserved")
    if module_queues is None:
        module_queues = [range(len(queues)) for _ in modules]
    if len(module_queues) != len(modules):
        raise ValueError("one resource declaration per Module required")
    queue_resources = [tuple(sorted(set(qids))) for qids in module_queues]
    if any(qid < 0 or qid >= len(queues) for qids in queue_resources for qid in qids):
        raise ValueError("unknown Queue in Module resources")
    if signals and (module_signals is None or signal_queues is None):
        raise ValueError("Signal use requires module_signals and signal_queues")
    module_signals = module_signals if module_signals is not None else [() for _ in modules]
    signal_queues = signal_queues if signal_queues is not None else []
    if len(module_signals) != len(modules) or len(signal_queues) != len(signals):
        raise ValueError("one resource declaration per Module and Signal required")
    if any(sid < 0 or sid >= len(signals) for sids in module_signals for sid in sids):
        raise ValueError("unknown Signal in Module resources")
    rule_signals = rule_signals if rule_signals is not None else [() for _ in rules]
    if len(rule_signals) != len(rules) or rule_signals[0]:
        raise ValueError("one Signal declaration per Rule required; zero is reserved")
    if any(sid < 0 or sid >= len(signals) for sids in rule_signals for sid in sids):
        raise ValueError("unknown Signal in Rule inputs")
    inputs = [tuple(sorted(set(qids))) for qids in signal_queues]
    if any(qid < 0 or qid >= len(queues) for qids in inputs for qid in qids):
        raise ValueError("unknown Queue in Signal inputs")
    objects = list(queues) + list(signals)
    if len({id(obj) for obj in objects}) != len(objects):
        raise ValueError("duplicate resource instance; aliases must share one ID")
    resources = queue_resources
    local_rules = [[] for _ in modules]
    sources = [set((0,)) for _ in queues]
    for rid, entry in enumerate(rules[1:], 1):
        if not 0 <= entry.module_id < len(modules):
            raise ValueError("unknown Rule owner")
        local_rules[entry.module_id].append(rid)
        if any(qid not in queue_resources[entry.module_id]
               for qid in entry.pops + entry.pushes + entry.revises):
            raise ValueError("Rule modifies undeclared Module Queue")
        for qid in entry.pops:
            queues[qid].pop_rule = rid
        for qid in entry.pushes:
            queues[qid].push_rule = rid
        for qid in entry.pops + entry.pushes + entry.revises:
            sources[qid].add(rid)
    sim = Simulator(queues, modules, rules, cache, signals=signals)
    for mid, module in enumerate(sim.modules):
        module.resource_ids = resources[mid]
        module.rule_ids = tuple(local_rules[mid])
        module.word_count = (len(module.rule_ids) + 63) // 64
        module.control_reads = [0] * len(module.resource_ids)
        module.rule_readers = [0] * (len(module.resource_ids) * module.word_count)
        module.dirty_words = [0] * module.word_count
        for local, rid in enumerate(module.rule_ids):
            sim.rules[rid].word_index, offset = divmod(local, 64)
            sim.rules[rid].bit = 1 << offset
    for qid, queue in enumerate(queues):
        queue.qid = qid
        queue.signal_readers = []
        queue.sources = sorted(sources[qid])
        queue.slots = [Proposal() for _ in queue.sources]
    for resource_id, resource in enumerate(objects):
        resource.resource_id, resource.engine = resource_id, sim
        resource.readers, resource.reader_slots = [], []
        resource.module_slots = [-1] * len(modules) if resource_id < len(queues) else []
    for mid, resource_ids in enumerate(resources):
        for slot, resource_id in enumerate(resource_ids):
            resource = objects[resource_id]
            resource.readers.append(mid)
            resource.reader_slots.append(slot)
            resource.module_slots[mid] = slot
    for sid, signal in enumerate(signals):
        signal.sid, signal.input_qids = sid, inputs[sid]
        signal.dependents = {mid: [] for mid, sids in enumerate(module_signals) if sid in sids}
        for qid in signal.input_qids:
            queues[qid].signal_readers.append(sid)
    for rid, sids in enumerate(rule_signals[1:], 1):
        mid, record = rules[rid].module_id, sim.rules[rid]
        for sid in set(sids):
            mask = signals[sid].dependents.get(mid)
            if not mask:
                mask = signals[sid].dependents[mid] = [0] * sim.modules[mid].word_count
            mask[record.word_index] |= record.bit
    for signal in signals:
        signal.dependents = {mid: tuple(mask) for mid, mask in sorted(signal.dependents.items())}
    for mid, module in enumerate(modules):
        if module.mid != mid:
            raise ValueError("ModuleId must match static table index")
        module.engine = sim
    return sim

"""Before simulation: construct the tables/slots that a compiler would emit."""

from heapq import heappop, heappush

from engine import ModuleRecord, ProposalSlot, RuleRecord, Simulator, StaticTables


def assemble(queues, modules, rules, reference=False):
    """IDs are explicit: sequence indices; rules[0] must be None."""
    rules = tuple(rules)
    if not rules or rules[0] is not None or any(m.mid != i for i, m in enumerate(modules)):
        raise ValueError("invalid fixed ID layout")
    permissions = [{} for _ in queues]
    producers = [[] for _ in queues]
    for rid, entry in enumerate(rules[1:], 1):
        if not 0 <= entry.module_id < len(modules):
            raise ValueError("invalid Rule owner")
        for op, qids in (("pop", entry.pops), ("push", entry.pushes), ("revise", entry.revises)):
            for qid in qids:
                if type(qid) is not int or not 0 <= qid < len(queues):
                    raise ValueError("invalid QueueId")
                permissions[qid].setdefault(rid, set()).add(op)
        for qid in set(entry.pushes):
            producers[qid].append(rid)
    edges, indegree = [set() for _ in rules], [0] * len(rules)
    for rid, entry in enumerate(rules[1:], 1):
        for qid in entry.pops:
            for producer in producers[qid]:
                if producer != rid and producer not in edges[rid]:
                    edges[rid].add(producer)
                    indegree[producer] += 1
    ready, order = [], []
    for rid in range(1, len(rules)):
        if indegree[rid] == 0:
            heappush(ready, rid)
    while ready:
        rid = heappop(ready)
        order.append(rid)
        for producer in edges[rid]:
            indegree[producer] -= 1
            if indegree[producer] == 0:
                heappush(ready, producer)
    if len(order) != len(rules) - 1:
        raise ValueError("cyclic static capacity dependencies are unsupported")
    rank = [-1] * len(rules)
    for priority, rid in enumerate(order):
        rank[rid] = priority
    for queue, sources in zip(queues, permissions):
        queue.source_index = {rid: i for i, rid in enumerate(sources)}
        queue.allowed_ops = tuple(frozenset(ops) for ops in sources.values())
        queue.slots = [ProposalSlot() for _ in sources]
    tables = StaticTables(tuple(m.Work for m in modules), rules,
                          tuple(tuple(p) for p in producers), tuple(rank))
    sim = Simulator(queues, tables, [ModuleRecord() for _ in modules],
                    [None] + [RuleRecord() for _ in rules[1:]], reference)
    for module in modules:
        module.engine = sim
    return sim

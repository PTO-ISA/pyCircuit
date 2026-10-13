"""Allocate visibility stages from declarations, never from a chosen rule order."""
from collections import defaultdict
from .ir import CompileError


def allocate(records, orders, resources, array_sizes, contracts):
    groups = defaultdict(list)
    for record in records:
        if record['view'] == 'ehr':
            groups[record['resource'], record.get('aspect', 'value')].append(record)
    for resource in resources.values():
        name = resource['name']
        if resource.get('visibility') == 'ehr': groups[name, 'value']
        if resource['kind'] in ('queue', 'qarray'):
            names = [name + '[' + str(i) + ']' for i in range(array_sizes[name])] if resource['kind'] == 'qarray' else [name]
            for identity in names: groups[identity, 'full']
    views, ports = [], {}
    for (resource, aspect), accesses in sorted(groups.items()):
        if aspect == 'full':
            declaration = contracts[resources[resource.split('[')[0]]['contract']]['views'][aspect]
            bindings = {'Queue:' + name: index for index, name in enumerate(declaration['stages'])}
            count = len(bindings)
        else:
            phases = sorted({a['phase'] for a in accesses if a['phase'] != '$direct'})
            for access in accesses:
                if access['operation'] != 'write': continue
                for context in access['phase_context'][-1:]:
                    witness = orders[context['contract']].get((context['interface'], context['interface']))
                    if witness:
                        raise CompileError(f'EHR stage cycle for {resource}: method={access["phase"]}; orders={witness}')
            parent = {p: p for p in phases}
            def root(p):
                while parent[p] != p:
                    parent[p] = parent[parent[p]]
                    p = parent[p]
                return p
            strict, equal = [], []
            for index, first in enumerate(accesses):
                for second in accesses[index + 1:]:
                    a, b = first['phase'], second['phase']
                    if '$direct' in (a, b) or a == b or 'write' not in (first['operation'], second['operation']):
                        continue
                    forward, backward = [], []
                    # Nested methods own their visibility. Ancestors only lift
                    # transaction dependencies in Analyzer.method_orders.
                    for left in first['phase_context'][-1:]:
                        for right in second['phase_context'][-1:]:
                            if left['contract'] != right['contract']: continue
                            table = orders[left['contract']]
                            forward.extend(table.get((left['interface'], right['interface']), []))
                            backward.extend(table.get((right['interface'], left['interface']), []))
                    if forward: strict.append((a, b, forward))
                    if backward: strict.append((b, a, backward))
                    if not forward and not backward:
                        equal.append((a, b, [first['loc'], second['loc']]))
                        parent[root(b)] = root(a)
            adjacency = defaultdict(set)
            evidence = []
            for a, b, witness in strict:
                evidence.append(dict(before=a, after=b, provenance=witness))
                if root(a) == root(b):
                    raise CompileError(f'EHR stage constraints conflict for {resource}: {a} SB {b}, but unordered accesses require the same port; orders={evidence}; same-stage={equal}')
                adjacency[root(a)].add(root(b))
            rank, active = {}, set()
            def stage(p):
                if p in active:
                    raise CompileError(f'EHR stage cycle for {resource}: methods={phases}; orders={evidence}')
                if p not in rank:
                    active.add(p)
                    predecessors = [q for q in set(map(root, phases)) if p in adjacency[q]]
                    rank[p] = max((stage(q) + 1 for q in predecessors), default=0)
                    active.remove(p)
                return rank[p]
            bindings = {p: stage(root(p)) for p in phases}
            bindings['$direct'] = 0
            count = max(bindings.values(), default=0) + 1
        for phase, port in bindings.items():
            ports[resource, aspect, phase] = port
        views.append(dict(resource=resource, aspect=aspect, port_count=count,
                          bindings=[dict(method=p, port=n) for p, n in sorted(bindings.items())],
                          semantics='last_effective_prior_port', fallback='old', same_port='invisible',
                          commit='queue.effects' if aspect == 'full' else 'transaction'))
    bindings = []
    for accesses in groups.values():
        for access in accesses:
            entry = dict(function=access['function'], identity=access['identity'], resource=access['resource'], aspect=access['aspect'], phase=access['phase'],
                         port=ports[access['resource'], access['aspect'], access['phase']])
            if entry not in bindings: bindings.append(entry)
    return dict(views=views, ports=ports, accesses=sorted(bindings, key=lambda a:(a['function'], str(a['identity']), a['resource'], a['aspect'])))

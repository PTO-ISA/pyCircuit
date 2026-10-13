"""Conditional transaction analysis over parsed MLIR SSA and CFG.

Formulas retain shared SSA predicates. Only constants, identical predicates and
component occupancy contracts are proved; bounded boolean expansion never
silently treats an inconclusive result as a real dependency cycle.
"""
from dataclasses import dataclass, field, replace
from collections import defaultdict
from functools import lru_cache
import hashlib
import json
from .ir import CompileError, VERSION
from .contracts import closure


def atom(name):
    return ('atom', name)


def neg(value):
    if isinstance(value, bool): return not value
    if value[0] == 'not': return value[1]
    if value[0] in ('and', 'or'):
        return combine('or' if value[0] == 'and' else 'and', *(neg(v) for v in value[1:]))
    return ('not', value)


def combine(kind, *values):
    identity = kind == 'and'
    items = set()
    for value in values:
        if value == (not identity): return not identity
        if value == identity: continue
        if isinstance(value, tuple) and value[0] == kind: items.update(value[1:])
        else: items.add(value)
    if any(neg(v) in items for v in items): return not identity
    if not items: return identity
    if len(items) == 1: return next(iter(items))
    opposite = 'or' if kind == 'and' else 'and'
    factors = [set(v[1:]) if isinstance(v, tuple) and v[0] == opposite else {v} for v in items]
    # Absorption and complementary branches keep repeated helper CFG merges
    # compact without losing predicates or introducing solver assumptions.
    for i, left in enumerate(factors):
        for right in factors[i + 1:]:
            if left <= right:
                return combine(kind, *(combine(opposite, *f) for f in factors if f != right))
            if right <= left:
                return combine(kind, *(combine(opposite, *f) for f in factors if f != left))
            a, b = left - right, right - left
            if len(a) == len(b) == 1 and neg(next(iter(a))) == next(iter(b)):
                return combine(kind, combine(opposite, *(left & right)),
                               *(combine(opposite, *f) for f in factors if f != left and f != right))
    # Under the outer connective, each sibling is already known true/false.
    # E.g. P || (!P && Q) reduces to P || Q.
    for item in items:
        if not isinstance(item, tuple) or item[0] != opposite: continue
        siblings = items - {item}
        reduced = [v for v in item[1:] if neg(v) not in siblings]
        if len(reduced) != len(item) - 1:
            return combine(kind, *siblings, combine(opposite, *reduced))
    common = set.intersection(*factors)
    if common:
        remainder = combine(kind, *(combine(opposite, *(part - common)) for part in factors))
        return combine(opposite, *common, remainder)
    return (kind, *sorted(items, key=repr))


def both(*values): return combine('and', *values)
def either(*values): return combine('or', *values)


def formula(value):
    if isinstance(value, bool): return value
    if value[0] == 'atom': return {'atom': value[1]}
    if value[0] == 'not': return {'not': formula(value[1])}
    return {value[0]: [formula(v) for v in value[1:]]}


def display(value):
    if isinstance(value, bool): return str(value).lower()
    if value[0] == 'atom': return value[1]
    if value[0] == 'not': return '!' + display(value[1])
    return '(' + (' && ' if value[0] == 'and' else ' || ').join(display(v) for v in value[1:]) + ')'


def cubes(value, invert=False, limit=256):
    """Small boolean proof, with a hard limit and an explicit unknown result."""
    if isinstance(value, bool): return [frozenset()] if value != invert else []
    if value[0] == 'not': return cubes(value[1], not invert, limit)
    if value[0] == 'atom': return [frozenset([(value[1], not invert)])]
    conjunct = (value[0] == 'and') != invert
    result = [frozenset()] if conjunct else []
    for part in value[1:]:
        other = cubes(part, invert, limit)
        if other is None: return None
        if not conjunct: result.extend(other)
        else:
            merged = []
            for a in result:
                for b in other:
                    c = a | b
                    if any((name, not polarity) in c for name, polarity in c): continue
                    if any(polarity and name.startswith('empty(') and ('full(' + name[6:], True) in c for name, polarity in c): continue
                    merged.append(c)
                    if len(merged) > limit: return None
            result = merged
        result = list(set(result))
        if len(result) > limit: return None
    return result


def satisfiable(value):
    result = cubes(value)
    if result is None: return None
    return bool(result)


@lru_cache(maxsize=8192)
def simplify_path(value):
    """Compact bounded boolean paths without changing inconclusive formulas."""
    if isinstance(value, bool): return value
    if satisfiable(neg(value)) is False: return True
    terms = cubes(value)
    if terms is None: return value
    if not terms: return False
    # Merge complementary cubes and discard covered cubes. Keep the original
    # factored formula when expansion would make its representation larger.
    terms = set(terms)
    while True:
        terms = {term for term in terms if not any(other < term for other in terms)}
        ordered = sorted(terms, key=lambda term: sorted(term))
        merged = None
        for i, first in enumerate(ordered):
            for second in ordered[i + 1:]:
                a, b = first - second, second - first
                if len(a) == len(b) == 1:
                    name, polarity = next(iter(a))
                    if (name, not polarity) in b:
                        merged = first, second, first & second
                        break
            if merged: break
        if not merged: break
        first, second, common = merged
        terms.difference_update((first, second))
        terms.add(common)
    reduced = either(*(both(*(atom(name) if polarity else neg(atom(name))
                             for name, polarity in term)) for term in terms))
    if len(repr(reduced)) < len(repr(value)): return reduced
    return value


@lru_cache(maxsize=8192)
def union_paths(first, second):
    """Keep demand unions compact when a later use is already covered.

    The bounded boolean proof may be inconclusive. In that case retain both
    paths, just as in the dependency and cycle analysis.
    """
    result = simplify_path(either(first, second))
    if result in (first, second): return result
    if satisfiable(both(second, neg(first))) is False: return first
    if satisfiable(both(first, neg(second))) is False: return second
    return result


def fingerprint(value):
    return hashlib.sha256(repr(value).encode()).hexdigest()[:16]


@dataclass(frozen=True)
class Data:
    expr: object = None
    const: object = None
    refs: frozenset = field(default_factory=frozenset)
    deps: frozenset = field(default_factory=frozenset)
    consume: bool = False
    items: tuple = ()
    origins: tuple = ()
    valid: object = True


def merge_data(parts, boolean=False):
    if not parts: return Data()
    values = [v for _, v in parts]
    first = values[0]
    expression = first.expr if all(v.expr == first.expr for v in values) else (
        either(*(both(path, v.expr) for path, v in parts)) if boolean else atom('ssa:phi:' + fingerprint([(p, v.expr) for p, v in parts])))
    return Data(expression, first.const if all(v.const == first.const for v in values) else None,
                frozenset().union(*(v.refs for v in values)), frozenset().union(*(v.deps for v in values)),
                any(v.consume for v in values), first.items if all(v.items == first.items for v in values) else (),
                tuple((ref, either(*(both(path, dict(v.origins).get(ref, True)) for path,v in parts if ref in v.refs))) for ref in sorted(set().union(*(v.refs for v in values)))),
                simplify_path(either(*(both(path, v.valid) for path, v in parts))))


class Analyzer:
    def __init__(self, program, allocation=None, collect_stages=False):
        self.collect_stages = collect_stages
        self.allocation = allocation
        self.program = program
        self.functions = {f['name']: f for f in program['functions']}
        # Locations are diagnostic metadata, never SSA or evaluation-site identity.
        # Prefer source IDs; distinguish anonymous/generated and duplicate-ID ops
        # by their position in the actual CFG exported by the native compiler.
        self.operation_ids = {}
        for fn in program['functions']:
            counts = defaultdict(int)
            for block in fn['blocks']:
                for op in block['ops']:
                    counts[op['attrs'].get('acir.id')] += 1
            for block in fn['blocks']:
                for index, op in enumerate(block['ops']):
                    source = op['attrs'].get('acir.id')
                    self.operation_ids[id(op)] = ('source', source) if source and counts[source] == 1 else ('ir', block['label'], index)
        self.resources = {r['name']: r for r in program['resources']}
        self.nodes = {}
        self.records = {}
        self.values = {}
        self.stack = []
        self.work_done = set()
        self.edges = []
        self.diagnostics = []
        self.constants = {}
        self.array_sizes = {}
        self.selections = defaultdict(list)
        self.contracts = {c['id']: dict(c, relations=list(c['relations'])) for c in program['model']['contracts']}
        self.orders = {name: closure(c) for name, c in self.contracts.items()}
        self.method_records = {}
        self.method_stack = []
        self.guard_records = defaultdict(list)
        self.method_summaries = []
        self.preparations = {}
        self.preparation_uses = defaultdict(dict)

    @staticmethod
    def integer(value, typ):
        if typ == 'bool': return bool(value)
        if typ.startswith(('u', 'i')) and typ[1:].isdigit():
            width = int(typ[1:]); value %= 1 << width
            if typ.startswith('i') and value >= 1 << (width - 1): value -= 1 << width
        return value

    def occupancy(self, kind, resource): return atom(kind + '(' + resource + ')')

    def unknown(self, fn, op, args, context):
        # The SSA definition and its evaluation site are retained.
        key = (context, fn['name'], self.operation_ids[id(op)], op['op'], [v.expr for v in args])
        return atom('ssa:' + fingerprint(key))

    def resource_names(self, refs):
        names = set()
        for r in refs:
            if r in self.array_sizes:
                names.update(r + '[' + str(i) + ']' for i in range(self.array_sizes[r]))
            else: names.add(r)
        return sorted(names)

    def access(self, fn, op, value, path, owner, kind, context):
        output = set()
        for resource in self.resource_names(value.refs):
            key = fingerprint((owner, context, fn['name'], self.operation_ids[id(op)], resource, kind))
            record = self.records.setdefault(key, dict(id=key, transaction=owner, function=fn['name'], resource=resource,
                  operation=kind, condition=False, loc=op.get('loc', {}), role=op['attrs'].get('role', 'observe'),
                  view=op['attrs'].get('view', 'old'),
                  aspect=op['attrs'].get('aspect', 'value'), identity=op['attrs'].get('acir.id'), methods=list(self.method_stack),
                  phase=self.method_records[self.method_stack[-1]]['identity'] if self.method_stack else '$direct',
                  phase_context=[dict(contract=self.method_records[m]['contract'], interface=self.method_records[m]['interface']) for m in self.method_stack],
                  sequence=len(self.records),
                  contract=self.resources[resource.split('[')[0]]['contract'], interface=op['attrs'].get('interface', {'read': 'get', 'write': 'set'}.get(kind, kind) if self.resources[resource.split('[')[0]]['kind'] == 'signal' else kind),
                  binding='conservative' if resource.split('[')[0] in value.refs and self.resources[resource.split('[')[0]]['kind'] == 'qarray' else 'exact',
                  call_chain=list(self.stack)))
            origin = dict(value.origins).get(resource, dict(value.origins).get(resource.split('[')[0], True))
            record['condition'] = either(record['condition'], both(path, origin))
            record['local_condition'] = record['condition']
            if record['aspect'] == 'full':
                record['phase'] = 'Queue:' + record['interface']
                if kind == 'read': record['operation'] = 'query.full'
            if record['view'] == 'ehr':
                record['port'] = self.allocation['ports'].get((resource, record['aspect'], record['phase']), 0) if self.allocation else (1 if record['aspect'] == 'full' and kind == 'read' else 0)
            if kind == 'pop': record['locations'] = op['attrs'].get('consumption_locations', [op.get('loc', {})])
            record['ready'] = op['attrs'].get('readiness', 'true').replace('{resource}', resource)
            output.add(key)
        return frozenset(output)

    def bind_preparation_method(self, identity, owner, path):
        original = self.method_records[identity]
        bound = fingerprint((owner, identity))
        ancestors = [self.bind_preparation_method(a, owner, path) for a in original['ancestors']]
        entry = self.method_records.setdefault(bound, dict(original, id=bound, transaction=owner,
            source_method=identity, ancestors=ancestors, condition=False))
        entry['condition'] = either(entry['condition'], both(original['condition'], path))
        return bound

    def add_dependencies(self, owner, dependencies, path, reason):
        for key in sorted(dependencies):
            if key.startswith('preparation:'):
                if owner is None: continue
                region = key[len('preparation:'):]
                uses = self.preparation_uses[owner]
                uses[region] = union_paths(uses.get(region, False), path)
                for method in self.preparations[region]['method_dependencies']:
                    self.bind_preparation_method(method, owner, path)
            else:
                record = self.records[key]
                if record['transaction'] == owner: continue
                copied = dict(record, transaction=owner, condition=both(record['condition'], path), via=reason,
                              source_access=record.get('source_access', key))
                if owner and record['transaction'] is None:
                    copied['source_methods'] = record['methods']
                    copied['methods'] = [self.bind_preparation_method(m, owner, path) for m in record['methods']]
                # A shared access has one association per consuming Rule.
                # Multiple uses retain their reasons without cloning the read.
                copykey = fingerprint((owner, key))
                previous = self.records.get(copykey)
                if previous:
                    copied['condition'] = union_paths(previous['condition'], copied['condition'])
                    copied['via'] = previous['via']
                copied['vias'] = sorted(set(previous.get('vias', []) if previous else []) | {reason})
                copied['id'] = copykey
                self.records[copykey] = copied

    def run_work(self, name):
        if name in self.work_done: return
        if name in self.stack: raise CompileError('recursive shared computation dependency: ' + name)
        fn = self.functions.get(name)
        if fn is None: raise CompileError('unknown shared computation: ' + name)
        self.evaluate(fn, [], True, None, name)
        self.work_done.add(name)

    def evaluate(self, fn, arguments, selected, owner, context):
        if fn['name'] in self.stack: raise CompileError('recursive MLIR call: ' + ' -> '.join(self.stack + [fn['name']]))
        self.stack.append(fn['name'])
        blocks = {b['label']: b for b in fn['blocks']}
        if not blocks: raise CompileError('transaction function has no body')
        # Remove CFG backedges for a single conservative loop summary. The full
        # loop is preserved in MLIR; loop-carried values become opaque SSA atoms.
        visited, active, order, backedges = set(), set(), [], set()
        def visit(label):
            if label in visited: return
            visited.add(label); active.add(label)
            for op in blocks[label]['ops']:
                for dest in [op.get('dest'), op.get('yes'), op.get('no')]:
                    if dest is None: continue
                    if dest in active: backedges.add((label, dest))
                    else: visit(dest)
            active.remove(label); order.append(label)
        entry = fn['blocks'][0]['label']; visit(entry); order.reverse()
        def reachable(source, target):
            seen, pending = set(), [source]
            while pending:
                current = pending.pop()
                if current == target: return True
                if current in seen: continue
                seen.add(current)
                for op in blocks[current]['ops']:
                    pending.extend(d for d in (op.get('dest'), op.get('yes'), op.get('no')) if d)
            return False
        loop_blocks = {b for tail,head in backedges for b in blocks if reachable(head, b) and reachable(b, tail)}
        # End control provenance at reconvergence. A read used only by an
        # earlier branch must not become readiness for every later Rule.
        successors = {label: {d for o in b['ops'] for d in (o.get('dest'), o.get('yes'), o.get('no')) if d} or {'$exit'} for label,b in blocks.items()}
        post = {label: set(blocks) | {'$exit'} for label in blocks}
        post['$exit'] = {'$exit'}
        changed = True
        while changed:
            changed = False
            for label in blocks:
                updated = {label} | set.intersection(*(post[d] for d in successors[label]))
                if updated != post[label]: post[label] = updated; changed = True
        reconverges = {}
        for label in blocks:
            candidates = post[label] - {label}
            reconverges[label] = next((d for d in candidates if candidates - {d} <= post[d]), '$exit')
        control_sources = {}
        branch_conditions = {b['label']: o['args'][0] for b in fn['blocks'] for o in b['ops'] if o['op'] == 'cf.cond_br'}
        static_refs = defaultdict(set)
        for i, parameter in enumerate(blocks[entry]['params']):
            if i < len(arguments): static_refs[parameter['id']].update(arguments[i].refs)
        changed = True
        while changed:
            changed = False
            for block in fn['blocks']:
                for op in block['ops']:
                    sources = {op['attrs']['name']} if op['op'] == 'acir.get' else set().union(*(static_refs[a] for a in op['args']))
                    links = [(r['id'], sources) for r in op['results'] if r['type'].startswith(('queue<', 'cell<', 'ehr<', 'signal<', 'qarray<'))]
                    for destination, arguments_ids in [(op.get('dest'), op.get('dest_args', [])), (op.get('yes'), op.get('yes_args', [])), (op.get('no'), op.get('no_args', []))]:
                        if destination:
                            links.extend((p['id'], static_refs[a]) for p,a in zip(blocks[destination]['params'], arguments_ids) if p['type'].startswith(('queue<', 'cell<', 'ehr<', 'signal<', 'qarray<')))
                    for identifier, additions in links:
                        existing = static_refs[identifier]; count = len(existing)
                        existing.update(additions); changed |= len(existing) != count
        incoming = defaultdict(list)
        argsmeta = fn['meta'].get('params', [])
        args = {}
        for i, parameter in enumerate(blocks[entry]['params']):
            value = arguments[i] if i < len(arguments) else Data(atom('config:' + fn['name'] + ':' + str(i)))
            if i < len(argsmeta): value = replace(value, consume=argsmeta[i].get('consume', False))
            args[parameter['id']] = value
        incoming[entry].append((selected, args, frozenset(), {}))
        returns, observed, invocations = [], {}, []
        for label in order:
            if not incoming[label]: continue
            arrivals = incoming[label]
            path = simplify_path(either(*(p for p, _, _, _ in arrivals)))
            if self.collect_stages: path = True
            if satisfiable(path) is False: continue
            ids = set().union(*(env.keys() for _, env, _, _ in arrivals))
            env = {}
            types = {p['id']: p['type'] for p in blocks[label]['params']}
            for identifier in ids:
                parts = [(p, e[identifier]) for p, e, _, _ in arrivals if identifier in e]
                env[identifier] = merge_data(parts, types.get(identifier) == 'bool')
            observed.update(env)
            needed = frozenset().union(*(n for _, _, n, _ in arrivals))
            controls = {}
            for _, _, _, incoming_controls in arrivals:
                for source, dependencies in incoming_controls.items():
                    if reconverges[source] != label:
                        controls[source] = controls.get(source, frozenset()) | dependencies
            control_sources[label] = set(controls)
            control_deps = frozenset().union(*controls.values())
            # A backedge header parameter may change on later iterations.
            if any(dest == label for _, dest in backedges):
                for param in blocks[label]['params']:
                    old = env[param['id']]
                    refs = frozenset(static_refs[param['id']]) | old.refs
                    origins = tuple((ref, True) for ref in sorted(refs))
                    env[param['id']] = replace(old, expr=atom('ssa:loop:' + fn['name'] + ':' + param['id']), const=None, refs=refs, origins=origins)
            for op in blocks[label]['ops']:
                if satisfiable(path) is False and not self.collect_stages: break
                attrs = op['attrs']; kind = op['op']
                vals = [env.get(a, Data(atom('ssa:undefined:' + fn['name'] + ':' + a))) for a in op['args']]
                deps = frozenset().union(*(v.deps for v in vals))
                expression, constant, refs, consume, items, origins = self.unknown(fn, op, vals, context), None, frozenset(), False, (), ()
                value = None
                validity = both(path, *(v.valid for v in vals))
                if kind == 'arith.constant':
                    constant = self.integer(attrs['value'], op['results'][0]['type'])
                    expression = bool(constant) if op['results'][0]['type'] == 'bool' else atom('const:' + str(constant))
                elif kind == 'acir.config': expression = atom('config:' + attrs['name'])
                elif kind == 'acir.get':
                    if attrs['name'] not in self.resources: raise CompileError('unknown MLIR resource: ' + attrs['name'])
                    refs = frozenset([attrs['name']]); origins = ((attrs['name'], True),); expression = atom('resource:' + attrs['name'])
                elif kind == 'acir.export': value = vals[0]
                elif kind == 'acir.external':
                    key = (attrs['source'], attrs['identity'])
                    if key not in self.values: self.run_work(attrs['source'])
                    if key not in self.values: raise CompileError('unknown shared SSA identity: ' + str(key))
                    value = self.values[key]
                elif kind == 'acir.read':
                    access = self.access(fn, op, vals[0], path, owner, 'read', context)
                    deps |= access
                    if attrs.get('aspect') == 'full':
                        names = self.resource_names(vals[0].refs)
                        if len(names) == 1: expression = atom('ehr.full(' + names[0] + ',1)@' + str(owner or context))
                    elif access and op['results'][0]['type'] == 'bool':
                        expression = atom('ehr.read:' + fingerprint([(self.records[k]['resource'], self.records[k].get('port', 0), owner if self.records[k].get('port', 0) else None) for k in sorted(access)])) if any(self.records[k]['view'] == 'ehr' for k in access) else expression
                    if owner: self.add_dependencies(owner, vals[0].deps, path, 'resource operand')
                elif kind in ('acir.pop', 'acir.push', 'acir.write'):
                    if owner is None: raise CompileError('state effect outside a transaction')
                    if any(self.functions[n]['meta'].get('kind') == 'guard' for n in self.stack): raise CompileError('guard must only observe state')
                    effects = self.access(fn, op, vals[0], path, owner, kind[5:], context)
                    for key in effects:
                        self.records[key]['value'] = vals[1].expr if len(vals) > 1 else None
                        self.records[key]['value_constant'] = vals[1].const if len(vals) > 1 else None
                        if attrs.get('aspect') == 'full':
                            self.records[key]['local_condition'] = both(self.records[key]['local_condition'], neg(self.occupancy('empty', self.records[key]['resource'])))
                    self.add_dependencies(owner, deps, path, 'effect payload')
                elif kind == 'acir.query':
                    access = self.access(fn, op, vals[0], path, owner, 'query.' + attrs['kind'], context)
                    deps |= access
                    names = self.resource_names(vals[0].refs)
                    if len(names) == 1 and attrs['kind'] in ('empty', 'full'): expression = self.occupancy(attrs['kind'], names[0])
                elif kind == 'acir.unreachable':
                    path = False
                elif kind in ('acir.guard', 'acir.check'):
                    if kind == 'acir.guard' and owner:
                        self.guard_records[owner].append(dict(condition=vals[0].expr, path=path, loc=op['loc'], function=fn['name'], name=attrs.get('name'), source=attrs.get('source', 'implicit' if attrs.get('implicit') else 'function'), implicit=attrs.get('implicit', False)))
                    self.add_dependencies(owner, deps, path, 'explicit guard') if owner else None
                    needed |= deps
                    if not attrs.get('implicit') and not self.collect_stages: path = both(path, vals[0].valid, vals[0].expr)
                elif kind == 'acir.extract' and vals[0].refs:
                    index = vals[1].const if len(vals) > 1 else None
                    if index is not None and vals[0].items:
                        if not 0 <= index < len(vals[0].items): raise CompileError('Queue list index out of range')
                        refs = vals[0].items[index]
                    elif index is not None:
                        candidates = set()
                        for r in vals[0].refs:
                            if self.resources[r]['kind'] == 'qarray':
                                if r in self.array_sizes and not 0 <= index < self.array_sizes[r]: raise CompileError('Queue array index out of range')
                                candidates.add(r + '[' + str(index) + ']')
                            else: candidates.add(r)
                        refs = frozenset(candidates)
                    else: refs = vals[0].refs
                    consume = vals[0].consume
                    origins = tuple((ref, dict(vals[0].origins).get(ref, dict(vals[0].origins).get(ref.split('[')[0], True))) for ref in sorted(refs))
                elif kind == 'acir.range':
                    constant = vals[0].const
                elif kind == 'acir.aggregate':
                    items = tuple(v.refs for v in vals)
                    refs = frozenset().union(*(v.refs for v in vals))
                    origins = tuple((ref, either(*(dict(v.origins).get(ref, True) for v in vals if ref in v.refs))) for ref in sorted(refs))
                elif kind == 'acir.cast':
                    expression, constant, refs, consume, items, origins = vals[0].expr, vals[0].const, vals[0].refs, vals[0].consume, vals[0].items, vals[0].origins
                    if constant is not None:
                        constant = self.integer(constant, op['results'][0]['type'])
                        expression = bool(constant) if op['results'][0]['type'] == 'bool' else atom('const:' + str(constant))
                elif kind == 'arith.select':
                    # Canonicalization can replace a resource-valued CFG phi with
                    # select. Preserve both identities and their selection paths.
                    value = merge_data([(vals[0].expr, vals[1]), (neg(vals[0].expr), vals[2])], op['results'][0]['type'] == 'bool')
                    deps |= value.deps
                    value = replace(value, deps=deps, valid=both(value.valid, vals[0].valid))
                elif kind == 'acir.unary' and attrs['operator_name'] == 'not':
                    expression = neg(vals[0].expr)
                    constant = not vals[0].const if vals[0].const is not None else None
                elif kind in ('arith.andi', 'arith.ori') and op['results'][0]['type'] == 'bool':
                    expression = (both if kind == 'arith.andi' else either)(*(v.expr for v in vals))
                elif kind == 'arith.xori' and op['results'][0]['type'] == 'bool':
                    expression = either(both(vals[0].expr, neg(vals[1].expr)), both(neg(vals[0].expr), vals[1].expr))
                elif kind == 'arith.cmpi' and all(v.const is not None for v in vals):
                    a, b = [v.const for v in vals]; pred = attrs['predicate']
                    constant = {0: a == b, 1: a != b, 2: a < b, 3: a <= b, 4: a > b, 5: a >= b, 6: a < b, 7: a <= b, 8: a > b, 9: a >= b}[pred]
                    expression = constant
                elif kind.startswith('arith.') and len(vals) == 2 and all(v.const is not None for v in vals):
                    a, b = [v.const for v in vals]
                    operations = {'arith.addi': lambda: a+b, 'arith.subi': lambda:a-b, 'arith.muli':lambda:a*b, 'arith.andi':lambda:a&b, 'arith.ori':lambda:a|b, 'arith.xori':lambda:a^b}
                    if kind in operations:
                        constant = self.integer(operations[kind](), op['results'][0]['type'])
                        expression = atom('const:' + str(constant))
                elif kind == 'func.call':
                    callee = attrs['callee'].lstrip('@')
                    if callee in self.functions:
                        target = self.functions[callee]
                        site = self.operation_ids[id(op)]
                        call_context = context + '/' + (site[1] if site[0] == 'source' else fingerprint(site))
                        call_selected = both(path, *(v.valid for v in vals))
                        method = target['meta'].get('method')
                        call = None
                        if method:
                            meta = target['meta']
                            call = fingerprint((owner, call_context, meta['interface']))
                            record = self.method_records.setdefault(call, dict(id=call, transaction=owner, component=meta['instance'],
                                contract=meta['contract'], interface=method, identity=meta['interface'], operation=method,
                                condition=False, guard=False, loc=op['loc'], ancestors=list(self.method_stack),
                                function=callee, arguments=[dict(resources=sorted(v.refs), value=v.expr) for v in vals]))
                            record['condition'] = either(record['condition'], call_selected)
                            self.method_stack.append(call)
                        value, requirements, completion = self.evaluate(target, vals, call_selected, owner, call_context)
                        if call:
                            self.method_stack.pop()
                            record['guard'] = either(record['guard'], completion)
                        # Preparation completion belongs to the returned value.
                        # A guarded observation must not gate unrelated rules.
                        if owner:
                            needed |= requirements
                            if not self.collect_stages: path = both(path, completion)
                        deps |= value.deps | requirements
                        value = replace(value, deps=deps, valid=both(value.valid, completion))
                    elif callee.startswith('ac_builtin_'): pass
                    else: raise CompileError('unknown MLIR callee: ' + callee)
                elif kind == 'acir.invoke':
                    if owner is not None: raise CompileError('rule may only be selected by Module preparation')
                    callee = attrs.get('callee')
                    if callee not in self.functions: raise CompileError('unknown rule function: ' + str(callee))
                    target = self.functions[callee]; meta = target['meta']
                    rule = meta['owner'] + '.' + meta['rule']
                    self.nodes.setdefault(rule, dict(id=rule, module=meta['owner'], rule=meta['rule'], function=callee, selection=False, locations=[]))
                    self.nodes[rule]['selection'] = either(self.nodes[rule]['selection'], path)
                    if op['loc'] not in self.nodes[rule]['locations']: self.nodes[rule]['locations'].append(op['loc'])
                    self.selections[rule].append((path, op['loc']))
                    self.nodes[rule].setdefault('calls', []).append(dict(condition=path, location=op['loc'], arguments=[dict(value=v.expr, resources=sorted(v.refs)) for v in vals]))
                    selection = both(path, *(v.valid for v in vals))
                    self.add_dependencies(rule, needed | control_deps | deps, path, 'selection or argument computation')
                    invocations.append((rule, path, op['args'], label))
                    before_ids = set(self.records)
                    _, _, completion = self.evaluate(target, vals, selection, rule, rule + '/' + fingerprint((fn['name'], self.operation_ids[id(op)])))
                    for key in set(self.records) - before_ids:
                        record = self.records[key]
                        if record['operation'] in ('pop', 'push', 'write'): record['condition'] = both(record['condition'], completion)
                    self.nodes[rule].setdefault('completion_conditions', []).append(completion)
                elif kind in ('cf.br', 'cf.cond_br'):
                    loop_header = any(dest == label for _, dest in backedges)
                    destinations = [(op['dest'], path, op['dest_args'])] if kind == 'cf.br' else [
                        (op['yes'], both(path, vals[0].valid, vals[0].expr), op['yes_args']), (op['no'], path if loop_header else both(path, vals[0].valid, neg(vals[0].expr)), op['no_args'])]
                    outgoing_controls = dict(controls)
                    if kind == 'cf.cond_br': outgoing_controls[label] = vals[0].deps
                    for dest, predicate, arguments_ids in destinations:
                        if (label, dest) in backedges:
                            continue
                        target_env = dict(env)
                        for parameter, arg in zip(blocks[dest]['params'], arguments_ids): target_env[parameter['id']] = env[arg]
                        incoming[dest].append((predicate, target_env, needed, outgoing_controls))
                elif kind == 'func.return':
                    returns.append((path, vals[0] if vals else Data(), needed | control_deps))
                if value is None: value = Data(expression, constant, refs, deps, consume, items, origins, validity)
                value = replace(value, deps=value.deps | control_deps)
                if owner is None and op['results'] and kind != 'acir.invoke' and fn['meta'].get('kind') != 'initializer':
                    identity = fingerprint((context, fn['name'], label, attrs.get('acir.id'), [r['id'] for r in op['results']]))
                    region = self.preparations.setdefault(identity, dict(id=identity, function=fn['name'], context=context,
                        block=label, identity=attrs.get('acir.id'), operation=kind.split('.', 1)[-1], dialect=kind.split('.', 1)[0], loc=op['loc'],
                        inputs=list(op['args']), outputs=op['results'], condition=False, output_validity=False,
                        boundary='call' if kind == 'func.call' else 'loop' if label in loop_blocks else 'block',
                        control_dependencies=[], state_dependencies=[], method_dependencies=[], users=[]))
                    region['condition'] = either(region['condition'], path)
                    region['output_validity'] = either(region['output_validity'], value.valid)
                    region['state_dependencies'] = sorted(k for k in value.deps | needed if k in self.records and self.records[k]['transaction'] is None)
                    region['control_dependencies'] = sorted(k[len('preparation:'):] for k in needed | control_deps if k.startswith('preparation:'))
                    region['method_dependencies'] = sorted(set(self.method_stack) | ({call} if kind == 'func.call' and callee in self.functions and call else set()))
                    region['method_dependencies'] = sorted(set(region['method_dependencies']) | {m for k in region['state_dependencies'] for m in self.records[k]['methods']})
                    value = replace(value, deps=value.deps | needed | frozenset(['preparation:' + identity]))
                for result in op['results']:
                    env[result['id']] = value
                    observed[result['id']] = value
                    identity = attrs.get('acir.id')
                    if identity: self.values[(fn['name'], identity)] = value
                    if fn['meta'].get('kind') in ('helper', 'initializer') and value.const is not None: self.constants[fn['name']] = value.const
        if backedges:
            # Propagate data provenance through loop-carried SSA block arguments
            # to a fixed point. This does not unroll or enumerate runtime paths.
            deps = {key:set(value.deps) for key,value in observed.items()}
            changed = True
            while changed:
                changed = False
                for block in fn['blocks']:
                    for op in block['ops']:
                        source = set().union(*(deps.get(a, set()) for a in op['args']))
                        source.update(set().union(*(deps.get(branch_conditions[c], set()) for c in control_sources.get(block['label'], set()))))
                        links = [(r['id'], source) for r in op['results']]
                        for dest, arguments_ids in [(op.get('dest'), op.get('dest_args', [])), (op.get('yes'), op.get('yes_args', [])), (op.get('no'), op.get('no_args', []))]:
                            if dest:
                                links.extend((p['id'], deps.get(a, set())) for p,a in zip(blocks[dest]['params'], arguments_ids))
                        for identifier, additions in links:
                            existing = deps.setdefault(identifier, set()); count = len(existing)
                            existing.update(additions); changed |= len(existing) != count
            for tail, head in backedges:
                cycle = {b for b in blocks if reachable(head, b) and reachable(b, tail)}
                if any(o['op'] == 'acir.invoke' for b in cycle for o in blocks[b]['ops']):
                    raise CompileError('runtime repeated rule selection is unsupported')
            for rule, predicate, argsids, label in invocations:
                self.add_dependencies(rule, set().union(*(deps.get(a, set()) for a in argsids)), predicate, 'loop-carried argument computation')
                self.add_dependencies(rule, set().union(*(deps.get(branch_conditions[c], set()) for c in control_sources.get(label, set()))), predicate, 'loop selection computation')
            for region in self.preparations.values():
                if region['context'] != context or region['function'] != fn['name']: continue
                additions = set().union(*(deps.get(r['id'], set()) for r in region['outputs']))
                region['state_dependencies'] = sorted(k for k in additions if k in self.records and self.records[k]['transaction'] is None)
                region['method_dependencies'] = sorted(set(region['method_dependencies']) | {m for k in region['state_dependencies'] for m in self.records[k]['methods']})
            for key, value in list(self.values.items()):
                if key[0] != fn['name']: continue
                # Results have a stable acir.id; recover their parsed SSA id.
                result_ids = [r['id'] for b in fn['blocks'] for o in b['ops'] if o['attrs'].get('acir.id') == key[1] for r in o['results']]
                additions = frozenset().union(*(deps.get(i, set()) for i in result_ids))
                self.values[key] = replace(value, deps=value.deps | additions)
            return_ids = [a for b in fn['blocks'] for o in b['ops'] if o['op'] == 'func.return' for a in o['args']]
            additions = frozenset().union(*(deps.get(a, set()) for a in return_ids))
            # Helper results must carry the fixed-point provenance, just like
            # shared exports and loop-carried rule arguments.
            returns = [(p, replace(v, deps=v.deps | additions), n | additions)
                       for p, v, n in returns]
        self.stack.pop()
        returned = merge_data([(p, v) for p, v, _ in returns], fn['meta'].get('result') == 'bool')
        requirements = frozenset().union(*(n for _, _, n in returns))
        return returned, requirements, either(*(p for p, _, _ in returns))

    def edge(self, source, target, condition, kind, resource, before, after, **metadata):
        if source == target and kind == 'method.order': return
        if satisfiable(condition) is False: return
        self.edges.append(dict(source=source, target=target, kind=kind, resource=resource,
                   condition=condition, operations=[before['operation'], after['operation']],
                   locations=[before['loc'], after['loc']], via=after.get('via'), source_access=before['id'], target_access=after['id'], **metadata))

    def method_orders(self, records):
        """Bind contracts to actual calls, then lift necessary orders through parents.

        Derived entries keep call bindings and evidence rather than asserting a
        context-free order for methods invoked with different resource arguments.
        This also preserves branch predicates when summaries are composed.
        """
        groups = defaultdict(list)
        for record in records:
            if record['transaction'] and satisfiable(record['local_condition'] if record['view'] == 'current' else record['condition']) is not False:
                groups[('resource', record['resource'], record.get('aspect', 'value'))].append(record)
        for record in self.method_records.values():
            if record['transaction']:
                groups[('component', record['component'], '')].append(record)
        constraints = {}
        def add(before, after, witness, condition, resource, contract):
            if before['id'] == after['id'] or satisfiable(condition) is False: return
            key = (before['id'], after['id'])
            if key in constraints:
                constraints[key]['condition'] = either(constraints[key]['condition'], condition)
                for item in witness:
                    if item not in constraints[key]['provenance']: constraints[key]['provenance'].append(item)
            else:
                constraints[key] = dict(before=before, after=after, provenance=list(witness), condition=condition,
                                        resource=resource, contract=contract)
        for (kind, identity, aspect), accesses in groups.items():
            for before in accesses:
                for after in accesses:
                    witness = self.orders[before['contract']].get((before['interface'], after['interface']))
                    if kind == 'resource' and before['view'] == 'ehr':
                        if before['operation'] != 'write' or after['operation'] not in ('read', 'query.full') or before['port'] >= after['port']:
                            continue
                        proof = list(witness or []) if aspect == 'full' else [p for left in before['phase_context'][-1:] for right in after['phase_context'][-1:] if left['contract'] == right['contract'] for p in self.orders[left['contract']].get((left['interface'], right['interface']), [])]
                        if aspect == 'full' and after['role'] == 'guard':
                            proof.extend(self.orders[after['contract']].get(('query.full', 'push'), []))
                        witness = [dict(before=before['phase'], after=after['phase'], relation='SB', source='derived', origin='EHR prior-port visibility', ports=[before['port'], after['port']], provenance=proof)]
                    if witness:
                        condition = both(before['local_condition'] if kind == 'resource' and before['view'] == 'current' else before['condition'], after['condition'])
                        if kind == 'component': condition = both(condition, before['guard'], after['guard'])
                        add(before, after, witness, condition, identity, before['contract'])
        # Parent summaries are composed from concrete child calls/base effects.
        # Each iteration moves up the finite static invocation hierarchy.
        pending = list(constraints.values())
        inferred = defaultdict(list)
        while pending:
            while pending:
                entry = pending.pop()
                before, after = entry['before'], entry['after']
                left = before.get('methods', before.get('ancestors', []))
                right = after.get('methods', after.get('ancestors', []))
                for a in left:
                    for b in right:
                        first, second = self.method_records[a], self.method_records[b]
                        if first['component'] != second['component'] or a == b: continue
                        evidence = dict(before=first['interface'], after=second['interface'], relation='SB', source='derived',
                                        origin=entry['contract'], bindings=[a, b], accesses=[before['id'], after['id']],
                                        provenance=list(entry['provenance']))
                        signal_order = entry['contract'] == 'Signal' or any(p.get('decision_order') for p in entry['provenance'])
                        evidence['decision_order'] = signal_order
                        condition = entry['condition'] if signal_order else both(entry['condition'], first['guard'], second['guard'])
                        if satisfiable(condition) is False: continue
                        if evidence not in inferred[first['contract']]: inferred[first['contract']].append(evidence)
                        old = (a, b) in constraints
                        add(first, second, [evidence], condition, first['component'], first['contract'])
                        if not old: pending.append(constraints[a, b])
            # Merge explicit and inferred orders before computing closure. Compose
            # only concrete calls of the same component, retaining all path guards.
            entries = [e for e in constraints.values() if 'component' in e['before'] and 'component' in e['after']]
            for first in entries:
                for second in entries:
                    if first['after']['id'] != second['before']['id']: continue
                    before, after = first['before'], second['after']
                    if before['component'] != after['component'] or before['id'] == after['id']: continue
                    condition = both(first['condition'], second['condition'])
                    if satisfiable(condition) is False or (before['id'], after['id']) in constraints: continue
                    evidence = dict(before=before['interface'], after=after['interface'], relation='SB', source='derived',
                                    origin=before['contract'], bindings=[before['id'], after['id']],
                                    provenance=first['provenance'] + second['provenance'])
                    inferred[before['contract']].append(evidence)
                    add(before, after, [evidence], condition, before['component'], before['contract'])
                    pending.append(constraints[before['id'], after['id']])
        self.resolved_contracts = [dict(c, relations=c['relations'] + inferred[name],
                                     closure=[dict(before=a, after=b, relation='SB', provenance=w) for (a,b),w in sorted(self.orders[name].items())])
                                   for name,c in self.contracts.items()]
        self.bound_orders = list(constraints.values())
        for entry in constraints.values():
            before, after = entry['before'], entry['after']
            self.edge(before['transaction'], after['transaction'], entry['condition'], 'method.order', entry['resource'], before, after,
                      relation='SB', contract=entry['contract'], interfaces=[before['interface'], after['interface']],
                      provenance=entry['provenance'])

    def infer_contracts(self):
        """Close unconditional summaries, including interfaces not called by rules.

        Conditional/argument-dependent orders continue to use actual invocation
        records. They must never become unconditional component declarations.
        """
        summaries = [f for f in self.functions.values() if f['meta'].get('summary')]
        if not summaries: return
        analyzer = Analyzer(self.program, self.allocation, self.collect_stages)
        analyzer.array_sizes = dict(self.array_sizes)
        for fn in summaries:
            meta = fn['meta']
            call = 'summary:' + meta['interface']
            analyzer.method_records[call] = dict(id=call, transaction=call, component=meta['instance'], contract=meta['contract'],
                interface=meta['method'], identity=meta['interface'], operation=meta['method'], condition=True, guard=True,
                loc={}, ancestors=[], function=fn['name'], arguments=[])
            analyzer.method_stack.append(call)
            _, _, completion = analyzer.evaluate(fn, [], True, call, call)
            analyzer.method_stack.pop()
            analyzer.method_records[call]['guard'] = completion
        self.method_summaries = [dict(r, accesses=[a for a in analyzer.records.values() if r['id'] in a.get('methods', [])])
                                 for r in analyzer.method_records.values() if r['id'].startswith('summary:')]
        if self.collect_stages: return
        uncertain = {r['id'] for r in self.method_summaries if any(a['binding'] == 'conservative' for a in r['accesses'])}
        inferred_pairs = set()
        changed = True
        while changed:
            changed = False
            analyzer.contracts = self.contracts
            analyzer.orders = self.orders
            analyzer.edges = []
            analyzer.method_orders(list(analyzer.records.values()))
            for entry in analyzer.bound_orders:
                before, after = entry['before'], entry['after']
                if not before['id'].startswith('summary:') or not after['id'].startswith('summary:') or entry['condition'] is not True: continue
                if before['id'] in uncertain or after['id'] in uncertain: continue
                contract = self.contracts[before['contract']]
                key = (before['contract'], before['interface'], after['interface'])
                proofs = [p for p in entry['provenance'] if p['source'] == 'derived']
                if key in inferred_pairs or not proofs: continue
                witness = dict(before=before['interface'], after=after['interface'], relation='SB', source='derived',
                               origin=entry['contract'], summaries=[before['function'], after['function']], provenance=proofs)
                contract['relations'].append(witness)
                inferred_pairs.add(key)
                self.orders[before['contract']] = closure(contract)
                changed = True

    def build(self):
        # Static array extents and construction expressions come from saved MLIR.
        for r in self.resources.values():
            if r['kind'] == 'qarray':
                fn = self.functions[r['iterable_fn']]
                value, _, _ = self.evaluate(fn, [], True, None, fn['name'])
                if isinstance(value.const, int) and value.const >= 0: self.array_sizes[r['name']] = value.const
                else: raise CompileError('Queue array extent must be a compile-time constant')
        self.infer_contracts()
        for fn in self.functions.values():
            if fn['meta'].get('kind') == 'work': self.run_work(fn['name'])
        for rule, calls in self.selections.items():
            for i, (a, loc) in enumerate(calls):
                for b, other in calls[i+1:]:
                    if satisfiable(both(a, b)) is not False:
                        raise CompileError(f'{loc}: rule {rule} selected more than once without proven mutual exclusion; other call at {other}')
        records = list(self.records.values())
        if self.allocation is None:
            from .ehr import allocate
            collector = Analyzer(self.program, collect_stages=True)
            collector.array_sizes = dict(self.array_sizes)
            collector.infer_contracts()
            for fn in collector.functions.values():
                if fn['meta'].get('kind') == 'work': collector.run_work(fn['name'])
            summary_accesses = [a for m in collector.method_summaries for a in m['accesses']]
            return Analyzer(self.program, allocate(list(collector.records.values()) + summary_accesses, self.orders, self.resources, self.array_sizes, self.contracts)).build()
        self.describe_bypass(records)
        self.method_orders(records)
        # Merge identical operation constraints, retaining all source locations.
        edges = {}
        for edge in self.edges:
            key = (edge['source'], edge['target'], edge['kind'], edge['resource'], tuple(edge['operations']), edge['source_access'], edge['target_access'], edge['via'])
            if key in edges:
                edges[key]['condition'] = either(edges[key]['condition'], edge['condition'])
                for witness in edge.get('provenance', []):
                    if witness not in edges[key]['provenance']: edges[key]['provenance'].append(witness)
            else: edges[key] = edge
        self.edges = sorted(edges.values(), key=lambda e: (e['source'], e['target'], e['kind'], e['resource'], e['source_access'], e['target_access']))
        self.cycles()
        nodes = []
        for node in sorted(self.nodes.values(), key=lambda n:n['id']):
            accesses = [dict(r) for r in records if r['transaction'] == node['id'] and satisfiable(r['local_condition'] if r['view'] == 'current' and r['operation'] == 'write' else r['condition']) is not False]
            # One conditional pop per actual resource/outer transaction. Locations
            # retain all reads and aliases that contributed to the consumption.
            pops, other = {}, []
            for r in accesses:
                if r['operation'] != 'pop': other.append(r); continue
                key = r['resource']
                if key not in pops: pops[key] = dict(r)
                else:
                    pops[key]['condition'] = either(pops[key]['condition'], r['condition'])
                    for source in r['locations']:
                        if source not in pops[key]['locations']: pops[key]['locations'].append(source)
            node = dict(node, guards=self.guard_records[node['id']], methods=[dict(r, accesses=[a['id'] for a in records if r['id'] in a.get('methods', [])]) for r in self.method_records.values() if r['transaction'] == node['id']],
                        preparations=sorted(self.preparation_uses[node['id']]),
                        preparation_bindings=[dict(preparation=p, condition=c) for p,c in sorted(self.preparation_uses[node['id']].items())],
                        accesses=sorted(other + list(pops.values()), key=lambda r:(r['resource'], r['operation'], r['id'])),
                        requirements=[dict(condition=r['condition'], ready=r['ready'], access=r['id']) for r in other + list(pops.values()) if r['ready'] != 'true'])
            nodes.append(node)
        for rule, uses in self.preparation_uses.items():
            for identity, condition in sorted(uses.items()):
                self.preparations[identity]['users'].append(dict(rule=rule, condition=condition))
        preparations = []
        for region in sorted(self.preparations.values(), key=lambda r:r['id']):
            preparations.append(dict(region,
                requirements=[dict(access=k, condition=self.records[k]['condition'], ready=self.records[k]['ready']) for k in region['state_dependencies'] if self.records[k]['ready'] != 'true']))
        return self.encode(dict(format_version=VERSION, contracts=self.resolved_contracts, method_summaries=self.method_summaries, top=self.program['model']['top'], instances=self.program['model'].get('instances', []),
                  resources=list(self.resources.values()), ehr=self.allocation['views'], ehr_bindings=self.allocation['accesses'], nodes=nodes, edges=self.edges,
                  computations=[r for r in records if r['transaction'] is None], preparations=preparations,
                  preparation_methods=[r for r in self.method_records.values() if r['transaction'] is None], diagnostics=self.diagnostics))

    def describe_bypass(self, records):
        for read in records:
            if read['view'] != 'current' or read['operation'] != 'read': continue
            sources = []
            for write in records:
                if write['view'] != 'current' or write['operation'] != 'write' or write['resource'] != read['resource']: continue
                local = write['transaction'] == read['transaction']
                if local and write['sequence'] >= read['sequence']: continue
                eligible = write['local_condition'] if local else both(write['condition'], atom('granted(' + str(write['transaction']) + ')'))
                if satisfiable(both(eligible, read['condition'])) is False: continue
                sources.append(dict(access=write['id'], transaction=write['transaction'], eligibility=eligible,
                    visibility='temporary' if local else 'granted_transaction', value=write.get('value'),
                    value_constant=write.get('value_constant'), sequence=write['sequence']))
            read['bypass'] = dict(selection='last_local_else_granted_set', sources=sorted(sources, key=lambda s:s['sequence']),
                fallback='retained', commit='transaction', arbitration='unspecified')
        for read in records:
            if read['view'] != 'ehr' or read['operation'] not in ('read', 'query.full'): continue
            sources = []
            for write in records:
                if write['view'] != 'ehr' or write['operation'] != 'write' or write['resource'] != read['resource'] or write['aspect'] != read['aspect'] or write['port'] >= read['port']: continue
                local = write['transaction'] == read['transaction']
                eligible = write['local_condition'] if local else both(write['local_condition'], write['condition'], atom('granted(' + str(write['transaction']) + ')'))
                if satisfiable(both(eligible, read['condition'])) is False: continue
                sources.append(dict(access=write['id'], port=write['port'], transaction=write['transaction'], eligibility=eligible,
                                    visibility='temporary' if local else 'granted_transaction', value=write.get('value'), value_constant=write.get('value_constant'), sequence=write['sequence']))
            read['bypass'] = dict(selection='last_effective_prior_port', sources=sorted(sources, key=lambda s:(s['port'], s['sequence'])), fallback='old', same_port_visible=False)

    def cycles(self):
        adjacency = defaultdict(list)
        for i, edge in enumerate(self.edges): adjacency[edge['source']].append((edge['target'], i))
        found = set()
        for start in sorted(self.nodes):
            def walk(node, path, edgeids, condition):
                for target, index in adjacency[node]:
                    nextcond = both(condition, self.edges[index]['condition'])
                    if satisfiable(nextcond) is False: continue
                    if target == start:
                        signature = tuple(sorted(edgeids + [index]))
                        if signature in found: continue
                        found.add(signature)
                        proof = cubes(nextcond)
                        confirmed = proof is not None and any(all(name.startswith(('empty(', 'full(', 'config:')) for name, _ in cube) for cube in proof)
                        self.diagnostics.append(dict(kind='confirmed-cycle' if confirmed else 'unproven-order', rules=path + [start],
                             edges=edgeids + [index], condition=nextcond,
                             message='可确认的同周期依赖环' if confirmed else '无法证明合法静态顺序'))
                    elif target not in path and target >= start:
                        walk(target, path + [target], edgeids + [index], nextcond)
            walk(start, [start], [], True)

    def encode(self, value):
        if isinstance(value, tuple): return formula(value)
        if isinstance(value, dict): return {k:self.encode(v) for k,v in value.items()}
        if isinstance(value, list): return [self.encode(v) for v in value]
        return value


def analyze_program(program): return Analyzer(program).build()


def dot(graph):
    lines = ['digraph rules {', '  rankdir=LR;']
    for n in graph['nodes']:
        accesses = sorted({a['resource'] + '.' + a['aspect'] + ' p' + str(a['port']) + ' ' + a['operation'] for a in n['accesses'] if a['view'] == 'ehr'})
        lines.append('  ' + json.dumps(n['id']) + ' [label=' + json.dumps(n['id'] + ('\nEHR: ' + '\n'.join(accesses) if accesses else '')) + '];')
    def label(expr):
        if isinstance(expr, bool): return str(expr).lower()
        if 'atom' in expr: return expr['atom']
        if 'not' in expr: return '!' + label(expr['not'])
        kind = next(iter(expr)); return '(' + (' && ' if kind == 'and' else ' || ').join(label(v) for v in expr[kind]) + ')'
    def sources(proof):
        result = {p['source'] for p in proof}
        for p in proof: result.update(sources(p.get('provenance', [])))
        return result
    for e in graph['edges']:
        text = e['kind'] + ': ' + e['resource'] + '\\n' + label(e['condition'])
        if e['kind'] == 'method.order':
            ports = [p['ports'] for p in e['provenance'] if 'ports' in p]
            if ports: text += '\\n' + ', '.join('p' + str(a) + ' → p' + str(b) for a,b in ports)
            text += '\\n' + ' SB '.join(e['interfaces']) + ' [' + ','.join(sorted(sources(e['provenance']))) + ']'
        lines.append('  ' + json.dumps(e['source']) + ' -> ' + json.dumps(e['target']) + ' [label=' + json.dumps(text) + '];')
    return '\n'.join(lines + ['}']) + '\n'

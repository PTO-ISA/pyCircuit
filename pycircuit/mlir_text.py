"""Textual MLIR serialization. No generated C++ or serialized Python AST."""
import re
from .ir import SCALARS, VERSION, element


def quote(s):
    # MLIR uses two-digit byte escapes, not JSON's Unicode escape syntax.
    encoded = []
    for byte in s.encode('utf-8'):
        if byte in (34, 92): encoded.append(chr(92) + chr(byte))
        elif byte < 32 or byte >= 127: encoded.append(chr(92) + format(byte, '02X'))
        else: encoded.append(chr(byte))
    return '"' + ''.join(encoded) + '"'


def attr(value):
    if value is None:
        return 'unit'
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, int):
        return f'{value} : i64'
    if isinstance(value, str):
        return quote(value)
    if isinstance(value, (list, tuple, set)):
        return '[' + ', '.join(attr(x) for x in value) + ']'
    return '{' + ', '.join(quote(k) + ' = ' + attr(v) for k, v in value.items()) + '}'


def typ(t):
    if t == 'bool':
        return 'i1'
    if t in SCALARS:
        return 'i' + t[1:]
    if t.startswith(('queue<', 'cell<', 'ehr<', 'signal<', 'qarray<', 'vector<', 'array<', 'tuple<')):
        kind, rest = t.split('<', 1)
        payload = rest[:-1]
        if kind == 'tuple':
            parts, start, depth = [], 0, 0
            for i, char in enumerate(payload):
                depth += (char == '<') - (char == '>')
                if char == ',' and depth == 0: parts.append(payload[start:i]); start = i + 1
            parts.append(payload[start:])
            return 'tuple<' + ', '.join(typ(x) for x in parts) + '>'
        if kind == 'array':
            payload, count = payload.rsplit(',', 1)
            return '!acir.array<' + typ(payload) + ', ' + count + '>'
        return '!acir.' + kind + '<' + typ(payload) + '>'
    return '!acir.struct<' + quote(t) + '>'


def serialize(model):
    functions = []
    resources = []
    global _symbols, _functions
    _symbols, _functions = {}, {}
    config = model['config']
    construction = {o['id']: o for o in model['construction']}
    def initializer(name, result, ids, sequence=False):
        ops, seen = [], set()
        params = [dict(name=p['name'], type=p['type'], id='cfg' + str(i), targets=[]) for i, p in enumerate(config)]
        cfg = {p['name']: p['id'] for p in params}
        aliases = {}
        def visit(i):
            if i in aliases:
                return aliases[i]
            o = construction[i]
            if o['op'] == 'config':
                aliases[i] = cfg[o['name']]
            else:
                args = [visit(a) for a in o['args']]
                ops.append(o | {'args': args})
                aliases[i] = i
            return aliases[i]
        inputs = [visit(i) for i in ids]
        if sequence:
            returned = inputs[0]
        elif len(inputs) == 1 and result == construction[ids[0]]['type']:
            returned = inputs[0]
        else:
            returned = 'assembled'
            ops.append(dict(op='aggregate', id=returned, type=result, args=inputs))
        ops.append(dict(op='return', id='return', type='void', args=[returned]))
        functions.append(dict(name=name, kind='helper', result=result, params=params, blocks=[dict(label='entry',params=[],ops=ops)]))
        return name
    for r in model['resources']:
        r = dict(r)
        name = r['name']
        if r['kind'] in ('queue', 'qarray'):
            r['capacity_fn'] = initializer('ac_init_' + name + '_cap', 'u32', [r.pop('capacity')])
        if r['kind'] in ('queue', 'cell', 'ehr'):
            if r['kind'] in ('cell', 'ehr', 'signal'): r.pop('capacity', None)
            ids, sequence = r.pop('initial'), r.pop('sequence')
            result = r['type'] if r['kind'] in ('cell', 'ehr', 'signal') else construction[ids[0]]['type'] if sequence else 'vector<' + r['type'] + '>'
            r['initial_fn'] = initializer('ac_init_' + name, result, ids, sequence)
        elif r['kind'] == 'qarray':
            i = r.pop('iterable')
            r['iterable_fn'] = initializer('ac_init_' + name + '_range', construction[i]['type'], [i])
            fn = r.pop('initializer')
            fn['result'] = r['type']
            functions.append(fn)
            r['initializer_fn'] = fn['name']
        elif r['kind'] == 'signal':
            r.pop('initial', None); r.pop('sequence', None)
        else:
            raise ValueError('unknown resource kind: ' + r['kind'])
        resources.append(r)
    functions.extend(model['functions'])
    modules = []
    for m in model['modules']:
        functions.append(dict(m['body'], owner=m['name']))
        modules.append(dict(name=m['name']))
    intrinsics = {}
    for fn in functions:
        for block in fn['blocks']:
            for op in block['ops']:
                if op['op'] == 'binary' and op['operator'] in ('<<', '>>', '//', '%'):
                    name = {'<<':'shl', '>>':'shr', '//':'div', '%':'mod'}[op['operator']]
                    intrinsic_symbol = 'ac_builtin_' + name + '_' + op['type']
                    intrinsics[intrinsic_symbol] = (name, op['type'])
    header = dict(format_version=VERSION, exports=model['exports'], top=model['top'], config=config, types=model['types'], type_order=list(model['types']), constants=model.get('constants',{}))
    # Resources retain their elaborated identity. Ownership follows instance members.
    owners = {}
    for instance in model.get('instances', []):
        for value in instance['members'].values():
            if isinstance(value, dict):
                for name in value.get('resources', []): owners.setdefault(name, instance['name'])
    for r in resources:
        owner = r.get('owner') or owners.get(r['name'], next((i['name'] for i in model.get('instances', []) if i['parent'] is None), 'top'))
        _symbols[r['name']] = (owner, r['name'])
    module_names = list(dict.fromkeys([i['name'] for i in model.get('instances', [])] + [m['name'] for m in modules]))
    for fn in functions:
        owner = fn.get('owner') or fn.get('instance') or next((m for m in sorted(module_names, key=len, reverse=True) if fn['name'].startswith(m + '.') or fn['name'] == m + '_body'), None)
        if fn['kind'] not in ('rule','body'): owner = None
        fn['_scope'] = owner
        _functions[fn['name']] = fn
        if owner: _symbols[fn['name']] = (owner, fn['name'])
    lines = ['module attributes {acir.model = ' + attr(header) + '} {']
    # Ordinary helper/intrinsic functions remain func operations. Global helpers
    # are imported into each module's single scope; lowering interns them once.
    globals_ = [f for f in functions if not f['_scope']]
    for intrinsic_symbol, (name, t) in sorted(intrinsics.items()):
        meta = dict(name=intrinsic_symbol, kind='intrinsic', intrinsic=name, result=t, params=[dict(name='a',type=t),dict(name='b',type=t)])
        globals_.append(dict(_intrinsic='  func.func private @' + intrinsic_symbol + '(' + typ(t) + ', ' + typ(t) + ') -> ' + typ(t) + ' attributes {acir.function = ' + attr(meta) + '} loc(unknown)'))
    for f in globals_:
        lines.extend([f['_intrinsic']] if '_intrinsic' in f else function(f))
    for name in module_names:
        instance = next((i for i in model.get('instances', []) if i['name'] == name), {})
        contract = next((c for c in model['contracts'] if c['id'] == 'Module:' + name), {})
        lines.append('  acir.module ' + sym(name) + ' attributes {acir.instance = ' + attr(instance) + '} {')
        for local, value in instance.get('members', {}).items():
            if isinstance(value, dict) and 'instance' in value:
                lines.append('    acir.instance ' + sym(local) + ' = ' + sym(value['instance']))
        lines.append('    "acir.contract"() {relations = ' + attr(contract.get('relations', [])) + ', rules = ' + attr(contract.get('rules', [])) + '} : () -> ()')
        for r in resources:
            if _symbols[r['name']][0] != name: continue
            data = {k:v for k,v in r.items() if k not in ('kind', 'owner', 'contract', 'visibility', 'loc')}
            decl = 'queue.array' if r['kind'] == 'qarray' else r['kind']
            lines.append('    acir.' + decl + ' ' + sym(r['name']) + ' : ' + typ(r['type']) + ' ' + attr(data))
        for f in globals_:
            if '_intrinsic' in f: lines.append(f['_intrinsic']); continue
            signature = ', '.join(typ(p['type']) for p in f['params'])
            result = '' if f['result'] == 'void' else ' -> ' + typ(f['result'])
            lines.append('    func.func private ' + sym(f['name']) + '(' + signature + ')' + result)

        for fn in functions:
            if fn['_scope'] != name: continue
            if fn['kind'] == 'body':
                meta = {k:v for k,v in fn.items() if k not in ('blocks','guard','_scope') and v is not None}
                lines.append('    acir.body attributes {acir.function = ' + attr(meta) + '} {')
                lines.extend(region(fn, 'body'))
                lines.append('    }')
            elif fn['kind'] == 'rule':
                meta = {k:v for k,v in fn.items() if k not in ('blocks','guard','_scope','outputs') and v is not None}
                signature = '(' + ', '.join(typ(p['type']) for p in fn['params']) + ') -> ' + ('()' if fn['result'] == 'void' else typ(fn['result']))
                extra = ''
                start = '    acir.rule ' + sym(fn['name']) + ' : ' + signature + ' {acir.function = ' + attr(meta) + extra + '}'
                guard_blocks = fn['guard']
                trivial = len(guard_blocks) == 1 and len(guard_blocks[0]['ops']) == 2 and guard_blocks[0]['ops'][0]['op'] == 'const' and guard_blocks[0]['ops'][0]['value'] is True
                if trivial: lines.append(start + ' body {')
                else:
                    lines.append(start + ' guard {')
                    guard = dict(fn, result='bool', blocks=guard_blocks)
                    lines.extend(region(guard, 'guard'))
                    lines.append('    } body {')
                lines.extend(region(fn, fn['kind']))
                lines.append('    }')
            else: lines.extend(function(fn))
        lines.append('  }')
    lines.append('}')
    return '\n'.join(lines) + '\n'


def sym(name):
    return '@' + (name if re.fullmatch(r'[a-zA-Z_$][a-zA-Z0-9_$.-]*', name) else quote(name))


def symbol(name):
    scope, local = _symbols[name]
    return sym(scope) + '::' + sym(local)


def region(fn, kind):
    lines = function(fn, kind)[1:-1]
    signature = ', '.join('%' + p['id'] + ': ' + typ(p['type']) for p in fn['params'])
    return ['    ^entry' + ('(' + signature + ')' if signature else '') + ':'] + lines


def function(fn, region_kind=None):
    params = fn['params']
    types = {p['id']: p['type'] for p in params}
    blocks = fn['blocks']
    for b in blocks:
        types.update({p['id']: p['type'] for p in b['params']})
        types.update({o['id']: o['type'] for o in b['ops']})
    result = fn['result']
    # A missing helper return is a diagnostic trap with a typed fallback return.
    meta = {k: v for k, v in fn.items() if k not in ('blocks', 'guard', '_scope') and v is not None}
    signature = ', '.join('%' + p['id'] + ': ' + typ(p['type']) + ' loc(unknown)' for p in params)
    ret = '' if result == 'void' else ' -> ' + typ(result)
    out = ['  func.func ' + sym(fn['name']) + '(' + signature + ')' + ret + ' attributes {acir.function = ' + attr(meta) + '} {']
    def emit(o):
        kind, t, ids = o['op'], o['type'], o['args']
        args = ', '.join('%' + x for x in ids)
        ts = ', '.join(typ(types[x]) for x in ids)
        lhs = '' if t == 'void' else '%' + o['id'] + ' = '
        attrs = {k: v for k, v in o.items() if k not in ('op','id','type','args','loc')}
        if kind.startswith(('queue.', 'cell.', 'ehr.', 'signal.')):
            attrs.pop('view', None); attrs.pop('interface', None)
        attrs['acir.source_type'] = t
        if o.get('loc', {}).get('line') or kind in ('export', 'rule.call'): attrs['acir.id'] = o['id']
        custom = {'resource': 'ref', 'signal.read': 'signal.get', 'signal.write': 'signal.set', 'queue.read': 'queue.peek', 'field': 'extract', 'index': 'extract', 'length': 'length', 'aggregate': 'aggregate', 'update': 'update', 'array.update': 'update', 'zero':'aggregate'}
        if kind == 'module.call':
            stmt = '"acir.module.call"() {target = ' + sym(o['target']) + '} : () -> ()'
        elif kind == 'param':
            return
        elif kind == 'br':
            stmt = 'cf.br ^' + o['dest'] + (f'({args} : {ts})' if ids else '')
        elif kind == 'cond_br':
            stmt = f'cf.cond_br {args}, ^{o["yes"]}, ^{o["no"]}'
        elif kind == 'return':
            if region_kind:
                ending = 'guard.yield' if region_kind == 'guard' else 'body.end' if region_kind == 'body' else 'return'
                stmt = '"acir.' + ending + '"(' + args + ') ' + attr(attrs) + f' : ({ts}) -> ()'
            elif not ids and result != 'void':
                out.append('    %fallback_' + o['id'] + ' = "acir.aggregate"() {acir.source_type = ' + quote(result) + '} : () -> ' + typ(result) + ' loc(unknown)')
                stmt = 'return %fallback_' + o['id'] + ' : ' + typ(result)
            else:
                stmt = 'return' + (f' {args} : {ts}' if ids else '')
        elif kind == 'const':
            width = 1 if t == 'bool' else int(t[1:])
            n = int(o['value']) % (1 << width)
            if n >= 1 << (width - 1): n -= 1 << width
            stmt = lhs + '"arith.constant"() {value = ' + str(n) + ' : ' + typ(t) + ', acir.source_type = ' + quote(t) + '} : () -> ' + typ(t)
        elif kind == 'binary':
            operator = o['operator']
            signed = types[ids[0]].startswith('i')
            binary = {'+':'addi','-':'subi','*':'muli','&':'andi','|':'ori','^':'xori','and':'andi','or':'ori','<<':'shli','>>':'shrsi' if signed else 'shrui','//':'floordivsi' if signed else 'divui','%':'remsi' if signed else 'remui'}
            cmp = {'==':'eq','!=':'ne','<':'slt' if signed else 'ult','<=':'sle' if signed else 'ule','>':'sgt' if signed else 'ugt','>=':'sge' if signed else 'uge'}
            if operator in ('<<', '>>', '//', '%'):
                helper = {'<<':'shl', '>>':'shr', '//':'div', '%':'mod'}[operator]
                stmt = lhs + 'func.call @ac_builtin_' + helper + '_' + t + f'({args}) ' + attr(attrs) + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
            elif types[ids[0]] in SCALARS:
                name = 'arith.' + (binary[operator] if operator in binary else 'cmpi')
                if operator in cmp:
                    attrs['predicate'] = {'eq':0,'ne':1,'slt':2,'sle':3,'sgt':4,'sge':5,'ult':6,'ule':7,'ugt':8,'uge':9}[cmp[operator]]
                attrs.pop('operator')
                if name == 'arith.cmpi':
                    extra = attr(attrs)
                else: extra = attr(attrs)
                stmt = lhs + '"' + name + f'"({args}) ' + extra + f' : ({ts}) -> {typ(t)}'
            else:
                attrs['operator_name'] = attrs.pop('operator')
                stmt = lhs + f'"acir.compare"({args}) ' + attr(attrs) + f' : ({ts}) -> {typ(t)}'
        elif kind == 'call':
            attrs.pop('name', None)
            target = _functions.get(o['name'])
            if target and target['kind'] == 'rule':
                attrs.pop('output_binding',None); attrs.pop('output_bindings',None)
                attributes = attr(attrs)[:-1] + ', target = ' + symbol(o['name'])
                if o.get('output_binding'): attributes += ', output_binding = ' + symbol(o['output_binding'])
                if o.get('output_bindings'): attributes += ', output_bindings = [' + ', '.join(symbol(o['output_bindings'][i]) if i in o['output_bindings'] else 'unit' for i in range(len(target['output_type']))) + ']' 
                attributes += '}'
                stmt = lhs + '"acir.rule.call"(' + args + ') ' + attributes + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
            else:
                stmt = lhs + 'func.call ' + sym(o['name']) + f'({args}) ' + attr(attrs) + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
        else:
            if kind == 'unary': attrs['operator_name'] = attrs.pop('operator')
            if kind == 'index' or kind == 'array.update': attrs['indexed'] = True
            if kind == 'resource':
                attrs.pop('name', None)
                attributes = attr(attrs)[:-1] + ', target = ' + symbol(o['name']) + '}'
            else: attributes = attr(attrs)
            stmt = lhs + '"acir.' + custom.get(kind, kind) + f'"({args}) ' + attributes + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
        loc = o.get('loc',{})
        if loc.get('line'):
            stmt += ' loc(' + quote(loc['file']) + f':{loc["line"]}:{loc.get("column",0) + 1})'
        else:
            stmt += ' loc(unknown)'
        out.append('    ' + stmt)
    def structured():
        # ACPy's reducible CFG records are rendered as SCF when there is one
        # exit. Multiple payload/None returns deliberately retain explicit CFG.
        if sum(o['op'] == 'return' for block in blocks for o in block['ops']) != 1: return False
        by_name = {block['label']: block for block in blocks}
        successors = {}
        for block in blocks:
            end = block['ops'][-1]
            successors[block['label']] = [end['dest']] if end['op'] == 'br' else [end['yes'], end['no']] if end['op'] == 'cond_br' else []
        post = {name: set(by_name) for name in by_name}
        for name, edges in successors.items():
            if not edges: post[name] = {name}
        changed = True
        while changed:
            changed = False
            for name, edges in successors.items():
                if edges:
                    value = {name} | set.intersection(*(post[e] for e in edges))
                    if value != post[name]: post[name] = value; changed = True
        def reaches(start, target, avoid):
            visited, pending = set(avoid), [start]
            while pending:
                name = pending.pop()
                if name == target: return True
                if name in visited: continue
                visited.add(name); pending.extend(successors[name])
            return False
        def mapped_emit(operation, aliases):
            begin = len(out); emit(operation)
            for index in range(begin, len(out)):
                out[index] = re.sub(r'%([a-zA-Z0-9_]+)', lambda m: '%' + aliases.get(m[1], m[1]), out[index])
        def names(values, aliases): return ', '.join('%' + aliases.get(v, v) for v in values)
        def yield_values(values, aliases):
            out.append('    scf.yield' + (' ' + names(values, aliases) + ' : ' + ', '.join(typ(types[v]) for v in values) if values else ''))
        def walk(name, stop=None, aliases=None):
            aliases = aliases or {}
            while name != stop:
                block = by_name[name]
                for operation in block['ops'][:-1]: mapped_emit(operation, aliases)
                end = block['ops'][-1]
                if end['op'] == 'return': mapped_emit(end, aliases); return
                if end['op'] == 'br':
                    dest = end['dest']
                    if dest == stop: yield_values(end['args'], aliases); return
                    head = by_name[dest]; term = head['ops'][-1]
                    loop = term['op'] == 'cond_br' and reaches(term['yes'], dest, {term['no']})
                    if loop:
                        params = head['params']; identifiers = [p['id'] for p in params]
                        before = {**aliases, **{v:v + '_before' for v in identifiers}}
                        after = {**aliases, **{v:v + '_after' for v in identifiers}}
                        ts = ', '.join(typ(p['type']) for p in params)
                        lhs = ', '.join('%' + v for v in identifiers) + ' = ' if identifiers else ''
                        assignments = ', '.join('%' + before[p['id']] + ' = %' + aliases.get(v,v) for p,v in zip(params, end['args']))
                        out.append('    ' + lhs + 'scf.while (' + assignments + ') : (' + ts + ') -> (' + ts + ') {')
                        for operation in head['ops'][:-1]: mapped_emit(operation, before)
                        out.append('    scf.condition(' + names(term['args'], before) + ')' + (' ' + names(identifiers, before) + ' : ' + ts if identifiers else ''))
                        out.append('    } do {')
                        signature = ', '.join('%' + after[p['id']] + ': ' + typ(p['type']) for p in params)
                        out.append('    ^after' + ('(' + signature + ')' if signature else '') + ':')
                        walk(term['yes'], dest, after)
                        out.append('    }')
                        name = term['no']; continue
                    # A straight branch binds the destination's arguments.
                    aliases = {**aliases, **{p['id']:aliases.get(v,v) for p,v in zip(head['params'], end['args'])}}
                    name = dest; continue
                yes, no = end['yes'], end['no']
                common = (post[yes] & post[no]) - {name}
                if not common: raise ValueError('no structured join')
                join = max(common, key=lambda n: len(post[n]))
                params = by_name[join]['params']
                lhs = ', '.join('%' + p['id'] for p in params) + ' = ' if params else ''
                result_types = ' -> (' + ', '.join(typ(p['type']) for p in params) + ')' if params else ''
                out.append('    ' + lhs + 'scf.if ' + names(end['args'], aliases) + result_types + ' {')
                walk(yes, join, dict(aliases)); out.append('    } else {'); walk(no, join, dict(aliases)); out.append('    }')
                name = join
        start = len(out)
        try: walk(blocks[0]['label'])
        except (ValueError, RecursionError):
            del out[start:]; return False
        return True
    if not structured():
        for index, b in enumerate(blocks):
            if index:
                args = ', '.join('%' + p['id'] + ': ' + typ(p['type']) + ' loc(unknown)' for p in b['params'])
                out.append('  ^' + b['label'] + ('(' + args + ')' if args else '') + ':')
            for o in b['ops']: emit(o)
    out.append('  } loc(unknown)')
    return out

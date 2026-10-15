"""Textual MLIR serialization. No generated C++ or serialized Python AST."""
import re
from .ir import SCALARS, VERSION, element, ArrayType, MaskType, array_type, descriptor


def quote(s):
    # MLIR uses two-digit byte escapes, not JSON's Unicode escape syntax.
    encoded = []
    for byte in s.encode('utf-8'):
        if byte in (34, 92): encoded.append(chr(92) + chr(byte))
        elif byte < 32 or byte >= 127: encoded.append(chr(92) + format(byte, '02X'))
        else: encoded.append(chr(byte))
    return '"' + ''.join(encoded) + '"'


class MLIRType:
    def __init__(self, value): self.value = value


def attr(value):
    if isinstance(value, MLIRType): return typ(value.value)
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
    a = array_type(t)
    if t.startswith('selected<'): return '!acir.selected<' + typ(element(t)) + '>'
    if isinstance(a, MaskType): return '!acir.mask<[' + ', '.join(map(str,a.shape)) + ']>'
    if isinstance(a, ArrayType): return '!acir.array<' + typ(a.item) + ', [' + ', '.join(map(str,a.shape)) + ']>'
    if t.startswith(('queue<', 'cell<', 'ehr<', 'signal<', 'tuple<')):
        kind, rest = t.split('<', 1)
        payload = rest[:-1]
        if kind == 'tuple':
            if not payload: return 'tuple<>'
            parts, start, depth = [], 0, 0
            for i, char in enumerate(payload):
                depth += (char == '<') - (char == '>')
                if char == ',' and depth == 0: parts.append(payload[start:i]); start = i + 1
            parts.append(payload[start:])
            return 'tuple<' + ', '.join(typ(x) for x in parts) + '>'
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
        if r['kind'] == 'queue':
            r['capacity_fn'] = initializer('ac_init_' + name + '_cap', 'u32', [r.pop('capacity')])
        if r['kind'] in ('queue', 'cell', 'ehr'):
            if r['kind'] in ('cell', 'ehr', 'signal'): r.pop('capacity', None)
            ids, sequence = r.pop('initial'), r.pop('sequence')
            result = r['type'] if r['kind'] in ('cell', 'ehr', 'signal') else construction[ids[0]]['type'] if sequence else 'tuple<' + ','.join(r['type'] for _ in ids) + '>'
            r['initial_fn'] = initializer('ac_init_' + name, result, ids, sequence)
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
    def operations(ops):
        for op in ops:
            yield op
            for branch in op.get('branches', []): yield from operations(branch)
    for fn in functions:
        for block in fn['blocks']:
            for op in operations(block['ops']):
                if op['op'] == 'binary' and op['operator'] in ('<<', '>>', '//', '%'):
                    name = {'<<':'shl', '>>':'shr', '//':'div', '%':'mod'}[op['operator']]
                    intrinsic_symbol = 'ac_builtin_' + name + '_' + op['type']
                    intrinsics[intrinsic_symbol] = (name, op['type'])
    header = dict(format_version=VERSION, top=model['top'], config=config, types={name:[field | {'payload_type': MLIRType(field['type'])} for field in fields] for name,fields in model['types'].items()}, type_order=list(model['types']), constants=model.get('constants',{}))
    for r in resources:
        _symbols[r['name']] = (r['owner'], r['name'])
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
        lines.append('    "acir.contract"() {relations = ' + attr(contract.get('relations', [])) + ', rules = ' + attr(contract.get('rules', [])) + '} : () -> ()')
        for r in resources:
            if _symbols[r['name']][0] != name: continue
            data = {k:v for k,v in r.items() if k not in ('kind', 'owner', 'contract', 'visibility', 'loc')}
            decl = r['kind']
            lines.append('    acir.' + decl + ' ' + sym(r['name']) + ' : ' + typ(r['type']) + ' ' + attr(data))
        for array in model.get('module_arrays', []):
            if array['owner'] == name:
                lines.append('    \"acir.instance.array\"() {name = ' + quote(array['name']) + ', shape = ' + attr(array['shape']) + ', targets = [' + ', '.join(sym(t) for t in array['targets']) + ']} : () -> ()')
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
    signature = ', '.join('%' + p['id'] + ': ' + typ(p['type']) + ' loc(' + quote('acir.binding:' + fn['name'] + ':' + p['name']) + '(unknown))' for p in fn['params'])
    return ['    ^entry' + ('(' + signature + ')' if signature else '') + ':'] + lines


def function(fn, region_kind=None):
    params = fn['params']
    types = {p['id']: p['type'] for p in params}
    blocks = fn['blocks']
    def register(ops):
        for op in ops:
            types[op['id']] = op['type']
            for output in op.get('outputs', []): types[output['id']] = output['type']
            for branch in op.get('branches', []): register(branch)
    for block in blocks: register(block['ops'])
    result = fn['result']
    # A missing helper return is a diagnostic trap with a typed fallback return.
    meta = {k: v for k, v in fn.items() if k not in ('blocks', 'guard', '_scope') and v is not None}
    signature = ', '.join('%' + p['id'] + ': ' + typ(p['type']) + ' loc(' + quote('acir.binding:' + fn['name'] + ':' + p['name']) + '(unknown))' for p in params)
    ret = '' if result == 'void' else ' -> ' + typ(result)
    out = ['  func.func ' + sym(fn['name']) + '(' + signature + ')' + ret + ' attributes {acir.function = ' + attr(meta) + '} {']
    def emit(o):
        kind, t, ids = o['op'], o['type'], o['args']
        args = ', '.join('%' + x for x in ids)
        ts = ', '.join(typ(types[x]) for x in ids)
        lhs = '' if t == 'void' else '%' + o['id'] + ' = '
        attrs = {k: v for k, v in o.items() if k not in ('op','id','type','args','loc','branches','outputs')}
        if kind.startswith(('queue.', 'cell.', 'ehr.', 'signal.')):
            attrs.pop('view', None); attrs.pop('interface', None)
        binding = attrs.pop('source_binding', None)
        scope = attrs.pop('source_scope', '')
        attrs['acir.source_type'] = t
        if o.get('loc', {}).get('line') or kind in ('export', 'rule.call'): attrs['acir.id'] = o['id']
        custom = {'resource': 'ref', 'signal.read': 'signal.get', 'signal.write': 'signal.set', 'queue.read': 'queue.peek', 'field': 'extract', 'index': 'extract', 'length': 'length', 'aggregate': 'aggregate', 'update': 'update', 'zero':'aggregate'}
        if kind == 'if':
            outputs = o.get('outputs', [dict(id=o['id'],type=t)] if t != 'void' else [])
            lhs = ', '.join('%' + v['id'] for v in outputs) + ' = ' if outputs else ''
            result_types = ' -> (' + ', '.join(typ(v['type']) for v in outputs) + ')' if outputs else ''
            out.append('    ' + lhs + 'scf.if ' + args + result_types + ' {')
            for branch_index, branch in enumerate(o['branches']):
                if branch_index: out.append('    } else {')
                for operation in branch: emit(operation)
            out.append('    }')
            return
        if kind == 'yield':
            out.append('    scf.yield' + (' ' + args + ' : ' + ts if ids else ''))
            return
        if kind == 'module.call':
            stmt = '"acir.module.call"() {target = ' + sym(o['target']) + '} : () -> ()'
        elif kind == 'param':
            return
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
            if kind == 'index': attrs['indexed'] = True
            if kind == 'resource':
                attrs.pop('name', None)
                attributes = attr(attrs)[:-1] + ', target = ' + symbol(o['name']) + '}'
            else: attributes = attr(attrs)
            stmt = lhs + '"acir.' + custom.get(kind, kind) + f'"({args}) ' + attributes + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
        loc = o.get('loc',{})
        if binding:
            child = quote(loc['file']) + f':{loc["line"]}:{loc.get("column",0) + 1}' if loc.get('line') else 'unknown'
            stmt += ' loc(' + quote('acir.binding:' + scope + ':' + binding) + '(' + child + '))'
        elif loc.get('line'):
            stmt += ' loc(' + quote(loc['file']) + f':{loc["line"]}:{loc.get("column",0) + 1})'
        else:
            stmt += ' loc(unknown)'
        out.append('    ' + stmt)
    for operation in blocks[0]['ops']: emit(operation)
    out.append('  } loc(unknown)')
    return out

"""Textual MLIR serialization. No generated C++ or serialized Python AST."""
import json
from .ir import SCALARS, element


def quote(s):
    return json.dumps(s, ensure_ascii=True)


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
    if t.startswith(('queue<', 'signal<', 'qarray<', 'vector<', 'array<')):
        kind, rest = t.split('<', 1)
        payload = rest[:-1]
        if kind == 'array':
            payload, count = payload.rsplit(',', 1)
            return '!acir.array<' + typ(payload) + ', ' + count + '>'
        return '!acir.' + kind + '<' + typ(payload) + '>'
    return '!acir.struct<' + quote(t) + '>'


def serialize(model):
    functions = []
    resources = []
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
        if r['kind'] == 'queue':
            ids, sequence = r.pop('initial'), r.pop('sequence')
            result = construction[ids[0]]['type'] if sequence else 'vector<' + r['type'] + '>'
            r['initial_fn'] = initializer('ac_init_' + name, result, ids, sequence)
        elif r['kind'] == 'qarray':
            i = r.pop('iterable')
            r['iterable_fn'] = initializer('ac_init_' + name + '_range', construction[i]['type'], [i])
            fn = r.pop('initializer')
            fn['result'] = r['type']
            functions.append(fn)
            r['initializer_fn'] = fn['name']
        else:
            fn = r.pop('function')
            fn['result'] = r['type']
            fn['owner'] = name
            functions.append(fn)
            r['evaluate_fn'] = fn['name']
        resources.append(r)
    functions.extend(model['functions'])
    modules = []
    for m in model['modules']:
        rules = []
        for r in m['rules']:
            fn = dict(r['function'], owner=m['name'], rule=r['name'])
            rules.append(dict(name=r['name'], function=fn['name']))
            functions.append(fn)
        functions.append(dict(m['work'], owner=m['name']))
        modules.append(dict(name=m['name'], rules=rules, work=m['work']['name']))
    intrinsics = {}
    for fn in functions:
        for block in fn['blocks']:
            for op in block['ops']:
                if op['op'] == 'binary' and op['operator'] in ('<<', '>>', '//', '%'):
                    name = {'<<':'shl', '>>':'shr', '//':'div', '%':'mod'}[op['operator']]
                    symbol = 'ac_builtin_' + name + '_' + op['type']
                    intrinsics[symbol] = (name, op['type'])
    header = dict(format_version=2, exports=model['exports'], top=model['top'], config=config, types=model['types'], type_order=list(model['types']), constants=model.get('constants',{}), modules=modules)
    lines = ['module attributes {acir.model = ' + attr(header) + '} {']
    for r in resources:
        lines.append('  "acir.resource"() ' + attr(r) + ' : () -> ()')
    for symbol, (name, t) in sorted(intrinsics.items()):
        meta = dict(name=symbol, kind='intrinsic', intrinsic=name, result=t,
                    params=[dict(name='a',type=t),dict(name='b',type=t)])
        lines.append('  func.func private @' + symbol + '(' + typ(t) + ', ' + typ(t) + ') -> ' + typ(t) + ' attributes {acir.function = ' + attr(meta) + '}')
    for fn in functions:
        lines.extend(function(fn))
    lines.append('}')
    return '\n'.join(lines) + '\n'


def function(fn):
    params = fn['params']
    types = {p['id']: p['type'] for p in params}
    blocks = fn['blocks']
    for b in blocks:
        types.update({p['id']: p['type'] for p in b['params']})
        types.update({o['id']: o['type'] for o in b['ops']})
    result = fn['result']
    # A missing helper return is a diagnostic trap with a typed fallback return.
    meta = {k: v for k, v in fn.items() if k != 'blocks'}
    signature = ', '.join('%' + p['id'] + ': ' + typ(p['type']) for p in params)
    ret = '' if result == 'void' else ' -> ' + typ(result)
    out = ['  func.func @' + fn['name'] + '(' + signature + ')' + ret + ' attributes {acir.function = ' + attr(meta) + '} {']
    def emit(o):
        kind, t, ids = o['op'], o['type'], o['args']
        args = ', '.join('%' + x for x in ids)
        ts = ', '.join(typ(types[x]) for x in ids)
        lhs = '' if t == 'void' else '%' + o['id'] + ' = '
        attrs = {k: v for k, v in o.items() if k not in ('op','id','type','args','loc')}
        attrs['acir.source_type'] = t
        custom = {'resource': 'get', 'queue.read': 'read', 'signal.read': 'read', 'queue.pop': 'pop', 'queue.push': 'push', 'queue.revise': 'revise', 'rule.call': 'invoke', 'field': 'extract', 'index': 'extract', 'length': 'length', 'aggregate': 'aggregate', 'update': 'update', 'array.update': 'update', 'zero':'aggregate'}
        if kind == 'param':
            return
        if kind == 'br':
            stmt = 'cf.br ^' + o['dest'] + (f'({args} : {ts})' if ids else '')
        elif kind == 'cond_br':
            stmt = f'cf.cond_br {args}, ^{o["yes"]}, ^{o["no"]}'
        elif kind == 'return':
            if not ids and result != 'void':
                out.append('    %fallback_' + o['id'] + ' = "acir.aggregate"() {acir.source_type = ' + quote(result) + '} : () -> ' + typ(result))
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
                stmt = lhs + 'func.call @ac_builtin_' + helper + '_' + t + f'({args}) ' + '{acir.source_type = ' + quote(t) + f'}} : ({ts}) -> {typ(t)}'
            elif types[ids[0]] in SCALARS:
                name = 'arith.' + (binary[operator] if operator in binary else 'cmpi')
                if operator in cmp:
                    attrs['predicate'] = {'eq':0,'ne':1,'slt':2,'sle':3,'sgt':4,'sge':5,'ult':6,'ule':7,'ugt':8,'uge':9}[cmp[operator]]
                attrs.pop('operator')
                if name == 'arith.cmpi':
                    extra = '{predicate = ' + str(attrs.pop('predicate')) + ' : i64, acir.source_type = "bool"}'
                else: extra = attr(attrs)
                stmt = lhs + '"' + name + f'"({args}) ' + extra + f' : ({ts}) -> {typ(t)}'
            else:
                attrs['operator_name'] = attrs.pop('operator')
                stmt = lhs + f'"acir.compare"({args}) ' + attr(attrs) + f' : ({ts}) -> {typ(t)}'
        elif kind == 'call':
            stmt = lhs + 'func.call @' + o['name'] + f'({args}) ' + '{acir.source_type = ' + quote(t) + f'}} : ({ts}) -> {typ(t)}'
        else:
            if kind == 'unary': attrs['operator_name'] = attrs.pop('operator')
            if kind in ('queue.empty', 'queue.full', 'queue.size'):
                attrs['kind'] = kind.split('.')[1]
                custom[kind] = 'query'
            if kind == 'index' or kind == 'array.update': attrs['indexed'] = True
            stmt = lhs + '"acir.' + custom.get(kind, kind) + f'"({args}) ' + attr(attrs) + f' : ({ts}) -> ' + ('()' if t == 'void' else typ(t))
        loc = o.get('loc',{})
        if loc.get('line'):
            stmt += ' loc(' + quote(loc['file']) + f':{loc["line"]}:{loc.get("column",0) + 1})'
        out.append('    ' + stmt)
    for index, b in enumerate(blocks):
        if index:
            args = ', '.join('%' + p['id'] + ': ' + typ(p['type']) + ' loc(' + quote('acir.type:' + p['type']) + ')' for p in b['params'])
            out.append('  ^' + b['label'] + ('(' + args + ')' if args else '') + ':')
        for o in b['ops']:
            emit(o)
    out.append('  }')
    return out

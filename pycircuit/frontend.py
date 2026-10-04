"""AST-only elaboration and typed HIR lowering to guarded SSA operations.

Construction is symbolic: host configuration values are constructor arguments,
never evaluated by Python. Only literal construction loops are expanded here.
"""
import ast
from pathlib import Path
from .hir import Block, Branch, Operation
from .ir import CompileError, SCALARS, VERSION, Value, element, resource, wrapped


def spelling(node):
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return spelling(node.value) + '.' + node.attr
    return ''


def decorator(node):
    for d in node.decorator_list:
        name = spelling(d.func if isinstance(d, ast.Call) else d)
        if name.startswith('ac.'):
            return name[3:]
    return 'helper'


class Frontend:
    def __init__(self):
        self.defs = {}
        self.constants = {}
        self.types = {}
        self.files = set()
        self.model = dict(version=VERSION, top='', types=self.types, config=[], resources=[],
                          modules=[], functions=[], construction=[], exports={})
        self.helpers = {}

    def error(self, node, message):
        raise CompileError(f'{getattr(node, "source", "<source>")}:{getattr(node, "lineno", 0)}: {message}')

    def read(self, path):
        path = Path(path).resolve()
        if path in self.files:
            return
        self.files.add(path)
        tree = ast.parse(path.read_text(), filename=str(path))
        for node in ast.walk(tree):
            node.source = str(path)
        for node in tree.body:
            if isinstance(node, ast.Import):
                if any(a.name not in ('pycircuit', 'pycircuit.ac') for a in node.names):
                    self.error(node, 'only ACPy or local from-imports are supported')
            elif isinstance(node, ast.ImportFrom):
                if node.module in ('pycircuit', '__future__'):
                    continue
                base = path.parent
                for _ in range(max(0, node.level - 1)):
                    base = base.parent
                target = base.joinpath(*(node.module or '').split('.')).with_suffix('.py')
                if not target.is_file():
                    self.error(node, f'local dependency not found: {target}')
                self.read(target)
                for alias in node.names:
                    if alias.asname:
                        table = self.defs if alias.name in self.defs else self.constants
                        table[alias.asname] = table[alias.name]
            elif isinstance(node, ast.FunctionDef):
                self.validate_function(node)
                self.defs[node.name] = node
            elif isinstance(node, ast.ClassDef):
                if node.bases or node.keywords or node.decorator_list:
                    self.error(node, 'struct inheritance and class decorators are unsupported')
                fields = []
                for f in node.body:
                    if not isinstance(f, ast.AnnAssign) or not isinstance(f.target, ast.Name):
                        self.error(f, 'structs contain annotated fields only')
                    fields.append(dict(name=f.target.id, type=self.annotation(f.annotation),
                                       default=self.literal(f.value) if f.value else None))
                self.types[node.name] = fields
            elif isinstance(node, (ast.Assign, ast.AnnAssign)):
                targets = node.targets if isinstance(node, ast.Assign) else [node.target]
                for target in targets:
                    if not isinstance(target, ast.Name):
                        self.error(node, 'top-level constants require names')
                    self.constants[target.id] = self.literal(node.value)
            elif isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and isinstance(node.value.value, str):
                pass
            else:
                self.error(node, 'unsupported top-level syntax')

    def validate_function(self, node):
        for fn in (n for n in ast.walk(node) if isinstance(n, ast.FunctionDef)):
            if fn.args.vararg or fn.args.kwarg or fn.args.kwonlyargs or fn.args.posonlyargs:
                self.error(fn, 'variadic, keyword-only and positional-only signatures are unsupported')
            for dec in fn.decorator_list:
                kind = spelling(dec.func if isinstance(dec, ast.Call) else dec)
                if kind not in ('ac.module', 'ac.rule', 'ac.signal', 'ac.work'):
                    self.error(dec, 'unsupported decorator')
                if isinstance(dec, ast.Call) and (dec.args or any(k.arg != 'capacity' or kind != 'ac.rule' for k in dec.keywords)):
                    self.error(dec, 'only Rule output capacity is configurable')

    def literal(self, node):
        if isinstance(node, ast.Name) and node.id in self.constants:
            return self.constants[node.id]
        try:
            return ast.literal_eval(node)
        except (ValueError, TypeError):
            self.error(node, 'expected a literal construction constant')

    def annotation(self, node):
        if node is None:
            raise CompileError('a value type annotation is required here')
        name = spelling(node).removeprefix('ac.')
        if name in SCALARS or name in self.types:
            return name
        if name == 'None' or isinstance(node, ast.Constant) and node.value is None:
            return 'void'
        if isinstance(node, ast.Subscript):
            kind = spelling(node.value).removeprefix('ac.')
            if kind == 'var':
                return self.annotation(node.slice)
            if kind in ('queue', 'ref', 'signal', 'vector', 'qarray'):
                return wrapped('queue' if kind == 'ref' else kind, self.annotation(node.slice))
            if kind == 'array' and isinstance(node.slice, ast.Tuple):
                return f'array<{self.annotation(node.slice.elts[0])},{self.literal(node.slice.elts[1])}>'
        self.error(node, f'unsupported type: {ast.unparse(node)}')

    def compile(self, path, top):
        self.read(path)
        if top not in self.defs or decorator(self.defs[top]) != 'module':
            raise CompileError(f'top Module not found: {top}')
        self.model['top'] = top
        self.model['constants'] = self.constants
        builder = Lower(self, 'construction', 'construct')
        args = []
        for arg in self.defs[top].args.args:
            typ = self.annotation(arg.annotation)
            if resource(typ):
                self.error(arg, 'top parameters must be constructor values')
            self.model['config'].append(dict(name=arg.arg, type=typ))
            args.append(builder.op('config', typ, name=arg.arg, node=arg))
        self.instantiate(self.defs[top], args, '', builder)
        self.model['construction'] = builder.ops
        # All call sites have now contributed possible resource identities.
        for mod in self.model['modules']:
            for rule in mod['rules']:
                lower = rule.pop('_lower')
                lower.finish_bindings()
            mod['resources'] = sorted(set(mod.pop('_resources')))
        return self.model

    def expand_loops(self, body):
        import copy
        result = []
        for stmt in body:
            if not isinstance(stmt, ast.For):
                result.append(stmt)
                continue
            if stmt.orelse or not isinstance(stmt.target, ast.Name) or not isinstance(stmt.iter, ast.Call) or spelling(stmt.iter.func) != 'range':
                self.error(stmt, 'construction loops require a literal range')
            args = [self.literal(x) for x in stmt.iter.args]
            for index in range(*args):
                variable = stmt.target.id
                class Substitute(ast.NodeTransformer):
                    def visit_Name(self, name):
                        if name.id == variable and isinstance(name.ctx, ast.Load):
                            new = ast.copy_location(ast.Constant(index), name)
                            new.source = getattr(name, 'source', '')
                            return new
                        return name
                expanded = [Substitute().visit(copy.deepcopy(s)) for s in stmt.body]
                result.extend(self.expand_loops(expanded))
        return result

    def bind_args(self, node, values, keywords, defaults=True):
        names = [p.arg for p in node.args.args]
        result = dict(zip(names, values))
        result.update(keywords)
        if len(values) > len(names) or set(result) - set(names):
            self.error(node, 'invalid call arguments')
        if defaults:
            for name, default in zip(names[-len(node.args.defaults):], node.args.defaults):
                result.setdefault(name, default)
        if any(n not in result for n in names):
            self.error(node, 'missing call argument')
        return [result[n] for n in names]

    def instantiate(self, node, args, prefix, builder):
        env = dict(zip((a.arg for a in node.args.args), args))
        mod = dict(name=prefix or 'top', source_name=node.name, rules=[], resources=[], _resources=set())
        local_defs = {}
        work = None
        outputs = None
        return_node = None
        implicit_work = []
        builder.env = env
        for stmt in self.expand_loops(node.body):
            if isinstance(stmt, ast.FunctionDef):
                if decorator(stmt) == 'work':
                    work = stmt
                else:
                    local_defs[stmt.name] = stmt
            elif isinstance(stmt, ast.Return):
                return_node = stmt
            elif isinstance(stmt, (ast.Assign, ast.AnnAssign)):
                target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target
                if isinstance(target, (ast.Tuple, ast.List)):
                    value = self.construct_value(stmt.value, (prefix or 'top') + '_connection' + str(len(self.model['resources'])), builder, local_defs)
                    if not isinstance(value, list) or len(value) != len(target.elts) or any(not isinstance(t, ast.Name) for t in target.elts):
                        self.error(stmt, 'Module output destructuring requires matching names')
                    env.update({t.id: v for t, v in zip(target.elts, value)})
                    builder.env = env
                    continue
                if not isinstance(target, ast.Name):
                    self.error(stmt, 'construction assignment requires a name')
                name = target.id
                value = stmt.value
                key = f'{prefix}_{name}' if prefix else name
                fn = local_defs.get(spelling(value.func), self.defs.get(spelling(value.func))) if isinstance(value, ast.Call) else None
                is_rule = fn is not None and decorator(fn) == 'rule'
                if is_rule or any(isinstance(n, ast.Attribute) and n.attr == 'value' for n in ast.walk(value)):
                    implicit_work.append(stmt)
                    continue
                env[name] = self.construct_value(value, key, builder, local_defs)
                if isinstance(stmt, ast.AnnAssign):
                    env[name] = builder.cast(env[name], self.annotation(stmt.annotation), stmt)
                if not prefix and isinstance(env[name], Value) and env[name].targets:
                    self.model['exports'][name] = sorted(env[name].targets)[0]
                builder.env = env
            elif isinstance(stmt, ast.If):
                implicit_work.append(stmt)
            elif isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Call):
                called = local_defs.get(spelling(stmt.value.func), self.defs.get(spelling(stmt.value.func)))
                if called and decorator(called) == 'rule':
                    implicit_work.append(stmt)
                    continue
                self.construct_value(stmt.value, f'{prefix}_m{len(self.model["modules"])}', builder, local_defs)
                builder.env = env
            elif isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Constant) and isinstance(stmt.value.value, str):
                pass
            else:
                self.error(stmt, 'Module construction supports bindings, declarations, instances and @ac.work')
        # A construction-only wrapper has no Work and no scheduler entry.
        if work or implicit_work:
            self.model['modules'].append(mod)
            lower = Lower(self, 'work', mod['name'] + '_Work', env, builder=builder, module=mod, defs=local_defs)
            lower.statements(implicit_work + (work.body if work else []))
            mod['work'] = lower.function()
            if return_node:
                outputs = lower.expr(return_node.value)
                def fixed(v):
                    if isinstance(v, list):
                        return [fixed(x) for x in v]
                    if v is None:
                        return None
                    if len(v.targets) != 1:
                        self.error(return_node, 'Module returns fixed resource connections')
                    key = next(iter(v.targets))
                    return builder.op('resource', v.type, name=key, targets={key}, node=return_node)
                outputs = fixed(outputs)
        elif return_node:
            outputs = builder.expr(return_node.value)
        return outputs

    def construct_value(self, node, name, builder, defs):
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Subscript) and spelling(node.func.value) == 'ac.queue':
            typ = self.annotation(node.func.slice)
            kw = {x.arg: x.value for x in node.keywords}
            if node.args or set(kw) - {'capacity', 'initial'}:
                self.error(node, 'Queue accepts capacity= and initial= only')
            capacity = builder.expr(kw['capacity']) if 'capacity' in kw else builder.const(1)
            initial = builder.expr(kw['initial'], typ) if 'initial' in kw else None
            # scalar initial is one element; a list is a FIFO initialization sequence.
            values = [] if initial is None else [initial.id]
            sequence = initial is not None and initial.type != typ and initial.type.startswith(('vector<', 'array<'))
            self.model['resources'].append(dict(name=name, kind='queue', type=typ, capacity=capacity.id,
                                                initial=values, sequence=sequence))
            return builder.op('resource', wrapped('queue', typ), name=name, targets={name}, node=node)
        if isinstance(node, ast.List) and any(isinstance(x, ast.Call) and isinstance(x.func, ast.Subscript) and spelling(x.func.value) == 'ac.queue' for x in node.elts):
            vals = [self.construct_value(x, name + '_' + str(i), builder, defs) for i, x in enumerate(node.elts)]
            if not vals or any(v.type != vals[0].type for v in vals):
                self.error(node, 'Queue list elements must have one type')
            return builder.op('aggregate', wrapped('qarray', element(vals[0].type)), vals, targets=set().union(*(v.targets for v in vals)), node=node)
        if isinstance(node, ast.ListComp):
            if len(node.generators) != 1 or node.generators[0].ifs or node.generators[0].is_async:
                self.error(node, 'Queue array requires one construction iterable')
            gen = node.generators[0]
            if not isinstance(gen.target, ast.Name) or not isinstance(node.elt, ast.Call):
                self.error(node, 'unsupported Queue array comprehension')
            call = node.elt
            if not isinstance(call.func, ast.Subscript) or spelling(call.func.value) != 'ac.queue':
                self.error(node, 'construction comprehensions only declare Queue arrays')
            typ = self.annotation(call.func.slice)
            iterable = builder.expr(gen.iter)
            init = Lower(self, 'initializer', name + '_init')
            init.env[gen.target.id] = init.param(gen.target.id, element(iterable.type))
            kw = {x.arg: x.value for x in call.keywords}
            if call.args or set(kw) - {'capacity', 'initial'} or 'initial' not in kw:
                self.error(call, 'Queue array comprehension requires initial= and optional capacity=')
            val = init.expr(kw.get('initial', ast.Constant(0)), typ)
            init.op('return', 'void', [val], node=node)
            cap = builder.expr(kw['capacity']) if 'capacity' in kw else builder.const(1)
            self.model['resources'].append(dict(name=name, kind='qarray', type=typ, iterable=iterable.id,
                                                capacity=cap.id, initializer=init.function()))
            return builder.op('resource', wrapped('qarray', typ), name=name, targets={name}, node=node)
        if isinstance(node, ast.Call):
            fn = defs.get(spelling(node.func), self.defs.get(spelling(node.func)))
            if fn and decorator(fn) in ('module', 'signal'):
                vals = [builder.expr(x) for x in node.args]
                kw = {k.arg: builder.expr(k.value) for k in node.keywords}
                args = self.bind_args(fn, vals, kw)
                args = [builder.expr(x) if isinstance(x, ast.AST) else x for x in args]
                if decorator(fn) == 'module':
                    return self.instantiate(fn, args, name, builder)
                lower = Lower(self, 'signal', name + '_evaluate', {**builder.env, **dict(zip((a.arg for a in fn.args.args), args))}, builder=builder)
                if fn.returns:
                    lower.expected_return = self.annotation(fn.returns)
                lower.statements(fn.body)
                typ = self.annotation(fn.returns) if fn.returns else lower.return_type
                self.model['resources'].append(dict(name=name, kind='signal', type=typ,
                                                    function=lower.function(), inputs=sorted(lower.accesses)))
                return builder.op('resource', wrapped('signal', typ), name=name, targets={name}, node=node)
        return builder.expr(node)

    def helper(self, node):
        if node.name in self.helpers:
            return self.helpers[node.name]
        if not node.returns:
            self.error(node, 'pure helper return type is required')
        typ = self.annotation(node.returns)
        self.helpers[node.name] = typ
        lower = Lower(self, 'helper', node.name)
        for arg in node.args.args:
            lower.env[arg.arg] = lower.param(arg.arg, self.annotation(arg.annotation))
        lower.expected_return = typ
        lower.statements(node.body)
        self.model['functions'].append(lower.function(result=typ))
        return typ


class Lower:
    def __init__(self, front, kind, name, env=None, builder=None, module=None, defs=None):
        self.f, self.kind, self.name = front, kind, name
        self.env = dict(env or {})
        self.builder, self.module, self.defs = builder, module, defs or {}
        self.ops, self.params = [], []
        self.hir = Block()
        self.block = self.hir
        self.guard = None
        self.serial = 0
        self.accesses = set()
        self.return_type = 'void'
        self.imports = {}
        self.effects = []
        self.rule = None

    def loc(self, node):
        return dict(file=getattr(node, 'source', ''), line=getattr(node, 'lineno', 0), column=getattr(node, 'col_offset', 0))

    def op(self, op, typ, args=(), node=None, targets=(), consume=False, **attrs):
        self.serial += 1
        value = Value(f'v{self.serial}', typ, set(targets), consume)
        operation = Operation(op, value.id, typ, [a.id for a in args], self.guard, self.loc(node), attrs)
        self.block.nodes.append(operation)
        self.ops.append(operation.freeze())
        return value

    def function(self, result=None):
        return dict(name=self.name, kind=self.kind, params=self.params,
                    result=result or self.return_type, ops=self.hir.freeze())

    def const(self, value, typ=None, node=None):
        if typ is None:
            typ = 'bool' if isinstance(value, bool) else 'u32'
        return self.op('const', typ, value=value, node=node)

    def param(self, name, typ, targets=(), consume=False):
        value = self.op('param', typ, name=name, targets=targets, consume=consume)
        self.params.append(dict(name=name, type=typ, id=value.id, targets=sorted(targets), consume=consume))
        return value

    def imported(self, val):
        if not isinstance(val, Value) or self.builder is None:
            return val
        if val.id not in self.imports:
            # Import the pure construction DAG by value; resources preserve identity.
            source = next(x for x in self.builder.ops if x['id'] == val.id)
            inputs = []
            for aid in source['args']:
                a = next(x for x in self.builder.ops if x['id'] == aid)
                inputs.append(self.imported(Value(aid, a['type'], {a['name']} if a['op'] == 'resource' else set())))
            attrs = {k: v for k, v in source.items() if k not in ('op', 'id', 'type', 'args', 'guard', 'loc')}
            self.imports[val.id] = self.op(source['op'], val.type, inputs, targets=val.targets, **attrs)
        return self.imports[val.id]

    def lookup(self, name, node):
        if name in self.env:
            val = self.env[name]
            # Imported construction bindings are replaced once in this environment.
            if self.builder and name not in getattr(self, 'locals', set()):
                val = self.imported(val)
            return val
        if name in self.f.constants:
            return self.const(self.f.constants[name], node=node)
        self.f.error(node, f'unknown name: {name}')

    def bind(self, name, val):
        if not hasattr(self, 'locals'):
            self.locals = set()
        self.locals.add(name)
        self.env[name] = val

    def cast(self, value, typ, node=None):
        if value.type == typ:
            return value
        if value.type in SCALARS and typ in SCALARS:
            return self.op('cast', typ, [value], node=node)
        self.f.error(node, f'type mismatch: {value.type} → {typ}')

    def predicate(self, left, right, negate=False):
        old = self.guard
        self.guard = None
        if negate:
            right = self.op('unary', 'bool', [right], operator='not')
        if left is not None:
            right = self.op('binary', 'bool', [Value(left, 'bool'), right], operator='and')
        self.guard = old
        return right.id

    def merge_guard(self, a, b):
        if a is None or b is None:
            return None
        old = self.guard
        self.guard = None
        out = self.op('binary', 'bool', [Value(a, 'bool'), Value(b, 'bool')], operator='or')
        self.guard = old
        return out.id

    def expr(self, node, expected=None):
        if node is None:
            return None
        if isinstance(node, ast.Name):
            return self.lookup(node.id, node)
        if isinstance(node, ast.Constant):
            if node.value is None:
                return None
            if not isinstance(node.value, (bool, int)):
                self.f.error(node, 'only integer and bool values are supported')
            return self.const(node.value, expected, node)
        if isinstance(node, ast.Tuple):
            return [self.expr(x) for x in node.elts]
        if isinstance(node, ast.List):
            et = element(expected) if expected and expected.startswith(('array<', 'vector<')) else expected
            vals = [self.expr(x, et) for x in node.elts]
            et = et or (vals[0].type if vals else 'u32')
            typ = expected if expected and expected.startswith('array<') else f'vector<{et}>'
            targets = set().union(*(v.targets for v in vals))
            if et.startswith('queue<'):
                typ = wrapped('qarray', element(et))
            return self.op('aggregate', typ, vals, node=node, targets=targets)
        if isinstance(node, ast.Attribute):
            val = self.expr(node.value)
            if node.attr == 'value' and resource(val.type):
                self.accesses.update(val.targets)
                if val.type.startswith('signal<'):
                    if self.kind == 'signal':
                        self.f.error(node, 'Signals may read Queue inputs, not other Signals')
                    return self.op('signal.read', element(val.type), [val], node=node)
                if not val.type.startswith('queue<'):
                    self.f.error(node, 'index Queue arrays before reading')
                out = self.op('queue.read', element(val.type), [val], node=node)
                if self.kind == 'rule' and val.consume:
                    self.op('queue.pop', 'void', [val], node=node)
                    self.effects.append((val, 'Pop'))
                return out
            fields = self.f.types.get(val.type, [])
            field = next((x for x in fields if x['name'] == node.attr), None)
            if not field:
                self.f.error(node, f'no field {node.attr} on {val.type}')
            return self.op('field', field['type'], [val], field=node.attr, node=node)
        if isinstance(node, ast.Subscript):
            val = self.expr(node.value)
            idx = self.expr(node.slice)
            if not val.type.startswith(('array<', 'vector<', 'qarray<')):
                self.f.error(node, f'cannot index {val.type}')
            typ = wrapped('queue', element(val.type)) if val.type.startswith('qarray<') else element(val.type)
            return self.op('index', typ, [val, idx], node=node, targets=val.targets, consume=val.consume)
        if isinstance(node, ast.BinOp):
            a, b = self.expr(node.left, expected), self.expr(node.right, expected)
            typ = expected or self.common(a, b, node)
            a, b = self.cast(a, typ, node), self.cast(b, typ, node)
            operators = {ast.Add: '+', ast.Sub: '-', ast.Mult: '*', ast.FloorDiv: '//', ast.Mod: '%',
                         ast.BitAnd: '&', ast.BitOr: '|', ast.BitXor: '^', ast.LShift: '<<', ast.RShift: '>>'}
            if type(node.op) not in operators:
                self.f.error(node, 'unsupported arithmetic operator')
            return self.op('binary', typ, [a, b], operator=operators[type(node.op)], node=node)
        if isinstance(node, ast.UnaryOp):
            a = self.expr(node.operand, expected)
            operators = {ast.Not: 'not', ast.Invert: '~', ast.USub: '-', ast.UAdd: '+'}
            return self.op('unary', 'bool' if isinstance(node.op, ast.Not) else a.type,
                           [a], operator=operators[type(node.op)], node=node)
        if isinstance(node, ast.BoolOp):
            outer = self.guard
            out = self.cast(self.expr(node.values[0]), 'bool', node)
            for rhs in node.values[1:]:
                self.guard = self.predicate(outer, out, isinstance(node.op, ast.Or))
                other = self.cast(self.expr(rhs), 'bool', rhs)
                self.guard = outer
                fallback = self.const(isinstance(node.op, ast.Or))
                out = self.op('select', 'bool', [out, fallback, other] if isinstance(node.op, ast.Or)
                              else [out, other, fallback], node=node)
            return out
        if isinstance(node, ast.Compare):
            a = self.expr(node.left)
            outer, out = self.guard, None
            for op, rhs in zip(node.ops, node.comparators):
                if out:
                    self.guard = self.predicate(outer, out)
                if isinstance(op, (ast.In, ast.NotIn)):
                    if not isinstance(rhs, (ast.Tuple, ast.List)):
                        self.f.error(rhs, 'membership requires a literal tuple/list')
                    parts = [self.op('binary', 'bool', [a, self.cast(self.expr(x, a.type), a.type, x)], operator='==', node=node) for x in rhs.elts]
                    value = self.const(False)
                    for part in parts:
                        value = self.op('binary', 'bool', [value, part], operator='or', node=node)
                    if isinstance(op, ast.NotIn):
                        value = self.op('unary', 'bool', [value], operator='not', node=node)
                else:
                    b = self.expr(rhs, a.type)
                    typ = self.common(a, b, node)
                    ops = {ast.Eq: '==', ast.NotEq: '!=', ast.Lt: '<', ast.LtE: '<=', ast.Gt: '>', ast.GtE: '>='}
                    if type(op) not in ops:
                        self.f.error(node, 'unsupported comparison')
                    value = self.op('binary', 'bool', [self.cast(a, typ), self.cast(b, typ)], operator=ops[type(op)], node=node)
                    a = b
                self.guard = outer
                out = value if out is None else self.op('binary', 'bool', [out, value], operator='and', node=node)
            return out
        if isinstance(node, ast.IfExp):
            c = self.cast(self.expr(node.test), 'bool', node)
            outer = self.guard
            self.guard = self.predicate(outer, c)
            a = self.expr(node.body, expected)
            self.guard = self.predicate(outer, c, True)
            b = self.expr(node.orelse, expected or a.type)
            self.guard = outer
            if resource(a.type) and a.consume != b.consume:
                self.f.error(node, 'resource selection must preserve message/observation roles; branch around the reads')
            return self.op('select', a.type, [c, a, self.cast(b, a.type)], node=node,
                           targets=a.targets | b.targets, consume=a.consume or b.consume)
        if isinstance(node, ast.Call):
            return self.call(node, expected)
        self.f.error(node, f'unsupported expression: {type(node).__name__}')

    def common(self, a, b, node):
        if a.type == b.type:
            return a.type
        if a.type in SCALARS and b.type in SCALARS:
            # Literals take the other operand's width; mixed signedness is explicit.
            for val, other in ((a, b), (b, a)):
                op = next((x for x in self.ops if x['id'] == val.id), {})
                if op.get('op') == 'const':
                    return other.type
        self.f.error(node, f'mixed types need an explicit cast: {a.type}, {b.type}')

    def call(self, node, expected):
        name = spelling(node.func)
        short = name.removeprefix('ac.')
        if short in SCALARS:
            return self.cast(self.expr(node.args[0]), short, node)
        if isinstance(node.func, ast.Subscript) and spelling(node.func.value) == 'ac.var':
            typ = self.f.annotation(node.func.slice)
            return self.cast(self.expr(node.args[0], typ), typ, node)
        if name == 'len':
            return self.op('length', 'u32', [self.expr(node.args[0])], node=node)
        if name == 'range' and self.kind == 'construction':
            vals = [self.expr(x) for x in node.args]
            if len(vals) != 1:
                self.f.error(node, 'construction range currently takes one bound')
            return self.op('range', 'vector<u32>', vals, node=node)
        if name in ('ac.wakeup', 'ac.requestWakeup'):
            if self.kind != 'rule':
                self.f.error(node, 'wakeup is a Rule effect')
            if len(node.args) != 1:
                self.f.error(node, 'wakeup(delay) targets the owning Module')
            delay = self.expr(node.args[0], 'u64')
            return self.op('event', 'void', [delay], node=node)
        if isinstance(node.func, ast.Attribute) and node.func.attr in ('empty', 'full', 'size'):
            q = self.expr(node.func.value)
            self.accesses.update(q.targets)
            return self.op('queue.' + node.func.attr, 'u32' if node.func.attr == 'size' else 'bool', [q], node=node)
        if name in self.f.types:
            fields = self.f.types[name]
            provided = dict(zip((x['name'] for x in fields), node.args))
            provided.update({k.arg: k.value for k in node.keywords})
            if set(provided) - {f['name'] for f in fields}:
                self.f.error(node, 'unknown struct field')
            vals = []
            for f in fields:
                if f['name'] in provided:
                    vals.append(self.cast(self.expr(provided[f['name']], f['type']), f['type'], node))
                elif f['default'] is not None:
                    vals.append(self.const(f['default'], f['type'], node))
                else:
                    vals.append(self.op('zero', f['type'], node=node))
            return self.op('aggregate', name, vals, node=node)
        fn = self.defs.get(name, self.f.defs.get(name))
        if not fn:
            self.f.error(node, f'unsupported call: {name}')
        args = [self.expr(x) for x in node.args]
        kw = {k.arg: self.expr(k.value) for k in node.keywords}
        args = self.f.bind_args(fn, args, kw)
        args = [self.expr(x) if isinstance(x, ast.AST) else x for x in args]
        if decorator(fn) == 'rule':
            if self.kind != 'work':
                self.f.error(node, 'Rules can only be selected by Module Work')
            return self.rule_call(fn, args, node)
        if decorator(fn) != 'helper':
            self.f.error(node, 'resource and Module declarations belong to construction')
        typ = self.f.helper(fn)
        vals = [self.cast(v, self.f.annotation(p.annotation), node) for p, v in zip(fn.args.args, args)]
        return self.op('call', typ, vals, name=fn.name, node=node)

    def rule_call(self, fn, args, node):
        rule = next((r for r in self.module['rules'] if r['name'] == fn.name), None)
        if rule is None:
            rule = dict(name=fn.name, outputs=[], bindings={}, _lower=None)
            self.module['rules'].append(rule)
            captured = {k: v for k, v in self.env.items() if k not in getattr(self, 'locals', set())}
            lower = Lower(self.f, 'rule', self.module['name'] + '_' + fn.name, captured,
                          builder=self.builder, module=self.module, defs=self.defs)
            lower.rule = rule
            for p, v in zip(fn.args.args, args):
                typ = self.f.annotation(p.annotation) if p.annotation else v.type
                consume = resource(typ) and not (p.annotation and spelling(p.annotation.value if isinstance(p.annotation, ast.Subscript) else p.annotation) == 'ac.ref')
                pv = lower.param(p.arg, typ, v.targets, consume)
                lower.bind(p.arg, pv)
            # Work locals used by closure become explicit value/resource cache arguments.
            used = {x.id for x in ast.walk(fn) if isinstance(x, ast.Name) and isinstance(x.ctx, ast.Load)}
            captures = sorted(used & getattr(self, 'locals', set()) - {p.arg for p in fn.args.args})
            rule['captures'] = captures
            for name in captures:
                v = self.env[name]
                lower.bind(name, lower.param(name, v.type, v.targets, False))
            lower.statements(fn.body)
            lower.op('complete', 'void', node=fn)
            rule['function'] = lower.function()
            rule['_lower'] = lower
        lower = rule['_lower']
        vals = args + [self.env[n] for n in rule['captures']]
        for p, v in zip(lower.params, vals):
            p['targets'] = sorted(set(p['targets']) | v.targets)
            lower.env[p['name']].targets.update(v.targets)
        vals = [self.cast(v, p['type'], node) for p, v in zip(lower.params, vals)]
        self.op('rule.call', 'void', vals, name=fn.name, node=node)
        outs = [self.op('resource', wrapped('queue', r['type']), name=r['name'], targets={r['name']}, node=node) if r else None for r in rule['outputs']]
        return outs[0] if len(outs) == 1 else outs if outs else None

    def finish_bindings(self):
        # Propagate provenance after all calls, including dynamic indexing/selection.
        origins = {}
        params = {p['id']: set(p['targets']) for p in self.params}
        reads = set()
        bindings = {}
        for op in self.ops:
            sources = set().union(*(origins.get(a, set()) for a in op['args']))
            if op['op'] == 'resource':
                sources = {op['name']}
            if op['op'] == 'param':
                sources = params[op['id']]
            origins[op['id']] = sources if resource(op['type']) else set()
            if op['op'].startswith(('queue.', 'signal.')):
                reads.update(sources)
                effect = {'queue.pop': 'Pop', 'queue.push': 'Push', 'queue.revise': 'Revise'}.get(op['op'])
                if effect:
                    for target in sources:
                        bindings.setdefault(target, set()).add(effect)
        self.rule['bindings'] = {k: sorted(v) for k, v in bindings.items()}
        self.module['_resources'].update(reads)

    def target_type(self, target):
        if isinstance(target, ast.Name):
            return self.lookup(target.id, target).type
        if isinstance(target, ast.Attribute):
            typ = self.target_type(target.value)
            if target.attr == 'value' and typ.startswith('queue<'):
                return element(typ)
            fields = self.f.types.get(typ, [])
            field = next((f for f in fields if f['name'] == target.attr), None)
            if field:
                return field['type']
        if isinstance(target, ast.Subscript):
            typ = self.target_type(target.value)
            return wrapped('queue', element(typ)) if typ.startswith('qarray<') else element(typ)
        self.f.error(target, 'unsupported assignment type')

    def assignment(self, target, value, node):
        if isinstance(target, ast.Name):
            self.bind(target.id, value)
            return
        if isinstance(target, (ast.Tuple, ast.List)):
            if not isinstance(value, list) or len(value) != len(target.elts):
                self.f.error(node, 'tuple assignment arity mismatch')
            for t, v in zip(target.elts, value):
                self.assignment(t, v, node)
            return
        if isinstance(target, ast.Attribute):
            path, base = [], target
            while isinstance(base, ast.Attribute):
                path.insert(0, base.attr)
                base = base.value
            q = self.expr(base)
            if q.type.startswith('queue<') and path and path[0] == 'value':
                if self.kind != 'rule':
                    self.f.error(node, 'Queue updates are Rule effects')
                self.op('queue.revise', 'void', [q, value], path=path[1:], node=node)
                self.effects.append((q, 'Revise'))
                return
            # Ordinary struct update produces a new SSA value.
            path, base = [], target
            while isinstance(base, ast.Attribute):
                path.insert(0, base.attr)
                base = base.value
            if not isinstance(base, ast.Name):
                self.f.error(node, 'local field update requires a named struct')
            val = self.lookup(base.id, node)
            result = self.op('update', val.type, [val, value], path=path, node=node)
            self.bind(base.id, result)
            return
        if isinstance(target, ast.Subscript) and isinstance(target.value, ast.Name):
            old = self.lookup(target.value.id, node)
            idx = self.expr(target.slice)
            self.bind(target.value.id, self.op('array.update', old.type, [old, idx, value], node=node))
            return
        self.f.error(node, 'unsupported assignment target')

    def statements(self, statements):
        for node in self.f.expand_loops(statements):
            if isinstance(node, (ast.Assign, ast.AnnAssign)):
                targets = node.targets if isinstance(node, ast.Assign) else [node.target]
                typ = self.f.annotation(node.annotation) if isinstance(node, ast.AnnAssign) else None
                if typ is None and isinstance(targets[0], (ast.Attribute, ast.Subscript)):
                    typ = self.target_type(targets[0])
                val = self.expr(node.value, typ)
                if typ:
                    val = self.cast(val, typ, node)
                for t in targets:
                    self.assignment(t, val, node)
            elif isinstance(node, ast.AugAssign):
                old = self.expr(node.target)
                rhs = self.expr(node.value, old.type)
                operators = {ast.Add: '+', ast.Sub: '-', ast.BitOr: '|', ast.BitAnd: '&', ast.BitXor: '^', ast.Mult: '*'}
                if type(node.op) not in operators:
                    self.f.error(node, 'unsupported augmented assignment')
                val = self.op('binary', old.type, [old, self.cast(rhs, old.type)], operator=operators[type(node.op)], node=node)
                self.assignment(node.target, val, node)
            elif isinstance(node, ast.If):
                cond = self.cast(self.expr(node.test), 'bool', node)
                outer, before = self.guard, dict(self.env)
                parent = self.block
                branch = Branch(cond.id, self.loc(node))
                parent.nodes.append(branch)
                self.block = branch.yes
                locals_before = set(getattr(self, 'locals', set()))
                self.guard = self.predicate(outer, cond)
                self.statements(node.body)
                yes, yes_guard, yes_locals = dict(self.env), self.guard, set(getattr(self, 'locals', set()))
                self.env, self.locals = dict(before), set(locals_before)
                self.block = branch.no
                self.guard = self.predicate(outer, cond, True)
                self.statements(node.orelse)
                no, no_guard = dict(self.env), self.guard
                self.block = parent
                self.guard = self.merge_guard(yes_guard, no_guard)
                self.env = dict(before)
                for name in sorted(yes.keys() & no.keys()):
                    a, b = yes[name], no[name]
                    if not isinstance(a, Value) or not isinstance(b, Value):
                        continue
                    if a.id == b.id:
                        self.env[name] = a
                    elif name in yes_locals or name in self.locals:
                        # Construction values used only on one branch must be imported before merging.
                        if name not in yes_locals and self.builder:
                            a = self.imported(a)
                        if name not in self.locals and self.builder:
                            b = self.imported(b)
                        if resource(a.type) and a.consume != b.consume:
                            self.f.error(node, 'resource merge must preserve message/observation roles; branch around the reads')
                        self.bind(name, self.op('select', a.type, [cond, a, self.cast(b, a.type)], node=node,
                                               targets=a.targets | b.targets, consume=a.consume or b.consume))
                self.locals |= yes_locals
            elif isinstance(node, ast.Return):
                val = self.expr(node.value)
                if self.kind == 'work':
                    if val is not None:
                        self.f.error(node, 'Work returns no payload; Module construction exports resources')
                    self.guard = self.predicate(self.guard, self.const(False))
                    continue
                if self.kind == 'rule':
                    values = val if isinstance(val, list) else [val]
                    for index, v in enumerate(values):
                        if v is None:
                            continue
                        while len(self.rule['outputs']) <= index:
                            self.rule['outputs'].append(None)
                        out = self.rule['outputs'][index]
                        if out is None:
                            name = self.name + f'_out{index}'
                            cap = 1
                            fn = self.defs.get(self.rule['name'], self.f.defs.get(self.rule['name']))
                            for dec in fn.decorator_list:
                                if isinstance(dec, ast.Call):
                                    cap = next((self.f.literal(k.value) for k in dec.keywords if k.arg == 'capacity'), 1)
                            c = self.builder.const(cap)
                            out = dict(name=name, kind='queue', type=v.type, capacity=c.id, initial=[], sequence=False)
                            self.f.model['resources'].append(out)
                            self.rule['outputs'][index] = out
                        q = self.op('resource', wrapped('queue', v.type), name=out['name'], targets={out['name']}, node=node)
                        self.op('queue.push', 'void', [q, self.cast(v, out['type'])], node=node)
                    self.guard = self.predicate(self.guard, self.const(False))
                else:
                    if val is None or isinstance(val, list):
                        self.f.error(node, 'helpers and Signals return one typed value')
                    if getattr(self, 'expected_return', None):
                        val = self.cast(val, self.expected_return, node)
                    self.return_type = val.type
                    self.op('return', 'void', [val], node=node)
                    self.guard = self.predicate(self.guard, self.const(False))
            elif isinstance(node, ast.Expr):
                if isinstance(node.value, ast.Constant) and isinstance(node.value.value, str):
                    continue
                self.expr(node.value)
            elif isinstance(node, ast.Assert):
                condition = self.cast(self.expr(node.test), 'bool', node)
                self.op('check', 'void', [condition], node=node)
            elif isinstance(node, ast.Pass):
                pass
            else:
                self.f.error(node, f'unsupported statement: {type(node).__name__}')
        if self.module:
            self.module['_resources'].update(self.accesses)


def compile_source(path, top):
    return Frontend().compile(path, top)

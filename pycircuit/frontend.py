"""AST-only static elaboration and typed MLIR control-flow construction.

Construction is symbolic: host configuration values are constructor arguments,
never evaluated by Python. Only literal construction loops are expanded here.
"""
import ast
from pathlib import Path
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
                if kind not in ('ac.module', 'ac.rule', 'ac.signal'):
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
                rule.pop('_lower')
            mod['resources'] = sorted(set(mod.pop('_resources')))
            mod.pop('_outputs', None)
            mod.pop('_pending_outputs', None)
            mod.pop('_vars', None)
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
        var_inputs = {}
        for parameter, value in zip(node.args.args, args):
            if isinstance(parameter.annotation, ast.Subscript) and spelling(parameter.annotation.value) == 'ac.var':
                expected = self.annotation(parameter.annotation)
                if not isinstance(value, Value) or value.type != wrapped('signal', expected):
                    self.error(parameter, 'Module var inputs must bind a Signal of the annotated type')
                var_inputs[parameter.arg] = expected
        mod = dict(name=prefix or 'top', source_name=node.name, rules=[], resources=[], _resources=set(), _vars=var_inputs)
        local_defs = {s.name: s for s in node.body if isinstance(s, ast.FunctionDef)}
        def function(call):
            return local_defs.get(spelling(call.func), self.defs.get(spelling(call.func))) if isinstance(call, ast.Call) else None
        def resource_declaration(n):
            return isinstance(n, ast.Call) and (isinstance(n.func, ast.Subscript) and spelling(n.func.value) == 'ac.queue' or spelling(n.func) == 'ac.array')
        def static_instance(n):
            fn = function(n)
            return resource_declaration(n) or fn is not None and decorator(fn) in ('module', 'signal')
        body = []
        for stmt in node.body:
            if isinstance(stmt, ast.For) and any(static_instance(n) for n in ast.walk(stmt)):
                body.extend(self.expand_loops([stmt]))
            else:
                body.append(stmt)
        runtime, pending = [], []
        return_node = None
        runtime_names = set()
        def runtime_expr(value):
            for n in ast.walk(value):
                if isinstance(n, ast.Name) and n.id in runtime_names: return True
                if isinstance(n, ast.Attribute) and n.attr in ('value', 'empty', 'full', 'size'): return True
                fn = function(n)
                if fn and decorator(fn) == 'rule': return True
            return False
        for stmt in body:
            if isinstance(stmt, ast.FunctionDef):
                continue
            if isinstance(stmt, ast.Return) and stmt.value is not None:
                return_node = stmt
                continue
            if isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Constant) and isinstance(stmt.value.value, str):
                continue
            if isinstance(stmt, (ast.Assign, ast.AnnAssign, ast.Expr)):
                value = stmt.value
                if static_instance(value) or isinstance(value, (ast.ListComp, ast.List)) and any(resource_declaration(n) for n in ast.walk(value)):
                    pending.append(stmt)
                    continue
                if not runtime_expr(value) and not isinstance(stmt, ast.Expr):
                    # Pure bindings whose inputs are already static are constructor values.
                    names = {n.id for n in ast.walk(value) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Load)}
                    if names <= env.keys() | self.defs.keys() | self.types.keys() | self.constants.keys() | {'ac', 'range', 'len', 'bool'}:
                        pending.append(stmt)
                        continue
            runtime.append(stmt)
            runtime_names.update(n.id for n in ast.walk(stmt) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store))
        def bind(target, value):
            if isinstance(target, ast.Name): env[target.id] = value
            elif isinstance(target, (ast.Tuple, ast.List)) and isinstance(value, list) and len(target.elts) == len(value):
                for t, v in zip(target.elts, value): bind(t, v)
            else: self.error(target, 'static connections require matching names or tuples')
        # Resolve static declarations/instances to a fixed point. Output resources
        # are available to both sibling instances and Work before behavior lowering.
        while pending:
            progress = False
            for stmt in pending[:]:
                value = stmt.value
                loaded = {n.id for n in ast.walk(value) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Load)}
                provided = env.keys() | self.defs.keys() | local_defs.keys() | self.types.keys() | self.constants.keys() | {'ac', 'range', 'len', 'bool'}
                if isinstance(value, ast.ListComp): provided |= {g.target.id for g in value.generators}
                if loaded - provided: continue
                target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target if isinstance(stmt, ast.AnnAssign) else None
                name = target.id if isinstance(target, ast.Name) else 'connection' + str(len(self.model['resources']))
                key = (prefix + '_' if prefix else '') + name if target else f'{prefix}_m{len(self.model["modules"])}'
                builder.env = env
                val = self.construct_value(value, key, builder, local_defs)
                builder.env = env
                if isinstance(stmt, ast.AnnAssign): val = builder.cast(val, self.annotation(stmt.annotation), stmt)
                if target: bind(target, val)
                pending.remove(stmt); progress = True
            self.predeclare_outputs(runtime, env, local_defs, mod, builder)
            if not progress and pending:
                if any((fn := function(stmt.value)) and decorator(fn) == 'signal' for stmt in pending):
                    self.error(pending[0], 'unresolved static Signal connection; check for a Signal dependency cycle or undefined input')
                self.error(pending[0], 'static connection/type cycle; add a Queue or Rule return type annotation')
        self.predeclare_outputs(runtime, env, local_defs, mod, builder)
        for name, val in env.items():
            if not prefix and isinstance(val, Value) and val.targets:
                self.model['exports'][name] = sorted(val.targets)[0]
        outputs = None
        if runtime:
            self.model['modules'].append(mod)
            lower = Lower(self, 'work', mod['name'] + '_Work', env, builder=builder, module=mod, defs=local_defs)
            lower.statements(runtime)
            # Module return exports static connections independently of Work paths.
            if return_node:
                def connection(n):
                    if isinstance(n, (ast.Tuple, ast.List)): return [connection(x) for x in n.elts]
                    if isinstance(n, ast.Constant) and n.value is None: return None
                    if not isinstance(n, ast.Name): self.error(n, 'Module returns fixed resource connections')
                    v = lower.env.get(n.id, env.get(n.id))
                    if isinstance(v, list): return v
                    if v is None: return None
                    if len(v.targets) != 1: self.error(n, 'Module returns fixed resource connections')
                    key = next(iter(v.targets))
                    return builder.op('resource', v.type, name=key, targets={key}, node=n)
                outputs = connection(return_node.value)
            mod['work'] = lower.function()
        elif return_node:
            builder.env = env
            outputs = builder.expr(return_node.value)
        return outputs

    def output_shape(self, fn):
        if fn.returns and self.annotation(fn.returns) != 'void': return 0
        def shape(node, counter):
            if isinstance(node, (ast.Tuple, ast.List)):
                return [shape(x, counter) for x in node.elts]
            index = counter[0]
            counter[0] += 1
            return index
        found = None
        for ret in (n for n in ast.walk(fn) if isinstance(n, ast.Return)):
            if ret.value is None or isinstance(ret.value, ast.Constant) and ret.value.value is None: continue
            current = shape(ret.value, [0])
            if found is not None and current != found:
                self.error(ret, 'Rule returns must have one fixed output structure')
            found = current
        return found

    def reshape_outputs(self, values, shape):
        if shape is None: return None
        if isinstance(shape, list): return [self.reshape_outputs(values, s) for s in shape]
        return values[shape] if shape < len(values) else None

    def infer_outputs(self, fn, arguments, captures):
        """Monotone type discovery; it never executes an ACPy function."""
        env = dict(captures)
        env.update({p.arg: self.annotation(p.annotation) if p.annotation else t for p, t in zip(fn.args.args, arguments)})
        outputs = []
        def expression(n, scope):
            if n is None or isinstance(n, ast.Constant) and n.value is None:
                return None
            if isinstance(n, ast.Name):
                if n.id in self.constants:
                    return 'bool' if isinstance(self.constants[n.id], bool) else 'u32'
                return scope[n.id]
            if isinstance(n, ast.Constant):
                return 'bool' if isinstance(n.value, bool) else 'u32'
            if isinstance(n, ast.Tuple):
                return [expression(x, scope) for x in n.elts]
            if isinstance(n, ast.List):
                e = expression(n.elts[0], scope)
                return wrapped('qarray', element(e)) if e.startswith('queue<') else wrapped('vector', e)
            if isinstance(n, ast.Attribute):
                t = expression(n.value, scope)
                if n.attr == 'value' and resource(t):
                    return element(t)
                return next(f['type'] for f in self.types[t] if f['name'] == n.attr)
            if isinstance(n, ast.Subscript):
                t = expression(n.value, scope)
                return wrapped('queue', element(t)) if t.startswith('qarray<') else element(t)
            if isinstance(n, (ast.Compare, ast.BoolOp)) or isinstance(n, ast.UnaryOp) and isinstance(n.op, ast.Not):
                return 'bool'
            if isinstance(n, ast.UnaryOp):
                return expression(n.operand, scope)
            if isinstance(n, ast.BinOp):
                return expression(n.right if isinstance(n.left, ast.Constant) else n.left, scope)
            if isinstance(n, ast.IfExp):
                return expression(n.body, scope) or expression(n.orelse, scope)
            if isinstance(n, ast.Call):
                name = spelling(n.func).removeprefix('ac.')
                if name in SCALARS or name in self.types:
                    return name
                if name == 'len' or isinstance(n.func, ast.Attribute) and n.func.attr == 'size':
                    return 'u32'
                if isinstance(n.func, ast.Attribute) and n.func.attr in ('empty', 'full'):
                    return 'bool'
                if isinstance(n.func, ast.Subscript) and spelling(n.func.value) == 'ac.var':
                    return self.annotation(n.func.slice)
                helper = self.defs.get(name)
                if helper and helper.returns:
                    return self.annotation(helper.returns)
            raise KeyError(ast.unparse(n))
        def assign(target, t, scope):
            if isinstance(target, ast.Name): scope[target.id] = t
            elif isinstance(target, (ast.Tuple, ast.List)) and isinstance(t, list):
                for a, b in zip(target.elts, t): assign(a, b, scope)
        def visit(body, scope):
            for stmt in body:
                if isinstance(stmt, (ast.Assign, ast.AnnAssign)):
                    t = self.annotation(stmt.annotation) if isinstance(stmt, ast.AnnAssign) else expression(stmt.value, scope)
                    for target in stmt.targets if isinstance(stmt, ast.Assign) else [stmt.target]: assign(target, t, scope)
                elif isinstance(stmt, ast.If):
                    a, b = dict(scope), dict(scope)
                    visit(stmt.body, a); visit(stmt.orelse, b)
                    scope.update({n: a[n] for n in a.keys() & b.keys() if a[n] == b[n]})
                elif isinstance(stmt, ast.For):
                    scope[stmt.target.id] = 'u32'
                    visit(stmt.body, scope)
                elif isinstance(stmt, ast.Return):
                    def leaves(n):
                        if isinstance(n, (ast.Tuple, ast.List)) and decorator(fn) == 'rule':
                            return [v for child in n.elts for v in leaves(child)]
                        value = expression(n, scope)
                        return value if isinstance(value, list) else [value]
                    values = leaves(stmt.value)
                    while len(outputs) < len(values): outputs.append(None)
                    for i, t in enumerate(values):
                        if t is not None:
                            if outputs[i] is not None and outputs[i] != t:
                                self.error(stmt, 'inconsistent Rule output types')
                            outputs[i] = t
        if fn.returns:
            t = self.annotation(fn.returns)
            if t != 'void': return [t]
        visit(fn.body, env)
        return outputs

    def predeclare_outputs(self, body, env, defs, mod, builder):
        def nodes(body):
            for stmt in body:
                if isinstance(stmt, ast.FunctionDef):
                    continue
                yield stmt
                if isinstance(stmt, ast.If):
                    yield from nodes(stmt.body); yield from nodes(stmt.orelse)
                elif isinstance(stmt, ast.For): yield from nodes(stmt.body)
        calls = []
        for stmt in nodes(body):
            value = stmt.value if isinstance(stmt, (ast.Assign, ast.AnnAssign, ast.Expr)) else None
            fn = defs.get(spelling(value.func), self.defs.get(spelling(value.func))) if isinstance(value, ast.Call) else None
            if fn and decorator(fn) == 'rule':
                target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target if isinstance(stmt, ast.AnnAssign) else None
                calls.append((fn, value, target))
        mod.setdefault('_outputs', {})
        def type_of(v): return [type_of(x) for x in v] if isinstance(v, list) else v.type if isinstance(v, Value) else None
        pending = list(calls)
        while pending:
            progress = False
            for fn, call, target in pending[:]:
                try:
                    # Infer ordinary Work values as well as fixed construction bindings.
                    scope = {n: type_of(v) for n, v in env.items()}
                    fake = ast.FunctionDef(name='inference', args=ast.arguments(posonlyargs=[],args=[],kwonlyargs=[],kw_defaults=[],defaults=[]), body=[], decorator_list=[])
                    # Parameter types are commonly explicit or fixed resource arguments.
                    arguments = []
                    supplied = self.bind_args(fn, call.args, {k.arg: k.value for k in call.keywords})
                    for param, value in zip(fn.args.args, supplied):
                        if param.annotation: arguments.append(self.annotation(param.annotation))
                        else:
                            probe = ast.FunctionDef(name='probe', args=fake.args, body=[ast.Return(value)], decorator_list=[], returns=None)
                            arguments.append(self.infer_outputs(probe, [], scope)[0])
                    types = self.infer_outputs(fn, arguments, scope)
                except (KeyError, StopIteration):
                    continue
                outputs = mod['_outputs'].get(fn.name)
                if outputs is None:
                    outputs = []
                    def leaves(t):
                        return [x for child in t.elts for x in leaves(child)] if isinstance(t, (ast.Tuple, ast.List)) else [t]
                    targets = leaves(target)
                    for i, t in enumerate(types):
                        binding = targets[i] if i < len(targets) else None
                        old = env.get(binding.id) if isinstance(binding, ast.Name) else None
                        if t is None and isinstance(old, Value) and old.type.startswith('queue<'): t = element(old.type)
                        if t is None:
                            outputs.append(None); continue
                        if isinstance(old, Value) and old.type.startswith('queue<'):
                            if element(old.type) != t: self.error(call, 'output payload does not match explicit Queue type')
                            out = dict(name=next(iter(old.targets)), type=t, reference=old.id, targets=sorted(old.targets))
                        else:
                            name = mod['name'] + '_' + fn.name + '_out' + str(i)
                            cap = 1
                            for dec in fn.decorator_list:
                                if isinstance(dec, ast.Call):
                                    cap = next((self.literal(k.value) for k in dec.keywords if k.arg == 'capacity'), 1)
                            if not isinstance(cap, int) or cap < 1: self.error(call, 'Rule default capacity must be a positive integer; declare separate output Queues for different capacities')
                            out = dict(name=name, kind='queue', type=t, capacity=builder.const(cap).id, initial=[], sequence=False)
                            self.model['resources'].append(out)
                        outputs.append(out)
                    mod['_outputs'][fn.name] = outputs
                vals = [Value(r['reference'], wrapped('queue', r['type']), set(r['targets'])) if r and 'reference' in r else builder.op('resource', wrapped('queue', r['type']), name=r['name'], targets={r['name']}, node=call) if r else None for r in outputs]
                def bind(target, value):
                    if isinstance(target, ast.Name): env[target.id] = value
                    elif isinstance(target, (ast.Tuple, ast.List)):
                        if not isinstance(value, list) or len(target.elts) != len(value): self.error(target, 'Rule output arity mismatch')
                        for a, b in zip(target.elts, value): bind(a, b)
                if target: bind(target, self.reshape_outputs(vals, self.output_shape(fn)))
                pending.remove((fn, call, target)); progress = True
            if not progress:
                # A later Work capture can be typed during Work lowering; true output
                # cycles need an explicit return or Queue annotation.
                mod['_pending_outputs'] = pending
                break

    def construct_value(self, node, name, builder, defs):
        if isinstance(node, ast.Call) and spelling(node.func) == 'ac.array':
            if len(node.args) != 1 or not isinstance(node.args[0], ast.Subscript) or spelling(node.args[0].value) != 'ac.queue':
                self.error(node, 'resource arrays use ac.array(ac.queue[T], shape=(N,), ...)')
            kw = {k.arg: k.value for k in node.keywords}
            if set(kw) - {'shape', 'capacity', 'initial'} or 'shape' not in kw:
                self.error(node, 'Queue array requires shape= and accepts capacity= and initial=')
            shape = kw['shape']
            if not isinstance(shape, ast.Tuple) or len(shape.elts) != 1:
                self.error(shape, 'resource arrays currently require one fixed dimension')
            typ = self.annotation(node.args[0].slice)
            iterable = builder.op('range', 'vector<u32>', [builder.expr(shape.elts[0], 'u32')], node=node)
            cap = builder.expr(kw['capacity']) if 'capacity' in kw else builder.const(1)
            init = Lower(self, 'initializer', name + '_init')
            index = init.param('index', 'u32')
            init.bind('index', index)
            initial = kw.get('initial')
            if isinstance(initial, ast.Name) and initial.id in self.defs:
                fn = self.defs[initial.id]
                result_type = self.helper(fn)
                value = init.op('call', result_type, [index], name=fn.name, node=node)
            elif initial is not None:
                value = init.expr(initial, typ)
            else:
                value = init.op('zero', typ, node=node)
            value = init.cast(value, typ, node)
            init.op('return', 'void', [value], node=node)
            self.model['resources'].append(dict(name=name, kind='qarray', type=typ, iterable=iterable.id,
                                               capacity=cap.id, initializer=init.function(), empty_initial=initial is None))
            return builder.op('resource', wrapped('qarray', typ), name=name, targets={name}, node=node)
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
                if any(not isinstance(v, Value) or not v.type.startswith(('queue<', 'qarray<', 'signal<')) for v in args):
                    self.error(node, 'Signal arguments must be Queues or Signals; capture configuration in the enclosing Module')
                lower = Lower(self, 'signal', name + '_evaluate', {**builder.env, **dict(zip((a.arg for a in fn.args.args), args))}, builder=builder)
                if fn.returns:
                    lower.expected_return = self.annotation(fn.returns)
                lower.statements(fn.body)
                typ = self.annotation(fn.returns) if fn.returns else lower.return_type
                inputs = lower.accesses | set().union(*(v.targets for v in args))
                self.model['resources'].append(dict(name=name, kind='signal', type=typ,
                                                    function=lower.function(), inputs=sorted(inputs)))
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
        self.blocks = [dict(label="entry", params=[], ops=[])]
        self.block = self.blocks[0]
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
        operation = dict(op=op, id=value.id, type=typ, args=[a.id for a in args], loc=self.loc(node), **attrs)
        self.block['ops'].append(operation)
        self.ops.append(operation)
        return value

    def terminated(self):
        return bool(self.block['ops'] and self.block['ops'][-1]['op'] in ('br', 'cond_br', 'return'))

    def new_block(self, values=()):
        block = dict(label='bb' + str(len(self.blocks)), params=[], ops=[])
        self.blocks.append(block)
        for v in values:
            self.serial += 1
            block['params'].append(Value('v' + str(self.serial), v.type, set(v.targets), v.consume))
        return block

    def jump(self, block, values=()):
        self.op('br', 'void', values, dest=block['label'])

    def conditional(self, cond, yes, no):
        self.op('cond_br', 'void', [cond], yes=yes['label'], no=no['label'])

    def function(self, result=None):
        if self.kind != 'construction' and not self.terminated():
            if self.kind in ('helper', 'signal', 'initializer'):
                self.op('unreachable', 'void')
            self.op('return', 'void')
        return dict(name=self.name, kind=self.kind, params=self.params,
                    result=result or self.return_type,
                    blocks=[dict(label=b['label'], params=[vars(p) | {'targets': sorted(p.targets)} for p in b['params']], ops=b['ops']) for b in self.blocks])

    def const(self, value, typ=None, node=None):
        if typ is None:
            typ = 'bool' if isinstance(value, bool) else 'u32'
        return self.op('const', typ, value=value, node=node)

    def param(self, name, typ, targets=(), consume=False):
        value = self.op('param', typ, name=name, targets=targets, consume=consume)
        self.params.append(dict(name=name, type=typ, id=value.id, targets=sorted(targets), consume=consume))
        return value

    def imported(self, val):
        if isinstance(val, list):
            return [self.imported(v) for v in val]
        if not isinstance(val, Value) or self.builder is None:
            return val
        if val.id not in self.imports:
            # Import the pure construction DAG by value; resources preserve identity.
            source = next(x for x in self.builder.ops if x['id'] == val.id)
            inputs = []
            for aid in source['args']:
                a = next(x for x in self.builder.ops if x['id'] == aid)
                inputs.append(self.imported(Value(aid, a['type'], {a['name']} if a['op'] == 'resource' else set())))
            attrs = {k: v for k, v in source.items() if k not in ('op', 'id', 'type', 'args', 'loc')}
            current = self.block
            self.block = self.blocks[0]
            tail = self.block['ops'].pop() if self.terminated() else None
            self.imports[val.id] = self.op(source['op'], val.type, inputs, targets=val.targets, **attrs)
            if tail:
                self.block['ops'].append(tail)
            self.block = current
        return self.imports[val.id]

    def lookup(self, name, node):
        if name in self.env:
            val = self.env[name]
            # Imported construction bindings are replaced once in this environment.
            if self.builder and name not in getattr(self, 'locals', set()):
                val = self.imported(val)
            if self.kind == 'work' and name in self.module.get('_vars', {}):
                self.accesses.update(val.targets)
                return self.op('signal.read', element(val.type), [val], node=node)
            return val
        if name in self.f.constants:
            return self.const(self.f.constants[name], node=node)
        if self.module:
            for _, _, target in self.module.get('_pending_outputs', []):
                if target is not None and any(isinstance(n, ast.Name) and n.id == name for n in ast.walk(target)):
                    self.f.error(node, f'cannot infer output {name} in a connection/type cycle; add an explicit Queue or Rule return type annotation')
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

    def choose(self, condition, yes_fn, no_fn, node):
        yes, no = self.new_block(), self.new_block()
        self.conditional(condition, yes, no)
        self.block = yes
        a = yes_fn()
        yes_end = self.block
        self.block = no
        b = no_fn()
        b = self.cast(b, a.type, node)
        no_end = self.block
        if resource(a.type) and a.consume != b.consume:
            self.f.error(node, 'resource selection must preserve message/observation roles; branch around the reads')
        merged = Value('', a.type, a.targets | b.targets, a.consume)
        join = self.new_block([merged])
        self.block = yes_end
        self.jump(join, [a])
        self.block = no_end
        self.jump(join, [b])
        self.block = join
        return join['params'][0]

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
            out = self.cast(self.expr(node.values[0]), 'bool', node)
            for rhs in node.values[1:]:
                evaluate = lambda: self.cast(self.expr(rhs), 'bool', rhs)
                out = self.choose(out, lambda: self.const(True), evaluate, node) if isinstance(node.op, ast.Or) else self.choose(out, evaluate, lambda: self.const(False), node)
            return out
        if isinstance(node, ast.Compare):
            def compare(a, index):
                op, rhs = node.ops[index], node.comparators[index]
                if isinstance(op, (ast.In, ast.NotIn)):
                    if not isinstance(rhs, (ast.Tuple, ast.List)):
                        self.f.error(rhs, 'membership requires a literal tuple/list')
                    parts = [self.op('binary', 'bool', [a, self.cast(self.expr(x, a.type), a.type, x)], operator='==', node=node) for x in rhs.elts]
                    value = self.const(False)
                    for part in parts:
                        value = self.op('binary', 'bool', [value, part], operator='or', node=node)
                    if isinstance(op, ast.NotIn):
                        value = self.op('unary', 'bool', [value], operator='not', node=node)
                    b = a
                else:
                    b = self.expr(rhs, a.type)
                    typ = self.common(a, b, node)
                    ops = {ast.Eq: '==', ast.NotEq: '!=', ast.Lt: '<', ast.LtE: '<=', ast.Gt: '>', ast.GtE: '>='}
                    if type(op) not in ops:
                        self.f.error(node, 'unsupported comparison')
                    value = self.op('binary', 'bool', [self.cast(a, typ), self.cast(b, typ)], operator=ops[type(op)], node=node)
                if index + 1 < len(node.ops):
                    return self.choose(value, lambda: compare(b, index + 1), lambda: self.const(False), node)
                return value
            return compare(self.expr(node.left), 0)
        if isinstance(node, ast.IfExp):
            c = self.cast(self.expr(node.test), 'bool', node)
            return self.choose(c, lambda: self.expr(node.body, expected), lambda: self.expr(node.orelse, expected), node)
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
            rule = dict(name=fn.name, shape=self.f.output_shape(fn), outputs=self.module.get('_outputs', {}).get(fn.name, []), bindings={}, _lower=None)
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
            # Work locals used by closure become explicit value/resource arguments.
            used = {x.id for x in ast.walk(fn) if isinstance(x, ast.Name) and isinstance(x.ctx, ast.Load)}
            assigned = {x.id for x in ast.walk(fn) if isinstance(x, ast.Name) and isinstance(x.ctx, ast.Store)}
            captures = sorted((used & (getattr(self, 'locals', set()) | self.module.get('_vars', {}).keys())) - {p.arg for p in fn.args.args} - assigned)
            rule['captures'] = captures
            for name in captures:
                v = self.lookup(name, node)
                lower.bind(name, lower.param(name, v.type, v.targets, False))
            lower.statements(fn.body)
            rule['function'] = lower.function()
            rule['_lower'] = lower
        lower = rule['_lower']
        vals = args + [self.lookup(n, node) for n in rule['captures']]
        for p, v in zip(lower.params, vals):
            p['targets'] = sorted(set(p['targets']) | v.targets)
            lower.env[p['name']].targets.update(v.targets)
        vals = [self.cast(v, p['type'], node) for p, v in zip(lower.params, vals)]
        self.op('rule.call', 'void', vals, name=fn.name, node=node)
        outs = [self.imported(Value(r['reference'], wrapped('queue', r['type']), set(r['targets']))) if r and 'reference' in r else self.op('resource', wrapped('queue', r['type']), name=r['name'], targets={r['name']}, node=node) if r else None for r in rule['outputs']]
        return self.f.reshape_outputs(outs, rule['shape'])

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
        if isinstance(target, (ast.Attribute, ast.Subscript)):
            path, indices, base = [], [], target
            while isinstance(base, (ast.Attribute, ast.Subscript)):
                if isinstance(base, ast.Attribute):
                    if base.attr == 'value' and self.target_type(base.value).startswith('queue<'):
                        q = self.expr(base.value)
                        if self.kind != 'rule': self.f.error(node, 'Queue updates are Rule effects')
                        indices = [self.expr(n) for n in indices]
                        self.op('queue.revise', 'void', [q, value, *indices], path=path, node=node)
                        return
                    path.insert(0, base.attr)
                    base = base.value
                else:
                    path.insert(0, None)
                    indices.insert(0, base.slice)
                    base = base.value
            if not isinstance(base, ast.Name): self.f.error(node, 'local updates require a named aggregate')
            original = self.lookup(base.id, node)
            indices = [self.expr(n) for n in indices]
            result = self.op('update', original.type, [original, value, *indices], path=path, node=node)
            self.bind(base.id, result)
            return
        self.f.error(node, 'unsupported assignment target')

    def statements(self, statements):
        for node in statements:
            if self.terminated():
                break
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
                before, locals_before = dict(self.env), set(getattr(self, 'locals', set()))
                yes, no = self.new_block(), self.new_block()
                self.conditional(cond, yes, no)
                ends = []
                for block, body in ((yes, node.body), (no, node.orelse)):
                    self.block, self.env, self.locals = block, dict(before), set(locals_before)
                    self.statements(body)
                    if not self.terminated():
                        ends.append((self.block, dict(self.env), set(self.locals)))
                if not ends:
                    continue
                names = sorted(set.intersection(*(set(e) for _, e, _ in ends)))
                changed = [n for n in names if any(n in ls for _, _, ls in ends) and all(isinstance(e[n], Value) for _, e, _ in ends)]
                values = []
                for n in changed:
                    vs = [e[n] if n in ls else self.imported(e[n]) for _, e, ls in ends]
                    if any(v.type != vs[0].type or v.consume != vs[0].consume for v in vs):
                        self.f.error(node, 'branch values must have the same type and resource role')
                    values.append(Value('', vs[0].type, set().union(*(v.targets for v in vs)), vs[0].consume))
                join = self.new_block(values)
                for end, env, ls in ends:
                    self.block = end
                    self.jump(join, [env[n] if n in ls else self.imported(env[n]) for n in changed])
                self.block, self.env, self.locals = join, dict(before), set(locals_before)
                for n, v in zip(changed, join['params']):
                    self.bind(n, v)
            elif isinstance(node, ast.For):
                if node.orelse or not isinstance(node.target, ast.Name) or not isinstance(node.iter, ast.Call) or spelling(node.iter.func) != 'range':
                    self.f.error(node, 'runtime loops require for name in range(...) without else')
                args = [self.expr(x, 'u32') for x in node.iter.args]
                if len(args) == 1:
                    start, stop, step = self.const(0), args[0], self.const(1)
                elif len(args) == 2:
                    start, stop, step = args[0], args[1], self.const(1)
                else:
                    self.f.error(node, 'runtime range accepts one or two bounds')
                assigned = {n.id for stmt in node.body for n in ast.walk(stmt) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store)}
                # A local aggregate field/index update creates a new SSA value
                # for its root, just like assigning the whole local. Carry that
                # value around the loop; Queue revisions remain resource effects.
                for stmt in node.body:
                    for target in ast.walk(stmt):
                        if not isinstance(target, (ast.Attribute, ast.Subscript)) or not isinstance(target.ctx, ast.Store):
                            continue
                        root = target
                        while isinstance(root, (ast.Attribute, ast.Subscript)):
                            root = root.value
                        if isinstance(root, ast.Name) and isinstance(self.env.get(root.id), Value) and not resource(self.env[root.id].type):
                            assigned.add(root.id)
                names = sorted((assigned & self.env.keys()) - {node.target.id})
                initial = [self.lookup(n, node) for n in names] + [start]
                names.append(node.target.id)
                head = self.new_block(initial)
                self.jump(head, initial)
                self.block = head
                for n, v in zip(names, head['params']):
                    self.bind(n, v)
                header_env, header_locals = dict(self.env), set(self.locals)
                cond = self.op('binary', 'bool', [self.env[node.target.id], stop], operator='<', node=node)
                body, end = self.new_block(), self.new_block()
                self.conditional(cond, body, end)
                self.block = body
                self.statements(node.body)
                if not self.terminated():
                    self.bind(node.target.id, self.op('binary', 'u32', [self.lookup(node.target.id, node), step], operator='+', node=node))
                    self.jump(head, [self.lookup(n, node) for n in names])
                self.block, self.env, self.locals = end, header_env, header_locals
            elif isinstance(node, ast.Return):
                if self.kind == 'work':
                    if node.value is not None:
                        self.f.error(node, 'Work returns no payload; Module return exports connections')
                    self.op('return', 'void', node=node)
                elif self.kind == 'rule':
                    def flatten(n):
                        return [x for child in n.elts for x in flatten(child)] if isinstance(n, (ast.Tuple, ast.List)) else [n]
                    leaves = flatten(node.value) if isinstance(self.rule['shape'], list) else [node.value]
                    def push(expr, index):
                        if expr is None or isinstance(expr, ast.Constant) and expr.value is None: return
                        if isinstance(expr, ast.IfExp):
                            cond = self.cast(self.expr(expr.test), 'bool', expr)
                            yes, no, end = self.new_block(), self.new_block(), self.new_block()
                            self.conditional(cond, yes, no)
                            self.block = yes
                            push(expr.body, index)
                            self.jump(end)
                            self.block = no
                            push(expr.orelse, index)
                            self.jump(end)
                            self.block = end
                            return
                        expected = self.rule['outputs'][index]['type'] if index < len(self.rule['outputs']) and self.rule['outputs'][index] else None
                        v = self.expr(expr, expected)
                        while len(self.rule['outputs']) <= index: self.rule['outputs'].append(None)
                        out = self.rule['outputs'][index]
                        if out is None:
                            name = self.name + f'_out{index}'
                            out = dict(name=name, kind='queue', type=v.type, capacity=self.builder.const(1).id, initial=[], sequence=False)
                            self.f.model['resources'].append(out)
                            self.rule['outputs'][index] = out
                        q = self.imported(Value(out['reference'], wrapped('queue', out['type']), set(out['targets']))) if 'reference' in out else self.op('resource', wrapped('queue', v.type), name=out['name'], targets={out['name']}, node=node)
                        self.op('queue.push', 'void', [q, self.cast(v, out['type'])], node=node)
                    for index, expr in enumerate(leaves): push(expr, index)
                    self.op('return', 'void', node=node)
                else:
                    val = self.expr(node.value)
                    if val is None or isinstance(val, list): self.f.error(node, 'helpers and Signals return one typed value')
                    if getattr(self, 'expected_return', None): val = self.cast(val, self.expected_return, node)
                    self.return_type = val.type
                    self.op('return', 'void', [val], node=node)
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

"""AST-only static elaboration and typed MLIR control-flow construction.

Construction is symbolic: host configuration values are constructor arguments,
never evaluated by Python. Only literal construction loops are expanded here.
"""
import ast
import hashlib
from pathlib import Path
from .ir import CompileError, SCALARS, VERSION, Value, Instance, RuleRef, element, resource, wrapped


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
        self.globals = {}
        self.global_values = {}
        self.constructing = set()
        self.call_stack = []
        self.instances = []
        self.instance_stack = []
        self.file_names = {}
        self.entry_names = {}
        self.closures = {}
        self.body_values = {}

    def error(self, node, message):
        raise CompileError(f'{getattr(node, "source", "<source>")}:{getattr(node, "lineno", 0)}: {message}')

    def capture_names(self, fn, definitions, instance, seen=None):
        seen = set(seen or ())
        if id(fn) in seen: return set()
        seen.add(id(fn))
        names = {x.id for stmt in fn.body for x in ast.walk(stmt) if isinstance(x, ast.Name) and isinstance(x.ctx, ast.Load)}
        assigned = {x.id for stmt in fn.body for x in ast.walk(stmt) if isinstance(x, ast.Name) and isinstance(x.ctx, ast.Store)}
        scope = self.closures.get((instance, id(fn)), ({},))[0]
        signals = {n for n in assigned if isinstance(scope.get(n), Value) and scope[n].type.startswith('signal<')}
        names -= {p.arg for p in fn.args.args} | (assigned - signals)
        names |= signals
        dependencies = set(names)
        for dec in fn.decorator_list:
            if not isinstance(dec, ast.Call): continue
            for kw in dec.keywords:
                if kw.arg == 'guard':
                    guard = spelling(kw.value)
                    if guard in definitions or guard in self.defs: dependencies.add(guard)
                    else: names |= {n.id for n in ast.walk(kw.value) if isinstance(n, ast.Name)}
        for name in dependencies:
            helper = definitions.get(name, self.defs.get(name))
            if helper and decorator(helper) in ('helper', 'rule'):
                names |= self.capture_names(helper, definitions, instance, seen)
        return names

    def read(self, path):
        path = Path(path).resolve()
        if path in self.files:
            return
        self.files.add(path)
        tree = ast.parse(path.read_text(), filename=str(path))
        for node in ast.walk(tree):
            node.source = str(path)
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign) and len(node.targets) != 1:
                self.error(node, 'chained assignment is unsupported; use separate assignments')
            if isinstance(node, ast.Call):
                names = [k.arg for k in node.keywords]
                if None in names or len(set(names)) != len(names):
                    self.error(node, 'unknown or duplicate keyword arguments')
            if isinstance(node, ast.FunctionDef):
                for stmt in node.body:
                    if any(isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)) for n in ast.walk(stmt)):
                        self.error(stmt, 'nested declarations are unsupported')
        # Resolve file-local symbols before lowering. No imported module is executed.
        namespace = 'src_' + path.stem + '_' + hashlib.sha256(str(path).encode()).hexdigest()[:8]
        self.file_names[path] = namespace
        bindings = {}
        imports = []
        for node in tree.body:
            if isinstance(node, (ast.FunctionDef, ast.ClassDef)):
                bindings[node.name] = namespace + '.' + node.name
            elif isinstance(node, (ast.Assign, ast.AnnAssign)):
                for target in node.targets if isinstance(node, ast.Assign) else [node.target]:
                    if not isinstance(target, ast.Name): self.error(node, 'top-level bindings require names')
                    bindings[target.id] = namespace + '.' + target.id
            elif isinstance(node, (ast.Import, ast.ImportFrom)):
                imports.append(node)
        self.entry_names[path] = bindings
        for node in imports:
            for alias in node.names:
                if alias.name == '*': self.error(node, 'wildcard imports are unsupported')
                imported = alias.name if isinstance(node, ast.Import) else node.module or ''
                if imported in ('pycircuit', 'pycircuit.ac', '__future__'):
                    if imported != '__future__': bindings[alias.asname or alias.name] = 'ac'
                    continue
                base = path.parent
                for _ in range(max(0, getattr(node, 'level', 0) - 1)): base = base.parent
                target = base.joinpath(*imported.split('.')).with_suffix('.py')
                if not target.is_file(): self.error(node, f'local dependency not found: {target}')
                self.read(target)
                imported_namespace = self.file_names[target.resolve()]
                if isinstance(node, ast.Import):
                    bindings[alias.asname or alias.name] = imported_namespace
                else:
                    bindings[alias.asname or alias.name] = self.entry_names[target.resolve()].get(alias.name, imported_namespace + '.' + alias.name)
        self.entry_names[path] = bindings
        class Qualify(ast.NodeTransformer):
            def __init__(self): self.locals = []
            def visit_FunctionDef(self, fn):
                for d in fn.decorator_list: self.visit(d)
                if fn.returns: fn.returns = self.visit(fn.returns)
                for arg in fn.args.args:
                    if arg.annotation: arg.annotation = self.visit(arg.annotation)
                fn.args.defaults = [self.visit(d) for d in fn.args.defaults]
                if not self.locals: fn.name = bindings[fn.name]
                bound = {a.arg for a in fn.args.args}
                for stmt in fn.body:
                    for n in ast.walk(stmt):
                        if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store): bound.add(n.id)
                    if isinstance(stmt, ast.FunctionDef): bound.add(stmt.name)
                self.locals.append(bound)
                fn.body = [self.visit(n) for n in fn.body]
                self.locals.pop()
                return fn
            def visit_ClassDef(self, cls):
                for d in cls.decorator_list: self.visit(d)
                if decorator(cls) == 'module':
                    members = {f.name for f in cls.body if isinstance(f, ast.FunctionDef)}
                    for f in cls.body:
                        if isinstance(f, ast.FunctionDef) and f.name == '__init__':
                            members |= {n.id for stmt in f.body for n in ast.walk(stmt) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store)}
                    self.locals.append(members)
                    cls.body = [self.visit(f) for f in cls.body]
                    self.locals.pop()
                else: cls.body = [self.visit(f) for f in cls.body]
                return cls
            def visit_Name(self, n):
                if not any(n.id in scope for scope in self.locals): n.id = bindings.get(n.id, n.id)
                return n
        tree = Qualify().visit(tree)
        for node in tree.body:
            if isinstance(node, (ast.Import, ast.ImportFrom)): continue
            if isinstance(node, ast.FunctionDef):
                self.validate_function(node)
                if decorator(node) == 'module': self.error(node, 'Module requires a class with __init__')
                self.defs[node.name] = node
            elif isinstance(node, ast.ClassDef):
                original = node.name
                node.name = bindings[original]
                if decorator(node) == 'module':
                    if node.bases or node.keywords: self.error(node, 'Module inheritance is unsupported')
                    if len(node.decorator_list)!=1 or isinstance(node.decorator_list[0],ast.Call) and (node.decorator_list[0].args or node.decorator_list[0].keywords): self.error(node,'@ac.module accepts no arguments')
                    self.validate_function(node)
                    self.defs[node.name] = node
                    continue
                if node.bases or node.keywords or node.decorator_list: self.error(node, 'struct inheritance and decorators are unsupported')
                fields = []
                for f in node.body:
                    if not isinstance(f, ast.AnnAssign) or not isinstance(f.target, ast.Name): self.error(f, 'structs contain annotated fields only')
                    fields.append(dict(name=f.target.id, type=self.annotation(f.annotation), default=self.literal(f.value) if f.value else None))
                self.types[node.name] = fields
            elif isinstance(node, (ast.Assign, ast.AnnAssign)):
                for target in node.targets if isinstance(node, ast.Assign) else [node.target]:
                    if isinstance(node.value, (ast.Call, ast.Attribute)) or isinstance(node.value, ast.Name) and node.value.id not in self.constants: self.globals[target.id] = node.value
                    else: self.constants[target.id] = self.literal(node.value)
            elif isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and isinstance(node.value.value, str): pass
            else: self.error(node, 'unsupported top-level syntax')

    def global_value(self, name, builder, node):
        if name not in self.global_values:
            if name in self.constructing: self.error(node, 'recursive static instance construction')
            self.constructing.add(name)
            previous_stack = self.instance_stack
            self.instance_stack = []
            self.global_values[name] = builder.expr(self.globals[name])
            self.instance_stack = previous_stack
            self.constructing.remove(name)
        return self.global_values[name]

    def validate_function(self, node):
        for fn in (n for n in ast.walk(node) if isinstance(n, ast.FunctionDef)):
            if fn.args.vararg or fn.args.kwarg or fn.args.kwonlyargs or fn.args.posonlyargs:
                self.error(fn, 'unsupported function signature')
            if fn.returns is None or any(p.annotation is None for p in fn.args.args):
                self.error(fn, 'all parameters and return types require explicit annotations')
            if any(p.arg == 'self' for p in fn.args.args): self.error(fn, 'Module uses implicit instance scope; omit self')
            for dec in fn.decorator_list:
                kind = spelling(dec.func if isinstance(dec, ast.Call) else dec)
                if kind != 'ac.rule': self.error(dec, 'only @ac.rule is supported on functions')
                allowed = {'guard', 'public', 'SB', 'SBR', 'C', 'CF'}
                if isinstance(dec, ast.Call) and (dec.args or any(k.arg not in allowed for k in dec.keywords) or len({k.arg for k in dec.keywords})!=len(dec.keywords)):
                    self.error(dec, 'Rule accepts guard, public, SB, SBR, C and CF')

    def literal(self, node):
        if spelling(node) in self.constants:
            return self.constants[spelling(node)]
        try:
            return ast.literal_eval(node)
        except (ValueError, TypeError):
            self.error(node, 'expected a literal construction constant')

    def annotation(self, node):
        if node is None:
            raise CompileError('a value type annotation is required here')
        name = spelling(node).removeprefix('ac.')
        if name in SCALARS or name in self.types or name in self.defs and isinstance(self.defs[name], ast.ClassDef):
            return name
        if name == 'None' or isinstance(node, ast.Constant) and node.value is None:
            return 'void'
        if isinstance(node, ast.Subscript):
            kind = spelling(node.value).removeprefix('ac.')
            if kind == 'tuple':
                return [self.annotation(x) for x in node.slice.elts]
            if kind in ('queue', 'cell', 'ehr', 'signal'):
                return wrapped(kind, self.annotation(node.slice))
            if kind == 'array' and isinstance(node.slice, ast.Tuple):
                return f'array<{self.annotation(node.slice.elts[0])},{self.literal(node.slice.elts[1])}>'
        self.error(node, f'unsupported type: {ast.unparse(node)}')

    def compile(self, path, top):
        self.read(path)
        key = self.entry_names[Path(path).resolve()].get(top, top)
        cls = self.defs.get(key)
        if not isinstance(cls, ast.ClassDef) or decorator(cls) != 'module': raise CompileError('top Module not found: ' + top)
        self.model['top'] = top
        self.model['constants'] = self.constants
        builder = Lower(self, 'construction', 'construct')
        init = next((f for f in cls.body if isinstance(f, ast.FunctionDef) and f.name == '__init__'), None)
        args = []
        defaults=dict(zip((p.arg for p in init.args.args[-len(init.args.defaults):]),init.args.defaults)) if init and init.args.defaults else {}
        for arg in init.args.args if init else []:
            typ = self.annotation(arg.annotation)
            if typ not in SCALARS: self.error(arg, 'top configuration requires a scalar type')
            entry=dict(name=arg.arg,type=typ)
            if arg.arg in defaults: entry['default']=self.literal(defaults[arg.arg])
            self.model['config'].append(entry)
            args.append(builder.op('config', typ, name=arg.arg, node=arg))
        inst = self.instantiate(cls, args, 'top', builder)
        if '__call__' in inst.definitions:
            self.connect(inst, [], {}, builder, cls)
        self.model['construction'] = builder.ops
        self.model['instances'] = [dict(name=i.name, parent=i.parent, definition=i.source_name, members={}, inputs={}, rules=sorted(i.rules)) for i in self.instances]
        self.model['contracts'] = []
        for i in self.instances:
            relations = []
            for name, fn in i.rules.items():
                for dec in fn.decorator_list:
                    if not isinstance(dec, ast.Call): continue
                    for kw in dec.keywords:
                        if kw.arg not in ('SB', 'SBR', 'C', 'CF'): continue
                        targets = self.literal(kw.value)
                        if isinstance(targets, str): targets = [targets]
                        if not isinstance(targets, (tuple, list)) or any(t not in i.rules for t in targets): self.error(kw.value, 'relation must reference a Rule of the same Module')
                        for target in targets:
                            if target == name: self.error(kw.value, 'self relation is invalid')
                            relations.append(dict(before=name, after=target, relation=kw.arg, source='user', loc=builder.loc(kw.value)))
            self.model['contracts'].append(dict(id='Module:' + i.name, rules=sorted(i.rules), relations=relations))
        return self.model

    def instantiate(self, cls, args, prefix, builder):
        if cls.name in self.constructing: self.error(cls, 'recursive Module construction')
        self.constructing.add(cls.name)
        init = next((f for f in cls.body if isinstance(f, ast.FunctionDef) and f.name == '__init__'), None)
        if init is None: self.error(cls, 'Module requires __init__')
        if self.annotation(init.returns) != 'void': self.error(init, '__init__ must return None')
        definitions = {f.name:f for f in cls.body if isinstance(f, ast.FunctionDef)}
        if any(resource(self.annotation(p.annotation)) for p in init.args.args): self.error(init,'component interfaces bind in __call__, not __init__')
        env = dict(zip((p.arg for p in init.args.args), args))
        instance = Instance(prefix, env, {n:f for n,f in definitions.items() if decorator(f) == 'rule'}, definitions=definitions, parent=self.instance_stack[-1] if self.instance_stack else None, source_name=cls.name)
        self.instances.append(instance)
        self.instance_stack.append(prefix)
        statements = self.expand_loops(init.body)
        saved = builder.env
        builder.env = env
        for stmt in statements:
            if isinstance(stmt, ast.Pass) or isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Constant): continue
            if not isinstance(stmt, (ast.Assign, ast.AnnAssign)): self.error(stmt, '__init__ accepts static bindings only')
            target = stmt.targets[0] if isinstance(stmt, ast.Assign) else stmt.target
            if not isinstance(target, ast.Name): self.error(target, 'construction requires named members')
            start=len(builder.ops)
            value = self.construct_value(stmt.value, prefix + '.' + target.id, builder, definitions)
            if any(o['op'].startswith(('queue.', 'cell.', 'ehr.', 'signal.')) for o in builder.ops[start:]): self.error(stmt,'__init__ cannot read runtime state')
            if isinstance(value, Value) and any(o['op'].endswith(('read', 'write')) or o['op'] == 'call' for o in builder.ops if o['id'] == value.id): self.error(stmt, 'runtime computation is not allowed in __init__')
            env[target.id] = value
        builder.env = saved
        self.instance_stack.pop()
        self.constructing.remove(cls.name)
        for fn in instance.definitions.values(): self.closures[instance.name, id(fn)] = (env, definitions, instance.name)
        return instance

    def connect(self, instance, args, keywords, builder, node, outputs=None, caller=None):
        fn = instance.definitions.get('__call__')
        if fn is None: self.error(node, 'Module has no __call__; invoke its public Rules directly')
        if getattr(instance, 'connected', False): self.error(node, 'Module __call__ binds fixed connections once')
        instance.connected = True
        output_type = self.annotation(fn.returns)
        def validate_outputs(value, typ):
            if isinstance(typ, list):
                if not isinstance(value, list) or len(value) != len(typ): self.error(node, 'Module output binding tuple arity mismatch')
                for v, t in zip(value, typ): validate_outputs(v, t)
            elif typ == 'void':
                if value is not None: self.error(node, 'Module returning None has no output binding')
            elif not resource(typ): self.error(fn, 'Module returns must declare component output types')
            elif not isinstance(value, Value) or value.type != typ: self.error(node, 'Module result must bind to a previously declared component of type ' + typ)
        validate_outputs(outputs, output_type)
        supplied = self.bind_args(fn, args, keywords)
        supplied = [builder.expr(v,reference=True) if isinstance(v,ast.AST) else v for v in supplied]
        env = dict(instance.env)
        for p,v in zip(fn.args.args, supplied):
            typ = self.annotation(p.annotation)
            if isinstance(v, Value) and v.type != typ: self.error(node, 'connection type mismatch: ' + p.arg)
            if isinstance(v,Instance) and v.source_name != typ: self.error(node,'Module connection type mismatch: '+p.arg)
            env[p.arg] = v
        instance.inputs = dict(zip((p.arg for p in fn.args.args), supplied))
        instance.env = env
        mod = dict(name=instance.name, rules=[], resources=[], _resources=set())
        self.model['modules'].append(mod)
        lower = Lower(self, 'body', instance.name + '_body', env, builder=builder, module=mod, defs=instance.definitions)
        lower.lexical_instance = instance.name
        lower.output_targets = outputs
        for rule in instance.definitions.values(): self.closures[instance.name, id(rule)] = (env, instance.definitions, instance.name)
        lower.statements(fn.body)
        mod['body'] = lower.function()
        mod['resources'] = sorted(mod.pop('_resources'))
        if caller is not None: caller.op('module.call', 'void', target=instance.name, node=node)

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
        if set(names[:len(values)]) & keywords.keys(): self.error(node, 'argument supplied more than once')
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
            self.model['resources'].append(dict(name=name, owner=self.instance_stack[-1] if self.instance_stack else 'top', kind='qarray', type=typ, iterable=iterable.id,
                                               capacity=cap.id, initializer=init.function(), empty_initial=initial is None))
            return builder.op('resource', wrapped('qarray', typ), name=name, targets={name}, node=node)
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Subscript) and spelling(node.func.value) in ('ac.queue', 'ac.cell', 'ac.ehr', 'ac.signal'):
            declared = spelling(node.func.value)[3:]
            kind = declared
            typ = self.annotation(node.func.slice)
            kw = {x.arg: x.value for x in node.keywords}
            if node.args or set(kw) - {'capacity', 'initial'}:
                self.error(node, 'Queue accepts capacity= and initial= only')
            if kind in ('cell', 'ehr') and ('initial' not in kw or 'capacity' in kw): self.error(node, kind.title() + ' requires initial= and has no capacity')
            capacity = (builder.expr(kw['capacity']) if 'capacity' in kw else builder.const(1)) if kind == 'queue' else None
            initial = builder.expr(kw['initial'], typ) if 'initial' in kw else None
            if kind == 'signal' and kw: self.error(node, 'Signal has no initial state or capacity')
            if kind in ('cell', 'ehr'):
                if initial is None: self.error(node, kind.title() + ' initial must be a payload value')
                initial = builder.cast(initial, typ, node)
            # scalar initial is one element; a list is a FIFO initialization sequence.
            values = [] if initial is None else [initial.id]
            sequence = initial is not None and initial.type != typ and initial.type.startswith(('vector<', 'array<'))
            self.model['resources'].append(dict(name=name, owner=self.instance_stack[-1] if self.instance_stack else 'top', kind=kind, type=typ, loc=builder.loc(node), **({'capacity': capacity.id} if capacity else {}),
                                                initial=values, sequence=sequence, visibility='ehr' if declared == 'ehr' else 'current' if kind == 'signal' else 'old'))
            return builder.op('resource', wrapped(kind, typ), name=name, targets={name}, node=node)
        if isinstance(node, ast.List) and any(isinstance(x, ast.Call) and isinstance(x.func, ast.Subscript) and spelling(x.func.value) == 'ac.queue' for x in node.elts):
            vals = [self.construct_value(x, name + '.' + str(i), builder, defs) for i, x in enumerate(node.elts)]
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
            self.model['resources'].append(dict(name=name, owner=self.instance_stack[-1] if self.instance_stack else 'top', kind='qarray', type=typ, iterable=iterable.id,
                                                capacity=cap.id, initializer=init.function()))
            return builder.op('resource', wrapped('qarray', typ), name=name, targets={name}, node=node)
        if isinstance(node, ast.Call):
            fn = defs.get(spelling(node.func), self.defs.get(spelling(node.func)))
            if fn and decorator(fn) == 'module':
                init = next((f for f in fn.body if isinstance(f, ast.FunctionDef) and f.name == '__init__'), None)
                if init is None: self.error(fn, 'Module requires __init__')
                supplied = self.bind_args(init, node.args, {k.arg:k.value for k in node.keywords})
                args = [builder.expr(x, reference=True) for x in supplied]
                if decorator(fn) == 'module':
                    result = self.instantiate(fn, args, name, builder)
                    return result
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
        self.untyped_literals = set()
        self.effects = []
        self.rule = None
        self.guard_context = kind == 'guard'
        self.consume_context = kind == 'rule'
        self.lexical_instance = module['name'] if module else None

    def loc(self, node):
        return dict(file=getattr(node, 'source', ''), line=getattr(node, 'lineno', 0), column=getattr(node, 'col_offset', 0))

    def op(self, op, typ, args=(), node=None, targets=(), consume=False, **attrs):
        if op in ('cell.read', 'cell.write') and args and args[0].type.startswith('ehr<'):
            op = op.replace('cell.', 'ehr.')
        self.serial += 1
        value = Value(f'v{self.serial}', typ, set(targets), consume)
        if self.kind == 'helper' and (op.startswith(('cell.', 'queue.', 'ehr.', 'signal.')) or op == 'resource'):
            self.f.error(node, 'payload helper cannot access runtime state')
        if self.guard_context and op in ('queue.pop', 'queue.push', 'cell.write', 'ehr.write', 'signal.write', 'rule.call'): self.f.error(node, 'guard must only observe state')
        operation = dict(op=op, id=value.id, type=typ, args=[a.id for a in args], loc=self.loc(node), **attrs)
        self.block['ops'].append(operation)
        self.ops.append(operation)
        if self.kind == 'body' and typ != 'void' and not resource(typ): self.f.body_values[id(value)] = (value,self)
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
            if self.kind in ('helper', 'guard', 'initializer') and (getattr(self, 'expected_return', None) not in (None, 'void') or self.return_type != 'void'):
                self.op('unreachable', 'void')
            self.op('return', 'void')
        return dict(name=self.name, kind=self.kind, params=self.params, guard=getattr(self, 'guard_blocks', None), guard_info=getattr(self, 'guard_info', None),
                    result=result or self.return_type,
                    blocks=[dict(label=b['label'], params=[vars(p) | {'targets': sorted(p.targets)} for p in b['params']], ops=b['ops']) for b in self.blocks])

    def const(self, value, typ=None, node=None):
        untyped = typ is None and not isinstance(value, bool)
        if typ is None:
            typ = 'bool' if isinstance(value, bool) else 'u32'
        if typ in SCALARS:
            if (typ == 'bool') != isinstance(value, bool):
                self.f.error(node, 'bool and integer literals are distinct')
            if typ != 'bool':
                width = int(typ[1:]); signed = typ.startswith('i')
                if not -(1 << (width - 1)) * signed <= value < (1 << (width - signed)):
                    self.f.error(node, 'integer literal out of range for ' + typ)
        result = self.op('const', typ, value=value, node=node)
        if untyped: self.untyped_literals.add(result.id)
        return result

    def param(self, name, typ, targets=(), consume=False):
        value = self.op('param', typ, name=name, targets=targets, consume=consume)
        self.params.append(dict(name=name, type=typ, id=value.id, targets=sorted(targets), consume=consume))
        return value

    def imported(self, val):
        if isinstance(val, list):
            return [self.imported(v) for v in val]
        if not isinstance(val, Value) or self.builder is None:
            return val
        entry = self.f.body_values.get(id(val))
        origin = entry[1] if entry else None
        if origin and origin is not self:
            exported = origin.export_value(val)
            return self.op('external', val.type, source=origin.name, identity=exported.id)
        if val.id.startswith('external:'):
            _, source, identity = val.id.split(':')
            return self.op('external', val.type, source=source, identity=identity)
        if resource(val.type) and len(val.targets) == 1:
            return self.op('resource', val.type, name=next(iter(val.targets)), targets=val.targets)
        if val.id not in self.imports:
            # Import the pure construction DAG by value; resources preserve identity.
            source = next((x for x in self.builder.ops if x['id'] == val.id), None)
            if source is None: self.f.error(None, 'SSA value must have a dominating definition before use')
            inputs = []
            for aid in source['args']:
                a = next(x for x in self.builder.ops if x['id'] == aid)
                inputs.append(self.imported(Value(aid, a['type'], {a['name']} if a['op'] == 'resource' else set())))
            attrs = {k: v for k, v in source.items() if k not in ('op', 'id', 'type', 'args', 'loc')}
            current = self.block
            self.block = self.blocks[0]
            tail = self.block['ops'].pop() if self.terminated() else None
            self.imports[val.id] = self.op(source['op'], val.type, inputs, targets=val.targets, **attrs)
            self.ops[-1]['loc'] = source.get('loc', {})
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
            return val
        if name in self.f.globals:
            return self.f.global_value(name, self.builder or self, node)
        if name in self.f.constants:
            return self.const(self.f.constants[name], node=node)
        self.f.error(node, f'unknown name: {name}')

    def bind(self, name, val):
        if isinstance(val, Value): self.untyped_literals.discard(val.id)
        if not hasattr(self, 'locals'):
            self.locals = set()
        self.locals.add(name)
        self.env[name] = val
        if self.kind == 'body' and isinstance(val, Value): self.f.body_values[id(val)] = (val, self)

    def export_value(self, val):
        if not hasattr(self, 'exports'): self.exports = {}
        if val.id not in self.exports:
            current = self.block
            block = next(b for b in self.blocks if any(o['id'] == val.id for o in b['ops']) or any(p.id == val.id for p in b['params']))
            position = next((i + 1 for i, o in enumerate(block['ops']) if o['id'] == val.id), 0)
            self.block = block
            exported = self.op('export', val.type, [val])
            block['ops'].insert(position, block['ops'].pop())
            self.block = current
            self.exports[val.id] = exported
            if self.module and 'body' in self.module: self.module['body'] = self.function()
        return self.exports[val.id]

    def cast(self, value, typ, node=None):
        if not isinstance(value, Value): self.f.error(node, 'expected a typed value, not a Module instance')
        if value.type == typ:
            return value
        if value.type in SCALARS and typ in SCALARS and 'bool' not in (value.type, typ):
            literal = next((o for o in self.ops if o['id'] == value.id and o['op'] == 'const'), None)
            if literal is not None and value.id in self.untyped_literals:
                return self.const(literal['value'], typ, node)
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

    def signal_value(self, value, node=None):
        if not isinstance(value, Value) or not resource(value.type) or value.type.startswith('qarray<'): return value
        self.accesses.update(value.targets)
        operation = 'queue.read' if value.type.startswith('queue<') else 'signal.read' if value.type.startswith('signal<') else 'cell.read'
        return self.op(operation, element(value.type), [value], node=node, role='consume' if operation == 'queue.read' and self.consume_context and not self.guard_context else 'observe')

    def expr(self, node, expected=None, reference=False):
        value = self._expr(node, expected, reference)
        return value if reference or expected and resource(expected) else self.signal_value(value, node)

    def _expr(self, node, expected=None, reference=False):
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
            return [self.expr(x, reference=reference) for x in node.elts]
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
            qualified = spelling(node)
            if qualified in self.f.globals: return self.lookup(qualified, node)
            if qualified in self.f.constants: return self.const(self.f.constants[qualified], node=node)
            val = self.expr(node.value, reference=True)
            if isinstance(val, Instance):
                if node.attr == 'outputs': self.f.error(node, 'Module .outputs is removed; bind results to explicitly declared components')
                if node.attr in val.rules: return RuleRef(val, val.rules[node.attr])
                self.f.error(node, 'resource is private to its Module; use a public Rule or an explicit connection')
            val = self.signal_value(val, node)
            fields = self.f.types.get(val.type, [])
            field = next((x for x in fields if x['name'] == node.attr), None)
            if not field:
                self.f.error(node, f'no field {node.attr} on {val.type}')
            return self.op('field', field['type'], [val], field=node.attr, node=node)
        if isinstance(node, ast.Subscript):
            val = self.expr(node.value, reference=True)
            if isinstance(val, list):
                index = self.f.literal(node.slice)
                if not isinstance(index, int) or not 0 <= index < len(val): self.f.error(node, 'fixed output tuple index out of range')
                return self.imported(val[index])
            idx = self.expr(node.slice)
            if resource(val.type) and not val.type.startswith('qarray<'): val=self.signal_value(val,node)
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
            if isinstance(node.op, ast.Not): a = self.cast(a, 'bool', node)
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
                if val.id in self.untyped_literals:
                    return other.type
        self.f.error(node, f'mixed types need an explicit cast: {a.type}, {b.type}')

    def call(self, node, expected, outputs=None):
        name = spelling(node.func)
        short = name.removeprefix('ac.')
        if short in SCALARS:
            self.f.error(node, 'scalar conversion calls are unsupported; use typed values')
        if name == 'len':
            if len(node.args) != 1 or node.keywords: self.f.error(node, 'len requires exactly one positional argument')
            return self.op('length', 'u32', [self.expr(node.args[0])], node=node)
        if name == 'range' and self.kind == 'construction':
            vals = [self.expr(x) for x in node.args]
            if len(vals) != 1:
                self.f.error(node, 'construction range currently takes one bound')
            return self.op('range', 'vector<u32>', vals, node=node)
        if isinstance(node.func, ast.Attribute) and node.func.attr in ('empty', 'full', 'size', 'peek') and isinstance(self.expr(node.func.value, reference=True), Value):
            if node.args or node.keywords: self.f.error(node, node.func.attr + ' accepts no arguments')
            q = self.expr(node.func.value, reference=True)
            if not q.type.startswith('queue<'): self.f.error(node, 'Queue operation requires a Queue')
            self.accesses.update(q.targets)
            return self.op('queue.read' if node.func.attr == 'peek' else 'queue.' + node.func.attr, element(q.type) if node.func.attr == 'peek' else 'u32' if node.func.attr == 'size' else 'bool', [q], role='observe', node=node)
        if name in self.f.types:
            fields = self.f.types[name]
            if len(node.args) > len(fields): self.f.error(node, 'too many struct arguments')
            if {f['name'] for f in fields[:len(node.args)]} & {k.arg for k in node.keywords}: self.f.error(node, 'struct field supplied more than once')
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
        if isinstance(node.func, ast.Name) and isinstance(self.env.get(node.func.id), Instance):
            if self.kind != 'body': self.f.error(node, 'Module __call__ connections belong in Module bodies')
            instance = self.env[node.func.id]
            args = [self.expr(x, reference=True) for x in node.args]
            keywords = {k.arg:self.expr(k.value, reference=True) for k in node.keywords}
            return self.f.connect(instance, args, keywords, self.builder, node, outputs=outputs, caller=self)
        bound = None
        fn = self.defs.get(name, self.f.defs.get(name))
        if not fn and isinstance(node.func, ast.Attribute):
            bound = self.expr(node.func)
            if isinstance(bound, RuleRef): fn = bound.definition
        if not fn:
            self.f.error(node, f'unsupported call: {name}')
        supplied = self.f.bind_args(fn, node.args, {k.arg: k.value for k in node.keywords})
        args = [self.expr(x, self.f.annotation(p.annotation) if p.annotation else None) for p, x in zip(fn.args.args, supplied)]
        if decorator(fn) not in ('helper', 'rule'): self.f.error(node, 'Module construction belongs to __init__')
        captured, definitions, instance = self.f.closures.get((self.lexical_instance, id(fn)), ({}, self.f.defs, None))
        if isinstance(bound, RuleRef):
            captured, definitions, instance = self.f.closures[bound.instance.name, id(fn)]
            private = any(isinstance(d, ast.Call) and any(k.arg == 'public' and self.f.literal(k.value) is False for k in d.keywords) for d in fn.decorator_list)
            if private and instance != self.lexical_instance: self.f.error(node, 'private Rule cannot be called from another Module')
        return self.behavior_call(fn, args, node, captured, 'rule' if decorator(fn) == 'rule' else 'helper', definitions, instance)

    def apply_guard(self, fn, args, node):
        if self.kind not in ('rule'): return self._apply_guard(fn, args, node)
        saved = self.blocks, self.block, self.ops, self.guard_context, self.imports.copy()
        self.blocks = [dict(label='entry', params=[], ops=[])]
        self.block, self.ops, self.guard_context = self.blocks[0], [], True
        self._apply_guard(fn, args, node)
        checks = [o for o in self.ops if o['op'] == 'guard']
        if checks:
            last = checks[-1]; self.block['ops'].remove(last); self.ops.remove(last)
            value = Value(last['args'][0], 'bool')
        else: value = self.const(True)
        yield_op = self.op('return', 'void', [value])
        if checks:
            self.block['ops'][-1]['loc'] = last['loc']
            self.guard_info = {k:last[k] for k in ('name', 'source') if k in last}
        self.guard_blocks = self.blocks
        self.blocks, self.block, self.ops, self.guard_context, self.imports = saved

    def _apply_guard(self, fn, args, node):
        for dec in fn.decorator_list:
            if isinstance(dec, ast.Call):
                guard = next((k.value for k in dec.keywords if k.arg == 'guard'), None)
                if guard is None: continue
                guard_fn = self.defs.get(spelling(guard), self.f.defs.get(spelling(guard)))
                if guard_fn is None:
                    value = self.expr(guard)
                else:
                    scope, definitions, instance = self.f.closures.get((self.lexical_instance, id(guard_fn)), ({}, self.f.defs, None))
                    value = self.behavior_call(guard_fn, args, node, scope, 'rule' if decorator(guard_fn) == 'rule' else 'helper', definitions, instance)
                if value.type != 'bool': self.f.error(guard, 'guard must return bool')
                self.op('guard', 'void', [value], node=guard, name=guard_fn.name if guard_fn else spelling(guard), source='function' if guard_fn else 'variable')

    def behavior_call(self, fn, args, node, captured, kind, definitions=None, instance=None):
        if self.kind == 'helper' and decorator(fn) == 'rule': self.f.error(node, 'payload helper cannot call a Rule')
        if kind == 'helper' and any(not isinstance(v, Value) or resource(v.type) for v in args): self.f.error(node, 'helper parameters must be payload')
        if any(isinstance(v, Instance) for v in args): self.f.error(node, 'Rule/helper cannot accept Module instances; bind the dependency statically')
        if id(fn) in self.f.call_stack: self.f.error(node, 'recursive helper/Rule/guard call is unsupported')
        if len(args) != len(fn.args.args): self.f.error(node, 'guard/Rule argument arity mismatch')
        self.f.call_stack.append(id(fn))
        name = self.name + '.0.call.' + str(len(self.f.model['functions'])) + '.' + str(self.serial)
        # Every dynamic closure value is passed as SSA, preserving shared reads.
        scope = dict(captured)
        lower = Lower(self.f, kind, name, scope, builder=self.builder, module=self.module, defs=definitions or self.defs)
        lower.lexical_instance = instance
        lower.guard_context = self.guard_context or kind == 'guard'
        lower.consume_context = self.consume_context or kind == 'rule'
        vals = []
        for parameter, value in zip(fn.args.args, args):
            typ = self.f.annotation(parameter.annotation) if parameter.annotation else value.type
            value = self.cast(value, typ, node)
            role = value.consume and not lower.guard_context
            lower.bind(parameter.arg, lower.param(parameter.arg, typ, value.targets, role))
            vals.append(value)
        used = self.f.capture_names(fn, definitions or self.defs, instance)
        for key, value in scope.items():
            if key in {p.arg for p in fn.args.args} or not isinstance(value, Value): continue
            # Construction captures are imported once as resource/config identities.
            if key in used and value is self.env.get(key) and key in getattr(self, 'locals', set()):
                value = self.lookup(key, node)
                lower.bind(key, lower.param(key, value.type, value.targets, value.consume and not lower.guard_context))
                vals.append(value)
        result = self.f.annotation(fn.returns) if fn.returns else None
        declared_result = result
        if isinstance(result, list): result = 'tuple<' + ','.join(element(t) if resource(t) else t for t in result) + ',' + ','.join('bool' for t in result) + '>'
        if result and resource(result): result = element(result)
        lower.expected_return = result
        lower.output_types = declared_result if isinstance(declared_result,list) else None
        lower.optional_output = isinstance(declared_result,str) and resource(declared_result)
        lower.apply_guard(fn, [lower.env[p.arg] for p in fn.args.args], node)
        lower.statements(fn.body)
        result = result or lower.return_type
        function = lower.function(result=result)
        function['definition'] = fn.name
        if instance: function['instance'] = instance
        if kind == 'rule': function['public'] = next((self.f.literal(k.value) for d in fn.decorator_list if isinstance(d,ast.Call) for k in d.keywords if k.arg=='public'), True)
        if decorator(fn) == 'rule' and instance:
            function['interface'] = 'Module:' + instance + ':' + fn.name
            function['rule'] = fn.name
            function['output_type'] = declared_result
            function['owner'] = instance
            function['contract'] = 'Module:' + instance
        self.f.model['functions'].append(function)
        self.f.call_stack.pop()
        binding = {k: function[k] for k in ('instance', 'interface', 'rule', 'contract') if k in function}
        out = self.op('call', result, vals, name=name, node=node, role=kind, **binding)
        if isinstance(declared_result,list):
            outputs=[]
            for i,t in enumerate(declared_result):
                v=self.op('index', element(t) if resource(t) else t, [out,self.const(i)], node=node)
                self.ops[-1]['output_call']=out.id; self.ops[-1]['slot']=i
                outputs.append(v)
            return outputs
        return out

    def target_type(self, target):
        if isinstance(target, ast.Name):
            return self.lookup(target.id, target).type
        if isinstance(target, ast.Attribute):
            try:
                v = self.expr(target, reference=True)
                if isinstance(v, Value): return element(v.type) if v.type.startswith('signal<') else v.type
            except CompileError: pass
        if isinstance(target, ast.Attribute):
            typ = self.target_type(target.value)
            if resource(typ): typ=element(typ)
            fields = self.f.types.get(typ, [])
            field = next((f for f in fields if f['name'] == target.attr), None)
            if field:
                return field['type']
        if isinstance(target, ast.Subscript):
            typ = self.target_type(target.value)
            if typ.startswith(('cell<','ehr<')): typ=element(typ)
            return wrapped('queue', element(typ)) if typ.startswith('qarray<') else element(typ)
        self.f.error(target, 'unsupported assignment type')

    def module_call(self, node):
        return isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and isinstance(self.env.get(node.func.id), Instance)

    def output_destination(self, target):
        if isinstance(target, (ast.Tuple, ast.List)):
            return [self.output_destination(t) for t in target.elts]
        if isinstance(target, ast.Name):
            value = self.env.get(target.id)
            if isinstance(value, Value) and resource(value.type) and len(value.targets) == 1:
                return value
        self.f.error(target, 'Module result must bind to a previously declared component')

    def bind_return(self, value, targets, node):
        if value is None: return
        if isinstance(targets, list):
            if not isinstance(value, list) or len(value) != len(targets): self.f.error(node, 'Module output binding tuple arity mismatch')
            for v, t in zip(value, targets): self.bind_return(v, t, node)
            return
        if targets is None: self.f.error(node, 'Module returning None cannot return an output value')
        if not isinstance(value, Value): self.f.error(node, 'Module output binding requires one Rule output per destination')
        if isinstance(value, Value) and resource(value.type): self.f.error(node, 'Module returns no longer export internal components; return a Rule output to an explicit destination')
        key = '$module_result'
        self.env[key] = targets
        self.assignment(ast.Name(id=key, ctx=ast.Store()), value, node)
        del self.env[key]

    def assignment(self, target, value, node):
        existing = self.env.get(target.id) if isinstance(target, ast.Name) else self.expr(target, reference=True) if isinstance(target, ast.Attribute) and isinstance(self.expr(target.value, reference=True), Instance) else None
        if isinstance(target,ast.Subscript):
            probe=self.expr(target,reference=True)
            if isinstance(probe,Value) and probe.type.startswith('queue<'): existing=probe
        if isinstance(existing, Value) and resource(existing.type) and not existing.type.startswith('qarray<'):
            existing = self.imported(existing)
            value = self.cast(self.signal_value(value,node), element(existing.type), node)
            producing = next((o for o in self.ops if o['id'] == value.id), None)
            slot=producing.get('slot') if producing else None
            if producing and producing.get('output_call'): producing=next(o for o in self.ops if o['id']==producing['output_call'])
            if self.kind == 'body':
                if isinstance(target,ast.Subscript): self.f.error(node,'dynamic output binding belongs inside a Rule')
                if producing is None or producing.get('role') != 'rule': self.f.error(node, 'Module state assignment requires a Rule component output')
                declared = next(f['output_type'] for f in self.f.model['functions'] if f['name'] == producing['name'])
                if slot is not None: declared = declared[slot]
                if declared != existing.type: self.f.error(node, 'Rule component output type must match destination: ' + existing.type)
                if slot is None: producing['output_binding'] = next(iter(existing.targets))
                else:
                    binding=producing.setdefault('output_bindings',{})
                    binding[slot]=next(iter(existing.targets))
            else:
                operation = 'queue.push' if existing.type.startswith('queue<') else 'signal.write' if existing.type.startswith('signal<') else 'cell.write'
                self.op(operation, 'void', [existing,value], node=node)
            return
        if isinstance(target, ast.Name):
            if self.kind == 'body' and isinstance(value, Value):
                producer = next((o for o in self.ops if o['id'] == value.id), None)
                if producer and producer.get('output_call'): producer = next(o for o in self.ops if o['id'] == producer['output_call'])
                function = next((f for f in self.f.model['functions'] if producer and f['name'] == producer.get('name')), None)
                if function and (resource(function.get('output_type')) or isinstance(function.get('output_type'), list)):
                    self.f.error(node, 'Rule component output must bind to a previously declared component')
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
                    path.insert(0, base.attr)
                    base = base.value
                else:
                    path.insert(0, None)
                    indices.insert(0, base.slice)
                    base = base.value
            if not isinstance(base, ast.Name): self.f.error(node, 'local updates require a named aggregate')
            original = self.lookup(base.id, node)
            component=original if resource(original.type) else None
            if component and component.type.startswith('queue<'): self.f.error(node,'Queue payload update is unsupported; consume and produce')
            if component: original=self.signal_value(self.imported(component),node)
            indices = [self.expr(n) for n in indices]
            result = self.op('update', original.type, [original, value, *indices], path=path, node=node)
            if component: self.op('signal.write' if component.type.startswith('signal<') else 'cell.write','void',[self.imported(component),result],node=node)
            else: self.bind(base.id, result)
            return
        self.f.error(node, 'unsupported assignment target')

    def statements(self, statements):
        for node in statements:
            if self.terminated():
                break
            if isinstance(node, (ast.Assign, ast.AnnAssign)):
                targets = node.targets if isinstance(node, ast.Assign) else [node.target]
                if self.module_call(node.value):
                    if len(targets) != 1: self.f.error(node, 'Module output binding requires one assignment target')
                    outputs = self.output_destination(targets[0])
                    if isinstance(node, ast.AnnAssign) and self.f.annotation(node.annotation) != outputs.type:
                        self.f.error(node, 'Module output binding annotation must match the declared component')
                    self.call(node.value, None, outputs=outputs)
                    continue
                typ = self.f.annotation(node.annotation) if isinstance(node, ast.AnnAssign) else None
                if typ is None and isinstance(targets[0], (ast.Attribute, ast.Subscript)):
                    typ = self.target_type(targets[0])
                    if resource(typ): typ=element(typ)
                val = self.expr(node.value, typ)
                if typ:
                    val = self.cast(val, typ, node)
                for t in targets:
                    if isinstance(node,ast.AnnAssign) and isinstance(t,ast.Name) and not resource(typ): self.bind(t.id,val)
                    else: self.assignment(t, val, node)
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
                if isinstance(node.iter,ast.Call) and spelling(node.iter.func)=='range' and all(isinstance(x,ast.Constant) or spelling(x) in self.f.constants for x in node.iter.args):
                    self.statements(self.f.expand_loops([node]))
                    continue
                self.f.error(node, 'only statically bounded loops are supported')
            elif isinstance(node, ast.Return):
                if self.kind == 'body':
                    if self.module_call(node.value):
                        self.call(node.value, None, outputs=self.output_targets)
                    elif node.value is not None:
                        self.bind_return(self.expr(node.value, reference=True), self.output_targets, node)
                    self.op('return', 'void', node=node)
                else:
                    if getattr(self,'output_types',None):
                        if not isinstance(node.value,ast.Tuple) or len(node.value.elts)!=len(self.output_types): self.f.error(node,'return tuple arity mismatch')
                        values=[];present=[]
                        for expr,t in zip(node.value.elts,self.output_types):
                            typ=element(t) if resource(t) else t
                            omitted=isinstance(expr,ast.Constant) and expr.value is None
                            values.append(self.op('zero',typ,node=node) if omitted else self.cast(self.expr(expr,typ),typ,node))
                            present.append(self.const(not omitted,'bool'))
                        result=self.op('aggregate',self.expected_return,values+present,node=node)
                        self.return_type=self.expected_return
                        self.op('return','void',[result],node=node)
                        continue
                    val = self.expr(node.value, getattr(self, 'expected_return', None))
                    if val is None and getattr(self,'optional_output',False):
                        fallback=self.op('zero',self.expected_return,node=node)
                        self.op('return','void',[fallback],node=node,omitted=True)
                        continue
                    if val is None and getattr(self, 'expected_return', None) in (None, 'void'):
                        self.op('return', 'void', node=node); continue
                    if val is None or isinstance(val, list): self.f.error(node, 'Rule/helper returns one typed value')
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
        if self.module and '_resources' in self.module:
            self.module['_resources'].update(self.accesses)


def compile_source(path, top):
    return Frontend().compile(path, top)

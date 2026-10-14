"""AST-only static elaboration and typed MLIR control-flow construction.

Construction is symbolic: host configuration values are constructor arguments,
never evaluated by Python. Only literal construction loops are expanded here.
"""
import ast
import hashlib
from pathlib import Path
from .ir import CompileError, SCALARS, VERSION, Value, Instance, RuleRef, element, resource, wrapped, ArrayType, MaskType, array_type, descriptor, tuple_items, ModuleArray, ModuleChoice, ChoiceRule


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
        self.aliases = {}
        self.type_bases = {}
        self.qualified_types = {}
        self.files = set()
        self.model = dict(version=VERSION, top='', types=self.types, config=[], resources=[],
                          modules=[], functions=[], construction=[], module_arrays=[])
        self.definition_cache = {}
        self.synthetic_definitions = []
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
        self.reference_arrays = {}

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
                    if isinstance(node.value, ast.Call) and spelling(node.value.func) == 'ac.array':
                        self.aliases[target.id] = self.annotation(node.value)
                        continue
                    if isinstance(node.value, (ast.Call, ast.Attribute)) or isinstance(node.value, ast.Name) and node.value.id not in self.constants: self.globals[target.id] = node.value
                    else: self.constants[target.id] = self.literal(node.value)
            elif isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and isinstance(node.value.value, str): pass
            else: self.error(node, 'unsupported top-level syntax')
        # Validate all global declarations, including unused ones. They must
        # never create hardware outside an instance's static constructor.
        for value in self.globals.values():
            for call in ast.walk(value):
                if not isinstance(call, ast.Call): continue
                constructor = call.func.value if isinstance(call.func, ast.Subscript) else call.func
                name = spelling(constructor)
                definition = self.defs.get(name)
                if name in ('ac.queue', 'ac.cell', 'ac.ehr', 'ac.signal') or definition and decorator(definition) == 'module':
                    self.error(call, 'components and Module instances can only be constructed in __init__')

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
        if isinstance(node, ast.Tuple): return tuple(self.literal(x) for x in node.elts)
        if isinstance(node, ast.List): return [self.literal(x) for x in node.elts]
        if isinstance(node, ast.BinOp):
            import operator
            operations={ast.Add:operator.add,ast.Sub:operator.sub,ast.Mult:operator.mul,ast.FloorDiv:operator.floordiv,ast.Mod:operator.mod}
            if type(node.op) in operations:
                try: return operations[type(node.op)](self.literal(node.left),self.literal(node.right))
                except (ZeroDivisionError,TypeError): self.error(node,'invalid static arithmetic')
        try:
            return ast.literal_eval(node)
        except (ValueError, TypeError):
            self.error(node, 'expected a literal construction constant')

    def compatible(self, actual, expected):
        if actual == expected: return True
        if not isinstance(actual,str) or not isinstance(expected,str): return False
        if actual.startswith('selected<'): return self.compatible(element(actual), expected)
        if self.type_bases.get(actual) == expected: return True
        a,z = array_type(actual),array_type(expected)
        if isinstance(a,ArrayType) and isinstance(z,ArrayType):
            return a.shape == z.shape and self.compatible(a.item,z.item)
        if actual.startswith('tuple<') and expected.startswith('tuple<'):
            a,z = tuple_items(actual),tuple_items(expected)
            return len(a)==len(z) and all(self.compatible(x,y) for x,y in zip(a,z))
        return False

    def payload_type(self, typ, allow_selection=False):
        if not isinstance(typ,str) or resource(typ) or typ.startswith('mask<'): return False
        if typ.startswith('selected<'): return allow_selection and self.payload_type(element(typ),True)
        if typ in SCALARS: return True
        if typ.startswith('tuple<'): return all(self.payload_type(t,allow_selection) for t in tuple_items(typ))
        array = array_type(typ)
        if isinstance(array, ArrayType): return self.payload_type(array.item,allow_selection)
        if typ in self.types: return all(self.payload_type(f['type'],allow_selection) for f in self.types[typ])
        return False

    def complete_payload(self, typ):
        return self.payload_type(typ)

    def qualified_struct(self, base, field_types):
        base = self.type_bases.get(base, base)
        if [f['type'] for f in self.types[base]] == field_types: return base
        key = base,tuple(field_types)
        if key not in self.qualified_types:
            name = base + '$selection' + str(len(self.qualified_types))
            self.types[name] = [f | {'type':t} for f,t in zip(self.types[base],field_types)]
            self.type_bases[name] = base
            self.qualified_types[key] = name
        return self.qualified_types[key]

    def joined_type(self, a, z, node):
        if a == z: return a
        if a.startswith('selected<') and element(a)==z: return a
        if z.startswith('selected<') and element(z)==a: return z
        aa,za = array_type(a),array_type(z)
        if isinstance(aa,ArrayType) and isinstance(za,ArrayType) and aa.shape == za.shape:
            joined = ArrayType(self.joined_type(aa.item,za.item,node),aa.shape)
            return wrapped('selected',joined) if a.startswith('selected<') or z.startswith('selected<') else joined
        if a.startswith('tuple<') and z.startswith('tuple<'):
            left,right=tuple_items(a),tuple_items(z)
            if len(left)==len(right): return 'tuple<' + ','.join(self.joined_type(x,y,node) for x,y in zip(left,right)) + '>'
        if a in self.types and z in self.types and self.type_bases.get(a,a)==self.type_bases.get(z,z):
            return self.qualified_struct(a,[self.joined_type(x['type'],y['type'],node) for x,y in zip(self.types[a],self.types[z])])
        self.error(node,'branch values must have the same type')

    def annotation(self, node):
        if node is None:
            raise CompileError('a value type annotation is required here')
        name = spelling(node).removeprefix('ac.')
        if name in self.aliases: return self.aliases[name]
        if isinstance(node, ast.Call) and spelling(node.func) == 'ac.array':
            if len(node.args) != 1 or [k.arg for k in node.keywords] != ['shape']: self.error(node, 'array type requires one element type and shape=')
            shape = self.literal(node.keywords[0].value)
            if not isinstance(shape, tuple) or not shape or any(type(d) is not int or d <= 0 for d in shape): self.error(node, 'array shape must be a nonempty tuple of positive static integers')
            item = self.annotation(node.args[0])
            if array_type(item): self.error(node, 'use a multidimensional shape instead of array of array')
            return ArrayType(item, shape)
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
        self.collect_connections(inst)
        if '__call__' in inst.definitions:
            self.connect(inst, [], {}, builder, cls)
        self.model['construction'] = builder.ops
        self.model['instances'] = [dict(name=i.name, parent=i.parent, definition=i.source_name, rules=sorted(i.rules)) for i in self.instances]
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
        checked=[]
        for parameter,value in zip(init.args.args,args):
            typ=self.annotation(parameter.annotation)
            if isinstance(value,Instance):
                if value.source_name!=typ:self.error(parameter,'constructor Module dependency type mismatch')
            elif isinstance(value,ModuleArray):
                if value.type!=typ:self.error(parameter,'constructor Module array type mismatch')
            else:value=builder.check_type(value,typ,parameter)
            checked.append(value)
        env = dict(zip((p.arg for p in init.args.args), checked))
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
            expected = self.annotation(stmt.annotation) if isinstance(stmt, ast.AnnAssign) else None
            value = self.construct_value(stmt.value, prefix + '.' + target.id, builder, definitions, expected)
            if any(o['op'].startswith(('queue.', 'cell.', 'ehr.', 'signal.')) for o in builder.ops[start:]): self.error(stmt,'__init__ cannot read runtime state')
            if isinstance(value, Value) and any(o['op'].endswith(('read', 'write')) or o['op'] == 'call' for o in builder.ops if o['id'] == value.id): self.error(stmt, 'runtime computation is not allowed in __init__')
            env[target.id] = value
        builder.env = saved
        self.instance_stack.pop()
        self.constructing.remove(cls.name)
        for fn in instance.definitions.values(): self.closures[instance.name, id(fn)] = (env, definitions, instance.name)
        return instance

    def collect_connections(self, root):
        """Bind fixed references throughout the hierarchy before generating any Rule."""
        visited, active = set(), set()
        def resolve(node, env):
            if isinstance(node, ast.Name): return env.get(node.id)
            if isinstance(node, ast.Subscript):
                base = resolve(node.value, env)
                if isinstance(base,Value) and id(base) in self.reference_arrays:
                    coords = node.slice.elts if isinstance(node.slice,ast.Tuple) else [node.slice]
                    for coord in coords:
                        i = self.literal(coord)
                        entries = self.reference_arrays.get(id(base))
                        if entries is None or type(i) is not int or not 0 <= i < len(entries): self.error(coord,'static resource array index out of bounds')
                        base = entries[i]
                    return base
                if isinstance(base, ModuleArray):
                    coords = node.slice.elts if isinstance(node.slice,ast.Tuple) else [node.slice]
                    for coord in coords:
                        i = self.literal(coord)
                        if type(i) is not int or not 0 <= i < base.type.shape[0]: self.error(node,'static Module array index out of bounds')
                        stride = len(base.slots)//base.type.shape[0]
                        slots = base.slots[i*stride:(i+1)*stride]
                        base = slots[0][0][0] if len(base.type.shape)==1 else ModuleArray(ArrayType(base.type.item,base.type.shape[1:]),slots)
                    return base
                if isinstance(base, list):
                    i = self.literal(node.slice)
                    if type(i) is not int or not 0 <= i < len(base): self.error(node, 'static Module array index out of bounds')
                    return base[i]
            return None
        def visit(instance):
            if instance.name in active: self.error(instance.definitions['__call__'], 'recursive Module call')
            if instance.name in visited: return
            fn = instance.definitions.get('__call__')
            if fn is None: return
            active.add(instance.name)
            children = []
            def statements(body):
                for stmt in self.expand_loops(body):
                    if isinstance(stmt, ast.If):
                        statements(stmt.body); statements(stmt.orelse); continue
                    for node in ast.walk(stmt):
                        if not isinstance(node, ast.Call): continue
                        child = resolve(node.func, instance.env)
                        if not isinstance(child, Instance): continue
                        call = child.definitions.get('__call__')
                        if call is None: self.error(node, 'Module has no __call__; invoke its public Rules directly')
                        args = self.bind_args(call, node.args, {k.arg:k.value for k in node.keywords})
                        for p,arg in zip(call.args.args,args):
                            value = resolve(arg, instance.env)
                            typ = self.annotation(p.annotation)
                            if resource(typ) or typ in self.defs or array_type(typ) and not self.payload_type(typ):
                                if value is None: self.error(arg, 'fixed interface requires an existing reference')
                                actual = value.source_name if isinstance(value, Instance) else value.type
                                if actual != typ: self.error(arg, 'fixed connection type mismatch')
                                child.env[p.arg] = value
                        for rule in child.definitions.values(): self.closures[child.name,id(rule)] = (child.env,child.definitions,child.name)
                        children.append(child)
            statements(fn.body)
            for child in children: visit(child)
            active.remove(instance.name); visited.add(instance.name)
        visit(root)

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

    def expand_list(self, node):
        if not isinstance(node, ast.ListComp): return node
        import copy
        if len(node.generators) != 1: self.error(node, 'use nested static comprehensions')
        gen = node.generators[0]
        if gen.ifs or gen.is_async or not isinstance(gen.target, ast.Name) or not isinstance(gen.iter, ast.Call) or spelling(gen.iter.func) != 'range' or gen.iter.keywords:
            self.error(node, 'array comprehension requires a static range')
        values = []
        for i in range(*(self.literal(a) for a in gen.iter.args)):
            class Substitute(ast.NodeTransformer):
                def visit_Name(self, n):
                    return ast.copy_location(ast.Constant(i), n) if n.id == gen.target.id else n
            values.append(Substitute().visit(copy.deepcopy(node.elt)))
        result = ast.copy_location(ast.List(elts=values, ctx=ast.Load()), node)
        result.source = getattr(node,'source','')
        return result

    def record_module_array(self, value, name):
        self.model['module_arrays'].append(dict(name=name,owner=self.instance_stack[-1],shape=value.type.shape,
            targets=[instance.name for slot in value.slots for instance,_ in slot]))

    def construct_value(self, node, name, builder, defs, expected=None):
        if array_type(expected):
            array = array_type(expected)
            node = self.expand_list(node)
            if not isinstance(node, ast.List) or len(node.elts) != array.shape[0]: self.error(node, 'array construction must exactly match shape')
            subtype = ArrayType(array.item, array.shape[1:]) if len(array.shape) > 1 else array.item
            vals = [self.construct_value(n, name + '.' + str(i), builder, defs, subtype) for i,n in enumerate(node.elts)]
            if vals and all(isinstance(v, ModuleArray) for v in vals):
                result = ModuleArray(array,[slot for child in vals for slot in child.slots])
                self.record_module_array(result,name)
                return result
            if any(isinstance(v, Instance) for v in vals):
                if any(not isinstance(v, Instance) or v.source_name != array.item for v in vals): self.error(node, 'Module array element type mismatch')
                result = ModuleArray(array,[[(v,None)] for v in vals])
                self.record_module_array(result,name)
                return result
            vals = [builder.check_type(v, subtype, node) for v in vals]
            result = builder.array_aggregate(array, vals, node, array_name=name)
            self.reference_arrays[id(result)] = vals
            return result
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Subscript) and spelling(node.func.value) in ('ac.queue', 'ac.cell', 'ac.ehr', 'ac.signal'):
            declared = spelling(node.func.value)[3:]
            kind = declared
            typ = self.annotation(node.func.slice)
            if not self.complete_payload(typ): self.error(node, 'component storage requires complete payload types')
            kw = {x.arg: x.value for x in node.keywords}
            if node.args or set(kw) - {'capacity', 'initial'}:
                self.error(node, 'Queue accepts capacity= and initial= only')
            if kind in ('cell', 'ehr') and ('initial' not in kw or 'capacity' in kw): self.error(node, kind.title() + ' requires initial= and has no capacity')
            capacity = (builder.expr(kw['capacity']) if 'capacity' in kw else builder.const(1)) if kind == 'queue' else None
            if kind == 'queue' and array_type(typ) and 'initial' in kw: self.error(node, 'Queue of array starts empty; preloading array messages is unsupported')
            initial_node = kw.get('initial')
            if kind == 'queue' and isinstance(initial_node, ast.List):
                vals = [builder.check_type(builder.expr(x, typ), typ, x) for x in initial_node.elts]
                initial = builder.op('aggregate', 'tuple<' + ','.join(typ for _ in vals) + '>', vals, node=node)
            else:
                initial = builder.expr(initial_node, typ) if initial_node is not None else None
            if kind == 'signal' and kw: self.error(node, 'Signal has no initial state or capacity')
            if kind in ('cell', 'ehr'):
                if initial is None: self.error(node, kind.title() + ' initial must be a payload value')
                initial = builder.check_type(initial, typ, node)
            # scalar initial is one element; a list is a FIFO initialization sequence.
            values = [] if initial is None else [initial.id]
            sequence = initial is not None and initial.type != typ and initial.type.startswith('tuple<')
            self.model['resources'].append(dict(name=name, owner=self.instance_stack[-1] if self.instance_stack else 'top', kind=kind, type=typ, loc=builder.loc(node), **({'capacity': capacity.id} if capacity else {}),
                                                initial=values, sequence=sequence, visibility='ehr' if declared == 'ehr' else 'current' if kind == 'signal' else 'old'))
            return builder.op('resource', wrapped(kind, typ), name=name, targets={name}, node=node)
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
        return builder.expr(node, expected)

class Lower:
    def __init__(self, front, kind, name, env=None, builder=None, module=None, defs=None):
        self.f, self.kind, self.name = front, kind, name
        self.env = dict(env or {})
        self.builder, self.module, self.defs = builder, module, defs or {}
        self.ops, self.params = [], []
        self.values = {}
        self.definition_blocks = {}
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

    def op(self, op, typ, args=(), node=None, targets=(), **attrs):
        if op in ('cell.read', 'cell.write') and args and args[0].type.startswith('ehr<'):
            op = op.replace('cell.', 'ehr.')
        self.serial += 1
        value = Value(f'v{self.serial}', typ, set(targets))
        self.values[id(value)] = value
        if getattr(self, 'predicate_context', False) and (op.startswith(('cell.', 'queue.', 'ehr.', 'signal.')) or op == 'resource'):
            self.f.error(node, 'match predicate cannot access runtime state')
        if self.kind == 'helper' and (op.startswith(('cell.', 'queue.', 'ehr.', 'signal.')) or op == 'resource'):
            self.f.error(node, 'payload helper cannot access runtime state')
        if self.guard_context and op in ('queue.pop', 'queue.push', 'cell.write', 'ehr.write', 'signal.write', 'rule.call'): self.f.error(node, 'guard must only observe state')
        operation = dict(op=op, id=value.id, type=typ, args=[a.id for a in args], loc=self.loc(node), **attrs)
        self.block['ops'].append(operation)
        self.definition_blocks[value.id] = self.block
        self.ops.append(operation)
        if self.kind == 'body' and typ != 'void' and not resource(typ): self.f.body_values[id(value)] = (value,self)
        return value

    def terminated(self):
        return self.block.get('returned', False) or bool(self.block['ops'] and self.block['ops'][-1]['op'] in ('return', 'yield'))

    def prepare_flow(self):
        if getattr(self, 'flow_prepared', False) or self.kind == 'construction': return
        self.flow_prepared = True
        self.bind('$live', self.const(True))
        result = getattr(self, 'expected_return', None)
        if result and result != 'void':
            self.bind('$result', self.op('zero', result))
            self.bind('$present', self.const(False))

    def finish_flow(self):
        if not getattr(self, 'flow_prepared', False) or getattr(self, 'flow_finished', False): return
        self.flow_finished = True
        if '$result' in self.env:
            value = self.env['$result']
            self.return_type = value.type
            if getattr(self, 'optional_output', False):
                value = self.op('optional', value.type, [value, self.env['$present']])
            elif self.kind in ('helper', 'rule'):
                returned = self.op('unary', 'bool', [self.env['$live']], operator='not')
                self.op('check', 'void', [returned])
            self.op('return', 'void', [value])
        else:
            self.op('return', 'void')

    def function(self, result=None):
        self.finish_flow()
        if not self.terminated(): self.op('return', 'void')
        if result and self.f.compatible(self.return_type, result): result = self.return_type
        return dict(name=self.name, kind=self.kind, params=self.params, guard=getattr(self, 'guard_blocks', None), guard_info=getattr(self, 'guard_info', None),
                    result=result or self.return_type,
                    blocks=[dict(label='entry', params=[], ops=self.blocks[0]['ops'])])

    def promote(self, value, typ):
        if value.type == typ: return value
        if typ.startswith('selected<'):
            domain = self.op('array.domain',MaskType(array_type(value.type).shape),[value])
            data = self.op('index',element(value.type),[value,self.const(0)],targets=value.targets) if value.type.startswith('selected<') else value
            data = self.promote(data,element(typ))
            return self.op('array.select',typ,[data,domain],targets=value.targets)
        if typ.startswith('tuple<'):
            elements=[]
            for i,(a,z) in enumerate(zip(tuple_items(value.type),tuple_items(typ))):
                child=self.op('index',a,[value,self.const(i)])
                elements.append(self.promote(child,z))
            return self.op('aggregate',typ,elements)
        if typ in self.f.types:
            elements=[]
            for a,z in zip(self.f.types[value.type],self.f.types[typ]):
                child=self.op('field',a['type'],[value],field=a['name'])
                elements.append(self.promote(child,z['type']))
            return self.op('aggregate',typ,elements)
        a,z = array_type(value.type),array_type(typ)
        if isinstance(a,ArrayType) and isinstance(z,ArrayType):
            actual = ArrayType(a.item,a.shape[1:]) if len(a.shape)>1 else a.item
            expected = ArrayType(z.item,z.shape[1:]) if len(z.shape)>1 else z.item
            elements = [self.promote(self.op('array.index',actual,[value,self.const(i)]),expected) for i in range(a.shape[0])]
            return self.op('aggregate',typ,elements)
        self.f.error(None,'incompatible qualified payload')

    def array_aggregate(self, array, values, node, **attrs):
        subtype = ArrayType(array.item,array.shape[1:]) if len(array.shape)>1 else array.item
        for value in values: subtype = self.f.joined_type(subtype,value.type,node)
        values = [self.promote(value,subtype) for value in values]
        if len(array.shape)>1 and subtype.startswith('selected<'):
            row = array_type(subtype)
            data = [self.op('index',row,[value,self.const(0)]) for value in values]
            masks = [self.op('array.domain',MaskType(row.shape),[value]) for value in values]
            typ = ArrayType(row.item,array.shape)
            packed = self.op('aggregate',typ,data,node=node,**attrs)
            mask = self.op('aggregate',MaskType(array.shape),masks,node=node)
            return self.op('array.select',wrapped('selected',typ),[packed,mask],node=node)
        typ = ArrayType(subtype.item if len(array.shape)>1 else subtype,array.shape)
        return self.op('aggregate',typ,values,node=node,targets=set().union(*(v.targets for v in values)),**attrs)

    def branch(self, condition, yes_fn, no_fn, node=None):
        """Build one structured if region with a single typed SSA result."""
        parent = self.block
        branches, results = [], []
        for fn in (yes_fn, no_fn):
            self.block = dict(label='region', params=[], ops=[])
            results.append(fn())
            branches.append(self.block)
        a, z = results
        typ = self.f.joined_type(a.type,z.type,node)
        self.block = branches[0]; a = self.promote(a,typ)
        self.block = branches[1]; z = self.promote(z,typ)
        for block, value in zip(branches, (a,z)):
            self.block = block
            self.op('yield', 'void', [value])
        self.block = parent
        return self.op('if', typ, [condition], branches=[b['ops'] for b in branches], node=node, targets=a.targets | z.targets)

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

    def param(self, name, typ, targets=()):
        value = self.op('param', typ, name=name, targets=targets)
        self.params.append(dict(name=name, type=typ, id=value.id, targets=sorted(targets)))
        return value

    def imported(self, val):
        if isinstance(val, list):
            return [self.imported(v) for v in val]
        if not isinstance(val, Value) or self.builder is None:
            return val
        if self.values.get(id(val)) is val:
            return val
        entry = self.f.body_values.get(id(val))
        origin = entry[1] if entry else None
        if origin and origin is not self:
            exported = origin.export_value(val)
            return self.op('external', val.type, source=origin.name, identity=exported.id)
        if resource(val.type) and len(val.targets) == 1:
            current = self.block
            self.block = self.blocks[0]
            tail = self.block['ops'].pop() if self.terminated() else None
            result = self.op('resource', val.type, name=next(iter(val.targets)), targets=val.targets)
            if tail: self.block['ops'].append(tail)
            self.block = current
            return result
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
            if id(val) in self.f.reference_arrays: self.f.reference_arrays[id(self.imports[val.id])] = self.f.reference_arrays[id(val)]
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
            block = self.definition_blocks[val.id]
            position = next((i + 1 for i, o in enumerate(block['ops']) if o['id'] == val.id or any(v['id'] == val.id for v in o.get('outputs', []))), 0)
            self.block = block
            exported = self.op('export', val.type, [val])
            block['ops'].insert(position, block['ops'].pop())
            self.block = current
            self.exports[val.id] = exported
            if self.module and 'body' in self.module: self.module['body'] = self.function()
        return self.exports[val.id]

    def check_type(self, value, typ, node=None):
        if not isinstance(value, Value): self.f.error(node, 'expected a typed value, not a Module instance')
        if self.f.compatible(value.type, typ):
            return value
        if value.type == typ:
            return value
        if value.type in SCALARS and typ in SCALARS and 'bool' not in (value.type, typ):
            literal = next((o for o in self.ops if o['id'] == value.id and o['op'] == 'const'), None)
            if literal is not None and value.id in self.untyped_literals:
                return self.const(literal['value'], typ, node)
        self.f.error(node, f'type mismatch: {value.type} → {typ}')

    def choose(self, condition, yes_fn, no_fn, node):
        return self.branch(condition, yes_fn, no_fn, node)

    def signal_value(self, value, node=None):
        if not isinstance(value, Value) or not resource(value.type): return value
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
        if isinstance(node, (ast.List, ast.ListComp)):
            node = self.f.expand_list(node)
            array = array_type(expected)
            if not isinstance(array, ArrayType): self.f.error(node, 'array construction requires an explicit array type')
            if len(node.elts) != array.shape[0]: self.f.error(node, 'array construction must exactly match shape')
            subtype = ArrayType(array.item, array.shape[1:]) if len(array.shape) > 1 else array.item
            vals = [self.check_type(self.expr(x, subtype), subtype, x) for x in node.elts]
            return self.array_aggregate(array,vals,node)
        if isinstance(node, ast.Attribute):
            qualified = spelling(node)
            if qualified in self.f.globals: return self.lookup(qualified, node)
            if qualified in self.f.constants: return self.const(self.f.constants[qualified], node=node)
            val = self.expr(node.value, reference=True)
            if isinstance(val, ModuleChoice):
                if any(node.attr not in instance.rules for instance,_ in val.candidates): self.f.error(node,'unknown public Rule on Module array')
                return ChoiceRule(val,node.attr)
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
            if isinstance(val, ModuleArray): return self.module_index(val,node.slice,node)
            if isinstance(val, list):
                index = self.f.literal(node.slice)
                if not isinstance(index, int) or not 0 <= index < len(val): self.f.error(node, 'fixed output tuple index out of range')
                return self.imported(val[index])
            if resource(val.type): val = self.signal_value(val, node)
            coordinates = node.slice.elts if isinstance(node.slice, ast.Tuple) else [node.slice]
            for coord in coordinates:
                array = array_type(val.type)
                if not array: self.f.error(node, f'cannot index {val.type}')
                try: static = self.f.literal(coord)
                except CompileError: static = None
                if type(static) is int and not 0 <= static < array.shape[0]: self.f.error(coord,'static array index out of bounds')
                idx = self.expr(coord)
                if array_type(idx.type) and isinstance(array_type(idx.type), MaskType):
                    if not isinstance(array, ArrayType) or array.shape != array_type(idx.type).shape: self.f.error(node, 'mask shape must match array')
                    val = self.op('array.select', wrapped('selected', str(array)), [val, idx], node=node, targets=val.targets)
                    continue
                if idx.type not in SCALARS or idx.type == 'bool': self.f.error(coord, 'array coordinate requires an integer')
                literal = next((o for o in self.ops if o['id'] == idx.id and o['op'] == 'const'), None)
                if literal and not 0 <= literal['value'] < array.shape[0]: self.f.error(coord, 'static array index out of bounds')
                if isinstance(array, MaskType):
                    typ = MaskType(array.shape[1:]) if len(array.shape) > 1 else 'bool'
                else:
                    typ = ArrayType(array.item, array.shape[1:]) if len(array.shape) > 1 else array.item
                    if val.type.startswith('selected<') and array_type(typ): typ = wrapped('selected', typ)
                val = self.op('array.index', typ, [val, idx], node=node, targets=val.targets, signed_index=idx.type.startswith('i'))
            return val
        if isinstance(node, ast.BinOp):
            a, b = self.expr(node.left, expected), self.expr(node.right, expected)
            typ = expected or self.common(a, b, node)
            a, b = self.check_type(a, typ, node), self.check_type(b, typ, node)
            operators = {ast.Add: '+', ast.Sub: '-', ast.Mult: '*', ast.FloorDiv: '//', ast.Mod: '%',
                         ast.BitAnd: '&', ast.BitOr: '|', ast.BitXor: '^', ast.LShift: '<<', ast.RShift: '>>'}
            if type(node.op) not in operators:
                self.f.error(node, 'unsupported arithmetic operator')
            return self.op('binary', typ, [a, b], operator=operators[type(node.op)], node=node)
        if isinstance(node, ast.UnaryOp):
            if isinstance(node.op, (ast.USub,ast.UAdd)) and isinstance(node.operand,ast.Constant) and type(node.operand.value) is int:
                n = -node.operand.value if isinstance(node.op,ast.USub) else node.operand.value
                return self.const(n, expected or ('i32' if n < 0 else None), node)
            a = self.expr(node.operand, expected)
            if isinstance(node.op, ast.Not): a = self.check_type(a, 'bool', node)
            operators = {ast.Not: 'not', ast.Invert: '~', ast.USub: '-', ast.UAdd: '+'}
            return self.op('unary', 'bool' if isinstance(node.op, ast.Not) else a.type,
                           [a], operator=operators[type(node.op)], node=node)
        if isinstance(node, ast.BoolOp):
            out = self.check_type(self.expr(node.values[0]), 'bool', node)
            for rhs in node.values[1:]:
                evaluate = lambda: self.check_type(self.expr(rhs), 'bool', rhs)
                out = self.choose(out, lambda: self.const(True), evaluate, node) if isinstance(node.op, ast.Or) else self.choose(out, evaluate, lambda: self.const(False), node)
            return out
        if isinstance(node, ast.Compare):
            def compare(a, index):
                op, rhs = node.ops[index], node.comparators[index]
                if isinstance(op, (ast.In, ast.NotIn)):
                    if not isinstance(rhs, (ast.Tuple, ast.List)):
                        self.f.error(rhs, 'membership requires a literal tuple/list')
                    parts = [self.op('binary', 'bool', [a, self.check_type(self.expr(x, a.type), a.type, x)], operator='==', node=node) for x in rhs.elts]
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
                    value = self.op('binary', 'bool', [self.check_type(a, typ), self.check_type(b, typ)], operator=ops[type(op)], node=node)
                if index + 1 < len(node.ops):
                    return self.choose(value, lambda: compare(b, index + 1), lambda: self.const(False), node)
                return value
            return compare(self.expr(node.left), 0)
        if isinstance(node, ast.IfExp):
            c = self.check_type(self.expr(node.test), 'bool', node)
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
        self.f.error(node, f'type mismatch; integer widths must match: {a.type}, {b.type}')

    def module_index(self, array, subscript, node):
        coordinates = subscript.elts if isinstance(subscript,ast.Tuple) else [subscript]
        for coord in coordinates:
            idx = self.expr(coord)
            if isinstance(array_type(idx.type),MaskType):
                if array_type(idx.type).shape != array.type.shape: self.f.error(coord,'mask shape must match Module array')
                slots=[]
                import itertools
                for slot,coords in zip(array.slots,itertools.product(*(range(d) for d in array.type.shape))):
                    bit=idx
                    for i in coords:
                        shape=array_type(bit.type).shape
                        bit=self.op('array.index',MaskType(shape[1:]) if len(shape)>1 else 'bool',[bit,self.const(i)],node=node)
                    slots.append([(instance,bit if enable is None else self.op('binary','bool',[enable,bit],operator='and')) for instance,enable in slot])
                array=ModuleArray(array.type,slots)
                continue
            if idx.type not in SCALARS or idx.type=='bool': self.f.error(coord,'Module array coordinate requires integer')
            literal=next((o for o in self.ops if o['id']==idx.id and o['op']=='const'),None)
            size=array.type.shape[0];stride=len(array.slots)//size
            if literal:
                i=literal['value']
                if not 0<=i<size:self.f.error(coord,'static Module array index out of bounds')
                slots=array.slots[i*stride:(i+1)*stride]
            else:
                slots=[[] for _ in range(stride)]
                for i in range(size):
                    condition=self.op('binary','bool',[idx,self.const(i,idx.type)],operator='==',node=node)
                    for j in range(stride):
                        for instance,enable in array.slots[i*stride+j]:
                            slots[j].append((instance,condition if enable is None else self.op('binary','bool',[enable,condition],operator='and')))
            if len(array.type.shape)==1:
                candidates=slots[0]
                return candidates[0][0] if len(candidates)==1 and candidates[0][1] is None else ModuleChoice(candidates)
            array=ModuleArray(ArrayType(array.type.item,array.type.shape[1:]),slots)
        return array

    def choice_call(self, ref, args, node):
        import copy
        first=ref.choice.candidates[0][0]
        original=first.rules[ref.name]
        scope={}
        parameters=copy.deepcopy(original.args.args)
        enabled=[]
        branches=[]
        for i,(instance,condition) in enumerate(ref.choice.candidates):
            name='$candidate'+str(i);flag='$enable'+str(i)
            scope[name]=instance
            parameters.append(ast.arg(arg=flag,annotation=ast.Name(id='bool',ctx=ast.Load())))
            enabled.append(condition if condition is not None else self.const(True))
            call=ast.Call(func=ast.Attribute(value=ast.Name(id=name,ctx=ast.Load()),attr=ref.name,ctx=ast.Load()),args=[ast.Name(id=p.arg,ctx=ast.Load()) for p in original.args.args],keywords=[])
            branches.append((ast.Name(id=flag,ctx=ast.Load()),[ast.Return(value=call)]))
        valid=ast.BoolOp(op=ast.Or(),values=[condition for condition,_ in branches]) if len(branches)>1 else branches[0][0]
        body=[]
        for condition,statements in reversed(branches): body=[ast.If(test=condition,body=statements,orelse=body)]
        fn=ast.FunctionDef(name='$dispatch'+str(len(self.f.model['functions'])),args=ast.arguments(posonlyargs=[],args=parameters,vararg=None,kwonlyargs=[],kw_defaults=[],kwarg=None,defaults=[]),body=body,decorator_list=[ast.Call(func=ast.Attribute(value=ast.Name(id='ac',ctx=ast.Load()),attr='rule',ctx=ast.Load()),args=[],keywords=[ast.keyword(arg='guard',value=valid)])],returns=copy.deepcopy(original.returns))
        for part in ast.walk(fn):
            ast.copy_location(part,node);part.source=getattr(node,'source','')
        self.f.synthetic_definitions.append(fn)
        return self.behavior_call(fn,args+enabled,node,scope,'rule',self.defs,self.lexical_instance)

    def match(self, node):
        if len(node.args) != 1 or node.keywords: self.f.error(node, 'match requires one predicate')
        value = self.expr(node.func.value, reference=True)
        array = array_type(value.type)
        if not isinstance(array, ArrayType) or not self.f.payload_type(array.item, True):
            self.f.error(node, 'match requires an explicitly obtained payload array')
        domain = self.op('array.domain', MaskType(array.shape), [value], node=node)
        predicate = node.args[0]
        fn = self.defs.get(spelling(predicate), self.f.defs.get(spelling(predicate)))
        if not isinstance(predicate, ast.Lambda) and (fn is None or decorator(fn) != 'helper'):
            self.f.error(predicate, 'match predicate must be a lambda or payload helper')
        if isinstance(predicate, ast.Lambda) and (len(predicate.args.args) != 1 or predicate.args.defaults or predicate.args.vararg or predicate.args.kwarg or predicate.args.kwonlyargs):
            self.f.error(predicate, 'match lambda requires one payload parameter')
        def build(prefix, shape):
            items = []
            for i in range(shape[0]):
                coords = prefix + (i,)
                if len(shape) > 1:
                    items.append(build(coords, shape[1:]))
                    continue
                data, enabled = value, domain
                for index in coords:
                    data_shape = array_type(data.type)
                    target = ArrayType(array.item, data_shape.shape[1:]) if len(data_shape.shape) > 1 else array.item
                    if data.type.startswith('selected<') and array_type(target): target = wrapped('selected', target)
                    data = self.op('array.index', target, [data, self.const(index)], node=node)
                    mask_shape = array_type(enabled.type).shape
                    target = MaskType(mask_shape[1:]) if len(mask_shape) > 1 else 'bool'
                    enabled = self.op('array.index', target, [enabled, self.const(index)], node=node)
                def evaluate():
                    saved_env = dict(self.env)
                    saved_locals = set(getattr(self, 'locals', set()))
                    previous = getattr(self, 'predicate_context', False)
                    self.predicate_context = True
                    try:
                        if isinstance(predicate, ast.Lambda):
                            self.env[predicate.args.args[0].arg] = data
                            self.locals = saved_locals | {predicate.args.args[0].arg}
                            result = self.expr(predicate.body)
                        else:
                            scope, defs, instance = self.f.closures.get((self.lexical_instance, id(fn)), ({}, self.f.defs, None))
                            result = self.behavior_call(fn, [data], node, scope, 'helper', defs, instance)
                        return self.check_type(result, 'bool', predicate)
                    finally:
                        self.env = saved_env
                        self.locals = saved_locals
                        self.predicate_context = previous
                items.append(self.choose(enabled, evaluate, lambda: self.const(False), node))
            return self.op('aggregate', MaskType(shape), items, node=node)
        return build((), array.shape)

    def call(self, node, expected, outputs=None):
        if isinstance(node.func, ast.Attribute) and node.func.attr == 'match': return self.match(node)
        name = spelling(node.func)
        short = name.removeprefix('ac.')
        if short in SCALARS:
            self.f.error(node, 'scalar conversion calls are unsupported; use typed values')
        if name == 'len':
            if len(node.args) != 1 or node.keywords: self.f.error(node, 'len requires exactly one positional argument')
            value = self.expr(node.args[0], reference=True)
            array = array_type(value.type) if isinstance(value, (Value,ModuleArray)) else None
            if not array: self.f.error(node, 'len requires an array')
            return self.const(array.shape[0], 'u32', node)
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
                    vals.append(self.check_type(self.expr(provided[f['name']], f['type']), f['type'], node))
                elif f['default'] is not None:
                    vals.append(self.const(f['default'], f['type'], node))
                else:
                    vals.append(self.op('zero', f['type'], node=node))
            result_type = self.f.qualified_struct(name,[v.type for v in vals])
            return self.op('aggregate', result_type, vals, node=node)
        if self.module_call(node):
            if self.kind != 'body': self.f.error(node, 'Module __call__ connections belong in Module bodies')
            instance = self.expr(node.func,reference=True)
            args = [self.expr(x, reference=True) for x in node.args]
            keywords = {k.arg:self.expr(k.value, reference=True) for k in node.keywords}
            return self.f.connect(instance, args, keywords, self.builder, node, outputs=outputs, caller=self)
        bound = None
        fn = self.defs.get(name, self.f.defs.get(name))
        if not fn and isinstance(node.func, ast.Attribute):
            bound = self.expr(node.func)
            if isinstance(bound, RuleRef): fn = bound.definition
            if isinstance(bound, ChoiceRule): fn = bound.choice.candidates[0][0].rules[bound.name]
        if not fn:
            self.f.error(node, f'unsupported call: {name}')
        if decorator(fn) == 'module': self.f.error(node, 'Module construction belongs to __init__')
        supplied = self.f.bind_args(fn, node.args, {k.arg: k.value for k in node.keywords})
        args = [self.expr(x, self.f.annotation(p.annotation) if p.annotation else None) for p, x in zip(fn.args.args, supplied)]
        if isinstance(bound, ChoiceRule): return self.choice_call(bound,args,node)
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
        if kind == 'helper' and any(not isinstance(v, Value) or not self.f.payload_type(v.type, True) for v in args): self.f.error(node, 'helper parameters must be payload')
        if any(isinstance(v, (Instance, ModuleArray, ModuleChoice)) for v in args): self.f.error(node, 'Rule/helper cannot accept Module instances; bind the dependency statically')
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
            value = self.check_type(value, typ, node)
            if self.f.compatible(value.type, typ): typ = value.type
            lower.bind(parameter.arg, lower.param(parameter.arg, typ, value.targets))
            vals.append(value)
        used = self.f.capture_names(fn, definitions or self.defs, instance)
        for key, value in scope.items():
            if key.startswith('$'): continue
            if key in {p.arg for p in fn.args.args} or not isinstance(value, Value): continue
            # Construction captures are imported once as resource/config identities.
            if key in used and value is self.env.get(key) and key in getattr(self, 'locals', set()):
                value = self.lookup(key, node)
                lower.bind(key, lower.param(key, value.type, value.targets))
                vals.append(value)
        result = self.f.annotation(fn.returns) if fn.returns else None
        declared_result = result
        if isinstance(result, list): result = 'tuple<' + ','.join([element(t) if resource(t) else t for t in result] + (['bool' for t in result] if any(resource(t) for t in result) else [])) + '>'
        if result and resource(result): result = element(result)
        lower.expected_return = result
        lower.output_types = declared_result if isinstance(declared_result,list) else None
        lower.optional_output = isinstance(declared_result,str) and resource(declared_result)
        cache_key = (kind, id(fn), instance, lower.guard_context, tuple((v.type, tuple(sorted(v.targets))) for v in vals))
        cached = self.f.definition_cache.get(cache_key)
        if cached is not None:
            self.f.call_stack.pop()
            return self.emit_call(cached, vals, node, declared_result, kind)
        lower.apply_guard(fn, [lower.env[p.arg] for p in fn.args.args], node)
        lower.statements(fn.body)
        if lower.return_type.startswith('selected<'): result = lower.return_type
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
        self.f.definition_cache[cache_key] = function
        self.f.call_stack.pop()
        return self.emit_call(function, vals, node, declared_result, kind)

    def emit_call(self, function, vals, node, declared_result, kind):
        name, result = function['name'], function['result']
        binding = {k: function[k] for k in ('instance', 'interface', 'rule', 'contract') if k in function}
        out = self.op('call', result, vals, name=name, node=node, role=kind, **binding)
        if isinstance(declared_result,list):
            outputs=[]
            for i,t in enumerate(declared_result):
                v=self.op('index', tuple_items(result)[i], [out,self.const(i)], node=node)
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
            array = array_type(typ)
            if not array: self.f.error(target, 'array update requires array')
            if isinstance(target.slice, ast.Name):
                selection = self.env.get(target.slice.id)
                if isinstance(selection, Value) and isinstance(array_type(selection.type), MaskType): return array.item
            count = len(target.slice.elts) if isinstance(target.slice, ast.Tuple) else 1
            return ArrayType(array.item, array.shape[count:]) if count < len(array.shape) else array.item
        self.f.error(target, 'unsupported assignment type')

    def module_call(self, node):
        if not isinstance(node,ast.Call): return False
        target=node.func
        if isinstance(target,ast.Name): return isinstance(self.env.get(target.id),Instance)
        if isinstance(target,ast.Subscript):
            root=target
            while isinstance(root,ast.Subscript): root=root.value
            if isinstance(root,ast.Name) and isinstance(self.env.get(root.id),ModuleArray):
                value=self.expr(target,reference=True)
                if not isinstance(value,Instance): self.f.error(node,'Module __call__ requires static array coordinates')
                return True
        return False

    def output_destination(self, target):
        if isinstance(target, (ast.Tuple, ast.List)):
            return [self.output_destination(t) for t in target.elts]
        if isinstance(target, ast.Subscript):
            coordinates=[];root=target
            while isinstance(root,ast.Subscript):
                coords=root.slice.elts if isinstance(root.slice,ast.Tuple) else [root.slice]
                coordinates[0:0]=coords;root=root.value
            value=self.env.get(root.id) if isinstance(root,ast.Name) else None
            for coordinate in coordinates:
                try: index=self.f.literal(coordinate)
                except CompileError: self.f.error(coordinate,'output binding requires static array coordinates')
                entries=self.f.reference_arrays.get(id(value))
                if entries is None or type(index) is not int or not 0<=index<len(entries): self.f.error(target,'output destination must name a statically declared component')
                value=entries[index]
            if isinstance(value,Value) and resource(value.type) and len(value.targets)==1: return value
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
            target_type = self.target_type(target)
            target_array = array_type(target_type)
            is_component = resource(target_type) or isinstance(target_array,ArrayType) and resource(target_array.item)
            probe = self.output_destination(target) if self.kind == 'body' else self.expr(target,reference=True) if is_component else None
            array = array_type(probe.type) if isinstance(probe, Value) else None
            if array and isinstance(array, ArrayType) and resource(array.item):
                if self.kind != 'rule' or self.guard_context: self.f.error(node, 'multi-selection write belongs in a Rule')
                value = self.check_type(value, element(array.item), node)
                if not self.f.complete_payload(value.type): self.f.error(node, 'selected array and mask cannot be stored across cycles')
                self.op('array.write', 'void', [probe, value], node=node)
                return
            if isinstance(probe,Value) and resource(probe.type): existing=probe
        if isinstance(existing, Value) and resource(existing.type):
            existing = self.imported(existing)
            value = self.check_type(self.signal_value(value,node), element(existing.type), node)
            if not self.f.complete_payload(value.type): self.f.error(node, 'selected array and mask cannot be stored across cycles')
            producing = next((o for o in self.ops if o['id'] == value.id), None)
            slot=producing.get('slot') if producing else None
            if producing and producing.get('output_call'): producing=next(o for o in self.ops if o['id']==producing['output_call'])
            if self.kind == 'body':
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
                if function and (resource(function.get('output_type')) or isinstance(function.get('output_type'), list) and any(resource(t) for t in function['output_type'])):
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
                    coords = base.slice.elts if isinstance(base.slice, ast.Tuple) else [base.slice]
                    path[0:0] = [None] * len(coords)
                    indices[0:0] = coords
                    base = base.value
            if not isinstance(base, ast.Name): self.f.error(node, 'local updates require a named aggregate')
            original = self.lookup(base.id, node)
            component=original if resource(original.type) else None
            if component and component.type.startswith('queue<'): self.f.error(node,'Queue payload update is unsupported; consume and produce')
            if component: original=self.signal_value(self.imported(component),node)
            indices = [self.expr(n) for n in indices]
            if any(isinstance(array_type(i.type),MaskType) for i in indices): self.f.error(node, 'masked assignment requires component references')
            result = self.update_value(original,path,indices,value,node)
            if component and not self.f.complete_payload(result.type): self.f.error(node, 'selected array and mask cannot be stored across cycles')
            if component: self.op('signal.write' if component.type.startswith('signal<') else 'cell.write','void',[self.imported(component),result],node=node)
            else: self.bind(base.id, result)
            return
        self.f.error(node, 'unsupported assignment target')

    def update_value(self, original, path, indices, value, node):
        if not path: return value
        if isinstance(path[0],str):
            fields = self.f.types[original.type]
            values = []
            for field in fields:
                child = self.op('field',field['type'],[original],field=field['name'],node=node)
                if field['name'] == path[0]: child = self.update_value(child,path[1:],indices,value,node)
                values.append(child)
            typ = self.f.qualified_struct(original.type,[v.type for v in values])
            return self.op('aggregate',typ,values,node=node)
        return self.op('update',original.type,[original,value,*indices],path=path,node=node,signed_indices=[i.type.startswith('i') for i in indices])

    def statements(self, statements):
        self.prepare_flow()
        for statement_index, node in enumerate(statements):
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
                    val = self.check_type(val, typ, node)
                for t in targets:
                    if isinstance(node,ast.AnnAssign) and isinstance(t,ast.Name) and not resource(typ): self.bind(t.id,val)
                    else: self.assignment(t, val, node)
            elif isinstance(node, ast.AugAssign):
                old = self.expr(node.target)
                rhs = self.expr(node.value, old.type)
                operators = {ast.Add: '+', ast.Sub: '-', ast.BitOr: '|', ast.BitAnd: '&', ast.BitXor: '^', ast.Mult: '*'}
                if type(node.op) not in operators:
                    self.f.error(node, 'unsupported augmented assignment')
                val = self.op('binary', old.type, [old, self.check_type(rhs, old.type)], operator=operators[type(node.op)], node=node)
                self.assignment(node.target, val, node)
            elif isinstance(node, ast.If):
                cond = self.check_type(self.expr(node.test), 'bool', node)
                parent = self.block
                before, locals_before = dict(self.env), set(getattr(self, 'locals', set()))
                branches, environments, endings = [], [], []
                for body in (node.body, node.orelse):
                    self.block = dict(label='region', params=[], ops=[])
                    self.env, self.locals = dict(before), set(locals_before)
                    self.statements(body)
                    branches.append(self.block)
                    environments.append((dict(self.env), set(self.locals)))
                    endings.append(self.block.get('returned', False))
                names = sorted(set(environments[0][0]) & set(environments[1][0]))
                changed = [n for n in names if all(isinstance(env[n], Value) for env,_ in environments)
                           and any(n in local for _,local in environments)
                           and environments[0][0][n] is not environments[1][0][n]]
                types, values = [], [[],[]]
                for name in changed:
                    pair = []
                    for i,(env,local) in enumerate(environments):
                        self.block = branches[i]
                        pair.append(env[name] if name in local else self.imported(env[name]))
                    typ = self.f.joined_type(pair[0].type,pair[1].type,node)
                    for i in range(2):
                        self.block = branches[i]
                        pair[i] = self.promote(pair[i],typ)
                    types.append(pair[0].type)
                    for i in range(2): values[i].append(pair[i])
                for i,block in enumerate(branches):
                    self.block = block
                    self.op('yield','void',values[i])
                self.block, self.env, self.locals = parent, dict(before), set(locals_before)
                self.op('if', 'void', [cond], branches=[b['ops'] for b in branches], node=node)
                operation = self.ops[-1]
                outputs = []
                for i,(name,typ) in enumerate(zip(changed,types)):
                    self.serial += 1
                    value = Value('v' + str(self.serial), typ, values[0][i].targets | values[1][i].targets)
                    outputs.append(dict(id=value.id,type=typ))
                    self.definition_blocks[value.id] = parent
                    self.bind(name,value)
                operation['outputs'] = outputs
                if all(endings):
                    self.block['returned'] = True
                elif any(endings):
                    # Only a returning branch changes continuation. Ordinary if
                    # statements never add a dependency to unrelated following work.
                    remaining = statements[statement_index+1:]
                    if remaining:
                        live = self.env['$live']
                        synthetic = ast.If(test=ast.Name(id='$live',ctx=ast.Load()),body=remaining,orelse=[])
                        ast.copy_location(synthetic,node)
                        self.statements([synthetic])
                    return
            elif isinstance(node, ast.For):
                if not isinstance(node.iter,ast.Call) or spelling(node.iter.func)!='range' or node.iter.keywords:
                    self.f.error(node,'only statically bounded range loops are supported')
                import copy
                loop=copy.deepcopy(node)
                bounds=[]
                for expression in node.iter.args:
                    try: value=self.f.literal(expression)
                    except CompileError:
                        if isinstance(expression,ast.Call) and spelling(expression.func)=='len':
                            result=self.expr(expression)
                            producer=next(o for o in self.ops if o['id']==result.id)
                            value=producer.get('value')
                        else: self.f.error(expression,'only statically bounded loops are supported')
                    if type(value) is not int: self.f.error(expression,'static range requires integer bounds')
                    bounds.append(ast.copy_location(ast.Constant(value),expression))
                if not 1<=len(bounds)<=3: self.f.error(node,'range requires one to three bounds')
                if len(bounds)==3 and bounds[2].value==0: self.f.error(node,'range step cannot be zero')
                loop.iter.args=bounds
                self.statements(self.f.expand_loops([loop]))
            elif isinstance(node, ast.Return):
                if self.kind == 'body':
                    if self.module_call(node.value): self.call(node.value, None, outputs=self.output_targets)
                    elif node.value is not None: self.bind_return(self.expr(node.value, reference=True), self.output_targets, node)
                else:
                    expected = getattr(self, 'expected_return', None)
                    if getattr(self, 'output_types', None):
                        if not isinstance(node.value, ast.Tuple) or len(node.value.elts) != len(self.output_types): self.f.error(node, 'return tuple arity mismatch')
                        values, present = [], []
                        for expr,t in zip(node.value.elts,self.output_types):
                            typ = element(t) if resource(t) else t
                            omitted = isinstance(expr,ast.Constant) and expr.value is None
                            if omitted and not resource(t): self.f.error(expr,'payload tuple cannot omit values')
                            values.append(self.op('zero',typ,node=node) if omitted else self.check_type(self.expr(expr,typ),typ,node))
                            present.append(self.const(not omitted))
                        if not any(resource(t) for t in self.output_types): present=[]
                        actual = 'tuple<' + ','.join(v.type for v in values+present) + '>'
                        value = self.op('aggregate',actual,values+present,node=node)
                    else:
                        value = self.expr(node.value,expected)
                    if isinstance(value,Value) and value.type == 'void': value = None
                    if value is None:
                        if expected not in (None,'void') and not getattr(self,'optional_output',False): self.f.error(node,'missing payload return value')
                    else:
                        if isinstance(value,list): self.f.error(node,'return requires a declared tuple type')
                        if expected: value = self.check_type(value,expected,node)
                        self.bind('$result',value)
                        self.bind('$present',self.const(True))
                        self.return_type = value.type
                self.bind('$live',self.const(False))
                self.block['returned'] = True
            elif isinstance(node, ast.Expr):
                if isinstance(node.value, ast.Constant) and isinstance(node.value.value, str):
                    continue
                self.expr(node.value)
            elif isinstance(node, ast.Assert):
                condition = self.check_type(self.expr(node.test), 'bool', node)
                self.op('check', 'void', [condition], node=node)
            elif isinstance(node, ast.Pass):
                pass
            else:
                self.f.error(node, f'unsupported statement: {type(node).__name__}')
        if self.module and '_resources' in self.module:
            self.module['_resources'].update(self.accesses)


def compile_source(path, top):
    return Frontend().compile(path, top)

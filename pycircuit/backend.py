"""C++ emission from serialized typed ACIR only; intentionally does not import ast."""
from pathlib import Path
from .ir import CompileError, SCALARS, VERSION, element

CPP = {'bool': 'bool', 'void': 'void', **{f'{s}{w}': f'std::{"u" if s == "u" else ""}int{w}_t'
       for s in ('u', 'i') for w in (8, 16, 32, 64)}}


def ctype(typ):
    if typ in CPP:
        return CPP[typ]
    if typ.startswith('array<'):
        inside, n = typ[6:-1].rsplit(',', 1)
        return f'std::array<{ctype(inside)}, {n}>'
    if typ.startswith('vector<'):
        return f'std::vector<{ctype(element(typ))}>'
    if typ.startswith('queue<'):
        return f'gfsim::Queue<{ctype(element(typ))}>*'
    if typ.startswith('signal<'):
        return f'gfsim::Signal<{ctype(element(typ))}>*'
    if typ.startswith('qarray<'):
        return f'std::vector<gfsim::Queue<{ctype(element(typ))}>*>'
    return typ


class Emitter:
    def __init__(self, model):
        if model.get('version') != VERSION:
            raise CompileError('unsupported ACIR version')
        self.m = model
        self.top = model['top']
        self.resources = {r['name']: r for r in model['resources']}
        self.construction = {o['id']: o for o in model['construction']}

    def expr(self, op, names, owner='model.'):
        a = [names[x] for x in op['args']]
        typ, kind = op['type'], op['op']
        ct = ctype(typ)
        if kind == 'const':
            v = op['value']
            if isinstance(v, bool):
                return 'true' if v else 'false'
            return f'ac_detail::cast<{ct}>({v % (1 << 64)}ULL)'
        if kind == 'zero':
            return ct + '{}'
        if kind == 'param':
            return op['name']
        if kind == 'config':
            return owner + op['name']
        if kind == 'resource':
            name = owner + op['name']
            return name + '.refs' if typ.startswith('qarray<') else '&' + name
        if kind == 'cast':
            return f'ac_detail::cast<{ct}>({a[0]})'
        if kind == 'binary':
            operator = op['operator']
            helpers = {'+': 'add', '-': 'sub', '*': 'mul', '//': 'div', '%': 'mod', '<<': 'shl', '>>': 'shr'}
            if operator in helpers:
                return f'ac_detail::{helpers[operator]}<{ct}>({a[0]}, {a[1]})'
            operator = {'and': '&&', 'or': '||'}.get(operator, operator)
            return f'ac_detail::cast<{ct}>(({a[0]}) {operator} ({a[1]}))'
        if kind == 'unary':
            operator = op['operator']
            if operator == '-':
                return f'ac_detail::neg<{ct}>({a[0]})'
            return f'ac_detail::cast<{ct}>({"!" if operator == "not" else operator}({a[0]}))'
        if kind == 'select':
            return f'({a[0]} ? {a[1]} : {a[2]})'
        if kind == 'field':
            return f'({a[0]}).{op["field"]}'
        if kind == 'index':
            return f'({a[0]}).at({a[1]})'
        if kind == 'length':
            return f'ac_detail::cast<std::uint32_t>(({a[0]}).size())'
        if kind == 'range':
            return f'ac_detail::range({a[0]})'
        if kind == 'aggregate':
            return ct + '{' + ', '.join(a) + '}'
        if kind == 'call':
            return op['name'] + '(' + ', '.join(a) + ')'
        if kind == 'queue.read':
            return f'({a[0]})->peek()'
        if kind.startswith('queue.') and kind[6:] in ('empty', 'full', 'size'):
            return f'({a[0]})->{kind[6:]}()'
        if kind == 'signal.read':
            return f'({a[0]})->value()'
        raise CompileError(f'unknown ACIR value operation: {kind}')

    def body(self, function, owner='model.', rule=None):
        lines, names, types = [], {}, {}
        ops = function['ops']
        # Required input checks stay at the guarded operation, never at Rule entry.
        # Work sees immutable current state. A successful check proves this exact
        # Queue nonempty for later operations under the same guard (or any guard
        # if the check was unconditional). This avoids checking again before pop.
        nonempty = set()
        missing = f'model.sim.abortRule(rid_{rule}); return;' if rule else 'return;'
        if rule:
            count = sum(o['op'] == 'queue.pop' for o in ops)
            if count:
                lines.append(f'ac_detail::Pops<{count}> pops;')
        # SSA names are declared once; guarded definitions dominate their uses.
        for op in ops:
            name, kind = op['id'], op['op']
            types[name] = op['type']
            if kind in ('config', 'resource', 'param'):
                names[name] = self.expr(op, names, owner)
            else:
                names[name] = name
                if op['type'] != 'void':
                    lines.append(f'[[maybe_unused]] {ctype(op["type"])} {name}{{}};')
        for op in ops:
            kind = op['op']
            if kind in ('config', 'resource', 'param'):
                continue
            a = [names[x] for x in op['args']]
            guard = f'if ({names[op["guard"]]}) ' if op['guard'] else ''
            if kind == 'queue.read' and (rule or function['kind'] == 'work'):
                pointer = f'input_{op["id"]}'
                stmt = (f'{{\n    const auto* {pointer} = ({a[0]})->tryPeek();\n'
                        f'    if (!{pointer}) {{ {missing} }}\n'
                        f'    {op["id"]} = *{pointer};\n}}')
                nonempty.add((a[0], op['guard']))
            elif kind == 'queue.pop':
                stmt = f'pops.add({a[0]}, rid_{rule});'
            elif kind == 'queue.push':
                stmt = f'({a[0]})->proposePush(rid_{rule}, {a[1]});'
            elif kind == 'queue.revise':
                paths, typ = [], element(types[op['args'][0]])
                for field in op['path']:
                    paths.append(f'&{typ}::{field}')
                    typ = next(f['type'] for f in self.m['types'][typ] if f['name'] == field)
                stmt = f'({a[0]})->proposeRevise<{", ".join(paths)}>(rid_{rule}, {a[1]});'
            elif kind == 'rule.call':
                stmt = f'work_{op["name"]}({", ".join(a)});'
            elif kind == 'event':
                stmt = f'model.sim.requestWakeup(rid_{rule}, mid, {a[0]});'
            elif kind == 'check':
                stmt = f'if (!({a[0]})) throw std::invalid_argument("ACPy assertion failed at {Path(op["loc"]["file"]).name}:{op["loc"]["line"]}");'
            elif kind == 'complete':
                # complete is unconditional on normal fallthrough, including early returns.
                guard = ''
                stmt = f'model.sim.completeRule(rid_{rule});'
            elif kind == 'return':
                stmt = f'return {a[0]};'
            elif kind == 'update':
                path = '.'.join(op['path'])
                stmt = f'{{ {op["id"]} = {a[0]}; {op["id"]}.{path} = {a[1]}; }}'
            elif kind == 'array.update':
                stmt = f'{{ {op["id"]} = {a[0]}; {op["id"]}.at({a[1]}) = {a[2]}; }}'
            else:
                stmt = f'{op["id"]} = {self.expr(op, names, owner)};'
            if kind in ('queue.pop', 'queue.revise'):
                proof = (a[0], op['guard'])
                if (a[0], None) not in nonempty and proof not in nonempty:
                    stmt = f'{{\n    if (({a[0]})->empty()) {{ {missing} }}\n    {stmt}\n}}'
                nonempty.add(proof)
            loc = op.get('loc', {})
            if loc.get('line'):
                lines.append(f'// {Path(loc["file"]).name}:{loc["line"]} {kind}')
            lines.append(guard + stmt)
        if function['kind'] in ('helper', 'signal', 'initializer'):
            lines.append('throw std::logic_error("ACIR function reached end without a value");')
        return '\n'.join('    ' + line for statement in lines for line in statement.splitlines())

    def construct_expr(self, id):
        op = self.construction[id]
        names = {a: self.construct_expr(a) for a in op['args']}
        return self.expr(op, names, '')

    def signature(self, function):
        return ', '.join(f'{ctype(p["type"])} {p["name"]}' for p in function['params'])

    def resource_loop(self, name, action):
        r = self.resources[name]
        if r['kind'] == 'qarray':
            return f'for (auto* q : {name}.refs) {action("*q")}'
        return action(name)

    def signal_inputs(self, function):
        """Static provenance from ACIR, including every call's parameter targets.

        Follow resource-valued SSA only: a Signal read in Work passed as an
        ordinary value remains a parameter comparison, not a Rule dependency.
        Guards do not narrow the declarations.
        """
        params = {p['id']: set(p['targets']) for p in function['params']}
        origins, inputs = {}, set()
        for op in function['ops']:
            sources = set().union(*(origins.get(a, set()) for a in op['args']))
            if op['op'] == 'resource':
                sources = {op['name']}
            elif op['op'] == 'param':
                sources = params[op['id']]
            origins[op['id']] = sources if op['type'].startswith('signal<') else set()
            if op['op'] == 'signal.read':
                inputs.update(sources)
        return sorted(inputs)

    def emit(self, directory):
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        header = ['// Generated solely from typed ACIR. Do not edit.', '#pragma once',
                  '#include "ac_support.hpp"', 'namespace ac_generated {']
        for name, value in self.m.get('constants', {}).items():
            header.append(f'inline constexpr std::uint32_t {name} = {value}U;')
        for name, fields in self.m['types'].items():
            header.append('struct ' + name + ' {')
            header += [f'    {ctype(f["type"])} {f["name"]}{{}};' for f in fields]
            header += [f'    bool operator==(const {name}&) const = default;', '};']
        functions = list(self.m['functions'])
        for r in self.m['resources']:
            if r['kind'] == 'qarray':
                fn = dict(r['initializer'], result=r['type'])
                functions.append(fn)
        for fn in functions:
            header.append(f'{ctype(fn["result"])} {fn["name"]}({self.signature(fn)});')
        header.append(f'struct {self.top};')
        source = ['// Generated solely from typed ACIR. Do not edit.', '#include "model.hpp"', 'namespace ac_generated {']
        for fn in functions:
            source.append(f'{ctype(fn["result"])} {fn["name"]}({self.signature(fn)}) {{\n{self.body(fn)}\n}}')
        for m in self.m['modules']:
            cls = 'Module_' + m['name']
            header += [f'struct {cls} {{', f'    {self.top}& model;', '    gfsim::ModuleId mid{};', '    void Work();']
            for rule in m['rules']:
                fn, name = rule['function'], rule['name']
                header.append(f'    gfsim::RuleId rid_{name}{{}};')
                if fn['params']:
                    header.append(f'    struct Args_{name} {{')
                    header += [f'        {ctype(p["type"])} {p["name"]};' for p in fn['params']]
                    header += [f'        bool operator==(const Args_{name}&) const = default;', '    };',
                               f'    gfsim::ParameterCache<Args_{name}> cache_{name};']
                header += [f'    void work_{name}({self.signature(fn)});', f'    bool arbitrate_{name}();']
                begin = f'model.sim.beginRule(rid_{name}'
                if fn['params']:
                    begin += f', cache_{name}, Args_{name}{{' + ', '.join(p['name'] for p in fn['params']) + '}'
                # Direct Queue operations use explicit missing-input exits. Keep
                # the catch for compatibility with calls into resource helpers;
                # other exceptions still terminate the Simulator as before.
                source.append(f'void {cls}::work_{name}({self.signature(fn)}) {{\n    if (!{begin})) return;\n    try {{\n{self.body(fn, rule=name)}\n    }} catch (const gfsim::NeedInput&) {{ model.sim.abortRule(rid_{name}); }}\n}}')
                source.append(f'bool {cls}::arbitrate_{name}() {{ return model.sim.arbitrateRule(rid_{name}); }}')
            header.append('};')
            source.append(f'void {cls}::Work() {{\n{self.body(m["work"])}\n}}')
        header.append(f'struct {self.top} {{')
        for p in self.m['config']:
            header.append(f'    const {ctype(p["type"])} {p["name"]};')
        for r in self.m['resources']:
            name, typ, kind = r['name'], ctype(r['type']), r['kind']
            if kind == 'queue':
                cap = self.construct_expr(r['capacity'])
                init = [self.construct_expr(x) for x in r['initial']]
                values = init[0] if r['sequence'] else f'std::vector<{typ}>{{' + ', '.join(init) + '}'
                header.append(f'    gfsim::Queue<{typ}> {name}{{{cap}, {values}}};')
            elif kind == 'qarray':
                header.append(f'    ac_detail::QueueArray<{typ}> {name}{{{self.construct_expr(r["iterable"])}, {self.construct_expr(r["capacity"])}, {r["initializer"]["name"]}}};')
            elif kind == 'signal':
                header += [f'    {typ} evaluate_{name}();', f'    gfsim::Signal<{typ}> {name}{{this, [](void* p) {{ return static_cast<{self.top}*>(p)->evaluate_{name}(); }}}};']
                source.append(f'{typ} {self.top}::evaluate_{name}() {{\n{self.body(r["function"], owner="")}\n}}')
            else:
                raise CompileError(f'unknown resource kind: {kind}')
        # All resources precede Simulator so they outlive its destructor.
        header.append('    gfsim::Simulator sim;')
        for m in self.m['modules']:
            header.append(f'    Module_{m["name"]} {m["name"]}{{*this}};')
        params, init = [], []
        for p in self.m['config']:
            typ = p['type']
            argtyp = f'std::span<const {ctype(element(typ))}>' if typ.startswith('vector<') else ctype(typ)
            name = p['name']
            params.append(f'{argtyp} arg_{name}')
            init.append(f'{name}(arg_{name}.begin(), arg_{name}.end())' if typ.startswith('vector<') else f'{name}(arg_{name})')
        params += ['bool cache = true', 'bool reverse = false']
        header += [f'    {self.top}({", ".join(params)});', '};', '} // namespace ac_generated']
        sig = ', '.join(x.replace(' = true', '').replace(' = false', '') for x in params)
        source.append(f'{self.top}::{self.top}({sig}) : {", ".join(init + ["sim(cache)"])} {{')
        module_setup = []
        for m in self.m['modules']:
            cls, name = 'Module_' + m['name'], m['name']
            module_setup.append(f'{name}.mid = sim.addModule<&{cls}::Work>({name});')
        source += ['    if (reverse) {', *('        ' + s for s in reversed(module_setup)), '    } else {',
                   *('        ' + s for s in module_setup), '    }']
        for m in self.m['modules']:
            for r in m['rules']:
                name, rn = m['name'], r['name']
                source.append(f'    {name}.rid_{rn} = sim.addRule({name}.mid, [](void* p, auto&, auto) {{ return static_cast<Module_{name}*>(p)->arbitrate_{rn}(); }});')
        for r in self.m['resources']:
            method = 'addSignal' if r['kind'] == 'signal' else 'addQueue'
            source.append('    ' + self.resource_loop(r['name'], lambda q: f'sim.{method}({q});'))
        for m in self.m['modules']:
            for name in m['resources']:
                source.append('    ' + self.resource_loop(name, lambda q: f'sim.declareResource({m["name"]}.mid, {q});'))
            for r in m['rules']:
                for name in self.signal_inputs(r['function']):
                    source.append(f'    sim.declareInput({m["name"]}.rid_{r["name"]}, {name});')
                for name, effects in r['bindings'].items():
                    ops = ' | '.join('gfsim::' + e for e in effects)
                    source.append('    ' + self.resource_loop(name, lambda q: f'sim.bind({m["name"]}.rid_{r["name"]}, {q}, {ops});'))
        for r in self.m['resources']:
            if r['kind'] == 'signal':
                for name in r['inputs']:
                    source.append('    ' + self.resource_loop(name, lambda q: f'sim.declareInput({r["name"]}, {q});'))
        source += ['    sim.freeze();', '}', '} // namespace ac_generated']
        (directory / 'model.hpp').write_text('\n'.join(header) + '\n')
        (directory / 'model.cpp').write_text('\n'.join(source) + '\n')
        (directory / 'ac_support.hpp').write_text(Path(__file__).with_name('support.hpp').read_text())


def emit(model, directory):
    Emitter(model).emit(directory)

"""Draw ACIR v6 Module hierarchies and rule orders from rule-graph.json.

Uses Python's standard library and the Graphviz dot executable. It does not
load model sources or invoke the MLIR compiler.
"""
import argparse
from collections import defaultdict
import json
import os
from pathlib import Path
import shutil
import subprocess


LOCAL_DOT = Path(__file__).resolve().parents[1] / 'pycircuit/builds/rule-graphviz/env/bin/dot'
HISTORICAL_DOT = Path(__file__).resolve().parents[1] / 'reference/builds/rule-graphviz/env/bin/dot'
FORMATS = {'svg', 'png', 'pdf', 'dot'}


def quote(value):
    return json.dumps(str(value), ensure_ascii=False)


def sources(proofs):
    result = set()
    for proof in proofs:
        result.add(proof['source'])
        result.update(sources(proof.get('provenance', [])))
    return result


def to_dot(graph, rankdir='LR', hide_empty=False):
    if graph.get('format_version') != 6:
        raise ValueError('expected an ACIR v6 rule-graph.json; recompile old models')
    modules = {m['name']: m for m in graph['instances']}
    nodes = {n['id']: n for n in graph['nodes']}
    if len(modules) != len(graph['instances']) or len(nodes) != len(graph['nodes']):
        raise ValueError('duplicate Module or rule identity')
    if any(n['module'] not in modules for n in nodes.values()):
        raise ValueError('rule refers to an unknown Module')
    for edge in graph['edges']:
        if edge['source'] not in nodes or edge['target'] not in nodes:
            raise ValueError('edge refers to an unknown rule')
        if edge['kind'] != 'method.order':
            raise ValueError('unsupported edge kind: ' + edge['kind'])

    # Verify hierarchy before rendering or filtering it, including empty modules.
    for name in modules:
        seen, current = set(), name
        while current is not None:
            if current not in modules:
                raise ValueError('unknown parent Module: ' + current)
            if current in seen:
                raise ValueError('cycle in Module hierarchy: ' + name)
            seen.add(current)
            current = modules[current].get('parent')
    visible = set(modules)
    if hide_empty:
        visible = set()
        for node in nodes.values():
            current = node['module']
            while current is not None:
                visible.add(current)
                current = modules[current].get('parent')

    children, rules = defaultdict(list), defaultdict(list)
    for name in sorted(visible):
        children[modules[name].get('parent')].append(name)
    for node in nodes.values():
        rules[node['module']].append(node)
    ehr_by_module = defaultdict(list)
    for view in graph.get('ehr', []):
        owners = [name for name in modules if view['resource'].startswith(name + '.')]
        owner = max(owners, key=len) if owners else next((name for name, m in modules.items() if m.get('parent') is None), None)
        if owner: ehr_by_module[owner].append(view)
    module_ids = {name: 'm' + str(i) for i, name in enumerate(sorted(visible))}
    rule_ids = {name: 'r' + str(i) for i, name in enumerate(sorted(nodes))}
    lines = [
        'digraph rules {',
        f'  rankdir={rankdir};',
        '  graph [fontname="DejaVu Sans", bgcolor="white", pad=0.25, nodesep=0.35, ranksep=0.65, labelloc=t];',
        '  node [shape=box, style="rounded,filled", fillcolor="#eff6ff", color="#3b82f6", fontname="DejaVu Sans", fontsize=12];',
        '  edge [fontname="DejaVu Sans", fontsize=10, arrowsize=0.8];',
        '  label=' + quote(graph['top'] + ' — Module / rule order\n'
                          'Solid: SB order\n'
                          'Red: confirmed cycle | Orange: unproven order\n'
                          'Conditional edges retain path/guard details in tooltips') + ';',
    ]

    def emit_module(name, depth=1):
        indent = '  ' * depth
        lines.extend([
            indent + 'subgraph cluster_' + module_ids[name] + ' {',
            indent + '  label=' + quote('Module: ' + name) + ';',
            indent + '  color="#94a3b8"; style="rounded"; labeljust=l;',
        ])
        if ehr_by_module[name]:
            descriptions = [v['resource'] + '.' + v['aspect'] + ' (EHR: ' + str(v['port_count']) + ' ports)' for v in ehr_by_module[name]]
            lines.append(indent + '  ' + module_ids[name] + '_ehr [shape=note, fillcolor="#fef3c7", color="#d97706", fontsize=10, label=' + quote('\n'.join(descriptions)) + ', tooltip=' + quote(json.dumps(ehr_by_module[name], ensure_ascii=False)) + '];')
        for node in sorted(rules[name], key=lambda n: n['id']):
            accesses = sorted({a['resource'] + '.' + a['aspect'] + ' p' + str(a['port']) + ' ' + a['operation'] for a in node['accesses'] if a['view'] == 'ehr'})
            lines.append(indent + '  ' + rule_ids[node['id']] + ' [label='
                         + quote('rule: ' + node['rule'] + ('\nEHR: ' + '\n'.join(accesses) if accesses else '')) + ', tooltip=' + quote(node['id']) + '];')
        for child in children[name]:
            emit_module(child, depth + 1)
        if not rules[name] and not children[name]:
            methods = modules[name].get('methods', [])
            label = ('methods\n' + '\n'.join(', '.join(methods[i:i + 2])
                     for i in range(0, len(methods), 2))) if methods else 'no independent rules'
            lines.append(indent + '  ' + module_ids[name] + ' [label=' + quote(label)
                         + ', shape=plaintext, fillcolor="white", fontcolor="#64748b", fontsize=10];')
        lines.append(indent + '}')

    for root in children[None]:
        emit_module(root)

    # Lay out sibling method-only components in a row instead of creating a
    # tall empty column beside the rule network. These links are invisible and
    # express no program relation.
    for siblings in children.values():
        leaves = [name for name in siblings if not rules[name] and not children[name]]
        for before, after in zip(leaves, leaves[1:]):
            lines.append('  ' + module_ids[before] + ' -> ' + module_ids[after]
                         + ' [style=invis, weight=2];')

    severity = {}
    for diagnostic in graph.get('diagnostics', []):
        level = 2 if diagnostic['kind'] == 'confirmed-cycle' else 1
        for index in diagnostic['edges']:
            if not 0 <= index < len(graph['edges']):
                raise ValueError('diagnostic refers to an unknown edge')
            severity[index] = max(severity.get(index, 0), level)
    groups = defaultdict(list)
    for index, edge in enumerate(graph['edges']):
        groups[edge['source'], edge['target'], edge['kind']].append((index, edge))
    for (source, target, kind), entries in sorted(groups.items()):
        byresource, origins = defaultdict(set), set()
        for _, edge in entries:
            if kind == 'method.order':
                order = ' SB '.join(edge['interfaces'])
                ports = [p['ports'] for p in edge.get('provenance', []) if 'ports' in p]
                if ports: order += ' ' + ', '.join('p' + str(a) + ' → p' + str(b) for a,b in ports)
                byresource[edge['resource']].add(order)
                origins.update(sources(edge.get('provenance', [])))
        if kind == 'method.order':
            labels = [resource + '\n' + '; '.join(sorted(orders))
                      for resource, orders in sorted(byresource.items())]
            if origins:
                labels.append('[' + ', '.join(sorted(origins)) + ']')
        if all(edge['condition'] is not True for _, edge in entries):
            labels.append('(conditional)')
        level = max(severity.get(index, 0) for index, _ in entries)
        color = ('#dc2626' if level == 2 else '#d97706' if level == 1
                 else '#0f766e' if 'derived' in origins else '#475569')
        # A merged edge retains every contributing condition/witness. In
        # particular it does not turn separate guarded accesses into an AND.
        tooltip = json.dumps([edge for _, edge in entries], ensure_ascii=False)
        lines.append('  ' + rule_ids[source] + ' -> ' + rule_ids[target]
                     + ' [label=' + quote('\n'.join(labels)) + ', color=' + quote(color)
                     + ', style=' + quote('solid')
                     + ', penwidth=' + ('2' if level else '1.2')
                     + ', tooltip=' + quote(tooltip) + '];')
    return '\n'.join(lines + ['}']) + '\n'


def find_dot(explicit=None):
    candidate = explicit or os.environ.get('ACPY_GRAPHVIZ_DOT')
    if candidate:
        return candidate
    for installed in (LOCAL_DOT, HISTORICAL_DOT):
        if installed.is_file(): return str(installed)
    candidate = shutil.which('dot')
    if not candidate:
        raise ValueError('Graphviz dot not found; configure pycircuit/graphviz-environment.yml or pass --dot')
    return candidate


def render(graph, output, executable=None, rankdir='LR', hide_empty=False):
    output = Path(output)
    format_name = output.suffix.lower().lstrip('.')
    if format_name not in FORMATS:
        raise ValueError('output extension must be .svg, .png, .pdf or .dot')
    text = to_dot(graph, rankdir, hide_empty)
    if format_name == 'dot':
        data = text.encode('utf-8')
    else:
        result = subprocess.run([find_dot(executable), '-T' + format_name],
                                input=text.encode('utf-8'), capture_output=True, timeout=120)
        if result.returncode:
            raise ValueError('Graphviz failed: ' + result.stderr.decode('utf-8', errors='replace'))
        data = result.stdout
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    companion = output.with_suffix('.dot')
    if companion != output:
        companion.write_text(text, encoding='utf-8')
    return output, companion


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('graph', type=Path, help='ACIR v6 rule-graph.json')
    parser.add_argument('-o', '--output', type=Path,
                        help='SVG/PNG/PDF/DOT path; default: module-rule-graph.svg beside JSON')
    parser.add_argument('--dot', help='path to Graphviz dot executable')
    parser.add_argument('--rankdir', choices=['LR', 'TB'], default='LR')
    parser.add_argument('--hide-empty-modules', action='store_true',
                        help='show only Modules containing rules and their ancestors')
    args = parser.parse_args()
    output = args.output or args.graph.with_name('module-rule-graph.svg')
    if output.resolve() == args.graph.resolve() or output.with_suffix('.dot').resolve() == args.graph.resolve():
        parser.error('output must not overwrite the input graph')
    try:
        graph = json.loads(args.graph.read_text(encoding='utf-8'))
        rendered, companion = render(graph, output, args.dot, args.rankdir, args.hide_empty_modules)
    except (OSError, ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + '\n')
    print(rendered)
    if companion != rendered:
        print(companion)


if __name__ == '__main__':
    main()

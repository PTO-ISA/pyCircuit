"""Render the MLIR compiler's transaction schedule report with Graphviz."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess


def dot(graph):
    lines = ['digraph Transactions {', '  rankdir=LR;', '  node [shape=box];']
    for node in graph['nodes']:
        lines.append(f"  t{node['txn']} [label={json.dumps(node['id'])}];")
    seen = set()
    for edge in graph['edges']:
        key = edge['from'], edge['to'], edge['resource'], edge['relation']
        if key in seen:
            continue
        seen.add(key)
        label = edge['resource'] + '\n' + edge['relation']
        lines.append(f"  t{edge['from']} -> t{edge['to']} [label={json.dumps(label)}];")
    lines.append('}')
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('graph', type=Path)
    parser.add_argument('-o', '--output', type=Path)
    args = parser.parse_args()
    output = args.output or args.graph.with_suffix('.svg')
    source = dot(json.loads(args.graph.read_text()))
    if output.suffix == '.dot':
        output.write_text(source)
        return
    tool = os.environ.get('ACPY_GRAPHVIZ_DOT') or shutil.which('dot')
    if not tool:
        parser.error('Graphviz dot not found; select a .dot output')
    subprocess.run([tool, '-T' + output.suffix[1:], '-o', str(output)], input=source, text=True, check=True)


if __name__ == '__main__':
    main()

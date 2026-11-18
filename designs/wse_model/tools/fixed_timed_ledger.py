"""Materialize the 40-node timed profile before pyCircuit source-unit compilation."""

import ast
from pathlib import Path

# A receive identity consists of a 6-bit source and a 16-bit sequence number.
LAYOUT = {
    "NODES": 40,
    "EARLY_SLOTS": 2,
    "SEEN_SLOTS": 56,
    "TOTAL_EARLY": 80,
    "TOTAL_SEEN": 2240,
    "ROW_BITS": 1232,
}


def materialize(source: Path, destination: Path) -> None:
    tree = ast.parse(source.read_text())
    if source.stem == "ledger":
        module = next(node for node in tree.body if getattr(node, "name", "") == "ReceiveLedger")
        if {arg.arg for arg in module.args.kwonlyargs} != LAYOUT.keys():
            raise ValueError("ReceiveLedger layout does not match the fixed timed profile")
        module.args.kwonlyargs.clear()
        module.args.kw_defaults.clear()

        class FixedLayout(ast.NodeTransformer):
            def visit_Name(self, node):
                if node.id in LAYOUT and isinstance(node.ctx, ast.Load):
                    return ast.copy_location(ast.Constant(LAYOUT[node.id]), node)
                return node

        FixedLayout().visit(module)
    else:
        calls = [
            n
            for n in ast.walk(tree)
            if isinstance(n, ast.Call)
            and isinstance(n.func, ast.Name)
            and n.func.id == "ReceiveLedger"
        ]
        if (
            len(calls) != 1
            or calls[0].args
            or {kw.arg: ast.literal_eval(kw.value) for kw in calls[0].keywords} != LAYOUT
        ):
            raise ValueError(f"{source}: expected one ReceiveLedger with the fixed timed layout")
        calls[0].keywords.clear()
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(ast.unparse(tree) + "\n")

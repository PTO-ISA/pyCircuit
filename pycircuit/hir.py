"""Internal typed control flow; source AST nodes never enter this representation.

The builder resolves types and constructs shared predicates while visiting each
block. Freezing linearizes the blocks without enumerating paths. Every value
operation already carries its SSA type and guard; merge values are explicit
selects in the containing block. This keeps the small frontend a single pass.
"""
from dataclasses import dataclass, field


@dataclass
class Operation:
    opcode: str
    result: str
    type: str
    operands: list[str]
    guard: str | None
    location: dict
    attributes: dict

    def freeze(self):
        return dict(op=self.opcode, id=self.result, type=self.type, args=self.operands,
                    guard=self.guard, loc=self.location, **self.attributes)


@dataclass
class Block:
    nodes: list = field(default_factory=list)

    def freeze(self):
        result = []
        for node in self.nodes:
            if isinstance(node, Branch):
                result.extend(node.yes.freeze())
                result.extend(node.no.freeze())
            else:
                result.append(node.freeze())
        return result


@dataclass
class Branch:
    condition: str
    location: dict
    yes: Block = field(default_factory=Block)
    no: Block = field(default_factory=Block)

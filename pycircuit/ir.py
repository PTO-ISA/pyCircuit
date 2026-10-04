"""Small, serializable typed data-flow IR. There are no Python AST nodes here."""
from dataclasses import dataclass, field
import json
from pathlib import Path

VERSION = 1
SCALARS = {'bool', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64'}


def wrapped(kind, element):
    return f'{kind}<{element}>'


def element(typ):
    inside = typ[typ.index('<') + 1:-1]
    return inside.rsplit(',', 1)[0] if typ.startswith('array<') else inside


def resource(typ):
    return typ.startswith(('queue<', 'qarray<', 'signal<'))


@dataclass
class Value:
    id: str
    type: str
    targets: set = field(default_factory=set)
    consume: bool = False


class CompileError(ValueError):
    pass


def save(model, path):
    Path(path).write_text(json.dumps(model, indent=2, ensure_ascii=False) + '\n')


def load(path):
    model = json.loads(Path(path).read_text())
    if model.get('version') != VERSION:
        raise CompileError('unsupported ACIR version')
    return model

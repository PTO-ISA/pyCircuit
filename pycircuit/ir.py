"""Frontend value identities and atomic MLIR artifact I/O."""
from dataclasses import dataclass, field
from pathlib import Path

VERSION = 6
SCALARS = {'bool', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64'}


def wrapped(kind, element):
    return f'{kind}<{element}>'


def element(typ):
    inside = typ[typ.index('<') + 1:-1]
    return inside.rsplit(',', 1)[0] if typ.startswith('array<') else inside


def resource(typ):
    return isinstance(typ, str) and typ.startswith(('queue<', 'cell<', 'signal<', 'qarray<'))


@dataclass
class Instance:
    name: str
    env: dict
    methods: dict
    outputs: object = None
    definitions: dict = field(default_factory=dict)
    parent: str | None = None
    source_name: str = ''
    inputs: dict = field(default_factory=dict)


@dataclass
class Method:
    instance: Instance
    definition: object


@dataclass
class Value:
    id: str
    type: str
    targets: set = field(default_factory=set)
    consume: bool = False


class CompileError(ValueError):
    pass


def save(model, path):
    from .mlir_text import serialize
    import os
    import tempfile
    path = Path(path)
    with tempfile.NamedTemporaryFile(mode='w', dir=path.parent, delete=False) as stream:
        stream.write(model if isinstance(model, str) else serialize(model))
        temporary = stream.name
    os.replace(temporary, path)


def load(path):
    return Path(path).read_text()

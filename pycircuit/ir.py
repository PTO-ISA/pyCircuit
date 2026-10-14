"""Frontend value identities and atomic MLIR artifact I/O."""
from dataclasses import dataclass, field
from pathlib import Path

VERSION = 9
SCALARS = {'bool', 'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64'}


def wrapped(kind, element):
    return f'{kind}<{element}>'


class ArrayType(str):
    """Canonical frontend array descriptor; shape is never inferred from data."""
    def __new__(cls, item, shape):
        shape = tuple(shape)
        obj = str.__new__(cls, f'array<{item};' + 'x'.join(map(str, shape)) + '>')
        obj.item, obj.shape = item, shape
        return obj


class MaskType(str):
    def __new__(cls, shape):
        obj = str.__new__(cls, 'mask<' + 'x'.join(map(str, shape)) + '>')
        obj.shape = tuple(shape)
        return obj


def descriptor(typ):
    if not isinstance(typ, str): return typ
    if typ.startswith('array<'):
        item, shape = typ[6:-1].rsplit(';', 1)
        return ArrayType(descriptor(item), tuple(map(int, shape.split('x'))))
    if typ.startswith('mask<'):
        return MaskType(tuple(map(int, typ[5:-1].split('x'))))
    return typ


def tuple_items(typ):
    inside = typ[6:-1]
    if not inside: return []
    values, begin, depth = [], 0, 0
    for i, char in enumerate(inside):
        depth += (char == '<') - (char == '>')
        if char == ',' and depth == 0:
            values.append(descriptor(inside[begin:i])); begin = i + 1
    values.append(descriptor(inside[begin:]))
    return values


def element(typ):
    typ = descriptor(typ)
    if isinstance(typ, ArrayType): return typ.item
    return descriptor(typ[typ.index('<') + 1:-1])


def array_type(typ):
    if isinstance(typ, str) and typ.startswith('selected<'): typ = element(typ)
    typ = descriptor(typ)
    return typ if isinstance(typ, (ArrayType, MaskType)) else None


def resource(typ):
    return isinstance(typ, str) and typ.startswith(('queue<', 'cell<', 'ehr<', 'signal<'))



@dataclass
class Instance:
    name: str
    env: dict
    rules: dict
    definitions: dict = field(default_factory=dict)
    parent: str | None = None
    source_name: str = ''


@dataclass
class RuleRef:
    instance: Instance
    definition: object


@dataclass
class ModuleArray:
    type: ArrayType
    # One candidate list per logical coordinate. None means unconditional.
    slots: list


@dataclass
class ModuleChoice:
    candidates: list


@dataclass
class ChoiceRule:
    choice: ModuleChoice
    name: str


@dataclass
class Value:
    id: str
    type: str
    targets: set = field(default_factory=set)


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

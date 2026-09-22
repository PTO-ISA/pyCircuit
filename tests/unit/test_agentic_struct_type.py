"""Regression coverage for the runtime ``@ac.struct`` payload type.

The compiler resolves payload layouts from source, so these tests exercise only
the ordinary Python authoring surface: nominal identity, immutable construction,
captured metadata, eager annotation families, and file-path loading.
"""

import dataclasses
import importlib.util
import sys
from dataclasses import FrozenInstanceError
from enum import Enum
from pathlib import Path
from types import ModuleType

import agentic_circuit as ac
import pytest

pytestmark = pytest.mark.unit

REPOSITORY = Path(__file__).resolve().parents[2]


@ac.struct
class Inner:
    value: ac.u8
    flag: bool


@ac.struct
class Outer:
    items: ac.array[3, Inner]
    pair: tuple[ac.u8, ac.u8]


class Mode(Enum):
    IDLE = 0
    RUN = 1


EAGER_MODULE = """\
import agentic_circuit as ac


@ac.struct
class Leaf:
    value: ac.u8


@ac.struct
class Branch:
    leaves: ac.array[2, Leaf]
    pair: tuple[ac.u8, ac.u8]


@ac.struct
class Recursive:
    branches: ac.array[2, Branch]
"""


def test_struct_is_a_real_nominal_python_type() -> None:
    instance = Inner(value=1, flag=True)

    assert type(Inner) is type
    assert Inner.__name__ == "Inner"
    assert isinstance(instance, Inner)
    assert not isinstance(object(), Inner)
    assert type(instance).__name__ == "Inner"


def test_struct_is_immutable_keyword_constructed_and_hashable() -> None:
    instance = Inner(value=1, flag=True)

    assert instance == Inner(value=1, flag=True)
    assert len({instance, Inner(value=1, flag=True)}) == 1
    assert dataclasses.is_dataclass(Inner)
    assert [field.name for field in dataclasses.fields(Inner)] == ["value", "flag"]
    with pytest.raises(FrozenInstanceError):
        instance.value = 2
    with pytest.raises(TypeError):
        Inner(1, True)
    with pytest.raises(TypeError):
        Inner()
    assert instance.with_fields(value=9) == Inner(value=9, flag=True)


def test_struct_exposes_captured_metadata_and_descriptor() -> None:
    assert Inner.__ac_struct__ is True
    assert [name for name, _ in Inner.__ac_fields__] == ["value", "flag"]
    assert Inner.__ac_definition__.kind == "struct"
    assert Inner.__ac_definition__.qualified_name.endswith("Inner")
    assert Inner.descriptor.name == "Inner"
    assert Inner.descriptor.bit_width() == 9


def test_struct_project_selects_the_target_nominal_record() -> None:
    @ac.struct
    class View:
        value: ac.u8

    projected = Inner(value=3, flag=False).project(View)

    assert projected == View(value=3)
    assert isinstance(projected, View)
    with pytest.raises(TypeError, match="record project target"):
        Inner(value=1, flag=True).project(int)


def test_eager_array_annotations_accept_struct_bool_tuple_and_enum() -> None:
    from _pycircuit_semantics import BitsType, BoolType, EnumType, TupleType

    struct_element = ac.array[3, Inner]
    assert struct_element.element == Inner.descriptor
    assert ac.array[3, bool].element == BoolType()
    assert ac.array[3, tuple[ac.u8, ac.u8]].element == TupleType(
        (BitsType(8), BitsType(8))
    )
    assert ac.array[5, Mode].element == EnumType("Mode", ("IDLE", "RUN"))
    assert Outer.__ac_fields__[0][1].element == Inner.descriptor
    assert ac.array[2, Outer.__ac_fields__[1][1]].element == TupleType(
        (BitsType(8), BitsType(8))
    )


def _load_eager_module(name: str, path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_file_path_loading_without_sys_modules_registration(tmp_path: Path) -> None:
    path = tmp_path / "eager_architecture.py"
    path.write_text(EAGER_MODULE, encoding="utf-8")

    # Deliberately leave the module out of sys.modules: provenance capture must
    # fail soft instead of raising "is a built-in class".
    sys.modules.pop("eager_architecture_probe", None)
    module = _load_eager_module("eager_architecture_probe", path)

    assert isinstance(module.Leaf(value=4), module.Leaf)
    assert module.Branch.__ac_fields__[0][1].element == module.Leaf.descriptor
    assert module.Recursive.__ac_fields__[0][1].element == module.Branch.descriptor


def test_file_path_loading_with_sys_modules_registration(tmp_path: Path) -> None:
    path = tmp_path / "eager_architecture_registered.py"
    path.write_text(EAGER_MODULE, encoding="utf-8")

    spec = importlib.util.spec_from_file_location("eager_architecture_registered", path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    try:
        spec.loader.exec_module(module)
    finally:
        sys.modules.pop(spec.name, None)

    leaves = (module.Leaf(value=1),) * 2
    assert isinstance(module.Branch(leaves=leaves, pair=(1, 2)), module.Branch)


def test_maintained_example_imports_as_ordinary_python() -> None:
    path = (
        REPOSITORY
        / "examples"
        / "agentic-circuit"
        / "blocks"
        / "bounded_integer_operations.py"
    )

    spec = importlib.util.spec_from_file_location(
        "bounded_integer_operations_probe", path
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    try:
        spec.loader.exec_module(module)
    finally:
        sys.modules.pop(spec.name, None)

    inner = module.ArrayInner(tag=1, mode=module.ArrayMode.IDLE, ordinal=0)
    assert isinstance(inner, module.ArrayInner)

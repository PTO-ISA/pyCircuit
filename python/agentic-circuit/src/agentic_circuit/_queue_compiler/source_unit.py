"""Finite-family helpers for synthetic source-unit roots."""

from __future__ import annotations

import ast
import copy
from collections.abc import Mapping

from .syntax import _decorator_name


def first_family_case(
    declarations: Mapping[str, ast.FunctionDef], module_name: str
) -> ast.Call | None:
    declaration = declarations.get(module_name)
    if declaration is None:
        return None
    decorator = next(
        decorator
        for decorator in declaration.decorator_list
        if _decorator_name(decorator).rsplit(".", 1)[-1] == "module_decl"
    )
    if not isinstance(decorator, ast.Call):
        return None
    finite_cases = next(
        (
            keyword.value
            for keyword in decorator.keywords
            if keyword.arg == "finite_cases"
        ),
        None,
    )
    if not isinstance(finite_cases, ast.Tuple) or not finite_cases.elts:
        return None
    candidate = finite_cases.elts[0]
    return copy.deepcopy(candidate) if isinstance(candidate, ast.Call) else None


def case_literal_bindings(family_case: ast.Call | None) -> dict[str, ast.expr]:
    if family_case is None:
        return {}
    bindings: dict[str, ast.expr] = {}
    for argument in family_case.args:
        if (
            isinstance(argument, ast.Tuple)
            and len(argument.elts) == 2
            and isinstance(argument.elts[0], ast.Constant)
            and type(argument.elts[0].value) is str
        ):
            bindings[argument.elts[0].value] = copy.deepcopy(argument.elts[1])
    return bindings


def specialize_annotation(
    annotation: ast.expr | None, bindings: Mapping[str, ast.expr]
) -> ast.expr | None:
    if annotation is None or not bindings:
        return copy.deepcopy(annotation)

    class Substitute(ast.NodeTransformer):
        def visit_Name(self, node: ast.Name) -> ast.expr:
            replacement = bindings.get(node.id)
            return copy.deepcopy(replacement) if replacement is not None else node

    return ast.fix_missing_locations(Substitute().visit(copy.deepcopy(annotation)))

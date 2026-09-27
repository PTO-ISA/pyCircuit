"""Serialize one captured Python source file into private MLIR transport."""

from __future__ import annotations

import ast

from ._source_capture import _CapturedSourceFile, _CapturedSourceSpan


def _mlir_string(value: str) -> str:
    """Quote UTF-8 bytes using MLIR's two-digit byte escape syntax."""

    encoded = value.encode("utf-8", errors="surrogatepass")
    parts = ['"']
    for byte in encoded:
        if 0x20 <= byte <= 0x7E and byte not in (0x22, 0x5C):
            parts.append(chr(byte))
        else:
            parts.append(f"\\{byte:02X}")
    parts.append('"')
    return "".join(parts)


def _integer_decimal(value: int) -> str:
    """Return canonical decimal text without Python's global digit limit."""

    if value == 0:
        return "0"

    sign = "-" if value < 0 else ""
    remaining = -value if value < 0 else value
    chunks: list[int] = []
    while remaining:
        remaining, chunk = divmod(remaining, 1_000_000_000)
        chunks.append(chunk)
    remainder = "".join(f"{chunk:09d}" for chunk in reversed(chunks[:-1]))
    return sign + str(chunks[-1]) + remainder


def _file_span(captured: _CapturedSourceFile) -> _CapturedSourceSpan:
    lines = captured.source.split("\n")
    last = lines[-1]
    return _CapturedSourceSpan(
        path=captured.path,
        line=1,
        byte_column=1,
        codepoint_column=1,
        end_line=len(lines),
        end_byte_column=len(last.encode("utf-8", errors="surrogatepass")) + 1,
        end_codepoint_column=len(last) + 1,
    )


def _span(
    captured: _CapturedSourceFile,
    node: ast.AST,
    inherited: _CapturedSourceSpan,
) -> _CapturedSourceSpan:
    position_attributes = {"lineno", "col_offset", "end_lineno", "end_col_offset"}
    if not position_attributes.intersection(getattr(type(node), "_attributes", ())):
        return inherited
    return captured.span_for(node)


def _span_attr(span: _CapturedSourceSpan) -> str:
    return (
        "{"
        + ", ".join(
            (
                f"start_line = {span.line} : i64",
                f"start_byte_column = {span.byte_column} : i64",
                f"start_codepoint_column = {span.codepoint_column} : i64",
                f"end_line = {span.end_line} : i64",
                f"end_byte_column = {span.end_byte_column} : i64",
                f"end_codepoint_column = {span.end_codepoint_column} : i64",
            )
        )
        + "}"
    )


def _value(
    captured: _CapturedSourceFile,
    value: object,
    inherited: _CapturedSourceSpan,
) -> str:
    if isinstance(value, ast.AST):
        return _node(captured, value, inherited)
    if value is None:
        return "unit"
    if value is Ellipsis:
        return '["ellipsis"]'
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int):
        return "{integer = " + _mlir_string(_integer_decimal(value)) + "}"
    if isinstance(value, float):
        return '["float", ' + _mlir_string(value.hex()) + "]"
    if isinstance(value, complex):
        return (
            '["complex", '
            + _mlir_string(value.real.hex())
            + ", "
            + _mlir_string(value.imag.hex())
            + "]"
        )
    if isinstance(value, bytes):
        return '["bytes", ' + _mlir_string(value.hex()) + "]"
    if isinstance(value, str):
        return _mlir_string(value)
    if isinstance(value, (list, tuple)):
        return (
            "[" + ", ".join(_value(captured, item, inherited) for item in value) + "]"
        )
    raise TypeError(f"cannot serialize Python AST field {type(value).__name__}")


def _node(
    captured: _CapturedSourceFile,
    node: ast.AST,
    inherited: _CapturedSourceSpan,
) -> str:
    span = _span(captured, node, inherited)
    fields = ", ".join(
        f"{name} = {_value(captured, value, span)}"
        for name, value in ast.iter_fields(node)
    )
    return (
        "{kind = "
        + _mlir_string(type(node).__name__)
        + ", fields = {"
        + fields
        + "}, span = "
        + _span_attr(span)
        + "}"
    )


def _emit_source_transport(captured: _CapturedSourceFile) -> str:
    """Emit one private ``ac.python_capture`` record for a captured source."""

    relative_path = captured.path.relative_to(captured.source_root).as_posix()
    body = _node(captured, captured.syntax, _file_span(captured))
    record = "{path = " + _mlir_string(relative_path) + ", body = [" + body + "]}"
    return "module attributes {ac.python_capture = [" + record + "]} {\n}\n"

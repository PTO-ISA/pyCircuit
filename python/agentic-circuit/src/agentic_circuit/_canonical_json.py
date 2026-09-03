"""Shared JSON value types and content digest spelling."""

from __future__ import annotations

import hashlib
import json
import math
from typing import TypeAlias


JsonScalar: TypeAlias = None | bool | int | float | str
JsonValue: TypeAlias = JsonScalar | list["JsonValue"] | dict[str, "JsonValue"]


def sha256_bytes(data: bytes) -> str:
    """Return a SHA-256 digest using the repository's public spelling."""

    return "sha256:" + hashlib.sha256(data).hexdigest()


def _validate_string(value: str) -> None:
    try:
        value.encode("utf-8", errors="strict")
    except UnicodeEncodeError as error:
        raise ValueError("JSON strings must contain Unicode scalar values") from error


def validate_ijson_value(value: JsonValue) -> None:
    if value is None or type(value) is bool or type(value) is str:
        if isinstance(value, str):
            _validate_string(value)
        return
    if type(value) is int:
        if not -((1 << 53) - 1) <= value <= (1 << 53) - 1:
            raise ValueError("JSON integer is outside the portable I-JSON range")
        return
    if type(value) is float:
        if not math.isfinite(value):
            raise ValueError("JSON number must be finite binary64")
        if value == 0.0 and math.copysign(1.0, value) < 0:
            raise ValueError("negative zero cannot be canonicalized")
        return
    if type(value) is list:
        for item in value:
            validate_ijson_value(item)
        return
    if type(value) is dict:
        for key, item in value.items():
            if type(key) is not str:
                raise ValueError("JSON object names must be strings")
            _validate_string(key)
            validate_ijson_value(item)
        return
    raise ValueError(f"unsupported JSON value {type(value).__name__}")


def utf16_sort_key(value: str) -> bytes:
    _validate_string(value)
    return value.encode("utf-16-be")


def _encode_string(value: str) -> str:
    _validate_string(value)
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def _encode_float(value: float) -> str:
    if not math.isfinite(value):
        raise ValueError("JSON number must be finite binary64")
    if value == 0.0:
        if math.copysign(1.0, value) < 0:
            raise ValueError("negative zero cannot be canonicalized")
        return "0"

    shortest = repr(value).lower()
    negative = shortest.startswith("-")
    magnitude = shortest[1:] if negative else shortest
    if "e" in magnitude:
        mantissa, exponent_text = magnitude.split("e", 1)
        explicit_exponent = int(exponent_text)
    else:
        mantissa = magnitude
        explicit_exponent = 0
    decimal = mantissa.find(".")
    if decimal < 0:
        decimal = len(mantissa)
    digits = mantissa.replace(".", "")
    first_nonzero = next(
        (index for index, character in enumerate(digits) if character != "0"),
        None,
    )
    if first_nonzero is None:
        return "0"
    scientific_exponent = explicit_exponent + decimal - first_nonzero - 1
    digits = digits[first_nonzero:].rstrip("0") or "0"

    prefix = "-" if negative else ""
    if 0 <= scientific_exponent < 21:
        integer_digits = scientific_exponent + 1
        if len(digits) <= integer_digits:
            return prefix + digits + "0" * (integer_digits - len(digits))
        return prefix + digits[:integer_digits] + "." + digits[integer_digits:]
    if -6 <= scientific_exponent < 0:
        return prefix + "0." + "0" * (-scientific_exponent - 1) + digits
    mantissa_text = digits[0]
    if len(digits) > 1:
        mantissa_text += "." + digits[1:]
    exponent_sign = "+" if scientific_exponent >= 0 else ""
    return prefix + mantissa_text + "e" + exponent_sign + str(scientific_exponent)


def _encode_value(value: JsonValue) -> str:
    if value is None:
        return "null"
    if type(value) is bool:
        return "true" if value else "false"
    if type(value) is str:
        return _encode_string(value)
    if type(value) is int:
        return str(value)
    if type(value) is float:
        return _encode_float(value)
    if type(value) is list:
        return "[" + ",".join(_encode_value(item) for item in value) + "]"
    if type(value) is dict:
        properties = sorted(value.items(), key=lambda item: utf16_sort_key(item[0]))
        return (
            "{"
            + ",".join(
                _encode_string(key) + ":" + _encode_value(item)
                for key, item in properties
            )
            + "}"
        )
    raise ValueError(f"unsupported JSON value {type(value).__name__}")


def canonical_json_bytes(value: JsonValue) -> bytes:
    validate_ijson_value(value)
    return _encode_value(value).encode("utf-8")

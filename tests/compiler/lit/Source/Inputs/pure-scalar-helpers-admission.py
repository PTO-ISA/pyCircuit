"""Public admission boundaries for fixed scalar expression helpers."""

PRELUDE = "from pycircuit import module, rule, struct, u1, u2, u8, bits, table\nfrom typing import Annotated\n"


def design(helper, expression="helper(value)", extra="", value_type="u8"):
    return (
        PRELUDE
        + extra
        + helper
        + f"""\n@struct
class Result:
    value: u8
@module
def Top(value: {value_type}) -> Result:
    return Result(value={expression})
"""
    )


def admission_cases():
    cases = []

    def add(name, text, categories):
        cases.append({"name": name, "text": text, "categories": categories})

    identity = "@rule\ndef helper(value: u8) -> u8:\n    return value\n"
    for name, expression, categories in (
        ("integer_actual", "helper(0)", ("kind", "fixed", "integer", "formal")),
        (
            "keyword_actual",
            "helper(value=value)",
            ("keyword", "positional", "signature"),
        ),
        ("missing_actual", "helper()", ("arity", "argument", "formal")),
        ("extra_actual", "helper(value, value)", ("arity", "argument", "formal")),
        ("unknown_actual_first", "helper(left_missing)", ("left_missing",)),
    ):
        add(name, design(identity, expression), categories)
    pair = "@rule\ndef helper(left: u8, right: u8) -> u8:\n    return left ^ right\n"
    add(
        "left_to_right_actuals",
        design(pair, "helper(left_missing, right_missing)"),
        ("left_missing",),
    )
    # Each actual is evaluated and bound before the next actual. The first
    # source-kind mismatch must precede a later actual name error.
    add(
        "binding_before_later_actual",
        design(pair, "helper(0, right_missing)"),
        ("kind", "integer", "fixed", "formal"),
    )
    add(
        "width_authority",
        design(identity, "helper(value)", value_type="u2"),
        ("width", "type", "formal"),
    )
    add(
        "count_width_authority",
        design(identity, "helper(value.count())", value_type="table[3, u1]"),
        ("width", "type", "formal"),
    )
    add(
        "boolean_actual",
        design(identity, "helper(True)"),
        ("kind", "boolean", "fixed", "formal"),
    )
    add(
        "integer_source_actual",
        design(identity, value_type="Annotated[int, range(256)]"),
        ("kind", "integer", "fixed", "formal"),
    )
    add(
        "boolean_source_actual",
        design(identity, value_type="bool"),
        ("kind", "boolean", "fixed", "formal"),
    )
    for name, signature in (
        ("unannotated_formal", "value"),
        ("logical_formal", "value: Annotated[int, range(256)]"),
        ("boolean_formal", "value: bool"),
        ("default_formal", "value: u8 = 1"),
        ("positional_only", "value: u8, /"),
        ("keyword_only", "*, value: u8"),
        ("variadic", "*value: u8"),
        ("duplicate_formal", "value: u8, value: u8"),
        ("reserved_formal", "pyc_clk: u8"),
    ):
        helper = f"@rule\ndef helper({signature}) -> u8:\n    return value\n"
        text = design(
            helper,
            "helper(value, value)" if name == "duplicate_formal" else "helper(value)",
        )
        add(
            name,
            text,
            (
                ("duplicate rule parameter",)
                if name == "duplicate_formal"
                else (
                    "arity",
                    "signature",
                    "formal",
                    "parameter",
                    "annotat",
                    "duplicate",
                    "reserved",
                    "collision",
                )
            ),
        )
    for name, result in (
        ("logical_result", "Annotated[int, range(256)]"),
        ("boolean_result", "bool"),
        ("computed_width", "bits[4 + 4]"),
        ("shift_width", "bits[1 << 3]"),
    ):
        add(
            name,
            design(identity.replace("-> u8", "-> " + result)),
            ("result", "signature", "fixed", "width", "annotat"),
        )
    add(
        "shadowed_width_alias",
        design(identity, extra="u8 = 8\n"),
        ("shadow", "fixed", "annotat", "signature", "type"),
    )
    add(
        "nominal_formal",
        design(
            identity.replace("value: u8", "value: Tag"),
            extra="@struct\nclass Tag:\n    value: u8\n",
        ),
        ("fixed", "signature", "annotat", "formal"),
    )
    add(
        "nominal_actual",
        design(
            identity, value_type="Tag", extra="@struct\nclass Tag:\n    value: u8\n"
        ),
        ("kind", "type", "formal", "fixed"),
    )
    add(
        "declaration_formal_collision",
        design(identity.replace("value: u8", "Result: u8")),
        ("collision", "formal", "declaration", "binding"),
    )
    for name, body in (
        ("assignment", "temporary = value\n    return temporary"),
        ("multiple_return", "return value\n    return value"),
        ("conditional_statement", "if True:\n        return value\n    return value"),
        ("slice", "return value[:4]"),
        ("division", "return value // 2"),
        ("shift", "return value << 1"),
        ("table_method", "return value.count()"),
        ("intrinsic", "return bits(value)"),
        ("literal_tree", "return 1 + 2"),
        ("literal_choice", "return 1 if value else 2"),
        ("predicate_arithmetic", "return (value == value) + value"),
        ("caller_capture", "return value + captured"),
        ("effect_dead_arm", "return value if True else print(value)"),
        ("import_statement", "import math\n    return value"),
        ("assert_statement", "assert value\n    return value"),
        ("nested_definition", "def hidden():\n        return value\n    return value"),
    ):
        helper = "@rule\ndef helper(value: u8) -> u8:\n    " + body + "\n"
        text = design(helper)
        if name == "caller_capture":
            text = text.replace(
                "    return Result(value=helper(value))",
                "    captured = value\n    return Result(value=helper(value))",
            )
        add(
            name,
            text,
            (
                "pure",
                "scalar",
                "expression",
                "helper",
                "unsupported",
                "literal",
                "statement",
                "captur",
            ),
        )
    add(
        "direct_body_recursion",
        design(identity.replace("return value", "return helper(value)")),
        ("recurs", "cycle", "acyclic"),
    )
    indirect = (
        identity.replace("return value", "return other(value)")
        + "@rule\ndef other(value: u8) -> u8:\n    return helper(value)\n"
    )
    add("indirect_body_recursion", design(indirect), ("recurs", "cycle", "acyclic"))
    add(
        "dead_arm_recursion",
        design(
            identity.replace("return value", "return value if True else helper(value)")
        ),
        ("recurs", "cycle", "acyclic"),
    )
    add(
        "wide_result_literal",
        design(identity.replace("return value", "return 256")),
        ("fit", "width", "range", "literal", "bound"),
    )
    add(
        "wide_right_choice",
        design(
            identity.replace("return value", "return value if value == value else 256")
        ),
        ("fit", "width", "range", "literal", "bound"),
    )
    add(
        "wide_left_choice",
        design(
            identity.replace("return value", "return 256 if value == value else value")
        ),
        ("fit", "width", "range", "literal", "bound"),
    )
    add(
        "caller_shadowed_helper",
        design(identity).replace(
            "    return Result(value=helper(value))",
            "    helper = value\n    return Result(value=helper(value))",
        ),
        ("shadow", "call", "helper"),
    )
    add(
        "query_callback_helper",
        design(
            identity, "value.map(lambda row: helper(row))[0]", value_type="table[3, u8]"
        ),
        ("callback", "helper", "lambda", "expression", "call"),
    )
    # Limits are the published source-invocation ledger contracts, not inferred
    # operation recipes. These sources are small despite exceeding the limits.
    nesting_limit = 128
    add(
        "actual_depth_budget",
        design(
            identity,
            "helper(" * (nesting_limit + 1) + "value" + ")" * (nesting_limit + 1),
        ),
        ("budget", "nesting", "depth"),
    )
    chain = "".join(
        f"@rule\ndef level{i}(value: u8) -> u8:\n    return "
        + (f"level{i + 1}(value)" if i < nesting_limit else "value")
        + "\n"
        for i in range(nesting_limit + 1)
    )
    add(
        "body_depth_budget",
        design(chain, "level0(value)"),
        ("budget", "nesting", "depth"),
    )
    dag = identity.replace("helper", "level0")
    for i in range(1, 14):
        dag += f"@rule\ndef level{i}(value: u8) -> u8:\n    return level{i - 1}(value) ^ level{i - 1}(value)\n"
    add(
        "expanded_body_budget",
        design(dag, "level13(value)"),
        ("budget", "occurrence", "work", "storage"),
    )
    max_width = 64 * 1048576
    add(
        "width_before_allocation",
        design(identity.replace("u8", f"bits[{max_width + 1}]")),
        ("budget", "width", "storage"),
    )
    # The capture parser accepts this decimal spelling. Its repeated expanded
    # payload cannot fit the published constant-storage cap, so preflight must
    # reject before parsing a wide native literal or checking the u8 result fit.
    magnitude = "9" * 4000
    large_literal = (
        "@rule\ndef wide_literal(value: u8) -> u8:\n    return value + "
        + magnitude
        + "\n"
    )
    large_literal += "@rule\ndef helper(value: u8) -> u8:\n    return wide_literal(value) ^ wide_literal(value)\n"
    add(
        "literal_storage_before_allocation",
        design(large_literal),
        ("budget", "envelope", "storage"),
    )
    return cases


def positive_controls():
    identity = "@rule\ndef helper(value: u8) -> u8:\n    return value\n"
    # A moderate finite chain exercises lifecycle balance without assuming the
    # implementation-dependent number of active preflight guards per call.
    depth = 32
    return [
        {
            "name": "finite_actual_depth",
            "text": design(identity, "helper(" * depth + "value" + ")" * depth),
        }
    ]

"""Source-only cases for ordered ordinary data and direct nested Rule bindings.

These are compiler inputs, not a second runner or an execution oracle.
"""

PREFIX = """import pycircuit as ac
from enum import Enum
@ac.encoding(width=8)
class Tag(Enum):
    ZERO = 0
    FIVE = 5
@ac.struct
class Packet:
    value: ac.u8
"""


def nominal(body, *, arguments="a: ac.u8", parameters=""):
    return PREFIX + f"@ac.module\ndef Top({arguments}{parameters}) -> Packet:\n" + body


def structural(body):
    return PREFIX + '@ac.module\ndef Top(a: ac.u8) -> {"result": ac.u8}:\n' + body


def reserved_binding_cases(data_before):
    # Each no-data counterpart was admitted by the installed 7bfe3263 baseline.
    # Reserved data spellings must stay protected without banning those Rules.
    cases = {}
    for profile in ("mapping", "nominal"):
        for name in (
            "bool",
            "int",
            "range",
            "Annotated",
            "module",
            "rule",
            "system",
            "log",
            "report",
        ):
            body = f"    {name} = a\n" if data_before else ""
            body += f"    @ac.rule\n    def {name}():\n"
            body += (
                "        assert True\n" if profile == "mapping" else "        pass\n"
            )
            body += f"    {name}()\n"
            body += (
                '    return {"result": a}\n'
                if profile == "mapping"
                else "    return Packet(value=a)\n"
            )
            key = (
                "reserved_"
                + ("data_before_" if data_before else "rule_only_")
                + profile
                + "_"
                + name
            )
            cases[key] = structural(body) if profile == "mapping" else nominal(body)
    return cases


def global_rule_collision_cases(data_before):
    cases = {}
    for profile in ("mapping", "nominal"):
        body = "    check = a\n" if data_before else ""
        body += "    @ac.rule\n    def check():\n"
        body += "        assert True\n" if profile == "mapping" else "        pass\n"
        body += "    check()\n"
        body += (
            '    return {"result": a}\n'
            if profile == "mapping"
            else "    return Packet(value=a)\n"
        )
        text = structural(body) if profile == "mapping" else nominal(body)
        text = text.replace(
            "@ac.module\ndef Top",
            "@ac.rule\ndef check():\n    pass\n@ac.module\ndef Top",
            1,
        )
        key = (
            "global_rule_"
            + ("data_conflict_" if data_before else "nested_only_")
            + profile
        )
        cases[key] = text
    return cases


def memory_rule_collision_cases():
    positives, negatives = {}, {}
    calls = {
        "sync_mem": "ac.sync_mem[ac.u8](ren, addr, False, addr, 0, 0, depth=2)",
        "byte_mem": "ac.byte_mem[ac.u8](addr, False, addr, 0, 0, depth=2)",
    }
    declaration = "    @ac.rule\n    def check():\n        pass\n    check()\n"
    for kind, call in calls.items():

        def design(body):
            return nominal(body, arguments="ren: ac.u1, addr: ac.u1")

        positives["memory_proper_" + kind] = design(
            "    raw_memory_result = "
            + call
            + "\n    return Packet(value=raw_memory_result)\n"
        )
        positives["memory_rule_without_data_" + kind] = design(
            "    raw_memory_result = "
            + call
            + "\n"
            + declaration
            + "    return Packet(value=raw_memory_result)\n"
        )
        positives["memory_ordinary_alias_" + kind] = design(
            "    raw_memory_result = "
            + call
            + "\n    check = raw_memory_result\n    saved = check\n"
            + declaration
            + "    return Packet(value=saved)\n"
        )
        # The only use of the colliding result is BEFORE declaration. A later
        # retired-name read cannot accidentally account for this rejection.
        negatives["memory_single_name_" + kind] = design(
            "    check = "
            + call
            + "\n    saved = check\n"
            + declaration
            + "    return Packet(value=saved)\n"
        )
    dp = "ac.sync_mem_dp[ac.u8](ren, addr, ren, addr, False, addr, 0, 0, depth=2)"
    positives["memory_dp_rule_without_data"] = nominal(
        "    first, second = "
        + dp
        + "\n"
        + declaration
        + "    return Packet(value=first ^ second)\n",
        arguments="ren: ac.u1, addr: ac.u1",
    )
    positives["memory_dp_ordinary_alias"] = nominal(
        "    first, second = "
        + dp
        + "\n    check = first\n    saved = check\n"
        + declaration
        + "    return Packet(value=saved ^ second)\n",
        arguments="ren: ac.u1, addr: ac.u1",
    )
    for slot, targets, saved in (
        ("first", "check, second", "check ^ second"),
        ("second", "first, check", "first ^ check"),
    ):
        negatives["memory_dp_name_" + slot] = nominal(
            "    "
            + targets
            + " = "
            + dp
            + "\n    saved = "
            + saved
            + "\n"
            + declaration
            + "    return Packet(value=saved)\n",
            arguments="ren: ac.u1, addr: ac.u1",
        )
    return positives, negatives


def positive_cases():
    cases = {
        "scalar_alias": structural("""    check = a
    saved = check
    @ac.rule
    def check():
        assert True
    check()
    return {"result": saved}
"""),
        "nominal_aliases": nominal("""    check = Packet(value=a)
    saved = check
    field = check.value
    @ac.rule
    def check():
        pass
    check()
    return Packet(value=saved.value ^ field)
"""),
        "table_alias": nominal(
            """    check = values
    saved = check
    field = check[1]
    @ac.rule
    def check():
        pass
    check()
    return Packet(value=saved[0] ^ field)
""",
            arguments="a: ac.u8, values: ac.table[2, ac.u8]",
        ),
        "behavioral_reassignment": nominal("""    check = a
    saved = check
    check = a ^ 1
    later = check
    @ac.rule
    def check():
        pass
    check()
    return Packet(value=saved ^ later)
"""),
        "helper_snapshot_before": nominal("""    check = a
    def read() -> ac.u8:
        return check
    check = a ^ 1
    @ac.rule
    def check():
        pass
    check()
    return Packet(value=read())
"""),
        "helper_formal_after": nominal("""    check = a
    @ac.rule
    def check():
        pass
    check()
    def read(check: ac.u8) -> ac.u8:
        return check
    return Packet(value=read(a))
"""),
        "helper_local_after": nominal("""    check = a
    @ac.rule
    def check():
        pass
    check()
    def read(value: ac.u8) -> ac.u8:
        check = value
        return check
    return Packet(value=read(a))
"""),
        "valued_return": nominal("""    check = Packet(value=a)
    saved = check
    @ac.rule
    def check() -> Packet:
        return saved
    return check()
"""),
        "valued_assignment": nominal("""    check = Packet(value=a)
    saved = check
    @ac.rule
    def check() -> Packet:
        return saved
    result = check()
    return result
"""),
        "early_structural_return": structural("""    check = a
    return {"result": check}
    @ac.rule
    def check():
        assert True
    check()
"""),
        "pure_fixed_result": nominal("""    check, member = ac.enum_from_bits[Tag](a)
    saved = check
    @ac.rule
    def check():
        pass
    check()
    return Packet(value=ac.enum_to_bits(saved))
"""),
    }
    # These three exact forward shapes use behavioral lowering and already pass.
    for annotation in ("", " -> None"):
        cases["forward_nominal" + ("_typed" if annotation else "")] = nominal(
            "    check()\n    @ac.rule\n    def check()"
            + annotation
            + ":\n        pass\n    return Packet(value=a)\n"
        )
    cases["forward_state_writer"] = """import pycircuit as ac
@ac.system
def Top():
    count: ac.u8 = 0
    advance(count)
    @ac.rule
    def advance(value):
        value = value + 1
"""
    # Mapping + assertion-only body is the structural declaration-first control.
    cases["structural_declared_first"] = structural("""    @ac.rule
    def check():
        assert a == a
    check()
    return {"result": a}
""")
    cases.update(reserved_binding_cases(False))
    cases.update(global_rule_collision_cases(False))
    cases.update(memory_rule_collision_cases()[0])
    return cases


def negative_cases():
    declaration = "    @ac.rule\n    def check():\n        pass\n"
    cases = {
        "data_call_before": nominal(
            "    check = a\n    check()\n"
            + declaration
            + "    check()\n    return Packet(value=a)\n"
        ),
        "forward_then_data": nominal(
            "    check()\n    check = a\n"
            + declaration
            + "    return Packet(value=a)\n"
        ),
        "missing_registration": nominal(
            "    check = a\n" + declaration + "    return Packet(value=a)\n"
        ),
        "duplicate_registration": nominal(
            "    check = a\n"
            + declaration
            + "    check()\n    check()\n    return Packet(value=a)\n"
        ),
        "structural_reassignment": structural(
            "    check = a\n    check = a ^ 1\n"
            + "    @ac.rule\n    def check():\n        assert True\n"
            + '    check()\n    return {"result": a}\n'
        ),
        "helper_capture_after": nominal(
            "    check = a\n"
            + declaration
            + "    check()\n    def read() -> ac.u8:\n        return check\n"
            + "    return Packet(value=read())\n"
        ),
        "nested_expression": nominal(
            "    check = a\n    @ac.rule\n    def check() -> ac.u8:\n        return a\n"
            + "    return Packet(value=check() + 1)\n"
        ),
        "callback": nominal(
            "    check = a\n    @ac.rule\n    def check(value):\n        return value\n"
            + "    mapped = values.map(check)\n    return Packet(value=mapped[0])\n",
            arguments="a: ac.u8, values: ac.table[2, ac.u8]",
        ),
        "loop_registration": nominal(
            "    check = a\n"
            + declaration
            + "    for index in range(1):\n        check()\n    return Packet(value=a)\n"
        ),
        "callable_registration": nominal(
            "    check = a\n"
            + declaration
            + "    def invoke() -> ac.u8:\n        check()\n        return a\n"
            + "    return Packet(value=invoke())\n"
        ),
        "formal_conflict": nominal(
            declaration + "    check()\n    return Packet(value=a)\n",
            arguments="a: ac.u8, check: ac.u8",
        ),
        "static_conflict": nominal(
            declaration + "    check()\n    return Packet(value=a)\n",
            parameters=", *, check: int = 1",
        ),
        "state_conflict": nominal(
            "    check: ac.u8 = 0\n"
            + declaration
            + "    check()\n    return Packet(value=a)\n"
        ),
        "new_local_after_return": structural(
            '    check = a\n    return {"result": check}\n'
            + declaration
            + "    late = a\n    check()\n"
        ),
        "typed_transaction_conflict": PREFIX
        + """@ac.module
def Owner():
    check = Packet(value=1)
    @ac.method
    def read() -> ac.u8:
        return 1
    @ac.rule
    def check() -> None:
        pass
    check()
""",
        "forward_structural": structural("""    check()
    @ac.rule
    def check():
        assert a == a
    return {"result": a}
"""),
    }
    for name, statement in {
        "copy": "saved = check",
        "field": "saved = check.value",
        "whole_write": "check = a",
        "field_write": "check.value = a",
        "annotation": "saved: ac.u8 = check.value",
    }.items():
        cases["after_declaration_" + name] = nominal(
            "    check = Packet(value=a)\n"
            + declaration
            + "    check()\n    "
            + statement
            + "\n    return Packet(value=a)\n"
        )
    cases["import_conflict"] = nominal(
        "    check = a\n" + declaration + "    check()\n    return Packet(value=a)\n"
    ).replace(
        "from enum import Enum",
        "from enum import Enum\nfrom pycircuit import u8 as check",
    )
    cases["source_helper_conflict"] = nominal(
        "    check = a\n" + declaration + "    check()\n    return Packet(value=a)\n"
    ).replace(
        "@ac.module",
        "def check(value: ac.u8) -> ac.u8:\n    return value\n@ac.module",
        1,
    )
    cases["late_namespace_shadow"] = nominal(
        "    check = a\n"
        + declaration
        + "    check()\n    ac = a\n    return Packet(value=a)\n"
    )
    cases["late_range_shadow"] = nominal("""    def read(value: ac.u8) -> ac.u8:
        for index in range(1):
            value = value + 1
        range = value
        return value
    return Packet(value=read(a))
""")
    instance_module = '@ac.module\ndef Child() -> {"q": ac.u8}:\n    return {"q": 0}\n'
    cases["structural_instance_conflict"] = structural(
        "    check = Child()\n    @ac.rule\n    def check():\n        assert True\n"
        + '    check()\n    return {"result": a}\n'
    ).replace("@ac.module\ndef Top", instance_module + "@ac.module\ndef Top", 1)
    cases["behavioral_instance_conflict"] = nominal(
        "    check = Child()\n"
        + declaration
        + "    check()\n    return Packet(value=a)\n"
    ).replace("@ac.module\ndef Top", instance_module + "@ac.module\ndef Top", 1)
    cases["late_type_shadow"] = nominal(
        "    check = a\n"
        + declaration
        + "    check()\n    Byte = a\n    return Packet(value=a)\n",
        arguments="a: Byte",
    ).replace(
        "from enum import Enum",
        "from enum import Enum\nfrom pycircuit import u8 as Byte",
    )
    cases["after_declaration_fixed_result"] = nominal(
        "    check = a\n"
        + declaration
        + "    check()\n"
        + "    check, member = ac.enum_from_bits[Tag](a)\n    return Packet(value=a)\n"
    )
    cases.update(reserved_binding_cases(True))
    cases.update(global_rule_collision_cases(True))
    cases.update(memory_rule_collision_cases()[1])
    return cases

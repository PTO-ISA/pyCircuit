"""Read-only hierarchy observers and literal-input source generation for the gate."""

import ast
import re

ENTRY_FIELDS = (
    ("valid", 1),
    ("age", 8),
    ("src0_tag", 8),
    ("src0_ready", 1),
    ("src1_tag", 8),
    ("src1_ready", 1),
)
WAKE_FIELDS = (("tag", 8), ("valid", 1))


def constructor(kind, value):
    if kind == "Event":
        return f"Event(value={int(value['value'])})"
    fields = ENTRY_FIELDS if kind == "Entry" else WAKE_FIELDS
    return kind + "(" + ", ".join(f"{key}={int(value[key])}" for key, _ in fields) + ")"


def scheduled_source(template, case):
    """Only independently authored input actions become source literals."""
    kind = case["kind"]
    target = kind + "_stimulus"
    tree = ast.parse(template)
    node = next(
        n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == target
    )
    if kind == "mailbox":
        initial = "MailboxFrame(left_valid=0, left_data=Event(value=0), left_take=0, right_valid=0, right_data=Event(value=0), right_take=0)"
    else:
        initial = "IssueFrame(allocation_valid=0, allocation_data=Entry(valid=0, age=0, src0_tag=0, src0_ready=0, src1_tag=0, src1_ready=0), wakeup_valid=0, wakeup_data=Wakeup(tag=0, valid=0), issued_take=0)"
    body = [
        f"@rule\ndef {target}(epoch: u16) -> {'MailboxFrame' if kind == 'mailbox' else 'IssueFrame'}:",
        f"    frame = {initial}",
    ]
    for row in case["rows"]:
        actions = row["actions"]
        assert not actions.get(
            "inject"
        ), "exceptional historical host injection is reference-only"
        lines = []
        for queue_name, values in actions.get("offer", {}).items():
            assert len(values) == 1, "one physical offer per input per epoch"
            name = (
                queue_name.removesuffix("_input")
                if kind == "mailbox"
                else {"allocations": "allocation", "wakeups": "wakeup"}[queue_name]
            )
            carrier = (
                "Event"
                if kind == "mailbox"
                else ("Entry" if queue_name == "allocations" else "Wakeup")
            )
            lines.extend(
                (
                    f"        frame.{name}_valid = 1",
                    f"        frame.{name}_data = {constructor(carrier, values[0])}",
                )
            )
        for queue_name in (*actions.get("take", []), *actions.get("sink", [])):
            name = queue_name.removesuffix("_output") if kind == "mailbox" else "issued"
            lines.append(f"        frame.{name}_take = 1")
        if lines:
            body.append(f"    if epoch == {row['epoch']}:")
            body.extend(lines)
    body.append("    return frame\n")
    lines = template.splitlines(keepends=True)
    start = min(node.lineno, *(d.lineno for d in node.decorator_list)) - 1
    return "".join(lines[:start]) + "\n".join(body) + "".join(lines[node.end_lineno :])


def family(header, name):
    section = header.split(f"class pyc_family_{name} final", 1)[1]
    return section.split(f"class {name} final", 1)[0]


def child(section, name):
    found = re.findall(
        rf"std::shared_ptr<[^;\n]*pyc_family_{name}<pyc_count>> (\w+);", section
    )
    assert len(found) == 1, (name, found)
    return found[0]


def queues(section):
    found = re.findall(
        r"std::shared_ptr<gfsim::collection_storage<gfsim::fifo_kernel<[^;\n]+>>> (\w+);",
        section,
    )
    assert len(found) in (3, 4), found
    return found


def cpp_config(system_header, provider_header, kind):
    """Locate actual emitted owners by their types; never supply expected data."""
    root = "dut.root_->pyc_implementation"
    section = family(
        system_header, "slot_rule_mailbox" if kind == "mailbox" else "resident_issue"
    )
    if kind == "mailbox":
        fifo = queues(section)
        left = root + "->" + child(section, "explicit_mailbox")
        right = root + "->" + child(section, "nested_mailbox")
        nested_section = family(provider_header, "nested_mailbox")
        right_valid = (
            right
            + "->"
            + child(nested_section, "valid_cell")
            + "->pyc_instance_q_state"
        )
        right_payload = (
            right
            + "->"
            + child(nested_section, "payload_cell")
            + "->pyc_instance_q_state"
        )
        image = []
        for path, incoming, outgoing in (
            (left, fifo[0], fifo[2]),
            (right, fifo[1], fifo[3]),
        ):
            image += [
                f"queue_bits(*{root}->{incoming}, 1)",
                f"bits({right_valid if path == right else path + '->pyc_instance_slot_valid_state'}->current(0).q)",
                f"bits({right_payload if path == right else path + '->pyc_instance_slot_payload_state'}->current(0).q)",
                f"queue_bits(*{root}->{outgoing}, 1)",
            ]
        control_wires = {}
        for side, path, incoming, outgoing in (
            ("left", left, fifo[0], fifo[2]),
            ("right", right, fifo[1], fifo[3]),
        ):
            inp, out = incoming.removesuffix("_state"), outgoing.removesuffix("_state")
            for port, base in (("in", inp), ("out", out)):
                for label, actual in (
                    ("ready", "in_ready"),
                    ("available", "out_valid"),
                    ("head", "out_data"),
                ):
                    control_wires[f"{side}_{port}_{label}"] = (
                        f"{root}->{base}_{actual}.element(0)"
                    )
            if side == "left":
                result = f"{path}->result.element(0).packed()"
                control_wires.update(
                    {
                        f"{side}_capture": f"gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::extract<1>({result}, 9))",
                        f"{side}_publish": f"gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::extract<1>({result}, 8))",
                        f"{side}_publish_data": f"gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::extract<8>({result}, 0))",
                    }
                )
            else:
                control_wires.update(
                    {
                        f"{side}_capture": f"{path}->incoming_take.element(0)",
                        f"{side}_publish": f"{path}->outgoing_valid.element(0)",
                        f"{side}_publish_data": f"{path}->outgoing_data.element(0)",
                    }
                )

    else:
        root += "->" + child(section, "issue")
        section = family(provider_header, "issue")
        fifo = queues(section)
        core = root + "->" + child(section, "issue_core")
        image = [
            f"queue_bits(*{root}->{fifo[1]}, 2)",
            f"queue_bits(*{root}->{fifo[0]}, 2)",
            f"bits({core}->pyc_instance_wakeup_valid_state->current(0).q)",
            f"bits({core}->pyc_instance_wakeup_payload_state->current(0).q)",
            f"bits({core}->pyc_instance_allocation_valid_state->current(0).q)",
            f"bits({core}->pyc_instance_allocation_payload_state->current(0).q)",
        ]
        image += [
            f"bits({core}->pyc_instance_entries_state->current({i}).q)"
            for i in range(4)
        ]
        image += [f"queue_bits(*{root}->{fifo[2]}, 1)"]
        names = (
            "allocation_ready",
            "allocation_available",
            "allocation_head",
            "wakeup_ready",
            "wakeup_available",
            "wakeup_head",
            "issued_ready",
            "issued_available",
            "issued_head",
            "allocation_capture",
            "wakeup_capture",
            "allocation_install",
            "wakeup_release",
            "selected",
            "publish",
            "publish_data",
        )
        control_wires = {name: f"{root}->{name}.element(0)" for name in names}
    controls = "\n".join(
        f'out += "{name}=" + std::to_string(unsigned_value({wire})) + ";";'
        for name, wire in control_wires.items()
    )
    return (
        "auto snapshot = [&]() { return "
        + " + ".join(image)
        + "; };\nauto controls = [&]() { std::string out; "
        + controls
        + " return out; };"
    )


def unpack(name, value, kind):
    fields = (
        (("value", 8),)
        if kind == "mailbox"
        else (WAKE_FIELDS if name.startswith("wakeup") else ENTRY_FIELDS)
    )
    result = {}
    for field, width in reversed(fields):
        result[field] = value & ((1 << width) - 1)
        value >>= width
    assert value == 0
    return result


def parse_controls(text, kind):
    result = {}
    for part in text.rstrip(";").split(";"):
        name, value = part.split("=")
        value = int(value)
        result[name] = (
            unpack(name, value, kind)
            if name.endswith(("_head", "_data"))
            else bool(value)
        )
    return result


def verilog_config(system_header, provider_header, kind):
    root = "dut.dut"
    section = family(
        system_header, "slot_rule_mailbox" if kind == "mailbox" else "resident_issue"
    )

    def queue_name(name):
        return "pyc_instance_" + name.removeprefix("pyc_queue_").removesuffix("_state")

    def queue_image(path, depth, width, count_width):
        values = [f"{count_width}'({path}.count)"]
        values += [
            f"{width}'(({path}.count > {i}) ? {path}.storage[({path}.rd + {i}) % {depth}] : 0)"
            for i in range(depth)
        ]
        return "{" + ", ".join(values) + "}"

    if kind == "mailbox":
        fifo = [queue_name(q) for q in queues(section)]
        left = root + "." + child(section, "explicit_mailbox")
        right = root + "." + child(section, "nested_mailbox")
        nested_section = family(provider_header, "nested_mailbox")
        right_valid = (
            right + "." + child(nested_section, "valid_cell") + ".pyc_instance_q"
        )
        right_payload = (
            right + "." + child(nested_section, "payload_cell") + ".pyc_instance_q"
        )
        image = []
        control_wires = {}
        for side, path, incoming, outgoing in (
            ("left", left, fifo[0], fifo[2]),
            ("right", right, fifo[1], fifo[3]),
        ):
            image += [
                queue_image(f"{root}.{incoming}", 1, 8, 1),
                f"{right_valid if side == 'right' else path + '.pyc_instance_slot_valid'}.q_current",
                f"{right_payload if side == 'right' else path + '.pyc_instance_slot_payload'}.q_current",
                queue_image(f"{root}.{outgoing}", 1, 8, 1),
            ]
            for port, queue in (("in", incoming), ("out", outgoing)):
                for label, actual in (
                    ("ready", "in_ready"),
                    ("available", "out_valid"),
                    ("head", "out_data"),
                ):
                    control_wires[f"{side}_{port}_{label}"] = f"{root}.{queue}.{actual}"
            value = path + (".result" if side == "left" else "")
            control_wires.update(
                {
                    f"{side}_capture": value + ".incoming_take",
                    f"{side}_publish": value + ".outgoing_valid",
                    f"{side}_publish_data": value + ".outgoing_data",
                }
            )
        width = 54
    else:
        root += "." + child(section, "issue")
        section = family(provider_header, "issue")
        fifo = [queue_name(q) for q in queues(section)]
        core = root + "." + child(section, "issue_core")
        image = [
            queue_image(root + "." + fifo[1], 2, 9, 2),
            queue_image(root + "." + fifo[0], 2, 27, 2),
        ]
        image += [
            f"{core}.pyc_instance_{name}.q_current"
            for name in (
                "wakeup_valid",
                "wakeup_payload",
                "allocation_valid",
                "allocation_payload",
            )
        ]
        image += [
            f"{core}.pyc_instances_entries[{i}].pyc_instance_entries.q_current"
            for i in range(4)
        ]
        image += [queue_image(root + "." + fifo[2], 1, 27, 1)]
        names = (
            "allocation_ready",
            "allocation_available",
            "allocation_head",
            "wakeup_ready",
            "wakeup_available",
            "wakeup_head",
            "issued_ready",
            "issued_available",
            "issued_head",
            "allocation_capture",
            "wakeup_capture",
            "allocation_install",
            "wakeup_release",
            "selected",
            "publish",
            "publish_data",
        )
        control_wires = {name: root + "." + name for name in names}
        width = 250
    values = ", ".join(control_wires.values())
    labels = "".join(name + "=%0d;" for name in control_wires)
    return f"function automatic logic [{width - 1}:0] snapshot(); return {{{', '.join(image)}}}; endfunction\nfunction automatic string controls(); return $sformatf(\"{labels}\", {values}); endfunction"

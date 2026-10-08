"""Independent historical slot and resident-only issue algorithms.

No DUT, compiler, generated code or retired frontend is imported. Offers commit
after Work, injections are exceptional committed pre-Work fixture operations,
and automatic sinks run after the historical read source. ``take`` is an
ordinary physical downstream pop, not an exceptional prepublished host pop.
Slots retain payload
after release and cannot capture and release in the same old-Q epoch.
"""

from copy import deepcopy

ENTRY_FIELDS = ("valid", "age", "src0_tag", "src0_ready", "src1_tag", "src1_ready")
ENTRY_WIDTHS = (1, 8, 8, 1, 8, 1)
WAKE_FIELDS = ("tag", "valid")
WAKE_WIDTHS = (8, 1)


def event(value=0):
    return {"value": value}


def entry(
    age=0, src0_tag=0, src1_tag=0, valid=False, src0_ready=False, src1_ready=False
):
    return {
        "valid": bool(valid),
        "age": age,
        "src0_tag": src0_tag,
        "src0_ready": bool(src0_ready),
        "src1_tag": src1_tag,
        "src1_ready": bool(src1_ready),
    }


def wakeup(tag=0, valid=False):
    return {"tag": tag, "valid": bool(valid)}


def slot(payload):
    return {"valid": False, "payload": payload}


class Model:
    def __init__(self, kind, initial_entries=None, first_epoch=0):
        assert kind in ("mailbox", "issue")
        self.kind = kind
        if kind == "mailbox":
            self.queues = {
                name: []
                for name in ("left_input", "right_input", "left_output", "right_output")
            }
            self.slots = {side: slot(event()) for side in ("left", "right")}
            self.entries = []
            self.depths = dict.fromkeys(self.queues, 1)
        else:
            self.queues = {name: [] for name in ("wakeups", "allocations", "output")}
            self.slots = {"wakeup": slot(wakeup()), "allocation": slot(entry())}
            self.entries = deepcopy(initial_entries or [entry() for _ in range(4)])
            assert len(self.entries) == 4
            self.depths = {"wakeups": 2, "allocations": 2, "output": 1}
        self.pops = dict.fromkeys(self.queues, 0)
        self.pushes = dict.fromkeys(self.queues, 0)
        self.received = {name: [] for name in self.queues if name.endswith("output")}
        self.epoch = first_epoch

    def snapshot(self):
        return deepcopy(
            {
                "queues": self.queues,
                "slots": self.slots,
                "entries": self.entries,
                "pops": self.pops,
                "pushes": self.pushes,
                "received": self.received,
            }
        )

    def step(self, actions=None, label="", rival=None):
        actions = deepcopy(actions or {})
        before = self.snapshot()
        offers = actions.get("offer", {})
        injections = actions.get("inject", {})
        takes = actions.get("take", [])
        sinks = actions.get("sink", [])
        assert not set(takes) & set(sinks)
        assert len(takes) == len(set(takes)) and len(sinks) == len(set(sinks))
        for operation in (offers, injections):
            for name, values in operation.items():
                assert name in self.queues and not name.endswith("output")
                assert 0 < len(values) <= self.depths[name] - len(self.queues[name]), (
                    label,
                    name,
                )
                for value in values:
                    self.validate(name, value)
        assert not set(offers) & set(injections)
        for name, values in injections.items():
            self.queues[name].extend(values)
            self.pushes[name] += len(values)
        work = self.snapshot()
        next_state = deepcopy(work)
        taken = {}
        for name in (*takes, *sinks):
            assert name.endswith("output"), name
            if name in takes:
                assert work["queues"][name], (label, name)
            if work["queues"][name]:
                taken[name] = deepcopy(work["queues"][name][0])
                self.pop(next_state, name)
                next_state["received"][name].append(taken[name])
        if self.kind == "mailbox":
            grants = self.mailbox(work, next_state, takes, rival)
        else:
            grants = self.issue(work, next_state, takes, rival)
        for name, values in offers.items():
            for value in values:
                self.push(next_state, name, value)
        for name, values in next_state["queues"].items():
            assert len(values) <= self.depths[name], (label, name, values)
        self.queues = next_state["queues"]
        self.slots = next_state["slots"]
        self.entries = next_state["entries"]
        self.pops = next_state["pops"]
        self.pushes = next_state["pushes"]
        self.received = next_state["received"]
        row = {
            "epoch": self.epoch,
            "label": label,
            "actions": actions,
            "before": before,
            "work": work,
            "after": self.snapshot(),
            "taken": taken,
            "grants": grants,
        }
        self.epoch += 1
        return row

    def validate(self, name, value):
        if self.kind == "mailbox":
            keys, widths = ("value",), (8,)
        elif name == "wakeups":
            keys, widths = WAKE_FIELDS, WAKE_WIDTHS
        else:
            keys, widths = ENTRY_FIELDS, ENTRY_WIDTHS
        assert set(value) == set(keys), (name, value)
        assert all(
            0 <= value[key] < (1 << width)
            for key, width in zip(keys, widths, strict=True)
        )

    @staticmethod
    def pop(state, name):
        value = state["queues"][name].pop(0)
        state["pops"][name] += 1
        return value

    @staticmethod
    def push(state, name, value):
        state["queues"][name].append(deepcopy(value))
        state["pushes"][name] += 1

    def mailbox(self, old, new, takes, rival):
        grants = {side: [] for side in ("left", "right")}
        for side in ("left", "right"):
            source, target = f"{side}_input", f"{side}_output"
            committed = old["slots"][side]
            dst = new["slots"][side]
            output_space = not old["queues"][target]
            if rival == "mailbox_output_replace" and target in takes:
                output_space = True
            consume = committed["valid"] and output_space
            if rival == "mailbox_release_without_output":
                consume = committed["valid"]
            capture = not committed["valid"] and bool(old["queues"][source])
            if rival == "mailbox_release_refill" and consume:
                capture = bool(old["queues"][source])
            if capture:
                grants[side].append("capture")
                dst["payload"] = self.pop(new, source)
                dst["valid"] = True
            if rival == "mailbox_capture_forward" and capture:
                consume = output_space
            if consume:
                grants[side].append("consume")
                if output_space:
                    value = (
                        dst["payload"]
                        if rival == "mailbox_capture_forward"
                        else committed["payload"]
                    )
                    self.push(new, target, value)
                if not (capture and rival == "mailbox_release_refill"):
                    dst["valid"] = False
                if rival == "mailbox_clear_payload":
                    dst["payload"] = event()
            if rival == "mailbox_shared_state" and side == "right":
                dst.update(deepcopy(new["slots"]["left"]))
        return grants

    def issue(self, old, new, takes, rival):
        table = old["entries"]
        wake_slot, alloc_slot = old["slots"]["wakeup"], old["slots"]["allocation"]
        wake_valid = wake_slot["valid"]
        if rival == "issue_payload_valid_guard":
            wake_valid = wake_valid and wake_slot["payload"]["valid"]
        grants = {
            "capture": [],
            "wakeup_src0": [],
            "wakeup_src1": [],
            "selected": None,
            "output_push": False,
            "valid_clear": False,
            "allocated": None,
            "release": [],
        }
        for slot_name, queue_name in (
            ("wakeup", "wakeups"),
            ("allocation", "allocations"),
        ):
            if not old["slots"][slot_name]["valid"] and old["queues"][queue_name]:
                if (
                    rival == "issue_capture_requires_free"
                    and slot_name == "allocation"
                    and all(e["valid"] for e in table)
                ):
                    continue
                new["slots"][slot_name].update(
                    valid=True, payload=self.pop(new, queue_name)
                )
                grants["capture"].append(slot_name)
        if rival == "issue_capture_forward":
            wake_slot, alloc_slot = new["slots"]["wakeup"], new["slots"]["allocation"]
            wake_valid = wake_slot["valid"]
        tag = wake_slot["payload"]["tag"]
        for i, value in enumerate(table):
            if wake_valid and value["valid"]:
                for operand in (0, 1):
                    if (
                        not value[f"src{operand}_ready"]
                        and value[f"src{operand}_tag"] == tag
                    ):
                        if (
                            rival == "issue_wakeup_whole_entry_last_writer"
                            and operand == 1
                        ):
                            new["entries"][i] = deepcopy(value)
                        new["entries"][i][f"src{operand}_ready"] = True
                        grants[f"wakeup_src{operand}"].append(i)
        selection_table = new["entries"] if rival == "issue_forward_wakeup" else table
        ready = [
            i
            for i, value in enumerate(selection_table)
            if value["valid"] and value["src0_ready"] and value["src1_ready"]
        ]
        if rival == "issue_src0_only_ready":
            ready = [
                i
                for i, value in enumerate(selection_table)
                if value["valid"] and value["src0_ready"]
            ]
        selected = (
            min(ready, key=lambda i: (selection_table[i]["age"], i)) if ready else None
        )
        if rival == "issue_first_instead_of_oldest":
            selected = min(ready) if ready else None
        if rival == "issue_reverse_ties":
            selected = (
                min(ready, key=lambda i: (selection_table[i]["age"], -i))
                if ready
                else None
            )
        if rival == "issue_serialize_wakeup" and wake_slot["valid"]:
            selected = None
        if rival == "issue_serialize_allocation" and alloc_slot["valid"]:
            selected = None
        space = not old["queues"]["output"]
        if rival == "issue_output_replace_for_take" and "output" in takes:
            space = True
        grants["selected"] = selected
        if selected is not None:
            if space:
                value = deepcopy(table[selected])
                if rival == "issue_read_new_candidate":
                    value["valid"] = False
                self.push(new, "output", value)
                grants["output_push"] = True
            if space or rival != "issue_clear_requires_output_space":
                new["entries"][selected]["valid"] = False
                grants["valid_clear"] = True
        free_table = new["entries"] if rival == "issue_refill_cleared_slot" else table
        free = next(
            (i for i, value in enumerate(free_table) if not value["valid"]), None
        )
        if alloc_slot["valid"] and free is not None:
            grants["allocated"] = free
            new["entries"][free] = deepcopy(alloc_slot["payload"])
            if rival == "issue_force_allocated_valid":
                new["entries"][free]["valid"] = True
            if rival == "issue_partial_allocation":
                new["entries"][free] = deepcopy(table[free])
                new["entries"][free].update(
                    valid=alloc_slot["payload"]["valid"],
                    age=alloc_slot["payload"]["age"],
                )
            if rival == "issue_forward_wakeup_to_allocation" and wake_valid:
                for operand in (0, 1):
                    if new["entries"][free][f"src{operand}_tag"] == tag:
                        new["entries"][free][f"src{operand}_ready"] = True
            new["slots"]["allocation"]["valid"] = False
            grants["release"].append("allocation")
        if wake_slot["valid"]:
            new["slots"]["wakeup"]["valid"] = False
            grants["release"].append("wakeup")
        if (
            rival == "issue_drop_full_allocation"
            and alloc_slot["valid"]
            and free is None
        ):
            new["slots"]["allocation"]["valid"] = False
        if rival == "issue_slot_release_refill":
            for slot_name in grants["release"]:
                queue_name = "wakeups" if slot_name == "wakeup" else "allocations"
                if old["queues"][queue_name]:
                    new["slots"][slot_name].update(
                        valid=True, payload=self.pop(new, queue_name)
                    )
        if rival == "issue_slot_clear_payload":
            for slot_name in grants["release"]:
                new["slots"][slot_name]["payload"] = (
                    wakeup() if slot_name == "wakeup" else entry()
                )
        return grants


def packed_snapshot(kind, snapshot):
    fields = []

    def record(value, keys, widths):
        fields.extend(
            (width, value[key]) for key, width in zip(keys, widths, strict=True)
        )

    def queue(name, depth, keys, widths, zero):
        values = snapshot["queues"][name]
        fields.append((1 if depth == 1 else 2, len(values)))
        for i in range(depth):
            record(values[i] if i < len(values) else zero, keys, widths)

    if kind == "mailbox":
        for side in ("left", "right"):
            queue(f"{side}_input", 1, ("value",), (8,), event())
            fields.append((1, snapshot["slots"][side]["valid"]))
            record(snapshot["slots"][side]["payload"], ("value",), (8,))
            queue(f"{side}_output", 1, ("value",), (8,), event())
    else:
        queue("wakeups", 2, WAKE_FIELDS, WAKE_WIDTHS, wakeup())
        queue("allocations", 2, ENTRY_FIELDS, ENTRY_WIDTHS, entry())
        for name, keys, widths in (
            ("wakeup", WAKE_FIELDS, WAKE_WIDTHS),
            ("allocation", ENTRY_FIELDS, ENTRY_WIDTHS),
        ):
            fields.append((1, snapshot["slots"][name]["valid"]))
            record(snapshot["slots"][name]["payload"], keys, widths)
        for value in snapshot["entries"]:
            record(value, ENTRY_FIELDS, ENTRY_WIDTHS)
        queue("output", 1, ENTRY_FIELDS, ENTRY_WIDTHS, entry())
    packed, width = 0, 0
    for bits, value in fields:
        assert 0 <= value < 1 << bits
        packed = (packed << bits) | int(value)
        width += bits
    assert width == (54 if kind == "mailbox" else 250), width
    return width, str(packed)


def expected_controls(kind, row):
    """Meaningful pre-edge public flags/data from OLD queues and grants.

    Empty queue heads and data without a selected/publishing token are omitted;
    the packed state separately normalizes unused queue cells to zero.
    """
    old, grants = row["work"], row["grants"]
    result = {}

    def queue(prefix, name, depth):
        values = old["queues"][name]
        result[f"{prefix}_ready"] = len(values) < depth
        result[f"{prefix}_available"] = bool(values)
        if values:
            result[f"{prefix}_head"] = deepcopy(values[0])

    if kind == "mailbox":
        for side in ("left", "right"):
            queue(f"{side}_in", f"{side}_input", 1)
            queue(f"{side}_out", f"{side}_output", 1)
            result[f"{side}_capture"] = "capture" in grants[side]
            result[f"{side}_publish"] = "consume" in grants[side]
            if result[f"{side}_publish"]:
                result[f"{side}_publish_data"] = deepcopy(old["slots"][side]["payload"])
    else:
        queue("allocation", "allocations", 2)
        queue("wakeup", "wakeups", 2)
        queue("issued", "output", 1)
        result.update(
            allocation_capture="allocation" in grants["capture"],
            wakeup_capture="wakeup" in grants["capture"],
            allocation_install=grants["allocated"] is not None,
            wakeup_release="wakeup" in grants["release"],
            selected=grants["selected"] is not None,
            publish=grants["output_push"],
        )
        if result["selected"]:
            result["publish_data"] = deepcopy(old["entries"][grants["selected"]])
    return result

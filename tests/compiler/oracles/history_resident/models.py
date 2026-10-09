"""Independent old-Q resident models, derived from the inert historical assets.

This test owner does not import a DUT, compiler, generated model, or retired
frontend. One step is one Work/Xfer epoch. Host offers are proposals committed
after Work; host takes publish pops before Work; ``inject`` is the historical
committed pre-Work readiness insertion, not an ordinary offer.
"""

from copy import deepcopy

ROB_WIDTHS = (2, 16, 16, 16, 1)
ISQ_WIDTHS = (2, 8, 6, 6, 16, 1)
ROB_FIELDS = ("index", "generation", "epoch", "value", "done")
ISQ_FIELDS = ("index", "age", "src0_tag", "src1_tag", "value", "valid")
ROB_QUEUES = tuple(
    f"{side}_{port}"
    for side in ("left", "right")
    for port in ("flush", "allocate", "completion")
) + ("left_allocated", "left_retired", "right_allocated", "right_retired")
ISQ_QUEUES = (
    "left_request",
    "left_readiness",
    "right_request",
    "right_readiness",
    "left_issued",
    "right_issued",
)


def event(value=0, index=0, generation=0, epoch=0, done=False):
    return {
        "index": index,
        "generation": generation,
        "epoch": epoch,
        "value": value,
        "done": bool(done),
    }


def entry(value=0, age=0, src0_tag=0, src1_tag=None, index=0, valid=False):
    return {
        "index": index,
        "age": age,
        "src0_tag": src0_tag,
        "src1_tag": src0_tag if src1_tag is None else src1_tag,
        "value": value,
        "valid": bool(valid),
    }


def readiness(tag, ready=True):
    return {"tag": tag, "ready": bool(ready)}


def completion(tag):
    return event(index=tag["index"], generation=tag["generation"], epoch=tag["epoch"])


class ResidentModel:
    def __init__(self, kind):
        self.kind = kind
        names = ROB_QUEUES if kind == "rob" else ISQ_QUEUES
        self.queues = {name: [] for name in names}
        self.instances = {}
        for side in ("left", "right"):
            if kind == "rob":
                self.instances[side] = {
                    "head": 0,
                    "tail": 0,
                    "count": 0,
                    "epoch": 0,
                    "entries": [event() for _ in range(4)],
                }
            else:
                self.instances[side] = {
                    "entries": [entry() for _ in range(4)],
                    "ready": [False] * 64,
                }
        self.epoch = 0

    def snapshot(self):
        return deepcopy({"queues": self.queues, "instances": self.instances})

    def step(self, actions=None, label="", rival=None):
        actions = deepcopy(actions or {})
        before_host = self.snapshot()
        offers = actions.get("offer", {})
        takes = actions.get("take", [])
        inject = actions.get("inject", {})
        assert len(takes) == len(set(takes))
        for name, value in offers.items():
            assert name in self.queues and not self.queues[name], (label, name)
            assert name not in inject
            assert not name.endswith(("allocated", "retired", "issued"))
            self._validate_record(name, value)
        taken = {}
        for name in takes:
            assert name.endswith(("allocated", "retired", "issued")), name
            assert self.queues[name], (label, name)
            taken[name] = deepcopy(self.queues[name][0])
        for name, value in inject.items():
            assert self.kind == "isq" and name.endswith("readiness"), name
            assert not self.queues[name], (label, name)
            self._validate_record(name, value)
            self.queues[name] = [value]
        # No other proposal may affect this immutable Work snapshot.
        work = self.snapshot()
        next_state = deepcopy(work)
        for name in takes:
            next_state["queues"][name] = []
        grants = {}
        writes = {}
        for side in ("left", "right"):
            if self.kind == "rob":
                grants[side], writes[side] = self._rob(
                    side, work, next_state, takes, rival
                )
            else:
                grants[side], writes[side] = self._isq(
                    side, work, next_state, takes, rival
                )
        for name, value in offers.items():
            # Host offers cannot exploit an internal pop not yet published.
            assert not next_state["queues"][name]
            next_state["queues"][name] = [value]
        self.queues = next_state["queues"]
        self.instances = next_state["instances"]
        result = {
            "epoch": self.epoch,
            "label": label,
            "actions": actions,
            "before": before_host,
            "work": work,
            "after": self.snapshot(),
            "taken": taken,
            "grants": grants,
            "writes": writes,
        }
        self.epoch += 1
        return result

    def _validate_record(self, name, value):
        if name.endswith("readiness"):
            fields, widths = ("tag", "ready"), (6, 1)
        elif self.kind == "rob":
            fields, widths = ROB_FIELDS, ROB_WIDTHS
        else:
            fields, widths = ISQ_FIELDS, ISQ_WIDTHS
        assert set(value) == set(fields), (name, value)
        assert all(
            0 <= value[f] < (1 << w) for f, w in zip(fields, widths, strict=True)
        )

    @staticmethod
    def _space(queues, name, takes, rival):
        # Transactional preparePush calls canPrepareBatch(0, 1), which rejects
        # a previously published host pop. The weaker canProposePush capacity
        # check alone is insufficient: take creates an output bubble.
        return not queues[name] or (name in takes and rival == "output_replace")

    def _rob(self, side, old, new, takes, rival):
        queues, state = old["queues"], old["instances"][side]
        dst, output = new["instances"][side], new["queues"]
        names = {
            port: f"{side}_{port}"
            for port in ("flush", "allocate", "completion", "allocated", "retired")
        }
        head, tail = state["head"], state["tail"]
        grants, writes = [], []
        flush = bool(queues[names["flush"]])
        allocate = (
            bool(queues[names["allocate"]])
            and state["count"] < 4
            and self._space(queues, names["allocated"], takes, rival)
            and not flush
        )
        complete = bool(queues[names["completion"]]) and not flush and not allocate
        if rival == "rob_allocate_complete_cofire":
            complete = bool(queues[names["completion"]]) and not flush
        tag = queues[names["completion"]][0] if complete else event()
        resident = state["entries"][tag["index"]]
        match = (
            resident["generation"] == tag["generation"]
            and resident["epoch"] == tag["epoch"]
            and resident["epoch"] == state["epoch"]
        )
        if rival == "rob_ignore_generation":
            match = resident["epoch"] == tag["epoch"] == state["epoch"]
        if rival == "rob_ignore_epoch":
            match = resident["generation"] == tag["generation"]
        conflict = complete and match and tag["index"] == head
        if rival == "rob_stale_completion_whole_entry_lock":
            conflict = complete and tag["index"] == head
        if rival == "rob_complete_retire_cofire":
            conflict = False
        retire = (
            state["count"] != 0
            and state["entries"][head]["done"]
            and self._space(queues, names["retired"], takes, rival)
            and not flush
            and not allocate
            and not conflict
        )
        if flush:
            grants.append("recover")
            output[names["flush"]] = []
            dst.update(head=tail, count=0, epoch=(state["epoch"] + 1) & 65535)
            writes.extend(("head", "tail", "count", "epoch"))
            if rival == "rob_flush_clear_entries":
                dst["entries"] = [event() for _ in range(4)]
        if allocate:
            grants.append("allocate")
            allocated = deepcopy(queues[names["allocate"]][0])
            allocated.update(
                index=tail,
                generation=(state["entries"][tail]["generation"] + 1) & 65535,
                epoch=state["epoch"],
                done=False,
            )
            dst["entries"][tail] = allocated
            dst.update(tail=(tail + 1) & 3, count=state["count"] + 1)
            output[names["allocate"]] = []
            output[names["allocated"]] = [deepcopy(allocated)]
            writes.extend((f"entries[{tail}].all", "tail", "count", "epoch"))
        if complete:
            grants.append("complete")
            output[names["completion"]] = []
            if match:
                dst["entries"][tag["index"]]["done"] = True
                writes.append(f"entries[{tag['index']}].done")
        if retire:
            grants.append("retire")
            output[names["retired"]] = [deepcopy(state["entries"][head])]
            dst["entries"][head]["done"] = False
            dst.update(head=(head + 1) & 3, count=state["count"] - 1)
            writes.extend((f"entries[{head}].done", "head", "count"))
        return grants, writes

    def _isq(self, side, old, new, takes, rival):
        queues, state = old["queues"], old["instances"][side]
        dst, output = new["instances"][side], new["queues"]
        request, readiness_name, issued = (
            f"{side}_{p}" for p in ("request", "readiness", "issued")
        )
        grants, writes = [], []
        update = bool(queues[readiness_name])
        changed = queues[readiness_name][0] if update else None
        free = next((i for i, e in enumerate(state["entries"]) if not e["valid"]), None)
        dispatch = bool(queues[request]) and free is not None
        # The historical query records both table reads before its predicate,
        # even for invalid entries; this is a reservation, not eligibility.
        snapshot_set = {
            e[f] for e in state["entries"] for f in ("src0_tag", "src1_tag")
        }
        if rival == "isq_valid_only_reservation":
            snapshot_set = {
                e[f]
                for e in state["entries"]
                if e["valid"]
                for f in ("src0_tag", "src1_tag")
            }
        if rival == "isq_src0_only_reservation":
            snapshot_set = {e["src0_tag"] for e in state["entries"]}
        if rival == "isq_full_ready_lock":
            snapshot_set = set(range(64))
        lookup = dst["ready"] if rival == "isq_new_ready_visibility" else state["ready"]
        if update and rival == "isq_new_ready_visibility":
            lookup[changed["tag"]] = changed["ready"]
        eligible = [
            i
            for i, e in enumerate(state["entries"])
            if e["valid"] and lookup[e["src0_tag"]] and lookup[e["src1_tag"]]
        ]
        selected = (
            min(eligible, key=lambda i: (state["entries"][i]["age"], i))
            if eligible
            else None
        )
        readiness_conflict = update and changed["tag"] in snapshot_set
        if rival == "isq_ignore_ready_reservation":
            readiness_conflict = False
        if rival == "isq_new_ready_visibility":
            readiness_conflict = False
        dispatch_conflict = dispatch and rival != "isq_dispatch_issue_cofire"
        issue = (
            selected is not None
            and self._space(queues, issued, takes, rival)
            and not dispatch_conflict
            and not readiness_conflict
        )
        if update:
            grants.append("update_ready")
            output[readiness_name] = []
            dst["ready"][changed["tag"]] = changed["ready"]
            writes.append(f"ready[{changed['tag']}]")
        if dispatch:
            grants.append("dispatch")
            installed = deepcopy(queues[request][0])
            installed.update(index=free, valid=True)
            dst["entries"][free] = installed
            output[request] = []
            writes.append(f"entries[{free}].all")
        if issue:
            grants.append("issue")
            output[issued] = [deepcopy(state["entries"][selected])]
            dst["entries"][selected]["valid"] = False
            writes.append(f"entries[{selected}].valid")
        return grants, writes


def packed_snapshot(kind, snapshot):
    """Complete, fixed-width snapshot; empty queue payloads are canonical zero.

    Concatenation is MSB-first in declared queue order, then left/right scalars
    (ROB), all left/right entries, then all left/right ready bits (ISQ).
    """
    fields = []
    names = ROB_QUEUES if kind == "rob" else ISQ_QUEUES
    for name in names:
        queue = snapshot["queues"][name]
        fields.append((1, bool(queue)))
        if name.endswith("readiness"):
            keys, widths = ("tag", "ready"), (6, 1)
        else:
            keys, widths = (
                (ROB_FIELDS, ROB_WIDTHS) if kind == "rob" else (ISQ_FIELDS, ISQ_WIDTHS)
            )
        fields.extend(
            (w, queue[0][key] if queue else 0)
            for key, w in zip(keys, widths, strict=True)
        )
    if kind == "rob":
        for side in ("left", "right"):
            state = snapshot["instances"][side]
            fields.extend(
                (w, state[key])
                for key, w in zip(
                    ("head", "tail", "count", "epoch"), (2, 2, 3, 16), strict=True
                )
            )
    for side in ("left", "right"):
        keys, widths = (
            (ROB_FIELDS, ROB_WIDTHS) if kind == "rob" else (ISQ_FIELDS, ISQ_WIDTHS)
        )
        for e in snapshot["instances"][side]["entries"]:
            fields.extend((w, e[key]) for key, w in zip(keys, widths, strict=True))
    if kind == "isq":
        for side in ("left", "right"):
            fields.extend((1, bit) for bit in snapshot["instances"][side]["ready"])
    result, width = 0, 0
    for bits, value in fields:
        assert 0 <= value < 1 << bits
        result = (result << bits) | int(value)
        width += bits
    assert width == (974 if kind == "rob" else 616), width
    return width, str(result)

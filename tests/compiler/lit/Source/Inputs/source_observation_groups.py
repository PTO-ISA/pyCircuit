from pycircuit import log, module, report, rule, system, u8, u16


@rule
def capture(value, incoming):
    log("info", "leaf", "old", value, "next", incoming, incoming == 17)
    value = incoming


@module
def GroupLeaf(incoming: u8) -> {"value": u8}:
    value: u8 = 0
    capture(value, incoming)
    return {"value": value}


@rule
def advance(phase):
    phase = phase + 1


@rule
def root_observations(phase, input_left, input_right, tag, stamp):
    log("info", "before", phase)
    log(
        "info",
        "mixed",
        "begin",
        phase == 1,
        input_left,
        "middle",
        input_right,
        tag,
        stamp,
        "end",
    )
    log("info", "literal", "words")
    log("info", "after", stamp)
    report("ticks", phase)


@rule
def check_window(phase):
    assert phase < 2, "grouped failure"


@system
def ObservationGroups():
    phase: u8 = 0
    input_left: u8 = 17
    input_right: u8 = 51
    tag: u8 = 93
    stamp: u8 = 119
    left = GroupLeaf()
    right = GroupLeaf()

    @rule
    def drive_left():
        left(incoming=input_left)

    @rule
    def drive_right():
        right(incoming=input_right)

    @rule
    def inspect():
        assert left.value == (0 if phase == 0 else 16 + phase), "left old Q"
        assert right.value == (0 if phase == 0 else 50 + phase), "right old Q"

    drive_left()
    drive_right()
    inspect()
    root_observations(phase, input_left, input_right, tag, stamp)
    advance(input_left)
    advance(input_right)
    advance(phase)


@system
def ObservationGroupsFailure():
    phase: u8 = 0
    input_left: u8 = 17
    input_right: u8 = 51
    tag: u8 = 93
    stamp: u8 = 119
    left = GroupLeaf()
    right = GroupLeaf()

    @rule
    def drive_left():
        left(incoming=input_left)

    @rule
    def drive_right():
        right(incoming=input_right)

    @rule
    def inspect():
        assert left.value == (0 if phase == 0 else 16 + phase), "left old Q"
        assert right.value == (0 if phase == 0 else 50 + phase), "right old Q"

    drive_left()
    drive_right()
    inspect()
    root_observations(phase, input_left, input_right, tag, stamp)
    advance(input_left)
    advance(input_right)
    advance(phase)
    check_window(phase)


@module
def ArithmeticLeaf(x: u8, wide: u16) -> {"seen": u8}:
    @rule
    def inspect():
        assert x <= 255, "arithmetic byte"
        log(
            "info",
            "arith",
            "sum_left",
            17 + x,
            "sum_right",
            x + 17,
            "subtract",
            99 - x,
            "product",
            2 * x,
            "equal",
            x == 99,
            "constants",
            93,
            119,
        )
        log("info", "arithmetic_scalar", x + 17)
        report("arithmetic_progress", x + 17)

    inspect()
    return {"seen": x}


@system
def ArithmeticObservationGroups():
    phase: u8 = 0
    dut = ArithmeticLeaf()

    @rule
    def drive():
        dut(
            x=(
                0
                if phase == 0
                else (
                    1
                    if phase == 1
                    else (
                        99
                        if phase == 2
                        else (
                            100
                            if phase == 3
                            else (239 if phase == 4 else (240 if phase == 5 else 255))
                        )
                    )
                )
            ),
            wide=1,
        )

    drive()
    advance(phase)


@rule
def next_sample(value, phase):
    if phase == 0:
        value = 1
    elif phase == 1:
        value = 99
    elif phase == 2:
        value = 100
    elif phase == 3:
        value = 239
    elif phase == 4:
        value = 240
    elif phase == 5:
        value = 255  # noqa: F841 - hardware register write


@system
def CapturedArithmeticObservationGroups():
    phase: u8 = 0
    x: u8 = 0
    wide: u16 = 1  # noqa: F841 - declared operand for rejection mutations

    @rule
    def inspect():
        assert x <= 255, "arithmetic byte"
        log(
            "info",
            "arith",
            "sum_left",
            17 + x,
            "sum_right",
            x + 17,
            "subtract",
            99 - x,
            "product",
            2 * x,
            "equal",
            x == 99,
            "constants",
            93,
            119,
        )
        log("info", "arithmetic_scalar", x + 17)
        report("arithmetic_progress", x + 17)

    inspect()
    next_sample(x, phase)
    advance(phase)


@rule
def advance_observation_sample(phase, sample):
    phase = phase + 1
    sample = sample + 3


@system
def OnlyObservationGroups():
    phase: u8 = 0
    sample: u8 = 17

    @rule
    def inspect_log():
        log("info", "only_log", "old", sample, phase + 7, sample == 20, 17 + sample)
        log("info", "only_literal", "unchanged")

    @rule
    def inspect_report():
        report("observation_sample", sample)

    advance_observation_sample(phase, sample)
    inspect_log()
    inspect_report()


@system
def OnlyObservationGroupsFailure():
    phase: u8 = 0
    sample: u8 = 17

    @rule
    def inspect_log():
        log("info", "only_log", "old", sample, phase + 7, sample == 20, 17 + sample)
        log("info", "only_literal", "unchanged")

    @rule
    def inspect_report():
        report("observation_sample", sample)

    advance_observation_sample(phase, sample)
    inspect_log()
    inspect_report()
    check_window(phase)

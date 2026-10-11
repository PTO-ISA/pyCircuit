"""Regular-clock traffic_lights_ce_pyc scenario; original physical-control oracles remain."""

from example_traffic_lights_ce_pyc.traffic_lights_ce_pyc import TopTrafficLights
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    end: bits[64]
    go: bits[1]
    emergency: bits[1]
    expected_ew_bcd: bits[8]
    expected_ns_bcd: bits[8]
    expected_ew_red: bits[1]
    expected_ew_yellow: bits[1]
    expected_ew_green: bits[1]
    expected_ns_red: bits[1]
    expected_ns_yellow: bits[1]
    expected_ns_green: bits[1]


@struct
class Scenarios:
    rows: table[55, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseTopTrafficLights():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(end=3, go=1, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=7, go=1, expected_ew_bcd=88, expected_ns_bcd=89, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=11, go=1, expected_ew_bcd=73, expected_ns_bcd=88, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=15, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=19, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=23, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_yellow=1, expected_ns_red=1),
            Scenario(end=27, go=1, expected_ew_bcd=89, expected_ns_bcd=88, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=31, go=1, expected_ew_bcd=88, expected_ns_bcd=73, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=35, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=39, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_red=1),
            Scenario(end=43, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_red=1, expected_ns_yellow=1),
            Scenario(end=47, go=1, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=51, go=1, expected_ew_bcd=88, expected_ns_bcd=89, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=55, go=1, expected_ew_bcd=73, expected_ns_bcd=88, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=59, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=63, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=67, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_yellow=1, expected_ns_red=1),
            Scenario(end=71, go=1, expected_ew_bcd=89, expected_ns_bcd=88, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=75, go=1, expected_ew_bcd=88, expected_ns_bcd=73, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=79, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=83, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_red=1),
            Scenario(end=87, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_red=1, expected_ns_yellow=1),
            Scenario(end=90, go=1, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=95, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=96, go=1, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=100, go=1, expected_ew_bcd=88, expected_ns_bcd=89, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=104, go=1, expected_ew_bcd=73, expected_ns_bcd=88, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=108, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=112, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=116, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_yellow=1, expected_ns_red=1),
            Scenario(end=121, expected_ew_bcd=89, expected_ns_bcd=88, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=125, go=1, expected_ew_bcd=89, expected_ns_bcd=88, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=130, expected_ew_bcd=88, expected_ns_bcd=73, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=138, go=1, emergency=1, expected_ew_bcd=136, expected_ns_bcd=136, expected_ew_red=1, expected_ns_red=1),
            Scenario(end=142, go=1, expected_ew_bcd=88, expected_ns_bcd=73, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=146, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=150, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_red=1),
            Scenario(end=154, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_red=1, expected_ns_yellow=1),
            Scenario(end=159, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=163, go=1, expected_ew_bcd=89, expected_ns_bcd=90, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=168, expected_ew_bcd=88, expected_ns_bcd=89, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=172, go=1, expected_ew_bcd=88, expected_ns_bcd=89, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=176, go=1, expected_ew_bcd=73, expected_ns_bcd=88, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=177, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=178, emergency=1, expected_ew_bcd=136, expected_ns_bcd=136, expected_ew_red=1, expected_ns_red=1),
            Scenario(end=181, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_green=1, expected_ns_red=1),
            Scenario(end=183, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=184, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=186, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ns_red=1),
            Scenario(end=190, go=1, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_yellow=1, expected_ns_red=1),
            Scenario(end=194, go=1, expected_ew_bcd=89, expected_ns_bcd=88, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=198, go=1, expected_ew_bcd=88, expected_ns_bcd=73, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=202, go=1, expected_ew_bcd=73, expected_ns_bcd=72, expected_ew_red=1, expected_ns_green=1),
            Scenario(end=206, go=1, expected_ew_bcd=72, expected_ns_bcd=73, expected_ew_red=1),
            Scenario(end=18446744073709551615, expected_ew_bcd=72, expected_ns_bcd=72, expected_ew_red=1, expected_ns_yellow=1),
        ),
    )
    # fmt: on
    position, found = scenarios.rows.first(where=lambda row: phase <= row.end)
    scenario = scenarios.rows[position]
    go = scenario.go
    emergency = scenario.emergency
    dut = TopTrafficLights(go, emergency)

    expected_ew_bcd = scenario.expected_ew_bcd
    expected_ns_bcd = scenario.expected_ns_bcd
    expected_ew_red = scenario.expected_ew_red
    expected_ew_yellow = scenario.expected_ew_yellow
    expected_ew_green = scenario.expected_ew_green
    expected_ns_red = scenario.expected_ns_red
    expected_ns_yellow = scenario.expected_ns_yellow
    expected_ns_green = scenario.expected_ns_green

    @rule
    def check_and_advance():
        if phase < 209:
            assert dut.ew_bcd == expected_ew_bcd, "traffic_lights_ce_pyc: ew_bcd"
            assert dut.ns_bcd == expected_ns_bcd, "traffic_lights_ce_pyc: ns_bcd"
            assert dut.ew_red == expected_ew_red, "traffic_lights_ce_pyc: ew_red"
            assert (
                dut.ew_yellow == expected_ew_yellow
            ), "traffic_lights_ce_pyc: ew_yellow"
            assert dut.ew_green == expected_ew_green, "traffic_lights_ce_pyc: ew_green"
            assert dut.ns_red == expected_ns_red, "traffic_lights_ce_pyc: ns_red"
            assert (
                dut.ns_yellow == expected_ns_yellow
            ), "traffic_lights_ce_pyc: ns_yellow"
            assert dut.ns_green == expected_ns_green, "traffic_lights_ce_pyc: ns_green"

        log("info", "traffic_lights_ce_pyc.ew_bcd", dut.ew_bcd)
        log("info", "traffic_lights_ce_pyc.ns_bcd", dut.ns_bcd)
        log("info", "traffic_lights_ce_pyc.ew_red", dut.ew_red)
        log("info", "traffic_lights_ce_pyc.ew_yellow", dut.ew_yellow)
        log("info", "traffic_lights_ce_pyc.ew_green", dut.ew_green)
        log("info", "traffic_lights_ce_pyc.ns_red", dut.ns_red)
        log("info", "traffic_lights_ce_pyc.ns_yellow", dut.ns_yellow)
        log("info", "traffic_lights_ce_pyc.ns_green", dut.ns_green)

    check_and_advance()
    advance(phase)

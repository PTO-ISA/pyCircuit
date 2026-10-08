"""Regular-clock traffic_lights_ce_pyc scenario; original physical-control oracles remain."""

from example_traffic_lights_ce_pyc.traffic_lights_ce_pyc import TopTrafficLights
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseTopTrafficLights():  # noqa: N802
    phase: bits[64] = 0
    go = (
        (
            (
                (((phase[:1] & 0) | 1) if phase < 91 else ((phase[:1] & 0) | 0))
                if phase < 96
                else (((phase[:1] & 0) | 1) if phase < 117 else ((phase[:1] & 0) | 0))
            )
            if phase < 122
            else (
                (((phase[:1] & 0) | 1) if phase < 126 else ((phase[:1] & 0) | 0))
                if phase < 131
                else (((phase[:1] & 0) | 1) if phase < 155 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 160
        else (
            (
                (((phase[:1] & 0) | 1) if phase < 164 else ((phase[:1] & 0) | 0))
                if phase < 169
                else (((phase[:1] & 0) | 1) if phase < 178 else ((phase[:1] & 0) | 0))
            )
            if phase < 179
            else (
                (((phase[:1] & 0) | 1) if phase < 184 else ((phase[:1] & 0) | 0))
                if phase < 185
                else (((phase[:1] & 0) | 1) if phase < 207 else ((phase[:1] & 0) | 0))
            )
        )
    )
    emergency = (
        (((phase[:1] & 0) | 0) if phase < 131 else ((phase[:1] & 0) | 1))
        if phase < 139
        else (
            ((phase[:1] & 0) | 0)
            if phase < 178
            else (((phase[:1] & 0) | 1) if phase < 179 else ((phase[:1] & 0) | 0))
        )
    )
    dut = TopTrafficLights(go, emergency)

    expected_ew_bcd = (
        (
            (
                (
                    (((phase[:8] & 0) | 89) if phase < 4 else ((phase[:8] & 0) | 88))
                    if phase < 8
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 12
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 16
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                )
                if phase < 20
                else (
                    (
                        ((phase[:8] & 0) | 72)
                        if phase < 24
                        else (
                            ((phase[:8] & 0) | 89)
                            if phase < 28
                            else ((phase[:8] & 0) | 88)
                        )
                    )
                    if phase < 32
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 36
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 44
                            else ((phase[:8] & 0) | 89)
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (((phase[:8] & 0) | 88) if phase < 52 else ((phase[:8] & 0) | 73))
                    if phase < 56
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 60
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 64
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                )
                if phase < 68
                else (
                    (
                        ((phase[:8] & 0) | 89)
                        if phase < 72
                        else (
                            ((phase[:8] & 0) | 88)
                            if phase < 76
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                    if phase < 80
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 88
                        else (
                            ((phase[:8] & 0) | 89)
                            if phase < 97
                            else ((phase[:8] & 0) | 88)
                        )
                    )
                )
            )
        )
        if phase < 101
        else (
            (
                (
                    (((phase[:8] & 0) | 73) if phase < 105 else ((phase[:8] & 0) | 72))
                    if phase < 109
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 113
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 117
                            else ((phase[:8] & 0) | 89)
                        )
                    )
                )
                if phase < 126
                else (
                    (
                        ((phase[:8] & 0) | 88)
                        if phase < 131
                        else (
                            ((phase[:8] & 0) | 136)
                            if phase < 139
                            else ((phase[:8] & 0) | 88)
                        )
                    )
                    if phase < 143
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 147
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 155
                            else ((phase[:8] & 0) | 89)
                        )
                    )
                )
            )
            if phase < 164
            else (
                (
                    (((phase[:8] & 0) | 88) if phase < 173 else ((phase[:8] & 0) | 73))
                    if phase < 177
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 178
                        else (
                            ((phase[:8] & 0) | 136)
                            if phase < 179
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                )
                if phase < 182
                else (
                    (
                        ((phase[:8] & 0) | 73)
                        if phase < 187
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 191
                            else ((phase[:8] & 0) | 89)
                        )
                    )
                    if phase < 195
                    else (
                        ((phase[:8] & 0) | 88)
                        if phase < 199
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 203
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                )
            )
        )
    )
    expected_ns_bcd = (
        (
            (
                (
                    (((phase[:8] & 0) | 90) if phase < 4 else ((phase[:8] & 0) | 89))
                    if phase < 8
                    else (
                        ((phase[:8] & 0) | 88)
                        if phase < 12
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 16
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                )
                if phase < 24
                else (
                    (
                        ((phase[:8] & 0) | 88)
                        if phase < 28
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 32
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                    if phase < 36
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 40
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 44
                            else ((phase[:8] & 0) | 90)
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (((phase[:8] & 0) | 89) if phase < 52 else ((phase[:8] & 0) | 88))
                    if phase < 56
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 60
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 68
                            else ((phase[:8] & 0) | 88)
                        )
                    )
                )
                if phase < 72
                else (
                    (
                        ((phase[:8] & 0) | 73)
                        if phase < 76
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 80
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                    if phase < 84
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 88
                        else (
                            ((phase[:8] & 0) | 90)
                            if phase < 97
                            else ((phase[:8] & 0) | 89)
                        )
                    )
                )
            )
        )
        if phase < 101
        else (
            (
                (
                    (((phase[:8] & 0) | 88) if phase < 105 else ((phase[:8] & 0) | 73))
                    if phase < 109
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 117
                        else (
                            ((phase[:8] & 0) | 88)
                            if phase < 126
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                )
                if phase < 131
                else (
                    (
                        ((phase[:8] & 0) | 136)
                        if phase < 139
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 143
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                    if phase < 147
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 151
                        else (
                            ((phase[:8] & 0) | 72)
                            if phase < 155
                            else ((phase[:8] & 0) | 90)
                        )
                    )
                )
            )
            if phase < 164
            else (
                (
                    (((phase[:8] & 0) | 89) if phase < 173 else ((phase[:8] & 0) | 88))
                    if phase < 177
                    else (
                        ((phase[:8] & 0) | 73)
                        if phase < 178
                        else (
                            ((phase[:8] & 0) | 136)
                            if phase < 179
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                )
                if phase < 182
                else (
                    (
                        ((phase[:8] & 0) | 72)
                        if phase < 191
                        else (
                            ((phase[:8] & 0) | 88)
                            if phase < 195
                            else ((phase[:8] & 0) | 73)
                        )
                    )
                    if phase < 199
                    else (
                        ((phase[:8] & 0) | 72)
                        if phase < 203
                        else (
                            ((phase[:8] & 0) | 73)
                            if phase < 207
                            else ((phase[:8] & 0) | 72)
                        )
                    )
                )
            )
        )
    )
    expected_ew_red = (
        (
            (((phase[:1] & 0) | 0) if phase < 24 else ((phase[:1] & 0) | 1))
            if phase < 44
            else (
                ((phase[:1] & 0) | 0)
                if phase < 68
                else (((phase[:1] & 0) | 1) if phase < 88 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 117
        else (
            (((phase[:1] & 0) | 1) if phase < 155 else ((phase[:1] & 0) | 0))
            if phase < 178
            else (
                ((phase[:1] & 0) | 1)
                if phase < 179
                else (((phase[:1] & 0) | 0) if phase < 191 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_ew_yellow = (
        (
            (((phase[:1] & 0) | 0) if phase < 20 else ((phase[:1] & 0) | 1))
            if phase < 24
            else (((phase[:1] & 0) | 0) if phase < 64 else ((phase[:1] & 0) | 1))
        )
        if phase < 68
        else (
            (((phase[:1] & 0) | 0) if phase < 113 else ((phase[:1] & 0) | 1))
            if phase < 117
            else (
                ((phase[:1] & 0) | 0)
                if phase < 187
                else (((phase[:1] & 0) | 1) if phase < 191 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_ew_green = (
        (
            (((phase[:1] & 0) | 1) if phase < 16 else ((phase[:1] & 0) | 0))
            if phase < 44
            else (
                ((phase[:1] & 0) | 1)
                if phase < 60
                else (((phase[:1] & 0) | 0) if phase < 88 else ((phase[:1] & 0) | 1))
            )
        )
        if phase < 109
        else (
            (((phase[:1] & 0) | 0) if phase < 155 else ((phase[:1] & 0) | 1))
            if phase < 178
            else (
                ((phase[:1] & 0) | 0)
                if phase < 179
                else (((phase[:1] & 0) | 1) if phase < 182 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_ns_red = (
        (
            (((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0))
            if phase < 44
            else (
                ((phase[:1] & 0) | 1)
                if phase < 68
                else (((phase[:1] & 0) | 0) if phase < 88 else ((phase[:1] & 0) | 1))
            )
        )
        if phase < 117
        else (
            (((phase[:1] & 0) | 0) if phase < 131 else ((phase[:1] & 0) | 1))
            if phase < 139
            else (
                ((phase[:1] & 0) | 0)
                if phase < 155
                else (((phase[:1] & 0) | 1) if phase < 191 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_ns_yellow = (
        (
            (((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1))
            if phase < 44
            else (((phase[:1] & 0) | 0) if phase < 84 else ((phase[:1] & 0) | 1))
        )
        if phase < 88
        else (
            (((phase[:1] & 0) | 0) if phase < 151 else ((phase[:1] & 0) | 1))
            if phase < 155
            else (((phase[:1] & 0) | 0) if phase < 207 else ((phase[:1] & 0) | 1))
        )
    )
    expected_ns_green = (
        (
            (((phase[:1] & 0) | 0) if phase < 24 else ((phase[:1] & 0) | 1))
            if phase < 36
            else (
                ((phase[:1] & 0) | 0)
                if phase < 68
                else (((phase[:1] & 0) | 1) if phase < 80 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 117
        else (
            (
                ((phase[:1] & 0) | 1)
                if phase < 131
                else (((phase[:1] & 0) | 0) if phase < 139 else ((phase[:1] & 0) | 1))
            )
            if phase < 147
            else (
                ((phase[:1] & 0) | 0)
                if phase < 191
                else (((phase[:1] & 0) | 1) if phase < 203 else ((phase[:1] & 0) | 0))
            )
        )
    )

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

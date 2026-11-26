# ruff: noqa: N802, F823, F841 -- pyCircuit modules and captured persistent state.
"""Conservative packet timing projection with one shared static slot table.

Each phase's disjoint groups run in parallel with identical timing. This module
projects their maximum completion time; it does not move payload or implement
per-port router queues. Runtime durations are derived from a declared profile.
One accepted start captures configuration at t=0. Every subsequent rising edge
consumes exactly one NoC timing tick, including transitions between stages.
"""

import pycircuit as ac


@ac.struct
class TimingConfig:
    row_read: ac.u32
    row_write: ac.u32
    row_flits: ac.u32
    column_read: ac.u32
    column_write: ac.u32
    column_flits: ac.u32
    hop: ac.u32
    tx: ac.u32
    rx: ac.u32
    alignment: ac.u32
    arm: ac.u32
    guard: ac.u32
    slot_cycles: ac.u32


@ac.struct
class TimingResult:
    total_cycles: ac.u32
    row_cycles: ac.u32
    column_cycles: ac.u32
    phase: ac.u1
    source_rank: ac.u3
    stage: ac.u4
    remaining: ac.u32
    slot_elapsed: ac.u32
    done: bool


@ac.module
def PacketTiming(start: bool, config: TimingConfig) -> TimingResult:
    settings: TimingConfig = TimingConfig(
        row_read=0,
        row_write=0,
        row_flits=0,
        column_read=0,
        column_write=0,
        column_flits=0,
        hop=0,
        tx=0,
        rx=0,
        alignment=0,
        arm=0,
        guard=0,
        slot_cycles=0,
    )
    total: ac.u32 = 0
    row: ac.u32 = 0
    column: ac.u32 = 0
    phase: ac.u1 = 0
    source: ac.u3 = 0
    stage: ac.u4 = 0
    remaining: ac.u32 = 0
    slot_elapsed: ac.u32 = 0

    @ac.rule
    def advance() -> None:
        if start:
            assert stage == 0, "timing start requires idle"
            assert config.row_read > 0 and config.row_read <= 1000000, "row read duration"
            assert config.row_write > 0 and config.row_write <= 1000000, "row write duration"
            assert config.row_flits > 0 and config.row_flits <= 1000000, "row flit count"
            assert config.column_read > 0 and config.column_read <= 1000000, "column read duration"
            assert (
                config.column_write > 0 and config.column_write <= 1000000
            ), "column write duration"
            assert config.column_flits > 0 and config.column_flits <= 1000000, "column flit count"
            assert config.hop > 0 and config.hop <= 1000000, "link hop duration"
            assert config.tx > 0 and config.tx <= 1000000, "transmit crossing duration"
            assert config.rx > 0 and config.rx <= 1000000, "receive crossing duration"
            assert config.alignment > 0 and config.alignment <= 1000000, "alignment duration"
            assert config.arm > 0 and config.arm <= 1000000, "arm duration"
            assert config.guard > 0 and config.guard <= 1000000, "guard duration"
            assert (
                config.slot_cycles > 0 and config.slot_cycles <= 1000000
            ), "fixed Calendar slot duration"
            row_network = config.row_flits + 7 * config.hop - 1
            column_network = config.column_flits + 3 * config.hop - 1
            common = config.tx + config.rx + config.guard
            row_service = config.row_read + row_network + config.row_write + common
            column_service = config.column_read + column_network + config.column_write + common
            assert row_service <= config.slot_cycles, "row packet exceeds fixed Calendar slot"
            assert column_service <= config.slot_cycles, "column packet exceeds fixed Calendar slot"
            settings = config
            remaining = config.alignment
            stage = 1
        elif stage > 0 and stage < 10:
            total = total + 1
            if phase == 0:
                row = row + 1
            else:
                column = column + 1
            if stage >= 3:
                slot_elapsed = slot_elapsed + 1
            assert remaining > 0, "active stage has a positive countdown"
            if remaining > 1:
                remaining = remaining - 1
            elif stage == 1:
                stage = 2
                remaining = settings.arm
            elif stage == 2:
                stage = 3
                remaining = settings.column_read if phase == 1 else settings.row_read
                slot_elapsed = 0
            elif stage == 3:
                stage = 4
                remaining = settings.tx
            elif stage == 4:
                stage = 5
                remaining = (
                    settings.column_flits + 3 * settings.hop - 1
                    if phase == 1
                    else settings.row_flits + 7 * settings.hop - 1
                )
            elif stage == 5:
                stage = 6
                remaining = settings.rx
            elif stage == 6:
                stage = 7
                remaining = settings.column_write if phase == 1 else settings.row_write
            elif stage == 7:
                stage = 8
                remaining = settings.guard
            elif stage == 8:
                last: ac.u3 = 3 if phase == 1 else 7
                assert slot_elapsed <= settings.slot_cycles, "Calendar window exhausted"
                if source == last:
                    if phase == 0:
                        phase = 1
                        source = 0
                        stage = 1
                        remaining = settings.alignment
                        slot_elapsed = 0
                    else:
                        stage = 10
                        remaining = 0
                elif slot_elapsed < settings.slot_cycles:
                    stage = 9
                    remaining = settings.slot_cycles - slot_elapsed
                else:
                    source = source + 1
                    stage = 3
                    remaining = settings.column_read if phase == 1 else settings.row_read
                    slot_elapsed = 0
            elif stage == 9:
                assert slot_elapsed == settings.slot_cycles, "next source starts at fixed slot"
                source = source + 1
                stage = 3
                remaining = settings.column_read if phase == 1 else settings.row_read
                slot_elapsed = 0

    return TimingResult(
        total_cycles=total,
        row_cycles=row,
        column_cycles=column,
        phase=phase,
        source_rank=source,
        stage=stage,
        remaining=remaining,
        slot_elapsed=slot_elapsed,
        done=stage == 10,
    )

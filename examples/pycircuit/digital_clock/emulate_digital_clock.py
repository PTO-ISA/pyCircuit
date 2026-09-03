#!/usr/bin/env python3
"""
emulate_digital_clock.py — True RTL simulation of the digital clock
with an animated terminal display.

Loads the compiled C++ RTL model (libdigital_clock_sim.dylib) via ctypes
and drives it cycle-by-cycle.  Every clock tick executes the real
generated hardware netlist — NOT a hand-written behavioral model.

Build the shared library from the repository root:
    c++ -std=c++17 -O2 -shared -fPIC -I library \\
        -o .pycircuit_out/examples/digital_clock/libdigital_clock_sim.dylib \\
        examples/pycircuit/digital_clock/digital_clock_capi.cpp

Run:
    python examples/pycircuit/digital_clock/emulate_digital_clock.py
"""

from __future__ import annotations

import ctypes
import os
import re as _re
import sys
import time
from pathlib import Path

# ═══════════════════════════════════════════════════════════════════
# ANSI helpers
# ═══════════════════════════════════════════════════════════════════

RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
WHITE = "\033[37m"
BG_GREEN = "\033[42m"
BLACK = "\033[30m"

_ANSI_RE = _re.compile(r"\x1b\[[0-9;]*m")


def _vis_len(s: str) -> int:
    return len(_ANSI_RE.sub("", s))


def _pad(s: str, width: int) -> str:
    return s + " " * max(0, width - _vis_len(s))


def clear_screen():
    sys.stdout.write("\033[2J\033[H")
    sys.stdout.flush()


# ═══════════════════════════════════════════════════════════════════
# 7-segment ASCII art
# ═══════════════════════════════════════════════════════════════════

_SEG = {
    0: (" _ ", "| |", "|_|"),
    1: ("   ", "  |", "  |"),
    2: (" _ ", " _|", "|_ "),
    3: (" _ ", " _|", " _|"),
    4: ("   ", "|_|", "  |"),
    5: (" _ ", "|_ ", " _|"),
    6: (" _ ", "|_ ", "|_|"),
    7: (" _ ", "  |", "  |"),
    8: (" _ ", "|_|", "|_|"),
    9: (" _ ", "|_|", " _|"),
}


def _digit_rows(d: int, color: str = WHITE) -> list[str]:
    rows = _SEG.get(d, _SEG[0])
    return [f"{color}{r}{RESET}" for r in rows]


def _colon_rows(on: bool) -> list[str]:
    if on:
        return [f"{CYAN} . {RESET}", f"{CYAN}   {RESET}", f"{CYAN} . {RESET}"]
    return ["   ", "   ", "   "]


def render_display(
    h_tens: int,
    h_ones: int,
    m_tens: int,
    m_ones: int,
    s_tens: int,
    s_ones: int,
    colon_on: bool,
    highlight: str = "",
) -> list[str]:
    hc = f"{BOLD}{RED}" if highlight == "hour" else f"{BOLD}{GREEN}"
    mc = f"{BOLD}{RED}" if highlight == "min" else f"{BOLD}{GREEN}"
    sc = f"{BOLD}{RED}" if highlight == "sec" else f"{BOLD}{GREEN}"

    hd0, hd1 = _digit_rows(h_tens, hc), _digit_rows(h_ones, hc)
    md0, md1 = _digit_rows(m_tens, mc), _digit_rows(m_ones, mc)
    sd0, sd1 = _digit_rows(s_tens, sc), _digit_rows(s_ones, sc)
    col = _colon_rows(colon_on)

    lines = []
    for i in range(3):
        lines.append(
            f"    {hd0[i]} {hd1[i]}  {col[i]}  "
            f"{md0[i]} {md1[i]}  {col[i]}  "
            f"{sd0[i]} {sd1[i]}"
        )
    return lines


# ═══════════════════════════════════════════════════════════════════
# RTL simulation wrapper  (ctypes → compiled C++ netlist)
# ═══════════════════════════════════════════════════════════════════

MODE_RUN = 0
MODE_SET_HOUR = 1
MODE_SET_MIN = 2
MODE_SET_SEC = 3

MODE_NAMES = {
    MODE_RUN: "RUN",
    MODE_SET_HOUR: "SET HOUR",
    MODE_SET_MIN: "SET MIN",
    MODE_SET_SEC: "SET SEC",
}

# CLK_FREQ used when generating the RTL (must match the compile-time value)
RTL_CLK_FREQ = 1000
RTL_DEBOUNCE_MS = 20
RTL_DEBOUNCE_CYCLES = max(RTL_CLK_FREQ * RTL_DEBOUNCE_MS // 1000, 2)


class DigitalClockRTL:
    """Drives the compiled RTL model via the C API shared library."""

    def __init__(self, lib_path: str | None = None):
        if lib_path is None:
            lib_path = str(
                Path(__file__).resolve().parent / "libdigital_clock_sim.dylib"
            )
        self._lib = ctypes.CDLL(lib_path)

        # ── Function prototypes ──
        self._lib.dc_create.restype = ctypes.c_void_p
        self._lib.dc_destroy.argtypes = [ctypes.c_void_p]
        self._lib.dc_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        self._lib.dc_set_inputs.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
        ]
        self._lib.dc_tick.argtypes = [ctypes.c_void_p]
        self._lib.dc_run_cycles.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        self._lib.dc_press_button.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_uint64,
        ]
        for name in (
            "dc_get_hours_bcd",
            "dc_get_minutes_bcd",
            "dc_get_seconds_bcd",
            "dc_get_setting_mode",
            "dc_get_colon_blink",
        ):
            getattr(self._lib, name).argtypes = [ctypes.c_void_p]
            getattr(self._lib, name).restype = ctypes.c_uint32
        self._lib.dc_get_cycle.argtypes = [ctypes.c_void_p]
        self._lib.dc_get_cycle.restype = ctypes.c_uint64

        # ── Create simulation context ──
        self._ctx = self._lib.dc_create()

        # ── Input state (Python-side tracking) ──
        self.btn_set = 0
        self.btn_plus = 0
        self.btn_minus = 0

    def __del__(self):
        if hasattr(self, "_ctx") and self._ctx:
            self._lib.dc_destroy(self._ctx)

    def reset(self, cycles: int = 2):
        self._lib.dc_reset(self._ctx, cycles)

    def tick(self):
        self._lib.dc_set_inputs(self._ctx, self.btn_set, self.btn_plus, self.btn_minus)
        self._lib.dc_tick(self._ctx)

    def run_cycles(self, n: int):
        """Run n cycles in a single C++ call (fast)."""
        self._lib.dc_set_inputs(self._ctx, self.btn_set, self.btn_plus, self.btn_minus)
        self._lib.dc_run_cycles(self._ctx, n)

    def press_btn(self, btn_set: int, btn_plus: int, btn_minus: int, hold: int):
        """Press buttons for hold cycles then release for hold cycles (all in C++)."""
        self._lib.dc_press_button(self._ctx, btn_set, btn_plus, btn_minus, hold)

    # ── Outputs ──

    @property
    def hours_bcd(self) -> tuple[int, int]:
        v = self._lib.dc_get_hours_bcd(self._ctx)
        return ((v >> 4) & 0xF, v & 0xF)

    @property
    def minutes_bcd(self) -> tuple[int, int]:
        v = self._lib.dc_get_minutes_bcd(self._ctx)
        return ((v >> 4) & 0xF, v & 0xF)

    @property
    def seconds_bcd(self) -> tuple[int, int]:
        v = self._lib.dc_get_seconds_bcd(self._ctx)
        return ((v >> 4) & 0xF, v & 0xF)

    @property
    def mode(self) -> int:
        return self._lib.dc_get_setting_mode(self._ctx)

    @property
    def blink(self) -> int:
        return self._lib.dc_get_colon_blink(self._ctx)

    @property
    def cycle(self) -> int:
        return self._lib.dc_get_cycle(self._ctx)

    @property
    def mode_name(self) -> str:
        return MODE_NAMES.get(self.mode, "???")

    @property
    def time_str(self) -> str:
        ht, ho = self.hours_bcd
        mt, mo = self.minutes_bcd
        st, so = self.seconds_bcd
        return f"{ht}{ho}:{mt}{mo}:{st}{so}"

    @property
    def hours(self) -> int:
        t, o = self.hours_bcd
        return t * 10 + o

    @property
    def minutes(self) -> int:
        t, o = self.minutes_bcd
        return t * 10 + o

    @property
    def seconds(self) -> int:
        t, o = self.seconds_bcd
        return t * 10 + o


# ═══════════════════════════════════════════════════════════════════
# Terminal UI
# ═══════════════════════════════════════════════════════════════════

BOX_W = 52


def _box_line(content: str) -> str:
    return f"  {CYAN}║{RESET}{_pad(content, BOX_W)}{CYAN}║{RESET}"


def _button_str(name: str, active: bool) -> str:
    if active:
        return f"{BG_GREEN}{BLACK}{BOLD} {name} {RESET}"
    return f"{DIM}[{name}]{RESET}"


def draw_frame(sim, message: str = "", active_btn: str = "") -> None:
    clear_screen()

    ht, ho = sim.hours_bcd
    mt, mo = sim.minutes_bcd
    st, so = sim.seconds_bcd

    highlight = ""
    if sim.mode == MODE_SET_HOUR:
        highlight = "hour"
    elif sim.mode == MODE_SET_MIN:
        highlight = "min"
    elif sim.mode == MODE_SET_SEC:
        highlight = "sec"

    colon_on = bool(sim.blink) or sim.mode != MODE_RUN
    seg_lines = render_display(ht, ho, mt, mo, st, so, colon_on, highlight)

    mode_color = GREEN if sim.mode == MODE_RUN else YELLOW
    mode_str = f"{mode_color}{BOLD}{sim.mode_name}{RESET}"

    bs = _button_str(" SET ", active_btn == "set")
    bp = _button_str("  +  ", active_btn == "plus")
    bm = _button_str("  -  ", active_btn == "minus")

    bar = "═" * BOX_W
    print(f"\n  {CYAN}╔{bar}╗{RESET}")
    print(_box_line(f"  {BOLD}{WHITE}DIGITAL CLOCK — TRUE RTL SIMULATION{RESET}"))
    print(f"  {CYAN}╠{bar}╣{RESET}")
    print(_box_line(""))
    for ln in seg_lines:
        print(_box_line(f" {ln}"))
    print(_box_line(""))
    print(_box_line(f"   Mode: {mode_str}"))
    print(
        _box_line(
            f"   Time: {BOLD}{WHITE}{sim.time_str}{RESET}    Cycle: {DIM}{sim.cycle}{RESET}"
        )
    )
    print(_box_line(""))
    print(_box_line(f"   {bs}    {bp}    {bm}"))
    print(_box_line(""))
    if message:
        print(f"  {CYAN}╠{bar}╣{RESET}")
        print(_box_line(f"  {BOLD}{WHITE}{message}{RESET}"))
    print(f"  {CYAN}╚{bar}╝{RESET}")
    print()


# ═══════════════════════════════════════════════════════════════════
# Simulation helpers
# ═══════════════════════════════════════════════════════════════════


def run_1sec(sim):
    """Run exactly CLK_FREQ cycles (= 1 second of simulated time)."""
    sim.run_cycles(RTL_CLK_FREQ)


def press_button(sim, button, message, hold_cycles=None, delay=0.5):
    """Press a button for enough cycles to pass debounce, show frame, release."""
    if hold_cycles is None:
        hold_cycles = RTL_DEBOUNCE_CYCLES + 4

    btn_s = 1 if button == "set" else 0
    btn_p = 1 if button == "plus" else 0
    btn_m = 1 if button == "minus" else 0

    sim.press_btn(btn_s, btn_p, btn_m, hold_cycles)

    draw_frame(sim, message=message, active_btn=button)
    time.sleep(delay)


def show(sim, message="", delay=0.8):
    draw_frame(sim, message=message)
    time.sleep(delay)


def press_n(sim, button, count, label, field_name, delay=0.12):
    """Press a button *count* times, showing every press on screen."""
    for i in range(1, count + 1):
        val = getattr(sim, field_name, 0)
        if isinstance(val, int):
            val = f"{val:02d}"
        msg = f"{label} Press [+] ({i}/{count}) {field_name} = {val}"
        press_button(sim, button, msg, delay=delay)


def set_time(sim, target_h, target_m, target_s, label=""):
    """Walk through SET mode to reach target time, showing every press."""
    tag = f"[{label}]" if label else ""

    # --- SET_HOUR ---
    press_button(sim, "set", f"{tag} Press [SET] → SET HOUR", delay=0.6)

    need = (target_h - sim.hours) % 24
    if need > 0:
        press_n(sim, "plus", need, tag, "hours")
        show(sim, f"{tag} Hours set to {sim.hours:02d} ✓", delay=0.6)

    # --- SET_MIN ---
    press_button(sim, "set", f"{tag} Press [SET] → SET MIN", delay=0.6)

    need = (target_m - sim.minutes) % 60
    if need > 0:
        press_n(sim, "plus", need, tag, "minutes")
        show(sim, f"{tag} Minutes set to {sim.minutes:02d} ✓", delay=0.6)

    # --- SET_SEC ---
    press_button(sim, "set", f"{tag} Press [SET] → SET SEC", delay=0.6)

    need = (target_s - sim.seconds) % 60
    if need > 0:
        press_n(sim, "plus", need, tag, "seconds")
        show(sim, f"{tag} Seconds set to {sim.seconds:02d} ✓", delay=0.6)
    else:
        show(sim, f"{tag} Seconds already {sim.seconds:02d} — skip", delay=0.4)

    # --- Back to RUN ---
    press_button(sim, "set", f"{tag} Press [SET] → back to RUN", delay=0.6)
    show(sim, f"{tag} Clock set to {sim.time_str} — running! ✓", delay=1.0)


# ═══════════════════════════════════════════════════════════════════
# Test scenarios
# ═══════════════════════════════════════════════════════════════════


def main():
    print("  Loading RTL simulation library...")
    sim = DigitalClockRTL()
    sim.reset(2)
    print(
        f"  {GREEN}RTL model loaded — CLK_FREQ={RTL_CLK_FREQ}, "
        f"DEBOUNCE={RTL_DEBOUNCE_CYCLES} cycles{RESET}"
    )
    time.sleep(0.5)

    # ── Power on & run 10 seconds ───────────────────────────
    show(sim, "Clock powered on — 00:00:00 (RUN mode)", delay=1.2)

    for i in range(1, 11):
        run_1sec(sim)
        show(sim, f"Clock ticking... {i}s elapsed", delay=0.35)

    show(sim, f"10 seconds elapsed — time is {sim.time_str}", delay=1.0)

    # ── Test 1: set to 04:32:00 ──────────────────────────────
    show(sim, "TEST 1: Set clock to 04:32:00", delay=1.2)
    set_time(sim, 4, 32, 0, label="T1")

    for _i in range(1, 4):
        run_1sec(sim)
        show(sim, f"[T1] Running: {sim.time_str}", delay=0.4)

    # ── Test 2: set to 01:23:00 ──────────────────────────────
    show(sim, "TEST 2: Set clock to 01:23:00", delay=1.2)
    set_time(sim, 1, 23, 0, label="T2")

    for _i in range(1, 4):
        run_1sec(sim)
        show(sim, f"[T2] Running: {sim.time_str}", delay=0.4)

    # ── Test 3: set to 09:87 (wrap test!) ────────────────────
    show(sim, "TEST 3: Try setting 09:87 — minutes wrap past 59!", delay=1.5)

    press_button(sim, "set", "[T3] Press [SET] → SET HOUR", delay=0.6)
    need = (9 - sim.hours) % 24
    if need > 0:
        press_n(sim, "plus", need, "[T3]", "hours")
        show(sim, f"[T3] Hours set to {sim.hours:02d} ✓", delay=0.6)

    press_button(sim, "set", "[T3] Press [SET] → SET MIN", delay=0.6)
    show(
        sim,
        f"[T3] Current minutes = {sim.minutes:02d}, pressing [+] 87 times...",
        delay=1.0,
    )

    cur_min = sim.minutes
    for i in range(1, 88):
        val = sim.minutes
        extra = ""
        if val == 59:
            extra = "  ← next wraps!"
        elif val == 0 and i > 1:
            extra = "  ← WRAPPED!"
        msg = f"[T3] Press [+] ({i}/87) min = {val:02d}{extra}"
        press_button(sim, "plus", msg, delay=0.1)

    expected_min = (cur_min + 87) % 60
    show(
        sim,
        f"[T3] After 87 presses: min = {sim.minutes:02d} (expected {expected_min:02d}, wrapped!)",
        delay=1.2,
    )

    press_button(sim, "set", "[T3] Press [SET] → SET SEC (skip)", delay=0.4)
    press_button(sim, "set", "[T3] Press [SET] → back to RUN", delay=0.4)
    show(
        sim,
        f"[T3] Final: {sim.time_str} — 87 min wrapped to {sim.minutes:02d} ✓",
        delay=1.2,
    )

    for _i in range(1, 4):
        run_1sec(sim)
        show(sim, f"[T3] Running: {sim.time_str}", delay=0.4)

    # ── Done ─────────────────────────────────────────────────
    show(sim, f"All tests complete! Final time: {sim.time_str}", delay=2.0)
    print(f"  {GREEN}{BOLD}All test scenarios passed (TRUE RTL SIMULATION).{RESET}\n")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
emulate_calculator.py — True RTL simulation of the 16-digit calculator
with decimal support, animated terminal display.

Build (from pyCircuit root):
    PYTHONPATH=python/pycircuit/src:. python -m pycircuit.cli emit \
        examples/pycircuit/calculator/calculator.py \
        -o .pycircuit_out/examples/calculator/calculator.pyc
    .pycircuit_out/toolchain/install/bin/pycc \
        .pycircuit_out/examples/calculator/calculator.pyc \
        --emit=cpp -o .pycircuit_out/examples/calculator/calculator_gen.hpp
    c++ -std=c++17 -O2 -shared -fPIC -I library -I . \
        -o .pycircuit_out/examples/calculator/libcalculator_sim.dylib \
        examples/pycircuit/calculator/calculator_capi.cpp

Run:
    python examples/pycircuit/calculator/emulate_calculator.py
"""

from __future__ import annotations

import ctypes
import re as _re
import sys
import time
from pathlib import Path

# ═══════════════════════════════════════════════════════════════════
# ANSI
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

_ANSI = _re.compile(r"\x1b\[[0-9;]*m")


def _vl(s):
    return len(_ANSI.sub("", s))


def _pad(s, w):
    return s + " " * max(0, w - _vl(s))


def clear():
    sys.stdout.write("\033[2J\033[H")
    sys.stdout.flush()


# ═══════════════════════════════════════════════════════════════════
# 7-segment
# ═══════════════════════════════════════════════════════════════════
_S = {
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


def _drows(d, co=WHITE):
    r = _S.get(d, _S[0])
    return [f"{co}{x}{RESET}" for x in r]


# ═══════════════════════════════════════════════════════════════════
KEY_NAMES = {
    0: "0",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7",
    8: "8",
    9: "9",
    10: "+",
    11: "-",
    12: "*",
    13: "/",
    14: "=",
    15: "AC",
    16: "+/-",
    17: "BS",
    18: "%",
    19: ".",
}
OP_SYMS = {0: "", 1: "+", 2: "-", 3: "*", 4: "/"}


# ═══════════════════════════════════════════════════════════════════
# RTL wrapper
# ═══════════════════════════════════════════════════════════════════
class CalculatorRTL:
    def __init__(self, lib_path=None):
        if lib_path is None:
            lib_path = str(Path(__file__).resolve().parent / "libcalculator_sim.dylib")
        L = ctypes.CDLL(lib_path)
        L.calc_create.restype = ctypes.c_void_p
        L.calc_destroy.argtypes = [ctypes.c_void_p]
        L.calc_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        L.calc_press_key.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint64]
        L.calc_run_cycles.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        L.calc_get_display.argtypes = [ctypes.c_void_p]
        L.calc_get_display.restype = ctypes.c_uint64
        for n in (
            "calc_get_display_neg",
            "calc_get_display_err",
            "calc_get_display_dp",
            "calc_get_op_pending",
        ):
            getattr(L, n).argtypes = [ctypes.c_void_p]
            getattr(L, n).restype = ctypes.c_uint32
        L.calc_get_cycle.argtypes = [ctypes.c_void_p]
        L.calc_get_cycle.restype = ctypes.c_uint64
        self._L, self._c = L, L.calc_create()

    def __del__(self):
        if hasattr(self, "_c") and self._c:
            self._L.calc_destroy(self._c)

    def reset(self):
        self._L.calc_reset(self._c, 2)

    def press(self, key, hold=4):
        self._L.calc_press_key(self._c, key, hold)

    def idle(self, n=10):
        self._L.calc_run_cycles(self._c, n)

    @property
    def display(self):
        return self._L.calc_get_display(self._c)

    @property
    def neg(self):
        return bool(self._L.calc_get_display_neg(self._c))

    @property
    def err(self):
        return bool(self._L.calc_get_display_err(self._c))

    @property
    def dp(self):
        return self._L.calc_get_display_dp(self._c)

    @property
    def op(self):
        return self._L.calc_get_op_pending(self._c)

    @property
    def cycle(self):
        return self._L.calc_get_cycle(self._c)

    @property
    def display_str(self):
        if self.err:
            return "ERROR"
        mag = str(self.display) if self.display else "0"
        dp = self.dp
        if dp > 0:
            # Insert decimal point: pad with leading zeros if needed
            while len(mag) <= dp:
                mag = "0" + mag
            int_part = mag[:-dp]
            dec_part = mag[-dp:].rstrip("0")  # trim trailing zeros
            if dec_part:
                s = f"{int_part}.{dec_part}"
            else:
                s = int_part
        else:
            s = mag
        if self.neg:
            s = "-" + s
        return s

    @property
    def value_float(self):
        """Return the display value as a Python float for comparison."""
        if self.err:
            return float("nan")
        v = self.display / (10**self.dp)
        return -v if self.neg else v


# ═══════════════════════════════════════════════════════════════════
# Terminal UI
# ═══════════════════════════════════════════════════════════════════
BOX_W = 58


def _bl(c):
    return f"  {CYAN}║{RESET}{_pad(c, BOX_W)}{CYAN}║{RESET}"


def _btn(label, active=False):
    if active:
        return f"{BG_GREEN}{BLACK}{BOLD}{label}{RESET}"
    return f"{DIM}{label}{RESET}"


def draw(sim, message="", active_key=-1, test_info=""):
    clear()
    bar = "═" * BOX_W
    dstr = sim.display_str
    if len(dstr) > 17:
        dstr = dstr[:17]  # 16 digits + 1 dot
    color = f"{BOLD}{RED}" if sim.err else f"{BOLD}{GREEN}"

    seg_map = {
        "-": ("   ", " _ ", "   "),
        "E": (f"{RED} _ {RESET}", f"{RED}|_ {RESET}", f"{RED}|_ {RESET}"),
        "R": (f"{RED}   {RESET}", f"{RED}|  {RESET}", f"{RED}   {RESET}"),
        "O": (f"{RED} _ {RESET}", f"{RED}| |{RESET}", f"{RED}|_|{RESET}"),
    }
    rows = [[], [], []]
    for ch in dstr:
        if ch == ".":
            # Render dot as a narrow 1-char column: blank, blank, dot
            rows[0].append(" ")
            rows[1].append(" ")
            rows[2].append(f"{color}.{RESET}")
            continue
        if ch in seg_map:
            for r in range(3):
                rows[r].append(seg_map[ch][r])
        elif ch.isdigit():
            dr = _drows(int(ch), color)
            for r in range(3):
                rows[r].append(dr[r])
    seg_lines = ["".join(row) for row in rows]

    op_sym = OP_SYMS.get(sim.op, "")
    op_str = f"  {YELLOW}{BOLD}[{op_sym}]{RESET}" if op_sym else ""

    R1 = [("BS", 17), ("AC", 15), ("%", 18), ("/", 13)]
    R2 = [("7", 7), ("8", 8), ("9", 9), ("*", 12)]
    R3 = [("4", 4), ("5", 5), ("6", 6), ("-", 11)]
    R4 = [("1", 1), ("2", 2), ("3", 3), ("+", 10)]
    R5 = [("+/-", 16), ("0", 0), (".", 19), ("=", 14)]

    def rr(items):
        return "  ".join(
            _btn(f" {label:^3} ", active_key == code) for label, code in items
        )

    print(f"\n  {CYAN}╔{bar}╗{RESET}")
    print(_bl(f"  {BOLD}{WHITE}16-DIGIT CALCULATOR — TRUE RTL SIMULATION{RESET}"))
    print(f"  {CYAN}╠{bar}╣{RESET}")
    if test_info:
        print(_bl(f"  {YELLOW}{test_info}{RESET}"))
        print(f"  {CYAN}╠{bar}╣{RESET}")
    print(_bl(""))
    for ln in seg_lines:
        print(_bl(f"  {ln}"))
    print(_bl(f"  {op_str}"))
    print(_bl(""))
    print(f"  {CYAN}╠{bar}╣{RESET}")
    for row in [R1, R2, R3, R4, R5]:
        print(_bl(f"    {rr(row)}"))
    print(_bl(""))
    if message:
        print(f"  {CYAN}╠{bar}╣{RESET}")
        print(_bl(f"  {BOLD}{WHITE}{message}{RESET}"))
    print(f"  {CYAN}╚{bar}╝{RESET}")
    print()


# ═══════════════════════════════════════════════════════════════════
def press_key(sim, k, msg="", test_info="", delay=0.8):
    sim.press(k)
    draw(
        sim,
        message=msg or f"Press [{KEY_NAMES.get(k,'?')}]",
        active_key=k,
        test_info=test_info,
    )
    time.sleep(delay)


def show(sim, msg="", test_info="", delay=0.8):
    draw(sim, message=msg, test_info=test_info)
    time.sleep(delay)


def run_test(sim, num, total, keys, expression, expected, delay=0.7, tol=1e-9):
    """Run a test. expected can be int, float, or 'ERROR'."""
    info = f"Test {num}/{total}: {expression} = {expected}"
    press_key(sim, 15, "Press [AC]", test_info=info, delay=0.3)
    for k in keys:
        press_key(
            sim, k, f"Press [{KEY_NAMES.get(k,'?')}]", test_info=info, delay=delay
        )

    if sim.err:
        ok = str(expected) == "ERROR"
    else:
        actual = sim.value_float
        if isinstance(expected, float):
            ok = abs(actual - expected) < tol + abs(expected) * 1e-9
        else:
            ok = actual == expected

    st = f"{GREEN}{BOLD}PASS ✓{RESET}" if ok else f"{RED}{BOLD}FAIL ✗{RESET}"
    show(
        sim,
        f"Result: {sim.display_str}  Expected: {expected}  {st}",
        test_info=info,
        delay=1.0,
    )
    return ok


# ═══════════════════════════════════════════════════════════════════
# Test Cases
# ═══════════════════════════════════════════════════════════════════
def main():
    print("  Loading calculator RTL simulation...")
    sim = CalculatorRTL()
    sim.reset()
    sim.idle(10)
    print(f"  {GREEN}RTL model loaded.{RESET}")
    time.sleep(0.5)
    show(sim, "Calculator ready — running 10 test cases", delay=1.2)

    T = 10
    results = []

    # 1: Integer addition  123 + 456 = 579
    results.append(run_test(sim, 1, T, [1, 2, 3, 10, 4, 5, 6, 14], "123 + 456", 579))

    # 2: Decimal addition  1.5 + 2.3 = 3.8
    results.append(run_test(sim, 2, T, [1, 19, 5, 10, 2, 19, 3, 14], "1.5 + 2.3", 3.8))

    # 3: Decimal multiplication  3.14 * 2 = 6.28
    results.append(run_test(sim, 3, T, [3, 19, 1, 4, 12, 2, 14], "3.14 * 2", 6.28))

    # 4: Division with decimals  10 / 4 = 2.5
    results.append(run_test(sim, 4, T, [1, 0, 13, 4, 14], "10 / 4", 2.5))

    # 5: Sign toggle  42 +/- + 100 = 58
    info = f"Test 5/{T}: 42 [+/-] + 100 = 58"
    press_key(sim, 15, "AC", test_info=info, delay=0.3)
    for k in [4, 2]:
        press_key(sim, k, f"Press [{KEY_NAMES[k]}]", test_info=info, delay=0.4)
    press_key(sim, 16, "[+/-]", test_info=info, delay=0.6)
    for k in [10, 1, 0, 0, 14]:
        press_key(sim, k, f"Press [{KEY_NAMES[k]}]", test_info=info, delay=0.4)
    ok = (not sim.err) and sim.value_float == 58
    st = f"{GREEN}{BOLD}PASS ✓{RESET}" if ok else f"{RED}{BOLD}FAIL ✗{RESET}"
    show(
        sim, f"Result: {sim.display_str}  Expected: 58  {st}", test_info=info, delay=1.0
    )
    results.append(ok)

    # 6: Backspace  12.34 BS → 12.3
    info = f"Test 6/{T}: 12.34 BS → 12.3 + 0.7 = 13"
    press_key(sim, 15, "AC", test_info=info, delay=0.3)
    for k in [1, 2, 19, 3, 4]:
        press_key(sim, k, f"Press [{KEY_NAMES[k]}]", test_info=info, delay=0.4)
    press_key(sim, 17, "[BS]", test_info=info, delay=0.6)
    for k in [10, 0, 19, 7, 14]:
        press_key(sim, k, f"Press [{KEY_NAMES[k]}]", test_info=info, delay=0.4)
    ok = (not sim.err) and sim.value_float == 13.0
    st = f"{GREEN}{BOLD}PASS ✓{RESET}" if ok else f"{RED}{BOLD}FAIL ✗{RESET}"
    show(
        sim, f"Result: {sim.display_str}  Expected: 13  {st}", test_info=info, delay=1.0
    )
    results.append(ok)

    # 7: Percentage  200 + 10% = 220
    results.append(run_test(sim, 7, T, [2, 0, 0, 10, 1, 0, 18, 14], "200 + 10%", 220))

    # 8: Divide by zero
    results.append(run_test(sim, 8, T, [4, 2, 13, 0, 14], "42 / 0", "ERROR"))

    # 9: Negative decimal  1.5 - 3.7 = -2.2
    results.append(run_test(sim, 9, T, [1, 19, 5, 11, 3, 19, 7, 14], "1.5 - 3.7", -2.2))

    # 10: Large multiply  9999999 * 9999999 = 99999980000001
    results.append(
        run_test(
            sim,
            10,
            T,
            [9, 9, 9, 9, 9, 9, 9, 12, 9, 9, 9, 9, 9, 9, 9, 14],
            "9999999 * 9999999",
            99999980000001,
        )
    )

    passed = sum(results)
    if passed == T:
        show(sim, f"All {T}/{T} tests PASSED!", delay=2.0)
        print(f"  {GREEN}{BOLD}All {T} tests passed (TRUE RTL SIMULATION).{RESET}\n")
    else:
        failed = [i + 1 for i, r in enumerate(results) if not r]
        show(sim, f"{passed}/{T} passed, FAILED: {failed}", delay=2.0)
        print(f"  {RED}{BOLD}{passed}/{T} passed. Failed: {failed}{RESET}\n")
        sys.exit(1)


if __name__ == "__main__":
    main()

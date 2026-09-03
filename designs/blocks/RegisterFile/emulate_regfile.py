#!/usr/bin/env python3
"""
emulate_regfile.py — True RTL simulation of the 256-entry, 10R/5W register file.

Runs:
  1. Functional correctness tests (write then read-back, constant ROM, etc.)
  2. Performance benchmark: 100K cycles of mixed read/write traffic.
"""
from __future__ import annotations

import ctypes
import sys
import time
from pathlib import Path

RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
CYAN = "\033[36m"

NR = 10
NW = 5
PTAG_COUNT = 256
CONST_COUNT = 128
MASK64 = (1 << 64) - 1


def const64(ptag: int) -> int:
    v = ptag & 0xFFFF_FFFF
    return ((v << 32) | v) & MASK64


class RegFileRTL:
    def __init__(self, lib_path: str | None = None):
        if lib_path is None:
            lib_path = str(Path(__file__).resolve().parent / "libregfile_sim.dylib")
        L = ctypes.CDLL(lib_path)

        L.rf_create.restype = ctypes.c_void_p
        L.rf_destroy.argtypes = [ctypes.c_void_p]
        L.rf_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        L.rf_drive_read.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint8]
        L.rf_drive_write.argtypes = [
            ctypes.c_void_p,
            ctypes.c_uint32,
            ctypes.c_uint8,
            ctypes.c_uint8,
            ctypes.c_uint64,
        ]
        L.rf_tick.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        L.rf_get_rdata.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        L.rf_get_rdata.restype = ctypes.c_uint64
        L.rf_get_cycle.argtypes = [ctypes.c_void_p]
        L.rf_get_cycle.restype = ctypes.c_uint64
        L.rf_run_bench.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        L.rf_run_bench_cd.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_uint32]

        self._L = L
        self._c = L.rf_create()

    def __del__(self):
        if hasattr(self, "_c") and self._c:
            self._L.rf_destroy(self._c)

    def reset(self):
        self._L.rf_reset(self._c, 2)

    def drive_read(self, lane: int, addr: int):
        self._L.rf_drive_read(self._c, lane, addr & 0xFF)

    def drive_write(self, lane: int, en: int, addr: int, data: int):
        self._L.rf_drive_write(self._c, lane, en & 1, addr & 0xFF, data & MASK64)

    def tick(self, n: int = 1):
        self._L.rf_tick(self._c, n)

    def get_rdata(self, lane: int) -> int:
        return self._L.rf_get_rdata(self._c, lane)

    @property
    def cycle(self) -> int:
        return self._L.rf_get_cycle(self._c)

    def run_bench(self, n_cycles: int):
        self._L.rf_run_bench(self._c, n_cycles)

    def run_bench_cd(self, n_cycles: int, active_pct: int = 100):
        self._L.rf_run_bench_cd(self._c, n_cycles, active_pct)


def test_functional(rf: RegFileRTL) -> tuple[int, int]:
    passed = 0
    failed = 0

    def check(desc: str, got: int, exp: int):
        nonlocal passed, failed
        if got == exp:
            passed += 1
        else:
            failed += 1
            print(f"  {RED}FAIL{RESET} {desc}: got=0x{got:016X} exp=0x{exp:016X}")

    rf.reset()

    # ── Test 1: constant ROM reads ──
    print(f"  {DIM}[T1]{RESET} Constant ROM reads (addr 0..9)...")
    for i in range(NR):
        rf.drive_read(i, i)
    rf.tick(1)
    for i in range(NR):
        check(f"const[{i}]", rf.get_rdata(i), const64(i))

    # ── Test 2: uninitialized data reads should be 0 ──
    print(f"  {DIM}[T2]{RESET} Uninitialized data reads (addr 128..137)...")
    for i in range(NR):
        rf.drive_read(i, CONST_COUNT + i)
    rf.tick(1)
    for i in range(NR):
        check(f"uninit[{CONST_COUNT + i}]", rf.get_rdata(i), 0)

    # ── Test 3: write then read-back ──
    print(f"  {DIM}[T3]{RESET} Write then read-back (5 entries)...")
    test_data = [
        (128, 0x1111222233334444),
        (129, 0x5555666677778888),
        (130, 0xDEADBEEFCAFEBABE),
        (200, 0x89ABCDEF01234567),
        (255, 0x0123456789ABCDEF),
    ]
    for lane, (addr, data) in enumerate(test_data):
        rf.drive_write(lane, 1, addr, data)
    rf.tick(1)
    # clear writes, set up reads
    for lane in range(NW):
        rf.drive_write(lane, 0, 0, 0)
    for i, (addr, _) in enumerate(test_data):
        rf.drive_read(i, addr)
    for i in range(len(test_data), NR):
        rf.drive_read(i, 0)
    rf.tick(1)
    for i, (addr, data) in enumerate(test_data):
        check(f"wb[{addr}]", rf.get_rdata(i), data)

    # ── Test 4: constant ROM writes are ignored ──
    print(f"  {DIM}[T4]{RESET} Writes to constant ROM are ignored...")
    rf.drive_write(0, 1, 7, 0xAAAAAAAAAAAAAAAA)
    rf.drive_write(1, 1, 127, 0xBBBBBBBBBBBBBBBB)
    for lane in range(2, NW):
        rf.drive_write(lane, 0, 0, 0)
    rf.tick(1)
    rf.drive_write(0, 0, 0, 0)
    rf.drive_write(1, 0, 0, 0)
    rf.drive_read(0, 7)
    rf.drive_read(1, 127)
    rf.tick(1)
    check("const[7] unchanged", rf.get_rdata(0), const64(7))
    check("const[127] unchanged", rf.get_rdata(1), const64(127))

    # ── Test 5: overwrite existing entries ──
    print(f"  {DIM}[T5]{RESET} Overwrite existing entries...")
    rf.drive_write(0, 1, 128, 0x0BADF00D0BADF00D)
    rf.drive_write(1, 1, 129, 0x0102030405060708)
    for lane in range(2, NW):
        rf.drive_write(lane, 0, 0, 0)
    rf.tick(1)
    for lane in range(NW):
        rf.drive_write(lane, 0, 0, 0)
    rf.drive_read(0, 128)
    rf.drive_read(1, 129)
    rf.tick(1)
    check("overwrite[128]", rf.get_rdata(0), 0x0BADF00D0BADF00D)
    check("overwrite[129]", rf.get_rdata(1), 0x0102030405060708)

    return passed, failed


def benchmark(rf: RegFileRTL, n_cycles: int) -> float:
    rf.reset()

    # warm up
    rf.run_bench(1000)

    # timed run
    t0 = time.perf_counter()
    rf.run_bench(n_cycles)
    t1 = time.perf_counter()
    return t1 - t0


def main():
    print(f"\n{BOLD}{CYAN}RegisterFile RTL Simulation{RESET}")
    print(
        f"  Config: {PTAG_COUNT} entries, {CONST_COUNT} constants, {NR}R/{NW}W, 64-bit data"
    )
    print(f"{'=' * 60}\n")

    rf = RegFileRTL()

    # ── Functional tests ──
    print(f"{BOLD}Functional Correctness Tests{RESET}")
    passed, failed = test_functional(rf)
    total = passed + failed
    if failed == 0:
        print(f"\n  {GREEN}{BOLD}ALL {total} checks PASSED{RESET}\n")
    else:
        print(f"\n  {RED}{BOLD}{failed}/{total} checks FAILED{RESET}\n")

    # ── Benchmark: 100% active ──
    N = 100_000
    print(f"{BOLD}Performance Benchmark ({N // 1000}K cycles, 100% active){RESET}")
    print("  Mixed random read/write traffic per cycle...")

    elapsed = benchmark(rf, N)
    khz = N / elapsed / 1000
    print(f"\n  Cycles:    {N:>12,}")
    print(f"  Elapsed:   {elapsed:>12.4f} s")
    print(f"  Throughput:{khz:>12.1f} Kcycles/s")
    print(f"  Per cycle: {elapsed / N * 1e6:>12.2f} us")

    # ── Benchmark: change-detection with varying activity rates ──
    print(f"\n{BOLD}Change-Detection Benchmark ({N // 1000}K cycles){RESET}")
    for pct in [100, 50, 25, 10, 1]:
        rf.reset()
        rf.run_bench_cd(1000, pct)  # warm up
        t0 = time.perf_counter()
        rf.run_bench_cd(N, pct)
        t1 = time.perf_counter()
        el = t1 - t0
        kc = N / el / 1000
        print(f"  {pct:3d}% active: {el:.4f}s  ({kc:.1f} Kcycles/s)")

    print(f"\n{GREEN}{BOLD}Done.{RESET}\n")

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()

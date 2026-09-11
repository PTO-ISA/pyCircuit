#!/usr/bin/env python3
import ctypes
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[3]
L = ctypes.CDLL(
    str(
        REPOSITORY
        / ".pycircuit_out"
        / "benchmarks"
        / "register_file"
        / "libinstr.dylib"
    )
)
L.rf_create.restype = ctypes.c_void_p
L.rf_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
L.rf_run_bench.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
L.rf_destroy.argtypes = [ctypes.c_void_p]
c = L.rf_create()
L.rf_reset(c, 2)
L.rf_run_bench(c, 10000)
L.rf_destroy(c)

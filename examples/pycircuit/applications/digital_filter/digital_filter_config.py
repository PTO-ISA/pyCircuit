from __future__ import annotations

DEFAULT_PARAMS = {
    "TAPS": 4,
    "DATA_W": 16,
    "COEFF_W": 16,
    "COEFFS": (1, 2, 3, 4),
}

TB_PRESETS = {
    "smoke": {"timeout": 64, "finish": 2},
    "nightly": {"timeout": 256, "finish": 8},
}

SIM_TIER = "normal"

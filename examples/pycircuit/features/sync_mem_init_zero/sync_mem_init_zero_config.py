from __future__ import annotations

DEFAULT_PARAMS = {
    "depth": 4,
    "data_width": 32,
    "addr_width": 2,
}

TB_PRESETS = {
    "smoke": {"timeout": 16, "finish": 2},
    "nightly": {"timeout": 128, "finish": 8},
}

SIM_TIER = "normal"

from __future__ import annotations

DEFAULT_PARAMS = {
    "depth": 4,
    "data_width": 32,
    "addr_width": 2,
}

TB_PRESETS = {
    "smoke": {"timeout": 64, "finish": 3},
    "nightly": {"timeout": 256, "finish": 10},
}

SIM_TIER = "normal"

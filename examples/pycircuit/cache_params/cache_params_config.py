from __future__ import annotations

DEFAULT_PARAMS = {
    "ways": 4,
    "sets": 64,
    "line_bytes": 64,
    "addr_width": 40,
    "data_width": 64,
}

TB_PRESETS = {
    "smoke": {"timeout": 64, "finish": 1},
    "nightly": {"timeout": 256, "finish": 16},
}

SIM_TIER = "normal"

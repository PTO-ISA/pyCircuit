from __future__ import annotations

DEFAULT_PARAMS = {"clk_freq": 50000000}

TB_PRESETS = {
    "smoke": {"timeout": 64, "finish": 1},
    "nightly": {"timeout": 256, "finish": 16},
}

SIM_TIER = "normal"

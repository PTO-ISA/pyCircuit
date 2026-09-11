from __future__ import annotations

DEFAULT_PARAMS = {"MAIN_CLK_BIT": 4}

TB_PRESETS = {
    "smoke": {"timeout": 32, "finish": 2},
    "nightly": {"timeout": 128, "finish": 16},
}

SIM_TIER = "heavy"

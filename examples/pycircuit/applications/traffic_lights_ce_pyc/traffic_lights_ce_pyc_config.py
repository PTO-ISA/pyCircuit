from __future__ import annotations

DEFAULT_PARAMS = {
    "CLK_FREQ": 4,
    "EW_GREEN_S": 3,
    "EW_YELLOW_S": 1,
    "NS_GREEN_S": 2,
    "NS_YELLOW_S": 1,
}

TB_PRESETS = {
    "smoke": {"timeout": 32, "finish": 4},
    "nightly": {"timeout": 128, "finish": 32},
}

SIM_TIER = "normal"

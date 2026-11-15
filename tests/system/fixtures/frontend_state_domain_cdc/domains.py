# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac
from checks.hierarchy import Hierarchy


@ac.system(
    samples=[
        {"clk_fast": 0, "clk_slow": 0, "rst_n": 1},
        {"clk_fast": 1, "clk_slow": 0, "rst_n": 1},
        {"clk_fast": 0, "clk_slow": 1, "rst_n": 1},
        {"clk_fast": 1, "clk_slow": 0, "rst_n": 1},
        {"clk_fast": 0, "clk_slow": 0, "rst_n": 0},
        {"clk_fast": 1, "clk_slow": 1, "rst_n": 0},
        {"clk_fast": 0, "clk_slow": 0, "rst_n": 1},
        {"clk_fast": 1, "clk_slow": 1, "rst_n": 1},
        {"clk_fast": 0, "clk_slow": 0, "rst_n": 1},
    ]
)
def Bench(rst_n: ac.u1):
    fast = ac.domain(clock="clk_fast")
    slow = ac.domain(clock="clk_slow")
    Hierarchy(rst_n, domain=fast, domains=[fast, slow], slot=1)

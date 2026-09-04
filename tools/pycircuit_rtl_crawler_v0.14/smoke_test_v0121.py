#!/usr/bin/env python3
from pathlib import Path
import tempfile
from check_qor_netlist import inspect
from run_timing_benchmark import residual_formal_constructs

def main():
    rs = Path("run_synthesis.py").read_text(encoding="utf-8")
    assert rs.count("chformal -remove") >= 2

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        bad = td/"bad.v"
        bad.write_text(
            """module x;
initial begin
  assert (1'b1);
end
endmodule
""", encoding="utf-8")
        assert inspect(bad)
        assert residual_formal_constructs(bad)

        good = td/"good.v"
        good.write_text(
            "module x(input a,b, output y); assign y=a&b; endmodule\n",
            encoding="utf-8")
        assert inspect(good) == []
        assert residual_formal_constructs(good) == []

    print("smoke_test_v0.12.1: PASS")

if __name__ == "__main__":
    main()

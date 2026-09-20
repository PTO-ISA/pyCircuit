// RUN: %not %pyc_opt -split-input-file %s 2>&1 | %FileCheck %s

module {
  func.func @missing_live_window(%clk: !pyc.clock, %rst: !pyc.reset,
      %ren: i1, %raddr: i2, %wvalid: i1, %waddr: i2, %wdata: i8,
      %wstrb: i1) -> i8 {
    // expected-error @+1 {{sync_mem requires the static aggressive live_window N=1}}
    %rdata = pyc.sync_mem %clk, %rst, %ren, %raddr, %wvalid, %waddr, %wdata, %wstrb {depth = 4, name = "mem"} : i2, i8, i1
    func.return %rdata : i8
  }
}

// CHECK: sync_mem requires the static aggressive live_window N=1

// -----

module {
  func.func @wrong_live_window(%clk: !pyc.clock, %rst: !pyc.reset,
      %ren0: i1, %raddr0: i2, %ren1: i1, %raddr1: i2, %wvalid: i1,
      %waddr: i2, %wdata: i8, %wstrb: i1) -> (i8, i8) {
    // expected-error @+1 {{sync_mem_dp requires the static aggressive live_window N=1}}
    %rdata0, %rdata1 = pyc.sync_mem_dp %clk, %rst, %ren0, %raddr0, %ren1, %raddr1, %wvalid, %waddr, %wdata, %wstrb {depth = 4, live_window = 2, name = "mem"} : i2, i8, i1
    func.return %rdata0, %rdata1 : i8, i8
  }
}

// CHECK: sync_mem_dp requires the static aggressive live_window N=1

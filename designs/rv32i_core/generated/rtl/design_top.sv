package pycircuit_types;
  function automatic logic signed [63:0] pyc_floordiv(input logic signed [63:0] a, b);
    return a/b - ((a%b != 0 && (a<0)!=(b<0)) ? 64'sd1 : 64'sd0);
  endfunction
  function automatic logic signed [63:0] pyc_mod(input logic signed [63:0] a, b);
    logic signed [63:0] r; r=a%b; return r != 0 && (r<0)!=(b<0) ? r+b : r;
  endfunction
typedef struct packed {
  logic [(1)-1:0] valid;
  logic [(32)-1:0] pc;
  logic [(32)-1:0] instruction;
  logic [(1)-1:0] fault;
} ac_rv32i_core_types_Fetch_t;
typedef struct packed {
  logic [(5)-1:0] rs1;
  logic [(5)-1:0] rs2;
  logic [(5)-1:0] rd;
  logic [(1)-1:0] use1;
  logic [(1)-1:0] use2;
  logic [(1)-1:0] writes;
  logic [(4)-1:0] operation;
  logic [(32)-1:0] immediate;
  logic [(1)-1:0] immediate_rhs;
  logic [(1)-1:0] load;
  logic [(1)-1:0] store;
  logic [(2)-1:0] size;
  logic [(1)-1:0] signed_load;
  logic [(1)-1:0] branch;
  logic [(3)-1:0] condition;
  logic [(2)-1:0] jump;
  logic [(1)-1:0] trap;
  logic [(4)-1:0] cause;
  logic [(32)-1:0] tval;
} ac_rv32i_core_types_Decoded_t;
typedef struct packed {
  logic [(1)-1:0] valid;
  logic [(32)-1:0] pc;
  logic [(32)-1:0] instruction;
  ac_rv32i_core_types_Decoded_t control;
  logic [(32)-1:0] first;
  logic [(32)-1:0] second;
} ac_rv32i_core_types_Execute_t;
typedef struct packed {
  logic [(1)-1:0] valid;
  logic [(32)-1:0] pc;
  logic [(32)-1:0] instruction;
  logic [(32)-1:0] next_pc;
  logic [(5)-1:0] rd;
  logic [(1)-1:0] writes;
  logic [(32)-1:0] result;
  logic [(1)-1:0] load;
  logic [(1)-1:0] store;
  logic [(32)-1:0] address;
  logic [(32)-1:0] store_data;
  logic [(32)-1:0] load_data;
  logic [(2)-1:0] size;
  logic [(1)-1:0] signed_load;
  logic [(1)-1:0] done;
  logic [(1)-1:0] redirect;
  logic [(1)-1:0] trap;
  logic [(4)-1:0] cause;
  logic [(32)-1:0] tval;
} ac_rv32i_core_types_Packet_t;
typedef struct packed {
  logic [(((1 * (32)) * (32)))-1:0] registers;
  logic [(32)-1:0] fetch_pc;
  logic [(32)-1:0] arch_pc;
  logic [(1)-1:0] fetch_busy;
  logic [(32)-1:0] fetch_request_pc;
  logic [(1)-1:0] fetch_killed;
  ac_rv32i_core_types_Fetch_t fetch_buffer;
  ac_rv32i_core_types_Fetch_t decode_stage;
  ac_rv32i_core_types_Execute_t execute_stage;
  ac_rv32i_core_types_Packet_t memory_stage;
  ac_rv32i_core_types_Packet_t writeback_stage;
  logic [(1)-1:0] stopping;
  logic [(1)-1:0] halted;
  logic [(64)-1:0] order;
} ac_rv32i_core_types_Machine_t;
typedef struct packed {
  logic [(1)-1:0] imem_valid;
  logic [(32)-1:0] imem_address;
  logic [(1)-1:0] dmem_valid;
  logic [(1)-1:0] dmem_write;
  logic [(32)-1:0] dmem_address;
  logic [(32)-1:0] dmem_data;
  logic [(4)-1:0] dmem_mask;
  logic [(1)-1:0] retire_valid;
  logic [(64)-1:0] retire_order;
  logic [(32)-1:0] retire_pc;
  logic [(32)-1:0] retire_instruction;
  logic [(32)-1:0] retire_next_pc;
  logic [(1)-1:0] retire_rd_write;
  logic [(5)-1:0] retire_rd;
  logic [(32)-1:0] retire_value;
  logic [(1)-1:0] retire_load;
  logic [(1)-1:0] retire_store;
  logic [(32)-1:0] retire_address;
  logic [(4)-1:0] retire_mask;
  logic [(32)-1:0] retire_mem_data;
  logic [(1)-1:0] retire_trap;
  logic [(4)-1:0] retire_cause;
  logic [(32)-1:0] retire_tval;
  logic [(1)-1:0] stage_if;
  logic [(1)-1:0] stage_id;
  logic [(1)-1:0] stage_ex;
  logic [(1)-1:0] stage_mem;
  logic [(1)-1:0] stage_wb;
  logic [(1)-1:0] load_use;
  logic [(1)-1:0] memory_wait;
  logic [(1)-1:0] redirected;
  logic [(1)-1:0] halted;
} ac_rv32i_core_types_Pins_t;
typedef struct packed {
  logic [(1)-1:0] moved;
  logic [(1)-1:0] kill;
  logic [(1)-1:0] redirected;
  logic [(1)-1:0] memory_fault;
} ac_rv32i_core_types_ExecuteControl_t;
typedef struct packed {
  ac_rv32i_core_types_Machine_t state;
  ac_rv32i_core_types_Pins_t io;
  ac_rv32i_core_types_ExecuteControl_t control;
} ac_rv32i_core_execute_ResultExecuteLogic_t;
typedef struct packed {
  ac_rv32i_core_types_Machine_t state;
  ac_rv32i_core_types_Pins_t io;
} ac_rv32i_core_frontend_ResultFrontend_t;
typedef struct packed {
  logic [(1)-1:0] fault;
  logic [(1)-1:0] space;
} ac_rv32i_core_types_MemoryControl_t;
typedef struct packed {
  ac_rv32i_core_types_Machine_t state;
  ac_rv32i_core_types_Pins_t io;
  ac_rv32i_core_types_Packet_t memory;
  ac_rv32i_core_types_MemoryControl_t control;
} ac_rv32i_core_memory_ResultMemory_t;
typedef struct packed {
  logic [(1)-1:0] store;
  logic [(1)-1:0] fault;
  logic [(1)-1:0] done;
  logic [(1)-1:0] space;
  logic [(1)-1:0] terminal;
} ac_rv32i_core_types_WritebackControl_t;
typedef struct packed {
  logic [(1)-1:0] imem_ready;
  logic [(32)-1:0] imem_data;
  logic [(1)-1:0] imem_fault;
  logic [(1)-1:0] dmem_ready;
  logic [(32)-1:0] dmem_data;
  logic [(1)-1:0] dmem_fault;
} ac_rv32i_core_types_BusInputs_t;
typedef struct packed {
  ac_rv32i_core_types_Machine_t state;
  ac_rv32i_core_types_Pins_t io;
  ac_rv32i_core_types_WritebackControl_t control;
} ac_rv32i_core_writeback_ResultWriteback_t;
endpackage
module pyc_root (
  input wire pycircuit_types::ac_rv32i_core_types_BusInputs_t bus,
  input wire logic [(1)-1:0] pyc_7079635f636c6b,
  input wire logic [(1)-1:0] pyc_7079635f727374,
  output wire pycircuit_types::ac_rv32i_core_types_Pins_t result_0,
  output wire pycircuit_types::ac_rv32i_core_types_Machine_t result_1
);
  wire pyc_local_error;
  wire pyc_root_commit_ok;
`ifndef SYNTHESIS
  bit pyc_physical_fault = 1'b0;
  wire pyc_physical_error = pyc_local_error | $isunknown(pyc_7079635f727374);
  assign pyc_root_commit_ok = !pyc_physical_fault && !pyc_physical_error;
  always @(pyc_7079635f636c6b) begin
    if ($isunknown(pyc_7079635f636c6b) || ((pyc_7079635f636c6b === 1'b1) && pyc_physical_error)) begin
      if (!pyc_physical_fault) $display("PYCIRCUIT_PHYSICAL_FAULT");
      pyc_physical_fault = 1'b1;
    end
  end
`else
  assign pyc_root_commit_ok = 1'b1;
`endif
  ac_rv32i_core_core_Core dut (
    .bus(bus),
    .pyc_7079635f636c6b(pyc_7079635f636c6b),
    .pyc_7079635f727374(pyc_7079635f727374),
    .result_0(result_0),
    .result_1(result_1),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_7079635f727374),
    .pyc_local_error(pyc_local_error)
  );
endmodule

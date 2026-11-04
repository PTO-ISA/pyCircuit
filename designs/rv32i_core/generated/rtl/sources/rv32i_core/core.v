module ac_rv32i_core_core_Core (
  input wire pycircuit_types::ac_rv32i_core_types_BusInputs_t bus,
  input wire logic [(1)-1:0] pyc_7079635f636c6b,
  input wire logic [(1)-1:0] pyc_7079635f727374,
  output wire pycircuit_types::ac_rv32i_core_types_Pins_t result_0,
  output wire pycircuit_types::ac_rv32i_core_types_Machine_t result_1,
  input wire pyc_root_commit_ok,
  input wire pyc_reset_active,
  output wire pyc_local_error
);
  wire pycircuit_types::ac_rv32i_core_types_Pins_t pyc_net_0;
  assign result_0 = pyc_net_0;
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_1;
  assign result_1 = pyc_net_1;
  wire logic [(1)-1:0] pyc_net_2;
  assign pyc_net_2 = (1)'( 1'd1 );
  wire logic [(1)-1:0] pyc_net_3;
  assign pyc_net_3 = (1)'( 1'd1 );
`ifndef SYNTHESIS
  wire pyc_check_error_0 = $isunknown(pyc_net_3) || ((pyc_reset_active === 1'b0) && (pyc_net_3 === 1'b1) && $isunknown(pyc_net_2));
`else
  wire pyc_check_error_0 = 1'b0;
`endif
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_4;
  wire pycircuit_types::ac_rv32i_core_types_Pins_t pyc_net_5;
  wire pycircuit_types::ac_rv32i_core_types_WritebackControl_t pyc_net_6;
  wire pyc_child_error_1;
  ac_rv32i_core_writeback_Writeback pyc_instance_pyc_5f5f7079635f63616c6c5f30 (
    .old(pyc_net_1),
    .bus(bus),
    .result_0(pyc_net_4),
    .result_1(pyc_net_5),
    .result_2(pyc_net_6),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_reset_active),
    .pyc_local_error(pyc_child_error_1)
  );
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_7;
  wire pycircuit_types::ac_rv32i_core_types_Pins_t pyc_net_8;
  wire pycircuit_types::ac_rv32i_core_types_Packet_t pyc_net_9;
  wire pycircuit_types::ac_rv32i_core_types_MemoryControl_t pyc_net_10;
  wire pyc_child_error_2;
  ac_rv32i_core_memory_Memory pyc_instance_pyc_5f5f7079635f63616c6c5f31 (
    .old(pyc_net_1),
    .prior(pyc_net_4),
    .original_io(pyc_net_5),
    .wb_control(pyc_net_6),
    .bus(bus),
    .result_0(pyc_net_7),
    .result_1(pyc_net_8),
    .result_2(pyc_net_9),
    .result_3(pyc_net_10),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_reset_active),
    .pyc_local_error(pyc_child_error_2)
  );
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_11;
  wire pycircuit_types::ac_rv32i_core_types_Pins_t pyc_net_12;
  wire pycircuit_types::ac_rv32i_core_types_ExecuteControl_t pyc_net_13;
  wire pyc_child_error_3;
  ac_rv32i_core_execute_ExecuteLogic pyc_instance_pyc_5f5f7079635f63616c6c5f32 (
    .old(pyc_net_1),
    .prior(pyc_net_7),
    .original_io(pyc_net_8),
    .wb_control(pyc_net_6),
    .mem(pyc_net_9),
    .mem_control(pyc_net_10),
    .result_0(pyc_net_11),
    .result_1(pyc_net_12),
    .result_2(pyc_net_13),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_reset_active),
    .pyc_local_error(pyc_child_error_3)
  );
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_14;
  wire pycircuit_types::ac_rv32i_core_types_Pins_t pyc_net_15;
  wire pyc_child_error_4;
  ac_rv32i_core_frontend_Frontend pyc_instance_pyc_5f5f7079635f63616c6c5f33 (
    .old(pyc_net_1),
    .prior(pyc_net_11),
    .original_io(pyc_net_12),
    .ex_control(pyc_net_13),
    .bus(bus),
    .result_0(pyc_net_14),
    .result_1(pyc_net_15),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_reset_active),
    .pyc_local_error(pyc_child_error_4)
  );
  wire logic [(1)-1:0] pyc_net_16;
  assign pyc_net_16 = pyc_net_6.terminal;
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_17;
  wire pyc_child_error_5;
  ac_rv32i_core_frontend_Finalize pyc_instance_pyc_5f5f7079635f63616c6c5f34 (
    .old(pyc_net_1),
    .stopped(pyc_net_4),
    .stopped_io(pyc_net_5),
    .active(pyc_net_14),
    .active_io(pyc_net_15),
    .terminal(pyc_net_16),
    .result_0(pyc_net_17),
    .result_1(pyc_net_0),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_reset_active(pyc_reset_active),
    .pyc_local_error(pyc_child_error_5)
  );
  wire pycircuit_types::ac_rv32i_core_types_Machine_t pyc_net_18;
  wire logic [(((1 * (32)) * (32)))-1:0] pyc_net_19;
  wire logic [(32)-1:0] pyc_net_20;
  assign pyc_net_20 = (32)'( 1'd0 );
  for (genvar pyc_index_21=0; pyc_index_21 < ((1 * (32))); ++pyc_index_21) begin : pyc_broadcast_pyc_net_19
    assign pyc_net_19[(((1 * (32)))-1-pyc_index_21)*(32) +: (32)] = pyc_net_20;
  end
  wire logic [(32)-1:0] pyc_net_22;
  assign pyc_net_22 = (32)'( 32'd2147483648 );
  wire logic [(32)-1:0] pyc_net_23;
  assign pyc_net_23 = (32)'( 32'd2147483648 );
  wire logic [(1)-1:0] pyc_net_24;
  assign pyc_net_24 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_25;
  assign pyc_net_25 = (32)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_26;
  assign pyc_net_26 = (1)'( 1'd0 );
  wire pycircuit_types::ac_rv32i_core_types_Fetch_t pyc_net_27;
  wire logic [(1)-1:0] pyc_net_28;
  assign pyc_net_28 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_29;
  assign pyc_net_29 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_30;
  assign pyc_net_30 = (32)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_31;
  assign pyc_net_31 = (1)'( 1'd0 );
  assign pyc_net_27 = {pyc_net_28, pyc_net_29, pyc_net_30, pyc_net_31};
  wire pycircuit_types::ac_rv32i_core_types_Fetch_t pyc_net_32;
  assign pyc_net_32 = {pyc_net_28, pyc_net_29, pyc_net_30, pyc_net_31};
  wire pycircuit_types::ac_rv32i_core_types_Execute_t pyc_net_33;
  wire logic [(1)-1:0] pyc_net_34;
  assign pyc_net_34 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_35;
  assign pyc_net_35 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_36;
  assign pyc_net_36 = (32)'( 1'd0 );
  wire pycircuit_types::ac_rv32i_core_types_Decoded_t pyc_net_37;
  wire logic [(5)-1:0] pyc_net_38;
  assign pyc_net_38 = (5)'( 1'd0 );
  wire logic [(5)-1:0] pyc_net_39;
  assign pyc_net_39 = (5)'( 1'd0 );
  wire logic [(5)-1:0] pyc_net_40;
  assign pyc_net_40 = (5)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_41;
  assign pyc_net_41 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_42;
  assign pyc_net_42 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_43;
  assign pyc_net_43 = (1)'( 1'd0 );
  wire logic [(4)-1:0] pyc_net_44;
  assign pyc_net_44 = (4)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_45;
  assign pyc_net_45 = (32)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_46;
  assign pyc_net_46 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_47;
  assign pyc_net_47 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_48;
  assign pyc_net_48 = (1)'( 1'd0 );
  wire logic [(2)-1:0] pyc_net_49;
  assign pyc_net_49 = (2)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_50;
  assign pyc_net_50 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_51;
  assign pyc_net_51 = (1)'( 1'd0 );
  wire logic [(3)-1:0] pyc_net_52;
  assign pyc_net_52 = (3)'( 1'd0 );
  wire logic [(2)-1:0] pyc_net_53;
  assign pyc_net_53 = (2)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_54;
  assign pyc_net_54 = (1)'( 1'd0 );
  wire logic [(4)-1:0] pyc_net_55;
  assign pyc_net_55 = (4)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_56;
  assign pyc_net_56 = (32)'( 1'd0 );
  assign pyc_net_37 = {pyc_net_38, pyc_net_39, pyc_net_40, pyc_net_41, pyc_net_42, pyc_net_43, pyc_net_44, pyc_net_45, pyc_net_46, pyc_net_47, pyc_net_48, pyc_net_49, pyc_net_50, pyc_net_51, pyc_net_52, pyc_net_53, pyc_net_54, pyc_net_55, pyc_net_56};
  wire logic [(32)-1:0] pyc_net_57;
  assign pyc_net_57 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_58;
  assign pyc_net_58 = (32)'( 1'd0 );
  assign pyc_net_33 = {pyc_net_34, pyc_net_35, pyc_net_36, pyc_net_37, pyc_net_57, pyc_net_58};
  wire pycircuit_types::ac_rv32i_core_types_Packet_t pyc_net_59;
  wire logic [(1)-1:0] pyc_net_60;
  assign pyc_net_60 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_61;
  assign pyc_net_61 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_62;
  assign pyc_net_62 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_63;
  assign pyc_net_63 = (32)'( 1'd0 );
  wire logic [(5)-1:0] pyc_net_64;
  assign pyc_net_64 = (5)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_65;
  assign pyc_net_65 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_66;
  assign pyc_net_66 = (32)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_67;
  assign pyc_net_67 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_68;
  assign pyc_net_68 = (1)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_69;
  assign pyc_net_69 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_70;
  assign pyc_net_70 = (32)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_71;
  assign pyc_net_71 = (32)'( 1'd0 );
  wire logic [(2)-1:0] pyc_net_72;
  assign pyc_net_72 = (2)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_73;
  assign pyc_net_73 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_74;
  assign pyc_net_74 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_75;
  assign pyc_net_75 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_76;
  assign pyc_net_76 = (1)'( 1'd0 );
  wire logic [(4)-1:0] pyc_net_77;
  assign pyc_net_77 = (4)'( 1'd0 );
  wire logic [(32)-1:0] pyc_net_78;
  assign pyc_net_78 = (32)'( 1'd0 );
  assign pyc_net_59 = {pyc_net_60, pyc_net_61, pyc_net_62, pyc_net_63, pyc_net_64, pyc_net_65, pyc_net_66, pyc_net_67, pyc_net_68, pyc_net_69, pyc_net_70, pyc_net_71, pyc_net_72, pyc_net_73, pyc_net_74, pyc_net_75, pyc_net_76, pyc_net_77, pyc_net_78};
  wire pycircuit_types::ac_rv32i_core_types_Packet_t pyc_net_79;
  assign pyc_net_79 = {pyc_net_60, pyc_net_61, pyc_net_62, pyc_net_63, pyc_net_64, pyc_net_65, pyc_net_66, pyc_net_67, pyc_net_68, pyc_net_69, pyc_net_70, pyc_net_71, pyc_net_72, pyc_net_73, pyc_net_74, pyc_net_75, pyc_net_76, pyc_net_77, pyc_net_78};
  wire logic [(1)-1:0] pyc_net_80;
  assign pyc_net_80 = (1)'( 1'd0 );
  wire logic [(1)-1:0] pyc_net_81;
  assign pyc_net_81 = (1)'( 1'd0 );
  wire logic [(64)-1:0] pyc_net_82;
  assign pyc_net_82 = (64)'( 1'd0 );
  assign pyc_net_18 = {pyc_net_19, pyc_net_22, pyc_net_23, pyc_net_24, pyc_net_25, pyc_net_26, pyc_net_27, pyc_net_32, pyc_net_33, pyc_net_59, pyc_net_79, pyc_net_80, pyc_net_81, pyc_net_82};
  localparam logic [(32)-1:0] pyc_initial_0 = (32)'(1'd0);
  localparam logic [(((1 * (32)) * (32)))-1:0] pyc_initial_1 = {((1 * (32))){pyc_initial_0}};
  localparam logic [(32)-1:0] pyc_initial_2 = (32)'(32'd2147483648);
  localparam logic [(32)-1:0] pyc_initial_3 = (32)'(32'd2147483648);
  localparam logic [(1)-1:0] pyc_initial_4 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_5 = (32)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_6 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_7 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_8 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_9 = (32)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_10 = (1)'(1'd0);
  localparam pycircuit_types::ac_rv32i_core_types_Fetch_t pyc_initial_11 = {pyc_initial_7, pyc_initial_8, pyc_initial_9, pyc_initial_10};
  localparam pycircuit_types::ac_rv32i_core_types_Fetch_t pyc_initial_12 = {pyc_initial_7, pyc_initial_8, pyc_initial_9, pyc_initial_10};
  localparam logic [(1)-1:0] pyc_initial_13 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_14 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_15 = (32)'(1'd0);
  localparam logic [(5)-1:0] pyc_initial_16 = (5)'(1'd0);
  localparam logic [(5)-1:0] pyc_initial_17 = (5)'(1'd0);
  localparam logic [(5)-1:0] pyc_initial_18 = (5)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_19 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_20 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_21 = (1)'(1'd0);
  localparam logic [(4)-1:0] pyc_initial_22 = (4)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_23 = (32)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_24 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_25 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_26 = (1)'(1'd0);
  localparam logic [(2)-1:0] pyc_initial_27 = (2)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_28 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_29 = (1)'(1'd0);
  localparam logic [(3)-1:0] pyc_initial_30 = (3)'(1'd0);
  localparam logic [(2)-1:0] pyc_initial_31 = (2)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_32 = (1)'(1'd0);
  localparam logic [(4)-1:0] pyc_initial_33 = (4)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_34 = (32)'(1'd0);
  localparam pycircuit_types::ac_rv32i_core_types_Decoded_t pyc_initial_35 = {pyc_initial_16, pyc_initial_17, pyc_initial_18, pyc_initial_19, pyc_initial_20, pyc_initial_21, pyc_initial_22, pyc_initial_23, pyc_initial_24, pyc_initial_25, pyc_initial_26, pyc_initial_27, pyc_initial_28, pyc_initial_29, pyc_initial_30, pyc_initial_31, pyc_initial_32, pyc_initial_33, pyc_initial_34};
  localparam logic [(32)-1:0] pyc_initial_36 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_37 = (32)'(1'd0);
  localparam pycircuit_types::ac_rv32i_core_types_Execute_t pyc_initial_38 = {pyc_initial_13, pyc_initial_14, pyc_initial_15, pyc_initial_35, pyc_initial_36, pyc_initial_37};
  localparam logic [(1)-1:0] pyc_initial_39 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_40 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_41 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_42 = (32)'(1'd0);
  localparam logic [(5)-1:0] pyc_initial_43 = (5)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_44 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_45 = (32)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_46 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_47 = (1)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_48 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_49 = (32)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_50 = (32)'(1'd0);
  localparam logic [(2)-1:0] pyc_initial_51 = (2)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_52 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_53 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_54 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_55 = (1)'(1'd0);
  localparam logic [(4)-1:0] pyc_initial_56 = (4)'(1'd0);
  localparam logic [(32)-1:0] pyc_initial_57 = (32)'(1'd0);
  localparam pycircuit_types::ac_rv32i_core_types_Packet_t pyc_initial_58 = {pyc_initial_39, pyc_initial_40, pyc_initial_41, pyc_initial_42, pyc_initial_43, pyc_initial_44, pyc_initial_45, pyc_initial_46, pyc_initial_47, pyc_initial_48, pyc_initial_49, pyc_initial_50, pyc_initial_51, pyc_initial_52, pyc_initial_53, pyc_initial_54, pyc_initial_55, pyc_initial_56, pyc_initial_57};
  localparam pycircuit_types::ac_rv32i_core_types_Packet_t pyc_initial_59 = {pyc_initial_39, pyc_initial_40, pyc_initial_41, pyc_initial_42, pyc_initial_43, pyc_initial_44, pyc_initial_45, pyc_initial_46, pyc_initial_47, pyc_initial_48, pyc_initial_49, pyc_initial_50, pyc_initial_51, pyc_initial_52, pyc_initial_53, pyc_initial_54, pyc_initial_55, pyc_initial_56, pyc_initial_57};
  localparam logic [(1)-1:0] pyc_initial_60 = (1)'(1'd0);
  localparam logic [(1)-1:0] pyc_initial_61 = (1)'(1'd0);
  localparam logic [(64)-1:0] pyc_initial_62 = (64)'(1'd0);
  localparam pycircuit_types::ac_rv32i_core_types_Machine_t pyc_initial_63 = {pyc_initial_1, pyc_initial_2, pyc_initial_3, pyc_initial_4, pyc_initial_5, pyc_initial_6, pyc_initial_11, pyc_initial_12, pyc_initial_38, pyc_initial_58, pyc_initial_59, pyc_initial_60, pyc_initial_61, pyc_initial_62};
  wire pyc_child_error_6;
  dff #( .T(pycircuit_types::ac_rv32i_core_types_Machine_t) , .pyc_physical(1'b1) , .pyc_initial(pyc_initial_63) ) pyc_instance_state (
    .clk(pyc_7079635f636c6b),
    .rst(pyc_7079635f727374),
    .d(pyc_net_17),
    .init(pyc_net_18),
    .q(pyc_net_1),
    .pyc_root_commit_ok(pyc_root_commit_ok),
    .pyc_local_error(pyc_child_error_6)
  );
  assign pyc_local_error = pyc_check_error_0 | pyc_child_error_1 | pyc_child_error_2 | pyc_child_error_3 | pyc_child_error_4 | pyc_child_error_5 | pyc_child_error_6;
endmodule

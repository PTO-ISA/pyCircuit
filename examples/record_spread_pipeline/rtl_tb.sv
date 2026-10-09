module tb;
  logic pyc_7079635f636c6b = 0, pyc_7079635f727374 = 1;
  logic base_valid = 0, header_valid = 0, payload_valid = 0, patch_valid = 0;
  logic [24:0] base = 0;
  logic [7:0] header = 0;
  logic [15:0] payload = 0;
  logic [4:0] patch = 0;
  logic take = 0;
  wire [29:0] result;
  wire base_ready = result[29], header_ready = result[28];
  wire payload_ready = result[27], patch_ready = result[26];
  wire out_valid = result[25];
  wire [24:0] out_data = result[24:0];

  logic base_q_valid = 0, header_q_valid = 0, payload_q_valid = 0;
  logic patch_q_valid = 0, composed_q_valid = 0, updated_q_valid = 0;
  logic [24:0] base_q = 0, composed_q = 0, updated_q = 0;
  logic [7:0] header_q = 0;
  logic [15:0] payload_q = 0;
  logic [4:0] patch_q = 0;
  bit last_clock = 0;
  integer row_index = 0, stalled = 0;
  integer accepted = 0, retired = 0, dropped = 0, peak_slots = 0;

  pyc_root dut(.*);

  function automatic logic [24:0] composed_value(
      input logic [7:0] h, input logic [15:0] p);
    return {h[7:4], h[3:0], p, 1'b1};
  endfunction
  function automatic logic [24:0] updated_value(
      input logic [24:0] packet, input logic [4:0] patch_value);
    return {packet[24:21], patch_value[4:1], packet[16:1], patch_value[0]};
  endfunction
  function automatic integer slots;
    return base_q_valid + header_q_valid + payload_q_valid + patch_q_valid +
           composed_q_valid + updated_q_valid;
  endfunction
  function automatic integer components;
    return base_q_valid + header_q_valid + payload_q_valid + patch_q_valid +
           3 * composed_q_valid + 4 * updated_q_valid;
  endfunction

  task row(input bit clock_level, reset_level,
           input logic [3:0] valid_levels, input logic [24:0] base_value,
           input logic [7:0] header_value, input logic [15:0] payload_value,
           input logic [4:0] patch_value, input bit take_level);
    bit pop_updated, ready_updated, fire_apply, ready_composed, fire_compose;
    bit expected_base_ready, expected_header_ready, expected_payload_ready;
    bit expected_patch_ready;
    logic [7:0] old_header;
    logic [15:0] old_payload;
    logic [24:0] old_composed;
    logic [4:0] old_patch;
    pop_updated = updated_q_valid && take_level;
    ready_updated = !updated_q_valid || pop_updated;
    fire_apply = composed_q_valid && patch_q_valid && ready_updated;
    ready_composed = !composed_q_valid || fire_apply;
    fire_compose = base_q_valid && header_q_valid && payload_q_valid &&
                   ready_composed;
    expected_base_ready = !base_q_valid || fire_compose;
    expected_header_ready = !header_q_valid || fire_compose;
    expected_payload_ready = !payload_q_valid || fire_compose;
    expected_patch_ready = !patch_q_valid || fire_apply;
    pyc_7079635f727374 = reset_level;
    {base_valid, header_valid, payload_valid, patch_valid} = valid_levels;
    base = base_value;
    header = header_value;
    payload = payload_value;
    patch = patch_value;
    take = take_level;
    #1;
    if (result !== {expected_base_ready, expected_header_ready,
                    expected_payload_ready, expected_patch_ready,
                    updated_q_valid, updated_q_valid ? updated_q : 25'b0})
      $fatal(1, "record_spread_pipeline six-slot oracle failed");
    $display("WORK %0d %0d%0d%0d%0d %0d %025b", row_index,
             expected_base_ready, expected_header_ready,
             expected_payload_ready, expected_patch_ready, updated_q_valid,
             updated_q_valid ? updated_q : 25'b0);
    stalled = stalled + (valid_levels[3] && !expected_base_ready);
    if (clock_level && !last_clock) begin
      if (reset_level) begin
        dropped = dropped + components();
        base_q_valid = 0;
        header_q_valid = 0;
        payload_q_valid = 0;
        patch_q_valid = 0;
        composed_q_valid = 0;
        updated_q_valid = 0;
      end else begin
        if (updated_q_valid && take_level) retired = retired + 1;
        accepted = accepted + (base_ready && base_valid) +
                   (header_ready && header_valid) +
                   (payload_ready && payload_valid) +
                   (patch_ready && patch_valid);
        old_header = header_q;
        old_payload = payload_q;
        old_composed = composed_q;
        old_patch = patch_q;
        if (fire_apply) begin
          updated_q_valid = 1;
          updated_q = updated_value(old_composed, old_patch);
        end else if (pop_updated) begin
          updated_q_valid = 0;
          updated_q = 0;
        end
        if (fire_compose) begin
          composed_q_valid = 1;
          composed_q = composed_value(old_header, old_payload);
        end else if (fire_apply) begin
          composed_q_valid = 0;
          composed_q = 0;
        end
        if (base_valid && expected_base_ready) begin
          base_q_valid = 1;
          base_q = base_value;
        end else if (fire_compose) base_q_valid = 0;
        if (header_valid && expected_header_ready) begin
          header_q_valid = 1;
          header_q = header_value;
        end else if (fire_compose) header_q_valid = 0;
        if (payload_valid && expected_payload_ready) begin
          payload_q_valid = 1;
          payload_q = payload_value;
        end else if (fire_compose) payload_q_valid = 0;
        if (patch_valid && expected_patch_ready) begin
          patch_q_valid = 1;
          patch_q = patch_value;
        end else if (fire_apply) patch_q_valid = 0;
      end
      if (slots() > peak_slots) peak_slots = slots();
      if (slots() > 6) $fatal(1, "record_spread slot capacity exceeded");
    end
    last_clock = clock_level;
    pyc_7079635f636c6b = clock_level;
    #1;
    row_index = row_index + 1;
  endtask

  task capture_edge(input logic [3:0] valid_levels,
                    input logic [24:0] base_value,
                    input logic [7:0] header_value,
                    input logic [15:0] payload_value,
                    input logic [4:0] patch_value,
                    input bit take_level = 1, input bit reset_level = 0);
    row(1, reset_level, valid_levels, base_value, header_value, payload_value,
        patch_value, take_level);
    row(0, reset_level, valid_levels, base_value, header_value, payload_value,
        patch_value, take_level);
  endtask

`ifdef PYC_RECORD_SPREAD_FOUR_STATE
  task raw_edge;
    pyc_7079635f636c6b = 1; #1; pyc_7079635f636c6b = 0; #1;
  endtask
  task four_case(input integer ordinal, input logic [24:0] bv,
                 input logic [7:0] hv, input logic [15:0] pv,
                 input logic [4:0] patchv, input logic [24:0] expected);
    pyc_7079635f727374 = 1;
    {base_valid, header_valid, payload_valid, patch_valid} = 0;
    take = 0;
    raw_edge();
    pyc_7079635f727374 = 0;
    {base_valid, header_valid, payload_valid, patch_valid} = 4'b1111;
    base = bv; header = hv; payload = pv; patch = patchv; take = 1;
    raw_edge();
    {base_valid, header_valid, payload_valid, patch_valid} = 0;
    raw_edge();
    raw_edge();
    take = 0; #1;
    if (out_valid !== 1'b1 || out_data !== expected)
      $fatal(1, "record_spread full-DUT X/Z case %0d failed", ordinal);
    $display("FOUR %0d %025b", ordinal, out_data);
  endtask
`endif

  logic [24:0] bases [0:4];
  logic [7:0] headers [0:4];
  logic [15:0] payloads [0:4];
  logic [4:0] patches [0:4];
  logic [3:0] skew_peers, skew_only;
  initial begin
    bases[0]=0;headers[0]=0;payloads[0]=0;patches[0]=0;
    bases[1]=25'h1ffffff;headers[1]=8'hff;payloads[1]=16'hffff;patches[1]=5'h1f;
    bases[2]={4'hf,4'h8,16'h8000,1'b0};headers[2]=8'hf0;payloads[2]=16'h8000;patches[2]=5'h1e;
    bases[3]={4'h5,4'ha,16'h5aa5,1'b1};headers[3]=8'h5a;payloads[3]=16'h5aa5;patches[3]=5'h0b;
    bases[4]={4'h8,4'h1,16'h0001,1'b0};headers[4]=8'h81;payloads[4]=16'h0001;patches[4]=5'h10;
    #1;pyc_7079635f636c6b=1;#1;pyc_7079635f636c6b=0;pyc_7079635f727374=0;#1;
    row(0,1,0,0,0,0,0,0);
    capture_edge(0,0,0,0,0,0,1);
    capture_edge(4'b0111,bases[0],headers[0],payloads[0],patches[0]);
    capture_edge(4'b0111,bases[1],headers[1],payloads[1],patches[1]);
    capture_edge(4'b1000,bases[0],headers[0],payloads[0],patches[0]);
    repeat(3)capture_edge(0,0,0,0,0);
    capture_edge(0,0,0,0,0,0,1);
    for(integer missing=1;missing<4;missing=missing+1)begin
      skew_peers=4'b1111;skew_peers[3-missing]=0;
      skew_only=0;skew_only[3-missing]=1;
      capture_edge(skew_peers,bases[missing],headers[missing],payloads[missing],patches[missing]);
      capture_edge(skew_peers,bases[missing+1],headers[missing+1],payloads[missing+1],patches[missing+1]);
      capture_edge(skew_only,bases[missing],headers[missing],payloads[missing],patches[missing]);
      repeat(3)capture_edge(0,0,0,0,0);
      capture_edge(0,0,0,0,0,0,1);
    end
    capture_edge(4'b1111,bases[0],headers[0],payloads[0],patches[0],0);
    capture_edge(4'b1111,bases[1],headers[1],payloads[1],patches[0],0);
    capture_edge(4'b1111,bases[2],headers[2],payloads[2],patches[1],0);
    row(1,0,4'b1111,bases[3],headers[3],payloads[3],patches[3],0);
    row(1,0,4'b1111,bases[4],headers[4],payloads[4],patches[4],1);
    row(0,0,4'b1111,bases[3],headers[3],payloads[3],patches[3],1);
    row(0,0,4'b1111,bases[4],headers[4],payloads[4],patches[4],1);
    capture_edge(4'b1111,bases[3],headers[3],payloads[3],patches[2]);
    capture_edge(4'b1111,bases[4],headers[4],payloads[4],patches[3]);
    capture_edge(0,0,0,0,0,1,1);
    for(integer n=0;n<30;n=n+1)begin
      logic [24:0] bv;logic [7:0] hv;logic [15:0] pv;logic [4:0] patchv;
      if(n<5)begin bv=bases[n];hv=headers[n];pv=payloads[n];patchv=patches[n];end
      else begin bv=25'b1<<(n-5);hv=8'b1<<((n-5)%8);pv=16'b1<<((n-5)%16);patchv=5'b1<<((n-5)%5);end
      capture_edge(4'b1111,bv,hv,pv,patchv);
      repeat(3)capture_edge(0,0,0,0,0);
    end
    capture_edge(0,0,0,0,0,1,1);
    if(row_index!=317 || peak_slots!=6 || accepted!=4*retired+dropped+components())
      $fatal(1,"record_spread finite history failed");
    $display("HISTORY_RTL %0d %0d %0d %0d %0d",accepted,retired,dropped,components(),peak_slots);
`ifdef PYC_RECORD_SPREAD_FOUR_STATE
    four_case(0,25'bzzzzzzzzzzzzzzzzzzzzzzzzz,8'b1x0zzzzz,16'b10xz10xz10xz10xz,5'bzx100,25'b1x0zzx1010xz10xz10xz10xz0);
    four_case(1,25'bxxxxxxxxxxxxxxxxxxxxxxxxx,8'bzx10xzxz,16'bzxzxzxzxzxzxzxzx,5'b01xzz,25'bzx1001xzzxzxzxzxzxzxzxzxz);
    four_case(2,25'b10100011zzzzzzzzzzzzzzzzx,8'bx0z10000,16'bxxxxxxxxxxxxxxxx,5'b11111,25'bx0z11111xxxxxxxxxxxxxxxx1);
    four_case(3,25'b0000000000000000000000000,8'b0z1x1111,16'bzzzzzzzzzzzzzzzz,5'bx0z1x,25'b0z1xx0z1zzzzzzzzzzzzzzzzx);
`endif
    $finish;
  end
endmodule

#!/usr/bin/env python3
"""Regenerate 32-entry ROB and issue queues in model.mlir."""

from __future__ import annotations

from pathlib import Path

N = 32
FULL_MASK = (1 << N) - 1
MODEL = Path(__file__).with_name("model.mlir")


def i64(value: int) -> str:
    value = int(value) & ((1 << 64) - 1)
    if value >= 1 << 63:
        value -= 1 << 64
    return str(value)


def queue(name: str) -> str:
    return (
        f'    ac.queue @{name} payload i64 entries 1 ordering "fifo" protocol @rv\n'
        f'        ownership "exclusive" id "{name}" path "{name}" '
        f'watermarks {{kind = "register"}}'
    )


def replace_module(src: str, name: str, body: str) -> str:
    marker = f"  ac.module @{name}()"
    start = src.find(marker)
    if start < 0:
        raise SystemExit(f"missing module {name}")
    nxt = src.find("\n  ac.module @", start + 1)
    if nxt < 0:
        raise SystemExit(f"no module after {name}")
    return src[:start] + body.rstrip() + "\n\n" + src[nxt + 1 :]


def emit_rob() -> str:
    lines: list[str] = [
        "  ac.module @ROB() parameters {} graph {",
        queue("head"),
        queue("tail"),
    ]
    for i in range(N):
        lines.append(queue(f"done{i}"))
    lines += [
        '    ac.stat @retired kind "counter"',
        '    ac.stat @opcode_0 kind "counter"',
        '    ac.stat @opcode_1 kind "counter"',
        '    ac.stat @opcode_2 kind "counter"',
        '    ac.stat @opcode_3 kind "counter"',
        '    ac.stat @opcode_4 kind "counter"',
        '    ac.stat @opcode_5 kind "counter"',
        '    ac.stat @opcode_vec kind "counter"',
        '    ac.stat @scalar kind "counter"',
        '    ac.stat @vector kind "counter"',
        '    ac.stat @cube kind "counter"',
        '    ac.stat @tlsu kind "counter"',
        '    ac.process @step kind "control" {',
    ]
    for i in range(N + 1):
        lines.append(f"      %c{i} = arith.constant {i} : i64")
    lines += [
        "      %c255 = arith.constant 255 : i64",
        "      %mask31 = arith.constant 31 : i64",
        "      %true = arith.constant true",
        "      %head, %head_valid = ac.try_recv @head : i64",
        "      %tail, %tail_valid = ac.try_recv @tail : i64",
    ]
    for i in range(N):
        lines.append(f"      %d{i}, %dv{i} = ac.try_recv @done{i} : i64")
    lines += [
        "      %completed, %has_completed = ac.try_recv @Core::@rob_done : i64",
        "      %completed_desc = ac.trace.decode %completed : i64 to i64",
        "      %completed_seq = arith.andi %completed_desc, %c255 : i64",
        "      %completed_slot_base = arith.andi %completed_seq, %mask31 : i64",
        "      %completed_slot = arith.addi %completed_slot_base, %c1 : i64",
    ]
    for i in range(N):
        lines += [
            f"      %complete{i}a = arith.cmpi eq, %completed_slot, %c{i + 1} : i64",
            f"      %complete{i} = arith.andi %has_completed, %complete{i}a : i1",
            f"      %sd{i} = arith.select %complete{i}, %c1, %d{i} : i64",
        ]
    lines += [
        "      %occupancy = arith.subi %tail, %head : i64",
        f"      %room = arith.cmpi ult, %occupancy, %c{N} : i64",
        "      %incoming, %has_incoming = scf.if %room -> (i64, i1) {",
        "        %value, %valid = ac.try_recv @Core::@rob_in : i64",
        "        scf.yield %value, %valid : i64, i1",
        "      } else {",
        "        %zero_valid = arith.cmpi ne, %c0, %c0 : i64",
        "        scf.yield %c0, %zero_valid : i64, i1",
        "      }",
        "      scf.if %has_incoming {",
        "        %forwarded = ac.try_send @Core::@rob_to_rename %incoming : i64",
        "      }",
        "      %new_tail0 = arith.addi %tail, %c1 : i64",
        "      %new_tail = arith.select %has_incoming, %new_tail0, %tail : i64",
        "      %head_slot_base = arith.andi %head, %mask31 : i64",
        "      %head_slot = arith.addi %head_slot_base, %c1 : i64",
    ]
    lines.append(
        f"      %head_is{N - 2} = arith.cmpi eq, %head_slot, %c{N - 1} : i64"
    )
    lines.append(
        f"      %hd{N - 2} = arith.select %head_is{N - 2}, %sd{N - 2}, %sd{N - 1} : i64"
    )
    for i in range(N - 3, -1, -1):
        lines.append(
            f"      %head_is{i} = arith.cmpi eq, %head_slot, %c{i + 1} : i64"
        )
        lines.append(
            f"      %hd{i} = arith.select %head_is{i}, %sd{i}, %hd{i + 1} : i64"
        )
    lines += [
        "      %can_retire = arith.cmpi eq, %hd0, %c1 : i64",
        "      %new_head0 = arith.addi %head, %c1 : i64",
        "      %new_head = arith.select %can_retire, %new_head0, %head : i64",
    ]
    for i in range(N):
        lines += [
            f"      %retire_slot{i}a = arith.cmpi eq, %head_slot, %c{i + 1} : i64",
            f"      %retire_slot{i} = arith.andi %can_retire, %retire_slot{i}a : i1",
            f"      %nd{i} = arith.select %retire_slot{i}, %c0, %sd{i} : i64",
        ]
    lines += [
        "      scf.if %can_retire {",
        "        %retired_one = arith.constant 1 : i64",
        "        ac.stat.add @retired %retired_one : i64",
        "        %retired_desc = ac.trace.decode %head : i64 to i64",
        "        %opcode_shifted = arith.shrui %retired_desc, %c8 : i64",
        "        %opcode = arith.andi %opcode_shifted, %mask31 : i64",
        '        ac.trace.event %head lane "ROB" phase "retire" : i64',
        "        %is_opcode0 = arith.cmpi eq, %opcode, %c0 : i64",
        "        scf.if %is_opcode0 {",
        "          ac.stat.add @opcode_0 %retired_one : i64",
        "        }",
        "        %is_opcode1 = arith.cmpi eq, %opcode, %c1 : i64",
        "        scf.if %is_opcode1 {",
        "          ac.stat.add @opcode_1 %retired_one : i64",
        "        }",
        "        %is_opcode2 = arith.cmpi eq, %opcode, %c2 : i64",
        "        scf.if %is_opcode2 {",
        "          ac.stat.add @opcode_2 %retired_one : i64",
        "        }",
        "        %is_opcode3 = arith.cmpi eq, %opcode, %c3 : i64",
        "        scf.if %is_opcode3 {",
        "          ac.stat.add @opcode_3 %retired_one : i64",
        "        }",
        "        %is_opcode4 = arith.cmpi eq, %opcode, %c4 : i64",
        "        scf.if %is_opcode4 {",
        "          ac.stat.add @opcode_4 %retired_one : i64",
        "        }",
        "        %is_opcode5 = arith.cmpi eq, %opcode, %c5 : i64",
        "        scf.if %is_opcode5 {",
        "          ac.stat.add @opcode_5 %retired_one : i64",
        "        }",
        "        %is_opcode_vec = arith.cmpi uge, %opcode, %c6 : i64",
        "        scf.if %is_opcode_vec {",
        "          ac.stat.add @opcode_vec %retired_one : i64",
        "        }",
        "        %is_engine_s = arith.cmpi eq, %opcode, %c0 : i64",
        "        scf.if %is_engine_s {",
        "          ac.stat.add @scalar %retired_one : i64",
        "        }",
        "        %is_engine_c0 = arith.cmpi eq, %opcode, %c3 : i64",
        "        %is_engine_c1 = arith.cmpi eq, %opcode, %c4 : i64",
        "        %is_engine_c = arith.ori %is_engine_c0, %is_engine_c1 : i1",
        "        scf.if %is_engine_c {",
        "          ac.stat.add @cube %retired_one : i64",
        "        }",
        "        %is_engine_t0 = arith.cmpi eq, %opcode, %c1 : i64",
        "        %is_engine_t1 = arith.cmpi eq, %opcode, %c5 : i64",
        "        %is_engine_t = arith.ori %is_engine_t0, %is_engine_t1 : i1",
        "        scf.if %is_engine_t {",
        "          ac.stat.add @tlsu %retired_one : i64",
        "        }",
        "        %not_s = arith.cmpi ne, %opcode, %c0 : i64",
        "        %not_c = arith.xori %is_engine_c, %true : i1",
        "        %not_t = arith.xori %is_engine_t, %true : i1",
        "        %is_engine_v0 = arith.andi %not_s, %not_c : i1",
        "        %is_engine_v = arith.andi %is_engine_v0, %not_t : i1",
        "        scf.if %is_engine_v {",
        "          ac.stat.add @vector %retired_one : i64",
        "        }",
        "      }",
        "      %total, %has_total = ac.try_recv @Core::@trace_total : i64",
        "      %all_retired = arith.cmpi eq, %new_head, %total : i64",
        "      %nonzero_total = arith.cmpi ne, %total, %c0 : i64",
        "      %eof_done0 = arith.andi %has_total, %nonzero_total : i1",
        "      %eof_done = arith.andi %eof_done0, %all_retired : i1",
        "      %rob_occ = arith.subi %new_tail, %new_head : i64",
        '      ac.trace.counter %rob_occ lane "ROB" : i64',
        "      scf.if %eof_done {",
        '        ac.assert %eof_done, "retired"',
        "      }",
        "      %head_stored = ac.try_send @head %new_head : i64",
        "      %tail_stored = ac.try_send @tail %new_tail : i64",
    ]
    for i in range(N):
        lines.append(f"      %done{i}_stored = ac.try_send @done{i} %nd{i} : i64")
    lines += [
        "      ac.yield_sim",
        "    }",
        "    ac.return",
        "  }",
    ]
    return "\n".join(lines)


def emit_occupancy(valid_ssa: str) -> list[str]:
    return [
        "      %occ_m1 = arith.constant 1431655765 : i64",
        "      %occ_m2 = arith.constant 858993459 : i64",
        "      %occ_m4 = arith.constant 252645135 : i64",
        "      %occ_m6 = arith.constant 63 : i64",
        f"      %occ_x1 = arith.shrui {valid_ssa}, %c1 : i64",
        "      %occ_a1 = arith.andi %occ_x1, %occ_m1 : i64",
        f"      %occ_x = arith.subi {valid_ssa}, %occ_a1 : i64",
        "      %occ_b0 = arith.andi %occ_x, %occ_m2 : i64",
        "      %occ_x2 = arith.shrui %occ_x, %c2 : i64",
        "      %occ_b1 = arith.andi %occ_x2, %occ_m2 : i64",
        "      %occ_y = arith.addi %occ_b0, %occ_b1 : i64",
        "      %occ_y4 = arith.shrui %occ_y, %c4 : i64",
        "      %occ_z0 = arith.addi %occ_y, %occ_y4 : i64",
        "      %occ_z = arith.andi %occ_z0, %occ_m4 : i64",
        "      %occ_z8 = arith.shrui %occ_z, %c8 : i64",
        "      %occ_w = arith.addi %occ_z, %occ_z8 : i64",
        "      %occ_w16 = arith.shrui %occ_w, %c16 : i64",
        "      %occ_t = arith.addi %occ_w, %occ_w16 : i64",
        "      %iq_occ = arith.andi %occ_t, %occ_m6 : i64",
    ]


def emit_iq(
    module: str,
    suffix: str,
    lane: str,
    occupancy_lane: str | None,
) -> str:
    lines: list[str] = [f"  ac.module @{module}() parameters {{}} graph {{"]
    for i in range(N):
        lines.append(queue(f"slot{i}"))
    lines.append(queue("valid"))
    for i in range(4):
        lines.append(queue(f"ready{i}"))
    lines.append('    ac.process @step kind "control" {')
    for i in range(N + 1):
        lines.append(f"      %c{i} = arith.constant {i} : i64")
    lines += [
        "      %c37 = arith.constant 37 : i64",
        "      %c63 = arith.constant 63 : i64",
        "      %c255 = arith.constant 255 : i64",
        "      %mask7 = arith.constant 7 : i64",
        "      %mask63 = arith.constant 63 : i64",
        "      %mask255 = arith.constant 255 : i64",
        f"      %full_mask = arith.constant {FULL_MASK} : i64",
        "      %dep0_shift = arith.constant 13 : i64",
        "      %dep1_shift = arith.constant 21 : i64",
        "      %dep2_shift = arith.constant 29 : i64",
        "      %dep0_vbit = arith.constant 1 : i64",
        "      %dep1_vbit = arith.constant 2 : i64",
        "      %dep2_vbit = arith.constant 4 : i64",
        "      %false = arith.constant false",
        "      %valid_bits, %valid_ok = ac.try_recv @valid : i64",
    ]
    for i in range(N):
        lines.append(f"      %s{i}, %s{i}_ok = ac.try_recv @slot{i} : i64")
    for i in range(4):
        lines.append(f"      %r{i}, %r{i}_ok = ac.try_recv @ready{i} : i64")
    lines += [
        f"      %update, %has_update = ac.try_recv @Core::@ready_to_iq_{suffix} : i64",
        "      %update_desc = ac.trace.decode %update : i64 to i64",
        "      %update_seq = arith.andi %update_desc, %mask255 : i64",
        "      %update_word = arith.shrui %update_seq, %c6 : i64",
        "      %update_off = arith.andi %update_seq, %mask63 : i64",
        "      %update_bit = arith.shli %c1, %update_off : i64",
        "      %update_is0a = arith.cmpi eq, %update_word, %c0 : i64",
        "      %update_is0 = arith.andi %has_update, %update_is0a : i1",
        "      %r0_set = arith.ori %r0, %update_bit : i64",
        "      %nr0 = arith.select %update_is0, %r0_set, %r0 : i64",
        "      %update_is1a = arith.cmpi eq, %update_word, %c1 : i64",
        "      %update_is1 = arith.andi %has_update, %update_is1a : i1",
        "      %r1_set = arith.ori %r1, %update_bit : i64",
        "      %nr1 = arith.select %update_is1, %r1_set, %r1 : i64",
        "      %update_is2a = arith.cmpi eq, %update_word, %c2 : i64",
        "      %update_is2 = arith.andi %has_update, %update_is2a : i1",
        "      %r2_set = arith.ori %r2, %update_bit : i64",
        "      %nr2 = arith.select %update_is2, %r2_set, %r2 : i64",
        "      %update_is3a = arith.cmpi eq, %update_word, %c3 : i64",
        "      %update_is3 = arith.andi %has_update, %update_is3a : i1",
        "      %r3_set = arith.ori %r3, %update_bit : i64",
        "      %nr3 = arith.select %update_is3, %r3_set, %r3 : i64",
    ]
    for i in range(N):
        q = f"q{i}"
        lines += [
            f"      %{q}_desc = ac.trace.decode %s{i} : i64 to i64",
            f"      %{q}_dv_s = arith.shrui %{q}_desc, %c37 : i64",
            f"      %{q}_dvalid = arith.andi %{q}_dv_s, %mask7 : i64",
        ]
        for dep, shift, vbit in ((0, "dep0_shift", "dep0_vbit"), (1, "dep1_shift", "dep1_vbit"), (2, "dep2_shift", "dep2_vbit")):
            d = f"{q}_dep{dep}"
            lines += [
                f"      %{d}_s = arith.shrui %{q}_desc, %{shift} : i64",
                f"      %{d} = arith.andi %{d}_s, %mask255 : i64",
                f"      %{d}_word = arith.shrui %{d}, %c6 : i64",
                f"      %{d}_off = arith.andi %{d}, %mask63 : i64",
                f"      %{d}_bit = arith.shli %c1, %{d}_off : i64",
                f"      %{d}_w0 = arith.cmpi eq, %{d}_word, %c0 : i64",
                f"      %{d}_w1 = arith.cmpi eq, %{d}_word, %c1 : i64",
                f"      %{d}_w2 = arith.cmpi eq, %{d}_word, %c2 : i64",
                f"      %{d}_sel1 = arith.select %{d}_w1, %nr1, %nr3 : i64",
                f"      %{d}_sel0 = arith.select %{d}_w0, %nr0, %{d}_sel1 : i64",
                f"      %{d}_bits = arith.select %{d}_w2, %nr2, %{d}_sel0 : i64",
                f"      %{d}_hit0 = arith.andi %{d}_bits, %{d}_bit : i64",
                f"      %{d}_hit = arith.cmpi ne, %{d}_hit0, %c0 : i64",
                f"      %{d}_vhit = arith.andi %{q}_dvalid, %{vbit} : i64",
                f"      %{d}_used = arith.cmpi ne, %{d}_vhit, %c0 : i64",
                f"      %{d}_unused = arith.cmpi eq, %{d}_used, %false : i1",
                f"      %{d}_ready = arith.ori %{d}_unused, %{d}_hit : i1",
            ]
        lines += [
            f"      %{q}_deps01 = arith.andi %{q}_dep0_ready, %{q}_dep1_ready : i1",
            f"      %{q}_deps = arith.andi %{q}_deps01, %{q}_dep2_ready : i1",
            f"      %{q}_vbit = arith.constant {1 << i} : i64",
            f"      %{q}_vhit = arith.andi %valid_bits, %{q}_vbit : i64",
            f"      %{q}_occupied = arith.cmpi ne, %{q}_vhit, %c0 : i64",
            f"      %{q}_eligible = arith.andi %{q}_occupied, %{q}_deps : i1",
        ]
    lines += [
        "      %oldest_init = arith.constant 256 : i64",
        f"      %index_init = arith.constant {N} : i64",
        "      %q0_older = arith.cmpi ult, %s0, %oldest_init : i64",
        "      %q0_choose = arith.andi %q0_eligible, %q0_older : i1",
        "      %oldest0 = arith.select %q0_choose, %s0, %oldest_init : i64",
        "      %index0 = arith.select %q0_choose, %c0, %index_init : i64",
    ]
    for i in range(1, N):
        lines += [
            f"      %q{i}_older = arith.cmpi ult, %s{i}, %oldest{i - 1} : i64",
            f"      %q{i}_choose = arith.andi %q{i}_eligible, %q{i}_older : i1",
            f"      %oldest{i} = arith.select %q{i}_choose, %s{i}, %oldest{i - 1} : i64",
            f"      %index{i} = arith.select %q{i}_choose, %c{i}, %index{i - 1} : i64",
        ]
    last = N - 1
    lines += [
        f"      %has_issue = arith.cmpi ne, %index{last}, %c{N} : i64",
        "      %issued = scf.if %has_issue -> i1 {",
        f"        %sent = ac.try_send @Core::@iq_to_eng_{suffix} %oldest{last} : i64",
        "        scf.if %sent {",
        f'          ac.trace.event %oldest{last} lane "{lane}" phase "issue" : i64',
        "        }",
        "        scf.yield %sent : i1",
        "      } else {",
        "        scf.yield %false : i1",
        "      }",
    ]
    prev_valid = "%valid_bits"
    for i in range(N):
        mask = i64(~(1 << i))
        lines += [
            f"      %clear_mask{i} = arith.constant {mask} : i64",
            f"      %cleared{i} = arith.andi {prev_valid}, %clear_mask{i} : i64",
            f"      %issued_slot{i}a = arith.cmpi eq, %index{last}, %c{i} : i64",
            f"      %issued_slot{i} = arith.andi %issued, %issued_slot{i}a : i1",
            f"      %va{i} = arith.select %issued_slot{i}, %cleared{i}, {prev_valid} : i64",
        ]
        prev_valid = f"%va{i}"
    lines += [
        f"      %full = arith.cmpi eq, %va{last}, %full_mask : i64",
        "      %not_full = arith.cmpi eq, %full, %false : i1",
        "      %incoming, %has_incoming = scf.if %not_full -> (i64, i1) {",
        f"        %value, %ok = ac.try_recv @Core::@dispatch_to_iq_{suffix} : i64",
        "        scf.yield %value, %ok : i64, i1",
        "      } else {",
        "        scf.yield %c0, %false : i64, i1",
        "      }",
    ]
    lines += [
        f"      %ins_bit{last} = arith.constant {1 << last} : i64",
        f"      %ins_hit{last} = arith.andi %va{last}, %ins_bit{last} : i64",
        f"      %empty{last} = arith.cmpi eq, %ins_hit{last}, %c0 : i64",
        f"      %ins{last} = arith.select %empty{last}, %c{last}, %c{N} : i64",
    ]
    for i in range(N - 2, -1, -1):
        lines += [
            f"      %ins_bit{i} = arith.constant {1 << i} : i64",
            f"      %ins_hit{i} = arith.andi %va{last}, %ins_bit{i} : i64",
            f"      %empty{i} = arith.cmpi eq, %ins_hit{i}, %c0 : i64",
            f"      %ins{i} = arith.select %empty{i}, %c{i}, %ins{i + 1} : i64",
        ]
    for i in range(N):
        lines += [
            f"      %put{i}a = arith.cmpi eq, %ins0, %c{i} : i64",
            f"      %put{i} = arith.andi %has_incoming, %put{i}a : i1",
            f"      %ns{i} = arith.select %put{i}, %incoming, %s{i} : i64",
        ]
    prev_nv = f"%va{last}"
    for i in range(N):
        lines += [
            f"      %with{i} = arith.ori {prev_nv}, %ins_bit{i} : i64",
            f"      %nv{i} = arith.select %put{i}, %with{i}, {prev_nv} : i64",
        ]
        prev_nv = f"%nv{i}"
    lines += [
        f"      %valid_stored = ac.try_send @valid %nv{last} : i64",
    ]
    for i in range(N):
        lines.append(f"      %slot{i}_stored = ac.try_send @slot{i} %ns{i} : i64")
    for i in range(4):
        lines.append(f"      %ready{i}_stored = ac.try_send @ready{i} %nr{i} : i64")
    if occupancy_lane:
        lines += emit_occupancy(f"%nv{last}")
        lines.append(
            f'      ac.trace.counter %iq_occ lane "{occupancy_lane}" : i64'
        )
    lines += [
        "      ac.yield_sim",
        "    }",
        "    ac.return",
        "  }",
    ]
    return "\n".join(lines)


def main() -> None:
    src = MODEL.read_text()
    src = replace_module(src, "ROB", emit_rob())
    src = replace_module(src, "IssueQueueS", emit_iq("IssueQueueS", "s", "IQScalar", None))
    src = replace_module(src, "IssueQueueV", emit_iq("IssueQueueV", "v", "IQVector", "IQVector"))
    src = replace_module(src, "IssueQueueC", emit_iq("IssueQueueC", "c", "IQCube", "IQCube"))
    src = replace_module(src, "IssueQueueT", emit_iq("IssueQueueT", "t", "IQTlsu", "IQTlsu"))
    MODEL.write_text(src)
    print(f"updated {MODEL} IQ/ROB capacity={N}")


if __name__ == "__main__":
    main()

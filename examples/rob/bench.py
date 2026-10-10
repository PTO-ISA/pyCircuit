"""Regular-clock rob scenario; original physical-control oracles remain."""

from example_rob.rob import Rob
from pycircuit import bits, log, rule, struct, system, table, u1, u2, u3, u8, u16


@struct
class Scenario:
    allocate: u1
    allocate_tag: u8
    complete: u1
    complete_index: u2
    complete_tag: u8
    complete_value: u16
    retire: u1
    flush: u1
    expected_allocate_accepted: u1
    expected_allocated_index: u2
    expected_complete_accepted: u1
    expected_retire_accepted: u1
    expected_retired_tag: u8
    expected_retired_value: u16
    expected_count: u3


@struct
class Scenarios:
    rows: table[284, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseRob():  # noqa: N802
    phase: bits[64] = 0
    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(complete=1, complete_value=7, retire=1),
            Scenario(allocate=1, allocate_tag=11, expected_allocate_accepted=1, expected_count=1),
            Scenario(allocate=1, allocate_tag=12, expected_allocate_accepted=1, expected_allocated_index=1, expected_count=2),
            Scenario(allocate=1, allocate_tag=13, expected_allocate_accepted=1, expected_allocated_index=2, expected_count=3),
            Scenario(allocate=1, allocate_tag=14, expected_allocate_accepted=1, expected_allocated_index=3, expected_count=4),
            Scenario(allocate=1, allocate_tag=15, expected_count=4),
            Scenario(complete=1, complete_tag=99, complete_value=999, expected_count=4),
            Scenario(complete=1, complete_index=2, complete_tag=13, complete_value=4915, expected_complete_accepted=1, expected_count=4),
            Scenario(retire=1, expected_count=4),
            Scenario(complete=1, complete_tag=11, complete_value=4369, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=11, expected_retired_value=4369, expected_count=3),
            Scenario(allocate=1, allocate_tag=15, expected_allocate_accepted=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=16, complete=1, complete_index=1, complete_tag=12, complete_value=4642, retire=1, expected_allocate_accepted=1, expected_allocated_index=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=12, expected_retired_value=4642, expected_count=4),
            Scenario(complete=1, complete_index=1, complete_tag=12, complete_value=65535, expected_count=4),
            Scenario(complete=1, complete_tag=15, complete_value=5461, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=13, expected_retired_value=4915, expected_count=3),
            Scenario(retire=1, expected_count=3),
            Scenario(allocate=1, allocate_tag=17, complete=1, complete_index=3, complete_tag=14, complete_value=5188, retire=1, expected_allocate_accepted=1, expected_allocated_index=2, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=14, expected_retired_value=5188, expected_count=3),
            Scenario(allocate=1, allocate_tag=18, complete=1, complete_tag=15, complete_value=5461, retire=1, expected_allocate_accepted=1, expected_allocated_index=3, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=15, expected_retired_value=5461, expected_count=3),
            Scenario(complete=1, complete_index=1, complete_tag=16, complete_value=5734, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=16, expected_retired_value=5734, expected_count=2),
            Scenario(complete=1, complete_index=2, complete_tag=17, complete_value=6007, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=17, expected_retired_value=6007, expected_count=1),
            Scenario(complete=1, complete_index=3, complete_tag=18, complete_value=6280, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=18, expected_retired_value=6280),
            Scenario(complete=1, complete_index=3, complete_tag=18, complete_value=9, retire=1),
            Scenario(allocate=1, allocate_tag=21, expected_allocate_accepted=1, expected_count=1),
            Scenario(allocate=1, allocate_tag=22, complete=1, complete_tag=21, complete_value=21, retire=1, flush=1),
            Scenario(allocate=1, allocate_tag=31, expected_allocate_accepted=1, expected_count=1),
            Scenario(complete=1, complete_tag=21, complete_value=21, retire=1, expected_count=1),
            Scenario(complete=1, complete_tag=31, complete_value=12593, retire=1, expected_complete_accepted=1, expected_retire_accepted=1, expected_retired_tag=31, expected_retired_value=12593),
            Scenario(flush=1),
            Scenario(allocate=1, allocate_tag=199, complete=1, complete_index=3, complete_tag=174, complete_value=44743, retire=1, expected_allocate_accepted=1, expected_count=1),
            Scenario(allocate=1, allocate_tag=12, complete_index=2, complete_tag=224, complete_value=57356, retire=1, expected_allocate_accepted=1, expected_allocated_index=1, expected_count=2),
            Scenario(allocate_tag=225, complete_index=2, complete_tag=15, complete_value=4065, expected_count=2),
            Scenario(allocate=1, allocate_tag=246, complete_index=3, complete_tag=112, complete_value=28918, retire=1, expected_allocate_accepted=1, expected_allocated_index=2, expected_count=3),
            Scenario(allocate_tag=2, complete_index=3, complete_tag=206, complete_value=52738, expected_count=3),
            Scenario(allocate=1, allocate_tag=69, complete=1, complete_index=1, complete_tag=107, complete_value=27461, expected_allocate_accepted=1, expected_allocated_index=3, expected_count=4),
            Scenario(allocate_tag=110, complete_index=3, complete_tag=38, complete_value=9838, expected_count=4),
            Scenario(allocate=1, allocate_tag=1, complete=1, complete_index=1, complete_tag=8, complete_value=2049, retire=1, expected_count=4),
            Scenario(allocate_tag=63, complete=1, complete_tag=113, complete_value=28991, expected_count=4),
            Scenario(allocate_tag=17, complete=1, complete_index=3, complete_tag=187, complete_value=47889, expected_count=4),
            Scenario(allocate=1, allocate_tag=39, complete=1, complete_index=3, complete_tag=222, complete_value=56871, expected_count=4),
            Scenario(allocate_tag=165, complete=1, complete_index=3, complete_tag=94, complete_value=24229, retire=1, expected_count=4),
            Scenario(allocate_tag=25, complete=1, complete_index=2, complete_tag=203, complete_value=51993, expected_count=4),
            Scenario(allocate=1, allocate_tag=92, complete=1, complete_index=3, complete_tag=174, complete_value=44636, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=76, complete_index=2, complete_tag=146, complete_value=37452, expected_count=4),
            Scenario(allocate_tag=94, complete=1, complete_index=2, complete_tag=224, complete_value=57438, expected_count=4),
            Scenario(allocate_tag=179, complete_index=3, complete_tag=138, complete_value=35507, retire=1, expected_count=4),
            Scenario(allocate_tag=120, complete=1, complete_index=1, complete_tag=90, complete_value=23160, expected_count=4),
            Scenario(allocate=1, allocate_tag=215, complete=1, complete_index=1, complete_tag=23, complete_value=6103, expected_count=4),
            Scenario(allocate_tag=146, complete_index=3, complete_tag=16, complete_value=4242, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=133, complete=1, complete_index=2, complete_tag=164, complete_value=42117, retire=1, expected_count=4),
            Scenario(allocate_tag=67, complete=1, complete_tag=111, complete_value=28483, retire=1, expected_count=4),
            Scenario(allocate_tag=184, complete=1, complete_tag=45, complete_value=11704, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=35, complete=1, complete_index=3, complete_tag=69, complete_value=17699, expected_complete_accepted=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=71, complete_index=2, complete_tag=183, complete_value=46919, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=165, complete_index=3, complete_tag=71, complete_value=18341, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=19, complete_index=2, complete_tag=224, complete_value=57363, expected_count=4),
            Scenario(allocate=1, allocate_tag=119, complete=1, complete_tag=52, complete_value=13431, expected_count=4),
            Scenario(allocate=1, allocate_tag=93, complete_tag=72, complete_value=18525, expected_count=4),
            Scenario(allocate=1, allocate_tag=5, complete_index=2, complete_tag=248, complete_value=63493, retire=1, expected_count=4),
            Scenario(allocate_tag=211, complete_index=1, complete_tag=55, complete_value=14291, retire=1, expected_count=4),
            Scenario(allocate_tag=24, complete_index=3, complete_tag=126, complete_value=32280, retire=1, expected_count=4),
            Scenario(allocate_tag=81, complete=1, complete_index=2, complete_tag=215, complete_value=55121, expected_count=4),
            Scenario(allocate=1, allocate_tag=252, complete=1, complete_index=1, complete_tag=189, complete_value=48636, retire=1, expected_count=4),
            Scenario(allocate_tag=127, complete=1, complete_index=1, complete_tag=67, complete_value=17279, expected_count=4),
            Scenario(allocate=1, allocate_tag=75, complete_index=1, complete_tag=89, complete_value=22859, retire=1, expected_count=4),
            Scenario(allocate_tag=6, complete_index=3, complete_tag=78, complete_value=19974, expected_count=4),
            Scenario(allocate=1, allocate_tag=209, complete=1, complete_index=1, complete_tag=107, complete_value=27601, expected_count=4),
            Scenario(allocate_tag=74, complete_index=1, complete_tag=167, complete_value=42826, expected_count=4),
            Scenario(allocate=1, allocate_tag=206, complete=1, complete_index=2, complete_tag=2, complete_value=718, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=9, complete=1, complete_index=3, complete_tag=242, complete_value=61961, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=19, complete=1, complete_tag=237, complete_value=60691, retire=1, expected_count=4),
            Scenario(complete_index=2, complete_tag=121, complete_value=30976, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=44, complete_index=3, complete_tag=41, complete_value=10540, expected_count=4),
            Scenario(allocate=1, allocate_tag=138, complete_index=2, complete_tag=188, complete_value=48266, retire=1, expected_count=4),
            Scenario(allocate_tag=69, complete_tag=143, complete_value=36677, flush=1),
            Scenario(allocate=1, allocate_tag=51, complete=1, complete_index=1, complete_tag=205, complete_value=52531, retire=1, expected_allocate_accepted=1, expected_count=1),
            Scenario(allocate_tag=19, complete=1, complete_index=3, complete_tag=201, complete_value=51475, expected_count=1),
            Scenario(allocate_tag=122, complete_index=1, complete_tag=43, complete_value=11130, retire=1, expected_count=1),
            Scenario(allocate_tag=168, complete=1, complete_index=2, complete_tag=206, complete_value=52904, expected_count=1),
            Scenario(allocate=1, allocate_tag=82, complete_tag=39, complete_value=10066, expected_allocate_accepted=1, expected_allocated_index=1, expected_count=2),
            Scenario(allocate_tag=179, complete=1, complete_index=2, complete_tag=147, complete_value=37811, retire=1, expected_count=2),
            Scenario(allocate_tag=232, complete_index=3, complete_tag=119, complete_value=30696, expected_count=2),
            Scenario(allocate=1, allocate_tag=52, complete=1, complete_index=1, complete_tag=184, complete_value=47156, retire=1, expected_allocate_accepted=1, expected_allocated_index=2, expected_count=3),
            Scenario(allocate_tag=210, complete=1, complete_index=1, complete_tag=115, complete_value=29650, expected_count=3),
            Scenario(allocate=1, allocate_tag=80, complete=1, complete_index=3, complete_tag=85, complete_value=21840, retire=1, expected_allocate_accepted=1, expected_allocated_index=3, expected_count=4),
            Scenario(allocate_tag=28, complete=1, complete_index=2, complete_tag=22, complete_value=5660, retire=1, expected_count=4),
            Scenario(allocate_tag=213, complete_index=2, complete_tag=36, complete_value=9429, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=208, complete=1, complete_index=1, complete_tag=111, complete_value=28624, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=11, complete=1, complete_index=1, complete_tag=79, complete_value=20235, expected_count=4),
            Scenario(allocate_tag=104, complete=1, complete_tag=94, complete_value=24168, retire=1, expected_count=4),
            Scenario(allocate_tag=2, complete=1, complete_index=2, complete_tag=85, complete_value=21762, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=244, complete_tag=208, complete_value=53492, retire=1, expected_count=4),
            Scenario(allocate_tag=109, complete=1, complete_index=2, complete_tag=131, complete_value=33645, retire=1, expected_count=4),
            Scenario(allocate_tag=189, complete=1, complete_index=3, complete_tag=44, complete_value=11453, expected_count=4),
            Scenario(allocate_tag=180, complete_index=2, complete_tag=44, complete_value=11444, expected_count=4),
            Scenario(allocate_tag=101, complete=1, complete_tag=7, complete_value=1893, retire=1, expected_count=4),
            Scenario(allocate_tag=106, complete_index=3, complete_tag=30, complete_value=7786, expected_count=4),
            Scenario(allocate=1, allocate_tag=221, complete=1, complete_tag=160, complete_value=41181, expected_count=4),
            Scenario(allocate=1, allocate_tag=14, complete=1, complete_index=1, complete_tag=16, complete_value=4110, expected_count=4),
            Scenario(allocate=1, allocate_tag=77, complete_index=1, complete_tag=90, complete_value=23117, expected_count=4),
            Scenario(allocate_tag=109, complete=1, complete_index=3, complete_tag=33, complete_value=8557, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=116, complete=1, complete_index=2, complete_tag=216, complete_value=55412, retire=1, expected_count=4),
            Scenario(allocate_tag=91, complete_tag=187, complete_value=47963, retire=1, expected_count=4),
            Scenario(allocate_tag=248, complete_tag=228, complete_value=58616, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=39, complete_index=2, complete_tag=175, complete_value=44839, expected_count=4),
            Scenario(allocate_tag=165, complete_index=2, complete_tag=5, complete_value=1445, expected_count=4),
            Scenario(allocate_tag=178, complete=1, complete_index=1, complete_tag=42, complete_value=10930, retire=1, expected_count=4),
            Scenario(allocate_tag=138, complete=1, complete_index=3, complete_tag=126, complete_value=32394, expected_count=4),
            Scenario(allocate=1, allocate_tag=22, complete=1, complete_tag=91, complete_value=23318, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=140, complete=1, complete_tag=170, complete_value=43660, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=105, complete_index=2, complete_tag=168, complete_value=43113, expected_count=4),
            Scenario(allocate=1, allocate_tag=213, complete_index=2, complete_tag=171, complete_value=43989, expected_count=4),
            Scenario(allocate_tag=88, complete=1, complete_index=3, complete_tag=209, complete_value=53592, expected_count=4),
            Scenario(allocate_tag=52, complete_index=2, complete_tag=92, complete_value=23604, expected_count=4),
            Scenario(allocate_tag=139, complete=1, complete_index=3, complete_tag=101, complete_value=25995, expected_count=4),
            Scenario(allocate_tag=153, complete=1, complete_index=1, complete_tag=23, complete_value=6041, expected_count=4),
            Scenario(allocate=1, allocate_tag=249, complete=1, complete_tag=75, complete_value=19449, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=239, complete=1, complete_index=3, complete_tag=78, complete_value=20207, expected_count=4),
            Scenario(allocate_tag=134, complete_index=3, complete_tag=67, complete_value=17286, retire=1, expected_count=4),
            Scenario(allocate_tag=132, complete_index=1, complete_tag=155, complete_value=39812, retire=1, expected_count=4),
            Scenario(allocate_tag=198, complete_index=3, complete_tag=60, complete_value=15558, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=23, complete=1, complete_index=3, complete_tag=15, complete_value=3863, expected_count=4),
            Scenario(allocate_tag=11, complete_index=3, complete_tag=55, complete_value=14091, retire=1, expected_count=4),
            Scenario(allocate_tag=45, complete_index=3, complete_tag=93, complete_value=23853, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=115, complete=1, complete_index=2, complete_tag=59, complete_value=15219, expected_count=4),
            Scenario(allocate_tag=108, complete=1, complete_index=3, complete_tag=3, complete_value=876, expected_count=4),
            Scenario(allocate=1, allocate_tag=16, complete_tag=76, complete_value=19472, expected_count=4),
            Scenario(allocate=1, allocate_tag=84, complete=1, complete_index=1, complete_tag=198, complete_value=50772, expected_count=4),
            Scenario(allocate_tag=24, complete_index=2, complete_tag=136, complete_value=34840, expected_count=4),
            Scenario(allocate_tag=152, complete=1, complete_index=2, complete_tag=32, complete_value=8344, expected_count=4),
            Scenario(allocate_tag=66, complete_index=1, complete_tag=134, complete_value=34370, retire=1, expected_count=4),
            Scenario(allocate_tag=147, complete_tag=220, complete_value=56467, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=125, complete=1, complete_index=1, complete_tag=118, complete_value=30333, expected_count=4),
            Scenario(allocate_tag=47, complete=1, complete_index=3, complete_tag=142, complete_value=36399, expected_count=4),
            Scenario(allocate=1, allocate_tag=253, complete_tag=140, complete_value=36093, retire=1, expected_count=4),
            Scenario(allocate_tag=193, complete=1, complete_index=1, complete_tag=240, complete_value=61633, expected_count=4),
            Scenario(allocate_tag=86, complete=1, complete_index=2, complete_tag=222, complete_value=56918, expected_count=4),
            Scenario(allocate=1, allocate_tag=144, complete=1, complete_tag=114, complete_value=29328, expected_count=4),
            Scenario(allocate_tag=212, complete=1, complete_tag=48, complete_value=12500, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=24, complete_index=3, complete_tag=107, complete_value=27416, expected_count=4),
            Scenario(allocate_tag=252, complete_index=2, complete_tag=59, complete_value=15356, retire=1, expected_count=4),
            Scenario(allocate_tag=75, complete=1, complete_index=3, complete_tag=60, complete_value=15435, expected_count=4),
            Scenario(allocate_tag=14, complete=1, complete_index=2, complete_tag=116, complete_value=29710, expected_count=4),
            Scenario(allocate_tag=172, complete_index=1, complete_tag=18, complete_value=4780, retire=1, expected_count=4),
            Scenario(allocate_tag=150, complete_index=3, complete_tag=149, complete_value=38294, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=137, complete=1, complete_tag=144, complete_value=37001, retire=1, expected_count=4),
            Scenario(allocate_tag=29, complete=1, complete_tag=244, complete_value=62493, expected_count=4),
            Scenario(allocate_tag=31, complete_index=2, complete_tag=223, complete_value=57119, expected_count=4),
            Scenario(allocate=1, allocate_tag=154, complete_index=1, complete_tag=123, complete_value=31642, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=18, complete_index=2, complete_tag=21, complete_value=5394, expected_count=4),
            Scenario(allocate=1, allocate_tag=56, complete_index=2, complete_tag=65, complete_value=16696, expected_count=4),
            Scenario(allocate=1, allocate_tag=212, complete=1, complete_tag=222, complete_value=57044, expected_count=4),
            Scenario(allocate_tag=153, complete=1, complete_index=2, complete_tag=2, complete_value=665, expected_count=4),
            Scenario(allocate=1, allocate_tag=227, complete=1, complete_tag=73, complete_value=18915, retire=1, expected_count=4),
            Scenario(allocate_tag=75, complete_index=2, complete_tag=197, complete_value=50507, expected_count=4),
            Scenario(allocate_tag=15, complete_index=1, complete_tag=189, complete_value=48399, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=36, complete_tag=55, complete_value=14116, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=70, complete_index=2, complete_tag=148, complete_value=37958, expected_count=4),
            Scenario(allocate=1, allocate_tag=170, complete=1, complete_index=1, complete_tag=241, complete_value=61866, expected_count=4),
            Scenario(allocate=1, allocate_tag=239, complete=1, complete_index=1, complete_tag=139, complete_value=35823, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=212, complete_tag=197, complete_value=50644, expected_count=4),
            Scenario(allocate=1, allocate_tag=5, complete_index=2, complete_tag=99, complete_value=25349, expected_count=4),
            Scenario(allocate_tag=104, complete=1, complete_tag=24, complete_value=6248, expected_count=4),
            Scenario(allocate_tag=150, complete=1, complete_index=1, complete_tag=2, complete_value=662, expected_count=4),
            Scenario(allocate=1, allocate_tag=100, complete=1, complete_index=2, complete_tag=144, complete_value=36964, expected_count=4),
            Scenario(allocate=1, allocate_tag=97, complete=1, complete_index=3, complete_tag=36, complete_value=9313, expected_count=4),
            Scenario(allocate=1, allocate_tag=207, complete=1, complete_index=1, complete_tag=38, complete_value=9935, expected_count=4),
            Scenario(allocate_tag=44, complete_tag=9, complete_value=2348, expected_count=4),
            Scenario(allocate_tag=248, complete=1, complete_index=3, complete_tag=55, complete_value=14328, expected_count=4),
            Scenario(allocate=1, allocate_tag=224, complete=1, complete_index=3, complete_tag=98, complete_value=25312, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=15, complete=1, complete_tag=85, complete_value=21775, expected_count=4),
            Scenario(allocate=1, allocate_tag=80, complete_index=2, complete_tag=16, complete_value=4176, retire=1, expected_count=4),
            Scenario(allocate_tag=101, complete=1, complete_index=3, complete_tag=234, complete_value=60005, retire=1, expected_count=4),
            Scenario(allocate_tag=187, complete_tag=236, complete_value=60603, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=226, complete=1, complete_index=3, complete_tag=69, complete_value=17890, retire=1, expected_count=4),
            Scenario(allocate_tag=128, complete=1, complete_index=3, complete_tag=76, complete_value=19584, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=246, complete=1, complete_index=3, complete_tag=42, complete_value=10998, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=25, complete_index=2, complete_tag=77, complete_value=19737, expected_count=4),
            Scenario(allocate=1, allocate_tag=42, complete_tag=116, complete_value=29738, retire=1, expected_count=4),
            Scenario(allocate_tag=171, complete_index=3, complete_tag=118, complete_value=30379, expected_count=4),
            Scenario(allocate=1, allocate_tag=243, complete_index=2, complete_tag=110, complete_value=28403, expected_count=4),
            Scenario(allocate_tag=177, complete=1, complete_tag=228, complete_value=58545, expected_count=4),
            Scenario(allocate_tag=157, complete_tag=80, complete_value=20637, expected_count=4),
            Scenario(allocate_tag=202, complete=1, complete_index=1, complete_tag=119, complete_value=30666, retire=1, expected_count=4),
            Scenario(allocate_tag=180, complete_index=2, complete_tag=158, complete_value=40628, expected_count=4),
            Scenario(allocate=1, allocate_tag=211, complete=1, complete_tag=226, complete_value=58067, expected_count=4),
            Scenario(allocate_tag=253, complete_index=2, complete_tag=72, complete_value=18685, expected_count=4),
            Scenario(allocate_tag=203, complete=1, complete_index=2, complete_tag=92, complete_value=23755, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=68, complete_tag=204, complete_value=52292, expected_count=4),
            Scenario(allocate=1, allocate_tag=141, complete_index=2, complete_tag=80, complete_value=20621, expected_count=4),
            Scenario(allocate=1, allocate_tag=210, complete=1, complete_index=1, complete_tag=71, complete_value=18386, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=178, complete=1, complete_index=2, complete_tag=58, complete_value=15026, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=67, complete=1, complete_index=1, complete_tag=77, complete_value=19779, expected_count=4),
            Scenario(allocate=1, allocate_tag=14, complete_index=3, complete_tag=129, complete_value=33038, retire=1, expected_count=4),
            Scenario(allocate_tag=145, complete=1, complete_tag=131, complete_value=33681, retire=1, expected_count=4),
            Scenario(allocate_tag=129, complete=1, complete_index=2, complete_tag=128, complete_value=32897, expected_count=4),
            Scenario(allocate_tag=208, complete=1, complete_index=2, complete_tag=148, complete_value=38096, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=70, complete_index=2, complete_tag=122, complete_value=31302, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=243, complete=1, complete_tag=21, complete_value=5619, expected_count=4),
            Scenario(allocate=1, allocate_tag=88, complete_index=1, complete_tag=223, complete_value=57176, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=120, complete=1, complete_index=1, complete_tag=114, complete_value=29304, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=25, complete=1, complete_tag=124, complete_value=31769, expected_count=4),
            Scenario(allocate=1, allocate_tag=87, complete_index=1, complete_tag=200, complete_value=51287, expected_count=4),
            Scenario(allocate=1, allocate_tag=126, complete_tag=53, complete_value=13694, retire=1, expected_count=4),
            Scenario(allocate_tag=104, complete_tag=143, complete_value=36712, expected_count=4),
            Scenario(allocate=1, allocate_tag=190, complete=1, complete_tag=104, complete_value=26814, expected_count=4),
            Scenario(allocate=1, allocate_tag=253, complete_tag=53, complete_value=13821, retire=1, expected_count=4),
            Scenario(allocate_tag=87, complete=1, complete_tag=122, complete_value=31319, retire=1, expected_count=4),
            Scenario(allocate_tag=80, complete=1, complete_index=1, complete_tag=137, complete_value=35152, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=170, complete=1, complete_tag=253, complete_value=64938, retire=1, expected_count=4),
            Scenario(allocate_tag=164, complete=1, complete_index=1, complete_tag=222, complete_value=56996, expected_count=4),
            Scenario(allocate=1, allocate_tag=230, complete=1, complete_tag=12, complete_value=3302, expected_count=4),
            Scenario(allocate=1, allocate_tag=31, complete=1, complete_index=2, complete_tag=203, complete_value=51999, expected_count=4),
            Scenario(allocate_tag=100, complete_index=2, complete_tag=231, complete_value=59236, retire=1, expected_count=4),
            Scenario(allocate_tag=237, complete_index=3, complete_tag=100, complete_value=25837, expected_count=4),
            Scenario(allocate_tag=145, complete=1, complete_tag=121, complete_value=31121, expected_count=4),
            Scenario(allocate_tag=37, complete_tag=244, complete_value=62501, expected_count=4),
            Scenario(allocate_tag=210, complete=1, complete_index=3, complete_tag=217, complete_value=55762, expected_count=4),
            Scenario(allocate_tag=92, complete_tag=22, complete_value=5724, expected_count=4),
            Scenario(allocate=1, allocate_tag=119, complete_index=3, complete_tag=220, complete_value=56439, expected_count=4),
            Scenario(allocate=1, allocate_tag=84, complete=1, complete_index=2, complete_tag=155, complete_value=39764, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=29, complete_tag=154, complete_value=39453, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=99, complete_index=2, complete_tag=55, complete_value=14179, expected_count=4),
            Scenario(allocate_tag=104, complete_index=3, complete_tag=2, complete_value=616, expected_count=4),
            Scenario(allocate=1, allocate_tag=144, complete_tag=104, complete_value=26768, expected_count=4),
            Scenario(allocate=1, allocate_tag=210, complete=1, complete_tag=255, complete_value=65490, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=140, complete=1, complete_index=3, complete_tag=141, complete_value=36236, expected_count=4),
            Scenario(allocate=1, allocate_tag=220, complete_tag=149, complete_value=38364, expected_count=4),
            Scenario(allocate=1, allocate_tag=148, complete_tag=49, complete_value=12692, expected_count=4),
            Scenario(allocate=1, allocate_tag=235, complete_tag=155, complete_value=39915, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=250, complete=1, complete_tag=82, complete_value=21242, expected_count=4),
            Scenario(allocate_tag=183, complete_index=3, complete_tag=193, complete_value=49591, expected_count=4),
            Scenario(allocate_tag=227, complete=1, complete_tag=181, complete_value=46563, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=95, complete=1, complete_index=2, complete_tag=169, complete_value=43359, retire=1, expected_count=4),
            Scenario(allocate_tag=35, complete_index=3, complete_tag=136, complete_value=34851, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=151, complete_index=2, complete_tag=144, complete_value=37015, expected_count=4),
            Scenario(allocate=1, allocate_tag=215, complete_index=1, complete_tag=68, complete_value=17623, retire=1, expected_count=4),
            Scenario(allocate_tag=83, complete=1, complete_tag=104, complete_value=26707, expected_count=4),
            Scenario(allocate_tag=64, complete=1, complete_tag=11, complete_value=2880, expected_count=4),
            Scenario(allocate=1, allocate_tag=238, complete=1, complete_index=3, complete_tag=122, complete_value=31470, expected_count=4),
            Scenario(allocate_tag=153, complete_index=3, complete_tag=76, complete_value=19609, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=74, complete_tag=24, complete_value=6218, expected_count=4),
            Scenario(allocate=1, allocate_tag=90, complete=1, complete_index=1, complete_tag=205, complete_value=52570, expected_count=4),
            Scenario(allocate=1, allocate_tag=239, complete=1, complete_index=1, complete_tag=176, complete_value=45295, expected_count=4),
            Scenario(allocate=1, allocate_tag=20, complete=1, complete_tag=75, complete_value=19220, expected_count=4),
            Scenario(allocate_tag=234, complete=1, complete_index=1, complete_tag=175, complete_value=45034, expected_count=4),
            Scenario(allocate=1, allocate_tag=17, complete=1, complete_index=2, complete_tag=146, complete_value=37393, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=210, complete_index=2, complete_tag=2, complete_value=722, expected_count=4),
            Scenario(allocate_tag=182, complete_index=3, complete_tag=50, complete_value=12982, expected_count=4),
            Scenario(allocate=1, allocate_tag=18, complete=1, complete_tag=29, complete_value=7442, retire=1, expected_count=4),
            Scenario(allocate_tag=17, complete=1, complete_index=2, complete_tag=58, complete_value=14865, retire=1, expected_count=4),
            Scenario(allocate_tag=254, complete=1, complete_index=3, complete_tag=34, complete_value=8958, retire=1, expected_count=4),
            Scenario(allocate_tag=169, complete=1, complete_tag=216, complete_value=55465, expected_count=4),
            Scenario(allocate_tag=92, complete_index=2, complete_tag=97, complete_value=24924, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=76, complete_tag=184, complete_value=47180, expected_count=4),
            Scenario(allocate=1, allocate_tag=1, complete_tag=45, complete_value=11521, retire=1, expected_count=4),
            Scenario(allocate_tag=154, complete_tag=99, complete_value=25498, retire=1, expected_count=4),
            Scenario(allocate_tag=107, complete=1, complete_index=3, complete_tag=139, complete_value=35691, expected_count=4),
            Scenario(allocate=1, allocate_tag=249, complete_index=2, complete_tag=56, complete_value=14585, retire=1, expected_count=4),
            Scenario(allocate_tag=207, complete=1, complete_tag=235, complete_value=60367, retire=1, expected_count=4),
            Scenario(allocate_tag=220, complete_index=1, complete_tag=188, complete_value=48348, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=177, complete_index=2, complete_tag=126, complete_value=32433, expected_count=4),
            Scenario(allocate_tag=164, complete=1, complete_index=3, complete_tag=189, complete_value=48548, expected_count=4),
            Scenario(allocate_tag=100, complete=1, complete_index=1, complete_tag=55, complete_value=14180, retire=1, expected_count=4),
            Scenario(allocate_tag=34, complete=1, complete_index=3, complete_tag=138, complete_value=35362, expected_count=4),
            Scenario(allocate=1, allocate_tag=217, complete_index=2, complete_tag=55, complete_value=14297, expected_count=4),
            Scenario(allocate_tag=183, complete_index=3, complete_tag=85, complete_value=21943, expected_count=4),
            Scenario(allocate_tag=232, complete_index=2, complete_tag=171, complete_value=44008, retire=1, expected_count=4),
            Scenario(allocate_tag=104, complete=1, complete_index=1, complete_tag=34, complete_value=8808, expected_count=4),
            Scenario(allocate=1, allocate_tag=191, complete_index=1, complete_tag=245, complete_value=62911, retire=1, expected_count=4),
            Scenario(allocate_tag=181, complete=1, complete_index=1, complete_tag=237, complete_value=60853, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=185, complete_index=3, complete_tag=185, complete_value=47545, retire=1, expected_count=4),
            Scenario(allocate_tag=231, complete_index=3, complete_tag=204, complete_value=52455, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=237, complete=1, complete_index=2, complete_tag=194, complete_value=49901, expected_count=4),
            Scenario(allocate_tag=129, complete_index=3, complete_tag=150, complete_value=38529, retire=1, expected_count=4),
            Scenario(allocate_tag=73, complete=1, complete_index=2, complete_tag=4, complete_value=1097, expected_count=4),
            Scenario(allocate_tag=229, complete_index=3, complete_tag=138, complete_value=35557, retire=1, expected_count=4),
            Scenario(allocate_tag=199, complete_index=3, complete_value=199, expected_count=4),
            Scenario(allocate_tag=20, complete_tag=209, complete_value=53524, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=205, complete=1, complete_tag=162, complete_value=41677, retire=1, expected_count=4),
            Scenario(allocate=1, complete_index=1, complete_tag=149, complete_value=38144, retire=1, expected_count=4),
            Scenario(allocate_tag=145, complete=1, complete_tag=113, complete_value=29073, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=140, complete=1, complete_index=3, complete_tag=39, complete_value=10124, retire=1, expected_count=4),
            Scenario(allocate=1, allocate_tag=27, complete_index=1, complete_tag=56, complete_value=14363, expected_count=4),
            Scenario(allocate=1, allocate_tag=7, complete=1, complete_index=3, complete_tag=207, complete_value=52999, expected_count=4),
            Scenario(flush=1),
        )
    )
    # fmt: on
    position = phase if phase < 284 else 283
    scenario = scenarios.rows[position]
    dut = Rob(
        scenario.allocate,
        scenario.allocate_tag,
        scenario.complete,
        scenario.complete_index,
        scenario.complete_tag,
        scenario.complete_value,
        scenario.retire,
        scenario.flush,
    )

    @rule
    def check_and_advance():
        if phase < 284:
            assert (
                dut.allocate_accepted == scenario.expected_allocate_accepted
            ), "rob: allocate_accepted"
            assert (
                dut.allocated_index == scenario.expected_allocated_index
            ), "rob: allocated_index"
            assert (
                dut.complete_accepted == scenario.expected_complete_accepted
            ), "rob: complete_accepted"
            assert (
                dut.retire_accepted == scenario.expected_retire_accepted
            ), "rob: retire_accepted"
            assert dut.retired_tag == scenario.expected_retired_tag, "rob: retired_tag"
            assert (
                dut.retired_value == scenario.expected_retired_value
            ), "rob: retired_value"
            assert dut.count == scenario.expected_count, "rob: count"
        log("info", "rob.allocate_accepted", dut.allocate_accepted)
        log("info", "rob.allocated_index", dut.allocated_index)
        log("info", "rob.complete_accepted", dut.complete_accepted)
        log("info", "rob.retire_accepted", dut.retire_accepted)
        log("info", "rob.retired_tag", dut.retired_tag)
        log("info", "rob.retired_value", dut.retired_value)
        log("info", "rob.count", dut.count)

    check_and_advance()
    advance(phase)

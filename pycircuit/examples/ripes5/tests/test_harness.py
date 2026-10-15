"""Independent loader fixtures and fault injection for the comparison layer."""
import copy
from pathlib import Path
import struct
import tempfile
import unittest

from .cases import load_cases, source_for
from .e2e import compare_native
from .program import ProgramImage, load_elf, compile_program
from .paths import ROOT
from .runner import check_program


def elf32(segments, entry=0x100):
    """Make a sectionless ELF fixture without calling the production toolchain."""
    ident = b'\x7fELF\x01\x01\x01' + bytes(9)
    result = bytearray(struct.pack('<16sHHIIIIIHHHHHH', ident, 2, 243, 1, entry,
                                   52, 0, 0, 52, 32, len(segments), 0, 0, 0))
    payload = bytearray()
    offset = 52 + 32 * len(segments)
    for address, data, memsz, flags in segments:
        result += struct.pack('<8I', 1, offset + len(payload), address, address,
                              len(data), memsz, flags, 1)
        payload += data
    return bytes(result + payload)


def store_observations(size=4, bus_value=17):
    """Minimal consistent observation pair; keep store data within one byte."""
    state = {f'top.cpu.rf.registers.{index}': 0 for index in range(32)}
    for name, length in [('if_id', 5), ('id_ex', 8), ('ex_mem', 16), ('mem_wb', 11)]:
        state['top.cpu.' + name] = [0] * length
    state.update({'top.cpu.fetch.pc': 4, 'top.cpu.writeback.retirement': [False] + [0] * 5,
                  'top.cpu.memory.store_event': [True, 4096, 17, size],
                  'top.cpu.memory.load_event': [False, 0, 0, 0],
                  'top.cpu.last_control': [False] * 7,
                  'top.cpu.memory.words': [0], 'top.cpu.writeback.retired': 0,
                  'top.cpu.system_event': [False] + [0] * 4})
    control = dict(stall=False, target=None, forward_a=0, forward_b=0, exmem_clear=False,
                   pc_enable=True, idex_enable=True, flush_ifid=False, flush_idex=False)
    raw = dict(pcs=[4, 0, 0, 0, 0], valid=[True, False, False, False, False],
               sequential_pcs=[0] * 4, ifid_expanded=0, mem_read=False,
               mem_address=4096, mem_value=0, mem_size=size)
    before = dict(control=control, raw=raw)
    after = dict(fetch_pc=4, raw=copy.deepcopy(raw), stalled=[False] * 5,
                 registers=[0] * 32, data=[0], retire=None, store=[4096, bus_value], retired=0)
    return [{'tick': 0, 'state': state}], [before, after], ProgramImage(bytes(8), isa='I')


class HarnessTests(unittest.TestCase):
    def setUp(self):
        temporary = ROOT / 'pycircuit/builds/benchmarks/test-runs'
        temporary.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=temporary)
        self.directory = Path(self.temporary.name)

    def tearDown(self):
        self.temporary.cleanup()

    def load(self, data):
        path = self.directory / 'test.elf'
        path.write_bytes(data)
        return load_elf(path, 'I')

    def test_load_segments_keep_addresses_and_zero_fill_bss(self):
        image = self.load(elf32([(0x100, b'\x13\0\0\0', 4, 5),
                                 (0x1004, b'\x2a\0\0\0', 12, 6)]))
        self.assertEqual(image.entry, 0x100)
        self.assertEqual(image.text, bytes(0x100) + b'\x13\0\0\0')
        self.assertEqual(image.ram, bytes(4) + b'\x2a\0\0\0' + bytes(8))

    def test_extra_allocated_section_is_loaded_from_real_elf(self):
        source = next(case for case in load_cases() if case['name'] == 'elf_extra_section')
        image = compile_program(source_for(source), self.directory, 'I')
        self.assertEqual(image.ram[:4], b'\x2a\0\0\0')

    def test_bad_segments_and_headers_fail_explicitly(self):
        text = (0x100, b'\x13\0\0\0', 4, 5)
        invalid = [b'not ELF', elf32([text])[:-1],
                   elf32([text, (8191, b'\1\2', 2, 6)]),
                   elf32([text, (4094, b'\1\2\3\4', 4, 4)]),
                   elf32([text, (0x100, b'\0', 1, 4)]),
                   elf32([text, (0x1000, b'\0\0', 1, 6)]),
                   elf32([text], entry=0x104)]
        wrong_machine = bytearray(elf32([text])); struct.pack_into('<H', wrong_machine, 18, 62)
        different_physical = bytearray(elf32([text])); struct.pack_into('<I', different_physical, 64, 0)
        invalid += [wrong_machine, different_physical]
        for data in invalid:
            with self.subTest(data=bytes(data[:20])):
                with self.assertRaises(ValueError):
                    self.load(data)

    def test_store_width_mutation_is_detected(self):
        trace, native, image = store_observations()
        compare_native(trace, native, image)
        trace[0]['state']['top.cpu.memory.store_event'][3] = 1
        with self.assertRaises(AssertionError) as error:
            compare_native(trace, native, image)
        self.assertEqual(error.exception.args[0]['field'], 'store_size')

    def test_native_width_and_trace_are_independent(self):
        trace, native, image = store_observations(1, 0x12340011)
        baseline = copy.deepcopy(native)
        compare_native(trace, native, image)
        compare_native(trace, native, image)
        self.assertEqual(native, baseline)
        native[0]['raw']['mem_size'] = 4
        with self.assertRaises(AssertionError):
            compare_native(trace, native, image)

    def test_manifest_references_all_existing_sources(self):
        cases = load_cases()
        regressions = {case['name']: case for case in cases if case['name'] in
                       ('audit_wrong_path_fetch', 'audit_store_then_print')}
        self.assertEqual(len(regressions), 2)
        self.assertTrue(all(case['group'] == 'boundary' for case in regressions.values()))
        self.assertEqual(sum(case['group'] == 'legacy' for case in cases), 9)
        for case in cases:
            self.assertTrue(source_for(case).strip(), case['name'])
        self.assertEqual(len({case['name'] for case in cases}), len(cases))

    def terminal_fixture(self, reason):
        trace, _, image = store_observations()
        state = trace[0]['state']
        word = 0x73 if reason == 'exit' else 0x100073
        kind = 1 if reason == 'exit' else 2
        event = [True, 0, kind, 93 if reason == 'exit' else 0, 42 if reason == 'exit' else 0]
        state['top.cpu.writeback.retirement'] = [True, 0, word, 0, False, 0]
        state['top.cpu.writeback.retired'] = 1
        state['top.cpu.system_event'] = event
        state['top.cpu.exit_status'] = 42
        oracle = dict(reason=reason, retired=[dict(pc=0, word=word, write=None)], registers=[0] * 32,
                      ram=[0] * 4, events=[event] if reason == 'exit' else [], status=42, output='')
        return trace, image, oracle

    def test_effects_after_exit_are_not_hidden_by_trace_trimming(self):
        trace, image, oracle = self.terminal_fixture('exit')
        check_program(trace, image, oracle)
        tail = copy.deepcopy(trace[0])
        tail['state']['top.cpu.writeback.retirement'][0] = False
        tail['state']['top.cpu.system_event'][0] = False
        tail['state']['top.cpu.memory.words'][0] = 1
        with self.assertRaisesRegex(AssertionError, 'RAM'):
            check_program(trace + [tail], image, oracle)

    def test_trap_kind_is_checked_independently_of_registers(self):
        trace, image, oracle = self.terminal_fixture('break')
        check_program(trace, image, oracle)
        trace[0]['state']['top.cpu.system_event'][2] = 3
        with self.assertRaises(AssertionError) as error:
            check_program(trace, image, oracle)
        self.assertEqual(error.exception.args[0]['field'], 'trap events')


if __name__ == '__main__':
    unittest.main()

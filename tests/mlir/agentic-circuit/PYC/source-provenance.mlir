// RUN: %split_file %s %t
// RUN: %pycc %t/good.pyc --emit=none
// RUN: %not %pycc %t/unmapped.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=UNMAPPED
// RUN: %python -c "from pathlib import Path; p=Path(r'%t/good.pyc'); s=p.read_text(); Path(r'%t/bad-specialization.pyc').write_text(s.replace(r'\22module_specializations\22:[]', r'\22module_specializations\22:[42]'))"
// RUN: %not %pycc %t/bad-specialization.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=BAD-MAP
// RUN: %python -c "from pathlib import Path; p=Path(r'%t/good.pyc'); s=p.read_text(); Path(r'%t/bad-root.pyc').write_text(s.replace(r'\22definition\22:null', r'\22unexpected\22:null'))"
// RUN: %not %pycc %t/bad-root.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=BAD-MAP
// RUN: %python -c "from pathlib import Path; p=Path(r'%t/good.pyc'); s=p.read_text(); Path(r'%t/bad-path.pyc').write_text(s.replace('src/model.py', './src/model.py'))"
// RUN: %not %pycc %t/bad-path.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=BAD-MAP
// RUN: %python -c "from pathlib import Path; p=Path(r'%t/good.pyc'); s=p.read_text(); old=r'\22kind\22:\22statement\22,\22line\22:7}]}]}'; new=r'\22kind\22:\22inline_callsite\22,\22line\22:7},{\22column\22:3,\22file\22:\22src/model.py\22,\22kind\22:\22statement\22,\22line\22:8}]}]}'; Path(r'%t/bad-stack.pyc').write_text(s.replace(old,new,1))"
// RUN: %not %pycc %t/bad-stack.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=BAD-MAP
// RUN: %python -c "from pathlib import Path; p=Path(r'%t/good.pyc'); s=p.read_text(); bad=r'{\22definition\22:\22leaf\22,\22name\22:\22instance\22,\22scope\22:\22/\22,\22source_provenance\22:{\22origins\22:[]},\22specialization\22:\22bogus\22}'; Path(r'%t/bad-fingerprint.pyc').write_text(s.replace(r'\22module_instances\22:[]', r'\22module_instances\22:['+bad+']'))"
// RUN: %not %pycc %t/bad-fingerprint.pyc --emit=none 2>&1 | %FileCheck %s --check-prefix=BAD-MAP

// UNMAPPED: src/model.py:8:3: error: [PYC986] PYC operation location stack is not present in `pyc.source_map`
// BAD-MAP: [PYC98{{[34]}}] `pyc.source_map`

//--- good.pyc
module attributes {
  pyc.frontend.contract = "pycircuit",
  pyc.source_map = "{\22blocks\22:[{\22expressions\22:[{\22kind\22:\22constant\22,\22nested\22:[],\22result\22:\22value\22,\22source_provenance\22:{\22origins\22:[{\22frames\22:[{\22column\22:3,\22file\22:\22src/model.py\22,\22kind\22:\22statement\22,\22line\22:7}]}]}}],\22index\22:0,\22kind\22:\22transform\22,\22name\22:\22out\22,\22source_provenance\22:{\22origins\22:[]},\22stable_id\22:\22\22}],\22contract_epoch\22:\220.5\22,\22definition\22:null,\22helpers\22:[],\22module_instances\22:[],\22module_specializations\22:[],\22schema\22:\22agentic-circuit-source-map\22,\22specialization\22:null,\22state_owners\22:[],\22system\22:\22source_test\22,\22table_matches\22:[],\22table_selections\22:[],\22version\22:\220.1\22}"
} {
  func.func @source_test() -> i8 attributes {
    arg_names = [], result_names = ["value"],
    pyc.value_params = [], pyc.value_param_types = [],
    pyc.kind = "module", pyc.inline = "false", pyc.params = "{}",
    pyc.base = "source_test",
    pyc.struct.metrics = "{\22ast_node_count\22:0,\22collection_count\22:0,\22collection_instance_count\22:0,\22estimated_inline_cost\22:0,\22hardware_call_count\22:0,\22instance_count\22:0,\22loop_count\22:0,\22module_call_count\22:0,\22module_family_collection_count\22:0,\22repeated_body_clusters\22:[],\22source_loc\22:0,\22state_alloc_count\22:0,\22state_call_count\22:0}",
    pyc.struct.collections = "[]"
  } {
    %value = pyc.constant 7 : i8 loc("src/model.py":7:3)
    func.return %value : i8
  }
}

//--- unmapped.pyc
module attributes {
  pyc.frontend.contract = "pycircuit",
  pyc.source_map = "{\22blocks\22:[{\22expressions\22:[{\22kind\22:\22constant\22,\22nested\22:[],\22result\22:\22value\22,\22source_provenance\22:{\22origins\22:[{\22frames\22:[{\22column\22:3,\22file\22:\22src/model.py\22,\22kind\22:\22statement\22,\22line\22:7}]}]}}],\22index\22:0,\22kind\22:\22transform\22,\22name\22:\22out\22,\22source_provenance\22:{\22origins\22:[]},\22stable_id\22:\22\22}],\22contract_epoch\22:\220.5\22,\22definition\22:null,\22helpers\22:[],\22module_instances\22:[],\22module_specializations\22:[],\22schema\22:\22agentic-circuit-source-map\22,\22specialization\22:null,\22state_owners\22:[],\22system\22:\22source_test\22,\22table_matches\22:[],\22table_selections\22:[],\22version\22:\220.1\22}"
} {
  func.func @source_test() -> i8 attributes {
    arg_names = [], result_names = ["value"],
    pyc.value_params = [], pyc.value_param_types = [],
    pyc.kind = "module", pyc.inline = "false", pyc.params = "{}",
    pyc.base = "source_test",
    pyc.struct.metrics = "{\22ast_node_count\22:0,\22collection_count\22:0,\22collection_instance_count\22:0,\22estimated_inline_cost\22:0,\22hardware_call_count\22:0,\22instance_count\22:0,\22loop_count\22:0,\22module_call_count\22:0,\22module_family_collection_count\22:0,\22repeated_body_clusters\22:[],\22source_loc\22:0,\22state_alloc_count\22:0,\22state_call_count\22:0}",
    pyc.struct.collections = "[]"
  } {
    %value = pyc.constant 7 : i8 loc("src/model.py":8:3)
    func.return %value : i8
  }
}

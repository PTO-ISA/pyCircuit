# C3 artifact-naming amendment：`.ac` 产物名跟随来源 Python 文件

状态：**用户指示的修订**（user-directed amendment），自 2026-09-29 起对新实现生效。
C3-C 冻结正文**不修改**；本页是其命名条款的修订记录，待下一次 C3 修订时并入。

## 来源

用户在 2026-09-29 明确指示：

> 不用，ac应该是和python的文件名一致

即：`.ac` 产物按来源 Python 文件名命名（`<stem>.ac`），不设保留标签。
`design_top.ac` 只是 `design_top.py` 这一 root 源文件的产物；
`test_increment.py` 产出 `test_increment.ac`。这与 C3-C 既有的逐单元
`<stem>.ac` / `<stem>.interface.ac` 命名一致。

## C3-C 冻结文本的 before / after

C3-C 现行 link/emit 段（`c3-driver-runtime.md`）：

```text
pycircuit link <unit-dir>... --top <qualified-module>
  [--parameters <bindings.json>] -o <program.ac> [--replace]

pycircuit emit <program.ac> --target cpp|verilog
  -o <generated-dir> [--replace]
```

按本修订应读作：

```text
pycircuit link <unit-dir>... --top <qualified-module>
  [--parameters <bindings.json>] -o <root-source-stem>.ac [--replace]

pycircuit emit <root-source-stem>.ac --target cpp|verilog
  -o <generated-dir> [--replace]
```

其中 `<root-source-stem>` 是拥有所选 root 的那份 Python 源文件的 stem。
实现者不得硬编码 `program.ac` 或任何保留文件名；driver 接受调用方给出的
输出路径，`--replace`/发布/恢复语义完全不变。

## 本修订**不**改变

- 不改 C3-C 的发布协议、journal 键、receipt/manifest schema 与
  `--replace` 无 `--force` 的规则；
- 不改 journal 中 `artifact:"source-unit"|"generated"|"program"` 的**种类**
  取值——`program` 是发布身份分类，不是文件名约定，本次不动；
- 不改任何 `ac.*` op/type/attribute、runtime ABI、SDK manifest 或
  generated.json 的 `kind`/`target`/`entry`/`files`/`source_groups` 形状；
- 不改 C3-C 冻结正文本身，也不追溯改写历史批准记录与哈希。

## 当前实现的一致性

- 私有桥 `acir-design-harness` 的 `--output` 始终由调用方给出，工具不发明名称；
- 私有测试与证据已按本规则命名（`test_increment.py` → `test_increment.ac`、
  `blinker.py` → `blinker.ac`、`counter.py` → `counter.ac`）；
- 公开 `pycircuit` driver 尚未实现（边界审计暂停中），因此本修订在其实现前
  落地没有兼容负担；实现时必须采用上面的 after 形式。

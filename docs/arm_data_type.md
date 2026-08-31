# ARM ASL 语言：基本数据类型、类型操作语法与基本运算

**依据**：Arm《Architecture Specification Language Readers' Guide》（文档号 111069，版本 A.a，2025-10-29）及 ASL1 正式语言定义。ASL1 是 Arm Architecture Reference Manual（DDI 0487，A-profile）中指令伪代码（pseudocode）所使用的规范语言。

## 1. 语言概貌

ASL1 的语言性质：

- **命令式、有可变状态**：代码是一系列步骤，可修改全局环境状态；
- **强静态类型**：类型在编译期检查，不一致即错误；**无 truthy/falsy**（`'1' == TRUE` 是编码错误）；
- **一阶语言**：函数只能全局定义，不能匿名定义或作为数据传递；
- **无指针/引用**：没有按引用传递、堆分配、指针运算。

区别于一般语言的特色能力：

- **位向量一等公民**：类型、字面量、切片表达式、专用运算符；
- **依赖值的类型**：位向量长度可以依赖 ASL 值（如 `Replicate{N,M}(x)` 返回 `bits(N)`）；
- **Accessor**：对体系结构状态读写的函数式抽象（getter/setter）；
- **无界整数与整数约束**、**无界精度有理数**、**非确定性**（`ARBITRARY`）。

注释：`// ...`（行注释）与 `/* ... */`（块注释，不可嵌套）。

**求值顺序**（要点）：赋值先求右侧再求左侧；子程序实参、元组、非短路二元运算、数组索引、位切片、record 构造、for 循环边界均**从左到右**求值；布尔 `&&`/`||`/`==>` 短路。

---

## 2. 基本数据类型

ASL1 共 9 类类型。**位向量是唯一的「具体」类型**——直接对应寄存器、内存与指令中被操纵的值；其余类型（整数、实数等）都是抽象的数学类型，伪代码通过显式转换在两者之间往返。

### 2.1 位向量 `bits(N)` / `bit`

```asl
bits(N)                                  // 长度为 N 的位向量类型
bit                                      // bits(1) 的同义词
bits(N) { [slice1, ..., sliceK] field, ... }   // 带命名位域的位向量类型
```

- 位向量是有限长的 0/1 序列，**每种长度都是不同的类型**；长度可以为 0；
- 位编号**从左到右为 N-1 downto 0**（最高有效位在左）；
- 位域（bitfield）为特定切片提供命名捷径，**位域之间可以重叠**：

```asl
type PSTATEType of bits(64) {
    [31] N, [30] Z, [29] C, [28] V,     // 条件标志
    [3:0] M                              // 模式域
};
```

### 2.2 整数 `integer`

```asl
integer            // 无约束整数
integer{...}       // 约束整数（约束写在花括号内）
integer{}          // 待定约束整数（约束由类型检查推导）
```

- ASL 整数是**无界的数学整数**（非机器整数），无溢出、无回绕；机器整数用位向量表示，通过 `UInt()`/`SInt()` 转换；
- **约束整数**限定取值集合，且约束随运算**自动传播**：

```asl
let x : integer{2,4,8} = ...;       // 只能取 2、4、8
let y : integer{0..3}  = ...;       // 0 到 3（含）
let z = x * y;                       // 类型为 integer{0,2,4,6,8,12,16,24}
let w : integer{} = x * y;           // 待定约束：从初始化表达式继承
```

### 2.3 实数 `real`

有限但无界大小与精度，对应**数学有理数**（既非浮点也非实数集）。机器浮点数用位向量表示，与 `real` 显式互转。

### 2.4 布尔 `boolean`

取值 `TRUE` / `FALSE`。**与 `bit` 严格区分**：`bit` 取 `'0'`/`'1'`，二者不可比较、不可混用。

### 2.5 枚举 `enumeration`

```asl
enumeration {RED, GREEN, BLUE}
```

- 必须**声明为命名类型**；至少一个命名值；命名值不能在不同枚举间共享；
- 惯例：命名值以「枚举类型名 + 下划线」开头（如 `SRType_LSL`）；
- **不可与整数互转**（与 C 不同）。

### 2.6 字符串 `string`

可打印字符（ASCII 32–126、制表符、换行符）的有序序列，主要用于诊断输出。

### 2.7 记录 `record`

```asl
record { shift : bits(2), amount : integer };
```

- 命名字段的集合，字段类型可不同、可嵌套结构化类型；
- 必须**声明为命名类型**（见 §4）。

### 2.8 元组（Tuple）

```asl
(bits(32), bit)          // 二元组类型
```

- 至少两个分量，类型可不同；常用作多返回值函数的返回类型，如 `AddWithCarry()` 返回 `(bits(N), bits(4))`（结果 + 条件标志）。

### 2.9 数组 `array`

```asl
array [[31]] of bits(64)      // 整数索引：0..30，元素为 bits(64)
array [[Color]] of integer    // 枚举索引：按 Color 的命名值索引
```

- 定长、单一元素类型（可结构化）；索引只能是整数或枚举；至少一个元素；**没有数组字面量语法**。

---

## 3. 字面量（Literals）

| 类型 | 语法 | 示例 |
|------|------|------|
| 布尔 | `TRUE` / `FALSE` | `TRUE` |
| 实数 | 必须带小数点 | `0.0`、`3.14`（`0` 是整数，`0.0` 才是实数） |
| 位向量 | 单引号内的 0/1 序列，可加空格 | `'0'`、`'1'`、`'11 11'` |
| 枚举 | 直接写命名值 | `RED` |
| 字符串 | 双引号；转义 `\t \n \" \\` | `"hello world!\n"` |
| 整数（十进制） | 可带下划线分隔 | `0`、`15`、`-12_34` |
| 整数（十六进制） | `0x` 前缀；无负号即为正 | `0x55`、`-0x80000000` |

> 整数字面量默认是**约束整数类型**（`let x = 15;` 中 `x : integer{15}`），可显式标注为无约束：`let z : integer = 3;`。

---

## 4. 类型声明与子类型

### 4.1 命名类型

```asl
type A of integer;
type B of integer;
```

每个命名类型声明都创建**互不兼容**的新类型——即使底层类型相同，`A` 与 `B` 的值也不可相互赋值（强命名类型语义）。枚举类型声明会同时声明每个命名值的标识符。

### 4.2 子类型 `subtypes`

```asl
type Shape1 of integer;
type Square subtypes Shape1;          // Square 是 Shape1 的子类型
// 等价：type Square of integer subtypes Shape1;

shape1 = square;                      // ✅ 子类型可替换父类型
// square = shape1;                   // ❌ 反向不成立
```

命名类型与描述其结构的**原始类型**（不含命名类型）互为子类型，因此允许把字面量赋给命名类型标识符：

```asl
type BV4 of bits(4);
var z : BV4 = '0000';       // ✅
var y : bits(4) = bv4;      // ✅
```

### 4.3 记录子类型扩展 `with`

```asl
type Coord2 of record { x : real, y : real };
type Coord3 subtypes Coord2 with { z : real };    // 扩展新字段
```

### 4.4 位向量子类型

位向量子类型**不能用 `with`**，必须重新列出父类型全部位域：

```asl
type BV1 of bits(4) { [0] fieldA };
type BV2 of bits(4) { [0] fieldA, [1] fieldB } subtypes BV1;
```

---

## 5. 数据操作语法

### 5.1 位向量切片（5 种切片形式）

语法为 `x[slice]`，切片结果是新的位向量：

| 形式 | 语法 | 语义 |
|------|------|------|
| 基本长度形式 | `x[lsb +: len]` | 从 lsb 开始、长 len 的切片，类型 `bits(len)` |
| 长度简写 | `x[:len]` | 等价 `x[0 +: len]` |
| 单比特 | `x[index]` | 等价 `x[index +: 1]` |
| 范围形式 | `x[msb:lsb]` | msb downto lsb（要求 `lsb <= msb`），等价 `x[lsb +: msb-lsb+1]` |
| 缩放形式 | `x[index *: len]` | 等价 `x[(index*len) +: len]`（按元素粒度索引） |

**多重切片**：`x[slice1, slice2, ..., sliceN]` 等价于各切片结果的拼接 `x[slice1] :: x[slice2] :: ... :: x[sliceN]`。

**整数切片**：切片也可作用于整数——把整数视为足够长的（符号扩展的）二补码位向量：

```asl
42[1] == '1'
8[:4] == '1000'
33[7:0] == '00100001'
(-20)[:7] == '1101100'
```

### 5.2 位域与记录字段访问

```asl
x.fld              // record 字段访问；或位向量的命名位域（即对应切片）访问
x.[fld1, fld2]     // 位域拼接访问：等价 x.fld1 :: x.fld2
```

**记录构造**（所有字段必须给全，顺序任意）：

```asl
RecordName { fld1 = expr1, fld2 = expr2, ... }
```

### 5.3 元组构造与提取

```asl
(expr1, ..., exprN)              // 元组构造
tup.item0, tup.item1, ...        // 零起始的分量提取：(1, 2.0, TRUE).item1 == 2.0
```

### 5.4 数组索引

```asl
arr[[idx]]        // 注意双方括号；整数索引须满足 0 <= idx < len，越界是编码错误
```

### 5.5 断言式类型转换（ATC，`as`）

```asl
expression as ty
```

同时完成两件事：**断言**表达式属于该类型（不满足即编码错误）+ **转换**表达式的静态类型。常用于收窄约束整数：

```asl
let x : integer{2,4,8} = ...;
let y = x as integer{2};       // 若 x != 2 则报错；y : integer{2}
```

### 5.6 任意值表达式

```asl
ARBITRARY : ty      // 产生该类型的某个未指定值（非确定性）；软件不得依赖其取值
```

### 5.7 模式匹配（`IN` 与位掩码）

```asl
expression IN  {pattern1, pattern2, ...}      // 命中任一 pattern → TRUE
expression IN !{pattern1, pattern2, ...}      // 命中任一 pattern → FALSE
```

六种 pattern 形式：

| 形式 | 语法 | 匹配 |
|------|------|------|
| 通配 | `-` | 任何值 |
| 单值 | `expr` | 等于该值 |
| 上界 | `<= expr` | ≤（整数） |
| 下界 | `>= expr` | ≥（整数） |
| 区间 | `e1 .. e2` | 闭区间（整数） |
| 元组 | `(p1, p2, ...)` | 逐分量匹配 |
| 位掩码 | `'1xx0'`、`'1(0)x0'` | `x` 为忽略位；括号内的 0/1 也视为忽略位 |

位掩码有专用等式简写：`x == bit_mask` ≡ `x IN {bit_mask}`；`x != bit_mask` ≡ `x IN !{bit_mask}`。例如以下写法等价，均匹配 `'1000' '1010' '1100' '1110'`：

```asl
opcode == '1xx0';
opcode == '1(0)x0';
opcode == '1(01)0';
```

---

## 6. 运算符与基本运算

### 6.1 关系运算符（结果为 `boolean`）

| 运算符 | 操作数 | 说明 |
|--------|--------|------|
| `==` `!=` | 同类型的 boolean / 位向量 / 整数 / 实数 / 枚举 / 字符串 | 相等 / 不等 |
| `<` `<=` `>` `>=` | 整数或实数 | 大小比较（**位向量不能直接比大小**，须先 `UInt`/`SInt`） |

### 6.2 布尔运算符（操作数与结果均为 `boolean`）

| 运算符 | 语义 |
|--------|------|
| `!x` | 逻辑非 |
| `x && y` | 逻辑与（**短路**） |
| `x \|\| y` | 逻辑或（**短路**） |
| `x ==> y` | 逻辑蕴含（短路：x 为 FALSE 直接 TRUE） |
| `x <=> y` | 逻辑等价（≡ `x == y`） |

### 6.3 位向量运算符

设 `x, y : bits(N)`、`z : bits(M)`：

| 运算符 | 结果类型 | 语义 |
|--------|----------|------|
| `NOT x` | `bits(N)` | 按位取反 |
| `x AND y` / `x OR y` / `x XOR y` | `bits(N)` | 按位与 / 或 / 异或（同长度） |
| `x :: z` | `bits(N+M)` | 拼接（左到右） |

> 注意：布尔用 `&& || !`，位向量用 `AND OR NOT XOR`——关键字大小写即类型纪律。

### 6.4 算术运算符

伪代码算术主要在整数/实数上进行（无界，无溢出问题），必要时与位向量显式互转。

| 运算符 | 操作数 → 结果 | 语义 |
|--------|--------------|------|
| `-x` | int→int，real→real | 取负（约束随之更新） |
| `x + y`, `x - y` | int/real 同型 | 加减 |
| | `bits(N) ± bits(N) → bits(N)` | 取整数加减结果的低 N 位（有/无符号解释结果一致）：`x+y = (UInt(x)+UInt(y))[:N]` |
| | `bits(N) ± integer → bits(N)` | `x + y[:N]`（如 `'0000' + 4`） |
| `x * y` | int×int→int；含 real→real | 乘法 |
| `x / y` | real/real→real | 有理数除法；除零是编码错误 |
| `x DIV y` | int | **精确除法**：要求整除，否则编码错误 |
| `x DIVRM y` | int | 向下取整除法（floor division） |
| `x MOD y` | int | 余数：`x - y*(x DIVRM y)`；y ≤ 0 是编码错误 |
| `x ^ n` | int^int→int；real^int→real | 幂（int 底数时 n 不得为负） |
| `x << n`, `x >> n` | int, int→int | **整数移位**：`x * 2^n` / `x DIVRM 2^n`（n 非负）。位向量移位用标准库 `LSL`/`LSR`/`ASR` |

### 6.5 优先级（低 → 高）

| 优先级 | 运算符 |
|--------|--------|
| 最低 | `\|\|` `&&` `==>` `<=>`、`as`（ATC） |
| | `==` `!=` |
| | `>` `>=` `<` `<=` |
| | `+` `-` `OR` `XOR` `AND` `::` |
| | `*` `DIV` `DIVRM` `/` `MOD` `<<` `>>` |
| | `^` |
| | 一元 `!` `-` `NOT` |
| 最高 | `IN`（模式匹配） |

**强制括号规则**：形如 `x op1 y op2 z` 的表达式，除非 op1/op2 优先级不同，或 op1 == op2 且可结合，否则**必须加括号**。可结合运算符仅有：`+ * && || AND OR XOR <=> ::`。

```asl
i > 0 && j > 0 && k > 0     // ✅ 不同优先级
i + j + k                    // ✅ + 可结合
i - j - k                    // ❌ - 不可结合，须写 (i - j) - k
i > 0 && j > 0 || k > 0      // ❌ && 与 || 同级，须加括号
```

### 6.6 条件表达式

```asl
if t then x else y      // t : boolean；x/y 须有公共祖先类型
```

---

## 7. 变量与常量声明

### 7.1 四种标识符声明

| 关键字 | 可变性 | 用途 |
|--------|--------|------|
| `var` | 可赋值 | 一般可变状态 |
| `let` | 不可变 | 局部/全局不再更新的值 |
| `constant` | 不可变 | 体系结构常量 |
| `config` | 不可变（类型标注必写） | 实现固定、跨实现可变的配置状态 |

```asl
var a = 1;                       // 类型从初始化表达式推断（integer{1}）
var b : real = 2.0;              // 显式标注（与初始化类型冲突是编码错误）
var c : bits(3);                 // 只标注类型，未初始化（使用前应显式赋值）
var d, e : integer;              // 多标识符

let g = 4;
constant i = 6;
config k : boolean = TRUE;
```

### 7.2 元组解构与丢弃

局部 `var`/`let` 可用元组初始化并用 `-` 丢弃分量：

```asl
let (res, -) = AddWithCarry{64}(x, y, carry_in);   // 丢弃标志位
var (a, -, c) : (integer, real, boolean) = (1, 2.0, TRUE);
- = SideEffecting();                                // 只为副作用调用，丢弃返回值
```

### 7.3 赋值

```asl
assignable = expression;
```

可赋值表达式包括：`var` 标识符、accessor 的 setter 调用、数组索引、record/位域字段访问、位切片、以及它们的元组组合。多字段形式：

```asl
x.(fld1, fld2) = '110' :: y;      // 等价 (x.fld1, x.fld2) = ...
PSTATE.[N,Z,C,V] = '0011';        // 同时写多个位域
```

对 accessor 局部赋值具有**读-改-写**语义（先 getter、修改、再 setter）。

### 7.4 与类型相关的语句

```asl
assert expression;      // 断言 boolean 表达式为 TRUE，否则编码错误
unreachable;            // 执行到即编码错误
pass;                   // 空语句
```

---

## 8. 与数据类型相关的标准库函数

标准库完整定义见 ASL 参考实现（herdtools7 `stdlib.asl`）。子程序可带**参数（parameters）**——写在花括号中的整数输入，用于确定位向量长度（类似泛型）：`Zeros{N}() => bits(N)`。标准库调用时可从实参类型推断而省略参数。

### 8.1 位向量 ↔ 整数转换（最核心）

| 函数 | 签名 | 语义 |
|------|------|------|
| `UInt` | `UInt{N}(x: bits(N)) => integer{0..2^N-1}` | 无符号解释 |
| `SInt` | `SInt{N}(x: bits(N)) => integer{-2^(N-1)..2^(N-1)-1}` | 二补码有符号解释 |
| `Real` | `Real(x: integer) => real` | 整数 → 有理数 |
| `RoundUp` / `RoundDown` | `(x: real) => integer` | 向上 / 向下取整 |

### 8.2 位向量构造与宽度变换

| 函数 | 语义 |
|------|------|
| `Zeros{N}()` / `Ones{N}()` | 全 0 / 全 1 的 `bits(N)` |
| `ZeroExtend{N,M}(x)` / `SignExtend{N,M}(x)` | 零 / 符号扩展到 N 位（M ≤ N） |
| `Extend{N,M}(x, unsigned)` | 按布尔选择零/符号扩展 |
| `Replicate{N,M}(x)` | 复制 x 拼满 N 位（N 整除 M） |

### 8.3 位向量移位与旋转（区别于整数 `<<` `>>`）

| 函数 | 语义 |
|------|------|
| `LSL{N}(x, sz)` / `LSL_C` | 逻辑左移；`_C` 变体额外返回最后移出的位 |
| `LSR{N}(x, sz)` / `LSR_C` | 逻辑右移 |
| `ASR{N}(x, sz)` / `ASR_C` | 算术右移（补符号位） |
| `ROL{N}(x, sz)` / `ROL_C`、`ROR{N}(x, sz)` / `ROR_C` | 左旋 / 右旋 |

### 8.4 位查询与判断

| 函数 | 语义 |
|------|------|
| `BitCount{N}(x) => integer{0..N}` | popcount |
| `HighestSetBit` / `LowestSetBit`（及 `NZ` 变体） | 最高/最低置位位置 |
| `CountLeadingZeroBits` / `CountLeadingSignBits` | 前导零 / 前导符号位计数 |
| `IsZero{N}(x)` / `IsOnes{N}(x)` | 全 0 / 全 1 判断 |
| `IsEven(x)` / `IsOdd(x)` / `IsPow2(x)` | 整数奇偶 / 2 的幂判断 |

### 8.5 整数/实数算术辅助

| 函数 | 语义 |
|------|------|
| `Abs` / `Min` / `Max` | 绝对值 / 最小 / 最大（int 与 real 重载） |
| `CeilLog2` / `FloorLog2` / `ILog2` | 对数（上取整 / 下取整 / real 版） |
| `FloorPow2(x)` | ≤ x 的最大 2 的幂 |
| `AlignDownP2` / `AlignDownSize` / `AlignUpSize` | 按 2 的幂 / 任意尺寸对齐（int 与 bits 重载） |
| `SqrtRounded(x, nbits)` | 指定精度的平方根（round-to-odd） |

### 8.6 典型使用模式

指令伪代码的标准套路：**位向量解包 → 无界整数运算 → 打包回位向量**：

```asl
// AddWithCarry：ASL 数据类型体系的教科书示例
func AddWithCarry{N}(x: bits(N), y: bits(N), carry_in: bit) => (bits(N), bits(4))
begin
    let unsigned_sum : integer = UInt(x) + UInt(y) + UInt(carry_in);
    let signed_sum   : integer = SInt(x) + SInt(y) + UInt(carry_in);
    let result : bits(N) = unsigned_sum[:N];          // 整数切片打包回位向量
    let n : bit = result[N-1];
    let z : bit = if IsZero(result) then '1' else '0';
    let c : bit = if UInt(result) == unsigned_sum then '0' else '1';
    let v : bit = if SInt(result) == signed_sum   then '0' else '1';
    return (result, n :: z :: c :: v);                // 元组 + 拼接
end;
```

---

## 9. ASL 与 PyCircuit V6 对照分析：类型整合、语法借鉴

ASL 是**行为规范语言**（描述指令做什么），PyCircuit 是**硬件构造语言**（描述电路是什么，每个运算都对应硬件代价）。两者定位不同，但类型体系与操作语法高度可比。本章回答四个问题：基础数据类型与访问语法能否整合一致？哪种定义更合理？基础运算是否一致？PyCircuit 应吸纳 ASL 的哪些语法优势？

### 9.1 基础数据类型对照与可整合性

| 关注点 | ASL1 | PyCircuit V6 | 可整合性判断 |
|--------|------|--------------|--------------|
| 机器值 | `bits(N)`，每种长度一个类型 | `CycleAwareSignal`（width=N）+ 周期标签 | **已一致**。两者都是"定宽位向量为唯一具体类型"；PyCircuit 额外携带 cycle（时间维度，ASL 无此概念） |
| 无界计算域 | 运行时 `integer`/`real`（无溢出） | **展开期** Python `int`/`float`（无溢出） | **概念同构、时机不同**（见 9.2 分析） |
| 布尔与位 | `boolean` 与 `bit` 类型级分离 | i1 信号兼作条件；但**禁止**信号作 Python `bool` | 纪律等效：ASL 禁 `'1'==TRUE`，PyCircuit 禁 `if sig:`——都在语言层阻断"位/真值混用"这一类错误 |
| 位域 | `bits(N) { [31] N, [3:0] M }`，可重叠 | 无单信号位域视图（Record 是端口级） | **建议吸纳**（见 9.5-①） |
| 结构化 | `record`（命名类型） | `RecordSpec` → 扁平端口 | 语义等效；PyCircuit 多一层"展开到端口"的实现约定 |
| 元组 | `(bits(N), bits(4))` + `.item0` | Python tuple / 输出 dict | Python 原生已覆盖（含 `-` 丢弃 ≈ `_`） |
| 数组 | `array[[len]] of ty`、枚举索引 | Python list（展开期）/ `Vec`（硬件） | PyCircuit 更强：`Vec` 是可综合的硬件容器，ASL 数组只是规范数据结构 |
| 枚举 | 命名类型，**不可与整数互转** | 裸 int 常量 | **建议吸纳**（见 9.5-④） |
| 约束整数 | `integer{0..3}`，约束随运算传播 | 位宽即约束（`width` 是唯一约束机制） | 不必照搬：对 HDL 而言 width 传播已承担同一职责；展开期参数可用 Python 断言 |
| 长度参数化 | `{N}` 参数（依赖值的类型） | keyword-only 配置参数（元编程展开） | 等效。ASL 在类型系统内实现，PyCircuit 靠 Python 展开——后者表达力更强、检查更晚 |
| 非确定性 | `ARBITRARY : ty` | 无 | 定位差异：规范需要留白，硬件必须确定 |

**核心洞察：两者其实共享同一个"双域模型"，只是切分位置不同。**

```text
ASL（规范，单一时间轴＝指令执行）:
   bits(N) ──UInt/SInt──▶ 无界 integer 运算 ──[:N] 切片──▶ bits(N)
   （机器域）                （数学域，运行时）              （机器域）

PyCircuit（硬件，双时间轴＝展开期 + 硬件运行期）:
   Python int/for/if ──cas()/const()──▶ CAS 定宽运算 ──wire_of()──▶ 端口
   （数学域，展开期＝生成电路结构）      （机器域，运行期＝电路本身）
```

ASL 的「数学域」发生在**指令执行时**（描述语义）；PyCircuit 的「数学域」发生在**电路展开时**（生成结构）。这是定位决定的，**不应该也不可能完全归并**：若 PyCircuit 在运行期引入无界整数，每个运算的硬件代价就不再显式——这恰恰违背 HDL 的第一原则。

### 9.2 访问语法对照：大部分已经一致

对照 ASL 的 5 种切片形式（§5.1），PyCircuit **已覆盖其中 3 种**，且各有对应实现：

| ASL 形式 | ASL 语法 | PyCircuit 现状 | 一致性 |
|----------|----------|----------------|--------|
| 单比特 | `x[i]` | `x[i]` | ✅ 完全一致 |
| 范围（含两端） | `x[msb:lsb]`（降序、闭区间） | `x.slice(high, low)`（CAS 方法，闭区间） | ✅ 语义一致，拼写不同 |
| 基本长度 | `x[lsb +: len]` | `Wire.slice(lsb=, width=)` | ✅ 语义一致（keyword 形式） |
| 长度简写 | `x[:len]` | `x[0:len]`（Python 半开） | ✅ 恰好等价 |
| 缩放形式 | `x[idx *: len]` | 需手写 `x[i*len:(i+1)*len]` | ⚠️ 无糖，建议吸纳（9.5-②） |
| 多重切片拼接 | `x[a, b, c]` | `cat(x[a], x[b], x[c])` | ✅ 显式 `cat` 反而更清晰 |
| 位域访问 | `x.fld`、`x.[f1,f2]` | 无（仅 Record 端口级） | ❌ 缺失，建议吸纳（9.5-①） |

**唯一真正的语法冲突**是 Python 下标 `x[a:b]` 的方向约定：

- ASL / Verilog：`x[7:0]` = 降序闭区间 = 低 8 位（硬件工程师直觉）；
- PyCircuit：`x[0:8]` = Python 半开区间 = 低 8 位（软件工程师直觉）。

两者取低 8 位的结果相同，但 `x[7:0]` 在 PyCircuit 中会因 `stop < start` 直接报错——这是**好的设计**：与其静默给出另一种解释，不如让两种直觉的冲突显式暴露。**结论：不要改 `__getitem__` 的方向语义**（会破坏 Python 生态直觉且无法两全），而是让 ASL 习惯者使用已有的 `slice(high, low)` 方法，两种拼写各自服务各自的读者。

**整合结论**：访问语法**不需要也不应该强行统一为一种**；应做的是把 PyCircuit 缺失的两块（位域视图、缩放切片）补齐，使 ASL 的每一种访问形式在 PyCircuit 中都有**语义等价、Python 原生**的拼写。

### 9.3 基础运算对照：机器域运算高度一致

| 运算 | ASL（bits 域） | PyCircuit | 一致性 |
|------|---------------|-----------|--------|
| 加减 | `bits(N) ± bits(N) → bits(N)`（截断，签名无关） | `+ -`（同宽截断） | ✅ 语义完全相同 |
| 位逻辑 | `AND OR XOR NOT`（关键字） | `& \| ^ ~`（Python 运算符） | ✅ 语义相同，拼写不同 |
| 拼接 | `x :: y` | `cat(x, y)` | ✅ |
| 相等 | `== !=`（仅同型） | `== !=` | ✅ |
| 大小比较 | bits **不可直接比较**，须 `UInt(x) < UInt(y)` 或 `SInt` | `<`（按 `signed` 标记选 ult/slt）；亦有显式 `.ult() .slt() .ugt() ...` | ⚠️ 见下 |
| 乘法 | bits 域**无乘法**（只在 integer 域） | `*`（截断到操作数宽度） | ⚠️ PyCircuit 更"硬件"：乘法器是真实电路 |
| 移位 | 整数域 `<< >>`；bits 用库函数 `LSL/LSR/ASR/ROL/ROR` | `<< >>`（右移按 signed 选算术/逻辑） | ✅ 覆盖（rotate 需手写 cat） |
| 除法 | `DIV`（精确）/ `DIVRM`（floor）/ `MOD`，仅整数域 | `// %`（按 signed 选 udiv/sdiv） | 定位差异：ASL 用于语义定义，PyCircuit 生成真实除法器 |
| 条件 | `if t then x else y`（表达式） | `mux(t, x, y)` | ✅ |
| 短路 `&& \|\|` | 布尔域短路求值 | 无意义（组合电路两支路都存在） | 定位差异，无需对齐 |

**关于比较运算的签名处理**——这是唯一值得斟酌的差异：

- ASL 强制写 `UInt(x) < UInt(y)`，签名意图**在调用点可见**；
- PyCircuit 的 `a < b` 依赖操作数身上的 `signed` 标记，调用点看不出签名——标记在远处 `as_signed()` 设置时，阅读者需要回溯。

好在 PyCircuit **已同时提供** ASL 风格的显式方法（`.ult()`/`.slt()`/`.uge()` 等）。建议在编程规范中**推荐**：凡签名不能从上下文一眼看出的比较，用显式方法而非运算符——这是零成本吸纳 ASL 优点（只需文档约定，无需改实现）。

### 9.4 哪种定义更合理？——按用途分野

| 评价维度 | 更合理的一方 | 理由 |
|----------|--------------|------|
| 定义**指令语义**（规范） | **ASL** | 无界整数消灭隐式溢出/回绕，所有截断在类型转换处显式；`boolean`/`bit` 分离、枚举不可转整数，把一整类规范错误变成编译错误 |
| 描述**硬件结构**（构造） | **PyCircuit** | 每个运算即电路：定宽截断语义 = 硬件真实行为；cycle 标签把时序纳入类型系统（ASL 完全没有时间维度）；`Vec`/Record 直接对应可综合结构 |
| 展开期元编程 | **PyCircuit** | 完整 Python 表达力 vs ASL 受限的 `{N}` 参数机制 |
| 错误的暴露时机 | **ASL** | 强命名类型 + 约束传播在类型检查期抓错；PyCircuit 部分错误要到展开期甚至仿真期才暴露 |

结论：**两种定义在各自领域都是合理的，且互为镜像**——ASL 把数学域放在运行时（因为它描述"应该算出什么"），PyCircuit 把数学域放在展开期（因为它描述"电路长什么样"）。正确的整合方向不是二选一，而是：PyCircuit 保持机器域语义不动，**吸收 ASL 在"显式性"上的语法投资**（位域命名、位掩码匹配、显式签名比较、断言式转换）。

### 9.5 建议 PyCircuit 吸纳的 ASL 语法（按价值排序）

**① 单信号位域视图（对应 ASL `bits(N) {...}`）——价值最高**

解码器/系统寄存器代码充满 `instr[25:21]` 这类魔法数字。建议增加位域 Spec，允许重叠字段（ASL 特性，同一寄存器的多种视图）：

```python
# 提议 API
INSTR = BitfieldSpec(width=32, fields={
    "opcode": (31, 26), "rd": (25, 21), "imm16": (15, 0),
    "imm26":  (25, 0),                  # 与 rd/imm16 重叠——合法（视图不同）
})
f = INSTR.view(instr)          # f["opcode"] ≡ instr.slice(31, 26)
wr = INSTR.update(instr, rd=new_rd)     # 读-改-写（对应 ASL accessor 局部赋值语义）
```

实现成本低（纯前端语法糖，展开为现有 slice/cat），可读性收益大。

**② 位掩码模式匹配（对应 ASL `'1xx0'` 与 `IN`）——解码器刚需**

```python
# 提议 API：编译为 (sig & mask) == value，一条 and + 一条 eq
hit  = opcode.matches("1xx0")              # x 为忽略位
sel  = opcode.in_({"000x", "0010", "11xx"})   # 命中集合 → or 归约
```

ASL 用它把指令编码表写得与架构手册一字不差；PyCircuit 解码器可获得同样的对照可读性。硬件代价完全透明（掩码比较）。

**③ 缩放切片（对应 ASL `x[idx *: len]`）**

```python
lane = bus.lane(i, width=8)     # ≡ bus[i*8:(i+1)*8]，i 可为 Python int（展开期）
```

配合 `Vec` 已覆盖大部分场景，此糖服务"扁平总线按元素取"的残余场景。

**④ 类型安全枚举（对应 ASL `enumeration`）**

```python
class SRType(PycEnum):          # 提议：展开为最小宽度常量，展开期做类型检查
    LSL = auto(); LSR = auto(); ASR = auto(); ROR = auto()

alu_op = m.input("op", enum=SRType)     # width 自动 = 2
hit = alu_op.is_(SRType.LSL)            # 与裸 int 比较 → 展开期报错
```

把 ASL"枚举不可与整数互转"的防错价值搬到展开期：拼错枚举、宽度不够、跨枚举比较都在生成电路前抓住。

**⑤ 断言式类型转换（对应 ASL `expression as ty`）**

```python
y = x.assert_fits(width=4)     # 提议：仿真期 pyc.assert(x < 16) + trunc(4)
```

PyCircuit 现有 `trunc()` 是静默截断；ASL 的 ATC 语义（断言 + 转换）用一条 `pyc.assert` 即可承载，把"我确信高位为零"的设计意图变成可仿真检查的契约。

#### 文档级约定（零实现成本）

- 签名敏感的比较推荐 `.ult()/.slt()` 显式方法（对应 ASL 强制 `UInt/SInt`）；
- `reduce_sum` 等易溢出处显式写 `width=`（对应 ASL"截断必须显式"哲学）；
- 常量集中用 `constant` 风格命名（对应 ASL `constant`/`config` 的意图分层）。

**不建议吸纳的**：约束整数（width 已承担该职责）、运行时无界整数（违背硬件代价显式原则）、`ARBITRARY`（硬件需确定性；X 值建模是仿真器议题而非语言议题）、短路布尔（组合电路无短路）。

---

## 参考资料

- Arm《Architecture Specification Language Readers' Guide》，文档号 111069，版本 A.a（2025-10-29）
- ASL1 正式语言定义与参考实现：herdtools7 项目（`asllib/libdir/stdlib.asl`）
- Arm Architecture Reference Manual for A-profile（DDI 0487）——ASL 伪代码的使用场景

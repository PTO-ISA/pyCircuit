# Struct 与成员路径设计

本文记录拟采用的重构方案，不代表当前框架已经实现。文中的 C++ 接口是设计示例；具体 Python 作者接口另行确定。

Struct 负责表达有类型的数据及其嵌套字段。Queue 的持久状态、延迟赋值动作和提交过程见 [queue.md](queue.md)。

## 数据表示

`@ac.struct` 定义复合 Entry，允许按值嵌套其他 struct。Entry 的值可作为 `ac.var` 参与组合计算，持久性由保存它的 Queue 提供。

GFSim 中使用普通 C++ struct 和 public 字段，各字段保留对应的 C++ 类型：

```cpp
struct Meta {
    UInt<1> ready{};
    UInt<8> epoch{};
};

struct Entry {
    Meta meta{};
    UInt<32> value{};
};
```

正常读取和局部组合值赋值使用普通成员访问：

```cpp
auto epoch = entry.meta.epoch;
entry.meta.epoch = UInt<8>{4};
```

对局部对象的赋值立即改变该局部对象。Queue 的 current 是本 tick 的持久状态快照，Work 通过只读接口观察它，状态修改通过 proposal 延迟到 Xfer。

## 用成员指针路径定位字段

数据成员指针表示某种对象中的哪个字段，不绑定具体对象：

```cpp
auto outer = &Entry::meta;
auto inner = &Meta::epoch;

(target.*outer).*inner = UInt<8>{4};
// 等价于 target.meta.epoch = UInt<8>{4};
```

嵌套路径从外到内排列：

```text
&Entry::meta, &Meta::epoch
Entry → meta → epoch
```

路径作为模板参数传给 Queue：

```cpp
queue.proposeRevise<&Entry::meta, &Meta::epoch>(ruleId, UInt<8>{4});
```

编译器根据前端字段表达式生成路径，新值先按目标字段类型计算。Python 作者不需要显式书写 C++ 成员指针。

## 通用字段访问函数

```cpp
template<auto Member, auto... Rest, class Object>
decltype(auto) fieldAt(Object &object) {
    if constexpr (sizeof...(Rest) == 0)
        return (object.*Member);
    else
        return fieldAt<Rest...>(object.*Member);
}
```

该函数返回真实字段的引用，因此可以赋值：

```cpp
fieldAt<&Entry::meta, &Meta::epoch>(target) = UInt<8>{4};
```

处理过程是先得到 `target.meta` 的引用，再得到其 `epoch` 字段的引用。更深嵌套只需增加成员路径。

递归层数和每层成员在模板实例化时确定，优化后可以成为直接的嵌套成员访问。不同层成员指针的 C++ 类型由模板保留，runtime 不需要保存或遍历字段名。

## 修改范围

| 范围 | 生成的 C++ 调用示例 |
| --- | --- |
| 单个字段 | `queue.proposeRevise<&Entry::value>(ruleId, UInt<32>{42})` |
| 嵌套字段 | `queue.proposeRevise<&Entry::meta, &Meta::epoch>(ruleId, UInt<8>{4})` |
| 整个子结构 | `queue.proposeRevise<&Entry::meta>(ruleId, nextMeta)` |
| 整个元素 | `queue.proposeRevise<>(ruleId, nextEntry)` |

每行是独立用法示例，`ruleId` 是当前 Rule 的来源身份。空路径表示整值替换，也适用于标量 Queue，由 `proposeRevise` 单独处理。`fieldAt` 只用于非空路径。

同 tick 不重叠字段修改可以共同生效。相同字段、父子字段或整项替换与局部修改重叠，属于初版不保证结果的竞争情况。

## 延迟赋值的职责划分

```text
成员路径：编译时确定修改哪个字段
新值：    Work 计算并按值保存
目标对象：Xfer 传入 Queue 的旧队尾
```

Queue 用 `std::function<void(Element &)>` 保存赋值动作。动作内部使用 `fieldAt` 取得引用并执行赋值；struct 本身只提供普通字段。具体记录和调用代码见 `queue.md`。

## 支持范围与约束

- 初版支持按值嵌套的 struct 数据成员路径，每层对象与成员指针类型必须匹配，最终字段必须可赋值。
- 字段必须可以形成数据成员指针；C++ bit-field 不适用。
- 初版路径不包含运行时数组索引、指针解引用或可选对象解包。
- 生成代码提供目标字段确切类型的新值，维持 AC 类型的位宽和值语义。
- 当前 `std::function` 方案要求捕获的新值可复制构造；示例的赋值从捕获值的 const 引用读取，因此目标还需支持相应赋值。
- 对象生命周期通过正常 C++ 构造、赋值和析构维护。具体允许的类型由 AC 类型系统决定，提交阶段的赋值还需要满足无失败约束。
- `fieldAt` 本身是立即访问工具，不能在 Work 中借此绕过 Queue 的只读 current 契约。

每条用到的路径可能产生模板实例和赋值 lambda。生成源码保持统一，实际机器码大小与性能需要在实现后测量。

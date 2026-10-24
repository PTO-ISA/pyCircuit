# ruff: noqa: F823, F841 -- Module state captured by Rules is compiler-owned.
import pycircuit as ac


@ac.module
def Counter():  # noqa: N802
    # 一个实例拥有一份持久状态；调用者不复制存储，也不手写 D/E 或提交连线。
    count: ac.u8 = 5

    @ac.rule(guard=lambda: count < 7)
    def increment() -> None:
        count = count + 1

    # 声明顺序保存在源单元接口中，按同一实际实例提升到外层调用事务。
    # 这里的 Rule 未在 Counter 中自主注册，仍然保留 increment -> read。
    @ac.rule(after=(increment,))
    def read() -> ac.u8:
        return count


@ac.system
def CalleeAfterOnly():  # noqa: N802
    counter = Counter()

    @ac.rule
    def writer() -> None:
        counter.increment()

    @ac.rule
    def reader() -> None:
        # 内部 after 已建立 writer -> reader。首拍读候选值 6；前驱不 fire
        # 时读取最近的已提交值 7，后继不会因此被禁止。
        value = counter.read()
        ac.log("info", "count", value)

    writer()
    reader()


@ac.system
def CallerAfter():  # noqa: N802
    counter = Counter()

    @ac.rule
    def writer() -> None:
        counter.increment()

    # 重复声明同一个外层顺序不会增加周期或创建另一次 increment 调用。
    @ac.rule(after=(writer,))
    def reader() -> None:
        value = counter.read()
        ac.log("info", "count", value)

    writer()
    reader()


# 两个入口都应得到 6、7、7、7。顺序由声明和实际实例身份决定，
# 不由 writer()/reader() 的源码排列决定。硬件前递选择整事务 fire 且
# 对应状态有写意图时的候选值；Q 仍只在全系统检查通过的提交点更新。

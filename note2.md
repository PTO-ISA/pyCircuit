又可以考虑重构了？当前框架似乎太慢了

状态对象：queue，cell，...
- 基本：
  - reset
  - 输出端口信号
  - 输入端口信号
  - enable？允许状态更新操作

先看写入：以queue为例，一个简单的queue应该有一根写入信号线，代表下一个要push的entry，只要enable信号打开就完成更新。cell，写入信号线代表希望写入的entry，enable信号打开完成更新。

在这种设计下，多跟写入线需要由一个额外的组合仲裁逻辑选取，比如多个signal进来，一个signal出去

组合逻辑var，signal

Module
- 以python的形式写控制逻辑
- 不涉及状态修改

rule
- 以python的形式写控制逻辑
- 涉及状态修改，写入状态对象输入端口信号，控制几个enable同时成立？
- enable靠仲裁阶段解决？
- 什么会导致不enable？目前只能考虑到queue满反压


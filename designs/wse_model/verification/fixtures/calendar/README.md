# 历史 Calendar AllGather fixture

此处保存旧的固定序列与反馈驱动 AllGather 元数据载体，以及对应 bench。
它们使用 `wse_calendar_fixture` 包名引用本目录内源码，通用组件仍引用
`wse_calendar`。Python 路由字面量和配置 metadata 回归测试继续读取这些文件。

这些 fixture 不注册到 `models/calendar/CMakeLists.txt` 或
`tools/verify_calendar.py`。它们只保留历史场景和源码参考；新的实际载荷与
AllGather 验收使用 `models/machine/` 的同一通用机器加载运行时程序。

组件门禁继续覆盖共享接收、配置、轮次、解码、逐跳转发和节点组合。历史
fixture 的旧执行结果各自绑定旧候选，不能作为新机器的执行证据。

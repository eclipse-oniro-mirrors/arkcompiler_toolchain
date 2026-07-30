# Inspector 会话层知识

本文只记录 `inspector/` 全量组件：会话连接管理、消息路由、动态库加载、ArkTS-Sta 入口初始化。
ArkTS-Sta 调试详情见 `docs/knowledge/ArkTS-Sta-Subsystem.md`，ArkTS-Dyn 调试详情见 `docs/knowledge/ArkTS-Dyn-Subsystem.md`，WebSocket 帧协议见 `docs/knowledge/WebSocket-Subsystem.md`。

## 核心模型和主链路

### 线程模型

- **OS_DebugThread**：Inspector 创建的专用 WebSocket 服务线程，运行 `WsServer::RunServer()`
- **OS_DbgConThread**：ConnectServer 的专用线程，运行 `ConnectServer::RunServer()`（非调试器域：ArkUI/WMS/Cangjie）
- **VM/应用线程**：调用 `StartDebug()`/`StopDebug()`/`StoreDebuggerInfo()` 等
- **任何线程**：可调用 `SendReply()`（通过 `g_mutex` + `g_sendMutex` 保护）

### 主链路

DevEco Studio → WsServer → Inspector::OnMessage → 按 sessionId 路由：
- 无 sessionId → ArkTS-Dyn（g_onMessage）
- 有 sessionId（空或数字）→ ArkTS-Sta（OnMessageStatic）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| StartDebug / StopDebug / InitializeInspector | 应用线程调用 → 加载动态库 → 初始化 Ark 函数 → 创建 Inspector → 启动 WS 线程 | `inspector.cpp` | **g_inspectors 按 VM 指针 1:1 映射**——同一 VM 不可创建两个 Inspector |
| OnMessage / sessionId / 混合路由 | WS 线程收到 CDP 消息 → 按 sessionId 路由到 Dyn 或 Sta | `inspector.cpp:326` | **sessionId 必须为空或数字字符串**——非数字值被拒绝，消息丢弃 |
| g_handle / LibraryLoader / LoadArkDebuggerLibrary / g_hasArkFuncsInited / InitializeArkFunctions | thread_local g_handle 持有首次加载的动态库句柄；g_hasArkFuncsInited 进程级原子标志控制只初始化一次；函数指针进程级共享 | `library_loader.cpp` `inspector.cpp:301` | **首次初始化线程持有 g_handle，初始化标志和函数指针进程级共享**——原因：g_hasArkFuncsInited 使后续线程直接返回，不会重复加载；StartDebug/StopDebug 应保持线程归属一致 |
| ConnectServer / ConnectInspector / g_connectMutex | 非调试器域（ArkUI/WMS/Cangjie）的回调注册与消息路由 | `connect_inspector.cpp` | **g_inspector 是全局单例**——原因：StartServerForSocketPair 只允许一个 ConnectServer |
| init_static / InitializeArkFunctionsForStatic / g_debuggerHandle | 加载 libarkinspector.so 并解析 9 个函数符号 | `init_static.cpp` | **g_debuggerHandle 是全局静态变量**——原因：无线程安全保护，并发调用可能双重加载 |
| debuggerPostTask_ / DispatchStatus | 消息分派后检查分发状态，按需投递任务到调试线程 | `inspector.cpp:348` | **任务必须执行在正确的调试线程**——原因：线程身份检查，错误线程直接返回 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| Inspector（仓库根） | `inspector/` 会话转发层，Dyn 与 Sta 共用入口 | 不是 `tooling/static` 的 Inspector 类（PtHooks 协调器） |
| LibraryLoader | `library_loader.cpp`，动态加载 libark_tooling.so | 不是 init_static（后者加载 libarkinspector.so） |
| ConnectServer | `connect_server.cpp`，非调试器域（ArkUI/WMS）的连接管理 | 不是 WsServer（后者是调试器域的 WebSocket 服务端） |
| g_handle | thread_local，首次初始化线程持有；初始化标志和函数指针进程级共享 | 不是 g_debuggerHandle（全局静态，进程共享） |

## 锁与线程安全

| 锁 | 类型 | 保护对象 | 读取方 | 写入方 |
|---|---|---|---|---|
| g_mutex | shared_mutex | g_inspectors, g_debuggerInfo, Ark 函数指针 | SendReply, GetDebuggerPostTask, GetEcmaVM | InitializeInspector, InitializeArkFunctions, StopDebug, StoreDebuggerInfo |
| g_sendMutex | shared_mutex | WsServer/ConnectServer 发送 | — | SendReply（所有实例） |
| g_connectMutex | mutex | ConnectInspector 所有字段和消息处理 | — | OnMessage, SetSwitchCallBack, SetDebugModeCallBack, StoreMessage 等 |
| wsMutex_ | mutex | WsServer 的 webSocket_ 成员 | — | RunServer, StopServer |

## 约束规则

- **禁止**同一 VM 指针创建两个 Inspector——原因：g_inspectors 1:1 映射，重复创建被静默忽略
- **禁止**从 debug 线程自身调用 StopDebug()——原因：会 pthread_join 自身导致死锁
- **禁止**混合 CDP 消息中 sessionId 为非数字值——原因：消息被拒绝丢弃
- **禁止**调用 StartServerForSocketPair() 两次期望创建第二个 ConnectServer——原因：返回 true 不创建
- **必须**在调用 GetJsBacktrace/OperateJsDebugMessage 后 free 返回的 const char*——原因：不释放内存泄漏
- **必须**在 StoreDebuggerInfo 之后调用 StartDebugForSocketpair——原因：否则 GetEcmaVM 返回 nullptr
- **禁止**并发调用 InitializeArkFunctionsForStatic——原因：g_debuggerHandle 无锁保护，并发双重加载

## 修改前检查

- 改 `inspector.cpp` OnMessage 路由前，确认 sessionId 正则匹配逻辑 → 误改致消息被错误路由
- 改 `init_static.cpp` 前，确认 g_debuggerHandle 无线程安全保护 → 并发调用风险
- 改 `connect_inspector.cpp` SendMessage 前，确认 g_inspector 指针访问无 mutex 保护 → 并发 ResetService 时指针可能悬空
- 改 `ws_server.cpp` 前，确认 SendReply 和 StopServer 的锁获取顺序 → 避免死锁
- 改 sessionId 路由规则前，确认对 Dyn/Sta 消息分发的影响 → 误改致消息路由到错误子系统
- 改 LibraryLoader 动态库加载策略前，确认对多实例和多线程的影响 → 误改致句柄泄漏或双重加载
- 改 ConnectServer 回调注册机制前，确认对 ArkUI/WMS/Cangjie 域的影响 → 误改致非调试器域消息丢失

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| 会话管理与 WebSocket 服务端 | `inspector/inspector.cpp` + `inspector/ws_server.cpp` |
| 多会话连接与回调注册 | `inspector/connect_server.cpp` + `inspector/connect_inspector.cpp` |
| 动态库加载 | `inspector/library_loader.cpp` |
| ArkTS-Sta 入口 | `inspector/init_static.cpp` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| Inspector 核心/连接 | `InspectorConnectTest` |

### 验证闭环

构建与测试从 OpenHarmony 源码根执行：

```bash
./build.sh --product-name rk3568 --build-target ark_debugger
./build.sh --product-name rk3568 --build-target InspectorConnectTest
```

Done 标准：变更行为实现 + 相关 build/test 已跑或说明无法跑的原因 + 回复含变更文件、验证命令与结果、剩余风险；不得为通过测试删除日志/错误码或绕过安全守卫。

# ArkTS-Sta 子系统知识

本文只记录 `tooling/static/` 全量组件：静态 ArkTS（ArkTS-Sta）调试的协议协调、消息分发、连接层、断点/单步、表达式求值、对象检视、混合调试帧提供者。ArkTS-Dyn（动态调试）见 `tooling/dynamic/`，仓库根的会话转发层见 `inspector/`，WebSocket 帧协议见 `websocket/`。已有子模块 AGENTS.md：`tooling/static/AGENTS.md`。

## 概述

ArkTS-Sta 是面向静态类型 ArkTS 的 CDP（Chrome 开发者工具协议）调试实现：`Inspector` 实现 `PtHooks` 挂载 VM 事件，经 `InspectorServer` 把 CDP JSON 消息分发到各 CDP 协议域处理器，`DebuggableThread` 在每个应用线程上驱动断点命中与单步状态机。最关键的架构边界：**ObjectRepository 操作必须在应用线程持 mutator lock 时进行；VM 死亡后任何操作前必须经 `CheckVmDead()` 守卫**。

高频修改路径（按改动频率与回归风险排序）：`inspector.cpp`（事件回调/协议接线）> `debugger/breakpoint_storage.cpp` + `thread_state.cpp`（断点与单步语义）> `inspector_server.cpp`（CDP 处理器注册）> `debugger/object_repository.cpp`（对象检视，受 mutator lock 约束）。

Where to look（任务→入口）：

| 任务 | 先读入口 |
|---|---|
| 断点/条件断点/单步行为 | `debugger/breakpoint_storage.cpp` + 本文档"断点与单步"表 |
| CDP 新增方法/消息分发 | `inspector_server.cpp` + "消息分发与会话"表 |
| 对象检视/属性 | `debugger/object_repository.cpp` + "对象检视与求值"表 |
| 连接/WebSocket 后端 | `connection/server.h` + "连接层"表 |
| 表达式求值 | `evaluation/evaluation_engine.cpp` + 本文档"对象检视与求值"表 |

## 核心模型和主链路

本节同时承担路由：按"触发词或任务"定位组件，根据"优先入口"读取代码，遵守"不变式/维护点"。进入计划阶段前，请在 plan 中声明：任务类别、已读本 SDD 章节、发现的不变式、是否需调用相关 skill。

### 协议协调与生命周期

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `Inspector` / `PtHooks` / `RegisterHooks` / `MethodEntry` / `SingleStep` / `LoadModule` / `VmDeath` / `ConsoleCall` | init.cpp 导出 C ABI → 构造 Inspector → `RegisterHooks(this)` 注册 VM 回调 → 事件驱动调试 | `inspector.cpp` `init.cpp` + 子模块 AGENTS.md | **每次操作前 `CheckVmDead()`**——原因：VM 死亡后继续访问 inspectorServer_/运行时会触碰已释放对象，CheckVmDead() 在 isVmDead_ 时 Kill server 并短路 |
| `StartDebugger` / `InitializeInspector` / `HandleMessage` / `OperateJsDebugMessageForStatic` / `IsStaticRuntimeOnCurrentThread` | C 导出符号，运行时入口；`IsStaticRuntimeOnCurrentThread` 区分静态/动态运行时 | `init.cpp/h` | **静态/动态运行时线程不可共用一个 inspector**——原因：导出符号按线程归属分发消息，混线程会跨运行时状态污染 |

### 消息分发与会话

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `InspectorServer` / `OnCallDebugger*` / `OnCallRuntime*` / `OnCallProfiler*` / `CallDebuggerPaused` / `CallDebuggerScriptParsed` | WebSocket 收 CDP JSON → Server::OnCall 注册的 handler → 分发到 Inspector → 构造 JsonObjectBuilder 响应（callFrames/scopes/remoteObjects） | `inspector_server.cpp/h` | **server 线程方法与应用线程方法不可混用**——原因：DebuggableThread 头注释显式标注 Continue/StepInto/IsPaused 在 server 线程、OnMethodEntry/OnSingleStep/OnException 在 app 线程，跨线程调用破坏 mutex_ 状态机 |
| `SessionManager` / `GetSessionIdByThread` / `GetThreadBySessionId` / `AddSession` / `RemoveSession` | 维护 PtThread↔sessionId 映射，受 mutex_ 保护 | `session_manager.cpp/h` | **会话映射查询必须持锁**——原因：多线程并发连接/断开会使 std::map 迭代器失效 |
| `SourceManager` / `GetScriptId` / `GetSourceFileName` / `ScriptId` | 源文件名↔ScriptId 双向映射，CDP `Debugger.scriptParsed` 用 | `source_manager.cpp/h` | **ScriptId 空间与 BreakpointId/RemoteObjectId 独立**——原因：numeric_id.h 各 ID 类型独立计数，混用致会话映射错乱 |

### 连接层

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `Server` / `Run` / `Pause` / `Continue` / `OnCall` / `Call` / `ParseMessage` | Server 基类单监听线程事件循环：OnCall 注册 handler，Call 发通知，ParseMessage 处理一条 CDP 消息 | `connection/server.h` | **`Pause()` 持读锁阻塞事件循环**——原因：Pause 用 RWLock taskExecution_ 读锁阻塞，期间 OnCall handler 持写锁等待，用于线程暂停期间安全操作 |
| `PANDA_TOOLING_ASIO` / `asio_server` / `ohos_ws_server` / `endpoint_base` / `server_endpoint_base` | ASIO（跨平台）或 OHOS WebSocket（OpenHarmony）两后端，构建期二选一 | `connection/asio/` `connection/ohos_ws/` | **后端构建期固定，不可运行时切换**——历史坑：两后端无运行时分支，改一个后端须同时验证两平台构建，否则另一平台静默编译失败 |

### 断点与单步

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `BreakpointStorage` / `SetBreakpoint` / `RemoveBreakpoint` / `ResolveBreakpoints` / `breakpointLocations_` / `breakpointsActive_` | 设断点→创建 Breakpoint/ConditionalBreakpoint；文件加载时 ResolveBreakpoints 解析行号→PtLocation 多重映射；命中查 breakpointLocations_ | `debugger/breakpoint_storage.cpp/h` | **breakpointLocations_ 是多重映射，一位置多断点**——原因：同一 PtLocation 可被多断点命中，遍历须取全 vector 而非第一个 |
| `Breakpoint` / `ConditionalBreakpoint` / `BreakpointBase` / `ShouldStopAt` / `IsUrlPattern` / `SetLocations` | Breakpoint 无条件多位置；ConditionalBreakpoint 单位置、条件字节码命中时经 EvaluationEngine 求值 | `debugger/breakpoint.cpp/h` `debugger/conditional_breakpoint.cpp/h` | **Breakpoint 多位置、ConditionalBreakpoint 单位置**——原因：ConditionalBreakpoint 仅 location_（optional），混用语义会丢条件或断点无法解析 |
| `ThreadState` / `StepKind` / `STEP_INTO` / `STEP_OVER` / `STEP_OUT` / `BREAK_ON_START` / `CONTINUE_TO` / `OnSingleStep` | 状态机：StepKind 决定 OnSingleStep/OnMethodEntry 是否暂停；stepLocations_ 语义随步类变 | `debugger/thread_state.cpp/h` | **`BREAK_ON_START` 不随新连接 Reset 清除**——原因：头注释明示 BREAK_ON_START 在 Reset 期间不重置以保留断起点语义，误清致初始入口不暂停 |
| `DebuggableThread` / `OnMethodEntry` / `OnFramePop` / `IsPaused` / `BreakOnStart` / `ResetObjectRepository` | 每应用线程一个，私有继承 PtThreadEvaluationEngine；server 线程发步进指令，app 线程回调驱动 | `debugger/debuggable_thread.cpp/h` | **步进指令发到未暂停线程无效**——原因：步进靠 OnSingleStep 回调，运行中线程无 step 事件 |

### 对象检视与求值

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `ObjectRepository` / `CreateFrameObject` / `CreateGlobalObject` / `GetProperties` / `RemoteObjectId` / `HandleScope` | 暂停时构造 frame/global 对象供 IDE 检视；GetProperties 返回 PropertyDescriptor | `debugger/object_repository.cpp/h` | **ObjectRepository 操作必须在应用线程持 mutator lock 时进行**——历史坑：内部 HandleScope/VMHandle 依赖 mutator 锁，跨线程或无锁访问致句柄悬空 |
| `EvaluationEngine` / `PtThreadEvaluationEngine` / `EvaluateExpression` / `ExpressionWrapper` / `base64` | IDE 发 base64 字节码→Inspector::Evaluate→PtThreadEvaluationEngine 在指定 frameNumber 应用线程求值→TypedValue+异常 | `evaluation/evaluation_engine.cpp/h` `evaluation/base64.h` | **求值在目标应用线程执行**——原因：PtThreadEvaluationEngine 绑定特定 ManagedThread，操作其栈帧，跨线程访问错栈 |

### 调试信息缓存与混合帧

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `DebugInfoCache` / `AddPandaFile` / `GetSourceLocation` / `GetBreakpointLocations` / `GetValidLineNumbers` / `GetLocals` / `GetSourceCode` / `DisasmBackedDebugInfoExtractor` | panda 文件加载→AddPandaFile 缓存 DebugInfoExtractor→断点解析/行号/源码均查 debugInfos_ | `debugger/debug_info_cache.cpp/h` | **debugInfos_ 受 debugInfosMutex_ 保护**——原因：文件加载(app线程)与断点查询(server线程)并发，无锁访问迭代器会因重哈希失效 |
| `StaticFrameProvider` / `IFrameInfoProvider` / `ExtractFrameInfo` / `UnifiedFrameInfo` / `UnifiedRemoteObject` / `ConvertToUnifiedRemoteObject` / `RegisterProvider` | Inspector 构造时向 hybrid_step::FrameInfoExtractor 注册 StaticFrameProvider(isStaticFrame=true)；混合调试时从 PtFrame 经 DebugInfoCache 取源位置构造 UnifiedFrameInfo+scopeChain | `hybrid/static_frame_provider.cpp/h` | **StaticFrameProvider 是静态帧唯一提供者，isStaticFrame=true 不可误标**——原因：hybrid_step 协调器按 isStaticFrame 分流，误标致 ArkTS 帧走动态路径、scope/object 转换错误 |

### 协议类型与序列化

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| `RemoteObject` / `Scope` / `Location` / `PropertyDescriptor` / `ObjectPreview` / `PauseOnExceptionsState` | CDP 协议类型定义；InspectorServer 构造响应 | `types/` | **RemoteObject 类型字符串须与 CDP 规范一致**——原因：StaticFrameProvider 按 RemoteObjectType 字符串分流转换，串错致 UnifiedRemoteObject 类型错 |
| `JRPCError` / `Serializable` / `JsonObjectBuilder` | JRPC 错误码与可序列化接口 | `json_serialization/` | **错误响应须用 JRPCError 而非裸字符串**——原因：CDP 客户端按 JSON-RPC error 结构解析，裸串致 IDE 无法识别错误 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| Inspector | `tooling/static` 的 PtHooks 协调器，仅服务静态 ArkTS | 不是仓库根 `inspector/` 那层会话转发，也不是 `tooling/dynamic` 的 JS 调试器 |
| 连接层 | `connection/` 的 Server 基类，ASIO/OHOS 构建期二选一 | 不是 `websocket/` 那层帧/握手协议实现 |
| 帧提供者 | `hybrid/StaticFrameProvider` 向 hybrid_step 注入静态帧 | 不是 hybrid_step 本身，仅是其 IFrameInfoProvider 的静态实现 |
| 求值引擎 | `evaluation/` 在应用线程求值 base64 字节码 | 不是 `tooling/dynamic` 的运行时求值，不解释 JS 源码 |

## 约束规则

- **禁止**在非应用线程或未持 mutator lock 时操作 ObjectRepository——历史坑：内部 HandleScope/VMHandle 依赖 mutator 锁，悬空句柄致崩溃
- **必须**在 Inspector 每次操作前调用 CheckVmDead()——原因：VM 死亡后 isVmDead_=true，未守卫会访问已释放运行时
- **禁止**跨线程调用 DebuggableThread 的 server 线程方法（Continue/StepInto/IsPaused）或 app 线程方法（OnMethodEntry/OnSingleStep/OnException）——原因：mutex_ 保护的状态机按线程归属设计，跨线程调用破坏暂停/步进语义
- **必须**在新增 CDP 方法处理器时同时注册 OnCall handler 与 Inspector 实现方法——原因：仅注册 handler 不接线，IDE 请求无响应；仅实现方法不注册，消息到不了
- **禁止**在 Reset() 中清除 BREAK_ON_START——原因：该步类刻意跨重连保留断起点语义，清除致初始入口不暂停
- **必须**改连接后端后同时验证 ASIO 与 OHOS 两平台构建——历史坑：两后端无运行时分支，漏验证另一平台静默编译失败

### 问人事项（Ask before）

- 新增 CDP 方法处理器或改既有处理器签名/响应格式前——影响 DevEco Studio 协议兼容性，须确认 IDE 侧是否适配
- 切换连接后端（ASIO↔OHOS WebSocket）或改 `connection/server.h` 基类契约前——两后端共享契约且构建期固定，须两平台同验
- 改 `ObjectRepository` 的线程归属契约（mutator lock 依赖）前——句柄悬空风险，须确认所有调用点
- 引入新的 runtime_core/第三方依赖前——影响构建产物与产物大小，须评审

## 修改前检查

- 改 `inspector.cpp` 事件回调前，确认是否触及 CheckVmDead() 守卫 → 若在 VmDeath 路径上，须保证 isVmDead_ 读改用 vmDeathLock_
- 改 `debugger/object_repository.cpp` 前，确认调用点是否在应用线程持 mutator lock → 否则补 HandleScope 守卫或改到暂停时执行
- 改 `debugger/thread_state.cpp` 步进状态机前，确认 StepKind 语义与 stepLocations_ 用法（CONTINUE_TO=目标集；STEP_INTO/OVER=当前行集）→ 误改致步进行为错乱
- 改 `connection/` 后端前，确认是否影响另一后端 → 两后端共享 Server 基类契约，改基类须两平台同验
- 改 `hybrid/static_frame_provider.cpp` 类型转换前，确认 RemoteObjectType 字符串与 CDP 规范一致 → 串错致 UnifiedRemoteObject 分流错
- 改 `debugger/breakpoint_storage.cpp` 前，确认 breakpointLocations_ 多重映射遍历取全 vector → 只取第一个致同位置多断点漏命中

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| 协调器与 PtHooks | `tooling/static/inspector.cpp` |
| 运行时 C 导出入口 | `tooling/static/init.cpp` |
| 消息分发 | `tooling/static/inspector_server.cpp` |
| 会话/源码管理 | `tooling/static/session_manager.cpp` `tooling/static/source_manager.cpp` |
| 连接层基类 | `tooling/static/connection/server.h` |
| 断点存储/断点/条件断点 | `tooling/static/debugger/breakpoint_storage.cpp` `debugger/breakpoint.cpp` `debugger/conditional_breakpoint.cpp` |
| 线程状态/可调试线程 | `tooling/static/debugger/thread_state.cpp` `debugger/debuggable_thread.cpp` |
| 对象仓储 | `tooling/static/debugger/object_repository.cpp` |
| 调试信息缓存 | `tooling/static/debugger/debug_info_cache.cpp` |
| 表达式求值 | `tooling/static/evaluation/evaluation_engine.cpp` |
| 混合调试帧提供者 | `tooling/static/hybrid/static_frame_provider.cpp` |
| 协议类型与序列化 | `tooling/static/types/` `tooling/static/json_serialization/` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| 断点/线程状态/对象仓储/会话/源码/调试信息缓存/inspector_server/base64 | `arkinspector_tests` — `tooling/static/tests/BUILD.gn`（host_unittest_action，源集含 base64.cpp、debug_info_cache.cpp、inspector_server.cpp、object_repository.cpp、session_manager.cpp、source_manager.cpp、thread_state.cpp、bigint_decimal_conversion_test.cpp） |
| breakpoint_storage / conditional_breakpoint / evaluation_engine | 无独立测试，验证方式：构建 `arkinspector_tests` 确认编译通过，断点命中经 inspector 集成测试覆盖 |

### 验证闭环

构建与测试从仓库根执行：

```bash
cd arkcompiler/toolchain
# 最小验证：构建静态库 + 单测
./build.sh --product-name rk3568 --build-target libarkinspector_plus
./build.sh --product-name rk3568 --build-target arkinspector_tests
```

任务级验证（按变更类型选最小集）：

| 变更类型 | 最小验证 |
|---|---|
| 断点/线程状态/对象仓储/会话/源码/inspector_server | 构建 + 跑 `arkinspector_tests`（对应源集文件） |
| `connection/` 后端 | ASIO 与 OHOS 两平台各构建一次 `libarkinspector_plus` |
| `evaluation/` 求值 | 构建 `arkinspector_tests` 确认编译通过（无独立求值单测，经 inspector 集成测试覆盖） |
| `hybrid/static_frame_provider` 类型转换 | 构建通过 + 混合调试集成测试验证 scope/object 转换 |
| 协议类型/序列化 | 构建 + `arkinspector_tests`（json_object_matcher） |

Done 标准：变更行为实现 + 相关 build/test 已跑或说明无法跑的原因 + 回复含变更文件、验证命令与结果、剩余风险；不得为通过测试删除日志/错误码或绕过 mutator lock 守卫。

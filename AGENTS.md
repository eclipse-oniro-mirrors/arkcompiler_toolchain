# ArkCompiler Toolchain

## 架构设计

```
DevEco Studio
      |
      v
WebSocket 通信层 (websocket/)
      |
      v
Inspector 层 (inspector/)  <-- 会话管理与消息转发
      |
      v
Tooling 层 (tooling/)
      |
      +-- ArkTS-Dyn (dynamic/)  <-- 运行时调试与性能调优
      +-- ArkTS-Sta (static/)    <-- 静态调试
      +-- 混合调试 (hybrid_step/)
```

工具链与 ArkCompiler Runtime 协作，提供运行时信息并执行调试/调优命令。

## 代码地图

```
arkcompiler/toolchain/
|-- adapter/              # 模拟器与预览器适配层
|-- common/               # 共享工具（日志、宏）
|-- docs/                 # 调试与调优特性文档
|-- figures/              # 架构图
|-- inspector/            # 调试协议对接层
|   |-- inspector.cpp     # 主 Inspector 实现
|   |-- ws_server.cpp     # WebSocket 服务端
|   `-- connect_inspector.cpp
|-- platform/             # 平台相关实现
|   |-- file.h            # 跨平台文件接口
|   |-- unix/             # Unix 实现
|   `-- windows/          # Windows 实现
|-- test/                 # 测试基础设施
|   |-- autotest/         # 自动化测试
|   |-- fuzztest/         # 模糊测试
|   |-- resource/         # 测试资源
|   `-- ut/               # 单元测试
|-- tooling/              # 核心调试调优工具
|   |-- dynamic/          # ArkTS-Dyn 实现
|   |   |-- agent/        # 运行时 Agent（runtime、dom、page、profiling、debugger）
|   |   |-- backend/      # 后端执行器与钩子
|   |   |-- base/         # 协议类型与工具
|   |   |-- client/       # 客户端工具（ark_multi、ark_cli）
|   |   `-- utils/        # 工具函数
|   |-- static/           # ArkTS-Sta 实现（静态类型 ArkTS 的 CDP 调试；详见 `docs/knowledge/ArkTS-Sta-Subsystem.md`）
|   |   |-- connection/   # 连接管理（ASIO/OHOS WebSocket，构建期可选）
|   |   |-- debugger/     # 核心调试功能（断点、单步、对象检视）
|   |   |-- evaluation/   # 表达式求值引擎（应用线程上执行 base64 字节码）
|   |   |-- types/        # 协议类型与序列化
|   |   `-- json_serialization/
|   `-- hybrid_step/      # 混合调试步进标志
|-- websocket/            # WebSocket 协议实现
|   |-- server/           # 服务端实现
|   |-- client/           # 客户端实现
|   |-- frame_builder.cpp # 帧构建工具
|   `-- handshake_helper.cpp
|-- BUILD.gn              # 主构建配置
|-- bundle.json           # 组件元数据
|-- toolchain.gni         # 工具链构建配置
`-- toolchain_config.gni  # 工具链配置选项
```

## 知识路由（SDD）

子系统知识文档（SDD）位于 `docs/knowledge/`，是 AGENTS.md 之上的一层路由索引 + 不变式。进入对应子系统目录前，先读相关 SDD。

### 基于任务的路由

- 断点、单步、对象检视、表达式求值、InspectorServer、PtHooks、条件断点、StaticFrameProvider、ArkTS-Sta 调试变更 → 阅读 `docs/knowledge/ArkTS-Sta-Subsystem.md` + `tooling/static/AGENTS.md`
- ArkTS-Dyn（动态调试）变更 → 阅读 `docs/knowledge/ArkTS-Dyn-Subsystem.md` + `tooling/dynamic/AGENTS.md`
- Inspector 会话/消息路由/动态库加载变更 → 阅读 `docs/knowledge/Inspector-Subsystem.md` + `inspector/AGENTS.md`
- WebSocket 帧协议/连接/握手变更 → 阅读 `docs/knowledge/WebSocket-Subsystem.md` + `websocket/AGENTS.md`
- 混合调试帧协调/步进标志变更 → 阅读 `docs/knowledge/ArkTS-Sta-Subsystem.md`（混合帧部分）+ `tooling/hybrid_step/`
- 构建系统变更 → 根目录 `BUILD.gn` + `bundle.json` + `toolchain.gni`

### 基于路径的路由

- `tooling/static/` → 静态 ArkTS 调试子系统；先读 `docs/knowledge/ArkTS-Sta-Subsystem.md`
- `tooling/dynamic/` → 动态 ArkTS 调试子系统；先读 `docs/knowledge/ArkTS-Dyn-Subsystem.md`
- `tooling/hybrid_step/` → 混合调试帧提取；读 `docs/knowledge/ArkTS-Sta-Subsystem.md`（混合帧部分）
- `inspector/` → 仓库根会话转发层；先读 `docs/knowledge/Inspector-Subsystem.md`
- `websocket/` → WebSocket 帧协议；先读 `docs/knowledge/WebSocket-Subsystem.md`

### 基于术语的路由

| 术语 | 风险提示 | SDD |
|---|---|---|
| PtHooks / InspectorServer / BreakpointStorage / ConditionalBreakpoint / DebuggableThread / ThreadState / ObjectRepository / EvaluationEngine / DebugInfoCache / StaticFrameProvider / CheckVmDead / PANDA_TOOLING_ASIO | ArkTS-Sta 调试；ObjectRepository 须在应用线程持 mutator lock、VM 死亡须 CheckVmDead 守卫、连接后端构建期固定 | `docs/knowledge/ArkTS-Sta-Subsystem.md` |
| JSPtHooks / JsDebuggerManager / EcmaVM / SingleStepper / DebuggerImpl / ProtocolHandler / Dispatcher / debugger_executor / pauseOnNextByteCode_ / DebuggerState | ArkTS-Dyn 调试；DebuggerState 无 mutex 保护（全在 VM 线程切换）、步进/恢复须 PAUSED 状态、pauseOnNextByteCode_ 是一次性标志 | `docs/knowledge/ArkTS-Dyn-Subsystem.md` |
| WsServer / ConnectServer / LibraryLoader / init_static / g_handle / g_inspectors / debuggerPostTask_ | Inspector 会话转发；g_handle 是 thread_local（首次初始化线程持有，初始化标志和函数指针进程级共享）、g_inspectors 按 VM 指针 1:1 映射、混合模式按 sessionId 路由 | `docs/knowledge/Inspector-Subsystem.md` |
| WebSocketBase / WebSocketServer / WebSocketClient / AcceptNewConnection / Decode / SendReply / connectionState_ / g_sendReplymutex | WebSocket 帧协议；AcceptNewConnection 不可与自身或 Decode 并发、g_sendReplymutex 是全局锁、SendReply 允许外部线程并发但消息可能跨连接代际 | `docs/knowledge/WebSocket-Subsystem.md` |
| 混合调试 / HybridStep / FrameInfoExtractor / IFrameInfoProvider / UnifiedFrameInfo / STATIC_TO_DYNAMIC / DYNAMIC_TO_STATIC | 混合调试帧协调；StaticFrameProvider 的 isStaticFrame=true 不可误标、混合步进标志须即时重置 | `docs/knowledge/ArkTS-Sta-Subsystem.md`（混合帧部分） |

## 约束与边界

### 架构不变式

- ArkTS-Sta 的 ObjectRepository 操作必须在应用线程持 mutator lock 时进行；VM 死亡后任何 Inspector 操作前必须经 `CheckVmDead()` 守卫，连接后端（ASIO/OHOS WebSocket）构建期固定不可运行时切换。详见 `docs/knowledge/ArkTS-Sta-Subsystem.md`。
- ArkTS-Sta 与 ArkTS-Dyn 是两套独立调试实现，不可共用 inspector 状态——IsStaticRuntimeOnCurrentThread 按线程归属分发消息，混线程会跨运行时状态污染。
- ArkTS-Dyn 的 DebuggerState 状态机无 mutex 保护——所有状态转换在 VM 线程上进行，步进/恢复操作须在 PAUSED 状态。
- Inspector 层 g_inspectors 按 VM 指针 1:1 映射——同一 VM 不可创建两个 Inspector。
- WebSocket 的 AcceptNewConnection() 不可与自身或 Decode() 并发调用——原因：连接 fd 竞态；SendReply 允许外部线程并发，但消息可能跨连接代际，须用 close/fail callback 规避。

### 不要做

- 不要在非应用线程或未持 mutator lock 时操作 ObjectRepository——历史坑：HandleScope/VMHandle 依赖 mutator 锁，悬空致崩溃
- 不要在 Reset() 中清除 ArkTS-Sta 的 BREAK_ON_START——该步类跨重连保留断起点语义
- 不要跨线程调用 DebuggableThread 的 server 线程方法（Continue/StepInto/IsPaused）或 app 线程方法（OnMethodEntry/OnSingleStep/OnException）——mutex_ 状态机按线程归属设计
- 不要在 ArkTS-Dyn 非 PAUSED 状态下调用步进/恢复方法——原因：状态机会拒绝
- 不要忘记在新增 CDP 扩展协议时将 methodName 加入对应域的 ProtocolsList——原因：IDE 依赖 Enable 响应中的扩展协议列表
- 不要为通过测试删除日志、错误码或绕过 CheckVmDead/mutator lock 守卫
- 不要引入新第三方依赖未经评审
- 不要改 CDP 协议响应格式或 JRPC 错误码结构而不确认 IDE 侧兼容性
- 不要从 debug 线程自身调用 StopDebug()——会 pthread_join 自身导致死锁
- 不要调用 GetJsBacktrace/OperateJsDebugMessage 后不 free 返回的 const char*——内存泄漏

## 验证闭环

### 最小验证

- 构建：`./build.sh --product-name rk3568 --build-target libarkinspector_plus`（静态库）或 `./build.sh --product-name rk3568 --build-target libark_ecma_debugger`（动态库）
- 全量测试：`./build.sh --product-name rk3568 --build-target ark_toolchain_host_unittest`
- 静态检查：编译即检查（`-Werror` 在 `ark_toolchain_common_config` 中，任何 warning 构建失败）

### 任务级验证

| 变更类型 | 最小验证 |
|---|---|
| ArkTS-Sta 调试变更 | 构建 `libarkinspector_plus` + 跑 `arkinspector_tests` |
| ArkTS-Dyn 调试变更 | 构建 `libark_ecma_debugger` + 跑 `DebuggerTest` |
| Inspector 会话变更 | 构建 `ark_debugger` + 跑 `InspectorConnectTest` |
| WebSocket 变更 | 构建 `ark_debugger` + 跑 `WebSocketTest` |
| 混合调试变更 | 构建 `libarkinspector_plus` + 混合调试集成测试 |
| connection 后端变更 | ASIO 与 OHOS 两平台各构建一次 |
| 构建配置变更 | `./build.sh --product-name rk3568 --build-target ark_toolchain_packages` |

### Done 标准

- 变更行为已实现
- 相关 build/test 已跑，或说明无法跑的原因
- 回复含变更文件列表、验证命令与结果、剩余风险
- 不得为通过测试删除日志/错误码或绕过安全守卫

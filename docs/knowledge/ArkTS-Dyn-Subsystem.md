# ArkTS-Dyn 子系统知识

本文只记录 `tooling/dynamic/` 全量组件：动态 ArkTS（ArkTS-Dyn）调试的协议分发、CDP 域处理器、EcmaVM 挂钩、步进器、变量访问。
ArkTS-Sta（静态调试）见 `docs/knowledge/ArkTS-Sta-Subsystem.md`，仓库根会话转发层见 `docs/knowledge/Inspector-Subsystem.md`，WebSocket 帧协议见 `docs/knowledge/WebSocket-Subsystem.md`。

## 核心模型和主链路

### 线程模型

- **WebSocket/Inspector 线程**（`OS_DebugThread`）：处理网络 I/O，将消息入队到 `ProtocolHandler::requestQueue_`
- **VM 线程**：执行所有 PtHooks 回调（SingleStep/Breakpoint/Exception 等）和 `ProcessCommand()` 命令处理
- **消息流转**：`DispatchCommand()`（Inspector 线程）→ `requestQueue_`（mutex 保护）→ `ProcessCommand()`（VM 线程）

### 消息流向

| 方向 | 链路 |
|---|---|
| Client → VM | Client (TCP) → ProtocolHandler::DispatchCommand → requestQueue_ → ProcessCommand → Dispatcher → AgentImpl → BackendHooks → EcmaVM |
| VM → Client | EcmaVM → JSPtHooks → AgentImpl → ProtocolHandler → Client |

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| JSPtHooks / SingleStep / Breakpoint / Exception / LoadModule / NativeOut / NativeCalling / NativeReturn | EcmaVM 字节码执行时调用 hooks → JSPtHooks 返回 true 暂停/false 继续 | `backend/js_pt_hooks.cpp` | **会进入 DebuggerImpl、创建临时 JSHandle/Local、执行暂停通知或脚本解析的 VM 回调必须创建 LocalScope**——原因：这些操作依赖 VM 线程句柄管理；纯标志操作（DisableFirstTimeFlag、HitSymbolicBreakpoint 等）无需 LocalScope |
| ProtocolHandler / DispatchCommand / ProcessCommand / WaitForDebugger | Inspector 线程入队 → VM 线程处理；ProcessCommand 在 DebuggerManagedScope 中分派 | `protocol_handler.cpp` | **分派前后保存/恢复 VM 异常状态**——原因：调试命令不应干扰 VM 异常状态 |
| DebuggerState / PAUSED / ENABLED / DISABLED | 状态机无 mutex 保护，全在 VM 线程切换；步进/恢复须在 PAUSED 状态 | `agent/debugger_impl.h` | **步进/恢复须在 PAUSED 状态**——原因：非 PAUSED 状态调用会直接返回失败 |
| SingleStepper / StepInto / StepOver / StepOut / pauseOnNextByteCode_ | 步进时创建 SingleStepper，捕获当前栈深度；OnSingleStep 中判断步进完成 | `backend/js_single_stepper.cpp` | **pauseOnNextByteCode_ 是一次性标志**——原因：消费后立即重置为 false，不重置会每次都暂停 |
| DebuggerExecutor / GetValue / SetValue / CmptEvaluateValue | 暂停时变量访问，按 Local→Lexical→Module→Global 顺序搜索 | `backend/debugger_executor.cpp` | **CmptEvaluateValue 须设置 EvalFrameHandler**——原因：底层 ASSERT frameHandler 非空，不设置会崩溃 |
| EvaluateOnCallFrame / SwitchContext | 求值时切换 VM 上下文到调试器全局环境，求值后恢复 | `agent/debugger_impl.cpp` | **求值后必须恢复原始上下文**——原因：不恢复会污染后续执行上下文 |
| HybridSingleStepper / STATIC_TO_DYNAMIC / DYNAMIC_TO_STATIC | 混合步进标志；STATIC_TO_DYNAMIC 立即重置避免重复暂停；DYNAMIC_TO_STATIC 在 StepInto/StepOut 设 true、Resume 设 false | `backend/js_pt_hooks.cpp` | **STATIC_TO_DYNAMIC 标志须立即重置**——原因：不重置致每次 SingleStep 都暂停 |
| debugger_service / InitializeDebugger / UninitializeDebugger | C 导出符号，运行时入口；每个入口检查 VM/JsDebuggerManager 非空 | `debugger_service.cpp` | **InitializeDebugger 是幂等的**——原因：重复初始化直接返回错误，不覆盖已有 handler |
| CleanUpOnPaused / CleanUpRuntimeProperties | 每次暂停时清理 callFrameHandlers_ 和 scopeObjects_；释放所有 Global<JSValueRef> 句柄 | `agent/debugger_impl.cpp` | **每次暂停必须清理 Global 句柄**——原因：不释放会泄漏 VM 全局句柄，最终致 GC 问题 |
| firstTime_ / BREAK_ON_START / DisableFirstTimeFlag | firstTime_ 初始 true，SingleStep 中触发 BREAK_ON_START；LoadModule/SendableMethodEntry 重置（非 launch accelerate 模式） | `backend/js_pt_hooks.cpp` | **firstTime_ 在 LoadModule 时重置**——原因：新脚本加载后需要在入口暂停 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| JSPtHooks | `backend/js_pt_hooks.cpp`，EcmaVM 事件回调接口 | 不是 ArkTS-Sta 的 PtHooks（后者是 `tooling/static/inspector.cpp`） |
| Dispatcher | `dispatcher.cpp`，CDP 命令路由 | 不是 ArkTS-Sta 的 InspectorServer（后者是 CDP 处理器注册+分发） |
| ProtocolHandler | `protocol_handler.cpp`，VM 线程命令处理循环 | 不是 Inspector 层的 WsServer（后者是网络层） |
| SingleStepper | `backend/js_single_stepper.cpp`，步进控制 | 不是 ArkTS-Sta 的 ThreadState（后者是状态机） |
| DebuggerExecutor | `backend/debugger_executor.cpp`，变量访问 | 不是 ArkTS-Sta 的 ObjectRepository（后者是对象检视） |

## 约束规则

- **禁止**在非 VM 线程操作 DebuggerState——原因：状态机无 mutex 保护，跨线程访问破坏状态一致性
- **必须**在会进入 DebuggerImpl、创建临时 JSHandle/Local、执行暂停通知或脚本解析的 VM 回调中创建 LocalScope——原因：这些操作依赖 VM 线程句柄管理；纯标志操作（DisableFirstTimeFlag、HitSymbolicBreakpoint、GetAllRecordNames、SetDebuggerAccessor）无需 LocalScope
- **禁止**在非 PAUSED 状态下调用步进/恢复方法——原因：状态机拒绝，操作无效
- **必须**在新增 CDP 扩展协议时将 methodName 加入对应域的 ProtocolsList——原因：IDE 依赖 Enable 响应中的扩展协议列表，遗漏致 IDE 不知道协议存在
- **必须**在 ProcessCommand 分派前后保存/恢复 VM 异常状态——原因：调试命令不应干扰 VM 异常状态
- **禁止**在 CmptEvaluateValue 中不设置 EvalFrameHandler——原因：底层 ASSERT frameHandler 非空，不设置会崩溃
- **必须**在 EvaluateOnCallFrame 后恢复原始 VM 上下文——原因：不恢复会污染后续执行上下文
- **禁止**在 ClearSingleStepper 中于栈深度 > 0 时重置——原因：嵌套调用中过早重置会破坏步进语义
- **必须**在每个暂停周期清理 Global<JSValueRef> 句柄（CleanUpRuntimeProperties）——原因：不释放会泄漏 VM 全局句柄
- **禁止**将 SetDebuggerState / SetNativeOutPause 用于生产代码——原因：头注释明确标注 "only use for test case"

## 修改前检查

- 改 `backend/js_pt_hooks.cpp` 前，确认涉及 DebuggerImpl/JSHandle/暂停通知/脚本解析的回调有 LocalScope → 纯标志操作无需 LocalScope
- 改 `agent/debugger_impl.cpp` 步进/恢复方法前，确认调用点是否在 VM 线程且处于 PAUSED 状态 → 否则操作无效
- 改 `backend/js_single_stepper.cpp` 前，确认 StepComplete 判断逻辑（STEP_INTO=范围+同方法；STEP_OVER=栈深度+范围；STEP_OUT=栈深度）→ 误改致步进行为错乱
- 改 `dispatcher.cpp` 前，确认新增的域是否注册到 dispatchers_ map → 不注册则消息到不了
- 改 `backend/debugger_executor.cpp` 前，确认 EvalFrameHandler 的设置/清理配对 → 不配对致 ASSERT 失败
- 改 `protocol_handler.cpp` 前，确认异常状态保存/恢复逻辑 → 遗漏致调试命令干扰 VM 异常
- 新增 CDP 扩展协议前，确认 IDE 侧是否适配 → 否则 IDE 无法识别新协议
- 改 DebuggerState 转换条件前，确认对所有步进行为的影响 → 误改致步进状态机错乱
- 改 ProtocolHandler 线程桥接模型前，确认对消息分派和 VM 暂停语义的影响 → 误改致消息丢失或死锁
- 改 EvaluateOnCallFrame 上下文切换逻辑前，确认句柄生命周期和上下文恢复 → 遗漏致句柄泄漏或上下文污染

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| EcmaVM 事件回调 | `tooling/dynamic/backend/js_pt_hooks.cpp` |
| 步进控制器 | `tooling/dynamic/backend/js_single_stepper.cpp` |
| 变量访问/修改 | `tooling/dynamic/backend/debugger_executor.cpp` |
| CDP 命令路由 | `tooling/dynamic/dispatcher.cpp` |
| 命令处理循环 | `tooling/dynamic/protocol_handler.cpp` |
| 调试器服务生命周期 | `tooling/dynamic/debugger_service.cpp` |
| CDP Debugger 域 | `tooling/dynamic/agent/debugger_impl.cpp` |
| CDP Runtime 域 | `tooling/dynamic/agent/runtime_impl.cpp` |
| CDP HeapProfiler 域 | `tooling/dynamic/agent/heapprofiler_impl.cpp` |
| 协议类型定义 | `tooling/dynamic/base/` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| 核心调试/步进/断点/协议类型 | `DebuggerTest`（28 个测试文件） |
| 调试器入口 | `DebuggerEntryTest` |
| C-解释器调试 | `DebuggerCInterpTest` |
| 调试客户端 | `DebuggerClientTest`、`DebuggerCIntClientTest` |
| 混合步进 | `hybrid_single_stepper_test.cpp`（含在 DebuggerTest 中） |

### 验证闭环

构建与测试从 OpenHarmony 源码根执行：

```bash
# 最小验证：构建动态库 + 单测
./build.sh --product-name rk3568 --build-target libark_ecma_debugger
./build.sh --product-name rk3568 --build-target DebuggerTest
./build.sh --product-name rk3568 --build-target DebuggerEntryTest
./build.sh --product-name rk3568 --build-target DebuggerCInterpTest
./build.sh --product-name rk3568 --build-target DebuggerClientTest
./build.sh --product-name rk3568 --build-target DebuggerCIntClientTest
```

任务级验证（按变更类型选最小集）：

| 变更类型 | 最小验证 |
|---|---|
| 后端 hooks/步进器/变量访问 | 构建 + 跑 `DebuggerTest` |
| CDP 域处理器/Dispatcher | 构建 + 跑 `DebuggerTest` |
| ProtocolHandler | 构建 + 跑 `DebuggerTest` + `protocol_handler_test.cpp` |
| 调试器服务生命周期 | 构建 + 跑 `debugger_service_test.cpp`（含在 DebuggerTest 中） |

Done 标准：变更行为实现 + 相关 build/test 已跑或说明无法跑的原因 + 回复含变更文件、验证命令与结果、剩余风险；不得为通过测试删除日志/错误码或绕过安全守卫。

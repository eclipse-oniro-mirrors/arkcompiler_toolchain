# WebSocket 协议知识

本文只记录 `websocket/` 全量组件：RFC 6455 WebSocket 帧编码/解码、握手、连接状态机、服务端/客户端实现。
Inspector 会话层见 `docs/knowledge/Inspector-Subsystem.md`，ArkTS 调试子系统见对应 SDD。

## 核心模型和主链路

### 连接状态机

```
CLOSED → CONNECTING → OPEN → CLOSING → CLOSED
         ↓ (握手失败)              ↑ (CloseConnection)
        CLOSED               CloseConnectionSocket
```

- `connectionState_` 是 `std::atomic<ConnectionState>`，所有读写用原子操作
- `CloseConnection` 用 CAS `OPEN→CLOSING`，非 OPEN 状态静默失败（防双重关闭）
- `CloseConnectionSocket` 只允许 `CLOSING→CLOSED`，CAS 失败则日志报错

### 线程安全模型

- **g_sendReplymutex**：全局 `std::mutex`（非实例级），所有 WebSocketBase 实例共享，序列化所有 SendReply
- **connectionMutex_**：`std::shared_mutex`，Send/Recv 取共享锁，Close 取独占锁
- **两阶段 Socket 关闭**：Phase 1（共享锁）ShutdownSocket 解阻塞 recv → Phase 2（独占锁）close + fd=-1

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| AcceptNewConnection / serverUp_ | CAS CLOSED→CONNECTING → 再检查 serverUp_ → 失败则回滚 CLOSED | `server/websocket_server.cpp` | **CAS 后必须重检查 serverUp_**——原因：Close 可能并发设置 serverUp_=false，不检查致关闭后仍接受连接 |
| Decode / SendReply / Close | Decode 不可对同一连接并发、不可与 AcceptNewConnection 并发；SendReply 允许外部线程并发（含 AcceptNewConnection 期间），但消息可能跨连接代际 | `websocket_base.cpp` | **g_sendReplymutex 是全局锁**——原因：所有实例共享，序列化所有 SendReply |
| ServerFrameBuilder / ClientFrameBuilder | 服务端帧不设 mask 位；客户端帧必须设 mask 位 | `frame_builder.cpp` | **RFC 6455：服务端禁止 mask、客户端必须 mask**——原因：协议合规，违反致连接关闭 |
| ValidateIncomingFrame | 服务端拒绝未 mask 帧；客户端拒绝已 mask 帧 | `server/websocket_server.cpp` `client/websocket_client.cpp` | **服务端必须拒绝未 mask 帧**——原因：RFC 6455 Section 5.1，否则协议违规 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| WebSocketBase | `websocket_base.cpp/h`，RFC 6455 基类，连接状态机+帧编码/解码 | 不是 ArkTS-Sta connection/ 的 Server 基类（后者是 CDP 消息分发层） |
| WebSocketServer | `server/websocket_server.cpp/h`，服务端连接管理 | 不是 ArkTS-Sta 的 asio_server/ohos_ws_server（后者是 CDP 通道层） |
| WebSocketClient | `client/websocket_client.cpp/h`，客户端连接 | 与 WebSocketServer 的帧 mask 规则相反 |
| g_sendReplymutex | 全局进程级 mutex | 不是 connectionMutex_（后者是实例级 shared_mutex） |

## 约束规则

- **禁止**并发调用 AcceptNewConnection() 与自身——原因：违反 WebSocketServer 保证，CAS 失败说明 bug
- **禁止**并发调用 AcceptNewConnection() 与 Decode()——原因：连接 fd 竞态
- **禁止**对同一连接并发调用 Decode()——原因：文档明确禁止
- **注意**外部线程可在 AcceptNewConnection 期间并发调用 SendReply()——消息可能发到新连接代际，须用 SetCloseConnectionCallback/SetFailConnectionCallback 规避
- **禁止**在 CloseConnectionSocket 中从非 CLOSING 状态转换——原因：CAS 会失败
- **禁止**在 SendReply 中使用非 TEXT/BINARY/CONTINUATION 的 frameType——原因：文档明确限制
- **必须**在 CAS CLOSED→CONNECTING 后重检查 serverUp_——原因：Close 可能并发关闭
- **禁止**在服务端发送 mask 帧——原因：RFC 6455 Section 5.1
- **禁止**在客户端发送未 mask 帧——原因：RFC 6455 Section 6.1
- **禁止**在服务端接受未 mask 帧——原因：必须关闭连接
- **禁止**在客户端接受已 mask 帧——原因：服务端不应 mask

## 修改前检查

- 改 `websocket_base.cpp` SendReply 前，确认 g_sendReplymutex 是全局锁 → 改粒度须评估所有调用方
- 改 `server/websocket_server.cpp` AcceptNewConnection 前，确认与 Decode 的并发约束 → 竞态条件
- 改 `websocket_base.cpp` CloseConnectionSocket 前，确认两阶段关闭逻辑 → 独占锁下 close 防 fd 复用
- 改 `frame_builder.cpp` 前，确认服务端/客户端 mask 规则 → RFC 6455 合规
- 改 `websocket_base.cpp` Decode 前，确认对同一连接不可并发调用 → 违反文档约束
- 改 g_sendReplymutex 作用域或粒度前，确认对所有连接并发安全的影响 → 误改致 SendReply 竞态
- 改连接状态机转换规则前，确认对所有连接生命周期的影响 → 误改致连接泄漏或无法关闭
- 改帧 mask 规则前，确认 RFC 6455 合规性 → 不合规致连接被对端关闭

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| WebSocket 基类与状态机 | `websocket/websocket_base.cpp` |
| 服务端实现 | `websocket/server/websocket_server.cpp` |
| 客户端实现 | `websocket/client/websocket_client.cpp` |
| 帧构建 | `websocket/frame_builder.cpp` |
| 握手/HTTP/网络 | `websocket/handshake_helper.cpp` `http.cpp` `network.cpp` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| WebSocket 核心/帧/握手 | `WebSocketTest`（4 个测试文件：frame_builder_test、http_decoder_test、web_socket_frame_test、websocket_test） |

### 验证闭环

构建与测试从 OpenHarmony 源码根执行：

```bash
./build.sh --product-name rk3568 --build-target ark_debugger
./build.sh --product-name rk3568 --build-target WebSocketTest
```

Done 标准：变更行为实现 + 相关 build/test 已跑或说明无法跑的原因 + 回复含变更文件、验证命令与结果、剩余风险；不得为通过测试删除日志/错误码或绕过安全守卫。

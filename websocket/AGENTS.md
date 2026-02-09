# AGENTS

This file provides guidance for AI agents when working with code in the websocket component.

## Project Overview

This is a **WebSocket protocol implementation** (RFC 6455) for the OpenHarmony ArkCompiler toolchain. It provides the communication layer between DevEco Studio (IDE) and running ArkTS applications, enabling debugging, profiling, and runtime communication capabilities.

## Architecture

The implementation follows a clean separation of concerns with three main classes:

### Core Components

1. **WebSocketBase** (`websocket_base.h/cpp`)
   - Abstract base class providing common WebSocket functionality
   - Manages connection states: CONNECTING → OPEN → CLOSING → CLOSED
   - Handles frame encoding/decoding with RFC 6455 compliance
   - Thread-safe operations using `std::shared_mutex`
   - Atomic connection state management

2. **WebSocketServer** (`server/websocket_server.h/cpp`)
   - Server-side implementation accepting WebSocket connections
   - Supports both TCP sockets and Unix domain sockets
   - Thread-safe with callbacks for connection lifecycle events
   - HTTP handshake and connection validation
   - Typical usage: spawn a server thread that calls `AcceptNewConnection()`, then loops on `Decode()` and `SendReply()`

3. **WebSocketClient** (`client/websocket_client.h/cpp`)
   - Client-side implementation for connecting to WebSocket servers
   - Supports TCP and Unix domain socket connections
   - Handles WebSocket handshake (upgrade request/response)
   - Built on top of WebSocketBase

### Supporting Components

- **WebSocketFrame** (`web_socket_frame.h`): Frame structure definitions
- **FrameBuilder** (`frame_builder.h/cpp`): Frame construction utilities
- **HandshakeHelper** (`handshake_helper.h/cpp`): HTTP WebSocket handshake helpers
- **HTTP** (`http.h/cpp`): HTTP protocol utilities for handshake
- **Network** (`network.h/cpp`): Low-level socket operations
- **StringUtils** (`string_utils.h`): String utility functions

### Thread Safety Model

- All operations are blocking (not async)
- Server thread runs `AcceptNewConnection()` and `Decode()` loop
- External threads can safely call `SendReply()`, `CloseConnection()`, and `Close()` concurrently
- Use callbacks (`SetCloseConnectionCallback`, `SetFailConnectionCallback`) to synchronize external threads with connection closure
- **Critical:** `AcceptNewConnection()` must not be called concurrently with `SendReply()` or `Decode()` (race condition)

See `server/README.md` for detailed sequence diagrams and state machine documentation.

## Build Commands

This component uses the GN build system. **Note:** The websocket component does not produce a standalone shared library (.so). Instead, it is compiled as static libraries and source sets that are linked into other components' shared libraries.

Build commands are run from the OpenHarmony root directory:

```bash
# Build inspector (which includes websocket)
./build.sh --product-name rk3568 --build-target ark_debugger

# Build connect server (which includes websocket)
./build.sh --product-name rk3568 --build-target connectserver_debugger
```

## Testing

### Test Structure

Located in `test/` directory:
- `websocket_test.cpp` - Main WebSocket functionality tests (connection, send/receive, frame encoding/decoding)
- `frame_builder_test.cpp` - Frame construction tests
- `http_decoder_test.cpp` - HTTP decoding tests
- `web_socket_frame_test.cpp` - Frame structure tests

Test targets: `WebSocketTest` (defined in `test/BUILD.gn`)

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
cd ark_standalone_build
python ark.py x64.debug WebSocketTestAction  # Single test (debug build)
python ark.py x64.release ut                 # All tests (release build)
```

## Protocol Compliance

The implementation follows **RFC 6455** WebSocket protocol:
- TEXT, BINARY, CONTINUATION frame types
- Control frames: CLOSE, PING, PONG
- Proper masking/unmasking for client frames
- Graceful connection closing with status codes
- Handshake via HTTP Upgrade mechanism

## Integration Context

This WebSocket component is part of the larger ArkCompiler toolchain:
- Used by `inspector/` for session management and message forwarding
- Used by `tooling/` for debugging and profiling protocol implementation
- Provides the transport layer for the Chrome DevTools Protocol

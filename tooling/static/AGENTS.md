# AGENTS

This file provides guidance for AI agents when working with code in the static tooling module.

## Project Overview

This is the **Static Tooling Module** of the Ark Compiler, part of the OpenHarmony ecosystem. It implements a Chrome DevTools Protocol-compatible debugging and profiling interface for **statically-typed ArkTS** (ArkTS-Sta) code running on OpenHarmony.

The inspector serves as the protocol bridge layer between:
- **DevEco Studio** (IDE) - sends debugging/profiling commands via WebSocket
- **ArkTS runtime** - executes the debugging/profiling logic

### Dynamic vs Static ArkTS Tooling

The Ark Compiler toolchain has two separate debugging implementations:

- **`@arkcompiler/toolchain/tooling/static`** (this directory) - For statically-typed ArkTS (ArkTS-Sta, with compile-time type checking)
- **`@arkcompiler/toolchain/tooling/dynamic`** - For dynamically-typed ArkTS (ArkTS-Dyn, JavaScript-like runtime behavior)

Both modules implement the Chrome DevTools Protocol but are optimized for their respective language variants.

## Architecture

## Directory Structure

```
static/
├── connection/            # Network connection layer (ASIO or OHOS WebSocket)
├── debugger/              # Core debugging functionality (breakpoints, stepping, object inspection)
├── evaluation/            # Expression evaluation engine
├── json_serialization/    # JSON protocol message serialization
├── types/                 # Protocol data type definitions
├── tests/                 # Unit tests (GTest/GMock)
├── inspector.cpp/h        # Main inspector coordinator (PtHooks interface)
├── inspector_server.cpp/h # WebSocket server wrapper
├── session_manager.cpp/h  # Debug session management
└── source_manager.cpp/h   # Source file management
```

### Core Components

**Inspector** (`inspector.cpp/h`)
- Implements `PtHooks` interface for VM event callbacks
- Manages debug sessions and breakpoints
- Coordinates between debugger and server
- Handles VM lifecycle events (MethodEntry, SingleStep, Exception, etc.)

**InspectorServer** (`inspector_server.cpp/h`)
- WebSocket server wrapper for client connections
- Message routing between WebSocket and debugger
- Protocol domain handlers (Debugger, Profiler, HeapProfiler, Runtime)

**Connection Layer**
- Two implementations: ASIO (cross-platform) or OHOS-specific
- Selected at build time via `PANDA_TOOLING_ASIO` flag
- Handles client connections and message serialization

**Debugger Components**
- `BreakpointStorage`: Manages breakpoints and conditions
- `DebugInfoCache`: Caches debug information for efficiency
- `ObjectRepository`: Manages object inspection and manipulation
- `ThreadState`: Tracks thread states during debugging

## Building

This project uses the **GN (Generate Ninja)** build system, integrated with the larger OpenHarmony/ArkCompiler build infrastructure.

### Build Commands

Full OpenHarmony build (from OpenHarmony root directory):

```bash
# Build the static inspector for statically-typed ArkTS
./build.sh --product-name rk3568 --build-target libarkinspector_plus
```

## Testing

### Test Structure

Located in `tests/` directory:
- `base64.cpp` - Base64 encoding/decoding tests
- `debug_info_cache.cpp` - Debug information caching tests
- `inspector_server.cpp` - WebSocket server functionality tests
- `object_repository.cpp` - Object inspection and manipulation tests
- `session_manager.cpp` - Debug session management tests
- `source_manager.cpp` - Source file management tests
- `thread_state.cpp` - Thread state management tests

Test targets: `arkinspector_tests` (defined in `tests/BUILD.gn`)

### Running Tests

From the OpenHarmony root directory:

```bash
# Download prebuilts first
./build/prebuilts_download.sh
# Build and run the tests
./build.sh --product-name rk3568 --build-target arkinspector_tests
```

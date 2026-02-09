# AGENTS

This file provides guidance for AI agents when working with code in the dynamic tooling module.

## Project Overview

This is the **Dynamic Tooling Module** of the Ark Compiler, part of the OpenHarmony ecosystem. It implements a Chrome DevTools Protocol-compatible debugging and profiling interface for **dynamically-typed ArkTS** (ArkTS-Dyn) code running on OpenHarmony.

### Dynamic vs Static ArkTS Tooling

The Ark Compiler toolchain has two separate debugging implementations:

- **`@arkcompiler/toolchain/tooling/dynamic`** (this directory) - For dynamically-typed ArkTS (ArkTS-Dyn, JavaScript-like runtime behavior)
- **`@arkcompiler/toolchain/tooling/static`** - For statically-typed ArkTS (ArkTS-Sta, with compile-time type checking)

Both modules implement the Chrome DevTools Protocol but are optimized for their respective language variants. The dynamic module uses the ECMAScript VM (EcmaVM) and supports JavaScript-compatible debugging features.

## Directory Structure

```
dynamic/
├── agent/                   # Chrome DevTools Protocol domain implementations (Debugger, Runtime, Profiler, etc.)
├── backend/                 # Low-level debugging hooks and execution control for EcmaVM
├── base/                    # Protocol base types and JSON serialization utilities
├── client/                  # Client-side tool
├── test/                    # Unit tests and integration tests
├── utils/                   # Shared utility functions
├── debugger_service.cpp/h   # Debugger service initialization and lifecycle management
├── dispatcher.cpp/h         # Protocol command routing and dispatching
└── protocol_handler.cpp/h   # Debug client communication and message handling
```

## Architecture

The implementation follows a layered architecture with clear separation between protocol handling, domain logic, and backend execution:

### Core Components

**Protocol Handler** (`protocol_handler.cpp/h`)
- Manages communication with debug clients over TCP
- Handles message queuing and dispatching
- Registered with the VM's JsDebuggerManager
- Coordinates response callbacks to the client

**Dispatcher** (`dispatcher.cpp/h`)
- Routes Chrome DevTools Protocol commands to appropriate handlers
- Parses JSON protocol messages and validates them
- Dispatches to domain-specific implementation classes
- Supports cross-language debugging scenarios

**Agent Layer** (`agent/`)
Protocol domain implementations for Chrome DevTools Protocol:

- `DebuggerImpl` - Breakpoints, stepping, call stacks, exception debugging
- `RuntimeImpl` - Runtime evaluation, object properties, remote object mapping
- `ProfilerImpl` - CPU profiling with sampling (excluded on Windows/mac)
- `HeapProfilerImpl` - Memory heap snapshots and profiling (excluded on Windows/mac)
- `TargetImpl` - Target discovery and lifecycle management
- `PageImpl` - Page/resource management and script loading
- `DOMImpl`, `CSSImpl`, `AnimationImpl`, `OverlayImpl` - Inspection and UI debugging features
- `TracingImpl` - Performance tracing capabilities

**Backend Layer** (`backend/`)
- `js_pt_hooks.cpp/h` - Implements PtHooks interface, the bridge between EcmaVM and debugger
  - EcmaVM calls these hooks during execution (SingleStep, Breakpoint, Exception, etc.)
  - Returns pause/continue decisions to VM
- `debugger_executor.cpp/h` - Variable access and modification in paused execution context
  - GetValue/SetValue for Local, Lexical, Module, and Global scopes
- `js_single_stepper.cpp/h` - Single stepping controller (STEP_INTO, STEP_OVER, STEP_OUT)
  - Monitors bytecode offset, stack depth, and step ranges to determine when to pause

**Protocol Base** (`base/`)
- Chrome DevTools Protocol type definitions and JSON handling
- `pt_json.cpp/h` - JSON serialization/deserialization
- `PtParams` - Protocol parameter types
- `PtReturns` - Protocol response types
- `PtEvents` - Protocol event types
- `PtScript` - Script representation and metadata
- `pt_base64.cpp/h` - Base64 encoding/decoding utilities

### Communication Flow

```
Client Commands:
Client (TCP) → ProtocolHandler → Dispatcher → AgentImpl → BackendHooks → EcmaVM
                                      ↓
                              PtJSON serialization

VM Events (Reverse Flow):
EcmaVM → PtHooks (JSPtHooks) → AgentImpl → ProtocolHandler → Client
```


### How Debugging Works: SingleStep Example

When a user clicks "Step Over" in DevEco Studio:

1. **Client → VM**: CDP command `Debugger.stepOver` sent via WebSocket
2. **Dispatcher** routes to `DebuggerImpl::StepOver()`
3. **DebuggerImpl** creates a `SingleStepper` (STEP_OVER mode) and sets `pauseOnNextByteCode_ = true`
4. **EcmaVM** continues executing bytecode
5. **EcmaVM** calls `hooks_->SingleStep(location)` after each bytecode instruction
6. **JSPtHooks::SingleStep()** calls `DebuggerImpl::NotifySingleStep(location)`
7. **SingleStepper** checks if:
   - Current bytecode offset is out of step range?
   - Stack depth changed (for STEP_OVER/STEP_OUT)?
8. If step complete → **JSPtHooks** returns `true` → **EcmaVM pauses**
9. **DebuggerImpl** sends `Debugger.paused` event to client with call stack and variables

**Key Point**: PtHooks is the bridge - EcmaVM doesn't know about DebuggerImpl, it just calls the interface.

## Building

This project uses the **GN (Generate Ninja)** build system, integrated with the larger OpenHarmony/ArkCompiler build infrastructure.

### Build Commands

Full OpenHarmony build (from OpenHarmony root directory):

```bash
# Build the debugger for dynamically-typed ArkTS
./build.sh --product-name rk3568 --build-target libark_ecma_debugger

# Build the CLI debugger tool
./build.sh --product-name rk3568 --build-target arkdb

# Build the multi-threaded testing tool
./build.sh --product-name rk3568 --build-target ark_multi
```

## Testing

### Test Structure

Located in `test/` directory:
- `debugger_impl_test.cpp` - Core debugger functionality tests (breakpoints, stepping, call stacks)
- `dispatcher_test.cpp` - Protocol command dispatch and routing tests
- `pt_json_test.cpp`, `pt_params_test.cpp`, `pt_returns_test.cpp` - Protocol JSON serialization/deserialization tests
- `js_pt_hooks_test.cpp` - Backend hook integration tests with EcmaVM
- ...

Test targets: `DebuggerTest` and related test suites (defined in `test/BUILD.gn`)

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
cd ark_standalone_build
python ark.py x64.debug DebuggerTestAction        # Core tests (debug build)
python ark.py x64.debug DebuggerEntryTestAction   # Entry tests (debug build)
python ark.py x64.debug DebuggerClientTestAction  # Client tests (debug build)
...
python ark.py x64.release ut                       # All tests (release build)
```

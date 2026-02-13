# AGENTS

This file provides guidance for AI agents when working with code in the inspector component.

## Project Overview

This is the **Inspector** component of the ArkCompiler Toolchain for OpenHarmony. It provides the debugging and profiling protocol layer that connects DevEco Studio with ArkTS applications during development.

The inspector serves as a **protocol bridge layer** between:
- **DevEco Studio** (IDE) - sends debugging/profiling commands via WebSocket connection
- **Ark tooling libraries** - implements the actual debugging/profiling logic

## Architecture

### Core Inspector Components

- **Core Inspector** (`inspector.cpp/h`, `ws_server.cpp/h`) - Main coordinator managing debug sessions with WebSocket server wrapper for connections and message routing between WebSocket and debugger
- **Connect Server** (`connect_server.cpp/h`, `connect_inspector.cpp/h`) - Manages multiple debug sessions and provides callback registration for non-debugger domains (ArkUI, WMS, etc.)
- **Library Loader** (`library_loader.cpp/h`) - Cross-platform dynamic library loading for the debugger implementation
- **Static Inspector** (`init_static.cpp/h`) - Debugger implementation for statically-typed ArkTS (ArkTS-Sta); provides similar functionality to Core Inspector for the static type system variant

## Building

This project uses the **GN (Generate Ninja)** build system, integrated with the larger OpenHarmony/ArkCompiler build infrastructure.

### Build Commands

Full OpenHarmony build (from OpenHarmony root directory):

```bash
./build.sh --product-name rk3568 --build-target ark_debugger
./build.sh --product-name rk3568 --build-target connectserver_debugger
```

## Testing

### Test Structure

Located in `test/` directory:
- `inspector_test.cpp` - Core inspector functionality tests (StartDebug, StartDebugForSocketpair, StopDebug, etc.)
- `connect_server_test.cpp` - Connection server and IDE integration tests

Test targets: `InspectorConnectTest` (defined in `test/BUILD.gn`)

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
cd ark_standalone_build
python ark.py x64.debug InspectorConnectTestAction  # Single test (debug build)
python ark.py x64.release ut                        # All tests (release build)
```

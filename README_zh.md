# 方舟工具链组件

- [方舟工具链](#方舟工具链)
  - [简介<a name="section0001"></a>](#简介)
  - [目录<a name="section0002"></a>](#目录)
  - [使用说明<a name="section0003"></a>](#使用说明)
  - [编译构建<a name="section0004"></a>](#编译构建)
  - [相关仓<a name="section0005"></a>](#相关仓)

## 简介<a name="section0001"></a>
方舟工具链（ArkCompiler Toolchain）为开发者提供了调试(Debugger), 性能调优(CPUProfiler), 内存调优(HeapProfiler)等针对ArkTS应用程序的调试调优工具。如需查看调试调优工具具体支持特性，请前往[调试调优工具介绍](docs/debugger-and-profiler-tools-introduction.md)。

### 方舟工具链架构图

![](/figures/arkcompiler-toolchain-arch.png)

方舟工具链提供的调试调优能力，需通过DevEco Studio使用，同时依赖于ArkCompiler Runtime提供运行时相关信息。

### 调试调优协议实现

调试调优实现划分不同的域，每个域定义其所支持的协议命令：
- Debugger域实现包括断点、单步、停止执行、恢复执行以及CallFrame求值等调试功能。
- Profiler域实现包括启动采样、停止采样以及设置采样间隔等CPU调优功能。
- HeapProfiler域实现包括启动采样、停止采样、垃圾收集以及获取堆内存快照等堆内存调优功能。
- Runtime域实现包括堆获取内存使用情况、获取对象属性等运行时相关功能。

## 目录<a name="section0002"></a>

```
/arkcompiler/toolchain
├─ tooling             # 调试调优协议实现
├─ inspector           # 调试协议对接层，包括会话连接，消息转发等
└─ websocket           # websocket协议实现
```

## 使用说明<a name="section0003"></a>

调试应用时，需要配套DevEco Studio和SDK使用，详细的指导请前往[应用调试](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ide_debug_device-0000001053822404)。此外，对调试调优特性支持情况可前往[DevEco Studio版本说明](https://developer.harmonyos.com/cn/docs/documentation/doc-releases/release_notes-0000001057597449)查看详细说明。


## 编译构建<a name="section0004"></a>
1. inspector
```sh
$ ./build.sh --product-name rk3568 --build-target ark_debugger
```
2. tooling
```sh
$ ./build.sh --product-name rk3568 --build-target libark_ecma_debugger
```

## 相关仓<a name="section0005"></a>

**[arkcompiler\_toolchain](https://gitee.com/openharmony/arkcompiler_toolchain)**

[arkcompiler\_ets\_runtime](https://gitee.com/openharmony/arkcompiler_ets_runtime)

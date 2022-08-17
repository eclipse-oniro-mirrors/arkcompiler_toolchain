# 方舟工具链组件

- [方舟工具链](#方舟工具链)
  - [简介<a name="section0001"></a>](#简介)
  - [目录<a name="section0002"></a>](#目录)
  - [使用说明<a name="section0003"></a>](#使用说明)
  - [编译构建<a name="section0004"></a>](#编译构建)
  - [约束<a name="section0005"></a>](#约束)
  - [相关仓<a name="section0006"></a>](#相关仓)

## 简介<a name="section0001"></a>

方舟工具链（ArkCompiler Toolchain）为开发者提供了一套OpenHarmony应用程序调试调优工具，其功能包括单步调试、断点调试、watch变量及表达式、cpu profiler和heap profiler等，并支持多实例和worker调试。

**方舟工具链架构图：**

![](/figures/arkcompiler-toolchain-arch.png)

## 目录<a name="section0002"></a>

```
/arkcompiler/toolchain
├─ tooling             # 调试调优实现
└─ inspector           # inspector调试协议，包括会话连接，消息转发等
```

## 使用说明<a name="section0003"></a>

调试应用时，需要配套DevEco Studio使用，详细的指导请前往[应用调试](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ide_debug_device-0000001053822404)。此外，对调试调优特性支持情况可前往[DevEco Studio版本说明](https://developer.harmonyos.com/cn/docs/documentation/doc-releases/release_notes-0000001057597449)查看详细说明。


## 编译构建<a name="section0004"></a>
1. inspector
```sh
$ ./build.sh --product-name rk3568 --build-target ark_debugger
```
2. tooling
```sh
$ ./build.sh --product-name rk3568 --build-target libark_ecma_debugger
```

## 约束<a name="section0005"></a>
- 需配套DevEco Studio和SDK使用

## 相关仓<a name="section0006"></a>

**[arkcompiler\_toolchain](https://gitee.com/openharmony/arkcompiler_toolchain)**

[arkcompiler\_ets\_runtime](https://gitee.com/openharmony/arkcompiler_ets_runtime)

# 方舟编译器工具链组件

- [方舟编译器工具链](#方舟编译器工具链)
  - [简介<a name="section0001"></a>](#简介)
  - [目录<a name="section0002"></a>](#目录)
  - [使用说明<a name="section0003"></a>](#使用说明)
  - [约束<a name="section0004"></a>](#约束)
  - [相关仓<a name="section0005"></a>](#相关仓)

## 简介<a name="section0001"></a>

方舟编译器工具链（ArkCompiler Toolchain）为开发者提供了一套OpenHarmony应用程序调试调优工具，其功能包括单步调试、断点调试、watch变量及表达式、cpu profiler和heap profiler等，并支持多实例和worker调试。


## 目录<a name="section0002"></a>

```
/arkcompiler/toolchain
├─ tooling             # 调试器实现
└─ inspector           # inspector检查器，包括会话连接，协议消息转发等
```

## 使用说明<a name="section0003"></a>

1. 连接好设备，新建或将已有工程导入[DevEco Studio](https://developer.harmonyos.com/cn/develop/deveco-studio)进行编译构建;
2. 编译成功后，点击右上角Debug按钮;
3. 安装运行后，即可在设备上查看应用示例运行效果，以及进行相关调试。


## 约束<a name="section0004"></a>
1. IDE版本及配套SDK问题，可前往[DevEco Studio版本说明](https://developer.harmonyos.com/cn/docs/documentation/doc-releases/ohos-release-notes-0000001226452454)来查看详细内容。


## 相关仓<a name="section0005"></a>

**[arkcompiler\_toolchain](https://gitee.com/openharmony/arkcompiler_toolchain)**

[arkcompiler\_ets\_runtime](https://gitee.com/openharmony/arkcompiler_ets_runtime)

/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_TOOLING_AGENT_DEBUGGER_IMPL_H
#define ECMASCRIPT_TOOLING_AGENT_DEBUGGER_IMPL_H

#include "agent/runtime_impl.h"
#include "backend/js_pt_hooks.h"
#include "tooling/base/pt_params.h"
#include "backend/js_single_stepper.h"
#include "dispatcher.h"

#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/debugger/js_pt_method.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
namespace test {
class TestHooks;
}  // namespace test

enum class DebuggerState { DISABLED, ENABLED, PAUSED };
enum class DebuggerFeature { LAUNCH_ACCELERATE, UNKNOWN };
class DebuggerImpl final {
public:
    DebuggerImpl(const EcmaVM *vm, ProtocolChannel *channel, RuntimeImpl *runtime);
    ~DebuggerImpl();

    // event
    bool NotifyScriptParsed(const std::string &fileName,
                            std::string_view entryPoint = "func_main_0");
    bool CheckScriptParsed(const std::string &fileName);
    bool NotifyScriptParsedBySendable(JSHandle<Method> method);
    bool MatchUrlAndFileName(const std::string &url, const std::string &fileName);
    bool NotifySingleStep(const JSPtLocation &location);
    void NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason);
    void GeneratePausedInfo(PauseReason reason,
                           std::vector<std::string> &hitBreakpoints,
                           const Local<JSValueRef> &exception);
    bool NotifyNativeOut();
    void NotifyHandleProtocolCommand();
    std::vector<void *> GetNativeAddr();
    void NotifyNativeCalling(const void *nativeAddress);
    void NotifyNativeReturn(const void *nativeAddress);
    void NotifyReturnNative();
    bool IsUserCode(const void *nativeAddress);
    void SetDebuggerState(DebuggerState debuggerState);
    void SetNativeOutPause(bool nativeOutPause);
    void AddBreakpointDetail(const std::string &url, int32_t lineNumber,
        std::string *outId, std::vector<std::unique_ptr<Location>> *outLocations);
    bool GenerateAsyncFrames(std::shared_ptr<AsyncStack> asyncStack, bool skipTopFrame);
    bool GenerateAsyncFrame(StackFrame *stackFrame, const FrameHandler *frameHandler);
    void SetPauseOnNextByteCode(bool pauseOnNextByteCode);

    DispatchResponse ContinueToLocation(const ContinueToLocationParams &params);
    DispatchResponse Enable(const EnableParams &params, UniqueDebuggerId *id);
    DispatchResponse Disable();
    DispatchResponse EvaluateOnCallFrame(const EvaluateOnCallFrameParams &params,
                                         std::unique_ptr<RemoteObject> *result);
    DispatchResponse GetPossibleBreakpoints(const GetPossibleBreakpointsParams &params,
                                            std::vector<std::unique_ptr<BreakLocation>> *outLocations);
    DispatchResponse GetScriptSource(const GetScriptSourceParams &params, std::string *source);
    DispatchResponse Pause();
    DispatchResponse RemoveBreakpoint(const RemoveBreakpointParams &params);
    DispatchResponse RemoveBreakpointsByUrl(const RemoveBreakpointsByUrlParams &params);
    DispatchResponse Resume(const ResumeParams &params);
    DispatchResponse SetAsyncCallStackDepth(const SetAsyncCallStackDepthParams &params);
    DispatchResponse SetBreakpointByUrl(const SetBreakpointByUrlParams &params, std::string *outId,
                                        std::vector<std::unique_ptr<Location>> *outLocations,
                                        bool isSmartBreakpoint = false);
    DispatchResponse SetBreakpointsActive(const SetBreakpointsActiveParams &params);
    DispatchResponse GetPossibleAndSetBreakpointByUrl(const GetPossibleAndSetBreakpointParams &params,
        std::vector<std::shared_ptr<BreakpointReturnInfo>> &outLocations);
    DispatchResponse SetPauseOnExceptions(const SetPauseOnExceptionsParams &params);
    DispatchResponse SetSkipAllPauses(const SetSkipAllPausesParams &params);
    DispatchResponse SetNativeRange(const SetNativeRangeParams &params);
    DispatchResponse ResetSingleStepper(const ResetSingleStepperParams &params);
    DispatchResponse StepInto(const StepIntoParams &params);
    DispatchResponse SmartStepInto(const SmartStepIntoParams &params);
    DispatchResponse StepOut();
    DispatchResponse StepOver(const StepOverParams &params);
    DispatchResponse SetBlackboxPatterns();
    DispatchResponse SetMixedDebugEnabled(const SetMixedDebugParams &params);
    DispatchResponse ReplyNativeCalling(const ReplyNativeCallingParams &params);
    DispatchResponse DropFrame(const DropFrameParams &params);
    DispatchResponse ClientDisconnect();
    DispatchResponse CallFunctionOn(
            const CallFunctionOnParams &params,
            std::unique_ptr<RemoteObject> *outRemoteObject,
            std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails);
    DispatchResponse SaveAllPossibleBreakpoints(const SaveAllPossibleBreakpointsParams &params);
    DispatchResponse SetSymbolicBreakpoints(const SetSymbolicBreakpointsParams &params);
    DispatchResponse RemoveSymbolicBreakpoints(const RemoveSymbolicBreakpointsParams &params);

    /**
     * @brief: match first script and callback
     *
     * @return: true means matched and callback execute success
     */
    template<class Callback>
    bool MatchScripts(const Callback &cb, const std::string &matchStr, ScriptMatchType type) const
    {
        for (const auto &script : scripts_) {
            std::string value;
            switch (type) {
                case ScriptMatchType::URL: {
                    value = script.second->GetUrl();
                    break;
                }
                case ScriptMatchType::FILE_NAME: {
                    value = script.second->GetFileName();
                    break;
                }
                case ScriptMatchType::HASH: {
                    value = script.second->GetHash();
                    break;
                }
                default: {
                    return false;
                }
            }
            if (matchStr == value) {
                return cb(script.second.get());
            }
        }
        return false;
    }

    std::vector<PtScript *> MatchAllScripts(const std::string &url) const
    {
        std::vector<PtScript *> result;
        for (const auto &script : scripts_) {
            if (url == script.second->GetUrl()) {
                result.push_back(script.second.get());
            }
        }
        return result;
    }
    bool GenerateCallFrames(std::vector<std::unique_ptr<CallFrame>> *callFrames, bool getScope);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(ProtocolChannel *channel, std::unique_ptr<DebuggerImpl> debugger)
            : DispatcherBase(channel), debugger_(std::move(debugger)) {}
        ~DispatcherImpl() override = default;

        void ContinueToLocation(const DispatchRequest &request);
        std::string GetJsFrames();
        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
        void EvaluateOnCallFrame(const DispatchRequest &request);
        void GetPossibleBreakpoints(const DispatchRequest &request);
        void GetScriptSource(const DispatchRequest &request);
        void Pause(const DispatchRequest &request);
        void RemoveBreakpoint(const DispatchRequest &request);
        void RemoveBreakpointsByUrl(const DispatchRequest &request);
        void Resume(const DispatchRequest &request);
        void SetAsyncCallStackDepth(const DispatchRequest &request);
        void SetBreakpointByUrl(const DispatchRequest &request);
        void SetBreakpointsActive(const DispatchRequest &request);
        void SetPauseOnExceptions(const DispatchRequest &request);
        void SetSkipAllPauses(const DispatchRequest &request);
        void SetNativeRange(const DispatchRequest &request);
        void ResetSingleStepper(const DispatchRequest &request);
        void StepInto(const DispatchRequest &request);
        void SmartStepInto(const DispatchRequest &request);
        void StepOut(const DispatchRequest &request);
        void StepOver(const DispatchRequest &request);
        void SetMixedDebugEnabled(const DispatchRequest &request);
        void SetBlackboxPatterns(const DispatchRequest &request);
        void ReplyNativeCalling(const DispatchRequest &request);
        void GetPossibleAndSetBreakpointByUrl(const DispatchRequest &request);
        void DropFrame(const DispatchRequest &request);
        void ClientDisconnect(const DispatchRequest &request);
        void CallFunctionOn(const DispatchRequest &request);
        void SaveAllPossibleBreakpoints(const DispatchRequest &request);
        void SetSymbolicBreakpoints(const DispatchRequest &request);
        void RemoveSymbolicBreakpoints(const DispatchRequest &request);

        enum class Method {
            CONTINUE_TO_LOCATION,
            ENABLE,
            DISABLE,
            EVALUATE_ON_CALL_FRAME,
            GET_POSSIBLE_BREAKPOINTS,
            GET_SCRIPT_SOURCE,
            PAUSE,
            REMOVE_BREAKPOINT,
            REMOVE_BREAKPOINTS_BY_URL,
            RESUME,
            SET_ASYNC_CALL_STACK_DEPTH,
            SET_BREAKPOINT_BY_URL,
            SET_BREAKPOINTS_ACTIVE,
            SET_PAUSE_ON_EXCEPTIONS,
            SET_SKIP_ALL_PAUSES,
            STEP_INTO,
            SMART_STEP_INTO,
            STEP_OUT,
            STEP_OVER,
            SET_MIXED_DEBUG_ENABLED,
            SET_BLACKBOX_PATTERNS,
            REPLY_NATIVE_CALLING,
            GET_POSSIBLE_AND_SET_BREAKPOINT_BY_URL,
            DROP_FRAME,
            SET_NATIVE_RANGE,
            RESET_SINGLE_STEPPER,
            CLIENT_DISCONNECT,
            CALL_FUNCTION_ON,
            SAVE_ALL_POSSIBLE_BREAKPOINTS,
            SET_SYMBOLIC_BREAKPOINTS,
            REMOVE_SYMBOLIC_BREAKPOINTS,
            UNKNOWN
        };
        Method GetMethodEnum(const std::string& method);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        std::unique_ptr<DebuggerImpl> debugger_ {};
    };

private:
    NO_COPY_SEMANTIC(DebuggerImpl);
    NO_MOVE_SEMANTIC(DebuggerImpl);

    std::string Trim(const std::string &str);
    DebugInfoExtractor *GetExtractor(const JSPandaFile *jsPandaFile);
    std::vector<DebugInfoExtractor *> GetExtractors(const std::string &url);
    std::optional<std::string> CmptEvaluateValue(CallFrameId callFrameId, const std::string &expression,
        std::unique_ptr<RemoteObject> *result);
    bool GenerateCallFrame(CallFrame *callFrame, const FrameHandler *frameHandler, CallFrameId frameId, bool getScope);
    void GenerateScopeChains(bool getScope, const FrameHandler *frameHandler, const JSPandaFile *jsPandaFile,
        std::vector<std::unique_ptr<Scope>> &scopeChain, std::unique_ptr<RemoteObject> &thisObj);
    void SaveCallFrameHandler(const FrameHandler *frameHandler);
    std::unique_ptr<Scope> GetLocalScopeChain(const FrameHandler *frameHandler,
        std::unique_ptr<RemoteObject> *thisObj);
    std::unique_ptr<Scope> GetModuleScopeChain(const FrameHandler *frameHandler);
    std::unique_ptr<Scope> GetGlobalScopeChain(const FrameHandler *frameHandler);
    std::vector<std::unique_ptr<Scope>> GetClosureScopeChains(const FrameHandler *frameHandler,
        std::unique_ptr<RemoteObject> *thisObj);
    void GetLocalVariables(const FrameHandler *frameHandler, panda_file::File::EntityId methodId,
        const JSPandaFile *jsPandaFile, Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj);
    void CleanUpOnPaused();
    void CleanUpRuntimeProperties();
    void UpdateScopeObject(const FrameHandler *frameHandler, std::string_view varName,
        Local<JSValueRef> newVal, const std::string& scope);
    void ClearSingleStepper();
    Local<JSValueRef> ConvertToLocal(const std::string &varValue);
    bool DecodeAndCheckBase64(const std::string &src, std::vector<uint8_t> &dest);
    bool IsSkipLine(const JSPtLocation &location);
    bool CheckPauseOnException();
    bool IsWithinVariableScope(const LocalVariableInfo &localVariableInfo, uint32_t bcOffset);
    bool ProcessSingleBreakpoint(const BreakpointInfo &breakpoint,
        std::vector<std::shared_ptr<BreakpointReturnInfo>> &outLocations);
    bool IsVariableSkipped(const std::string &varName);
    Local<FunctionRef> CheckAndGenerateCondFunc(const std::optional<std::string> &condition);
    void InitializeExtendedProtocolsList();
    bool NeedToSetBreakpointsWhenParsingScript(const std::string &url);
    std::vector<std::shared_ptr<BreakpointReturnInfo>> SetBreakpointsWhenParsingScript(const std::string &url);
    void SavePendingBreakpoints(const SaveAllPossibleBreakpointsParams &params);
    bool InsertIntoPendingBreakpoints(const BreakpointInfo &breakpoint);
    void SaveParsedScriptsAndUrl(const std::string &fileName, const std::string &url,
        const std::string &recordName, const std::string &source = "");
    void EnableDebuggerFeatures(const EnableParams &params);
    DebuggerFeature GetDebuggerFeatureEnum(std::string &option);
    void EnableFeature(DebuggerFeature feature);

    const std::unordered_set<std::string> &GetRecordName(const std::string &url)
    {
        static const std::unordered_set<std::string> recordName;
        auto iter = recordNames_.find(url);
        if (iter != recordNames_.end()) {
            return iter->second;
        }
        return recordName;
    }
    void EnableLaunchAccelerateMode()
    {
        breakOnStartEnable_ = false;
    }
    bool IsLaunchAccelerateMode() const
    {
        return !breakOnStartEnable_;
    }

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}
        ~Frontend() = default;

        void BreakpointResolved(const EcmaVM *vm);
        void Paused(const EcmaVM *vm, const tooling::Paused &paused);
        void Resumed(const EcmaVM *vm);
        void NativeCalling(const EcmaVM *vm, const tooling::NativeCalling &nativeCalling);
        void MixedStack(const EcmaVM *vm, const tooling::MixedStack &mixedStack);
        void ScriptFailedToParse(const EcmaVM *vm);
        void ScriptParsed(const EcmaVM *vm, const PtScript &script);
        void WaitForDebugger(const EcmaVM *vm);
        void RunIfWaitingForDebugger(const EcmaVM *vm);

    private:
        bool AllowNotify(const EcmaVM *vm) const;

        ProtocolChannel *channel_ {nullptr};
    };

    const EcmaVM *vm_ {nullptr};
    Frontend frontend_;

    RuntimeImpl *runtime_ {nullptr};
    std::unique_ptr<JSPtHooks> hooks_ {nullptr};
    JSDebugger *jsDebugger_ {nullptr};

    std::unordered_map<std::string, std::unordered_set<std::string>> recordNames_ {};
    std::unordered_map<std::string, std::unordered_set<std::string>> urlFileNameMap_ {};
    std::unordered_map<ScriptId, std::shared_ptr<PtScript>> scripts_ {};
    PauseOnExceptionsState pauseOnException_ {PauseOnExceptionsState::NONE};
    DebuggerState debuggerState_ {DebuggerState::ENABLED};
    bool pauseOnNextByteCode_ {false};
    bool breakpointsState_ {true};
    bool skipAllPausess_ {false};
    bool mixStackEnabled_ {false};
    int32_t maxAsyncCallChainDepth_ {0};
    std::unique_ptr<SingleStepper> singleStepper_ {nullptr};
    Location location_ {};

    std::unique_ptr<SingleStepper> nativeOut_ {nullptr};
    std::vector<void *>  nativePointer_;

    bool nativeOutPause_ {false};
    std::vector<NativeRange> nativeRanges_ {};
    std::unordered_map<JSTaggedType *, std::unordered_map<std::string,
        std::vector<RemoteObjectId>>> scopeObjects_ {};
    std::vector<std::shared_ptr<FrameHandler>> callFrameHandlers_;
    JsDebuggerManager::ObjectUpdaterFunc updaterFunc_ {nullptr};
    JsDebuggerManager::SingleStepperFunc stepperFunc_ {nullptr};
    JsDebuggerManager::ReturnNativeFunc returnNative_ {nullptr};
    std::vector<std::string> debuggerExtendedProtocols_ {};
    // For launch accelerate mode
    std::unordered_map<std::string, CUnorderedSet<std::shared_ptr<BreakpointInfo>, HashBreakpointInfo>>
        breakpointPendingMap_ {};
    bool breakOnStartEnable_ {true};

    friend class JSPtHooks;
    friend class test::TestHooks;
    friend class DebuggerImplFriendTest;
};
}  // namespace panda::ecmascript::tooling
#endif
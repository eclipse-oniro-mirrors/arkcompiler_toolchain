/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "agent/runtime_impl.h"

#include "protocol_channel.h"

namespace panda::ecmascript::tooling {
// Whenever adding a new protocol which is not a standard CDP protocol,
// must add its methodName to the runtimeProtocolsList
void RuntimeImpl::InitializeExtendedProtocolsList()
{
    std::vector<std::string> runtimeProtocolList {};
    runtimeExtendedProtocols_ = std::move(runtimeProtocolList);
}

void RuntimeImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    Method method = GetMethodEnum(request.GetMethod());
    LOG_DEBUGGER(DEBUG) << "dispatch [" << request.GetMethod() << "] to RuntimeImpl";
    switch (method) {
        case Method::ENABLE:
            Enable(request);
            break;
        case Method::DISABLE:
            Disable(request);
            break;
        case Method::GET_PROPERTIES:
            GetProperties(request);
            break;
        case Method::RUN_IF_WAITING_FOR_DEBUGGER:
            RunIfWaitingForDebugger(request);
            break;
        case Method::GET_HEAP_USAGE:
            GetHeapUsage(request);
            break;
        default:
            SendResponse(request, DispatchResponse::Fail("unknown method: " + request.GetMethod()));
            break;
    }
}

RuntimeImpl::DispatcherImpl::Method RuntimeImpl::DispatcherImpl::GetMethodEnum(const std::string& method)
{
    if (method == "enable") {
        return Method::ENABLE;
    } else if (method == "disable") {
        return Method::DISABLE;
    } else if (method == "getProperties") {
        return Method::GET_PROPERTIES;
    } else if (method == "runIfWaitingForDebugger") {
        return Method::RUN_IF_WAITING_FOR_DEBUGGER;
    } else if (method == "getHeapUsage") {
        return Method::GET_HEAP_USAGE;
    } else {
        return Method::UNKNOWN;
    }
}

void RuntimeImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->Enable();
    runtime_->InitializeExtendedProtocolsList();
    EnableReturns result(runtime_->runtimeExtendedProtocols_);
    SendResponse(request, response, result);
}

void RuntimeImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->Disable();
    SendResponse(request, response);
}

void RuntimeImpl::DispatcherImpl::RunIfWaitingForDebugger(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->RunIfWaitingForDebugger();
    SendResponse(request, response);
}

void RuntimeImpl::DispatcherImpl::GetProperties(const DispatchRequest &request)
{
    std::unique_ptr<GetPropertiesParams> params = GetPropertiesParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    std::vector<std::unique_ptr<PropertyDescriptor>> outPropertyDesc;
    std::optional<std::vector<std::unique_ptr<InternalPropertyDescriptor>>> outInternalDescs;
    std::optional<std::vector<std::unique_ptr<PrivatePropertyDescriptor>>> outPrivateProperties;
    std::optional<std::unique_ptr<ExceptionDetails>> outExceptionDetails;
    DispatchResponse response = runtime_->GetProperties(*params, &outPropertyDesc, &outInternalDescs,
        &outPrivateProperties, &outExceptionDetails);
    if (outExceptionDetails) {
        ASSERT(outExceptionDetails.value() != nullptr);
        LOG_DEBUGGER(WARN) << "GetProperties thrown an exception";
    }
    GetPropertiesReturns result(std::move(outPropertyDesc),
        std::move(outInternalDescs),
        std::move(outPrivateProperties),
        std::move(outExceptionDetails));
    SendResponse(request, response, result);
}

void RuntimeImpl::DispatcherImpl::GetHeapUsage(const DispatchRequest &request)
{
    double usedSize = 0;
    double totalSize = 0;
    DispatchResponse response = runtime_->GetHeapUsage(&usedSize, &totalSize);
    GetHeapUsageReturns result(usedSize, totalSize);
    SendResponse(request, response, result);
}

bool RuntimeImpl::Frontend::AllowNotify() const
{
    return channel_ != nullptr;
}

void RuntimeImpl::Frontend::RunIfWaitingForDebugger()
{
    if (!AllowNotify()) {
        return;
    }

    channel_->RunIfWaitingForDebugger();
}

DispatchResponse RuntimeImpl::Enable()
{
    internalObjects_ = Global<MapRef>(vm_, MapRef::New(vm_));
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::Disable()
{
    internalObjects_.FreeGlobalHandleAddr();
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::RunIfWaitingForDebugger()
{
    frontend_.RunIfWaitingForDebugger();
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::GetHeapUsage(double *usedSize, double *totalSize)
{
#ifdef ECMASCRIPT_SUPPORT_HEAPPROFILER
    *totalSize = static_cast<double>(DFXJSNApi::GetHeapTotalSize(vm_));
    *usedSize = static_cast<double>(DFXJSNApi::GetHeapUsedSize(vm_));
#else
    *totalSize = 0;
    *usedSize = 0;
#endif
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::GetProperties(const GetPropertiesParams &params,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc,
    [[maybe_unused]] std::optional<std::vector<std::unique_ptr<InternalPropertyDescriptor>>> *outInternalDescs,
    [[maybe_unused]] std::optional<std::vector<std::unique_ptr<PrivatePropertyDescriptor>>> *outPrivateProps,
    [[maybe_unused]] std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails)
{
    RemoteObjectId objectId = params.GetObjectId();
    bool isOwn = params.GetOwnProperties();
    bool isAccessorOnly = params.GetAccessPropertiesOnly();
    auto iter = properties_.find(objectId);
    if (iter == properties_.end()) {
        LOG_DEBUGGER(ERROR) << "RuntimeImpl::GetProperties Unknown object id: " << objectId;
        return DispatchResponse::Fail("Unknown object id");
    }
    Local<JSValueRef> value = Local<JSValueRef>(vm_, iter->second);
    if (value.IsEmpty() || !value->IsObject(vm_)) {
        LOG_DEBUGGER(ERROR) << "RuntimeImpl::GetProperties should a js object";
        return DispatchResponse::Fail("Not a object");
    }
    bool skipProto = false;
    if (!internalObjects_.IsEmpty() && internalObjects_->Get(vm_, value)->IsNumber()) {
        if (static_cast<ArkInternalValueType>(internalObjects_->Get(vm_, value)->ToNumber(vm_)->Value()) ==
            ArkInternalValueType::Entry || static_cast<ArkInternalValueType>(internalObjects_->Get(vm_, value)->
            ToNumber(vm_)->Value()) == ArkInternalValueType::Scope) {
            skipProto = true;
        }
    }
    if (value->IsArrayBuffer(vm_)) {
        Local<ArrayBufferRef> arrayBufferRef(value);
        AddTypedArrayRefs(arrayBufferRef, outPropertyDesc);
    } else if (value->IsSharedArrayBuffer(vm_)) {
        Local<ArrayBufferRef> arrayBufferRef(value);
        AddSharedArrayBufferRefs(arrayBufferRef, outPropertyDesc);
    } else if (value->IsProxy(vm_)) {
        GetProxyValue(value, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsMapIterator(vm_)) {
        GetMapIteratorValue(value, outPropertyDesc);
    } else if (value->IsSetIterator(vm_)) {
        GetSetIteratorValue(value, outPropertyDesc);
    } else if (value->IsJSPrimitiveRef(vm_) && value->IsJSPrimitiveNumber(vm_)) {
        GetPrimitiveNumberValue(value, outPropertyDesc);
    } else if (value->IsJSPrimitiveRef(vm_) && value->IsJSPrimitiveString(vm_)) {
        GetPrimitiveStringValue(value, outPropertyDesc);
    } else if (value->IsJSPrimitiveRef(vm_) && value->IsJSPrimitiveBoolean(vm_)) {
        GetPrimitiveBooleanValue(value, outPropertyDesc);
    } else if (value->IsGeneratorFunction(vm_)) {
        GetGeneratorFunctionValue(value, outPropertyDesc);
    } else if (value->IsGeneratorObject(vm_)) {
        GetGeneratorObjectValue(value, outPropertyDesc);
    } else if (value->IsJSNumberFormat(vm_)) {
        GetNumberFormatValue(value, outPropertyDesc);
    } else if (value->IsJSCollator(vm_)) {
        GetCollatorValue(value, outPropertyDesc);
    } else if (value->IsJSDateTimeFormat(vm_)) {
        GetDateTimeFormatValue(value, outPropertyDesc);
    } else if (value->IsSharedMap(vm_)) {
        GetSharedMapValue(value, outPropertyDesc);
    } else if (value->IsMap(vm_)) {
        GetMapValue(value, outPropertyDesc);
    } else if (value->IsWeakMap(vm_)) {
        GetWeakMapValue(value, outPropertyDesc);
    } else if (value->IsRegExp(vm_)) {
        GetRegExpValue(value, outPropertyDesc);
    } else if (value->IsSharedSet(vm_)) {
        GetSendableSetValue(value, outPropertyDesc);
    } else if (value->IsSet(vm_)) {
        GetSetValue(value, outPropertyDesc);
    } else if (value->IsWeakSet(vm_)) {
        GetWeakSetValue(value, outPropertyDesc);
    } else if (value->IsDataView(vm_)) {
        GetDataViewValue(value, outPropertyDesc);
    } else if (value->IsHashMap(vm_)) {
        GetHashMapValue(value, outPropertyDesc);
    } else if (value->IsHashSet(vm_)) {
        GetHashSetValue(value, outPropertyDesc);
    } else if (value->IsLightWeightMap(vm_)) {
        GetLightWeightMapValue(value, outPropertyDesc);
    } else if (value->IsLightWeightSet(vm_)) {
        GetLightWeightSetValue(value, outPropertyDesc);
    } else if (value->IsLinkedList(vm_)) {
        GetLinkedListValue(value, outPropertyDesc);
    } else if (value->IsList(vm_)) {
        GetListValue(value, outPropertyDesc);
    } else if (value->IsPlainArray(vm_)) {
        GetPlainArrayValue(value, outPropertyDesc);
    } else if (value->IsTreeMap(vm_)) {
        GetTreeMapValue(value, outPropertyDesc);
    } else if (value->IsTreeSet(vm_)) {
        GetTreeSetValue(value, outPropertyDesc);
    } else if (value->IsArrayList(vm_)) {
        GetArrayListValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsDeque(vm_)) {
        GetDequeValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsQueue(vm_)) {
        GetQueueValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsStack(vm_)) {
        GetStackValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsVector(vm_)) {
        GetVectorValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    } else if (value->IsPromise(vm_)) {
        GetPromiseValue(value, outPropertyDesc);
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
        return DispatchResponse::Ok();
    }
    Local<ArrayRef> keys = Local<ObjectRef>(value)->GetOwnPropertyNames(vm_);
    int32_t length = static_cast<int32_t>(keys->Length(vm_));
    Local<JSValueRef> name = JSValueRef::Undefined(vm_);
    for (int32_t i = 0; i < length; ++i) {
        name = keys->Get(vm_, i);
        PropertyAttribute jsProperty = PropertyAttribute::Default();
        if (!Local<ObjectRef>(value)->GetOwnProperty(vm_, name, jsProperty)) {
            continue;
        }
        std::unique_ptr<PropertyDescriptor> debuggerProperty =
            PropertyDescriptor::FromProperty(vm_, name, jsProperty);
        if (isAccessorOnly && !jsProperty.HasGetter() && !jsProperty.HasSetter()) {
            continue;
        }
        if (debuggerProperty->HasGet()) {
            debuggerProperty->GetGet()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsProperty.GetGetter(vm_));
        }
        if (debuggerProperty->HasSet()) {
            debuggerProperty->GetSet()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsProperty.GetSetter(vm_));
        }
        if (debuggerProperty->HasValue()) {
            Local<JSValueRef> vValue = jsProperty.GetValue(vm_);
            if (vValue->IsObject(vm_)) {
                debuggerProperty->GetValue()->SetObjectId(curObjectId_);
                properties_[curObjectId_++] = Global<JSValueRef>(vm_, vValue);
            }
        }
        if (debuggerProperty->HasSymbol()) {
            debuggerProperty->GetSymbol()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, name);
        }
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    if (!skipProto) {
        GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
    }

    return DispatchResponse::Ok();
}

void RuntimeImpl::AddTypedArrayRefs(Local<ArrayBufferRef> arrayBufferRef,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    int32_t arrayBufferByteLength = arrayBufferRef->ByteLength(vm_);
    int32_t typedArrayLength = arrayBufferByteLength;
    AddTypedArrayRef<Int8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ClampedArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8ClampedArray]]", outPropertyDesc);

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_16BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_16BITS;
        AddTypedArrayRef<Int16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int16Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint16Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_32BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_32BITS;
        AddTypedArrayRef<Int32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int32Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint32Array]]", outPropertyDesc);
        AddTypedArrayRef<Float32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float32Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_64BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_64BITS;
        AddTypedArrayRef<Float64ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigInt64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigInt64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigUint64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigUint64Array]]", outPropertyDesc);
    }
}

void RuntimeImpl::AddSharedArrayBufferRefs(Local<ArrayBufferRef> arrayBufferRef,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    int32_t arrayBufferByteLength = arrayBufferRef->ByteLength(vm_);
    int32_t typedArrayLength = arrayBufferByteLength;
    AddTypedArrayRef<Int8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8Array]]", outPropertyDesc);

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_16BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_16BITS;
        AddTypedArrayRef<Int16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int16Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_32BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_32BITS;
        AddTypedArrayRef<Int32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int32Array]]", outPropertyDesc);
    }
    Local<JSValueRef> jsValueRef;
    jsValueRef = NumberRef::New(vm_, arrayBufferByteLength);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[ArrayBufferByteLength]]");
    SetKeyValue(jsValueRef, outPropertyDesc, "byteLength");
}

template <typename TypedArrayRef>
void RuntimeImpl::AddTypedArrayRef(Local<ArrayBufferRef> arrayBufferRef, int32_t length, const char* name,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRefTypedArray(TypedArrayRef::New(vm_, arrayBufferRef, 0, length));
    std::unique_ptr<RemoteObject> remoteObjectTypedArray = RemoteObject::FromTagged(vm_, jsValueRefTypedArray);
    remoteObjectTypedArray->SetObjectId(curObjectId_);
    properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsValueRefTypedArray);
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName(name)
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(remoteObjectTypedArray));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void RuntimeImpl::CacheObjectIfNeeded(Local<JSValueRef> valRef, RemoteObject *remoteObj)
{
    if (valRef->IsObject(vm_)) {
        remoteObj->SetObjectId(curObjectId_);
        properties_[curObjectId_++] = Global<JSValueRef>(vm_, valRef);
    }
}

void RuntimeImpl::GetProtoOrProtoType(Local<JSValueRef> value, bool isOwn, bool isAccessorOnly,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    if (!isAccessorOnly && isOwn) {
        return;
    }
    // Get Function ProtoOrHClass
    if (value->IsConstructor(vm_)) {
        Local<JSValueRef> prototype = Local<FunctionRef>(value)->GetFunctionPrototype(vm_);
        std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(vm_, prototype);
        CacheObjectIfNeeded(prototype, protoObj.get());
        std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
        debuggerProperty->SetName("prototype")
            .SetWritable(false)
            .SetConfigurable(false)
            .SetEnumerable(false)
            .SetIsOwn(true)
            .SetValue(std::move(protoObj));
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    // Get __proto__
    Local<JSValueRef> proto = Local<ObjectRef>(value)->GetPrototype(vm_);
    std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(vm_, proto);
    CacheObjectIfNeeded(proto, protoObj.get());
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName("__proto__")
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(protoObj));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void RuntimeImpl::GetAdditionalProperties(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    // The length of the TypedArray have to be limited(less than or equal to lengthTypedArrayLimit) until we construct
    // the PropertyPreview class. Let lengthTypedArrayLimit be 10000 temporarily.
    static const uint32_t lengthTypedArrayLimit = 10000;

    // The width of the string-expression for JSTypedArray::MAX_TYPED_ARRAY_INDEX which is euqal to
    // JSObject::MAX_ELEMENT_INDEX which is equal to std::numeric_limits<uint32_t>::max(). (42,9496,7295)
    static const int32_t widthStrExprMaxElementIndex = 10;

    if (value->IsTypedArray(vm_)) {
        Local<TypedArrayRef> localTypedArrayRef(value);
        uint32_t lengthTypedArray = localTypedArrayRef->ArrayLength(vm_);
        if (lengthTypedArray > lengthTypedArrayLimit) {
            LOG_DEBUGGER(ERROR) << "The length of the TypedArray is non-compliant or unsupported.";
            return;
        }
        for (uint32_t i = 0; i < lengthTypedArray; ++i) {
            Local<JSValueRef> localValRefElement = localTypedArrayRef->Get(vm_, i);
            std::unique_ptr<RemoteObject> remoteObjElement = RemoteObject::FromTagged(vm_, localValRefElement);
            remoteObjElement->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, localValRefElement);
            std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();

            std::ostringstream osNameElement;
            osNameElement << std::right << std::setw(widthStrExprMaxElementIndex) << i;
            std::string cStrNameElement = osNameElement.str();
            debuggerProperty->SetName(cStrNameElement)
                .SetWritable(true)
                .SetConfigurable(true)
                .SetEnumerable(false)
                .SetIsOwn(true)
                .SetValue(std::move(remoteObjElement));
            outPropertyDesc->emplace_back(std::move(debuggerProperty));
        }
    }
}

void RuntimeImpl::SetKeyValue(Local<JSValueRef> &jsValueRef,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc, const std::string &strProName)
{
    std::unique_ptr<RemoteObject> remoteObj = RemoteObject::FromTagged(vm_, jsValueRef);
    remoteObj->SetObjectId(curObjectId_);
    properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsValueRef);
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName(strProName)
        .SetWritable(false)
        .SetConfigurable(false)
        .SetEnumerable(false)
        .SetIsOwn(false)
        .SetValue(std::move(remoteObj));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void RuntimeImpl::GetPrimitiveNumberValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    jsValueRef = value->ToNumber(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[PrimitiveValue]]");
}

void RuntimeImpl::GetPrimitiveStringValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    jsValueRef = value->ToString(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[PrimitiveValue]]");
}

void RuntimeImpl::GetPrimitiveBooleanValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    Local<PrimitiveRef> primitiveRef(value);
    jsValueRef = primitiveRef->GetValue(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[PrimitiveValue]]");
}

void RuntimeImpl::GetMapIteratorValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    Local<MapIteratorRef> iterRef = value->ToObject(vm_);
    if (!iterRef.IsEmpty()) {
        jsValueRef = NumberRef::New(vm_, iterRef->GetIndex());
        SetKeyValue(jsValueRef, outPropertyDesc, "[[IteratorIndex]]");
        jsValueRef = iterRef->GetKind(vm_);
        SetKeyValue(jsValueRef, outPropertyDesc, "[[IteratorKind]]");
    }
}

void RuntimeImpl::GetSetIteratorValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    Local<SetIteratorRef> iterRef = value->ToObject(vm_);
    if (!iterRef.IsEmpty()) {
        jsValueRef = NumberRef::New(vm_, iterRef->GetIndex());
        SetKeyValue(jsValueRef, outPropertyDesc, "[[IteratorIndex]]");
        jsValueRef = iterRef->GetKind(vm_);
        SetKeyValue(jsValueRef, outPropertyDesc, "[[IteratorKind]]");
    }
}

void RuntimeImpl::GetGeneratorFunctionValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    Local<GeneratorFunctionRef> genFuncRef = value->ToObject(vm_);
    if (!genFuncRef.IsEmpty()) {
        jsValueRef = BooleanRef::New(vm_, genFuncRef->IsGenerator(vm_));
        SetKeyValue(jsValueRef, outPropertyDesc, "[[IsGenerator]]");
    }
}

void RuntimeImpl::GetGeneratorObjectValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef;
    Local<GeneratorObjectRef> genObjRef = value->ToObject(vm_);
    if (!genObjRef.IsEmpty()) {
        jsValueRef = genObjRef->GetGeneratorState(vm_);
        SetKeyValue(jsValueRef, outPropertyDesc, "[[GeneratorState]]");
        jsValueRef = genObjRef->GetGeneratorFunction(vm_);
        SetKeyValue(jsValueRef, outPropertyDesc, "[[GeneratorFunction]]");
        jsValueRef = JSNApi::GetGlobalObject(vm_);
        SetKeyValue(jsValueRef, outPropertyDesc, "[[GeneratorReceiver]]");
    }
}

void RuntimeImpl::GetNumberFormatValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<NumberFormatRef> numberFormatRef = value->ToObject(vm_);
    Local<JSValueRef> jsValueRef = numberFormatRef->GetFormatFunction(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "format");
}

void RuntimeImpl::GetCollatorValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<CollatorRef> collatorRef = value->ToObject(vm_);
    Local<JSValueRef> jsValueRef = collatorRef->GetCompareFunction(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "compare");
}

void RuntimeImpl::GetDateTimeFormatValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<DataTimeFormatRef> dtFormatRef = value->ToObject(vm_);
    Local<JSValueRef> jsValueRef = dtFormatRef->GetFormatFunction(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "format");
}

void RuntimeImpl::GetSharedMapValue(Local<JSValueRef> value,
                                    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<SendableMapRef> sendableMapRef(value);
    uint32_t size = sendableMapRef->GetSize(vm_);
    uint32_t len = sendableMapRef->GetTotalElements(vm_);
    uint32_t index = 0;
    Local<JSValueRef> jsValueRef = NumberRef::New(vm_, size);
    SetKeyValue(jsValueRef, outPropertyDesc, "size");
    jsValueRef = ArrayRef::New(vm_, size);
    for (uint32_t i = 0; i < len; ++i) {
        Local<JSValueRef> jsKey = sendableMapRef->GetKey(vm_, i);
        if (jsKey->IsHole()) {
            continue;
        }
        Local<JSValueRef> jsValue = sendableMapRef->GetValue(vm_, i);
        Local<ObjectRef> objRef = ObjectRef::New(vm_);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "key"), jsKey);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), jsValue);
        DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
        ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}
void RuntimeImpl::GetMapValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<MapRef> mapRef = value->ToObject(vm_);
    int32_t size = mapRef->GetSize(vm_);
    int32_t len = mapRef->GetTotalElements(vm_);
    int32_t index = 0;
    Local<JSValueRef> jsValueRef = NumberRef::New(vm_, size);
    SetKeyValue(jsValueRef, outPropertyDesc, "size");
    jsValueRef = ArrayRef::New(vm_, size);
    for (int32_t i = 0; i < len; ++i) {
        Local<JSValueRef> jsKey = mapRef->GetKey(vm_, i);
        if (jsKey->IsHole()) {
            continue;
        }
        Local<JSValueRef> jsValue = mapRef->GetValue(vm_, i);
        Local<ObjectRef> objRef = ObjectRef::New(vm_);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "key"), jsKey);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), jsValue);
        DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
        ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}

void RuntimeImpl::GetWeakMapValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<WeakMapRef> weakMapRef = value->ToObject(vm_);
    int32_t size = weakMapRef->GetSize(vm_);
    int32_t len = weakMapRef->GetTotalElements(vm_);
    int32_t index = 0;
    Local<JSValueRef> jsValueRef = ArrayRef::New(vm_, size);
    for (int32_t i = 0; i < len; i++) {
        Local<JSValueRef> jsKey = weakMapRef->GetKey(vm_, i);
        if (jsKey->IsHole() || !jsKey->IsObject(vm_)) {
            continue;
        }
        Local<JSValueRef> jsValue = weakMapRef->GetValue(vm_, i);
        Local<ObjectRef> objRef = ObjectRef::New(vm_);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "key"), jsKey);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), jsValue);
        DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
        ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}

void RuntimeImpl::GetSendableSetValue(Local<JSValueRef> value,
                                      std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<SendableSetRef> setRef = value->ToObject(vm_);
    uint32_t size = setRef->GetSize(vm_);
    uint32_t len = setRef->GetTotalElements(vm_);
    int32_t index = 0;
    Local<JSValueRef> jsValueRef = NumberRef::New(vm_, size);
    SetKeyValue(jsValueRef, outPropertyDesc, "size");
    jsValueRef = ArrayRef::New(vm_, size);
    for (uint32_t i = 0; i < len; ++i) {
        Local<JSValueRef> elementRef = setRef->GetValue(vm_, i);
        if (elementRef->IsHole()) {
            continue;
        } else if (elementRef->IsObject(vm_)) {
            Local<ObjectRef> objRef = ObjectRef::New(vm_);
            objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), elementRef);
            DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
            ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
        } else {
            ArrayRef::SetValueAt(vm_, jsValueRef, index++, elementRef);
        }
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}

void RuntimeImpl::GetSetValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<SetRef> setRef = value->ToObject(vm_);
    int32_t size = setRef->GetSize(vm_);
    int32_t len = setRef->GetTotalElements(vm_);
    int32_t index = 0;
    Local<JSValueRef> jsValueRef = NumberRef::New(vm_, size);
    SetKeyValue(jsValueRef, outPropertyDesc, "size");
    jsValueRef = ArrayRef::New(vm_, size);
    for (int32_t i = 0; i < len; ++i) {
        Local<JSValueRef> elementRef = setRef->GetValue(vm_, i);
        if (elementRef->IsHole()) {
            continue;
        } else if (elementRef->IsObject(vm_)) {
            Local<ObjectRef> objRef = ObjectRef::New(vm_);
            objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), elementRef);
            DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
            ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
        } else {
            ArrayRef::SetValueAt(vm_, jsValueRef, index++, elementRef);
        }
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}

void RuntimeImpl::GetWeakSetValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<WeakSetRef> weakSetRef = value->ToObject(vm_);
    int32_t size = weakSetRef->GetSize(vm_);
    int32_t len = weakSetRef->GetTotalElements(vm_);
    int32_t index = 0;
    Local<JSValueRef> jsValueRef = ArrayRef::New(vm_, size);
    for (int32_t i = 0; i < len; ++i) {
        Local<JSValueRef> elementRef = weakSetRef->GetValue(vm_, i);
        if (elementRef->IsHole()) {
            continue;
        }
        Local<ObjectRef> objRef = ObjectRef::New(vm_);
        objRef->Set(vm_, StringRef::NewFromUtf8(vm_, "value"), elementRef);
        DebuggerApi::AddInternalProperties(vm_, objRef, ArkInternalValueType::Entry, internalObjects_);
        ArrayRef::SetValueAt(vm_, jsValueRef, index++, objRef);
    }
    DebuggerApi::AddInternalProperties(vm_, jsValueRef, ArkInternalValueType::Entry, internalObjects_);
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Entries]]");
}

void RuntimeImpl::GetDataViewValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<DataViewRef> dataViewRef = value->ToObject(vm_);
    int32_t byteLength = static_cast<int32_t>(dataViewRef->ByteLength());
    int32_t byteOffset = static_cast<int32_t>(dataViewRef->ByteOffset());
    Local<JSValueRef> jsValueRef = dataViewRef->GetArrayBuffer(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "buffer");
    jsValueRef = NumberRef::New(vm_, byteLength);
    SetKeyValue(jsValueRef, outPropertyDesc, "byteLength");
    jsValueRef = NumberRef::New(vm_, byteOffset);
    SetKeyValue(jsValueRef, outPropertyDesc, "byteOffset");
}

void RuntimeImpl::GetRegExpValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<RegExpRef> regExpRef = value->ToObject(vm_);
    Local<JSValueRef> jsValueRef = regExpRef->IsGlobal(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "global");
    jsValueRef = regExpRef->IsIgnoreCase(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "ignoreCase");
    jsValueRef = regExpRef->IsMultiline(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "multiline");
    jsValueRef = regExpRef->IsDotAll(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "dotAll");
    SetKeyValue(jsValueRef, outPropertyDesc, "hasIndices");
    jsValueRef = regExpRef->IsUtf16(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "unicode");
    jsValueRef = regExpRef->IsStick(vm_);
    SetKeyValue(jsValueRef, outPropertyDesc, "sticky");
    std::string strFlags = regExpRef->GetOriginalFlags(vm_);
    jsValueRef = StringRef::NewFromUtf8(vm_, strFlags.c_str());
    SetKeyValue(jsValueRef, outPropertyDesc, "flags");
    std::string strSource = regExpRef->GetOriginalSource(vm_)->ToString(vm_);
    jsValueRef = StringRef::NewFromUtf8(vm_, strSource.c_str());
    SetKeyValue(jsValueRef, outPropertyDesc, "source");
}

void RuntimeImpl::GetArrayListValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetArrayListValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[ArrayList]]");
}

void RuntimeImpl::GetDequeValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetDequeValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Deque]]");
}

void RuntimeImpl::GetHashMapValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetHashMapValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[HashMap]]");
}

void RuntimeImpl::GetHashSetValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetHashSetValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[HashSet]]");
}

void RuntimeImpl::GetLightWeightMapValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetLightWeightMapValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[LightWeightMap]]");
}

void RuntimeImpl::GetLightWeightSetValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetLightWeightSetValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[LightWeightSet]]");
}

void RuntimeImpl::GetLinkedListValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetLinkedListValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[LinkedList]]");
}

void RuntimeImpl::GetListValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetListValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[List]]");
}

void RuntimeImpl::GetPlainArrayValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetPlainArrayValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[PlainArray]]");
}

void RuntimeImpl::GetQueueValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetQueueValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Queue]]");
}

void RuntimeImpl::GetStackValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetStackValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Stack]]");
}

void RuntimeImpl::GetTreeMapValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetTreeMapValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[TreeMap]]");
}

void RuntimeImpl::GetTreeSetValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetTreeSetValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[TreeSet]]");
}

void RuntimeImpl::GetVectorValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRef = DebuggerApi::GetVectorValue(vm_, value, internalObjects_);
    Local<JSValueRef> size = NumberRef::New(vm_, DebuggerApi::GetContainerLength(vm_, jsValueRef));
    SetKeyValue(size, outPropertyDesc, "size");
    SetKeyValue(jsValueRef, outPropertyDesc, "[[Vector]]");
}

void RuntimeImpl::GetProxyValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<ProxyRef> proxyRef = value->ToObject(vm_);
    if (proxyRef.IsEmpty()) {
        return;
    }
    Local<JSValueRef> target = proxyRef->GetTarget(vm_);
    SetKeyValue(target, outPropertyDesc, "[[Target]]");
    Local<JSValueRef> handler = proxyRef->GetHandler(vm_);
    SetKeyValue(handler, outPropertyDesc, "[[Handler]]");
    Local<JSValueRef> isRevoked = BooleanRef::New(vm_, proxyRef->IsRevoked());
    SetKeyValue(isRevoked, outPropertyDesc, "[[IsRevoked]]");
}

void RuntimeImpl::GetPromiseValue(Local<JSValueRef> value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<PromiseRef> promiseRef = value->ToObject(vm_);
    if (promiseRef.IsEmpty()) {
        return;
    }
    Local<JSValueRef> promiseState = promiseRef->GetPromiseState(vm_);
    SetKeyValue(promiseState, outPropertyDesc, "[[PromiseState]]");
    Local<JSValueRef> promiseResult = promiseRef->GetPromiseResult(vm_);
    SetKeyValue(promiseResult, outPropertyDesc, "[[PromiseResult]]");
}
}  // namespace panda::ecmascript::tooling

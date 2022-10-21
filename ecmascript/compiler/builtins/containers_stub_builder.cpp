/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"

namespace panda::ecmascript::kungfu {
// common IR for containers apis that use function call
void ContainersStubBuilder::ContainersCommonFuncCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::INT64(), Int64(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIVector(env);
    Label valueNotJSAPIVector(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIVector(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    Branch(IsContainer(*thisObj, type), &valueIsJSAPIVector, &valueNotJSAPIVector);
    Bind(&valueNotJSAPIVector);
    {
        Branch(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            Branch(IsJSAPIVector(tempObj), &objIsJSAPIVector, slowPath);
            Bind(&objIsJSAPIVector);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIVector);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIVector);
    {
        Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            callbackFnHandle = GetCallArg0();
            Branch(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1();
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label lenChange(env);
            Label hasException(env);
            Label notHasException(env);
            Label setValue(env);
            Branch(Int32LessThan(*k, *length), &next, &afterLoop);
            Bind(&next);
            {
                kValue = ContainerGetValue(*thisObj, *k, type);
                key = IntToTaggedInt(*k);
                GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                    JSCallMode::CALL_THIS_ARG3_WITH_RETURN, { *thisArg, *kValue, *key, *thisObj });
                Branch(HasPendingException(glue), &hasException, &notHasException);
                Bind(&hasException);
                {
                    result->WriteVariable(retValue);
                    Jump(exit);
                }
                Bind(&notHasException);
                GateRef tempLen = ContainerGetSize(*thisObj, type);
                Branch(Int32NotEqual(tempLen, *length), &lenChange, &setValue);
                Bind(&lenChange);
                length = tempLen;
                Jump(&setValue);
                Bind(&setValue);
                if (IsReplaceAllElements(type)) {
                    ContainerSet(glue, *thisObj, *k, retValue, type);
                }
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        k = Int32Add(*k, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&afterLoop);
    Jump(exit);
}
}  // namespace panda::ecmascript::kungfu
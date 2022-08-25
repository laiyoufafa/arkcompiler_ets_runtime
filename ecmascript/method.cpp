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

#include "ecmascript/method.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
std::string Method::ParseFunctionName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::ParseFunctionName(jsPandaFile, GetMethodId());
}

const char *Method::GetMethodName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetMethodName(jsPandaFile, GetMethodId());
}

panda_file::File::StringData Method::GetName() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetName(jsPandaFile, GetMethodId());
}

uint32_t Method::GetCodeSize() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    return MethodLiteral::GetCodeSize(jsPandaFile, GetMethodId());
}

const JSPandaFile *Method::GetJSPandaFile() const
{
    JSTaggedValue constpool = GetConstantPool();
    if (constpool.IsUndefined()) {
        return nullptr;
    }

    // JSPandaFile is located at the first index of constPool.
    JSTaggedValue fileValue = ConstantPool::Cast(constpool.GetTaggedObject())->GetObjectFromCache(0);
    if (fileValue.IsUndefined()) {
        return nullptr;
    }

    void *nativePointer = JSNativePointer::Cast(fileValue.GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<JSPandaFile *>(nativePointer);
}

const panda_file::File *Method::GetPandaFile() const
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile();
    if (jsPandaFile == nullptr) {
        return nullptr;
    }
    return jsPandaFile->GetPandaFile();
}
} // namespace panda::ecmascript
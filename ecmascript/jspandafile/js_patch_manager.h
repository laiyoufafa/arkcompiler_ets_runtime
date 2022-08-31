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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PATCH_MANAGER_H
#define ECMASCRIPT_JSPANDAFILE_JS_PATCH_MANAGER_H

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
class JSPatchManager {
public:
    JSPatchManager() = default;
    ~JSPatchManager()
    {
        reservedBaseInfo_.clear();
    }

    bool LoadPatch(JSThread *thread, const std::string &patchFileName, const std::string &baseFileName);
    bool LoadPatch(JSThread *thread,
                   const std::string &patchFileName, const void *patchBuffer, size_t patchSize,
                   const std::string &baseFileName, const void *baseBuffer, size_t baseSize);
    bool UnLoadPatch(JSThread *thread, const std::string &patchFileName);

private:
    bool ReplaceMethod(JSThread *thread,
                       const JSHandle<ConstantPool> &baseConstpool,
                       const JSHandle<ConstantPool> &patchConstpool,
                       const JSHandle<Program> &patchProgram);

    const JSPandaFile *baseFile_ {nullptr};
    const JSPandaFile *patchFile_ {nullptr};

    // key: base constpool index, value: base methodLiteral.
    CUnorderedMap<uint32_t, MethodLiteral *> reservedBaseInfo_ {}; // for unload patch.
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_JS_PATCH_MANAGER_H
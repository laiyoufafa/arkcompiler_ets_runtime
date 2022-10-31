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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/debugger/hot_reload_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript::tooling {
// dummy class for test only
class ProtocolHandler {};
}

namespace panda::test {
using namespace panda::ecmascript;

using ProtocolHandler = tooling::ProtocolHandler;

class HotReloadManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
        ecmaVm->GetJsDebuggerManager()->SetDebuggerHandler(new ProtocolHandler());
    }

    void TearDown() override
    {
        ecmaVm->GetJsDebuggerManager()->SetDebuggerHandler(nullptr);
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(HotReloadManagerTest, LoadAndUnload)
{
    std::string baseFileName = DEBUGGER_ABC_DIR "base.abc";
    std::string patchFileName = DEBUGGER_ABC_DIR "patch.abc";
    std::string sourceFile = DEBUGGER_JS_DIR "patch.js";

    const auto *hotReloadManager = ecmaVm->GetJsDebuggerManager()->GetHotReloadManager();
    JSNApi::EnableUserUncaughtErrorHandler(ecmaVm);

    bool result = JSNApi::Execute(ecmaVm, baseFileName, JSPandaFile::ENTRY_FUNCTION_NAME);
    EXPECT_TRUE(result);

    auto baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName.c_str());
    EXPECT_TRUE(baseFile != nullptr);
    EXPECT_TRUE(hotReloadManager->GetBaseJSPandaFile(baseFile) == baseFile);

    result = JSNApi::LoadPatch(ecmaVm, patchFileName, baseFileName);
    EXPECT_TRUE(result);
    auto patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName.c_str());
    EXPECT_TRUE(patchFile != nullptr);
    EXPECT_TRUE(hotReloadManager->GetBaseJSPandaFile(patchFile) == baseFile);

    [[maybe_unused]] auto *patchExtractor = hotReloadManager->GetPatchExtractor(sourceFile);
    EXPECT_TRUE(patchExtractor != nullptr);

    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(ecmaVm);
    result = JSNApi::IsQuickFixCausedException(ecmaVm, exception, patchFileName);
    EXPECT_FALSE(result);

    result = JSNApi::UnloadPatch(ecmaVm, patchFileName);
    EXPECT_TRUE(result);
    EXPECT_TRUE(hotReloadManager->GetBaseJSPandaFile(patchFile) != baseFile);
    EXPECT_TRUE(hotReloadManager->GetPatchExtractor(sourceFile) == nullptr);
}
}  // namespace panda::test

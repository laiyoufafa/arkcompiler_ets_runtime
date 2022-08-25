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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class JSPandaFileTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
protected:
    JSPandaFile *CreateJSPandaFile(const char *source, const CString filename)
    {
        Parser parser;
        const std::string fn = "SRC.pa"; // test file name : "SRC.pa"
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
        return pf;
    }
};

HWTEST_F_L0(JSPandaFileTest, CreateJSPandaFile)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    EXPECT_TRUE(pf != nullptr);
}

HWTEST_F_L0(JSPandaFileTest, GetJSPandaFileDesc)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const CString expectFileName = pf->GetJSPandaFileDesc();
    EXPECT_STREQ(expectFileName.c_str(), "test.pa");
}

HWTEST_F_L0(JSPandaFileTest, GetPandaFile)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    EXPECT_TRUE(file != nullptr);
}

HWTEST_F_L0(JSPandaFileTest, GetMethodLiterals_GetNumMethods)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    MethodLiteral *method = pf->GetMethodLiterals();
    EXPECT_TRUE(method != nullptr);

    uint32_t methodNum = pf->GetNumMethods();
    EXPECT_EQ(methodNum, 3U); // 3 : number of methods
}

HWTEST_F_L0(JSPandaFileTest, SetMethodToMap_FindMethods)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId class_id = file->GetClassId(typeDesc);
    EXPECT_TRUE(class_id.IsValid());

    ClassDataAccessor cda(*file, class_id);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 3); // 3 : number of methods

    MethodLiteral method1(pf, methodId[0]);
    MethodLiteral method2(pf, methodId[1]);
    MethodLiteral method3(pf, methodId[2]);
    pf->SetMethodToMap(&method1);
    pf->SetMethodToMap(&method2);
    pf->SetMethodToMap(&method3);
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf, methodId[0]).c_str(), "foo1");
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf, methodId[1]).c_str(), "foo2");
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf, methodId[2]).c_str(), "foo3");
}

HWTEST_F_L0(JSPandaFileTest, GetOrInsertConstantPool_GetConstpoolIndex_GetConstpoolMap)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId class_id = file->GetClassId(typeDesc);
    EXPECT_TRUE(class_id.IsValid());

    ClassDataAccessor cda(*file, class_id);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 3); // 3 : number of methods

    uint32_t index1 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset());
    uint32_t index2 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[1].GetOffset());
    uint32_t index3 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[2].GetOffset());
    EXPECT_EQ(index1, 1U);
    EXPECT_EQ(index2, 2U);
    EXPECT_EQ(index3, 3U);

    uint32_t conPoolIndex = pf->GetConstpoolIndex();
    EXPECT_EQ(conPoolIndex, 4U); // 4 : 3 + 1, Index of the next insertion position

    CUnorderedMap<uint32_t, uint64_t> constpoolMap = pf->GetConstpoolMap();
    ConstPoolValue constPoolValue1(constpoolMap.at(methodId[0].GetOffset()));
    ConstPoolValue constPoolValue2(constpoolMap.at(methodId[1].GetOffset()));
    ConstPoolValue constPoolValue3(constpoolMap.at(methodId[2].GetOffset()));
    ConstPoolType type1 = constPoolValue1.GetConstpoolType();
    ConstPoolType type2 = constPoolValue2.GetConstpoolType();
    ConstPoolType type3 = constPoolValue3.GetConstpoolType();
    uint32_t gotIndex1 = constPoolValue1.GetConstpoolIndex();
    uint32_t gotIndex2 = constPoolValue2.GetConstpoolIndex();
    uint32_t gotIndex3 = constPoolValue3.GetConstpoolIndex();
    EXPECT_EQ(type1, ConstPoolType::METHOD);
    EXPECT_EQ(type2, ConstPoolType::METHOD);
    EXPECT_EQ(type3, ConstPoolType::METHOD);
    EXPECT_EQ(gotIndex1, 1U);
    EXPECT_EQ(gotIndex2, 2U);
    EXPECT_EQ(gotIndex3, 3U);
}

HWTEST_F_L0(JSPandaFileTest, GetMainMethodIndex_UpdateMainMethodIndex)
{
    const char *source = R"(
        .function void func1() {}
        .function void func2() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId class_id = file->GetClassId(typeDesc);
    EXPECT_TRUE(class_id.IsValid());

    ClassDataAccessor cda(*file, class_id);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 2); // 2 : number of methods

    uint32_t mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, 0U);

    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, methodId[0].GetOffset());

    pf->UpdateMainMethodIndex(methodId[1].GetOffset());
    mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, methodId[1].GetOffset());
}

HWTEST_F_L0(JSPandaFileTest, GetClasses)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId class_id = file->GetClassId(typeDesc);
    EXPECT_TRUE(class_id.IsValid());
 
    const File::Header *header = file->GetHeader();
    Span fileData(file->GetBase(), header->file_size);
    Span class_idx_data = fileData.SubSpan(header->class_idx_off, header->num_classes * sizeof(uint32_t));
    auto classesData = Span(reinterpret_cast<const uint32_t *>(class_idx_data.data()), header->num_classes);

    Span<const uint32_t> classes = pf->GetClasses();
    EXPECT_EQ(classes.Data(), classesData.Data());
}

HWTEST_F_L0(JSPandaFileTest, IsModule_IsCjs_HasTSTypes_GetTypeSummaryIndex)
{
    const char *source1 = R"(
        .function void foo1() {}
    )";
    const CString fileName1 = "test1.pa";
    JSPandaFile *pf1 = CreateJSPandaFile(source1, fileName1);
    EXPECT_EQ(pf1->IsModule(), false);
    EXPECT_EQ(pf1->IsCjs(), false);
    EXPECT_EQ(pf1->HasTSTypes(), false);
    EXPECT_EQ(pf1->GetTypeSummaryIndex(), 0U);
}

HWTEST_F_L0(JSPandaFileTest, SetLoadedAOTStatus_IsLoadedAOT)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    bool isLoadedAOT = pf->IsLoadedAOT();
    EXPECT_EQ(isLoadedAOT, false);

    pf->SetLoadedAOTStatus(true);
    isLoadedAOT = pf->IsLoadedAOT();
    EXPECT_EQ(isLoadedAOT, true);
}

HWTEST_F_L0(JSPandaFileTest, GetFileUniqId)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    JSPandaFile *pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetFileUniqId(), 2886575167U); // merge_hashes(FILENAME_HASH, GetHash32(GetHeader(), headerSize))
}
}  // namespace panda::test
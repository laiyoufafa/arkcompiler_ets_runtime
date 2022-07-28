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

#include "ecmascript/compiler/file_generators.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "llvm_ir_builder.h"

namespace panda::ecmascript::kungfu {
void StubFileGenerator::CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name,
    uint32_t bridgeModuleIdx)
{
    uint32_t funSize = 0;
    for (size_t i = 0; i < asmModule_.GetFunctionCount(); i++) {
        auto cs = asmModule_.GetCSign(i);
        auto entryOffset = asmModule_.GetFunction(cs->GetID());
        if (i < asmModule_.GetFunctionCount() - 1) {
            auto nextcs = asmModule_.GetCSign(i + 1);
            funSize = asmModule_.GetFunction(nextcs->GetID()) - entryOffset;
        } else {
            funSize = asmModule_.GetBufferSize() - entryOffset;
        }
        stubInfo_.AddStubEntry(cs->GetTargetKind(), cs->GetID(), entryOffset, bridgeModuleIdx, 0, funSize);
        ASSERT(!cs->GetName().empty());
        auto curSecBegin = asmModule_.GetBuffer();
        uintptr_t entry = reinterpret_cast<uintptr_t>(curSecBegin) + entryOffset;
        addr2name[entry] = cs->GetName();
    }
}

void StubFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, stubInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des);
        stubInfo_.AddModuleDes(des);
    }
    // idx for bridge module is the one after last module in modulePackage
    CollectAsmStubCodeInfo(addr2name, modulePackage_.size());
    DisassembleEachFunc(addr2name);
}

void AOTFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, aotInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des);
        aotInfo_.AddModuleDes(des, aotfileHashs_[i]);
    }
#ifndef NDEBUG
    DisassembleEachFunc(addr2name);
#endif
}

void StubFileGenerator::RunAsmAssembler()
{
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    asmModule_.Run(&cfg_, &chunk);

    auto buffer = asmModule_.GetBuffer();
    auto bufferSize = asmModule_.GetBufferSize();
    if (bufferSize == 0U) {
        return;
    }
    stubInfo_.FillAsmStubTempHolder(buffer, bufferSize);
    stubInfo_.accumulateTotalSize(bufferSize);
}

void StubFileGenerator::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
    RunAsmAssembler();
    CollectCodeInfo();
    stubInfo_.Save(filename);
}

void AOTFileGenerator::SaveAOTFile(const std::string &filename)
{
    RunLLVMAssembler();
    CollectCodeInfo();
    aotInfo_.Save(filename);
    DestoryModule();
}

void AOTFileGenerator::SaveSnapshotFile()
{
    TSManager *tsManager = vm_->GetTSManager();
    Snapshot snapshot(vm_);
    CVector<JSTaggedType> constStringTable = tsManager->GetConstStringTable();
    CVector<JSTaggedType> staticHClassTable = tsManager->GetStaticHClassTable();
    CVector<JSTaggedType> tsManagerSerializeTable(constStringTable);
    tsManagerSerializeTable.insert(tsManagerSerializeTable.end(), staticHClassTable.begin(), staticHClassTable.end());
    const CString snapshotPath(vm_->GetJSOptions().GetSnapshotOutputFile().c_str());
    snapshot.Serialize(reinterpret_cast<uintptr_t>(tsManagerSerializeTable.data()), tsManagerSerializeTable.size(),
                       snapshotPath);
}
}  // namespace panda::ecmascript::kungfu

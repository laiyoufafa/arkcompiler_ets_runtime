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

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"

#include <string>

#include "ecmascript/base/file_path_helper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {

PGOProfiler::~PGOProfiler()
{
    isEnable_ = false;
    profilerMap_.clear();
}

void PGOProfiler::Sample(JSTaggedType value)
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue jsValue(value);
    if (jsValue.IsJSFunction() && JSFunction::Cast(jsValue)->GetMethod().IsMethod()) {
        auto jsMethod = Method::Cast(JSFunction::Cast(jsValue)->GetMethod());
        JSTaggedValue recordNameValue = JSFunction::Cast(jsValue)->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto iter = profilerMap_.find(recordName);
        if (iter != profilerMap_.end()) {
            auto methodCountMap = iter->second;
            auto result = methodCountMap->find(jsMethod->GetMethodId());
            if (result != methodCountMap->end()) {
                auto info = result->second;
                info->IncreaseCount();
            } else {
                auto info = chunk_.New<MethodProfilerInfo>(1, std::string(jsMethod->GetMethodName()));
                methodCountMap->emplace(jsMethod->GetMethodId(), info);
                methodCount_++;
            }
        } else {
            ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *methodsCountMap =
                chunk_.New<ChunkUnorderedMap<EntityId, MethodProfilerInfo *>>(&chunk_);
            auto info = chunk_.New<MethodProfilerInfo>(1, std::string(jsMethod->GetMethodName()));
            methodsCountMap->emplace(jsMethod->GetMethodId(), info);
            profilerMap_.emplace(recordName, methodsCountMap);
            methodCount_++;
        }
        // Merged every 10 methods
        if (methodCount_ >= MERGED_EVERY_COUNT) {
            LOG_ECMA(INFO) << "Sample: post task to save profiler";
            PGOProfilerManager::GetInstance()->TerminateSaveTask();
            PGOProfilerManager::GetInstance()->Merge(this);
            PGOProfilerManager::GetInstance()->PostSaveTask();
            methodCount_ = 0;
        }
    }
}

void PGOProfilerManager::Initialize(uint32_t hotnessThreshold, const std::string &outDir)
{
    hotnessThreshold_ = hotnessThreshold;
    outDir_ = outDir;
}

void PGOProfilerManager::Destroy()
{
    if (!isEnable_) {
        return;
    }
    // SaveTask is already finished
    SaveProfiler();
    globalProfilerMap_->clear();
    chunk_.reset();
    nativeAreaAllocator_.reset();
    isEnable_ = false;
}

void PGOProfilerManager::InitializeData()
{
    os::memory::LockHolder lock(mutex_);
    if (!isEnable_) {
        isEnable_ = true;
        nativeAreaAllocator_ = std::make_unique<NativeAreaAllocator>();
        chunk_ = std::make_unique<Chunk>(nativeAreaAllocator_.get());
        globalProfilerMap_ = chunk_->
            New<ChunkUnorderedMap<CString, ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *>>(chunk_.get());
    }
}

void PGOProfilerManager::Merge(PGOProfiler *profiler)
{
    if (!isEnable_) {
        return;
    }

    os::memory::LockHolder lock(mutex_);
    for (auto iter = profiler->profilerMap_.begin(); iter != profiler->profilerMap_.end(); iter++) {
        auto recordName = iter->first;
        auto methodCountMap = iter->second;
        auto globalMethodCountIter = globalProfilerMap_->find(recordName);
        ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *globalMethodCountMap = nullptr;
        if (globalMethodCountIter == globalProfilerMap_->end()) {
            globalMethodCountMap = chunk_->New<ChunkUnorderedMap<EntityId, MethodProfilerInfo *>>(chunk_.get());
            globalProfilerMap_->emplace(recordName, globalMethodCountMap);
        } else {
            globalMethodCountMap = globalMethodCountIter->second;
        }
        for (auto countIter = methodCountMap->begin(); countIter != methodCountMap->end(); countIter++) {
            auto methodId = countIter->first;
            auto &localInfo = countIter->second;
            auto result = globalMethodCountMap->find(methodId);
            if (result != globalMethodCountMap->end()) {
                auto &info = result->second;
                info->AddCount(localInfo->GetCount());
            } else {
                auto info = chunk_->New<MethodProfilerInfo>(localInfo->GetCount(), localInfo->GetMethodName());
                globalMethodCountMap->emplace(methodId, info);
            }
            localInfo->ClearCount();
        }
    }
}

void PGOProfilerManager::SaveProfiler(SaveTask *task)
{
    std::string realOutPath;
    if (!base::FilePathHelper::RealPath(outDir_, realOutPath, false)) {
        LOG_ECMA(ERROR) << "The file path(" << outDir_ << ") real path failure!";
        outDir_ = "";
        isEnable_ = false;
        return;
    }
    static const std::string PROFILE_FILE_NAME = "/profiler.aprof";
    realOutPath += PROFILE_FILE_NAME;
    LOG_ECMA(INFO) << "Save profiler to file:" << realOutPath;

    std::ofstream fileStream(realOutPath.c_str());
    if (!fileStream.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath << ") open failure!";
        return;
    }
    ProcessProfile(fileStream, task);
    fileStream.close();
}

void PGOProfilerManager::ProcessProfile(std::ofstream &fileStream, SaveTask *task)
{
    std::string profilerString;
    for (auto iter = globalProfilerMap_->begin(); iter != globalProfilerMap_->end(); iter++) {
        auto methodCountMap = iter->second;
        bool isFirst = true;
        for (auto countIter = methodCountMap->begin(); countIter != methodCountMap->end(); countIter++) {
            LOG_ECMA(DEBUG) << "Method:" << countIter->first << "/" <<  countIter->second->GetMethodName()
                            << "(" << countIter->second->GetCount() << ")";
            if (task && task->IsTerminate()) {
                LOG_ECMA(INFO) << "ProcessProfile: task is already terminate";
                return;
            }
            if (countIter->second->GetCount() < hotnessThreshold_) {
                continue;
            }
            if (!isFirst) {
                profilerString += ",";
            } else {
                auto recordName = iter->first;
                profilerString += recordName;
                profilerString += ":[";
                isFirst = false;
            }
            profilerString += std::to_string(countIter->first.GetOffset());
            profilerString += "/";
            profilerString += std::to_string(countIter->second->GetCount());
            profilerString += "/";
            profilerString += countIter->second->GetMethodName();
        }
        if (!isFirst) {
            profilerString += "]\n";
            fileStream.write(profilerString.c_str(), profilerString.size());
            profilerString.clear();
        }
    }
}

void PGOProfilerManager::TerminateSaveTask()
{
    if (!isEnable_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->TerminateTask(GLOBAL_TASK_ID, TaskType::PGO_SAVE_TASK);
}

void PGOProfilerManager::PostSaveTask()
{
    if (!isEnable_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SaveTask>(GLOBAL_TASK_ID));
}

void PGOProfilerManager::StartSaveTask(SaveTask *task)
{
    if (task == nullptr) {
        return;
    }
    if (task->IsTerminate()) {
        LOG_ECMA(ERROR) << "StartSaveTask: task is already terminate";
        return;
    }
    os::memory::LockHolder lock(mutex_);
    SaveProfiler(task);
}
} // namespace panda::ecmascript

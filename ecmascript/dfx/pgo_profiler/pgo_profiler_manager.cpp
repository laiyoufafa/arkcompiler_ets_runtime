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

#include <sstream>

#include "ecmascript/base/file_path_helper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
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
            auto &methodCountMap = iter->second;
            auto result = methodCountMap.find(jsMethod->GetMethodId());
            if (result != methodCountMap.end()) {
                auto &info = result->second;
                info->IncreaseCount();
            } else {
                auto info = new MethodProfilerInfo(1, jsMethod->ParseFunctionName());
                methodCountMap.emplace(jsMethod->GetMethodId(), info);
            }
        } else {
            std::unordered_map<EntityId, MethodProfilerInfo *> methodsCountMap;
            auto info = new MethodProfilerInfo(1, jsMethod->ParseFunctionName());
            methodsCountMap.emplace(jsMethod->GetMethodId(), info);
            profilerMap_.emplace(recordName, methodsCountMap);
        }
    }
}

void PGOProfilerManager::Initialize(uint32_t hotnessThreshold, const std::string &outDir)
{
    isEnable_ = false;
    globalProfilerMap_.clear();
    hotnessThreshold_ = hotnessThreshold;
    if (outDir.empty()) {
        outDir_ = "";
        isEnable_ = false;
    }
    outDir_ = outDir;
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
        auto globalMethodCountIter = globalProfilerMap_.find(recordName);
        if (globalMethodCountIter == globalProfilerMap_.end()) {
            globalProfilerMap_.emplace(recordName, methodCountMap);
            continue;
        }
        auto &globalMethodCountMap = globalMethodCountIter->second;
        for (auto countIter = methodCountMap.begin(); countIter != methodCountMap.end(); countIter++) {
            auto methodId = countIter->first;
            auto result = globalMethodCountMap.find(methodId);
            if (result != globalMethodCountMap.end()) {
                auto &info = result->second;
                info->AddCount(countIter->second->GetCount());
                delete countIter->second;
            } else {
                globalMethodCountMap.emplace(methodId, countIter->second);
            }
        }
    }
    profiler->profilerMap_.clear();
}

void PGOProfilerManager::SaveProfiler()
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

    std::ofstream file(realOutPath.c_str());
    if (!file.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath << ") open failure!";
        return;
    }
    std::string profilerString = ProcessProfile();
    file.write(profilerString.c_str(), profilerString.size());
    file.close();
}

std::string PGOProfilerManager::ProcessProfile()
{
    std::stringstream profilerStream;
    for (auto iter = globalProfilerMap_.begin(); iter != globalProfilerMap_.end(); iter++) {
        auto methodCountMap = iter->second;
        bool isFirst = true;
        for (auto countIter = methodCountMap.begin(); countIter != methodCountMap.end(); countIter++) {
            LOG_ECMA(DEBUG) << "Method id:" << countIter->first << "(" << countIter->second->GetCount() << ")";
            if (countIter->second->GetCount() < hotnessThreshold_) {
                continue;
            }
            if (!isFirst) {
                profilerStream << ",";
            } else {
                auto recordName = iter->first;
                profilerStream << recordName;
                profilerStream << ":[";
                isFirst = false;
            }
            profilerStream << countIter->first.GetOffset();
            profilerStream << "/";
            profilerStream << countIter->second->GetCount();
            profilerStream << "/";
            profilerStream << countIter->second->GetMethodName();
            delete countIter->second;
        }
        if (!isFirst) {
            profilerStream << "]\n";
        }
    }
    globalProfilerMap_.clear();

    return profilerStream.str();
}
} // namespace panda::ecmascript

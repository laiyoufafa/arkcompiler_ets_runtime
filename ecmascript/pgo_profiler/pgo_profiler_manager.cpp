/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
namespace panda::ecmascript {
bool PGOProfilerManager::MergeApFiles(const std::string &inFiles, const std::string &outPath, uint32_t hotnessThreshold)
{
    arg_list_t pandaFileNames = base::StringHelper::SplitString(inFiles, GetFileDelimiter());
    PGOProfilerEncoder merger(outPath, hotnessThreshold);
    if (!merger.InitializeData()) {
        LOG_ECMA(ERROR) << "PGO Profiler encoder initialized failed. outPath: " << outPath
                        << " ,hotnessThreshold: " << hotnessThreshold;
        return false;
    }
    bool isFirstFile = true;
    std::string firstApFileName;
    for (const auto &fileName : pandaFileNames) {
        if (!base::StringHelper::EndsWith(fileName, ".ap")) {
            LOG_ECMA(ERROR) << "The file path(" << fileName << ") does not end with .ap";
            continue;
        }
        PGOProfilerDecoder decoder(fileName, hotnessThreshold);
        if (!decoder.LoadFull()) {
            LOG_ECMA(ERROR) << "Fail to load file path(" << fileName << "), skip it.";
            continue;
        }
        if (isFirstFile) {
            firstApFileName = fileName;
        } else {
            if (!merger.VerifyPandaFileMatched(decoder.GetPandaFileInfos(), firstApFileName, fileName)) {
                continue;
            }
        }
        merger.Merge(decoder.GetRecordDetailInfos());
        merger.Merge(decoder.GetPandaFileInfos());
        isFirstFile = false;
    }
    if (isFirstFile) {
        LOG_ECMA(ERROR) << "No input file processed. Input files: " << inFiles;
        return false;
    }
    merger.Save();
    return true;
}

bool PGOProfilerManager::MergeApFiles(uint32_t checksum, PGOProfilerDecoder &merger)
{
    uint32_t hotnessThreshold = merger.GetHotnessThreshold();
    std::string inFiles(merger.GetInPath());
    arg_list_t pandaFileNames = base::StringHelper::SplitString(inFiles, GetFileDelimiter());
    if (pandaFileNames.empty()) {
        return true;
    }
    merger.InitMergeData();
    bool isFirstFile = true;
    std::string firstApFileName;
    for (const auto &fileName : pandaFileNames) {
        PGOProfilerDecoder decoder(fileName, hotnessThreshold);
        if (!decoder.LoadAndVerify(checksum)) {
            LOG_ECMA(ERROR) << "Load and verify file(" << fileName << ") failed, skip it.";
            continue;
        }
        if (isFirstFile) {
            firstApFileName = fileName;
        } else {
            if (!merger.GetPandaFileInfos().VerifyChecksum(decoder.GetPandaFileInfos(), firstApFileName, fileName)) {
                continue;
            }
        }
        merger.Merge(decoder);
        isFirstFile = false;
    }
    if (isFirstFile) {
        LOG_ECMA(ERROR) << "No input file processed. Input files: " << inFiles;
        return false;
    }
    return true;
}
} // namespace panda::ecmascript
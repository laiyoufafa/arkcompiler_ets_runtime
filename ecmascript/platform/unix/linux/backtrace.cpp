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

#include "ecmascript/platform/backtrace.h"

#include <execinfo.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
static const int MAX_STACK_SIZE = 256;
static const int FRAMES_LEN = 16;

void Backtrace(std::ostringstream &stack, [[maybe_unused]] bool enableCache)
{
    void *buffer[MAX_STACK_SIZE];
    char **stackList = nullptr;
    int framesLen = backtrace(buffer, MAX_STACK_SIZE);
    stackList = backtrace_symbols(buffer, framesLen);
    if (stackList == nullptr) {
        return;
    }

    stack << "=====================Backtrace========================";
    for (int i = 0; i < FRAMES_LEN; ++i) {
        if (stackList[i] == nullptr) {
            break;
        }
        stack << stackList[i];
    }

    free(stackList);
}
} // namespace panda::ecmascript
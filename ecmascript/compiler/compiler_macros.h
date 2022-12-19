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

#ifndef ECMASCRIPT_COMPILER_MACROS_H
#define ECMASCRIPT_COMPILER_MACROS_H

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_macros.h"

#define ECMASCRIPT_ENABLE_COMPILER_LOG 0

#define COMPILER_LOG(level) ECMASCRIPT_ENABLE_COMPILER_LOG && LOG_ECMA(level)

#endif // ECMASCRIPT_COMPILER_MACROS_H
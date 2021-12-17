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

#ifndef ECMASCRIPT_INTERFACE_RUNTIME_API_H
#define ECMASCRIPT_INTERFACE_RUNTIME_API_H

#include "ecmascript/common.h"
#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/remembered_set.h"

namespace panda::ecmascript {
class WorkerHelper;

class PUBLIC_API RuntimeApi {
public:
    static bool AtomicTestAndSet(RangeBitmap *bitmap, TaggedObject *object);
    static void PushWorkList(WorkerHelper *worklist, uint32_t threadId, TaggedObject *object, Region *region);
    static void AtomicInsertCrossRegionRememberedSet(RememberedSet *rset, uintptr_t addr);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERFACE_RUNTIME_API_H

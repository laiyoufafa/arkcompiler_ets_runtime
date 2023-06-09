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

#ifndef ECMASCRIPT_JOBS_PENDING_JOB_H
#define ECMASCRIPT_JOBS_PENDING_JOB_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/record.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::job {
class PendingJob final : public Record {
public:
    static PendingJob *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsPendingJob());
        return static_cast<PendingJob *>(object);
    }

    static JSTaggedValue ExecutePendingJob(const JSHandle<PendingJob> &pendingJob, JSThread *thread)
    {
        JSHandle<JSTaggedValue> job(thread, pendingJob->GetJob());
        ASSERT(job->IsCallable());
        JSHandle<TaggedArray> argv(thread, pendingJob->GetArguments());
        const size_t argsLength = argv->GetLength();
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, job, undefined, undefined, argsLength);
        info.SetCallArg(argsLength, argv);
        return JSFunction::Call(&info);
    }

    static constexpr size_t JOB_OFFSET = Record::SIZE;
    ACCESSORS(Job, JOB_OFFSET, ARGUMENT_OFFSET);
    ACCESSORS(Arguments, ARGUMENT_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(JOB_OFFSET, SIZE)
};
}  // namespace panda::ecmascript::job
#endif  // ECMASCRIPT_JOBS_PENDING_JOB_H

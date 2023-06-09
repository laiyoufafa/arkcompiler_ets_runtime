/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ECMA_VM_H
#define ECMASCRIPT_ECMA_VM_H

#include <tuple>

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/taskpool/task.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "libpandabase/macros.h"

namespace panda {
class JSNApi;
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class RegExpParserCache;
class EcmaRuntimeStat;
class Heap;
class HeapTracker;
class JSNativePointer;
class Program;
class CpuProfiler;
class RegExpExecResultCache;
class JSPromise;
enum class PromiseRejectionEvent : uint8_t;
class JSPandaFileManager;
class JSPandaFile;
class SnapShotSerialize;
namespace job {
class MicroJobQueue;
}  // namespace job

namespace tooling {
class JsDebuggerManager;
}  // namespace tooling

template<typename T>
class JSHandle;
class JSArrayBuffer;
class JSFunction;
class Program;
class TSLoader;
class ModuleManager;
class AotCodeInfo;

using HostPromiseRejectionTracker = void (*)(const EcmaVM* vm,
                                             const JSHandle<JSPromise> promise,
                                             const JSHandle<JSTaggedValue> reason,
                                             PromiseRejectionEvent operation,
                                             void* data);
using PromiseRejectCallback = void (*)(void* info);

class EcmaVM {
public:
    static EcmaVM *Create(const JSRuntimeOptions &options);

    static bool Destroy(EcmaVM *vm);

    explicit EcmaVM(JSRuntimeOptions options);

    EcmaVM();

    ~EcmaVM();

    bool IsInitialized() const
    {
        return vmInitialized_;
    }

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    bool Initialize();

    bool InitializeFinish();

    GCStats *GetEcmaGCStats() const
    {
        return gcStats_;
    }

    JSThread *GetAssociatedJSThread() const
    {
        return thread_;
    }

    const JSRuntimeOptions &GetJSOptions() const
    {
        return options_;
    }

    JSHandle<GlobalEnv> GetGlobalEnv() const;

    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    bool ExecutePromisePendingJob() const;

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    JSMethod *GetMethodForNativeFunction(const void *func);

    EcmaStringTable *GetEcmaStringTable() const
    {
        ASSERT(stringTable_ != nullptr);
        return stringTable_;
    }

    JSThread *GetJSThread() const
    {
#if defined(ECMASCRIPT_ENABLE_THREAD_CHECK) && ECMASCRIPT_ENABLE_THREAD_CHECK
        // Exclude GC thread
        if (options_.IsEnableThreadCheck()) {
            if (!Taskpool::GetCurrentTaskpool()->IsInThreadPool(std::this_thread::get_id()) &&
                thread_->GetThreadId() != JSThread::GetCurrentThreadId()) {
                    LOG(FATAL, RUNTIME) << "Fatal: ecma_vm cannot run in multi-thread!"
                                        << " thread:" << thread_->GetThreadId()
                                        << " currentThread:" << JSThread::GetCurrentThreadId();
            }
        }
#endif
        return thread_;
    }

    JSThread *GetJSThreadNoCheck() const
    {
        return thread_;
    }

    bool ICEnable() const
    {
        return icEnable_;
    }

    void PushToNativePointerList(JSNativePointer *array);
    void RemoveFromNativePointerList(JSNativePointer *array);

    JSHandle<ecmascript::JSTaggedValue> GetAndClearEcmaUncaughtException() const;
    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    void EnableUserUncaughtErrorHandler();

    EcmaRuntimeStat *GetRuntimeStat() const
    {
        return runtimeStat_;
    }

    void SetRuntimeStatEnable(bool flag);

    bool IsRuntimeStatEnabled() const
    {
        return runtimeStatEnabled_;
    }

    bool IsOptionalLogEnabled() const
    {
        return optionalLogEnabled_;
    }

    void Iterate(const RootVisitor &v);

    const Heap *GetHeap() const
    {
        return heap_;
    }

    void CollectGarbage(TriggerGCType gcType) const;

    void StartHeapTracking(HeapTracker *tracker);

    void StopHeapTracking();

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_.get();
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_.get();
    }

    Chunk *GetChunk() const
    {
        return const_cast<Chunk *>(&chunk_);
    }
    void ProcessNativeDelete(const WeakRootVisitor &v0);
    void ProcessReferences(const WeakRootVisitor &v0);

    ModuleManager *GetModuleManager() const
    {
        return moduleManager_;
    }

    TSLoader *GetTSLoader() const
    {
        return tsLoader_;
    }

    SnapShotEnv *GetSnapShotEnv() const
    {
        return snapshotEnv_;
    }

    void LoadStubs();
    void SetupRegExpResultCache();

    JSHandle<JSTaggedValue> GetRegExpCache() const
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&regexpCache_));
    }

    void SetRegExpCache(JSTaggedValue newCache)
    {
        regexpCache_ = newCache;
    }

    tooling::JsDebuggerManager *GetJsDebuggerManager() const
    {
        return debuggerManager_;
    }

    void SetEnableForceGC(bool enable)
    {
        options_.SetEnableForceGC(enable);
    }

    void SetData(void* data)
    {
        data_ = data;
    }

    void SetPromiseRejectCallback(PromiseRejectCallback cb)
    {
        promiseRejectCallback_ = cb;
    }

    PromiseRejectCallback GetPromiseRejectCallback() const
    {
        return promiseRejectCallback_;
    }

    void SetHostPromiseRejectionTracker(HostPromiseRejectionTracker cb)
    {
        hostPromiseRejectionTracker_ = cb;
    }

    void PromiseRejectionTracker(const JSHandle<JSPromise> &promise,
                                 const JSHandle<JSTaggedValue> &reason, PromiseRejectionEvent operation)
    {
        if (hostPromiseRejectionTracker_ != nullptr) {
            hostPromiseRejectionTracker_(this, promise, reason, operation, data_);
        }
    }

    void SetConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool);

    JSTaggedValue FindConstpool(const JSPandaFile *jsPandaFile);

    void TryLoadSnapshotFile();

    AotCodeInfo *GetAotCodeInfo() const
    {
        return aotInfo_;
    }

protected:

    void HandleUncaughtException(ObjectHeader *exception);

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo);

private:
    class CpuProfilingScope {
    public:
        explicit CpuProfilingScope(EcmaVM* vm);
        ~CpuProfilingScope();

    private:
        [[maybe_unused]] EcmaVM* vm_;
        [[maybe_unused]] CpuProfiler* profiler_;
    };
    void SetGlobalEnv(GlobalEnv *global);

    void SetMicroJobQueue(job::MicroJobQueue *queue);

    Expected<JSTaggedValue, bool> InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile);

    JSTaggedValue InvokeEcmaAotEntrypoint();

    void InitializeEcmaScriptRunStat();

    bool VerifyFilePath(const CString &filePath) const;

    void ClearBufferData();

    void ClearNativeMethodsData();

    void LoadAOTFile(const std::string &fileName);

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // VM startup states.
    JSRuntimeOptions options_;
    bool icEnable_ {true};
    bool vmInitialized_ {false};
    GCStats *gcStats_ {nullptr};
    bool snapshotSerializeEnable_ {false};
    bool snapshotDeserializeEnable_ {false};
    bool isUncaughtExceptionRegistered_ {false};

    // VM memory management.
    EcmaStringTable *stringTable_ {nullptr};
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<HeapRegionAllocator> heapRegionAllocator_;
    Chunk chunk_;
    Heap *heap_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    ChunkVector<JSNativePointer *> nativePointerList_;

    // VM execution states.
    JSThread *thread_ {nullptr};
    RegExpParserCache *regExpParserCache_ {nullptr};
    JSTaggedValue globalEnv_ {JSTaggedValue::Hole()};
    JSTaggedValue regexpCache_ {JSTaggedValue::Hole()};
    JSTaggedValue microJobQueue_ {JSTaggedValue::Hole()};
    bool runtimeStatEnabled_ {false};
    EcmaRuntimeStat *runtimeStat_ {nullptr};

    // For framewrok file snapshot.
    CString snapshotFileName_;
    CString frameworkAbcFileName_;
    JSTaggedValue frameworkProgram_ {JSTaggedValue::Hole()};
    const JSPandaFile *frameworkPandaFile_ {nullptr};
    CMap<const JSPandaFile *, JSTaggedValue> cachedConstpools_ {};

    // VM resources.
    ChunkVector<JSMethod *> nativeMethods_;
    ModuleManager *moduleManager_ {nullptr};
    TSLoader *tsLoader_ {nullptr};
    SnapShotEnv *snapshotEnv_ {nullptr};
    bool optionalLogEnabled_ {false};
    AotCodeInfo *aotInfo_ {nullptr};

    // Debugger
    tooling::JsDebuggerManager *debuggerManager_ {nullptr};

    // Registered Callbacks
    PromiseRejectCallback promiseRejectCallback_ {nullptr};
    HostPromiseRejectionTracker hostPromiseRejectionTracker_ {nullptr};
    void* data_ {nullptr};

    friend class SnapShotSerialize;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
    friend class JSPandaFileExecutor;
};
}  // namespace ecmascript
}  // namespace panda

#endif

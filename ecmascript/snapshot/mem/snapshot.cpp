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

#include "ecmascript/snapshot/mem/snapshot.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
constexpr uint32_t PANDA_FILE_ALIGNMENT = 4096;

void SnapShot::Serialize(TaggedObject *objectHeader, const panda_file::File *pf, const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return;
    }
    std::fstream write(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!write.good()) {
        LOG(DEBUG, RUNTIME) << "snapshot open file failed";
        return;
    }

    SnapShotSerialize serialize(vm_);
    vm_->GetSnapShotEnv()->Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    if (objectHeader->GetClass()->GetObjectType() == JSType::PROGRAM) {
        serialize.SetProgramSerializeStart();
    }

    serialize.EncodeTaggedObject(objectHeader, &objectQueue, &data);

    size_t rootObjSize = objectQueue.size();

    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();

        serialize.SerializeObject(taggedObject, &objectQueue, &data);
    }
    vm_->GetHeap()->GetSnapShotSpace()->Stop();

    WriteToFile(write, pf, rootObjSize, serialize.GetStringVector());
    vm_->GetSnapShotEnv()->ClearEnvMap();
}

void SnapShot::Serialize(uintptr_t startAddr, size_t size, const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return;
    }
    std::fstream write(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!write.good()) {
        LOG(DEBUG, RUNTIME) << "snapshot open file failed";
        return;
    }

    SnapShotSerialize serialize(vm_);
    vm_->GetSnapShotEnv()->Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    ObjectSlot start(startAddr);
    ObjectSlot end(startAddr + size * sizeof(JSTaggedType));

    serialize.EncodeTaggedObjectRange(start, end, &objectQueue, &data);
    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();
        serialize.SerializeObject(taggedObject, &objectQueue, &data);
    }

    vm_->GetHeap()->GetSnapShotSpace()->Stop();

    WriteToFile(write, nullptr, size, serialize.GetStringVector());
    vm_->GetSnapShotEnv()->ClearEnvMap();
}

const JSPandaFile *SnapShot::Deserialize(SnapShotType type, const CString &snapshotFile)
{
    SnapShotSerialize serialize(vm_);
    std::pair<bool, CString> filePath = VerifyFilePath(snapshotFile);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return nullptr;
    }

    int fd = open(filePath.second.c_str(), O_CLOEXEC);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (UNLIKELY(fd == -1)) {
        LOG_ECMA(FATAL) << "open file failed";
        UNREACHABLE();
    }
    int32_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        LOG_ECMA(FATAL) << "lseek failed";
        UNREACHABLE();
    }
    auto readFile = ToUintPtr(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    auto hdr = *ToNativePtr<const Header>(readFile);
    size_t defaultSnapshotSpaceCapacity = vm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    if (defaultSnapshotSpaceCapacity == 0) {
        LOG_ECMA_MEM(FATAL) << "defaultSnapshotSpaceCapacity must have a size bigger than 0";
        UNREACHABLE();
    }
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    uintptr_t snapshotBegin = readFile + sizeof(Header);
    size_t regionSize = 0U;
    if (hdr.snapshotSize != 0) {
        regionSize = (hdr.snapshotSize - 1) / defaultSnapshotSpaceCapacity + 1; // round up
    }
    for (size_t i = 0; i < regionSize; i++) {
        Region *region = const_cast<HeapRegionAllocator *>(vm_->GetHeap()->GetHeapRegionAllocator())
                            ->AllocateAlignedRegion(space, defaultSnapshotSpaceCapacity);
        auto fileRegion = ToNativePtr<Region>(snapshotBegin + i * defaultSnapshotSpaceCapacity);

        uint64_t base = region->allocateBase_;
        GCBitset *markGCBitset = region->markGCBitset_;
        uint64_t begin = (fileRegion->begin_) % defaultSnapshotSpaceCapacity;
        uint64_t waterMark = (fileRegion->highWaterMark_) % defaultSnapshotSpaceCapacity;
        size_t copyBytes = fileRegion->highWaterMark_ - fileRegion->allocateBase_;

        if (memcpy_s(region, copyBytes, fileRegion, copyBytes) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }

        // allocate_base_
        region->allocateBase_ = base;
        // begin_
        region->begin_ = ToUintPtr(region) + begin;
        // end_
        region->end_ = ToUintPtr(region) + defaultSnapshotSpaceCapacity;
        // high_water_mark_
        region->highWaterMark_ = ToUintPtr(region) + waterMark;
        // prev_
        region->prev_ = nullptr;
        // next_
        region->next_ = nullptr;
        // mark_bitset_
        region->markGCBitset_ = markGCBitset;
        // cross_region_set_
        region->crossRegionSet_ = nullptr;
        // old_to_new_set_
        region->oldToNewSet_ = nullptr;
        // space_
        region->space_ = space;
        // heap_
        auto heap = const_cast<Heap *>(vm_->GetHeap());
        region->heap_ = heap;
        // nativePoniterAllocator_
        region->nativeAreaAllocator_ = heap->GetNativeAreaAllocator();

        space->AddRegion(region);
    }
    space->ResetAllocator();
    uintptr_t stringBegin = snapshotBegin + hdr.snapshotSize;
    uintptr_t stringEnd = stringBegin + hdr.stringSize;
    serialize.DeserializeString(stringBegin, stringEnd);
    vm_->GetHeap()->GetSnapShotSpace()->Stop();
    if (type == SnapShotType::TS_LOADER) {
        auto stringVector = serialize.GetStringVector();
        for (uint32_t i = 0; i < hdr.rootObjectSize; ++i) {
            JSTaggedValue result(reinterpret_cast<EcmaString *>(stringVector[i]));
            vm_->GetTSLoader()->AddConstString(result);
        }
    }
    munmap(ToNativePtr<void>(readFile), hdr.pandaFileBegin);
    const JSPandaFile *jsPandaFile = nullptr;
    if (static_cast<uint32_t>(file_size) > hdr.pandaFileBegin) {
        uintptr_t panda_file_mem = readFile + hdr.pandaFileBegin;
        auto pf = panda_file::File::OpenFromMemory(os::mem::ConstBytePtr(ToNativePtr<std::byte>(panda_file_mem),
            static_cast<uint32_t>(file_size) - hdr.pandaFileBegin, os::mem::MmapDeleter));
        jsPandaFile = JSPandaFileManager::GetInstance()->NewJSPandaFile(pf.release(), "");
    }
    close(fd);
    // relocate object field
    serialize.Relocate(type, jsPandaFile, hdr.rootObjectSize);
    return jsPandaFile;
}

size_t SnapShot::AlignUpPageSize(size_t spaceSize)
{
    if (spaceSize % Constants::PAGE_SIZE_ALIGN_UP == 0) {
        return spaceSize;
    }
    return Constants::PAGE_SIZE_ALIGN_UP * (spaceSize / Constants::PAGE_SIZE_ALIGN_UP + 1);
}

std::pair<bool, CString> SnapShot::VerifyFilePath(const CString &filePath)
{
    if (filePath.size() > PATH_MAX) {
        return std::make_pair(false, "");
    }
    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (result == resolvedPath.data() || errno == ENOENT) {
        return std::make_pair(true, CString(resolvedPath.data()));
    }
    return std::make_pair(false, "");
}

void SnapShot::WriteToFile(std::fstream &write, const panda_file::File *pf,
                           size_t size, const CVector<uintptr_t> &stringVector)
{
    uint32_t totalStringSize = 0U;
    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t objectSize = AlignUp(str->ObjectSize(), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        totalStringSize += objectSize;
    }
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    size_t defaultSnapshotSpaceCapacity = vm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    auto lastRegion = space->GetCurrentRegion();
    size_t regionCount = space->GetRegionCount();
    uint32_t snapshotSize;
    if (regionCount == 0) {
        snapshotSize = 0U;
    } else if (regionCount == 1) {
        snapshotSize = lastRegion->GetHighWaterMarkSize();
    } else {
        snapshotSize = (regionCount - 1) * defaultSnapshotSpaceCapacity + lastRegion->GetHighWaterMarkSize();
    }
    uint32_t pandaFileBegin = RoundUp(snapshotSize + totalStringSize + sizeof(Header), PANDA_FILE_ALIGNMENT);
    Header hdr {snapshotSize, totalStringSize, pandaFileBegin, size};
    write.write(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    if (regionCount > 0) {
        space->EnumerateRegions([&write, &defaultSnapshotSpaceCapacity, lastRegion](Region *current) {
            if (current != lastRegion) {
                write.write(reinterpret_cast<char *>(current), defaultSnapshotSpaceCapacity);
                write.flush();
            }
        });
        write.write(reinterpret_cast<char *>(lastRegion), lastRegion->GetHighWaterMarkSize());
        write.flush();
        space->ReclaimRegions();
    }

    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    auto stringClass = reinterpret_cast<JSHClass *>(globalConst->GetStringClass().GetTaggedObject());

    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t strSize = AlignUp(str->ObjectSize(), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        write.write(reinterpret_cast<char *>(str), strSize);
        write.flush();
        str->SetClass(stringClass);
    }
    ASSERT(static_cast<size_t>(write.tellp()) == snapshotSize + totalStringSize + sizeof(Header));
    if (pf) {
        write.seekp(pandaFileBegin);
        write.write(reinterpret_cast<const char *>(pf->GetBase()), pf->GetHeader()->file_size);
    }
    write.close();
}
}  // namespace panda::ecmascript

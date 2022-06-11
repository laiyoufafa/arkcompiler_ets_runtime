
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

#include "ecmascript/interpreter/frame_handler.h"

#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
void FrameHandler::PrevFrame()
{
    auto type = GetFrameType();
    switch (type) {
        case FrameType::OPTIMIZED_FRAME: {
            auto frame = OptimizedFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
            auto frame = OptimizedJSFunctionArgConfigFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = OptimizedJSFunctionFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME: {
            auto frame = OptimizedEntryFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_FRAME: {
            auto frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            if (thread_->IsAsmInterpreter()) {
                fp_ = frame->GetCurrentFramePointer();
            }
            break;
        }
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto frame = InterpretedFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME: {
            auto frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            auto frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME: {
            auto frame = BuiltinFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME: {
            auto frame = InterpretedEntryFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME: {
            auto frame = AsmInterpretedEntryFrame::GetFrameFromSp(sp_);
            sp_ = frame->GetPrevFrameFp();
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "frame type error!";
            UNREACHABLE();
        }
    }
}

uintptr_t FrameHandler::GetPrevFrameCallSiteSp(const JSTaggedType *sp, uintptr_t curPc)
{
    if (sp == nullptr) {
        return 0U;
    }

    auto type = GetFrameType(sp);
    switch (type) {
        case FrameType::LEAVE_FRAME: {
            auto frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            auto frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME: {
            auto frame = BuiltinFrame::GetFrameFromSp(sp);
            return frame->GetCallSiteSp();
        }
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto callSiteSp = reinterpret_cast<uintptr_t>(sp) +
                thread_->GetEcmaVM()->GetStackMapParser()->GetFuncFpDelta(curPc);
            return callSiteSp;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME : {
            auto callSiteSp = reinterpret_cast<uintptr_t>(sp) + sizeof(uintptr_t);
            return callSiteSp;
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        default: {
            return 0;
        }
    }
}

ARK_INLINE void FrameHandler::AdvanceToInterpretedFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        return;
    }
    for (; HasFrame() && !(IsInterpretedFrame() || IsInterpretedEntryFrame()); PrevFrame());
}

ARK_INLINE void FrameHandler::PrevInterpretedFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        auto frame = InterpretedFrameBase::GetFrameFromSp(sp_);
        sp_ = frame->GetPrevFrameFp();
        return;
    }
    AdvanceToInterpretedFrame();
    PrevFrame();
    AdvanceToInterpretedFrame();
}

JSTaggedType* FrameHandler::GetPrevInterpretedFrame()
{
    PrevInterpretedFrame();
    return GetSp();
}

uint32_t FrameHandler::GetNumberArgs()
{
    if (thread_->IsAsmInterpreter()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return static_cast<uint32_t>(frame->GetCurrentFramePointer() - sp_);
    }
    ASSERT(IsInterpretedFrame());
    JSTaggedType *prevSp = nullptr;
    if (IsAsmInterpretedFrame()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        prevSp = frame->GetPrevFrameFp();
    } else {
        auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
        prevSp = frame->GetPrevFrameFp();
    }
    auto prevSpEnd = reinterpret_cast<JSTaggedType*>(GetInterpretedFrameEnd(prevSp));
    return static_cast<uint32_t>(prevSpEnd - sp_);
}

JSTaggedValue FrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void FrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

JSTaggedValue FrameHandler::GetAcc() const
{
    ASSERT(IsInterpretedFrame());
    if (IsAsmInterpretedFrame()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return frame->acc;
    } else {
        auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
        return frame->acc;
    }
}

uint32_t FrameHandler::GetBytecodeOffset() const
{
    ASSERT(IsInterpretedFrame());
    JSMethod *method = GetMethod();
    auto offset = GetPc() - method->GetBytecodeArray();
    return static_cast<uint32_t>(offset);
}

JSMethod *FrameHandler::GetMethod() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
}

JSMethod *FrameHandler::CheckAndGetMethod() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    if (function.IsJSFunctionBase() || function.IsJSProxy()) {
        return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
    }
    return nullptr;
}

JSTaggedValue FrameHandler::GetFunction() const
{
    ASSERT(IsInterpretedFrame());
    if (thread_->IsAsmInterpreter()) {
        FrameType type = GetFrameType();
        switch (type) {
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                auto frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
                return frame->function;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto *frame = BuiltinWithArgvFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto *frame = BuiltinFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME:
            case FrameType::INTERPRETER_ENTRY_FRAME:
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME:
            case FrameType::LEAVE_FRAME_WITH_ARGV:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    } else {
        auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
        return frame->function;
    }
}

const uint8_t *FrameHandler::GetPc() const
{
    ASSERT(IsInterpretedFrame());
    if (IsAsmInterpretedFrame()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return frame->pc;
    } else {
        auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
        return frame->pc;
    }
}

ConstantPool *FrameHandler::GetConstpool() const
{
    ASSERT(IsInterpretedFrame());
    auto function = GetFunction();
    JSTaggedValue constpool = JSFunction::Cast(function.GetTaggedObject())->GetConstantPool();
    return ConstantPool::Cast(constpool.GetTaggedObject());
}

JSTaggedValue FrameHandler::GetEnv() const
{
    ASSERT(IsInterpretedFrame());
    if (IsAsmInterpretedFrame()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return frame->env;
    } else {
        auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
        return frame->env;
    }
}

void FrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    FrameHandler frameHandler(thread_);
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        os << "[" << i++
        << "]:" << frameHandler.GetMethod()->ParseFunctionName()
        << "\n";
    }
}

void FrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    FrameHandler frameHandler(thread_);
    ASSERT(frameHandler.HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - frameHandler.GetMethod()->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

ARK_INLINE uintptr_t FrameHandler::GetInterpretedFrameEnd(JSTaggedType *prevSp) const
{
    uintptr_t end = 0U;
    FrameType type = FrameHandler::GetFrameType(prevSp);
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto frame = AsmInterpretedFrame::GetFrameFromSp(prevSp);
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto frame = InterpretedFrame::GetFrameFromSp(prevSp);
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME:
            end = ToUintPtr(GetInterpretedEntryFrameStart(prevSp));
            break;
        case FrameType::BUILTIN_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME:
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::LEAVE_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        default: {
            LOG_ECMA(FATAL) << "frame type error!";
            UNREACHABLE();
        }
    }
    return end;
}

ARK_INLINE void FrameHandler::InterpretedFrameIterate(const JSTaggedType *sp,
                                                      const RootVisitor &v0,
                                                      const RootRangeVisitor &v1) const
{
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp);
    if (frame->function == JSTaggedValue::Hole()) {
        return;
    }
    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    uintptr_t start = ToUintPtr(sp);
    uintptr_t end = 0U;
    FrameType type = FrameHandler::GetFrameType(prevSp);
    if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
        auto prevFrame = InterpretedFrame::GetFrameFromSp(prevSp);
        end = ToUintPtr(prevFrame);
    } else if (type == FrameType::INTERPRETER_ENTRY_FRAME) {
        end = ToUintPtr(GetInterpretedEntryFrameStart(prevSp));
    } else {
        LOG_ECMA(FATAL) << "frame type error!";
    }
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));

    // pc == nullptr, init InterpretedFrame & native InterpretedFrame.
    if (frame->pc != nullptr) {
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
    }
}

ARK_INLINE void FrameHandler::AsmInterpretedFrameIterate(const JSTaggedType *sp,
                                                         const RootVisitor &v0,
                                                         const RootRangeVisitor &v1,
                                                         ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                         bool isVerifying)
{
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(sp);
    uintptr_t start = ToUintPtr(sp);
    uintptr_t end = ToUintPtr(frame->GetCurrentFramePointer());
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        v0(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
    }

    uintptr_t curPc = optimizedReturnAddr_;
    std::set<uintptr_t> slotAddrs;
    bool ret = thread_->GetEcmaVM()->GetStackMapParser()->CollectStackMapSlots(
        curPc, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying,
        optimizedCallSiteSp_);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << curPc;
#endif
        return;
    }


    for (auto slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::BuiltinFrameIterate(const JSTaggedType *sp,
                                                  [[maybe_unused]] const RootVisitor &v0,
                                                  const RootRangeVisitor &v1,
                                                  [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                  [[maybe_unused]] bool isVerifying)
{
    auto frame = BuiltinFrame::GetFrameFromSp(sp);
    // no need to visit stack map for entry frame
    if (frame->type == FrameType::BUILTIN_ENTRY_FRAME) {
        // only visit function
        v0(Root::ROOT_FRAME, ObjectSlot(frame->GetStackArgsAddress()));
        return;
    }
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    auto argc = frame->GetNumArgs() + BuiltinFrame::RESERVED_CALL_ARGCOUNT;
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void FrameHandler::BuiltinWithArgvFrameIterate(const JSTaggedType *sp,
                                                          [[maybe_unused]] const RootVisitor &v0,
                                                          const RootRangeVisitor &v1,
                                                          [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                          [[maybe_unused]] bool isVerifying)
{
    auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp);
    auto argc = frame->GetNumArgs() + BuiltinFrame::RESERVED_CALL_ARGCOUNT;
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE JSTaggedType *FrameHandler::GetInterpretedEntryFrameStart(const JSTaggedType *sp)
{
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    JSTaggedType *argcSp = const_cast<JSTaggedType *>(sp) - INTERPRETER_ENTRY_FRAME_STATE_SIZE - 1;
    uint32_t argc = argcSp[0];
    return argcSp - argc - RESERVED_CALL_ARGCOUNT;
}

ARK_INLINE void FrameHandler::InterpretedEntryFrameIterate(const JSTaggedType *sp,
                                                           [[maybe_unused]] const RootVisitor &v0,
                                                           const RootRangeVisitor &v1) const
{
    uintptr_t start = ToUintPtr(GetInterpretedEntryFrameStart(sp));
    uintptr_t end = ToUintPtr(sp - INTERPRETER_ENTRY_FRAME_STATE_SIZE);
    v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void FrameHandler::OptimizedFrameIterate(const JSTaggedType *sp,
    const RootVisitor &v0,
    [[maybe_unused]] const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    bool isVerifying)
{
    std::set<uintptr_t> slotAddrs;
    bool ret = thread_->GetEcmaVM()->GetStackMapParser()->CollectStackMapSlots(
        optimizedReturnAddr_, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying, optimizedCallSiteSp_);

    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << optimizedReturnAddr_;
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedJSFunctionFrameIterate(const JSTaggedType *sp,
    const RootVisitor &v0,
    [[maybe_unused]] const RootRangeVisitor &v1,
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
    bool isVerifying)
{
    OptimizedJSFunctionFrame *frame = OptimizedJSFunctionFrame::GetFrameFromSp(sp);

    int delta = thread_->GetEcmaVM()->GetStackMapParser()->GetFuncFpDelta(optimizedReturnAddr_);
    uintptr_t *preFrameSp = frame->ComputePrevFrameSp(sp, delta);

    auto argc = *(reinterpret_cast<uint64_t *>(preFrameSp));
    JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
    if (argc > 0) {
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }

    std::set<uintptr_t> slotAddrs;
    auto currentPc = optimizedReturnAddr_;
    bool ret = thread_->GetEcmaVM()->GetStackMapParser()->CollectStackMapSlots(
        currentPc, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying, optimizedCallSiteSp_);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << currentPc;
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedEntryFrameIterate(const JSTaggedType *sp,
                                                         const RootVisitor &v0,
                                                         [[maybe_unused]] const RootRangeVisitor &v1,
                                                         ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                         bool isVerifying)
{
    std::set<uintptr_t> slotAddrs;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto returnAddr = reinterpret_cast<uintptr_t>(*(reinterpret_cast<uintptr_t*>(const_cast<JSTaggedType *>(sp)) + 1));
    bool ret = thread_->GetEcmaVM()->GetStackMapParser()->CollectStackMapSlots(
        returnAddr, reinterpret_cast<uintptr_t>(sp), slotAddrs, derivedPointers, isVerifying, optimizedReturnAddr_);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << returnAddr;
#endif
        return;
    }

    for (const auto &slot : slotAddrs) {
        v0(Root::ROOT_FRAME, ObjectSlot(slot));
    }
}

ARK_INLINE void FrameHandler::OptimizedLeaveFrameIterate(const JSTaggedType *sp,
                                                         [[maybe_unused]] const RootVisitor &v0,
                                                         [[maybe_unused]] const RootRangeVisitor &v1,
                                                         [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                         [[maybe_unused]] bool isVerifying)
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void FrameHandler::OptimizedWithArgvLeaveFrameIterate(const JSTaggedType *sp,
                                                                 [[maybe_unused]] const RootVisitor &v0,
                                                                 [[maybe_unused]] const RootRangeVisitor &v1,
                                                                 [[maybe_unused]] ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers,
                                                                 [[maybe_unused]] bool isVerifying)
{
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t* argvPtr = reinterpret_cast<uintptr_t *>(&frame->argc + 1);
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(*argvPtr);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        v1(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

void FrameHandler::IterateRsp(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    IterateFrameChain(current, v0, v1);
}

void FrameHandler::IterateSp(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    while (current) {
        // only interpreter entry frame is on thread sp when rsp is enable
        ASSERT(FrameHandler::GetFrameType(current) == FrameType::INTERPRETER_ENTRY_FRAME);
        InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(current);
        InterpretedEntryFrameIterate(current, v0, v1);
        current = frame->GetPrevFrameFp();
    }
}

void FrameHandler::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (thread_->IsAsmInterpreter()) {
        IterateSp(v0, v1);
        IterateRsp(v0, v1);
        return;
    }
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    FrameType frameType = FrameHandler::GetFrameType(current);
    if (frameType != FrameType::INTERPRETER_ENTRY_FRAME) {
        auto leaveFrame = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
        if (leaveFrame != nullptr) {
            current = leaveFrame;
        }
    }
    IterateFrameChain(current, v0, v1);
}

void FrameHandler::IterateFrameChain(JSTaggedType *start, const RootVisitor &v0, const RootRangeVisitor &v1)
{
    ChunkMap<DerivedDataKey, uintptr_t> *derivedPointers = thread_->GetEcmaVM()->GetHeap()->GetDerivedPointers();
    bool isVerifying = false;

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    isVerifying = thread_->GetEcmaVM()->GetHeap()->IsVerifying();
#endif
    JSTaggedType *current = start;
    for (FrameIterator it(current); !it.done(); it.Advance()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_FRAME: {
                auto frame = it.GetFrame<OptimizedFrame>();
                OptimizedFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp(), optimizedReturnAddr_);
                optimizedReturnAddr_ = frame->returnAddr;
                break;
            }
            case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp());
                optimizedReturnAddr_ = frame->returnAddr;
                break;
            }
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                OptimizedJSFunctionFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp(), optimizedReturnAddr_);
                optimizedReturnAddr_ = frame->returnAddr;
                break;
            }
            case FrameType::OPTIMIZED_ENTRY_FRAME: {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
                break;
            }
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME: {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
                break;
            }
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                AsmInterpretedFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
                break;
            }
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME: {
                InterpretedFrameIterate(it.GetSp(), v0, v1);
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
                break;
            }
            case FrameType::LEAVE_FRAME: {
                auto frame = it.GetFrame<OptimizedLeaveFrame>();
                OptimizedLeaveFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp());
                optimizedReturnAddr_ = frame->returnAddr;
                break;
            }
            case FrameType::LEAVE_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<OptimizedWithArgvLeaveFrame>();
                OptimizedWithArgvLeaveFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp());
                optimizedReturnAddr_ = frame->returnAddr;
                break;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<BuiltinWithArgvFrame>();
                BuiltinWithArgvFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedReturnAddr_ = frame->returnAddr;
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp());
                break;
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto frame = it.GetFrame<BuiltinFrame>();
                BuiltinFrameIterate(it.GetSp(), v0, v1, derivedPointers, isVerifying);
                optimizedReturnAddr_ = frame->returnAddr;
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(it.GetSp());
                break;
            }
            case FrameType::INTERPRETER_ENTRY_FRAME: {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
}
}  // namespace panda::ecmascript

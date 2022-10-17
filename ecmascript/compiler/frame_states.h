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

#ifndef ECMASCRIPT_COMPILER_FRAME_STATE_H
#define ECMASCRIPT_COMPILER_FRAME_STATE_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {

class FrameStateInfo {
public:
    explicit FrameStateInfo(const MethodLiteral *literal)
        : numVregs_(literal->GetNumberVRegs() + 1),
          accumulatorIndex_(GetNumberVRegs() - 1),
          literal_(literal)
    {
    }

    explicit FrameStateInfo(const FrameStateInfo *other)
        : numVregs_(other->numVregs_),
          accumulatorIndex_(other->accumulatorIndex_),
          literal_(other->literal_)
    {
        values_ = other->values_;
    }

    size_t GetNumberVRegs() const
    {
        return numVregs_;
    }

    std::vector<GateRef> *GetValues()
    {
        return &values_;
    }

    void SetValuesAt(size_t index, GateRef gate)
    {
        ASSERT(gate != Circuit::NullGate());
        ASSERT(index < values_.size());
        values_[index] = gate;
    }

    GateRef ValuesAt(size_t index) const
    {
        ASSERT(index < values_.size());
        return values_[index];
    }

    GateRef ValuesAtAccumulator() const
    {
        return ValuesAt(accumulatorIndex_);
    }

    void UpdateAccumulator(GateRef gate)
    {
        return UpdateVirtualRegister(accumulatorIndex_, gate);
    }
    void UpdateVirtualRegister(size_t index, GateRef gate)
    {
        SetValuesAt(index, gate);
    }
    FrameStateInfo *Clone();
    size_t GetAccumulatorIndex() const
    {
        return accumulatorIndex_;
    }
private:
    size_t numVregs_ {0};
    size_t accumulatorIndex_ {0};
    const MethodLiteral *literal_ {nullptr};
    // [numVRegs_] [extra args] [numArgs_] [accumulator]
    std::vector<GateRef> values_{};
};

class FrameStateBuilder {
public:
    FrameStateBuilder(Circuit *circuit, const MethodLiteral *literal);
    ~FrameStateBuilder();

    GateRef ValuesAt(size_t index) const
    {
        return currentInfo_->ValuesAt(index);
    }

    GateRef ValuesAtAccumulator() const
    {
        return currentInfo_->ValuesAtAccumulator();
    }

    void UpdateAccumulator(GateRef gate)
    {
        currentInfo_->UpdateAccumulator(gate);
    }
    void UpdateVirtualRegister(size_t index, GateRef gate)
    {
        currentInfo_->UpdateVirtualRegister(index, gate);
    }
    void BindGuard(GateRef gate, size_t pcOffset, GateRef glue);
    void AdvenceToBasicBlock(size_t id);
    size_t GetAccumulatorIndex() const
    {
        return currentInfo_->GetAccumulatorIndex();
    }
private:
    FrameStateInfo *CloneFrameState(FrameStateInfo *current, size_t id)
    {
        ASSERT(stateInfos_[id] == nullptr);
        auto info = current->Clone();
        stateInfos_[id] = info;
        return info;
    }
    GateRef FrameState(size_t pcOffset);
    void InitFrameValues();

    FrameStateInfo *currentInfo_{nullptr};
    const MethodLiteral *literal_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor gateAcc_;
    std::map<size_t, FrameStateInfo *> stateInfos_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FRAME_STATE_H
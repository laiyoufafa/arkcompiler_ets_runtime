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

#ifndef ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H
#define ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H

#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class NumberSpeculativeLowering {
public:
    NumberSpeculativeLowering(Circuit *circuit, ChunkVector<TypeInfo>& typeInfos)
        : circuit_(circuit), acc_(circuit), builder_(circuit), typeInfos_(typeInfos) {}
    void Run();

private:
    void VisitGate(GateRef gate);
    void VisitNumberBinaryOp(GateRef gate);
    void VisitConstant(GateRef gate);
    void VisitPhi(GateRef gate);
    
    template<TypedBinOp Op>
    void VisitNumberCalculate(GateRef gate);
    template<TypedBinOp Op>
    void VisitNumberCompare(GateRef gate);

    template<TypedBinOp Op>
    GateRef CalculateInts(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CalculateDoubles(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareInts(GateRef left, GateRef right);
    template<TypedBinOp Op>
    GateRef CompareDoubles(GateRef left, GateRef right);

    Circuit* circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    // ChunkVector<UseInfo>& useInfos_;
    ChunkVector<TypeInfo>& typeInfos_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_SPECULATIVE_LOWERING_H
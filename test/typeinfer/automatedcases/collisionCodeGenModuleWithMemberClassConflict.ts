/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/collisionCodeGenModuleWithMemberClassConflict.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export class m1 {
    }
}
let foo = new m1.m1();
AssertType(foo, "m1.m1");
AssertType(new m1.m1(), "m1.m1");
AssertType(m1.m1, "typeof m1.m1");

module m2 {
    export class m2 {
    }

    export class _m2 {
    }
}
let foo = new m2.m2();
AssertType(foo, "m1.m1");
AssertType(new m2.m2(), "m2.m2");
AssertType(m2.m2, "typeof m2.m2");

let foo = new m2._m2();
AssertType(foo, "m1.m1");
AssertType(new m2._m2(), "m2._m2");
AssertType(m2._m2, "typeof m2._m2");


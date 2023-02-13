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

// === tests/cases/compiler/localClassesInLoop_ES6.ts ===
declare function AssertType(value:any, type:string):void;
declare function use(a: any);

"use strict"
AssertType("use strict", "string");

let data = [];
AssertType(data, "any[]");
AssertType([], "undefined[]");

for (let x = 0; x < 2; ++x) {
    class C { }
    data.push(() => C);
AssertType(data.push(() => C), "number");
AssertType(data.push, "(...any[]) => number");
AssertType(() => C, "() => typeof C");
AssertType(C, "typeof C");
}

use(data[0]() === data[1]());
AssertType(use(data[0]() === data[1]()), "any");
AssertType(use, "(any) => any");
AssertType(data[0]() === data[1](), "boolean");
AssertType(data[0](), "any");
AssertType(data[0], "any");
AssertType(data, "any[]");
AssertType(0, "int");
AssertType(data[1](), "any");
AssertType(data[1], "any");
AssertType(data, "any[]");
AssertType(1, "int");


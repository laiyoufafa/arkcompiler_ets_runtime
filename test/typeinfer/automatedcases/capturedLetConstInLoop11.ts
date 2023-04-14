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

// === tests/cases/compiler/capturedLetConstInLoop11.ts ===
declare function AssertType(value:any, type:string):void;
for (;;) {
    let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    () => x;
AssertType(() => x, "() => number");
AssertType(x, "number");
}

function foo() {
    for (;;) {
        const a = 0;
AssertType(a, "int");
AssertType(0, "int");

        switch(a) {
AssertType(a, "int");

            case 0: 
AssertType(0, "int");
AssertType(() => a, "() => number");
AssertType(a, "int");
return () => a;
        }
    }
}

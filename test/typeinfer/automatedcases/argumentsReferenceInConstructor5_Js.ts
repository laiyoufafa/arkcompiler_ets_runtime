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

// === /a.js ===
declare function AssertType(value:any, type:string):void;
const bar = {
AssertType(bar, "{ arguments: {}; }");
AssertType({	arguments: {}}, "{ arguments: {}; }");

	arguments: {
AssertType(arguments, "{}");

AssertType({}, "{}");
}
}

class A {
	/**
	 * Constructor
	 *
	 * @param {object} [foo={}]
	 */
	constructor(foo = {}) {
		/**
		 * @type object
		 */
		this.foo = foo;
AssertType(this.foo = foo, "any");
AssertType(this.foo, "any");
AssertType(this, "this");
AssertType(foo, "any");

		/**
		 * @type object
		 */
		this.bar = bar.arguments;
AssertType(this.bar = bar.arguments, "{}");
AssertType(this.bar, "any");
AssertType(this, "this");
AssertType(bar.arguments, "{}");
	}
}


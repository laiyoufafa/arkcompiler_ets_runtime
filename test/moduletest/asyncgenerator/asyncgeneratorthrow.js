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

/*
 * @tc.name:asyncgenerator
 * @tc.desc:test asyncgenerator function
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
async function* g() {
  while (true) {
    try {
      yield 3;
    } catch (e) {
      print(e);
    }
  }
}

const it = g();
print("asyncgenerator throw start");
it.next(1).then((res) => print(res.value)); // { value: 3, done: false }
it.throw(new Error('async generator wrong')) // async generator wrong
  .then((res) => print(res.value)); // { value: 3, done: false }
print("asyncgenerator throw end"); 

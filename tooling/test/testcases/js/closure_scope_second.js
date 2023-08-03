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
var globalFunc = null;

function func1() {
  var a = 10;
  for (let i = 0; i < 10; i++) {
    print(a);
    function func2() {
      let b = 3;
      print(i);
      print(i+1);
    }
    if (i == 5) {
      globalFunc = function func3() {
        print(a);
        print(i);
        print(a+1);
      }
    }
  }
}
func1();
globalFunc();
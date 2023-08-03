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
function func() {
    let v1 = 1;
    function func2() {
      var v2_1 = 2;
      const v2_2 = 3;
      function func3() {
        const v3 = 4;
        print(v1);
        print(v2_1)
        function func4() {
          let v4 = 5;
          print(v2_2);
          function func5() {
            print(v3);
            print(v1);
          }
          return func5;
        }
        return func4;
      }
      return func3;
    }
    return func2;
}
const func2 = func();
const func3 = func2();
const func4 = func3();
const func5 = func4();
func5();
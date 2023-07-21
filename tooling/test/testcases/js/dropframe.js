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

function test_dropframe() {
    function test_nested() {
        a = 2;
        var d = 0;
        function nested_inner1() {
            b = 3;
            d = 1;
            var e = 0;
            function nested_inner2() {
                c = 4;
                d = 2;
                e = 1;
            }
            nested_inner2();
        }
        nested_inner1();
    }

    function test_parallel() {
        a = 6;
        b = 4;
        c = 5;
        var d = 1;
        function parallel_inner1() {
            a = 4;
            b = 5;
            c = 6;
            d = d * 2;
        }
        function parallel_inner2() {
            d = d + 1;
            parallel_inner1();
        }
        parallel_inner2();
    }

    function test_multicall() {
        var d = 1;
        function multicall_inner() {
            a = a * 2;
            b = b + a;
            c = c * b;
            d = d * c + b * a;
        }
        multicall_inner();
        multicall_inner();
    }

    function test_forloop1() {
        var d = 1;
        function forloop1_inner() {
            a = a * 2;
            b = b + a;
            c = c + b * a;
            d = d * (b - a);
        }
        for (let i = 1; i <= 2; i++) {
            for (let j = 3; j <= 4; j++) {
                forloop1_inner();
            }
        }
    }

    function test_forloop2() {
        var d = 1;
        for (let i = 1; i <= 2; i++) {
            for (let j = 3; j <= 4; j++) {
                d = d + i * j;
                function forloop2_inner() {
                    a = a * i;
                    b = b + a * j;
                    c = c + a + b;
                    d = d * i * j + c;
                }
                forloop2_inner();
            }
        }
    }

    function test_assign() {
        var func = undefined
        function func1() {
            function func2() {
                var s = 2
                func = function func3() {
                    s++
                }
                func()
            }
            func2()
            func()
        }
        func1();
    }

    function test_recursion() {
        var x = 6;
        x = x + 1;
        a = a + 1;
        if (a < 10) {
            test_recursion();
        }
        x = x + 1;
        a = a + 1;
    }

    var a = 1;
    var b = 2;
    var c = 3;
    test_nested();
    test_parallel();
    test_multicall();
    test_forloop1();
    test_forloop2();
    test_assign();
    a = 1;
    test_recursion();
    a = a + c;
    b = b + a;
}

test_dropframe();
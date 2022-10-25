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

var o = {
    "foo" : function() {
        var number0 = 1;
        var string0 = "helloworld";
        var boolean0 = new Boolean(0);
        var obj0 = {
            "key0": "value0",
            "key1": 100
        };
        function function0() {
            var test = 0;
        }
        var map0 = new Map();
        var set0 = new Set();
        var undefined0 = undefined;
        let array0 = ['Apple', 'Banana']
        var date0 = new Date('1995-12-17T03:24:00');
        function* generator0() {
            let aa = 0;
            var a1 = 100;
            yield 1;
            yield 2;
            yield 3;
        }
        var regexp0 = /^\d+\.\d+$/i;
        var arraybuffer0 = new ArrayBuffer(10);
        var uint8array0 = new Uint8Array(arraybuffer0);
        const dataview0 = new DataView(arraybuffer0, 0);
        var bigint0 = BigInt(999n);

        var set1 = new Set();
        set1.add(number0);
        var set2 = new Set(string0);
        var set3 = new Set();
        set3.add(obj0);
        var set4 = new Set(undefined0);
        var set5 = new Set(array0);
        var set6 = new Set();
        var set7 = new Set();
        var set8 = new Set();
        set8.add(generator0);
        var set9 = new Set();
        set9.add(regexp0);
        var set10 = new Set();
        set10.add(arraybuffer0);
        var set11 = new Set();
        set11.add(uint8array0);
        var set12 = new Set();
        set12.add(dataview0);
        var set13 = new Set();
        set13.add(date0);
        var set14 = new Set();
        set14.add(function0);
        var set15 = set0;
        var set16 = new Set();
        set16.add(0);
        set16.add("hello");
        set16.add(obj0);
        set16.add(date0);
        var set17 = new Set();
        set17.add(map0);
        var set18 = new Set();
        set18.add(bigint0);
        var set19 = new Set();
        set19.add(boolean0);
        var set20 = new Set(set0);

        var number1 = 65535;
        var number2 = 5e-324;
        var number3 = 10 ** 10;
        var number4 = 0x1ffffffffff;
        var number5 = 0b11111111111111;
        var number6 = new Number(bigint0);
        var number7 = 123e45;
        var number8 = number0;
        var number9 = number0 + number1;
        var number10 = number0 - number1;
        var number11 = number0 * number1;
        var number12 = number0 / number1;
        var number13 = number0 % number1;
        var number14 = new Number(0);
        var number15 = new Number(1.7976931348623157e+308);
        var number16 = new Number(5e-324);
        var number17 = new Number(10 ** 10);
        var number18 = new Number(0x1ffffffffff);
        var number19 = new Number(0b11111111111111);
        var number20 = new Number(123e45);
        var number21 = new Number(number0);
        var number22 = new Number(number0 + number1);
        var number23 = new Number(number0 - number1);
        var number24 = new Number(number0 * number1);
        var number25 = new Number(number0 / number1);
        var number26 = new Number(number0 % number1);
        var number27 = 1.7976931348623157e+308;

        var string1 = "";
        var string2 = string1;
        var string3 = string1 + 'world';
        var string4 = 'hello' + 'world'
        var string5 = string4.charAt(1);
        var string6 = string1 + number0;
        var string7 = new String(string1);
        var string8 = new String(set0);
        var string9 = new String(number0);
        var string10 = new String(string0);
        var string11 = new String(obj0);
        var string12 = new String(undefined0);
        var string13 = new String(array0);
        var string14 = new String(date0);
        var string15 = new String(bigint0);
        var string16 = new String(generator0);
        var string17 = new String(regexp0);
        var string18 = new String(arraybuffer0);
        var string19 = new String(uint8array0);
        var string20 = new String(dataview0);
        var string21 = new String(map0);
        var string22 = new String(function0);

        var nop = undefined;
    }
}

o.foo()
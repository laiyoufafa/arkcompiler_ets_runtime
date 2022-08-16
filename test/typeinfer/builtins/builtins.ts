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

declare function AssertType(value: any, type: string): void;
{
    let anyArr: any[] = [];
    let it = anyArr[Symbol.iterator]();

    // Object
    let obj = new Object();
    AssertType(obj.toString(), "string");
    AssertType(obj.toLocaleString(), "string");
    AssertType(obj.hasOwnProperty("key"), "boolean");
    AssertType(obj.isPrototypeOf({}), "boolean");
    AssertType(obj.propertyIsEnumerable(""), "boolean");
    AssertType(Object.getOwnPropertyDescriptor({}, "str"), "union");
    AssertType(Object.getOwnPropertyNames({}), "array");
    AssertType(Object.getOwnPropertySymbols({})[0], "symbol");
    AssertType(Object.getPrototypeOf({}), "union");
    AssertType(Object.is("arg1", "arg2"), "boolean");
    AssertType(Object.entries({}), "array");
    AssertType(Object.fromEntries(it), "any");

    // Function
    let f = new Function();
    AssertType(f.toString(), "string");
    AssertType(f.length, "number");

    // Error
    let err = new Error();
    AssertType(err.message, "string");
    AssertType(err.toString(), "string");

    // RangeError
    let rangeErr = new RangeError();
    AssertType(rangeErr.message, "string");
    AssertType(rangeErr.toString(), "string");

    // Boolean
    let bool = new Boolean(4);
    AssertType(bool.toString(), "string");
    AssertType(bool.valueOf(), "boolean");

    // Date
    let date = new Date();
    AssertType(date.getDate(), "number");
    AssertType(date.setDate(111), "number");
    AssertType(date.toDateString(), "string");
    AssertType(date.toISOString(), "string");
    AssertType(date.valueOf(), "number");
    AssertType(Date.parse(""), "number");
    AssertType(Date.UTC(1, 1), "number");
    AssertType(Date.now(), "number");

    // Math
    AssertType(Math.E, "number");
    AssertType(Math.acos(1), "number");
    AssertType(Math.atan2(1, 2), "number");

    // JSON
    AssertType(JSON.stringify("111"), "string");
    AssertType(JSON.parse("111"), "any");

    // Number
    let n = new Number(123);
    AssertType(n.toExponential(123), "string");
    AssertType(n.toFixed(), "string");
    AssertType(n.toPrecision(), "string");
    AssertType(n.toString(111), "string");
    AssertType(n.valueOf(), "number");
    AssertType(Number.isFinite(1), "boolean");
    AssertType(Number.isSafeInteger(1), "boolean");
    AssertType(Number.parseFloat("1.2"), "number");
    AssertType(Number.MAX_VALUE, "number");

    // Set
    let set = new Set();
    AssertType(set.size, "number");
    AssertType(set.add(1), "any");
    AssertType(set.delete(1), "boolean");
    AssertType(set.entries(), "interface");
    AssertType(set.clear(), "void");
    AssertType(set.forEach(e => { }), "void");
    AssertType(set.has(1), "boolean");
    AssertType(set.values(), "interface");

    // WeakSet
    let weakset = new WeakSet();
    AssertType(weakset.add(obj), "any");
    AssertType(weakset.delete(obj), "boolean");
    AssertType(weakset.has(obj), "boolean");

    // Array
    let arr = new Array();
    AssertType(arr.length, "number");
    AssertType(arr.concat([1, 2, 3]), "array");
    AssertType(arr.copyWithin(0, 1), "any");
    AssertType(arr.entries(), "interface");
    AssertType(arr.every((v: any, i: number, arr: any[]) => { }), "boolean");
    AssertType(arr.fill(1), "any");
    AssertType(arr.filter((v: any, i: number, arr: any[]) => { }), "array");
    AssertType(arr.findIndex((v: any, i: number, arr: any[]) => { }), "number");
    AssertType(arr.forEach((v: any, i: number, arr: any[]) => { }), "void");
    AssertType(arr.indexOf(1), "number");
    AssertType(arr.join(","), "string");
    AssertType(arr.keys(), "interface");
    AssertType(arr.lastIndexOf(1), "number");
    AssertType(arr.map(((v: any, i: number, arry: any[]) => { })), "array");
    AssertType(arr.pop(), "union");
    AssertType(arr.push([1, 2, 3]), "number");
    AssertType(arr.reduce((p: any, cv: any, ci: any, arr: any[]) => { }), "any");
    AssertType(arr.reduceRight((p: any, cv: any, ci: any, arr: any[]) => { }), "any");
    AssertType(arr.reverse(), "array");
    AssertType(arr.shift(), "union");
    AssertType(arr.slice(), "array");
    AssertType(arr.some((v: any, i: number, arr: any[]) => { }), "boolean");
    AssertType(arr.sort(), "any");
    AssertType(arr.splice(1, 2, [1, 2]), "array");
    AssertType(arr.toLocaleString(), "string");
    AssertType(arr.toString(), "string");
    AssertType(arr.unshift([1, 2, 3]), "number");
    AssertType(arr.values(), "interface");
    AssertType(arr.includes(1), "boolean");
    AssertType(arr.flatMap((x) => [x]), "array");
    AssertType(arr.flat(), "array");

    // ArrayBuffer
    let arrBuf = new ArrayBuffer(5);
    AssertType(arrBuf.byteLength, "number");
    AssertType(arrBuf.slice(1), "class_instance");
    AssertType(ArrayBuffer.isView(1), "boolean");

    // SharedArrayBuffer
    let sharedArrBuf = new SharedArrayBuffer(5);
    AssertType(sharedArrBuf.byteLength, "number");
    AssertType(sharedArrBuf.slice(2), "class_instance");

    // String
    let str = new String("111");
    AssertType(str.length, "number");
    AssertType(str.charAt(1), "string");
    AssertType(str.charCodeAt(1), "number");
    AssertType(str.concat("123"), "string");
    AssertType(str.includes("123"), "boolean");
    AssertType(str.indexOf("123"), "number");
    AssertType(str.localeCompare("123"), "number");
    AssertType(str.match("123"), "union");
    AssertType(str.matchAll(/e/), "interface");
    AssertType(str.normalize("123"), "string");
    AssertType(str.repeat(0), "string");
    AssertType(str.replace("111", "222"), "string");
    AssertType(str.search("123"), "number");
    AssertType(str.slice(), "string");
    AssertType(str.split("1"), "array");
    AssertType(str.startsWith("123"), "boolean");
    AssertType(str.substring(1), "string");
    AssertType(str.toLocaleLowerCase(), "string");
    AssertType(str.toLowerCase(), "string");
    AssertType(str.trim(), "string");
    AssertType(str.valueOf(), "string");
    AssertType(String.fromCharCode(1,2), "string");
    AssertType(String.raw`Multiline\nstring`, "string");

    // Symbol
    let sym = Symbol.prototype;
    AssertType(sym.description, "union");
    AssertType(sym.valueOf(), "symbol");
    AssertType(sym.toString(), "string");
    AssertType(Symbol.for("11"), "symbol");
    AssertType(Symbol.keyFor(Symbol.iterator), "union");
    AssertType(Symbol.iterator, "symbol");
    AssertType(Symbol.prototype, "class_instance");

    // WeakRef
    let werkRef = new WeakRef(str);
    AssertType(werkRef.deref(), "union");
}

/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/snapshot/mem/snapshot_serialize.h"

#include "ecmascript/base/error_type.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_deque.h"
#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_queue.h"
#include "ecmascript/containers/containers_stack.h"
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_plain_array_iterator.h"
#include "ecmascript/js_api_queue_iterator.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"

namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using BuiltinsBigInt = builtins::BuiltinsBigInt;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsTypedArray = builtins::BuiltinsTypedArray;
using BuiltinsIterator = builtins::BuiltinsIterator;
using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using ErrorType = base::ErrorType;
using Global = builtins::BuiltinsGlobal;
using BuiltinsString = builtins::BuiltinsString;
using StringIterator = builtins::BuiltinsStringIterator;
using RegExp = builtins::BuiltinsRegExp;
using Function = builtins::BuiltinsFunction;
using Math = builtins::BuiltinsMath;
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using Json = builtins::BuiltinsJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
using ErrorType = base::ErrorType;
using DataView = builtins::BuiltinsDataView;
using Intl = builtins::BuiltinsIntl;
using Locale = builtins::BuiltinsLocale;
using DateTimeFormat = builtins::BuiltinsDateTimeFormat;
using NumberFormat = builtins::BuiltinsNumberFormat;
using RelativeTimeFormat = builtins::BuiltinsRelativeTimeFormat;
using Collator = builtins::BuiltinsCollator;
using PluralRules = builtins::BuiltinsPluralRules;
using ArrayList = containers::ContainersArrayList;
using TreeMap = containers::ContainersTreeMap;
using TreeSet = containers::ContainersTreeSet;
using Queue = containers::ContainersQueue;
using PlainArray = containers::ContainersPlainArray;
using Deque = containers::ContainersDeque;
using ContainerStack = panda::ecmascript::containers::ContainersStack;
using ContainersPrivate = containers::ContainersPrivate;

constexpr int METHOD_SIZE = sizeof(JSMethod);

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static uintptr_t g_nativeTable[] = {
    reinterpret_cast<uintptr_t>(nullptr),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Species),
    reinterpret_cast<uintptr_t>(StringIterator::Next),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeInvokeSelf),
    reinterpret_cast<uintptr_t>(Function::FunctionConstructor),
    reinterpret_cast<uintptr_t>(JSFunction::AccessCallerArgumentsThrowTypeError),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeApply),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeBind),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeCall),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeToString),
    reinterpret_cast<uintptr_t>(Object::ObjectConstructor),
    reinterpret_cast<uintptr_t>(Error::ErrorConstructor),
    reinterpret_cast<uintptr_t>(Error::ToString),
    reinterpret_cast<uintptr_t>(RangeError::RangeErrorConstructor),
    reinterpret_cast<uintptr_t>(RangeError::ToString),
    reinterpret_cast<uintptr_t>(ReferenceError::ReferenceErrorConstructor),
    reinterpret_cast<uintptr_t>(ReferenceError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::TypeErrorConstructor),
    reinterpret_cast<uintptr_t>(TypeError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::ThrowTypeError),
    reinterpret_cast<uintptr_t>(URIError::URIErrorConstructor),
    reinterpret_cast<uintptr_t>(URIError::ToString),
    reinterpret_cast<uintptr_t>(SyntaxError::SyntaxErrorConstructor),
    reinterpret_cast<uintptr_t>(SyntaxError::ToString),
    reinterpret_cast<uintptr_t>(EvalError::EvalErrorConstructor),
    reinterpret_cast<uintptr_t>(EvalError::ToString),
    reinterpret_cast<uintptr_t>(Number::NumberConstructor),
    reinterpret_cast<uintptr_t>(Number::ToExponential),
    reinterpret_cast<uintptr_t>(Number::ToFixed),
    reinterpret_cast<uintptr_t>(Number::ToLocaleString),
    reinterpret_cast<uintptr_t>(Number::ToPrecision),
    reinterpret_cast<uintptr_t>(Number::ToString),
    reinterpret_cast<uintptr_t>(Number::ValueOf),
    reinterpret_cast<uintptr_t>(Number::IsFinite),
    reinterpret_cast<uintptr_t>(Number::IsInteger),
    reinterpret_cast<uintptr_t>(Number::IsNaN),
    reinterpret_cast<uintptr_t>(Number::IsSafeInteger),
    reinterpret_cast<uintptr_t>(Number::ParseFloat),
    reinterpret_cast<uintptr_t>(Number::ParseInt),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::BigIntConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsUintN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsIntN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ValueOf),
    reinterpret_cast<uintptr_t>(Symbol::SymbolConstructor),
    reinterpret_cast<uintptr_t>(Symbol::For),
    reinterpret_cast<uintptr_t>(Symbol::KeyFor),
    reinterpret_cast<uintptr_t>(Symbol::DescriptionGetter),
    reinterpret_cast<uintptr_t>(Symbol::ToPrimitive),
    reinterpret_cast<uintptr_t>(Symbol::ToString),
    reinterpret_cast<uintptr_t>(Symbol::ValueOf),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeHasInstance),
    reinterpret_cast<uintptr_t>(Date::DateConstructor),
    reinterpret_cast<uintptr_t>(Date::GetDate),
    reinterpret_cast<uintptr_t>(Date::GetDay),
    reinterpret_cast<uintptr_t>(Date::GetFullYear),
    reinterpret_cast<uintptr_t>(Date::GetHours),
    reinterpret_cast<uintptr_t>(Date::GetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetMinutes),
    reinterpret_cast<uintptr_t>(Date::GetMonth),
    reinterpret_cast<uintptr_t>(Date::GetSeconds),
    reinterpret_cast<uintptr_t>(Date::GetTime),
    reinterpret_cast<uintptr_t>(Date::GetTimezoneOffset),
    reinterpret_cast<uintptr_t>(Date::GetUTCDate),
    reinterpret_cast<uintptr_t>(Date::GetUTCDay),
    reinterpret_cast<uintptr_t>(Date::GetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::GetUTCHours),
    reinterpret_cast<uintptr_t>(Date::GetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::GetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::GetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::SetDate),
    reinterpret_cast<uintptr_t>(Date::SetFullYear),
    reinterpret_cast<uintptr_t>(Date::SetHours),
    reinterpret_cast<uintptr_t>(Date::SetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetMinutes),
    reinterpret_cast<uintptr_t>(Date::SetMonth),
    reinterpret_cast<uintptr_t>(Date::SetSeconds),
    reinterpret_cast<uintptr_t>(Date::SetTime),
    reinterpret_cast<uintptr_t>(Date::SetUTCDate),
    reinterpret_cast<uintptr_t>(Date::SetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::SetUTCHours),
    reinterpret_cast<uintptr_t>(Date::SetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::SetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::SetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::ToDateString),
    reinterpret_cast<uintptr_t>(Date::ToISOString),
    reinterpret_cast<uintptr_t>(Date::ToJSON),
    reinterpret_cast<uintptr_t>(Date::ToLocaleDateString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleTimeString),
    reinterpret_cast<uintptr_t>(Date::ToString),
    reinterpret_cast<uintptr_t>(Date::ToTimeString),
    reinterpret_cast<uintptr_t>(Date::ToUTCString),
    reinterpret_cast<uintptr_t>(Date::ValueOf),
    reinterpret_cast<uintptr_t>(Date::ToPrimitive),
    reinterpret_cast<uintptr_t>(Date::Now),
    reinterpret_cast<uintptr_t>(Date::Parse),
    reinterpret_cast<uintptr_t>(Date::UTC),
    reinterpret_cast<uintptr_t>(Object::Assign),
    reinterpret_cast<uintptr_t>(Object::Create),
    reinterpret_cast<uintptr_t>(Object::DefineProperties),
    reinterpret_cast<uintptr_t>(Object::DefineProperty),
    reinterpret_cast<uintptr_t>(Object::Freeze),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyNames),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertySymbols),
    reinterpret_cast<uintptr_t>(Object::GetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::Is),
    reinterpret_cast<uintptr_t>(Object::IsExtensible),
    reinterpret_cast<uintptr_t>(Object::IsFrozen),
    reinterpret_cast<uintptr_t>(Object::IsSealed),
    reinterpret_cast<uintptr_t>(Object::Keys),
    reinterpret_cast<uintptr_t>(Object::PreventExtensions),
    reinterpret_cast<uintptr_t>(Object::Seal),
    reinterpret_cast<uintptr_t>(Object::SetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::HasOwnProperty),
    reinterpret_cast<uintptr_t>(Object::IsPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::PropertyIsEnumerable),
    reinterpret_cast<uintptr_t>(Object::ToLocaleString),
    reinterpret_cast<uintptr_t>(Object::ToString),
    reinterpret_cast<uintptr_t>(Object::ValueOf),
    reinterpret_cast<uintptr_t>(Object::ProtoGetter),
    reinterpret_cast<uintptr_t>(Object::ProtoSetter),
    reinterpret_cast<uintptr_t>(Object::CreateRealm),
    reinterpret_cast<uintptr_t>(Object::Entries),
    reinterpret_cast<uintptr_t>(Boolean::BooleanConstructor),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeToString),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeValueOf),
    reinterpret_cast<uintptr_t>(RegExp::RegExpConstructor),
    reinterpret_cast<uintptr_t>(RegExp::Exec),
    reinterpret_cast<uintptr_t>(RegExp::Test),
    reinterpret_cast<uintptr_t>(RegExp::ToString),
    reinterpret_cast<uintptr_t>(RegExp::GetFlags),
    reinterpret_cast<uintptr_t>(RegExp::GetSource),
    reinterpret_cast<uintptr_t>(RegExp::GetGlobal),
    reinterpret_cast<uintptr_t>(RegExp::GetIgnoreCase),
    reinterpret_cast<uintptr_t>(RegExp::GetMultiline),
    reinterpret_cast<uintptr_t>(RegExp::GetDotAll),
    reinterpret_cast<uintptr_t>(RegExp::GetSticky),
    reinterpret_cast<uintptr_t>(RegExp::GetUnicode),
    reinterpret_cast<uintptr_t>(RegExp::Split),
    reinterpret_cast<uintptr_t>(RegExp::Search),
    reinterpret_cast<uintptr_t>(RegExp::Match),
    reinterpret_cast<uintptr_t>(RegExp::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsSet::SetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsSet::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Values),
    reinterpret_cast<uintptr_t>(BuiltinsSet::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Species),
    reinterpret_cast<uintptr_t>(BuiltinsMap::MapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsMap::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Values),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsMap::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::WeakMapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::WeakSetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Pop),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Push),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Shift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Splice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Unshift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IsArray),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Unscopables),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::TypedArrayBaseConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Set),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Subarray),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetBuffer),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteOffset),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToStringTag),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ClampedArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::StringConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharCodeAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CodePointAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsString::EndsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsString::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LocaleCompare),
    reinterpret_cast<uintptr_t>(BuiltinsString::Match),
    reinterpret_cast<uintptr_t>(BuiltinsString::Normalize),
    reinterpret_cast<uintptr_t>(BuiltinsString::Repeat),
    reinterpret_cast<uintptr_t>(BuiltinsString::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsString::Search),
    reinterpret_cast<uintptr_t>(BuiltinsString::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsString::Split),
    reinterpret_cast<uintptr_t>(BuiltinsString::StartsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Substring),
    reinterpret_cast<uintptr_t>(BuiltinsString::SubStr),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::Trim),
    reinterpret_cast<uintptr_t>(BuiltinsString::ValueOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetStringIterator),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCharCode),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCodePoint),
    reinterpret_cast<uintptr_t>(BuiltinsString::Raw),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetLength),
    reinterpret_cast<uintptr_t>(ArrayBuffer::ArrayBufferConstructor),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Slice),
    reinterpret_cast<uintptr_t>(ArrayBuffer::IsView),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Species),
    reinterpret_cast<uintptr_t>(ArrayBuffer::GetByteLength),
    reinterpret_cast<uintptr_t>(DataView::DataViewConstructor),
    reinterpret_cast<uintptr_t>(DataView::GetFloat32),
    reinterpret_cast<uintptr_t>(DataView::GetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetInt8),
    reinterpret_cast<uintptr_t>(DataView::GetInt16),
    reinterpret_cast<uintptr_t>(DataView::GetInt32),
    reinterpret_cast<uintptr_t>(DataView::GetUint8),
    reinterpret_cast<uintptr_t>(DataView::GetUint16),
    reinterpret_cast<uintptr_t>(DataView::GetUint32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::GetBigUint64),
    reinterpret_cast<uintptr_t>(DataView::SetInt8),
    reinterpret_cast<uintptr_t>(DataView::SetInt16),
    reinterpret_cast<uintptr_t>(DataView::SetInt32),
    reinterpret_cast<uintptr_t>(DataView::SetUint8),
    reinterpret_cast<uintptr_t>(DataView::SetUint16),
    reinterpret_cast<uintptr_t>(DataView::SetUint32),
    reinterpret_cast<uintptr_t>(DataView::GetBuffer),
    reinterpret_cast<uintptr_t>(DataView::GetByteLength),
    reinterpret_cast<uintptr_t>(DataView::GetOffset),
    reinterpret_cast<uintptr_t>(DataView::SetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::SetBigUint64),
    reinterpret_cast<uintptr_t>(Global::PrintEntrypoint),
    reinterpret_cast<uintptr_t>(Global::NotSupportEval),
    reinterpret_cast<uintptr_t>(Global::IsFinite),
    reinterpret_cast<uintptr_t>(Global::IsNaN),
    reinterpret_cast<uintptr_t>(Global::DecodeURI),
    reinterpret_cast<uintptr_t>(Global::DecodeURIComponent),
    reinterpret_cast<uintptr_t>(Global::EncodeURI),
    reinterpret_cast<uintptr_t>(Global::EncodeURIComponent),
    reinterpret_cast<uintptr_t>(Math::Abs),
    reinterpret_cast<uintptr_t>(Math::Acos),
    reinterpret_cast<uintptr_t>(Math::Acosh),
    reinterpret_cast<uintptr_t>(Math::Asin),
    reinterpret_cast<uintptr_t>(Math::Asinh),
    reinterpret_cast<uintptr_t>(Math::Atan),
    reinterpret_cast<uintptr_t>(Math::Atanh),
    reinterpret_cast<uintptr_t>(Math::Atan2),
    reinterpret_cast<uintptr_t>(Math::Cbrt),
    reinterpret_cast<uintptr_t>(Math::Ceil),
    reinterpret_cast<uintptr_t>(Math::Clz32),
    reinterpret_cast<uintptr_t>(Math::Cos),
    reinterpret_cast<uintptr_t>(Math::Cosh),
    reinterpret_cast<uintptr_t>(Math::Exp),
    reinterpret_cast<uintptr_t>(Math::Expm1),
    reinterpret_cast<uintptr_t>(Math::Floor),
    reinterpret_cast<uintptr_t>(Math::Fround),
    reinterpret_cast<uintptr_t>(Math::Hypot),
    reinterpret_cast<uintptr_t>(Math::Imul),
    reinterpret_cast<uintptr_t>(Math::Log),
    reinterpret_cast<uintptr_t>(Math::Log1p),
    reinterpret_cast<uintptr_t>(Math::Log10),
    reinterpret_cast<uintptr_t>(Math::Log2),
    reinterpret_cast<uintptr_t>(Math::Max),
    reinterpret_cast<uintptr_t>(Math::Min),
    reinterpret_cast<uintptr_t>(Math::Pow),
    reinterpret_cast<uintptr_t>(Math::Random),
    reinterpret_cast<uintptr_t>(Math::Round),
    reinterpret_cast<uintptr_t>(Math::Sign),
    reinterpret_cast<uintptr_t>(Math::Sin),
    reinterpret_cast<uintptr_t>(Math::Sinh),
    reinterpret_cast<uintptr_t>(Math::Sqrt),
    reinterpret_cast<uintptr_t>(Math::Tan),
    reinterpret_cast<uintptr_t>(Math::Tanh),
    reinterpret_cast<uintptr_t>(Math::Trunc),
    reinterpret_cast<uintptr_t>(Json::Parse),
    reinterpret_cast<uintptr_t>(Json::Stringify),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Next),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Return),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Throw),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSForInIterator::Next),
    reinterpret_cast<uintptr_t>(JSSetIterator::Next),
    reinterpret_cast<uintptr_t>(JSMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSArrayIterator::Next),
    reinterpret_cast<uintptr_t>(Proxy::ProxyConstructor),
    reinterpret_cast<uintptr_t>(Proxy::Revocable),
    reinterpret_cast<uintptr_t>(Reflect::ReflectApply),
    reinterpret_cast<uintptr_t>(Reflect::ReflectConstruct),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDefineProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDeleteProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetPrototypeOf),
    reinterpret_cast<uintptr_t>(Reflect::ReflectHas),
    reinterpret_cast<uintptr_t>(Reflect::ReflectIsExtensible),
    reinterpret_cast<uintptr_t>(Reflect::ReflectOwnKeys),
    reinterpret_cast<uintptr_t>(Reflect::ReflectPreventExtensions),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSetPrototypeOf),
    reinterpret_cast<uintptr_t>(AsyncFunction::AsyncFunctionConstructor),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeNext),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeReturn),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeThrow),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorFunctionConstructor),
    reinterpret_cast<uintptr_t>(Promise::PromiseConstructor),
    reinterpret_cast<uintptr_t>(Promise::All),
    reinterpret_cast<uintptr_t>(Promise::Race),
    reinterpret_cast<uintptr_t>(Promise::Resolve),
    reinterpret_cast<uintptr_t>(Promise::Reject),
    reinterpret_cast<uintptr_t>(Promise::Catch),
    reinterpret_cast<uintptr_t>(Promise::Then),
    reinterpret_cast<uintptr_t>(Promise::GetSpecies),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseReactionJob),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseResolveThenableJob),
    reinterpret_cast<uintptr_t>(Intl::GetCanonicalLocales),
    reinterpret_cast<uintptr_t>(Locale::LocaleConstructor),
    reinterpret_cast<uintptr_t>(Locale::Maximize),
    reinterpret_cast<uintptr_t>(Locale::Minimize),
    reinterpret_cast<uintptr_t>(Locale::ToString),
    reinterpret_cast<uintptr_t>(Locale::GetBaseName),
    reinterpret_cast<uintptr_t>(Locale::GetCalendar),
    reinterpret_cast<uintptr_t>(Locale::GetCaseFirst),
    reinterpret_cast<uintptr_t>(Locale::GetCollation),
    reinterpret_cast<uintptr_t>(Locale::GetHourCycle),
    reinterpret_cast<uintptr_t>(Locale::GetNumeric),
    reinterpret_cast<uintptr_t>(Locale::GetNumberingSystem),
    reinterpret_cast<uintptr_t>(Locale::GetLanguage),
    reinterpret_cast<uintptr_t>(Locale::GetScript),
    reinterpret_cast<uintptr_t>(Locale::GetRegion),
    reinterpret_cast<uintptr_t>(DateTimeFormat::DateTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(DateTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(DateTimeFormat::Format),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(DateTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRange),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRangeToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::NumberFormatConstructor),
    reinterpret_cast<uintptr_t>(NumberFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(NumberFormat::Format),
    reinterpret_cast<uintptr_t>(NumberFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(NumberFormat::NumberFormatInternalFormatNumber),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::RelativeTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::Format),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Collator::CollatorConstructor),
    reinterpret_cast<uintptr_t>(Collator::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(Collator::Compare),
    reinterpret_cast<uintptr_t>(Collator::ResolvedOptions),
    reinterpret_cast<uintptr_t>(PluralRules::PluralRulesConstructor),
    reinterpret_cast<uintptr_t>(PluralRules::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(PluralRules::Select),
    reinterpret_cast<uintptr_t>(PluralRules::ResolvedOptions),

    // non ECMA standard jsapi containers.
    reinterpret_cast<uintptr_t>(ContainersPrivate::Load),
    reinterpret_cast<uintptr_t>(ArrayList::ArrayListConstructor),
    reinterpret_cast<uintptr_t>(ArrayList::Add),
    reinterpret_cast<uintptr_t>(ArrayList::Insert),
    reinterpret_cast<uintptr_t>(ArrayList::Clear),
    reinterpret_cast<uintptr_t>(ArrayList::Clone),
    reinterpret_cast<uintptr_t>(ArrayList::Has),
    reinterpret_cast<uintptr_t>(ArrayList::GetCapacity),
    reinterpret_cast<uintptr_t>(ArrayList::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(ArrayList::TrimToCurrentLength),
    reinterpret_cast<uintptr_t>(ArrayList::GetIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::IsEmpty),
    reinterpret_cast<uintptr_t>(ArrayList::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByIndex),
    reinterpret_cast<uintptr_t>(ArrayList::Remove),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByRange),
    reinterpret_cast<uintptr_t>(ArrayList::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(ArrayList::SubArrayList),
    reinterpret_cast<uintptr_t>(ArrayList::ConvertToArray),
    reinterpret_cast<uintptr_t>(ArrayList::ForEach),
    reinterpret_cast<uintptr_t>(ArrayList::GetIteratorObj),
    reinterpret_cast<uintptr_t>(ArrayList::Get),
    reinterpret_cast<uintptr_t>(ArrayList::Set),
    reinterpret_cast<uintptr_t>(ArrayList::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIArrayListIterator::Next),
    reinterpret_cast<uintptr_t>(TreeMap::TreeMapConstructor),
    reinterpret_cast<uintptr_t>(TreeMap::Set),
    reinterpret_cast<uintptr_t>(TreeMap::Get),
    reinterpret_cast<uintptr_t>(TreeMap::Remove),
    reinterpret_cast<uintptr_t>(TreeMap::GetFirstKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLastKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLowerKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetHigherKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasValue),
    reinterpret_cast<uintptr_t>(TreeMap::SetAll),
    reinterpret_cast<uintptr_t>(TreeMap::Replace),
    reinterpret_cast<uintptr_t>(TreeMap::Keys),
    reinterpret_cast<uintptr_t>(TreeMap::Values),
    reinterpret_cast<uintptr_t>(TreeMap::Entries),
    reinterpret_cast<uintptr_t>(TreeMap::ForEach),
    reinterpret_cast<uintptr_t>(TreeMap::Clear),
    reinterpret_cast<uintptr_t>(TreeMap::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeMap::GetLength),
    reinterpret_cast<uintptr_t>(TreeSet::TreeSetConstructor),
    reinterpret_cast<uintptr_t>(TreeSet::Add),
    reinterpret_cast<uintptr_t>(TreeSet::Has),
    reinterpret_cast<uintptr_t>(TreeSet::Remove),
    reinterpret_cast<uintptr_t>(TreeSet::GetFirstValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLastValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLowerValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetHigherValue),
    reinterpret_cast<uintptr_t>(TreeSet::PopFirst),
    reinterpret_cast<uintptr_t>(TreeSet::PopLast),
    reinterpret_cast<uintptr_t>(TreeSet::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeSet::Values),
    reinterpret_cast<uintptr_t>(TreeSet::Entries),
    reinterpret_cast<uintptr_t>(TreeSet::ForEach),
    reinterpret_cast<uintptr_t>(TreeSet::Clear),
    reinterpret_cast<uintptr_t>(TreeSet::GetLength),
    reinterpret_cast<uintptr_t>(JSAPITreeMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSAPITreeSetIterator::Next),
    reinterpret_cast<uintptr_t>(Deque::DequeConstructor),
    reinterpret_cast<uintptr_t>(Deque::InsertFront),
    reinterpret_cast<uintptr_t>(Deque::InsertEnd),
    reinterpret_cast<uintptr_t>(Deque::GetFirst),
    reinterpret_cast<uintptr_t>(Deque::GetLast),
    reinterpret_cast<uintptr_t>(Deque::Has),
    reinterpret_cast<uintptr_t>(Deque::PopFirst),
    reinterpret_cast<uintptr_t>(Deque::PopLast),
    reinterpret_cast<uintptr_t>(Deque::ForEach),
    reinterpret_cast<uintptr_t>(Deque::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Deque::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIDequeIterator::Next),
    reinterpret_cast<uintptr_t>(Queue::QueueConstructor),
    reinterpret_cast<uintptr_t>(Queue::Add),
    reinterpret_cast<uintptr_t>(Queue::GetFirst),
    reinterpret_cast<uintptr_t>(Queue::Pop),
    reinterpret_cast<uintptr_t>(Queue::ForEach),
    reinterpret_cast<uintptr_t>(Queue::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Queue::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIQueueIterator::Next),
    reinterpret_cast<uintptr_t>(PlainArray::PlainArrayConstructor),
    reinterpret_cast<uintptr_t>(PlainArray::Add),
    reinterpret_cast<uintptr_t>(PlainArray::Clear),
    reinterpret_cast<uintptr_t>(PlainArray::Clone),
    reinterpret_cast<uintptr_t>(PlainArray::Has),
    reinterpret_cast<uintptr_t>(PlainArray::Get),
    reinterpret_cast<uintptr_t>(PlainArray::GetIteratorObj),
    reinterpret_cast<uintptr_t>(PlainArray::ForEach),
    reinterpret_cast<uintptr_t>(PlainArray::ToString),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfKey),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfValue),
    reinterpret_cast<uintptr_t>(PlainArray::IsEmpty),
    reinterpret_cast<uintptr_t>(PlainArray::GetKeyAt),
    reinterpret_cast<uintptr_t>(PlainArray::Remove),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveAt),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveRangeFrom),
    reinterpret_cast<uintptr_t>(PlainArray::SetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIPlainArrayIterator::Next),
    reinterpret_cast<uintptr_t>(ContainerStack::StackConstructor),
    reinterpret_cast<uintptr_t>(ContainerStack::Iterator),
    reinterpret_cast<uintptr_t>(ContainerStack::IsEmpty),
    reinterpret_cast<uintptr_t>(ContainerStack::Push),
    reinterpret_cast<uintptr_t>(ContainerStack::Peek),
    reinterpret_cast<uintptr_t>(ContainerStack::Pop),
    reinterpret_cast<uintptr_t>(ContainerStack::Locate),
    reinterpret_cast<uintptr_t>(ContainerStack::ForEach),
    reinterpret_cast<uintptr_t>(ContainerStack::GetLength),
    reinterpret_cast<uintptr_t>(JSAPIStackIterator::Next),

    // not builtins method
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeSetter),
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeGetter),
    reinterpret_cast<uintptr_t>(JSFunction::NameGetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthSetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthGetter),
    reinterpret_cast<uintptr_t>(JSPandaFileManager::RemoveJSPandaFile),
    reinterpret_cast<uintptr_t>(JSPandaFileManager::GetInstance)
};

void SnapShotSerialize::SetObjectEncodeField(uintptr_t obj, size_t offset, uint64_t value)
{
    *reinterpret_cast<uint64_t *>(obj + offset) = value;
}

void SnapShotSerialize::DeserializeString(uintptr_t stringBegin, uintptr_t stringEnd)
{
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    ASSERT(stringVector_.empty());
    while (stringBegin < stringEnd) {
        EcmaString *str = reinterpret_cast<EcmaString *>(stringBegin);
        size_t strSize = str->ObjectSize();
        strSize = AlignUp(strSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        auto strFromTable = stringTable->GetString(str);

        if (strFromTable) {
            stringVector_.emplace_back(ToUintPtr(strFromTable));
        } else {
            uintptr_t snapshotObj = const_cast<Heap *>(vm_->GetHeap())->AllocateSnapShotSpace(strSize);
            if (snapshotObj == 0) {
                LOG_ECMA_MEM(FATAL) << "SnapShotAllocator OOM";
            }
            if (memcpy_s(ToVoidPtr(snapshotObj), strSize, str, strSize) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            str = reinterpret_cast<EcmaString *>(snapshotObj);
            str->ClearInternStringFlag();
            stringTable->GetOrInternString(str);
            stringVector_.emplace_back(snapshotObj);
        }
        stringBegin += strSize;
    }
}

void SnapShotSerialize::DeserializeHandleRootObject(SnapShotType type, uintptr_t rootObjectAddr,
                                                    size_t objType, size_t objIndex)
{
    switch (type) {
        case SnapShotType::VM_ROOT:
            if (JSType(objType) == JSType::GLOBAL_ENV) {
                vm_->SetGlobalEnv(reinterpret_cast<GlobalEnv *>(rootObjectAddr));
            } else if (JSType(objType) == JSType::MICRO_JOB_QUEUE) {
                vm_->SetMicroJobQueue(reinterpret_cast<job::MicroJobQueue *>(rootObjectAddr));
            }
            break;
        case SnapShotType::GLOBAL_CONST: {
            JSTaggedValue result(rootObjectAddr);
            auto constants = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
            constants->SetConstant(ConstantIndex(objIndex), result);
            break;
        }
        default:
            break;
    }
}

void SnapShotSerialize::SerializeObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                        std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> *data)
{
    auto hclass = objectHeader->GetClass();
    JSType objectType = hclass->GetObjectType();
    if (objectType== JSType::STRING) {
        EncodeBit encodeBit = HandleObjectHeader(objectHeader, static_cast<size_t>(objectType), queue, data);
        SetObjectEncodeField(ToUintPtr(objectHeader), 0, encodeBit.GetValue());
        return;
    }
    uintptr_t snapshotObj;
    if (UNLIKELY(data->find(ToUintPtr(objectHeader)) == data->end())) {
        LOG_ECMA(FATAL) << "Data map can not find object";
        UNREACHABLE();
    } else {
        snapshotObj = data->find(ToUintPtr(objectHeader))->second.first;
    }

    // header
    EncodeBit encodeBit = HandleObjectHeader(objectHeader, static_cast<size_t>(objectType), queue, data);
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());

    auto visitor = [this, snapshotObj, queue, data](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                    bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            if (isNative) {
                auto nativePointer = *reinterpret_cast<void **>(slot.SlotAddress());
                SetObjectEncodeField(snapshotObj, slot.SlotAddress() - ToUintPtr(root),
                                     NativePointerToEncodeBit(nativePointer).GetValue());
            } else {
                auto fieldAddr = reinterpret_cast<JSTaggedType *>(slot.SlotAddress());
                SetObjectEncodeField(snapshotObj, slot.SlotAddress() - ToUintPtr(root),
                                     HandleTaggedField(fieldAddr, queue, data));
            }
        }
    };

    objXRay_.VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

void SnapShotSerialize::Relocate(SnapShotType type, const JSPandaFile *jsPandaFile, uint64_t rootObjSize)
{
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    size_t methodNums = 0;
    JSMethod *methods = nullptr;
    if (jsPandaFile) {
        methodNums = jsPandaFile->GetNumMethods();
        methods = jsPandaFile->GetMethods();
    }

    size_t others = 0;
    size_t objIndex = 0;
    space->EnumerateRegions([&others, &objIndex, &rootObjSize, &type, this, methods, &methodNums](Region *current) {
        size_t allocated = current->GetAllocatedBytes();
        uintptr_t begin = current->GetBegin();
        uintptr_t end = begin + allocated;
        while (begin < end) {
            if (others != 0) {
                for (size_t i = 0; i < others; i++) {
                    pandaMethod_.emplace_back(begin);
                    auto method = reinterpret_cast<JSMethod *>(begin);
                    method->SetBytecodeArray(method->GetBytecodeArray());
                    if (memcpy_s(methods + (--methodNums), METHOD_SIZE, method, METHOD_SIZE) != EOK) {
                        LOG_ECMA(FATAL) << "memcpy_s failed";
                        UNREACHABLE();
                    }
                    begin += METHOD_SIZE;
                    if (begin >= end) {
                        others = others - i - 1;
                    }
                }
                break;
            }
            EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(begin));
            auto objType = encodeBit.GetObjectType();
            if (objType == Constants::MASK_METHOD_SPACE_BEGIN) {
                begin += sizeof(uint64_t);
                for (size_t i = 0; i < encodeBit.GetNativeOrGlobalIndex(); i++) {
                    pandaMethod_.emplace_back(begin);
                    auto method = reinterpret_cast<JSMethod *>(begin);
                    method->SetBytecodeArray(method->GetBytecodeArray());
                    if (memcpy_s(methods + (--methodNums), METHOD_SIZE, method, METHOD_SIZE) != EOK) {
                        LOG_ECMA(FATAL) << "memcpy_s failed";
                        UNREACHABLE();
                    }
                    begin += METHOD_SIZE;
                    if (begin >= end) {
                        others = encodeBit.GetNativeOrGlobalIndex() - i - 1;
                        break;
                    }
                }
                break;
            }
            TaggedObject *objectHeader = reinterpret_cast<TaggedObject *>(begin);
            DeserializeHandleClassWord(objectHeader);
            DeserializeHandleField(objectHeader);
            if (objIndex < rootObjSize) {
                DeserializeHandleRootObject(type, begin, objType, objIndex);
            }
            begin = begin + AlignUp(objectHeader->GetClass()->SizeFromJSHClass(objectHeader),
                                    static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            objIndex++;
        }
    });
}

EncodeBit SnapShotSerialize::HandleObjectHeader(TaggedObject *objectHeader, size_t objectType,
                                                CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> *data)
{
    auto *hclass = objectHeader->GetClass();
    EncodeBit encodeBit(0);
    ASSERT(hclass != nullptr);
    size_t hclassIndex = vm_->GetSnapShotEnv()->GetEnvObjectIndex(ToUintPtr(hclass));
    if (hclassIndex != SnapShotEnv::MAX_UINT_32) {
        encodeBit.SetGlobalEnvConst();
        encodeBit.SetNativeOrGlobalIndex(hclassIndex);
        encodeBit.SetObjectType(objectType);
        return encodeBit;
    }
    if (data->find(ToUintPtr(hclass)) == data->end()) {
        encodeBit = EncodeTaggedObject(hclass, queue, data);
    } else {
        std::pair<uint64_t, EncodeBit> valuePair = data->find(ToUintPtr(hclass))->second;
        encodeBit = valuePair.second;
    }
    encodeBit.SetObjectType(objectType);
    return encodeBit;
}

uint64_t SnapShotSerialize::HandleTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                                              std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> *data)
{
    JSTaggedValue taggedValue(*tagged);
    if (taggedValue.IsWeak()) {
        EncodeBit special(JSTaggedValue::Undefined().GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();
    }

    if (taggedValue.IsSpecial()) {
        EncodeBit special(taggedValue.GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();  // special encode bit
    }

    if (!taggedValue.IsHeapObject()) {
        return taggedValue.GetRawData();  // not object
    }

    EncodeBit encodeBit(0);
    size_t globalEnvIndex = vm_->GetSnapShotEnv()->GetEnvObjectIndex(ToUintPtr(taggedValue.GetTaggedObject()));
    if (globalEnvIndex != SnapShotEnv::MAX_UINT_32) {
        encodeBit.SetGlobalEnvConst();
        encodeBit.SetNativeOrGlobalIndex(globalEnvIndex);
        return encodeBit.GetValue();
    }
    if (data->find(*tagged) == data->end()) {
        encodeBit = EncodeTaggedObject(taggedValue.GetTaggedObject(), queue, data);
    } else {
        std::pair<uint64_t, EncodeBit> valuePair = data->find(taggedValue.GetRawData())->second;
        encodeBit = valuePair.second;
    }

    if (taggedValue.IsString()) {
        encodeBit.SetReferenceToString(true);
    }
    return encodeBit.GetValue();  // object
}

void SnapShotSerialize::DeserializeHandleTaggedField(uint64_t *value)
{
    EncodeBit encodeBit(*value);
    if (encodeBit.IsGlobalEnvConst()) {
        size_t index = encodeBit.GetNativeOrGlobalIndex();
        auto globalEnv = vm_->GetGlobalEnv();
        auto globalEnvObjectValue = globalEnv->GetGlobalEnvObjectByIndex(index);
        *value = ToUintPtr(globalEnvObjectValue->GetTaggedObject());
        return;
    }
    if (encodeBit.IsReference() && !encodeBit.IsSpecial()) {
        uintptr_t taggedObjectAddr = TaggedObjectEncodeBitToAddr(encodeBit);
        *value = taggedObjectAddr;
        return;
    }

    if (encodeBit.IsSpecial()) {
        encodeBit.ClearObjectSpecialFlag();
        *value = encodeBit.GetValue();
    }
}

void SnapShotSerialize::DeserializeHandleClassWord(TaggedObject *object)
{
    EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(object));
    if (encodeBit.IsGlobalEnvConst()) {
        size_t hclassIndex = encodeBit.GetNativeOrGlobalIndex();
        auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
        JSTaggedValue hclassValue = globalConst->GetGlobalConstantObject(hclassIndex);
        ASSERT(hclassValue.IsJSHClass());
        object->SetClass(JSHClass::Cast(hclassValue.GetTaggedObject()));
        return;
    }
    uintptr_t hclassAddr = TaggedObjectEncodeBitToAddr(encodeBit);
    object->SetClass(reinterpret_cast<JSHClass *>(hclassAddr));
}

void SnapShotSerialize::DeserializeHandleField(TaggedObject *objectHeader)
{
    auto visitor = [this]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end, bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto encodeBitAddr = reinterpret_cast<uint64_t *>(slot.SlotAddress());
            if (isNative) {
                DeserializeHandleNativePointer(encodeBitAddr);
            } else {
                DeserializeHandleTaggedField(encodeBitAddr);
            }
        }
    };

    objXRay_.VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

EncodeBit SnapShotSerialize::NativePointerToEncodeBit(void *nativePointer)
{
    EncodeBit native(0);
    if (nativePointer != nullptr) {  // nativePointer
        size_t index = Constants::MAX_C_POINTER_INDEX;

        if (programSerialize_) {
            pandaMethod_.emplace_back(ToUintPtr(nativePointer));
            ASSERT(pandaMethod_.size() + GetNativeTableSize() <= Constants::MAX_UINT_16);
            // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
            index = pandaMethod_.size() + GetNativeTableSize() - 1;
        } else {
            index = SearchNativeMethodIndex(nativePointer);
        }

        LOG_IF(index > Constants::MAX_C_POINTER_INDEX, FATAL, RUNTIME) << "MAX_C_POINTER_INDEX: " + ToCString(index);
        native.SetNativeOrGlobalIndex(index);
    }
    return native;
}

void *SnapShotSerialize::NativePointerEncodeBitToAddr(EncodeBit nativeBit)
{
    size_t index = nativeBit.GetNativeOrGlobalIndex();
    void *addr = nullptr;
    size_t nativeTableSize = GetNativeTableSize();

    if (index < nativeTableSize - Constants::PROGRAM_NATIVE_METHOD_BEGIN) {
        addr = reinterpret_cast<void *>(vm_->nativeMethods_.at(index));
    } else if (index < nativeTableSize) {
        addr = reinterpret_cast<void *>(g_nativeTable[index]);
    } else {
        addr = ToVoidPtr(pandaMethod_.at(index - nativeTableSize));
    }
    return addr;
}

size_t SnapShotSerialize::SearchNativeMethodIndex(void *nativePointer)
{
    size_t nativeMethodSize = GetNativeTableSize() - Constants::PROGRAM_NATIVE_METHOD_BEGIN;
    for (size_t i = 0; i < Constants::PROGRAM_NATIVE_METHOD_BEGIN; i++) {
        if (nativePointer == reinterpret_cast<void *>(g_nativeTable[i + nativeMethodSize])) {
            return i + nativeMethodSize;
        }
    }

    // not found
    auto nativeMethod = reinterpret_cast<JSMethod *>(nativePointer)->GetNativePointer();
    for (size_t i = 0; i < nativeMethodSize; i++) {
        if (nativeMethod == reinterpret_cast<void *>(g_nativeTable[i])) {
            return i;
        }
    }
    return Constants::MAX_C_POINTER_INDEX;
}

uintptr_t SnapShotSerialize::TaggedObjectEncodeBitToAddr(EncodeBit taggedBit)
{
    ASSERT(taggedBit.IsReference());
    if (taggedBit.IsReferenceToString()) {
        size_t stringIndex = taggedBit.GetStringIndex();
        return stringVector_[stringIndex];
    }
    size_t regionIndex = taggedBit.GetRegionIndex();
    size_t objectOffset  = taggedBit.GetObjectOffsetInRegion();
    auto snapShotSpace = const_cast<Heap *>(vm_->GetHeap())->GetSnapShotSpace();
    size_t defaultSnapshotSpaceCapacity = vm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    return ToUintPtr(snapShotSpace->GetFirstRegion()) + regionIndex * defaultSnapshotSpaceCapacity + objectOffset;
}

void SnapShotSerialize::DeserializeHandleNativePointer(uint64_t *value)
{
    EncodeBit native(*value);
    size_t index = native.GetNativeOrGlobalIndex();
    uintptr_t addr = 0U;
    size_t nativeTableSize = GetNativeTableSize();

    if (index == Constants::MAX_C_POINTER_INDEX) {
        return;
    }
    if (index < nativeTableSize - Constants::PROGRAM_NATIVE_METHOD_BEGIN) {
        addr = reinterpret_cast<uintptr_t>(vm_->nativeMethods_.at(index));
    } else if (index < nativeTableSize) {
        addr = g_nativeTable[index];
    } else {
        addr = pandaMethod_.at(index - nativeTableSize);
    }
    *value = addr;
}

void SnapShotSerialize::SerializePandaFileMethod()
{
    EncodeBit encodeBit(0);
    encodeBit.SetObjectType(Constants::MASK_METHOD_SPACE_BEGIN);
    encodeBit.SetNativeOrGlobalIndex(pandaMethod_.size());

    ObjectFactory *factory = vm_->GetFactory();
    // panda method space begin
    uintptr_t snapshotObj = factory->NewSpaceBySnapShotAllocator(sizeof(uint64_t));
    if (snapshotObj == 0) {
        LOG(ERROR, RUNTIME) << "SnapShotAllocator OOM";
        return;
    }
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());  // methods

    // panda methods
    for (auto &it : pandaMethod_) {
        // write method
        size_t methodObjSize = METHOD_SIZE;
        uintptr_t methodObj = factory->NewSpaceBySnapShotAllocator(methodObjSize);
        if (methodObj == 0) {
            LOG(ERROR, RUNTIME) << "SnapShotAllocator OOM";
            return;
        }
        if (memcpy_s(ToVoidPtr(methodObj), methodObjSize, ToVoidPtr(it), METHOD_SIZE) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

EncodeBit SnapShotSerialize::EncodeTaggedObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t,
                                                                   std::pair<uint64_t, ecmascript::EncodeBit>> *data)
{
    queue->emplace(objectHeader);
    if (objectHeader->GetClass()->GetObjectType() == JSType::STRING) {
        ASSERT(stringVector_.size() < Constants::MAX_STRING_SIZE);
        EncodeBit encodeBit(stringVector_.size());
        stringVector_.emplace_back(ToUintPtr(objectHeader));
        data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
        return encodeBit;
    }
    size_t objectSize = objectHeader->GetClass()->SizeFromJSHClass(objectHeader);
    if (objectSize > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        LOG_ECMA_MEM(FATAL) << "It is a huge object. Not Support.";
    }

    if (objectSize == 0) {
        LOG_ECMA_MEM(FATAL) << "It is a zero object. Not Support.";
    }
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    uintptr_t snapshotObj = heap->AllocateSnapShotSpace(objectSize);
    if (snapshotObj == 0) {
        LOG_ECMA_MEM(FATAL) << "SnapShotAllocator OOM";
    }
    if (memcpy_s(ToVoidPtr(snapshotObj), objectSize, objectHeader, objectSize) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    auto snapShotSpace = heap->GetSnapShotSpace();
    size_t regionIndex = snapShotSpace->GetRegionCount() - 1;
    size_t objOffset = snapshotObj - ToUintPtr(snapShotSpace->GetCurrentRegion());
    EncodeBit encodeBit(static_cast<uint64_t>(regionIndex));
    encodeBit.SetObjectOffsetInRegion(objOffset);
    data->emplace(ToUintPtr(objectHeader), std::make_pair(snapshotObj, encodeBit));
    return encodeBit;
}

void SnapShotSerialize::EncodeTaggedObjectRange(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t,
                                                                   std::pair<uint64_t, ecmascript::EncodeBit>> *data)
{
    while (start < end) {
        JSTaggedValue object(start.GetTaggedType());
        start++;
        if (object.IsHeapObject()) {
            EncodeBit encodeBit(0);
            if (data->find(object.GetRawData()) == data->end()) {
                encodeBit = EncodeTaggedObject(object.GetTaggedObject(), queue, data);
            }
        }
    }
}

void SnapShotSerialize::GeneratedNativeMethod()  // NOLINT(readability-function-size)
{
    size_t nativeMethodSize = GetNativeTableSize() - Constants::PROGRAM_NATIVE_METHOD_BEGIN;
    for (size_t i = 0; i < nativeMethodSize; i++) {
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(g_nativeTable[i]));
    }
}

size_t SnapShotSerialize::GetNativeTableSize() const
{
    return sizeof(g_nativeTable) / sizeof(g_nativeTable[0]);
}
}  // namespace panda::ecmascript

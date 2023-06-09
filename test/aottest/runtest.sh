#!/bin/bash
# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

expect_output="expect_output.txt"
run_output="run_output.txt"
build_output="build_output.txt"
test_name=""
test_dir=$(dirname $0)
out_dir="out/hi3516dv300/clang_x64/aottest"
test_all_mode="no"
run_args=""
oat_args=""
run_mode="aot"
cur_dir=$(pwd)
test_timeout=120

check_result_fexit()
{
    if [ $? -ne 0 ]; then
        echo -e "\033[31;2m[error] $1\033[0m"
        exit 1
    elif [ -n "$2" ]; then
        echo -e "$2"
    fi
}

check_failed_exit()
{
    ret1=$(grep 'FAILED' $1)
    ret2=$(grep 'failed' $1)
    ret3=$(grep 'FAULT' $1)
    ret4=$(grep 'fault' $1)
    if [ -n "$ret1" -o -n "$ret2" -o -n "$ret3" -o -n "$ret4" ]; then
        echo -e "\033[31;2m[error] $2\033[0m"
        exit 1
    fi
}

echo_pass()
{
    echo -e "\033[32;2m$1\033[0m"
}

echo_fail()
{
    echo -e "\033[31;2m[error] $1\033[0m"
}

run_check()
{
    timeout $@
    # FIXME: run result can not be checked by $? when run unexpectedly exit, such as segmentation fault
    ret=$?
    if [ $ret -eq 124 -o $ret -eq 142 ]; then
        echo_fail "Run timeout, be killed!"
        exit 1
    elif [ $ret -ne 0 ]; then
        echo_fail "Run FAILED!"
        exit 1
    fi
}

usage()
{
    echo -e "Usage: runtest.sh [options] test_name
    Options:
    -mode:aot       run on aot mode, default
    -mode:int       run on interpret mode
    -mode:asmint    run on asm interpret mode
    -debug          run on debug mode
    -timeout n      specify seconds of test timeout, n > 0
    -v              show version
    -h              print this usage statement"
}

while [ $# -gt 0 ]
do
    case $1 in
        -mode:aot)
            run_mode="aot"
            shift 1 ;;
        -mode:asmint)
            run_mode="asmint"
            shift 1 ;;
        -mode:int)
            run_mode="int"
            shift 1 ;;
        -debug)
            run_args="$run_args debug=yes"
            shift 1 ;;
        -v)
            tail -n +14 $test_dir/version
            exit 0 ;;
        -h)
            usage
            exit 0 ;;
        -*)
            echo "invalid option $1"
            exit 0 ;;
        *)
            test_name="$1"
            shift 1 ;;
    esac
done

if [ ! -d "$cur_dir/ark" ]; then
    echo "Please run at openharmony root dir that ark located"
    exit 0
fi

# run test
test_name=$(basename $test_name)
echo "Run test: $test_dir/$test_name ================="

if [ ! -f "$out_dir/com_stub.m" ]; then
    make -n -f $test_dir/makefile $run_args stub
    run_check $test_timeout make -s -f $test_dir/makefile $run_args stub
fi

make_cmd="make -f $test_dir/makefile $run_args test=$test_name"
$make_cmd -n abc
run_check $test_timeout $make_cmd -s abc

case "$run_mode" in
    "aot")
        $make_cmd -n aot
        run_check $test_timeout $make_cmd -s aot
        $make_cmd -n run
        run_check $test_timeout $make_cmd -s run > $out_dir/$test_name/$run_output
        ;;
    "int")
        $make_cmd -n int
        run_check $test_timeout $make_cmd -s int > $out_dir/$test_name/$run_output
        ;;
    "asmint")
        $make_cmd -n asmint
        run_check $test_timeout $make_cmd -s asmint > $out_dir/$test_name/$run_output
        ;;
esac

tail -n +14 $test_dir/$test_name/$expect_output > $out_dir/$test_name/$expect_output
diff $out_dir/$test_name/$run_output $out_dir/$test_name/$expect_output

check_result_fexit "Test Case FAILED!"
echo_pass "======================= Test Case PASSED!"
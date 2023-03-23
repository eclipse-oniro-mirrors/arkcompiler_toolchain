#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2022 Huawei Device Co., Ltd.
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
#

from __future__ import print_function
from datetime import datetime
import errno
import os
import subprocess
import sys

ARCHES = ["x64", "arm", "arm64"]
DEFAULT_ARCH = "x64"
MODES = ["release", "debug"]
DEFAULT_MODE = "release"
TARGETS = ["ets_runtime", "ets_frontend", "runtime_core", "default", "mingw_packages"]
DEFAULT_TARGET = "default"
TARGETS_TEST = ["test262", "unittest"]


USER_ARGS_TEMPLATE = """\
%s
"""

OUTDIR = "out"


Help_message = """
format: python ark.py [arch].[mode] [options] [test] [test target]
for example , python ark.py x64.release
[arch] only support "x64" now
[mode] can be one of ["release", "debug"]
[options]
  target: support [ets_runtime | ets_frontend | runtime_core | default | mingw_packages] now
  clean: clear your data in output dir
[test] only support run on x64 platform now
  test262: run test262
  unittest: run unittest
[test target]
  when [test] is test262: means test file or test dir, like "built-ins/Array/name.js" or "built-ins/Array"
  when [test] is unittest: means test target action defined in gn, like "DebuggerTestAction" or "addAotAction"
"""

def PrintHelp():
    print(Help_message)
    sys.exit(0)


def _call(cmd):
    print("# %s" % cmd)
    return subprocess.call(cmd, shell=True)


def _write(filename, content, mode):
    with open(filename, mode) as f:
        f.write(content)


def GetPath(arch, mode):
    subdir = "%s.%s" % (arch, mode)
    return os.path.join(OUTDIR, subdir)


def _callWithOutput(cmd, file):
    print("# %s" % cmd)
    host = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE)
    while True:
        try:
            build_data = host.stdout.readline().decode('utf-8')
            sys.stdout.flush()
            print(build_data)
            _write(file,build_data, "a")
        except OSError as error:
            if error == errno.ENOENT:
                print("no such file")
            elif error == errno.EPERM:
                print("permission denied")
            break
        if not build_data:
            break
    host.wait()
    return host.returncode


def Get_args(argvs):
    args_list = argvs
    args_len = len(args_list)
    if args_len < 1:
        print("Wrong usage")
        PrintHelp()
    elif args_len == 1:
        args_out = args_list
        if "--help" in args_out:
            PrintHelp()
    else :
        args_out = args_list
    return Get_template(args_out)

def Get_time():
    return datetime.now()

def Get_template(args_list):
    global_arch = DEFAULT_ARCH
    global_mode = DEFAULT_MODE
    global_target = DEFAULT_TARGET
    global_test = ''
    global_clean = False
    test_target = ''
    for args in args_list:
        if global_test != '':
            # only test has extra args
            test_target = args
        parameter = args.split(".")
        for part in parameter:
            if part in ARCHES:
                global_arch = part
            elif part in MODES:
                global_mode = part
            elif part in TARGETS:
                global_target = part
            elif part == "clean":
                global_clean = True
            elif part in TARGETS_TEST:
                global_test = part
            elif global_test == '':
                print("\033[34mIllegal command line option: %s\033[0m" % part)
                PrintHelp()
# Determine the target CPU
    target_cpu = "target_cpu = \"%s\"" % global_arch
# Determine the target CPU
    if global_arch in ("arm", "arm64"):
        ark_os = "ohos"
    else:
        ark_os = "linux"
    target_os = "target_os = \"%s\"" % ark_os
    if global_mode == "debug":
        is_debug = "is_debug = true"
    else:
        is_debug = "is_debug = false"
    all_part = (is_debug + "\n" + target_os + "\n" + target_cpu + "\n")
    return [global_arch, global_mode, global_target, global_clean,
            USER_ARGS_TEMPLATE % (all_part), global_test, test_target]


def Build(template):
    arch = template[0]
    mode = template[1]
    target = template[2]
    clean = template[3]
    template_part = template[4]
    path = GetPath(arch, mode)
    if not os.path.exists(path):
        print("# mkdir -p %s" % path)
        os.makedirs(path)
    if clean:
        print("=== start clean ===")
        code = _call("./prebuilts/build-tools/linux-x86/bin/gn clean %s" % path)
        code += _call("./prebuilts/build-tools/linux-x86/bin/ninja -C %s -t clean" % path)
        if code != 0:
            return code
        print("=== clean success! ===")
        exit(0)
    build_log = os.path.join(path, "build.log")
    if not os.path.exists("args.gn"):
        args_gn = os.path.join(path, "args.gn")
        _write(args_gn, template_part, "w")
        _write(build_log, "\nbuild_time:{}\nbuild_target:{}\n".format(Get_time().replace(microsecond=0), target), "a")
    if not os.path.exists("build.ninja"):
        build_ninja = os.path.join(path, "build.ninja")
        code = _callWithOutput("./prebuilts/build-tools/linux-x86/bin/gn gen %s" % path, build_log)
        if code != 0:
            return code
        else:
            print("=== gn success! ===")
    pass_code = _callWithOutput("./prebuilts/build-tools/linux-x86/bin/ninja -C %s %s" %
                                          (path, target), build_log)
    if pass_code == 0:
        print("=== ninja success! ===")
    return pass_code


def RunTest(template):
    arch = template[0]
    mode = template[1]
    test = template[5]
    test_target = template[6]
    path = GetPath(arch, mode)
    test_dir = arch + "." + mode
    test_log = os.path.join(path, "test.log")
    if ("test262" == test):
        print("=== come to test262 ===")
        target = "all"
        if test_target != '':
            raw_target = "arkcompiler/ets_frontend/test262/data/test/" + test_target
            target = "test262/data/test_es2021/" + test_target
            if os.path.isdir(raw_target):
                target = "--dir " + target
            elif os.path.isfile(raw_target):
                target = "--file " + target
            else:
                print("Can't find %s in arkcompiler/ets_frontend/test262/data/test/" % (test_target))
                return -1

        test262_code = '''cd arkcompiler/ets_frontend
        python3 test262/run_test262.py --es2021 %s --timeout 180000 --libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib --ark-tool=../../out/%s/clang_x64/arkcompiler/ets_runtime/ark_js_vm --ark-frontend-binary=../../out/%s/clang_x64/arkcompiler/ets_frontend/es2abc --merge-abc-binary=../../out/%s/clang_x64/arkcompiler/ets_frontend/merge_abc --ark-frontend=es2panda
        ''' % (target, test_dir, test_dir, test_dir)
        _write(test_log, "\ntest_time:{}\ntest_target:{}\n".format(Get_time().replace(microsecond=0), target), "a")
        pass_code =_callWithOutput(test262_code, test_log)
        if pass_code == 0:
            print("=== test262 success! ===")
        else:
            print("=== test262 fail! ===")
        return pass_code
    elif ("unittest" == test):
        print("=== come to unittest ===")
        if test_target == '':
            test_target = "unittest_packages"
        unittest_code = "./prebuilts/build-tools/linux-x86/bin/ninja -C %s %s" % (path, test_target)
        _write(test_log, "\ntest_time:{}\ntest_target:{}\n".format(Get_time().replace(microsecond=0), test_target), "a")
        pass_code =_callWithOutput(unittest_code, test_log)
        if pass_code == 0:
            print("=== unittest success! ===")
        else:
            print("=== unittest fail! ===")
        return pass_code
    else:
        print("=== nothing to test ===")
        return 0


def Main(argvs):
    pass_code = 0
    template = Get_args(argvs)
    pass_code += Build(template)
    if pass_code == 0:
        pass_code += RunTest(template)
    if pass_code == 0:
        print('\033[32mDone!\033[0m', '\033[32m{} compilation finished successfully.\033[0m'.format(argvs[0]))
    else:
        print('\033[31mError!\033[0m', '\033[31m{} compilation finished with errors.\033[0m'.format(argvs[0]))
    return pass_code


if __name__ == "__main__":
    sys.exit(Main(sys.argv[1:]))

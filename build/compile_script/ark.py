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
import errno
import os
import subprocess
import sys

USE_PTY = "linux" in sys.platform
if USE_PTY:
    import pty

ARCHES = ["x64", "arm", "arm64"]
DEFAULT_ARCHES = "x64"
MODES = ["release", "debug"]
DEFAULT_MODES = "release"
TARGETS = ["ets_runtime", "ets_frontend", "default", "all"]
DEFAULT_TARGETS = "default"
TARGETS_TEST = ["-test262"]


USER_ARGS_TEMPLATE = """\
is_standard_system = true
%s
"""

OUTDIR = "out"


Help_message = """
formot like python ark.py [arch].[mode] [options] [test]
for example , python ark.py x64.release
[arch] can be one of ["x64", "arm", "arm64"]
[mode] can be one of ["release", "debug"]
[options]
  target: only support [ets_runtime | ets_frontend | default | all] now
  clean: clear your data in output dir
[test]
  test262: run test262
"""

def PrintHelp():
    print(Help_message)
    sys.exit(0)


def _Call(cmd, silent=False):
    if not silent:
        print("# %s" % cmd)
    return subprocess.call(cmd, shell=True)


def _Write(filename, content, mode):
    with open(filename, mode) as f:
        f.write(content)


def get_path(arch, mode):
    subdir = "%s.%s" % (arch, mode)
    return os.path.join(OUTDIR, subdir)


def call_with_output(cmd, file):
    host, guest = pty.openpty()
    h = subprocess.Popen(cmd, shell=True, stdin=guest, stdout=guest, stderr=guest)
    os.close(guest)
    output_data = []
    while True:
        try:
            build_data = os.read(host, 512).decode('utf-8')
        except OSError as error:
            if error == errno.ENOENT:
                print("no such file")
            elif error == errno.EPERM:
                print("permission denied")
            break
        else:
            if not build_data:
                break
            print(build_data)
            sys.stdout.flush()
            _Write(file, build_data, "a")
    os.close(host)
    h.wait()
    return h.returncode


def get_args(argvs):
    args_list = argvs
    args_len = len(args_list)
    if args_len < 1:
        print("Wrong usage")
        PrintHelp()
    elif args_len == 1:
        args_out = args_list
        if "-help" in args_out:
            PrintHelp()
    else :
        args_out = args_list
    return get_templete(args_out)


def get_templete(args_list):
    global_arche = DEFAULT_ARCHES
    global_mode = DEFAULT_MODES
    global_target = DEFAULT_TARGETS
    global_test = ''
    global_clean = False
    for args in args_list:
        parameter = args.split(".")
        for part in parameter:
            if part in ARCHES:
                global_arche = part
            elif part in MODES:
                global_mode = part
            elif part in TARGETS:
                global_target = part
            elif part == "clean":
                global_clean = True
            elif part in TARGETS_TEST:
                global_test = part
            else:
                print("\033[34mUnkown word: %s\033[0m" % part)
                PrintHelp()
                sys.exit(1)
# Determine the target CPU
    if global_arche in ("arm", "arm64"):
        ark_cpu = global_arche
    else:
        ark_cpu = "x64"
    target_cpu = "target_cpu = \"%s\"" % ark_cpu
# Determine the target OS,Only ohos for now
    ark_os = "ohos"
    target_os = "target_os = \"%s\"" % ark_os
    if global_mode == "debug":
        is_debug = "is_debug = true"
    else:
        is_debug = "is_debug = false"
    all_part = (is_debug + "\n" + target_os + "\n" + target_cpu)
    return [global_arche, global_mode, global_target, global_clean, USER_ARGS_TEMPLATE % (all_part), global_test]


def Build(template):
    arch = template[0]
    mode = template[1]
    target = template[2]
    clean = template[3]
    template_part = template[4]
    path = get_path(arch, mode)
    if not os.path.exists(path):
        print("# mkdir -p %s" % path)
        os.makedirs(path)
    if clean:
        print("=== start clean ===")
        code = _Call("./prebuilts/build-tools/linux-x86/bin/gn clean %s" % path)
        code += _Call("./prebuilts/build-tools/linux-x86/bin/ninja -C %s -t clean" % path)
        if code != 0:
            return code
        print("=== clean success! ===")
        exit(0)
    build_log = os.path.join(path, "build.log")
    if not os.path.exists("args.gn"):
        args_gn = os.path.join(path, "args.gn")
        _Write(args_gn, template_part, "w")
    if not os.path.exists("build.ninja"):
        build_ninja = os.path.join(path, "build.ninja")
        code = _Call("./prebuilts/build-tools/linux-x86/bin/gn gen %s" % path)
        print("=== gn success! ===")
        if code != 0:
            return code
    pass_code = call_with_output("./prebuilts/build-tools/linux-x86/bin/ninja -C %s %s" %
                                          (path, target), build_log)
    if pass_code == 0:
        print("=== ninja success! ===")
    return pass_code


def run_test(template):
    arch = template[0]
    mode = template[1]
    test = template[5]
    test_dir = arch + "." + mode
    test262_code = '''cd ets_frontend
    python3 test262/run_test262.py --es2021 all --timeout 180000 --libs-dir ../out/%s:../prebuilts/clang/ohos/linux-x86_64/llvm/lib --ark-tool=../out/%s/arkcompiler/ets_runtime/ark_js_vm --ark-frontend-binary=../out/%s/clang_x64/arkcompiler/ets_frontend/es2abc --merge-abc-binary=../out/%s/clang_x64/arkcompiler/ets_frontend/merge_abc --ark-frontend=es2panda
    ''' % (test_dir, test_dir, test_dir, test_dir)
    if ("-test262" == test):
        print("=== come to test ===")
        return _Call(test262_code)
    else:
        print("=== nothing to test ===")
        return 0


def Main(argvs):
    pass_code = 0
    templete = get_args(argvs)
    pass_code += Build(templete)
    if pass_code == 0:
        pass_code += run_test(templete)
    if pass_code == 0:
        print('\033[32mDone!\033[0m', '\033[32mARK_{} compilation finished successfully.\033[0m'.format(argvs[0].split('.')[0]))
    else:
        print('\033[31mError!\033[0m', '\033[31mARK_{} compilation finished with errors.\033[0m'.format(argvs[0].split('.')[0]))
    return pass_code


if __name__ == "__main__":
    sys.exit(Main(sys.argv[1:]))

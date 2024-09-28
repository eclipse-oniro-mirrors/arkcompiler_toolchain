#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
from fnmatch import fnmatch
import errno
import json
import os
import platform
import subprocess
import sys
from typing import List, Any, Tuple, Union, Optional

CURRENT_FILENAME = os.path.basename(__file__)


def str_of_time_now() -> str:
    return datetime.now().strftime("%Y-%m-%d-%H-%M-%S-%f")[:-3]


def _call(cmd: str):
    print("# %s" % cmd)
    return subprocess.call(cmd, shell=True)


def _write(filename: str, content: str, mode: str):
    with open(filename, mode) as f:
        f.write(content)


def call_with_output(cmd: str, file: str):
    print("# %s" % cmd)
    host = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    while True:
        try:
            build_data = host.stdout.readline().decode('utf-8')
            sys.stdout.flush()
            print(build_data)
            _write(file, build_data, "a")
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


def enable_ccache():
    try:
        ccache_path = subprocess.check_output(['which', 'ccache']).strip().decode()
    except subprocess.CalledProcessError:
        print("Error: ccache not found.")
        return
    os.environ['CCACHE_EXEC'] = ccache_path
    os.environ['USE_CCACHE'] = "1"


def backup(file: str, mode: str):
    if os.path.exists(file):
        with open(file, 'r+') as src_file:
            src_content = src_file.read()
            src_file.seek(0)
            src_file.truncate()

        with open(file[:-4] + "_last.log", mode) as dst_file:
            dst_file.write(src_content)


class ArkPy:
    # constants determined by designer of this class
    NAME_OF_OUT_DIR_OF_FIRST_LEVEL = "out"
    DELIMITER_BETWEEN_OS_CPU_MODE_FOR_COMMAND = "."
    DELIMITER_FOR_SECOND_OUT_DIR_NAME = "."
    GN_TARGET_LOG_FILE_NAME = "build.log"
    UNITTEST_LOG_FILE_NAME = "unittest.log"
    TEST262_LOG_FILE_NAME = "test262.log"
    REGRESS_TEST_LOG_FILE_NAME = "regresstest.log"
    PREBUILTS_DOWNLOAD_CONFIG_FILE_PATH = \
        "./arkcompiler/toolchain/build/prebuilts_download/prebuilts_download_config.json"
    INDENTATION_STRING_PER_LEVEL = "  "  # for help message
    # In ARG_DICT, "flags" and "description" are must-keys for the leaf-dicts in it.
    # (Future designer need know.)
    ARG_DICT = {
        "os_cpu": {
            "linux_x64": {
                "flags": ["linux_x64", "x64"],
                "description":
                    "Build for arkcompiler target of target-operating-system linux and "
                    "target-central-processing-unit x64.",
                "gn_args": ["target_os=\"linux\"", "target_cpu=\"x64\""],
                "prefix_of_name_of_out_dir_of_second_level": "x64",
            },
            "linux_x86": {
                "flags": ["linux_x86", "x86"],
                "description":
                    "Build for arkcompiler target of target-operating-system linux and "
                    "target-central-processing-unit x86.",
                "gn_args": ["target_os=\"linux\"", "target_cpu=\"x86\""],
                "prefix_of_name_of_out_dir_of_second_level": "x86",
            },
            "ohos_arm": {
                "flags": ["ohos_arm", "arm"],
                "description":
                    "Build for arkcompiler target of target-operating-system ohos and "
                    "target-central-processing-unit arm.",
                "gn_args": ["target_os=\"ohos\"", "target_cpu=\"arm\""],
                "prefix_of_name_of_out_dir_of_second_level": "arm",
            },
            "ohos_arm64": {
                "flags": ["ohos_arm64", "arm64"],
                "description":
                    "Build for arkcompiler target of target-operating-system ohos and "
                    "target-central-processing-unit arm64.",
                "gn_args": ["target_os=\"ohos\"", "target_cpu=\"arm64\""],
                "prefix_of_name_of_out_dir_of_second_level": "arm64",
            },
            "android_arm64": {
                "flags": ["android_arm64"],
                "description":
                    "Build for arkcompiler target of target-operating-system android and "
                    "target-central-processing-unit arm64.",
                "gn_args": ["target_os=\"android\"", "target_cpu=\"arm64\""],
                "prefix_of_name_of_out_dir_of_second_level": "android_arm64",
            },
            "mingw_x86_64": {
                "flags": ["mingw_x86_64"],
                "description":
                    "Build for arkcompiler target of target-operating-system MinGW(Minimalist GNU on Windows) and "
                    "target-central-processing-unit x86_64.",
                "gn_args": ["target_os=\"mingw\"", "target_cpu=\"x86_64\""],
                "prefix_of_name_of_out_dir_of_second_level": "mingw_x86_64",
            },
            "ohos_mipsel": {
                "flags": ["ohos_mipsel", "mipsel"],
                "description":
                    "Build for arkcompiler target of target-operating-system ohos and "
                    "target-central-processing-unit mipsel(32-bit little-endian mips).",
                "gn_args": ["target_os=\"ohos\"", "target_cpu=\"mipsel\""],
                "prefix_of_name_of_out_dir_of_second_level": "mipsel",
            },
            "mac_arm64": {
                "flags": ["mac_arm64", "arm64"],
                "description":
                    "Build for arkcompiler target of target-operating-system linux and "
                    "target-central-processing-unit arm64.",
                "gn_args": ["target_os=\"mac\"", "target_cpu=\"arm64\""],
                "prefix_of_name_of_out_dir_of_second_level": "mac_arm64",
            },
            "mac_x86": {
                "flags": ["mac_x86", "x86"],
                "description":
                    "Build for arkcompiler target of target-operating-system mac and "
                    "target-central-processing-unit x86.",
                "gn_args": ["target_os=\"mac\"", "target_cpu=\"x86\""],
                "prefix_of_name_of_out_dir_of_second_level": "mac_x86",
            },
        },
        "mode": {
            "release": {
                "flags": ["release", "r"],
                "description": "Build for arkcompiler target(executables and libraries) for distribution.",
                "gn_args": ["is_debug=false"],
                "suffix_of_name_of_out_dir_of_second_level": "release",
            },
            "debug": {
                "flags": ["debug", "d"],
                "description": "Build for arkcompiler target(executables and libraries) for debugging.",
                "gn_args": ["is_debug=true"],
                "suffix_of_name_of_out_dir_of_second_level": "debug",
            },
            "fastverify": {
                "flags": ["fastverify", "fv"],
                "description": "Build for arkcompiler target(executables and libraries) for fastverify.",
                "gn_args": ["is_debug=true is_fastverify=true"],
                "suffix_of_name_of_out_dir_of_second_level": "fastverify",
            },
        },
        "target": {
            "test262": {
                "flags": ["test262", "test-262", "test_262", "262test", "262-test", "262_test", "262"],
                "description": "Compile arkcompiler target and run test262 with arkcompiler target.",
                "gn_targets_depend_on": ["default"],
                "arm64_gn_targets_depend_on": ["ark_js_packages"],
            },
            "unittest": {
                "flags": ["unittest", "ut"],
                "description":
                    "Compile and run unittest of arkcompiler target. "
                    "Add --keep-going=N to keep running unittest when errors occured less than N. "
                    "Add --gn-args=\"run_with_qemu=true\" timeout=\"1200\"\
                    \"disable_force_gc=true\" to command when running unittest of non-host type with qemu.",
                "gn_targets_depend_on": ["unittest_packages"],
            },
            "workload": {
                "flags": ["workload", "work-load", "work_load"],
                "description": "Compile arkcompiler target and run workload with arkcompiler target.",
                "gn_targets_depend_on": ["default"],
            },
            "regresstest": {
                "flags": ["regresstest", "regress_test", "regress", "testregress", "test_regress"],
                "description": "Compile arkcompiler target and run regresstest with arkcompiler target.",
                "gn_targets_depend_on": ["default"],
            },
            "gn_target": {
                "flags": ["<name of target in \"*.gn*\" file>"],  # any other flags
                "description":
                    "Build for arkcompiler target assigned by user. Targets include group(ets_runtime), "
                    "ohos_executable(ark_js_vm), ohos_shared_library(libark_jsruntime), "
                    "ohos_static_library(static_icuuc), ohos_source_set(libark_jsruntime_set), "
                    "ohos_unittest(EcmaVm_001_Test), action(EcmaVm_001_TestAction) and other target of user-defined "
                    "template type in \"*.gn*\" file.",
                "gn_targets_depend_on": [],  # not need, depend on deps of itself in "*.gn*" file
            },
        },
        "option": {
            "clean": {
                "flags": ["--clean", "-clean"],
                "description":
                    "Clean the root-out-dir(x64.release-->out/x64.release) execept for file args.gn. "
                    "Then exit.",
            },
            "clean-continue": {
                "flags": ["--clean-continue", "-clean-continue"],
                "description":
                    "Clean the root-out-dir(x64.release-->out/x64.release) execept for file args.gn. "
                    "Then continue to build.",
            },
            "gn-args": {
                "flags": ["--gn-args=*", "-gn-args=*"],
                "description":
                    "Pass args(*) to gn command. Example: python3 ark.py x64.release "
                    "--gn-args=\"bool_declared_in_src_gn=true string_declared_in_src_gn=\\\"abcd\\\" "
                    "list_declared_in_src_gn=[ \\\"element0\\\", \\\"element1\\\" ] print(list_declared_in_src_gn) "
                    "exec_script(\\\"script_in_src\\\", [ \\\"arg_to_script\\\" ])\"  .",
            },
            "keepdepfile": {
                "flags": ["--keepdepfile", "-keepdepfile"],
                "description":
                    "Keep depfile(\"*.o.d\") generated by commands(CXX, CC ...) called by ninja during compilation.",
            },
            "verbose": {
                "flags": ["--verbose", "-verbose"],
                "description": "Print full commands(CXX, CC, LINK ...) called by ninja during compilation.",
            },
            "keep-going": {
                "flags": ["--keep-going=*", "-keep-going=*"],
                "description": "Keep running unittest etc. until errors occured less than N times"
                               " (use 0 to ignore all errors).",
            },
        },
        "help": {
            "flags": ["help", "--help", "--h", "-help", "-h"],
            "description": "Show the usage of ark.py.",
        },
    }

    # variables which would change with the change of host_os or host_cpu
    gn_binary_path = ""
    ninja_binary_path = ""

    # variables which would change with the change of ark.py command
    has_cleaned = False
    enable_verbose = False
    enable_keepdepfile = False
    ignore_errors = 1

    def __main__(self, arg_list: list):
        enable_ccache()
        # delete duplicate arg in arg_list
        arg_list = list(dict.fromkeys(arg_list))
        # match [help] flag
        if len(arg_list) == 0 or (
                True in [self.is_dict_flags_match_arg(self.ARG_DICT.get("help"), arg) for arg in arg_list]):
            print(self.get_help_msg_of_all())
            return
        # match [[os_cpu].[mode]] flag
        [match_success, key_to_dict_in_os_cpu, key_to_dict_in_mode] = self.dict_in_os_cpu_mode_match_arg(arg_list[0])
        if match_success:
            self.start_for_matched_os_cpu_mode(key_to_dict_in_os_cpu, key_to_dict_in_mode, arg_list[1:])
        else:
            print("\033[92mThe command is not supported! Help message shows below.\033[0m\n{}".format(
                self.get_help_msg_of_all()))
        return

    @staticmethod
    def is_dict_flags_match_arg(dict_to_match: dict, arg_to_match: str) -> bool:
        for flag in dict_to_match["flags"]:
            if fnmatch(arg_to_match, flag):
                return True
        return False

    @staticmethod
    def libs_dir(is_arm, is_aot, is_pgo, out_dir, x64_out_dir) -> str:
        if is_arm and is_aot and is_pgo:
            return (f"--libs-dir ../../{out_dir}/arkcompiler/ets_runtime:"
                    f"../../{out_dir}/thirdparty/icu:"
                    f"../../{out_dir}/third_party/icu:"
                    f"../../thirdparty/zlib:"
                    f"../../prebuilts/clang/ohos/linux-x86_64/llvm/lib")
        if is_arm and is_aot and not is_pgo:
            return ("--libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib"
                    f":../../{x64_out_dir}/thirdparty/icu/")
        if not is_arm and is_aot:
            return (f"--libs-dir ../../{out_dir}/arkcompiler/ets_runtime"
                    f":../../{out_dir}/thirdparty/icu:"
                    f"../../{out_dir}/third_party/icu:"
                    f"../../thirdparty/zlib:"
                    f"../../prebuilts/clang/ohos/linux-x86_64/llvm/lib")
        # not is_arm and not is_aot
        return " --libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib"

    @staticmethod
    def get_cmd(test_suite, test_script_name, test_script_path, gn_args, out_path, x64_out_path, aot_mode, run_pgo,
                enable_litecg, args_to_cmd, timeout, ignore_list: Optional[str] = None):
        cmd = [
            f"cd {test_script_path}",
            f"&& python3 {test_script_name} {args_to_cmd}",
            f"--timeout {timeout}",
            f"--ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm",
            "--ark-frontend=es2panda"
        ]
        is_arm = any('target_cpu="arm64"' in arg for arg in gn_args)
        if is_arm:
            cmd.append("--ark-arch aarch64")
            cmd.append(f"--ark-arch-root=../../{out_path}/common/common/libc/")
            cmd.append(f"--ark-frontend-binary=../../{x64_out_path}/arkcompiler/ets_frontend/es2abc")
            cmd.append(f"--merge-abc-binary=../../{x64_out_path}/arkcompiler/ets_frontend/merge_abc")
            if aot_mode:
                cmd.append(f"--ark-aot-tool=../../{out_path}/arkcompiler/ets_runtime/ark_aot_compiler")
                if test_suite == "regresstest":
                    cmd.append(f"--stub-path=../../{out_path}/gen/arkcompiler/ets_runtime/stub.an")
        else:
            cmd.append(f"--ark-frontend-binary=../../{out_path}/arkcompiler/ets_frontend/es2abc")
            cmd.append(f"--merge-abc-binary=../../{out_path}/arkcompiler/ets_frontend/merge_abc")
            if aot_mode:
                cmd.append(f"--ark-aot-tool=../../{out_path}/arkcompiler/ets_runtime/ark_aot_compiler")
                if test_suite == "regresstest":
                    cmd.append(f"--stub-path=../../{out_path}/gen/arkcompiler/ets_runtime/stub.an")

        cmd.append(ArkPy.libs_dir(
            is_arm=is_arm,
            is_aot=aot_mode,
            is_pgo=run_pgo,
            out_dir=out_path,
            x64_out_dir=x64_out_path
        ))

        if aot_mode:
            cmd.append("--ark-aot")
            mode = ["AOT"]
            if run_pgo:
                cmd.append("--run-pgo")
                mode.append("PGO")
            if enable_litecg:
                cmd.append("--enable-litecg")
                mode.append("LiteCG")
            mode_str = " ".join(mode)
            print(f"Running {test_suite} in {mode_str} Mode\n")

        if test_suite == "regresstest" and ignore_list:
            cmd.append(f"--ignore-list {ignore_list}")

        if test_suite == "regresstest":
            cmd.append(f"--out-dir ../../{out_path}")

        return " ".join(cmd)

    @staticmethod
    def get_test262_aot_cmd(gn_args, out_path, x64_out_path, run_pgo, enable_litecg, args_to_test262_cmd,
                            timeout):
        print("running test262 in AotMode\n")
        if any('target_cpu="arm64"' in arg for arg in gn_args):
            if run_pgo:
                test262_cmd = f"cd arkcompiler/ets_frontend && python3 test262/run_test262.py {args_to_test262_cmd}" \
                              f" --timeout {timeout}" \
                              f" --libs-dir ../../{out_path}/arkcompiler/ets_runtime:../../{out_path}/thirdparty/icu:" \
                              f"../../{out_path}/thirdparty/zlib:../../prebuilts/clang/ohos/linux-x86_64/llvm/lib" \
                              " --ark-arch aarch64" \
                              f" --ark-arch-root=../../{out_path}/common/common/libc/" \
                              f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                              f" --ark-aot-tool=../../{out_path}/arkcompiler/ets_runtime/ark_aot_compiler" \
                              f" --ark-frontend-binary=../../{x64_out_path}/arkcompiler/ets_frontend/es2abc" \
                              f" --merge-abc-binary=../../{x64_out_path}/arkcompiler/ets_frontend/merge_abc" \
                              " --ark-aot" \
                              " --ark-frontend=es2panda" \
                              " --run-pgo"
            else:
                test262_cmd = "cd arkcompiler/ets_frontend && python3 test262/run_test262.py {0} --timeout {3}" \
                              " --libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib:../../{2}/thirdparty/icu/" \
                              " --ark-arch aarch64" \
                              " --ark-arch-root=../../{1}/common/common/libc/" \
                              " --ark-aot" \
                              " --ark-aot-tool=../../{2}/arkcompiler/ets_runtime/ark_aot_compiler" \
                              " --ark-tool=../../{1}/arkcompiler/ets_runtime/ark_js_vm" \
                              " --ark-frontend-binary=../../{2}/arkcompiler/ets_frontend/es2abc" \
                              " --merge-abc-binary=../../{2}/arkcompiler/ets_frontend/merge_abc" \
                              " --ark-frontend=es2panda".format(args_to_test262_cmd, out_path, x64_out_path, timeout)
        else:
            run_pgo_arg = " --run-pgo" if run_pgo else ""
            test262_cmd = f"cd arkcompiler/ets_frontend && python3 test262/run_test262.py {args_to_test262_cmd}" \
                          f" --timeout {timeout}" \
                          f" --libs-dir ../../{out_path}/arkcompiler/ets_runtime:../../{out_path}/thirdparty/icu" \
                          f":../../{out_path}/thirdparty/zlib:../../prebuilts/clang/ohos/linux-x86_64/llvm/lib" \
                          f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                          f" --ark-aot-tool=../../{out_path}/arkcompiler/ets_runtime/ark_aot_compiler" \
                          f" --ark-frontend-binary=../../{out_path}/arkcompiler/ets_frontend/es2abc" \
                          f" --merge-abc-binary=../../{out_path}/arkcompiler/ets_frontend/merge_abc" \
                          " --ark-aot" \
                          " --ark-frontend=es2panda" \
                          f" {run_pgo_arg}"
        if enable_litecg:
            test262_cmd = test262_cmd + " --enable-litecg"
        return test262_cmd

    @staticmethod
    def get_jit_cmd(test_suite, test_script_name, test_script_path, gn_args, out_path, x64_out_path, args_to_cmd,
                    timeout):
        print(f"running {test_suite} in JIT mode\n")
        if any('target_cpu="arm64"' in arg for arg in gn_args):
            cmd = f"cd {test_script_path} && python3 {test_script_name} {args_to_cmd} --timeout {timeout}" \
                  f" --libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib:../../{out_path}/thirdparty/icu/" \
                  f":../../{out_path}/thirdparty/bounds_checking_function" \
                  f":../../{out_path}/arkcompiler/ets_runtime:" \
                  " --ark-arch aarch64" \
                  " --run-jit" \
                  f" --ark-arch-root=../../{out_path}/common/common/libc/" \
                  f" --ark-aot-tool=../../{x64_out_path}/arkcompiler/ets_runtime/ark_aot_compiler" \
                  f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                  f" --ark-frontend-binary=../../{x64_out_path}/arkcompiler/ets_frontend/es2abc" \
                  f" --merge-abc-binary=../../{x64_out_path}/arkcompiler/ets_frontend/merge_abc" \
                  " --ark-frontend=es2panda"
        else:
            cmd = f"cd arkcompiler/ets_frontend && python3 {test_script_name} {args_to_cmd} --timeout {timeout}" \
                  f" --libs-dir ../../{out_path}/arkcompiler/ets_runtime:../../{out_path}/thirdparty/icu" \
                  f":../../{out_path}/thirdparty/zlib:../../prebuilts/clang/ohos/linux-x86_64/llvm/lib" \
                  " --run-jit" \
                  f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                  f" --ark-frontend-binary=../../{out_path}/arkcompiler/ets_frontend/es2abc" \
                  f" --merge-abc-binary=../../{out_path}/arkcompiler/ets_frontend/merge_abc" \
                  " --ark-frontend=es2panda"
        return cmd

    @staticmethod
    def get_baseline_jit_cmd(test_suite, test_script_name, test_script_path, gn_args, out_path, x64_out_path,
                             args_to_test262_cmd, timeout):
        print(f"running {test_suite} in baseline JIT mode\n")
        if any('target_cpu="arm64"' in arg for arg in gn_args):
            cmd = f"cd {test_script_path} && python3 {test_script_name} {args_to_test262_cmd} --timeout {timeout}" \
                  f" --libs-dir ../../prebuilts/clang/ohos/linux-x86_64/llvm/lib" \
                  f":../../{out_path}/thirdparty/icu" \
                  f":../../prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos" \
                  f":../../{out_path}/thirdparty/bounds_checking_function" \
                  f":../../{out_path}/arkcompiler/ets_runtime" \
                  f":../../{out_path}/common/common/libc/lib" \
                  " --ark-arch aarch64" \
                  " --run-baseline-jit" \
                  f" --ark-arch-root=../../{out_path}/common/common/libc/" \
                  f" --ark-aot-tool=../../{x64_out_path}/arkcompiler/ets_runtime/ark_aot_compiler" \
                  f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                  f" --ark-frontend-binary=../../{x64_out_path}/arkcompiler/ets_frontend/es2abc" \
                  f" --merge-abc-binary=../../{x64_out_path}/arkcompiler/ets_frontend/merge_abc" \
                  " --ark-frontend=es2panda"
        else:
            cmd = f"cd {test_script_path} && python3 {test_script_name} {args_to_test262_cmd} --timeout {timeout}" \
                  f" --libs-dir ../../{out_path}/lib.unstripped/arkcompiler/ets_runtime" \
                  f":../../{out_path}/thirdparty/icu" \
                  ":../../prebuilts/clang/ohos/linux-x86_64/llvm/lib" \
                  f":../../{out_path}/thirdparty/bounds_checking_function/" \
                  " --run-baseline-jit" \
                  f" --ark-tool=../../{out_path}/arkcompiler/ets_runtime/ark_js_vm" \
                  f" --ark-frontend-binary=../../{out_path}/arkcompiler/ets_frontend/es2abc" \
                  f" --merge-abc-binary=../../{out_path}/arkcompiler/ets_frontend/merge_abc" \
                  " --ark-frontend=es2panda"
        return cmd

    @staticmethod
    def build_args_to_test262_cmd(arg_list):
        args_to_test262_cmd = []

        disable_force_gc_name = "--disable-force-gc"
        disable_force_gc_value, arg_list = ArkPy.parse_bool_option(
            arg_list, option_name=disable_force_gc_name, default_value=False
        )
        if disable_force_gc_value:
            args_to_test262_cmd.extend([disable_force_gc_name])

        threads_name = "--threads"
        threads_value, arg_list = ArkPy.parse_option(arg_list, option_name=threads_name, default_value=None)
        if threads_value:
            args_to_test262_cmd.extend([threads_name, threads_value])

        test_list_name = "--test-list"
        test_list_value, arg_list = ArkPy.parse_option(arg_list, option_name=test_list_name, default_value=None)
        if test_list_value is not None:
            args_to_test262_cmd.extend([test_list_name, test_list_value])

        enable_rm = [arg for arg in arg_list if "enable-rm" in arg]
        if enable_rm:
            args_to_test262_cmd.append("--enable-rm")
            arg_list.remove(enable_rm[0])

        skip_list_name = "--skip-list"
        skip_list_value, arg_list = ArkPy.parse_option(arg_list, option_name=skip_list_name, default_value=None)
        if skip_list_value is not None:
            args_to_test262_cmd.extend([skip_list_name, skip_list_value])

        if len(arg_list) == 0:
            args_to_test262_cmd.append("--es2021 all")
        elif len(arg_list) == 1:
            arg = arg_list[0]
            if arg == "sendable":
                args_to_test262_cmd.append("--sendable sendable")
            elif ".js" in arg:
                args_to_test262_cmd.append("--file test262/data/test_es2021/{}".format(arg))
            else:
                args_to_test262_cmd.append("--dir test262/data/test_es2021/{}".format(arg))
        else:
            print("\033[92m\"test262\" not support multiple additional arguments.\033[0m\n".format())
            sys.exit(0)

        return " ".join(args_to_test262_cmd)

    @staticmethod
    def build_args_to_regress_cmd(arg_list):
        args_to_regress_cmd = []

        disable_force_gc_name = "--disable-force-gc"
        disable_force_gc_value, arg_list = ArkPy.parse_bool_option(
            arg_list, option_name=disable_force_gc_name, default_value=False
        )
        if disable_force_gc_value:
            args_to_regress_cmd.extend([disable_force_gc_name])

        processes_name = "--processes"
        processes_value, arg_list = ArkPy.parse_option(arg_list, option_name=processes_name, default_value=1)
        args_to_regress_cmd.extend([processes_name, processes_value])

        test_list_name = "--test-list"
        test_list_value, arg_list = ArkPy.parse_option(arg_list, option_name=test_list_name, default_value=None)
        if test_list_value is not None:
            args_to_regress_cmd.extend([test_list_name, test_list_value])

        if len(arg_list) == 1:
            arg = arg_list[0]
            if ".js" in arg:
                args_to_regress_cmd.append(f"--test-file {arg}")
            else:
                args_to_regress_cmd.append(f"--test-dir {arg}")
        elif len(arg_list) > 1:
            print("\033[92m\"regresstest\" not support multiple additional arguments.\033[0m\n".format())
            sys.exit(0)

        return " ".join([str(arg) for arg in args_to_regress_cmd])

    @staticmethod
    def parse_option(arg_list: List[str], option_name: str, default_value: Optional[Union[str, int]]) \
            -> Tuple[Optional[Union[str, int]], List[str]]:
        option_value, arg_list = ArkPy.__parse_option_with_space(arg_list, option_name)
        if option_value is None:
            option_value, arg_list = ArkPy.__parse_option_with_equal(arg_list, option_name)
        if option_value is None and default_value is not None:
            option_value = default_value
        return option_value, arg_list

    @staticmethod
    def parse_bool_option(arg_list: List[str], option_name: str, default_value: bool) \
            -> Tuple[bool, List[str]]:
        if option_name in arg_list:
            option_index = arg_list.index(option_name)
            option_value = not default_value
            arg_list = arg_list[:option_index] + arg_list[option_index + 1:]
        else:
            option_value = default_value

        return option_value, arg_list

    @staticmethod
    def __is_option_value_int(value: Optional[Union[str, int]]) -> Tuple[bool, Optional[int]]:
        if isinstance(value, int):
            return True, int(value)
        else:
            return False, None

    @staticmethod
    def __is_option_value_str(value: Optional[Union[str, int]]) -> Tuple[bool, Optional[str]]:
        if isinstance(value, str):
            return True, str(value)
        else:
            return False, None

    @staticmethod
    def __get_option_value(option_name: str, value: Optional[Union[str, int]]) -> Union[str, int]:
        result, res_value = ArkPy.__is_option_value_int(value)
        if result:
            return res_value
        result, res_value = ArkPy.__is_option_value_str(value)
        if result:
            return res_value
        print(f"Invalid '{option_name}' value.")
        sys.exit(1)

    @staticmethod
    def __parse_option_with_space(arg_list: List[str], option_name: str) \
            -> Tuple[Optional[Union[str, int]], List[str]]:
        if option_name in arg_list:
            option_index = arg_list.index(option_name)
            if len(arg_list) > option_index + 1:
                option_value = ArkPy.__get_option_value(option_name, arg_list[option_index + 1])
                arg_list = arg_list[:option_index] + arg_list[option_index + 2:]
            else:
                print(f"Missing {option_name} value.")
                sys.exit(1)

            return option_value, arg_list
        return None, arg_list

    @staticmethod
    def __parse_option_with_equal(arg_list: List[str], option_name: str) \
            -> Tuple[Optional[Union[str, int]], List[str]]:
        for index, arg in enumerate(arg_list):
            local_option_name = f"{option_name}="
            if arg.startswith(local_option_name):
                option_value = arg[len(local_option_name):]
                if option_value:
                    option_value = ArkPy.__get_option_value(option_name, option_value)
                    arg_list = arg_list[:index] + arg_list[index + 1:]
                    return option_value, arg_list
                else:
                    print(f"Missing {option_name} value.")
                    sys.exit(1)
        return None, arg_list

    @staticmethod
    def __get_x64_out_path(out_path) -> str:
        if 'release' in out_path:
            return 'out/x64.release'
        if 'debug' in out_path:
            return 'out/x64.debug'
        if 'fastverify' in out_path:
            return 'out/x64.fastverify'
        return ""

    def get_binaries(self):
        host_os = sys.platform
        host_cpu = platform.machine()
        try:
            with open(self.PREBUILTS_DOWNLOAD_CONFIG_FILE_PATH) as file_prebuilts_download_config:
                prebuilts_download_config_dict = json.load(file_prebuilts_download_config)
                file_prebuilts_download_config.close()
            for element in prebuilts_download_config_dict[host_os][host_cpu]["copy_config"]:
                if element["unzip_filename"] == "gn":
                    self.gn_binary_path = os.path.join(element["unzip_dir"], element["unzip_filename"])
                elif element["unzip_filename"] == "ninja":
                    self.ninja_binary_path = os.path.join(element["unzip_dir"], element["unzip_filename"])
        except Exception as error:
            print("\nLogic of getting gn binary or ninja binary does not match logic of prebuilts_download." \
                  "\nCheck func \033[92m{0} of class {1} in file {2}\033[0m against file {3} if the name of this " \
                  "file had not changed!\n".format(
                sys._getframe().f_code.co_name, self.__class__.__name__, CURRENT_FILENAME,
                self.PREBUILTS_DOWNLOAD_CONFIG_FILE_PATH))
            raise error
        if self.gn_binary_path == "" or self.ninja_binary_path == "":
            print("\nLogic of prebuilts_download may be wrong." \
                  "\nCheck \033[92mdata in file {0}\033[0m against func {1} of class {2} in file {3}!\n".format(
                self.PREBUILTS_DOWNLOAD_CONFIG_FILE_PATH, sys._getframe().f_code.co_name, self.__class__.__name__,
                CURRENT_FILENAME))
            sys.exit(0)
        if not os.path.isfile(self.gn_binary_path) or not os.path.isfile(self.ninja_binary_path):
            print("\nStep for prebuilts_download may be ommited. (\033[92m./prebuilts_download.sh\033[0m)" \
                  "\nCheck \033[92mwhether gn binary and ninja binary are under directory prebuilts\033[0m!\n".format())
            sys.exit(0)
        return

    def which_dict_flags_match_arg(self, dict_including_dicts_to_match: dict, arg_to_match: str) -> str:
        for key in dict_including_dicts_to_match.keys():
            if self.is_dict_flags_match_arg(dict_including_dicts_to_match[key], arg_to_match):
                return key
        return ""

    def dict_in_os_cpu_mode_match_arg(self, arg: str) -> [bool, str, str]:
        os_cpu_part = ""
        mode_part = ""
        match_success = True
        key_to_dict_in_os_cpu_matched_arg = ""
        key_to_dict_in_mode_matched_arg = ""
        arg_to_list = arg.split(self.DELIMITER_BETWEEN_OS_CPU_MODE_FOR_COMMAND)
        if len(arg_to_list) == 1:
            os_cpu_part = arg_to_list[0]
            mode_part = "release"
            key_to_dict_in_os_cpu_matched_arg = self.which_dict_flags_match_arg(self.ARG_DICT.get("os_cpu"), os_cpu_part)
            key_to_dict_in_mode_matched_arg = self.which_dict_flags_match_arg(self.ARG_DICT.get("mode"), mode_part)
        elif len(arg_to_list) == 2:
            os_cpu_part = arg_to_list[0]
            mode_part = arg_to_list[1]
            key_to_dict_in_os_cpu_matched_arg = self.which_dict_flags_match_arg(self.ARG_DICT.get("os_cpu"), os_cpu_part)
            key_to_dict_in_mode_matched_arg = self.which_dict_flags_match_arg(self.ARG_DICT.get("mode"), mode_part)
        else:
            print("\"\033[92m{0}\033[0m\" combined with more than 2 flags is not supported.".format(arg))
        if (key_to_dict_in_os_cpu_matched_arg == "") | (key_to_dict_in_mode_matched_arg == ""):
            match_success = False
        return [match_success, key_to_dict_in_os_cpu_matched_arg, key_to_dict_in_mode_matched_arg]

    def get_help_msg_of_dict(self, dict_in: dict, indentation_str_current: str, indentation_str_per_level: str) -> str:
        help_msg = "".format()
        for key in dict_in.keys():
            if isinstance(dict_in[key], dict):
                help_msg += "{0}{1}:\n".format(indentation_str_current, key)
                help_msg += self.get_help_msg_of_dict(
                    dict_in[key], indentation_str_current + indentation_str_per_level, indentation_str_per_level)
            elif key == "flags":
                help_msg += "{0}{1}: \033[92m{2}\033[0m\n".format(indentation_str_current, key, " ".join(dict_in[key]))
            elif key == "description":
                help_msg += "{0}{1}: {2}\n".format(indentation_str_current, key, dict_in[key])
        return help_msg

    def get_help_msg_of_all(self) -> str:
        help_msg = "".format()
        # Command template
        help_msg += "\033[32mCommand template:\033[0m\n{}\n\n".format(
            "  python3 ark.py \033[92m[os_cpu].[mode] [gn_target] [option]\033[0m\n"
            "  python3 ark.py \033[92m[os_cpu].[mode] [test262] [none or --aot] " \
            "[none or --pgo] [none or --litecg] [none, file or dir] [none or --threads=X] [option]\033[0m\n"
            "  python3 ark.py \033[92m[os_cpu].[mode] [test262] [none or --jit] [none or --threads=X]\033[0m\n"
            "  python3 ark.py \033[92m[os_cpu].[mode] [test262] [none or --baseline-jit] [none or --enable-rm] " \
            "[none or --threads=X and/or --test-list TEST_LIST_NAME]\033[0m\n"
            "  python3 ark.py \033[92m[os_cpu].[mode] [unittest] [option]\033[0m\n"
            "  python3 ark.py \033[92m[os_cpu].[mode] [regresstest] [none, file or dir] " \
              "[none or --processes X and/or --test-list TEST_LIST_NAME]\033[0m\n")
        # Command examples
        help_msg += "\033[32mCommand examples:\033[0m\n{}\n\n".format(
            "  python3 ark.py \033[92mx64.release\033[0m\n"
            "  python3 ark.py \033[92mx64.release ets_runtime\033[0m\n"
            "  python3 ark.py \033[92mx64.release ark_js_vm es2panda\033[0m\n"
            "  python3 ark.py \033[92mx64.release ark_js_vm es2panda --clean\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 --threads=16\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 --aot --pgo --litecg\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 --aot --pgo --litecg --threads=8\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 --jit --enable-rm\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 --baseline-jit\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 built-ins/Array\033[0m\n"
            "  python3 ark.py \033[92mx64.release test262 built-ins/Array/name.js\033[0m\n"
            "  python3 ark.py \033[92mx64.release unittest\033[0m\n"
            "  python3 ark.py \033[92mx64.release regresstest\033[0m\n"
            "  python3 ark.py \033[92mx64.release regresstest --processes=4\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload report\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload report dev\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload report dev -20\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload report dev -20 10\033[0m\n"
            "  python3 ark.py \033[92mx64.release workload report dev -20 10 weekly_workload\033[0m\n")
        # Arguments
        help_msg += "\033[32mArguments:\033[0m\n{}".format(
            self.get_help_msg_of_dict(
                self.ARG_DICT, self.INDENTATION_STRING_PER_LEVEL, self.INDENTATION_STRING_PER_LEVEL))
        return help_msg

    def clean(self, out_path: str):
        if not os.path.exists(out_path):
            print("Path \"{}\" does not exist! No need to clean it.".format(out_path))
        else:
            print("=== clean start ===")
            code = _call("{0} clean {1}".format(self.gn_binary_path, out_path))
            if code != 0:
                print("=== clean failed! ===\n")
                sys.exit(code)
            print("=== clean success! ===\n")
        return

    def build_for_gn_target(self, out_path: str, gn_args: list, arg_list: list, log_file_name: str):
        # prepare log file
        build_log_path = os.path.join(out_path, log_file_name)
        backup(build_log_path, "w")
        if arg_list is not None:
            build_target = " ".join([str(arg).strip() for arg in arg_list
                                     if arg is not None or len(str(arg).strip()) > 0])
        else:
            build_target = ""
        str_to_build_log = "================================\nbuild_time: {0}\nbuild_target: {1}\n\n".format(
            str_of_time_now(), build_target)
        _write(build_log_path, str_to_build_log, "a")
        # gn command
        print("=== gn gen start ===")
        code = call_with_output(
            "{0} gen {1} --args=\"{2}\" --export-compile-commands".format(
                self.gn_binary_path, out_path, " ".join(gn_args).replace("\"", "\\\"")),
            build_log_path)
        if code != 0:
            print("=== gn gen failed! ===\n")
            sys.exit(code)
        else:
            print("=== gn gen success! ===\n")
        # ninja command
        # Always add " -d keeprsp" to ninja command to keep response file("*.rsp"), thus we could get shared libraries
        # of an excutable from its response file.
        ninja_cmd = \
            self.ninja_binary_path + \
            (" -v" if self.enable_verbose else "") + \
            (" -d keepdepfile" if self.enable_keepdepfile else "") + \
            " -d keeprsp" + \
            " -C {}".format(out_path) + \
            " {}".format(" ".join(arg_list if arg_list else [])) + \
            " -k {}".format(self.ignore_errors)
        print(ninja_cmd)
        code = call_with_output(ninja_cmd, build_log_path)
        if code != 0:
            print("=== ninja failed! ===\n")
            sys.exit(code)
        else:
            print("=== ninja success! ===\n")
        return

    def call_build_gn_target(self, gn_args, out_path, x64_out_path, test_suite, log_file_name):
        if any('target_cpu="arm64"' in arg for arg in gn_args):
            gn_args.append("so_dir_for_qemu=\"../../{0}/common/common/libc/\"".format(out_path))
            gn_args.append("run_with_qemu=true".format(out_path))
            if not os.path.exists(x64_out_path):
                os.makedirs(x64_out_path)
            self.build_for_gn_target(
                x64_out_path,
                ['target_os="linux"', 'target_cpu="x64"', 'is_debug=false'],
                self.ARG_DICT.get("target").get(test_suite).get("gn_targets_depend_on"),
                log_file_name)
            self.build_for_gn_target(
                out_path,
                gn_args,
                self.ARG_DICT.get("target").get(test_suite).get("arm64_gn_targets_depend_on"),
                log_file_name)
        else:
            self.build_for_gn_target(
                out_path,
                gn_args,
                self.ARG_DICT.get("target").get(test_suite).get("gn_targets_depend_on"),
                log_file_name)

    def get_build_cmd(self, *, test_suite, test_script_name, test_script_path,
                      out_path, x64_out_path, gn_args: list, args_to_cmd: str, timeout,
                      run_jit: bool = False, run_baseline_jit: bool = False, aot_mode: bool = False,
                      run_pgo: bool = False, enable_litecg: bool = False, ignore_list: Optional[str] = None) -> str:
        if run_jit:
            cmd = self.get_jit_cmd(test_suite, test_script_name, test_script_path,
                                   gn_args, out_path, x64_out_path, args_to_cmd, timeout)
        elif run_baseline_jit:
            cmd = self.get_baseline_jit_cmd(test_suite, test_script_name, test_script_path,
                                            gn_args, out_path, x64_out_path, args_to_cmd, timeout)
        elif aot_mode and test_suite == "test262":
            cmd = self.get_test262_aot_cmd(gn_args, out_path, x64_out_path, run_pgo,
                                           enable_litecg, args_to_cmd, timeout)
        else:
            cmd = self.get_cmd(test_suite, test_script_name, test_script_path,
                               gn_args, out_path, x64_out_path, aot_mode, run_pgo,
                               enable_litecg, args_to_cmd, timeout, ignore_list)
        return cmd

    def build_for_suite(self, *, test_suite, test_script_name, test_script_path,
                        out_path, gn_args: list, log_file_name, args_to_cmd: str, timeout,
                        run_jit: bool = False, run_baseline_jit: bool = False, aot_mode: bool = False,
                        run_pgo: bool = False, enable_litecg: bool = False, ignore_list: Optional[str] = None):
        x64_out_path = self.__get_x64_out_path(out_path)
        self.call_build_gn_target(gn_args, out_path, x64_out_path, test_suite, log_file_name)
        cmd = self.get_build_cmd(
            test_suite=test_suite,
            test_script_name=test_script_name,
            test_script_path=test_script_path,
            out_path=out_path,
            x64_out_path=x64_out_path,
            gn_args=gn_args,
            args_to_cmd=args_to_cmd,
            timeout=timeout,
            run_jit=run_jit,
            run_baseline_jit=run_baseline_jit,
            aot_mode=aot_mode, run_pgo=run_pgo, enable_litecg=enable_litecg, ignore_list=ignore_list)
        log_path = str(os.path.join(out_path, log_file_name))
        str_to_log = "================================\n{2}_time: {0}\n{2}_target: {1}\n\n".format(
            str_of_time_now(), args_to_cmd, test_suite)
        _write(log_path, str_to_log, "a")
        print(f"=== {test_suite} start ===")
        code = call_with_output(cmd, log_path)
        if code != 0:
            print(f"=== {test_suite} fail! ===\n")
            sys.exit(code)
        print(f"=== {test_suite} success! ===\n")

    def build_for_test262(self, out_path, gn_args: list, arg_list: list):
        timeout, arg_list = self.parse_timeout(arg_list)
        arg_list = arg_list[1:]

        is_aot_mode, arg_list = self.__purge_arg_list("--aot", arg_list)
        is_pgo, arg_list = self.__purge_arg_list("--pgo", arg_list)
        is_litecg, arg_list = self.__purge_arg_list("--litecg", arg_list)
        is_jit, arg_list = self.__purge_arg_list("--jit", arg_list)
        is_baseline_jit, arg_list = self.__purge_arg_list("--baseline-jit", arg_list)
        print(f"Test262: arg_list = {arg_list}")

        args_to_test262_cmd = self.build_args_to_test262_cmd(arg_list)
        self.build_for_suite(
            test_suite="test262",
            test_script_name="test262/run_test262.py",
            test_script_path="arkcompiler/ets_frontend",
            out_path=out_path,
            gn_args=gn_args,
            log_file_name=self.TEST262_LOG_FILE_NAME,
            args_to_cmd=args_to_test262_cmd,
            timeout=timeout,
            run_jit=is_jit,
            run_pgo=is_pgo,
            run_baseline_jit=is_baseline_jit,
            aot_mode=is_aot_mode,
            enable_litecg=is_litecg
        )

    def build_for_unittest(self, out_path: str, gn_args: list, log_file_name: str):
        self.build_for_gn_target(
            out_path, gn_args, self.ARG_DICT.get("target").get("unittest").get("gn_targets_depend_on"),
            log_file_name)
        return

    def build_for_regress_test(self, out_path, gn_args: list, arg_list: list):
        timeout, arg_list = self.parse_option(arg_list, option_name="--timeout", default_value=200)
        ignore_list, arg_list = self.parse_option(arg_list, option_name="--ignore-list", default_value=None)

        arg_list = arg_list[1:]

        is_aot, arg_list = self.__purge_arg_list("--aot", arg_list)
        is_pgo, arg_list = self.__purge_arg_list("--pgo", arg_list)
        is_litecg, arg_list = self.__purge_arg_list("--litecg", arg_list)
        is_jit, arg_list = self.__purge_arg_list("--jit", arg_list)
        is_baseline_jit, arg_list = self.__purge_arg_list("--baseline-jit", arg_list)
        print(f"Regress: arg_list = {arg_list}")

        args_to_regress_test_cmd = self.build_args_to_regress_cmd(arg_list)
        self.build_for_suite(
            test_suite="regresstest",
            test_script_name="test/regresstest/run_regress_test.py",
            test_script_path="arkcompiler/ets_runtime",
            out_path=out_path,
            gn_args=gn_args,
            log_file_name=self.REGRESS_TEST_LOG_FILE_NAME,
            args_to_cmd=args_to_regress_test_cmd,
            timeout=timeout,
            run_jit=is_jit,
            run_pgo=is_pgo,
            run_baseline_jit=is_baseline_jit,
            aot_mode=is_aot,
            enable_litecg=is_litecg,
            ignore_list=ignore_list
        )

    def build(self, out_path: str, gn_args: list, arg_list: list):
        if not os.path.exists(out_path):
            print("# mkdir -p {}".format(out_path))
            os.makedirs(out_path)
        if len(arg_list) == 0:
            self.build_for_gn_target(out_path, gn_args, ["default"], self.GN_TARGET_LOG_FILE_NAME)
        elif self.is_dict_flags_match_arg(self.ARG_DICT.get("target").get("workload"), arg_list[0]):
            self.build_for_workload(arg_list, out_path, gn_args, 'workload.log')
        elif self.is_dict_flags_match_arg(self.ARG_DICT.get("target").get("test262"), arg_list[0]):
            self.build_for_test262(out_path, gn_args, arg_list)
        elif self.is_dict_flags_match_arg(self.ARG_DICT.get("target").get("unittest"), arg_list[0]):
            if len(arg_list) > 1:
                print("\033[92m\"unittest\" not support additional arguments.\033[0m\n".format())
                sys.exit(0)
            self.build_for_unittest(out_path, gn_args, self.UNITTEST_LOG_FILE_NAME)
        elif self.is_dict_flags_match_arg(self.ARG_DICT.get("target").get("regresstest"), arg_list[0]):
            self.build_for_regress_test(out_path, gn_args, arg_list)
        else:
            self.build_for_gn_target(out_path, gn_args, arg_list, self.GN_TARGET_LOG_FILE_NAME)
        return

    def parse_timeout(self, arg_list) -> Tuple[Optional[Union[str, int]], List[str]]:
        return self.parse_option(arg_list, option_name="--timeout", default_value=400000)

    def match_options(self, arg_list: list, out_path: str) -> [list, list]:
        arg_list_ret = []
        gn_args_ret = []
        for arg in arg_list:
            # match [option][clean] flag
            if self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("clean"), arg):
                self.clean(out_path)
                sys.exit(0)
            # match [option][clean-continue] flag
            elif self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("clean-continue"), arg):
                if not self.has_cleaned:
                    self.clean(out_path)
                    self.has_cleaned = True
            # match [option][gn-args] flag
            elif self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("gn-args"), arg):
                gn_args_ret.append(arg[(arg.find("=") + 1):])
            # match [option][keepdepfile] flag
            elif self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("keepdepfile"), arg):
                if not self.enable_keepdepfile:
                    self.enable_keepdepfile = True
            # match [option][verbose] flag
            elif self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("verbose"), arg):
                if not self.enable_verbose:
                    self.enable_verbose = True
            # match [option][keep-going] flag
            elif self.is_dict_flags_match_arg(self.ARG_DICT.get("option").get("keep-going"), arg):
                if self.ignore_errors == 1:
                    input_value = arg[(arg.find("=") + 1):]
                    try:
                        self.ignore_errors = int(input_value)
                    except Exception as _:
                        print("\033[92mIllegal value \"{}\" for \"--keep-going\" argument.\033[0m\n".format(
                            input_value
                        ))
                        sys.exit(0)
            # make a new list with flag that do not match any flag in [option]
            else:
                arg_list_ret.append(arg)
        return [arg_list_ret, gn_args_ret]

    def build_for_workload(self, arg_list, out_path, gn_args, log_file_name):
        root_dir = os.path.dirname(os.path.abspath(__file__))
        report = False
        tools = 'dev'
        boundary_value = '-10'
        run_count = '10'
        code_v = ''
        run_interpreter = False
        if len(arg_list) >= 2 and arg_list[1] == 'report':
            report = True
        if len(arg_list) >= 3 and arg_list[2]:
            tools = arg_list[2]
        if len(arg_list) >= 4 and arg_list[3]:
            boundary_value = arg_list[3]
        if len(arg_list) >= 5 and arg_list[4]:
            run_count = arg_list[4]
        if len(arg_list) >= 6 and arg_list[5]:
            code_v = arg_list[5]
        if len(arg_list) >= 7 and arg_list[6] == '--run-interpreter':
            run_interpreter = True
        self.build_for_gn_target(out_path, gn_args, ["default"], self.GN_TARGET_LOG_FILE_NAME)
        workload_cmd = "cd arkcompiler/ets_runtime/test/workloadtest/ && python3 work_load.py" \
                       " --code-path {0}" \
                       " --report {1}" \
                       " --tools-type {2}" \
                       " --boundary-value {3}" \
                       " --run-count {4}" \
                       " --code-v {5}" \
            .format(root_dir, report, tools, boundary_value, run_count, code_v)
        if run_interpreter:
            workload_cmd += " --run-interpreter true"
        workload_log_path = os.path.join(out_path, log_file_name)
        str_to_workload_log = "================================\nwokload_time: {0}\nwokload_target: {1}\n\n".format(
            str_of_time_now(), 'file')
        _write(workload_log_path, str_to_workload_log, "a")
        print("=== workload start ===")
        code = call_with_output(workload_cmd, workload_log_path)
        if code != 0:
            print("=== workload fail! ===\n")
            sys.exit(code)
        print("=== workload success! ===\n")
        return

    def start_for_matched_os_cpu_mode(self, os_cpu_key: str, mode_key: str, arg_list: list):
        # get binary gn and ninja
        self.get_binaries()
        # get out_path
        name_of_out_dir_of_second_level = \
            self.ARG_DICT.get("os_cpu").get(os_cpu_key).get("prefix_of_name_of_out_dir_of_second_level") + \
            self.DELIMITER_FOR_SECOND_OUT_DIR_NAME + \
            self.ARG_DICT.get("mode").get(mode_key).get("suffix_of_name_of_out_dir_of_second_level")
        out_path = os.path.join(self.NAME_OF_OUT_DIR_OF_FIRST_LEVEL, name_of_out_dir_of_second_level)
        # match [option] flag
        [arg_list, gn_args] = self.match_options(arg_list, out_path)
        # get expression which would be written to args.gn file
        gn_args.extend(self.ARG_DICT.get("os_cpu").get(os_cpu_key).get("gn_args"))
        gn_args.extend(self.ARG_DICT.get("mode").get(mode_key).get("gn_args"))
        # start to build
        self.build(out_path, gn_args, arg_list)
        return

    def __purge_arg_list(self, option_name: str, arg_list: List[Any]) -> Tuple[bool, List[Any]]:
        if option_name in arg_list:
            arg_list.remove(option_name)
            return True, arg_list
        return False, arg_list


if __name__ == "__main__":
    ark_py = ArkPy()
    ark_py.__main__(sys.argv[1:])

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

import argparse
import hashlib
import json
import os
import sys


def __check_changes(output_file, content):
    if os.path.exists(output_file) and os.path.isfile(output_file):
        # file content md5 val
        sha256_obj = hashlib.sha256()
        sha256_obj.update(str(read_json_file(output_file)).encode())
        hash_value = sha256_obj.hexdigest()
        # new content md5 val
        sha256_obj_new = hashlib.sha256()
        sha256_obj_new.update(str(content).encode())
        hash_value_new = sha256_obj_new.hexdigest()
        if hash_value_new == hash_value:
            return False
    return True


# Read json file data
def read_json_file(input_file):
    if not os.path.exists(input_file):
        print("file '{}' doesn't exist.".format(input_file))
        return None

    data = None
    try:
        with open(input_file, 'r') as input_f:
            data = json.load(input_f)
    except json.decoder.JSONDecodeError:
        print("The file '{}' format is incorrect.".format(input_file))
        raise
    return data


# Write json file data
def write_json_file(output_file, content, check_changes=False):
    file_dir = os.path.dirname(os.path.abspath(output_file))
    if not os.path.exists(file_dir):
        os.makedirs(file_dir, exist_ok=True)

    if check_changes is True:
        changed = __check_changes(output_file, content)
    else:
        changed = True
    if changed is True:
        with open(output_file, 'w') as output_f:
            json.dump(content, output_f, sort_keys=True, indent=2)


def get_full_path_from_target_name(config_info, target_name) -> str:
    inner_kits = config_info["component"]["build"]["inner_kits"]
    for inner_kit in inner_kits:
        inner_kit_parts = inner_kit["name"].split(":")
        if inner_kit_parts[1].startswith(target_name):
            return inner_kit["name"]
    print("Attemp to get a target({}) which is not in the component's inner_kits!".format(target_name))
    sys.exit(1)
    return ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root-src-dir", required=True)
    parser.add_argument("--external-deps-temp-file", required=True)
    parser.add_argument("--external-deps", nargs='+', required=True)
    args = parser.parse_args()

    deps = []
    for dep in args.external_deps:
        if dep.startswith("ets_runtime"):
            config_info = read_json_file("{}arkcompiler/ets_runtime/bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("runtime_core"):
            config_info = read_json_file("{}arkcompiler/runtime_core/bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("ets_frontend"):
            config_info = read_json_file("{}arkcompiler/ets_frontend/bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("toolchain"):
            config_info = read_json_file("{}arkcompiler/toolchain/bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("libuv"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/" \
              "third_party_gn/libuv/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("bounds_checking_function"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "bounds_checking_function/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("icu"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "icu/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("cJSON"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "cJSON/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("openssl"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "openssl/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("zlib"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "zlib/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        elif dep.startswith("json"):
            config_info = read_json_file("{}arkcompiler/toolchain/build/third_party_gn/" \
                    "json/dummy_bundle.json".format(args.root_src_dir))
            target_name = dep.split(":")[1]
            deps.append(get_full_path_from_target_name(config_info, target_name))
        else:
            print("Component in which the external_dep defined is ommited in the logic of {}!".format(__file__))
            sys.exit(1)
            return

    result = {}
    if deps:
        result['deps'] = deps
    write_json_file(args.external_deps_temp_file, result)
    return 0


if __name__ == "__main__":
    main()

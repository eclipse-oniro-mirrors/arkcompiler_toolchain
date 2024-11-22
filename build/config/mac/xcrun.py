#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2024 Huawei Device Co., Ltd.
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

import argparse
import os
import subprocess
import sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='A script to execute a command via xcrun.')
    parser.add_argument('--stamp', action='store', type=str,
                        help='Write a stamp file to this path on success.')
    parser.add_argument('--developer_dir', required=False,
                        help='Path to Xcode.')
    args, unknown_args = parser.parse_known_args()

    if args.developer_dir:
        os.environ['DEVELOPER_DIR'] = args.developer_dir

    rv = subprocess.check_call(['xcrun'] + unknown_args)
    flags = os.O_RDWR | os.O_CREAT | os.O_EXCL
    modes = stat.S_IWUSR | stat.S_IRUSR
    if rv == 0 and args.stamp:
        if os.path.exists(args.stamp):
            os.unlink(args.stamp)
    with os.fdopen(os.open(args.stamp, flags, modes), 'w+') as fp:
        sys.exit(rv)

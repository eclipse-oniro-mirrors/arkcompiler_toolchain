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

import os
import sys

# Usage: python get_tool_mtime.py path/to/file1.py path/to/file2.py
#
# Prints a GN scope with the variable name being the basename sans-extension
# and the value being the file modification time. A variable is emitted for
# each file argument on the command line.

if __name__ == '__main__':
    for f in sys.argv[1:]:
        variable = os.path.splitext(os.path.basename(f))[0]
        print('%s = %d' % (variable, os.path.getmtime(f)))

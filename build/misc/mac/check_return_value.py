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

"""This program wraps an arbitrary command and prints "1" if the command ran
successfully.
"""

import os
import subprocess
import sys

with open(os.devnull, 'wb') as devnull:
    if not subprocess.call(sys.argv[1:], stdout=devnull, stderr=devnull):
        print(1)
    else:
        print(0)

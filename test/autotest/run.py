#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copyright (c) 2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: Run all test cases.
"""

import logging
import os
import subprocess
import sys

import pytest

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))


def check_python_version():
    required_version = (3, 8, 0)
    current_version = sys.version_info

    if current_version >= required_version:
        logging.info(f"Python version meets requirements: "
                     f"{current_version.major}.{current_version.minor}.{current_version.micro}")
    else:
        logging.error(f"Python version too low: "
                      f"{current_version.major}.{current_version.minor}.{current_version.micro}, "
                      f"needs to be at least 3.8.0")
        sys.exit(1)


def install_requirements(requirements_file=r'.\requirements.txt'):
    try:
        subprocess.check_call([sys.executable, '-m', 'pip', 'install', '-r', requirements_file], timeout=10)
        logging.info(f'Successfully installed dependencies from {requirements_file}')
    except subprocess.CalledProcessError as e:
        logging.error(f'Error occurred while installing dependencies: {e}')
        sys.exit(1)


def clear_log_handler():
    # remove all old handlers of the root logger
    log = logging.getLogger()
    for handler in log.handlers[:]:
        log.removeHandler(handler)


if __name__ == '__main__':
    check_python_version()
    install_requirements()

    clear_log_handler()

    args = sys.argv[1:]

    pytest.main(args)

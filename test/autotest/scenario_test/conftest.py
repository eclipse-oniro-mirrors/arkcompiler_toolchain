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

Description: Fixtures of pytest.
"""

import logging
import os

import pytest

from aw import Application
from aw import Fport
from aw import TaskPool
from aw import WebSocket


@pytest.fixture(scope='class')
def test_suite_debug_01():
    logging.info('running test_suite_debug_01')

    bundle_name = 'com.example.worker'
    hap_name = 'MyApplicationWorker.hap'

    config = {'connect_server_port': 15678,
              'debugger_server_port': 15679,
              'bundle_name': bundle_name,
              'hap_path': rf'{os.path.dirname(__file__)}\..\resource\{hap_name}'}

    pid = Application.launch_application(config['bundle_name'], config['hap_path'], start_mode='-D')
    assert pid != 0, logging.error(f'Pid of {hap_name} is 0!')
    config['pid'] = pid

    Fport.clear_fport()
    Fport.fport_connect_server(config['connect_server_port'], config['pid'], config['bundle_name'])

    config['websocket'] = WebSocket(config['connect_server_port'], config['debugger_server_port'])

    config['taskpool'] = TaskPool()

    return config

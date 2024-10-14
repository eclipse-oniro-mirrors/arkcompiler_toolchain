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

from aw import Application, Fport, WebSocket, TaskPool


@pytest.fixture(scope='class')
def test_suite_worker_01():
    logging.info('running worker_01 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15678,
        'debugger_server_port': 15679,
        'bundle_name': 'com.example.multiWorker01',
        'hap_name': 'MultiWorker01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_01_debug():
    logging.info('running worker_01 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15678,
        'debugger_server_port': 15679,
        'bundle_name': 'com.example.multiWorker01',
        'hap_name': 'MultiWorker01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_02():
    logging.info('running worker_02 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15680,
        'debugger_server_port': 15681,
        'bundle_name': 'com.example.multiWorker02',
        'hap_name': 'MultiWorker02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker02.hap'
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_02_profile():
    logging.info('running worker_02 in profile mode')
    config = {
        'start_mode': '-p profile jsperf',
        'connect_server_port': 15680,
        'debugger_server_port': 15681,
        'bundle_name': 'com.example.multiWorker02',
        'hap_name': 'MultiWorker02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker02.hap'
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_taskpool_01():
    logging.info('running taskpool_01 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15682,
        'debugger_server_port': 15683,
        'bundle_name': 'com.example.taskPool01',
        'hap_name': 'TaskPool01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\TaskPool01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_taskpool_01_debug():
    logging.info('running taskpool_01 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15682,
        'debugger_server_port': 15683,
        'bundle_name': 'com.example.taskPool01',
        'hap_name': 'TaskPool01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\TaskPool01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


def launch_hap(config):
    pid = Application.launch_application(config['bundle_name'], config['hap_path'], start_mode=config['start_mode'])
    assert pid != 0, logging.error(f'Pid of {config["hap_name"]} is 0!')
    config['pid'] = pid

    Fport.clear_fport()
    connect_server_port = Fport.fport_connect_server(config['connect_server_port'],
                                                     config['pid'],
                                                     config['bundle_name'])
    assert connect_server_port > 0, logging.error('Failed to fport connect server for 3 times, '
                                                  'the port is very likely occupied')
    config['connect_server_port'] = connect_server_port

    config['websocket'] = WebSocket(config['connect_server_port'], config['debugger_server_port'])
    config['taskpool'] = TaskPool()
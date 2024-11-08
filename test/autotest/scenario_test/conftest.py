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

from aw import Application, Fport, WebSocket, TaskPool, Utils


@pytest.fixture(scope='class')
def test_suite_main_instance_01_debug():
    logging.info('running main_instance_01 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15700,
        'debugger_server_port': 15701,
        'bundle_name': 'com.example.mainInstance01',
        'hap_name': 'MainInstance01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MainInstance01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_main_instance_02_debug():
    logging.info('running main_instance_02 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15702,
        'debugger_server_port': 15703,
        'bundle_name': 'com.example.mainInstance02',
        'hap_name': 'MainInstance02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MainInstance02.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_main_instance_03_debug():
    logging.info('running main_instance_03 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15704,
        'debugger_server_port': 15705,
        'bundle_name': 'com.example.mainInstance03',
        'hap_name': 'MainInstance03.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MainInstance03.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_main_instance_04_debug():
    logging.info('running main_instance_04 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15706,
        'debugger_server_port': 15707,
        'bundle_name': 'com.example.mainInstance04',
        'hap_name': 'MainInstance04.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MainInstance04.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_08_debug():
    logging.info('running worker_08 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15698,
        'debugger_server_port': 15699,
        'bundle_name': 'com.example.multiWorker08',
        'hap_name': 'MultiWorker08.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker08.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_07_debug():
    logging.info('running worker_07 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15696,
        'debugger_server_port': 15697,
        'bundle_name': 'com.example.multiWorker07',
        'hap_name': 'MultiWorker07.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker07.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_06_debug():
    logging.info('running worker_06 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15694,
        'debugger_server_port': 15695,
        'bundle_name': 'com.example.multiWorker06',
        'hap_name': 'MultiWorker06.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker06.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_05_debug():
    logging.info('running worker_05 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15692,
        'debugger_server_port': 15693,
        'bundle_name': 'com.example.multiWorker05',
        'hap_name': 'MultiWorker05.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker05.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_04_debug():
    logging.info('running worker_04 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15690,
        'debugger_server_port': 15691,
        'bundle_name': 'com.example.multiWorker04',
        'hap_name': 'MultiWorker04.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker04.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_03_debug():
    logging.info('running worker_03 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15688,
        'debugger_server_port': 15689,
        'bundle_name': 'com.example.multiWorker03',
        'hap_name': 'MultiWorker03.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker03.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_02_dumpheap():
    logging.info('running worker_02 in profile mode')
    config = {
        'start_mode': '-p dumpheap',
        'connect_server_port': 15686,
        'debugger_server_port': 15687,
        'bundle_name': 'com.example.multiWorker02',
        'hap_name': 'MultiWorker02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker02.hap'
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_02_profile_jsperf():
    logging.info('running worker_02 in profile mode')
    config = {
        'start_mode': '-p profile jsperf',
        'connect_server_port': 15686,
        'debugger_server_port': 15687,
        'bundle_name': 'com.example.multiWorker02',
        'hap_name': 'MultiWorker02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker02.hap'
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_02():
    logging.info('running worker_02 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15684,
        'debugger_server_port': 15685,
        'bundle_name': 'com.example.multiWorker02',
        'hap_name': 'MultiWorker02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\MultiWorker02.hap'
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_worker_01_debug():
    logging.info('running worker_01 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15682,
        'debugger_server_port': 15683,
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
def test_suite_worker_01():
    logging.info('running worker_01 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15680,
        'debugger_server_port': 15681,
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
def test_suite_native_02_debug():
    logging.info('running native_02 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15678,
        'debugger_server_port': 15679,
        'bundle_name': 'com.example.native02',
        'hap_name': 'Native02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\Native02.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
            'worker': 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_native_01_debug():
    logging.info('running native_01 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15676,
        'debugger_server_port': 15677,
        'bundle_name': 'com.example.native01',
        'hap_name': 'Native01.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\Native01.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_taskpool_02_debug():
    logging.info('running taskpool_02 in debug mode')
    config = {
        'start_mode': '-D',
        'connect_server_port': 15674,
        'debugger_server_port': 15675,
        'bundle_name': 'com.example.taskPool02',
        'hap_name': 'TaskPool02.hap',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\TaskPool02.hap',
        'file_path': {
            'entry_ability': 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts',
            'index': 'entry|entry|1.0.0|src/main/ets/pages/Index.ts',
        }
    }
    launch_hap(config)
    return config


@pytest.fixture(scope='class')
def test_suite_taskpool_01():
    logging.info('running taskpool_01 in default mode')
    config = {
        'start_mode': None,
        'connect_server_port': 15672,
        'debugger_server_port': 15673,
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
        'connect_server_port': 15670,
        'debugger_server_port': 15671,
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
def test_suite_hotreload_har_hsp_01():
    logging.info('running application HotReloadHarHsp01')
    config = {
        'bundle_name': 'com.example.HotReloadHarHsp01',
        'hap_dir': rf'{os.path.dirname(__file__)}\..\resource\HotReloadHarHsp01',
        'hap_entry_name': 'HotReloadHarHsp01.hap',
        'hsp_discovery_name': 'HspDiscovery.hsp',

        'local_hqf_entry_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadHarHsp01.hqf',
        'remote_hqf_entry_path': '/data/HotReloadHarHsp01.hqf',
        'hqf_entry_name': 'HotReloadHarHsp01.hqf',

        'local_hqf_discovery_path': rf'{os.path.dirname(__file__)}\..\resource\HspDiscovery.hqf',
        'remote_hqf_discovery_path': '/data/HspDiscovery.hqf',
        'hqf_discovery_name': 'HspDiscovery.hap',
    }
    pid = Application.launch_application(config['bundle_name'], config['hap_dir'])
    assert pid != 0, logging.error(f'Pid of {config["bundle_name"]} is 0!')
    config['pid'] = pid
    return config


@pytest.fixture(scope='class')
def test_suite_hotreload_pages_01():
    logging.info('running application HotReloadPages01')
    config = {
        'bundle_name': 'com.example.HotReloadPages01',
        'hap_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadPages01.hap',

        'hqf_01_name': 'HotReloadPages01.01.hqf',
        'local_hqf_01_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadPages01.01.hqf',
        'remote_hqf_01_path': '/data/HotReloadPages01.01.hqf',

        'hqf_02_name': 'HotReloadPages01.02.hqf',
        'local_hqf_02_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadPages01.02.hqf',
        'remote_hqf_02_path': '/data/HotReloadPages01.02.hqf',

        'hqf_03_name': 'HotReloadPages01.03.hqf',
        'local_hqf_03_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadPages01.03.hqf',
        'remote_hqf_03_path': '/data/HotReloadPages01.03.hqf',

        'hqf_04_name': 'HotReloadPages01.04.hqf',
        'local_hqf_04_path': rf'{os.path.dirname(__file__)}\..\resource\HotReloadPages01.04.hqf',
        'remote_hqf_04_path': '/data/HotReloadPages01.04.hqf',
    }
    pid = Application.launch_application(config['bundle_name'], config['hap_path'])
    assert pid != 0, logging.error(f'Pid of {config["bundle_name"]} is 0!')
    config['pid'] = pid
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

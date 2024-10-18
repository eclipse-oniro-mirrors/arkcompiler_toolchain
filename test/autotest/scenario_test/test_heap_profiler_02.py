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

Description: Scenario test case.
"""

import logging
import os
import time

import pytest

from aw import Application
from aw import Utils
from aw.api import debugger_api, runtime_api, heap_profiler_api


@pytest.mark.heap_profiler
@pytest.mark.timeout(40)
class TestHeapProfiler02:
    """
    测试用例：多实例内存调优 HeapDump 录制
    测试步骤：
        1.  拉起应用，attach 主线程
        2.  连接 connect server 和主线程 debugger server
        3.  连接 worker 线程 debugger server
        4.  所有线程使能 Runtime（Runtime.enable）
        5.  所有线程去使能 Debugger（Debugger.disable）
        5.  所有线程拍摄内存快照（HeapProfiler.takeHeapSnapshot）
        6.  等待 10 秒后，所有线程再次拍摄内存快照（HeapProfiler.takeHeapSnapshot）
        7.  销毁 worker 线程，对应的 debugger server 连接断开
        8.  关闭主线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestHeapProfiler02: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_heap_profiler_02.hilog.txt'
        self.id_generator = Utils.message_id_generator()

        # receive the hilog before the test start
        Utils.clear_fault_log()
        self.hilog_process, self.write_thread = Utils.save_hilog(log_path=self.log_path,
                                                                 file_name=self.hilog_file_name,
                                                                 debug_on=True)

    def teardown_method(self):
        Application.uninstall(self.config['bundle_name'])

        # terminate the hilog receive process after the test done
        time.sleep(3)
        self.hilog_process.stdout.close()
        self.hilog_process.terminate()
        self.hilog_process.wait()
        self.write_thread.join()

        Utils.save_fault_log(log_path=self.log_path)
        logging.info('TestHeapProfiler02 done')

    def test(self, test_suite_worker_02):
        logging.info('Start running TestHeapProfiler02: test')
        self.config = test_suite_worker_02
        websocket = self.config['websocket']
        taskpool = self.config['taskpool']
        pid = self.config['pid']
        self.debugger_impl = debugger_api.DebuggerImpl(self.id_generator, websocket)
        self.runtime_impl = runtime_api.RuntimeImpl(self.id_generator, websocket)
        self.heap_profiler_impl = heap_profiler_api.HeapProfilerImpl(self.id_generator, websocket)

        Application.attach(self.config['bundle_name'])
        taskpool.submit(websocket.main_task(taskpool, self.procedure, pid))
        taskpool.await_taskpool()
        taskpool.task_join()
        if taskpool.task_exception:
            raise taskpool.task_exception

    async def procedure(self, websocket):
        ################################################################################################################
        # main thread: connect the debugger server
        ################################################################################################################
        main_thread = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], True)
        logging.info(f'Connect to the debugger server of instance: {main_thread.instance_id}')
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        worker_thread_1 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_1.instance_id}')
        worker_thread_2 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_2.instance_id}')
        ################################################################################################################
        # main thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", main_thread)
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", worker_thread_1)
        await self.runtime_impl.send("Runtime.enable", worker_thread_2)
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", main_thread)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread_1)
        await self.debugger_impl.send("Debugger.disable", worker_thread_2)
        ################################################################################################################
        # main thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", main_thread)
        ################################################################################################################
        # worker thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", worker_thread_1)
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", worker_thread_2)
        ################################################################################################################
        # all thread: sleep 10 seconds
        ################################################################################################################
        time.sleep(10)
        ################################################################################################################
        # main thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", main_thread)
        ################################################################################################################
        # worker thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", worker_thread_1)
        await self.heap_profiler_impl.send("HeapProfiler.takeHeapSnapshot", worker_thread_2)
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(worker_thread_1.instance_id, worker_thread_1.send_msg_queue,
                                                    'close')
        await websocket.send_msg_to_debugger_server(worker_thread_2.instance_id, worker_thread_2.send_msg_queue,
                                                    'close')
        await websocket.send_msg_to_debugger_server(main_thread.instance_id, main_thread.send_msg_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
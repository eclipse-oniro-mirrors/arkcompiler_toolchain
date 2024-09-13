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

import json
import logging
import os
import time

import pytest

from aw import Application
from aw import Utils
from aw import communicate_with_debugger_server
from aw import debugger, runtime, heap_profiler


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

        Application.attach(self.config['bundle_name'])
        taskpool.submit(websocket.main_task(taskpool, websocket, self.procedure, pid))
        taskpool.await_taskpool()
        taskpool.task_join()
        if taskpool.task_exception:
            raise taskpool.task_exception

    async def procedure(self, websocket):
        ################################################################################################################
        # main thread: connect the debugger server
        ################################################################################################################
        send_msg = {"type": "connected"}
        await websocket.send_msg_to_connect_server(send_msg)
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'addInstance'
        assert response['instanceId'] == 0, logging.error('instance id of the main thread not equal to 0')
        assert response['tid'] == self.config['pid']
        main_thread_instance_id = await websocket.get_instance()
        main_thread_to_send_queue = websocket.to_send_msg_queues[main_thread_instance_id]
        main_thread_received_queue = websocket.received_msg_queues[main_thread_instance_id]
        logging.info(f'Connect to the debugger server of instance: {main_thread_instance_id}')
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        workers_num = 2
        worker_instances_id = []
        worker_thread_to_send_queues = []
        worker_thread_received_queues = []
        for i in range(workers_num):
            response = await websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'addInstance'
            assert response['instanceId'] != 0
            assert response['tid'] != self.config['pid']
            assert 'workerThread_' in response['name']
            worker_instance_id = await websocket.get_instance()
            worker_instances_id.append(worker_instance_id)
            worker_thread_to_send_queues.append(websocket.to_send_msg_queues[worker_instance_id])
            worker_thread_received_queues.append(websocket.received_msg_queues[worker_instance_id])
            logging.info(f'Connect to the debugger server of instance: {worker_instance_id}')
        ################################################################################################################
        # main thread: Runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              runtime.enable(), message_id)
            assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              debugger.disable(), message_id)
            while not json.loads(response).get('id', None):
                response = await websocket.recv_msg_of_debugger_server(worker_instances_id[i],
                                                                       worker_thread_received_queues[i])
            assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          heap_profiler.take_heap_snapshot(), message_id)
        assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
        pre_response = response
        while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
            pre_response = response
            response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                                   main_thread_received_queue)
        assert pre_response.endswith(r'\n]\n}\n"}}')
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              heap_profiler.take_heap_snapshot(), message_id)
            assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
            pre_response = response
            while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
                pre_response = response
                response = await websocket.recv_msg_of_debugger_server(worker_instances_id[i],
                                                                       worker_thread_received_queues[i])
            assert pre_response.endswith(r'\n]\n}\n"}}')
            assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # all thread: sleep 10 seconds
        ################################################################################################################
        time.sleep(10)
        ################################################################################################################
        # main thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          heap_profiler.take_heap_snapshot(), message_id)
        assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
        pre_response = response
        while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
            pre_response = response
            response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                                   main_thread_received_queue)
        assert pre_response.endswith(r'\n]\n}\n"}}')
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: HeapProfiler.takeHeapSnapshot
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              heap_profiler.take_heap_snapshot(), message_id)
            assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
            pre_response = response
            while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
                pre_response = response
                response = await websocket.recv_msg_of_debugger_server(worker_instances_id[i],
                                                                       worker_thread_received_queues[i])
            assert pre_response.endswith(r'\n]\n}\n"}}')
            assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        for i in range(workers_num):
            await websocket.send_msg_to_debugger_server(worker_instances_id[i],
                                                        worker_thread_to_send_queues[i], 'close')
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(main_thread_instance_id, main_thread_to_send_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
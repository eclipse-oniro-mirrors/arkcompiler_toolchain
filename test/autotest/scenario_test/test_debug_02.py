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
from aw import debugger, runtime


@pytest.mark.debug
@pytest.mark.timeout(30)
class TestDebug02:
    """
    测试用例：多实例 attach 调试
    测试步骤：
        1.  拉起应用，attach 主线程
        2.  连接 connect server 和主线程 debugger server
        3.  创建两个子线程，连接子线程 debugger server
        4.  所有线程使能 Runtime 和 Debugger
        5.  主线程 Index.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        6.  触发点击事件，主线程命中断点
        7.  销毁其中一个子线程
        8.  子线程 Worker.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        9.  主线程 resume，暂停在下一断点（Debugger.resume）
        10. 重新创建一个子线程，使能并设置断点
        11. 主线程 resume，发送消息给子线程，主线程暂停在下一断点（Debugger.resume）
        12. 子线程命中断点后 stepOut，发消息给主线程（Debugger.stepOut）
        13. 主线程 stepOver，发送消息给另一子线程，主线程暂停在下一行（Debugger.stepOver）
        14. 子线程命中断点后 resume，发消息给主线程（Debugger.resume）
        15. 销毁所有子线程，对应的 debugger server 连接断开
        16. 关闭主线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestDebug02: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_debug_02.hilog.txt'
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
        logging.info('TestDebug02 done')

    def test(self, test_suite_worker_01):
        logging.info('Start running TestDebug02: test')
        self.config = test_suite_worker_01
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
        # worker thread: Debugger.enable
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              debugger.enable(), message_id)
            response = json.loads(response)
            assert response['method'] == 'Debugger.scriptParsed'
            assert response['params']['url'] == self.config['file_path']['worker']
            assert response['params']['endLine'] == 0
            response = await websocket.recv_msg_of_debugger_server(worker_instances_id[i],
                                                                   worker_thread_received_queues[i])
            assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                         "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        for i in range(workers_num):
            message_id = next(self.id_generator)
            response = await communicate_with_debugger_server(worker_instances_id[i],
                                                              worker_thread_to_send_queues[i],
                                                              worker_thread_received_queues[i],
                                                              runtime.run_if_waiting_for_debugger(), message_id)
            assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.enable(), message_id)
        assert json.loads(response)['method'] == 'Debugger.scriptParsed'
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response)['method'] == 'Debugger.scriptParsed'
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                     "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # main thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          runtime.run_if_waiting_for_debugger(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.remove_breakpoints_by_url(
                                                              self.config['file_path']['index']),
                                                          message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=53),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=57)]
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:53:0:' + self.config['file_path']['index']
        assert response['result']['locations'][1]['id'] == 'id:57:0:' + self.config['file_path']['index']
        ################################################################################################################
        # main thread: click on the screen
        ################################################################################################################
        Application.click_on_middle()
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:53:16:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'destroyInstance'
        if response['instanceId'] == worker_instances_id[0]:
            worker_instances_id[0] = worker_instances_id[1]
            worker_thread_to_send_queues[0] = worker_thread_to_send_queues[1]
            worker_thread_received_queues[0] = worker_thread_received_queues[1]
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[0],
                                                          worker_thread_to_send_queues[0],
                                                          worker_thread_received_queues[0],
                                                          debugger.remove_breakpoints_by_url(
                                                              self.config['file_path']['worker']),
                                                          message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=11)]
        response = await communicate_with_debugger_server(worker_instances_id[0],
                                                          worker_thread_to_send_queues[0],
                                                          worker_thread_received_queues[0],
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:11:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:57:20:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'addInstance'
        assert response['instanceId'] != 0
        assert response['tid'] != self.config['pid']
        assert 'workerThread_' in response['name']
        worker_instances_id[1] = await websocket.get_instance()
        worker_thread_to_send_queues[1] = websocket.to_send_msg_queues[worker_instances_id[1]]
        worker_thread_received_queues[1] = websocket.received_msg_queues[worker_instances_id[1]]
        logging.info(f'Connect to the debugger server of instance: {worker_instances_id[1]}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                     "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          runtime.run_if_waiting_for_debugger(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        response = json.loads(response)
        assert response['method'] == 'Debugger.scriptParsed'
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.remove_breakpoints_by_url(
                                                              self.config['file_path']['worker']),
                                                          message_id)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=11)]
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:11:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:57:20:' + self.config['file_path']['index']]
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[0],
                                                               worker_thread_received_queues[0])
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[0],
                                                          worker_thread_to_send_queues[0],
                                                          worker_thread_received_queues[0],
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[0],
                                                               worker_thread_received_queues[0])
        assert json.loads(response) == {"id": message_id, "result": {}}
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[0],
                                                               worker_thread_received_queues[0])
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:11:4:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.stepOut
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[0],
                                                          worker_thread_to_send_queues[0],
                                                          worker_thread_received_queues[0],
                                                          debugger.step_out(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[0],
                                                               worker_thread_received_queues[0])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[0],
                                                          worker_thread_to_send_queues[0],
                                                          worker_thread_received_queues[0],
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[0],
                                                               worker_thread_received_queues[0])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.stepOver
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:11:4:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instances_id[1],
                                                          worker_thread_to_send_queues[1],
                                                          worker_thread_received_queues[1],
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instances_id[1],
                                                               worker_thread_received_queues[1])
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        for i in range(workers_num):
            response = await websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'destroyInstance'
            assert response['instanceId'] in worker_instances_id
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(main_thread_instance_id, main_thread_to_send_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
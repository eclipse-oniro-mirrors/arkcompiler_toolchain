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
class TestDebug03:
    """
    测试用例：多 task 实例 debug 调试
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  连接子线程 debugger server，用于执行 task 任务
        4.  子线程使能 Runtime 和 Debugger
        5.  主线程 Index.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        6.  触发点击事件，主线程命中断点
        7.  子线程 Index.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        8.  子线程 resume，命中断点（Debugger.resume）
        9.  子线程 getProperties，返回给定对象的属性（Runtime.getProperties）
        10. 子线程 stepOut，主线程命中断点（Debugger.stepOut）
        11. 主线程 getProperties（Runtime.getProperties）
        12. 主线程 resume（Debugger.resume）
        13. 子线程命中断点后 resume（Debugger.resume）
        14. 关闭所有线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestDebug03: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_debug_03.hilog.txt'
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
        logging.info('TestDebug03 done')

    def test(self, test_suite_taskpool_01_debug):
        logging.info('Start running TestDebug03: test')
        self.config = test_suite_taskpool_01_debug
        websocket = self.config['websocket']
        taskpool = self.config['taskpool']
        pid = self.config['pid']

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
        # main thread: Runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # main thread: Debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.enable(), message_id)
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
        # main thread: Debugger.scriptParsed
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.scriptParsed'
        assert response['params']['url'] == self.config['file_path']['entry_ability']
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['entry_ability']
        assert response['params']['reason'] == 'Break on start'
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
        # main thread: Debugger.scriptParsed
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.scriptParsed'
        assert response['params']['url'] == self.config['file_path']['index']
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'Break on start'
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
        # worker thread: connect the debugger server
        ################################################################################################################
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'addInstance'
        assert response['instanceId'] != 0
        assert response['tid'] != self.config['pid']
        assert 'workerThread_' in response['name']
        worker_instance_id = await websocket.get_instance()
        worker_thread_to_send_queue = websocket.to_send_msg_queues[worker_instance_id]
        worker_thread_received_queue = websocket.received_msg_queues[worker_instance_id]
        logging.info(f'Connect to the debugger server of instance: {worker_instance_id}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                     "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
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
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=10),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=17),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=25)]
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:10:0:' + self.config['file_path']['index']
        assert response['result']['locations'][1]['id'] == 'id:17:0:' + self.config['file_path']['index']
        assert response['result']['locations'][2]['id'] == 'id:25:0:' + self.config['file_path']['index']
        ################################################################################################################
        # main thread: click on the screen
        ################################################################################################################
        Application.click_on_middle()
        ################################################################################################################
        # worker thread: Debugger.scriptParsed
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.scriptParsed'
        assert response['params']['url'] == self.config['file_path']['index']
        assert response['params']['endLine'] == 0
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.remove_breakpoints_by_url(
                                                              self.config['file_path']['index']),
                                                          message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=10),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=17),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=25)]
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:10:0:' + self.config['file_path']['index']
        assert response['result']['locations'][1]['id'] == 'id:17:0:' + self.config['file_path']['index']
        assert response['result']['locations'][2]['id'] == 'id:25:0:' + self.config['file_path']['index']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:10:14:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          runtime.get_properties(object_id='0',
                                                                                 own_properties=True,
                                                                                 accessor_properties_only=False,
                                                                                 generate_preview=True),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['result'][0]['name'] == 'add'
        assert response['result']['result'][0]['value']['type'] == 'function'
        ################################################################################################################
        # worker thread: Debugger.stepOut
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.step_out(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:25:4:' + self.config['file_path']['index']]
        ################################################################################################################
        # main thread: Runtime.getProperties
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          runtime.get_properties(object_id='0',
                                                                                 own_properties=True,
                                                                                 accessor_properties_only=False,
                                                                                 generate_preview=True),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['result'][0]['name'] == 'taskpoolTest'
        assert response['result']['result'][0]['value']['type'] == 'function'
        assert response['result']['result'][1]['name'] == 'valueSub'
        assert response['result']['result'][1]['value']['type'] == 'undefined'
        assert response['result']['result'][2]['name'] == 'valueAdd'
        assert response['result']['result'][2]['value']['type'] == 'number'
        assert response['result']['result'][2]['value']['description'] == '300'
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
        # worker thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:17:14:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id,
                                                               worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(worker_instance_id, worker_thread_to_send_queue, 'close')
        await websocket.send_msg_to_debugger_server(main_thread_instance_id, main_thread_to_send_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
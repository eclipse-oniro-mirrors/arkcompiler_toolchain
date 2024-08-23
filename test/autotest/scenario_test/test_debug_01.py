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
class TestDebug01:
    """
    测试用例：多实例调试_01
    测试步骤：
        1.  连接 connect server
        2.  连接主线程 debugger server
        3.  主线程文件 Index.ts 设置断点，恢复执行并命中断点
        4.  连接 worker 线程 debugger server
        5.  子线程文件 Worker.ts 设置断点
        6.  主线程 step over，发送消息给子线程，主线程暂停在下一行，子线程命中断点
        7.  子线程 step over，暂停在下一行，随后 getProperties
        8.  主线程 step over，暂停在下一行
        9.  主线程 resume，命中断点
        10. 主线程 step over，暂停在下一行
        11. 主线程 resume， 执行结束
        12. 子线程销毁，对应的 debugger server 连接断开
        13. 关闭主线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestDebug01: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_debug_01.hilog.txt'
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
        logging.info('TestDebug01 done')

    def test(self, test_suite_debug_01):
        logging.info('Start running TestDebug01: test')
        self.config = test_suite_debug_01
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
        # main thread: runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}

        ################################################################################################################
        # main thread: debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                     "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # main thread: runtime.run_if_waiting_for_debugger
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
        assert response['params']['url'] == 'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts'
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert (response['params']['callFrames'][0]['url'] ==
                'entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts')
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
        assert response['params']['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # main thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        url = 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.remove_breakpoints_by_url(url), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url='entry|entry|1.0.0|src/main/ets/pages/Index.ts', line_number=22),
                     debugger.BreakLocationUrl(url='entry|entry|1.0.0|src/main/ets/pages/Index.ts', line_number=26)]
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:22:0:entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['result']['locations'][1]['id'] == 'id:26:0:entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id,
                                                               main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['hitBreakpoints'] == ["id:22:4:entry|entry|1.0.0|src/main/ets/pages/Index.ts"]
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
        # main thread: Runtime.getProperties.
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
        assert response['result']['result'][0]['name'] == 'newWorker'
        assert response['result']['result'][0]['value']['type'] == 'function'
        ################################################################################################################
        # worker thread: runtime.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          runtime.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"protocols": []}}
        ################################################################################################################
        # worker thread: debugger.enable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.enable(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {"debuggerId": "0",
                                                                     "protocols": Utils.get_custom_protocols()}}
        ################################################################################################################
        # worker thread: runtime.run_if_waiting_for_debugger
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          runtime.run_if_waiting_for_debugger(), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.scriptParsed
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.scriptParsed'
        assert response['params']['url'] == 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # worker thread: Debugger.paused
        ################################################################################################################
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        url = 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.remove_breakpoints_by_url(url), message_id)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        message_id = next(self.id_generator)
        locations = [debugger.BreakLocationUrl(url='entry|entry|1.0.0|src/main/ets/workers/Worker.ts', line_number=17),
                     debugger.BreakLocationUrl(url='entry|entry|1.0.0|src/main/ets/workers/Worker.ts', line_number=20)]
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert response['result']['locations'][0]['id'] == 'id:17:0:entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        assert response['result']['locations'][1]['id'] == 'id:20:0:entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # main thread: step over
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ["id:17:8:entry|entry|1.0.0|src/main/ets/workers/Worker.ts"]
        ################################################################################################################
        # worker thread: step over
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # worker thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/workers/Worker.ts'
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Runtime.getProperties.
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
        assert response['result']['result'][0]['name'] == ''
        assert response['result']['result'][0]['value']['type'] == 'function'
        assert response['result']['result'][1]['name'] == 'str'
        assert response['result']['result'][1]['value']['type'] == 'string'
        assert response['result']['result'][2]['name'] == 'e'
        assert response['result']['result'][2]['value']['type'] == 'object'
        ################################################################################################################
        # main thread: step over
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['location']['lineNumber'] == 25
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['callFrames'][1]['location']['lineNumber'] == 40
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['location']['lineNumber'] == 26
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:26:8:entry|entry|1.0.0|src/main/ets/pages/Index.ts']
        ################################################################################################################
        # main thread: step over
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        # main thread: Debugger.paused
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        assert response['params']['callFrames'][0]['location']['lineNumber'] == 27
        assert response['params']['callFrames'][0]['url'] == 'entry|entry|1.0.0|src/main/ets/pages/Index.ts'
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(worker_instance_id,
                                                          worker_thread_to_send_queue,
                                                          worker_thread_received_queue,
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(worker_instance_id, worker_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'destroyInstance'
        assert response['instanceId'] == worker_instance_id
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        message_id = next(self.id_generator)
        response = await communicate_with_debugger_server(main_thread_instance_id,
                                                          main_thread_to_send_queue,
                                                          main_thread_received_queue,
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(main_thread_instance_id, main_thread_received_queue)
        assert json.loads(response) == {"id": message_id, "result": {}}
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(main_thread_instance_id, main_thread_to_send_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################

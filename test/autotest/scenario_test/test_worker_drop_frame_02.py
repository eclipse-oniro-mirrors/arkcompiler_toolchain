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
from aw import debugger
from aw.api import debugger_api, runtime_api


@pytest.mark.debug
@pytest.mark.timeout(80)
class TestWorkerDropFrame02:
    """
    测试用例：多线程 debug 调试之 drop frame
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  主线程 resume（Debugger.resume）
        4.  触发点击事件，创建子线程，连接子线程 debugger server
        5.  子线程使能 Runtime 和 Debugger
        6.  创建 taskpool 线程，连接 debugger server,使能 Runtime 和 Debugger
        7.  子线程 Worker.ets 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        8.  子线程 resume，停在断点处（Debugger.resume）
        9.  子线程 evaluateOnCallFrame，观察指定变量的值（Debugger.evaluateOnCallFrame）
        10. 子线程时光调试，回到方法调用前（Debugger.dropFrame）
        11. 子线程 evaluateOnCallFrame，观察指定变量的值是否变化（Debugger.evaluateOnCallFrame）
        12. 子线程重复步骤 6-10，测试 dropFrame 在不同方法内的执行情况
        13. 执行到 taskpool 任务时切换到 taskpool 线程进行 dropFrame 操作（Debugger.dropFrame）
        14. 所有线程去使能 debugger（Debugger.disable）
        15. 关闭所有线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestWorkerDropFrame02: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_worker_drop_frame_02.hilog.txt'
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
        logging.info('TestWorkerDropFrame02 done')

    def test(self, test_suite_worker_07_debug):
        logging.info('Start running TestWorkerDropFrame02: test')
        self.config = test_suite_worker_07_debug
        websocket = self.config['websocket']
        taskpool = self.config['taskpool']
        pid = self.config['pid']
        self.debugger_impl = debugger_api.DebuggerImpl(self.id_generator, websocket)
        self.runtime_impl = runtime_api.RuntimeImpl(self.id_generator, websocket)

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
        # main thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", main_thread)
        ################################################################################################################
        # main thread: Debugger.enable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.enable", main_thread)
        ################################################################################################################
        # main thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", main_thread)
        ################################################################################################################
        # main thread: Debugger.scriptParsed
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.scriptParsed", main_thread)
        assert response['params']['url'] == self.config['file_path']['entry_ability']
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['entry_ability']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # main thread: Debugger.scriptParsed
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.scriptParsed", main_thread)
        assert response['params']['url'] == self.config['file_path']['index']
        assert response['params']['endLine'] == 0
        ################################################################################################################
        # main thread: Debugger.paused
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # main thread: click on the screen
        ################################################################################################################
        Application.click_on_middle()
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        worker_thread_1 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_1.instance_id}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", worker_thread_1)
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.enable", worker_thread_1)
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", worker_thread_1)
        ################################################################################################################
        # worker thread: Debugger.scriptParsed
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.scriptParsed", worker_thread_1)
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        worker_thread_2 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_2.instance_id}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.enable", worker_thread_2)
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=64)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:64:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:64:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        P1_INTRODUCE = (
            'UEFOREEAAAAAAAAADAACAGQBAAAAAAAAAAAAAAIAAAA8AAAAAQAAAGABAAAAAAAARAAAAAEAAABEAAAAoQAAANQAAACAAAAAZAEAAAIAAA'
            'BsAAAAAwAAAHQAAAD/////////////////////oQAAANQAAACAAAAAkgAAAJ0AAAAhZGVidWdnZXJHZXRWYWx1ZQATaW50cm9kdWNlAAVw'
            'MQAzTF9FU1Nsb3ROdW1iZXJBbm5vdGF0aW9uOwAAAAAAgUAAAAIAABdmdW5jX21haW5fMAATTF9HTE9CQUw7AAAAAAABAAECAAABAP//xw'
            'AAAIgCAR4BAAACAAVWAQAABhEBAAAAFVNsb3ROdW1iZXIAAAABAAUBAAAIAAAANwoDLwBEoESxRMJtYQZgBkIAAABhBz4CAGEIAmEJYAcr'
            'AggJYQVgBUIEAQBhBGAELQYFZAtrAQ8A/////w8AAgAoAFEBAAA=')
        ARR_JOIN = (
            'UEFOREEAAAAAAAAADAACAGABAAAAAAAAAAAAAAIAAAA8AAAAAQAAAFwBAAAAAAAARAAAAAEAAABEAAAAnQAAANAAAACAAAAAYAEAAAIAAA'
            'BsAAAAAwAAAHQAAAD/////////////////////nQAAANAAAACAAAAAhQAAAJcAAAAHYXJyACFkZWJ1Z2dlckdldFZhbHVlAAlqb2luADNM'
            'X0VTU2xvdE51bWJlckFubm90YXRpb247AAAAAACBQAAAAgAAF2Z1bmNfbWFpbl8wABNMX0dMT0JBTDsAAAAAAAEAAQIAAAEA///DAAAAiA'
            'IBGgEAAAIABVIBAAAGDQEAAAAVU2xvdE51bWJlcgAAAAEAAQEAAAgAAAA3CgMvAESgRLFEwm1hBmAGQgABAGEHPgAAYQgCYQlgBysCCAlh'
            'BWAFQgQCAGEEYAQtBgVkC2sBDwD/////DwACACgATQEAAA==')
        params = debugger.EvaluateOnCallFrameParams(P1_INTRODUCE)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "name = Kate; age = 10"
        params = debugger.EvaluateOnCallFrameParams('map')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'].startswith("Map(2) {0 => 'str0', 1 => 'str1'}")
        params = debugger.EvaluateOnCallFrameParams(ARR_JOIN)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "10,11,2,3"
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams(P1_INTRODUCE)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "name = Kate; age = 10"
        params = debugger.EvaluateOnCallFrameParams('map')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'].startswith("Map(2) {0 => 'str0', 1 => 'str1'}")
        params = debugger.EvaluateOnCallFrameParams(ARR_JOIN)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "10,11,2,3"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=72)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:72:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:72:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams(P1_INTRODUCE)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "name = Tony; age = 30"
        params = debugger.EvaluateOnCallFrameParams('map')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'].startswith("Map(1) {2 => 'str2'}")
        params = debugger.EvaluateOnCallFrameParams(ARR_JOIN)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "4,3,2,1,0"
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams(P1_INTRODUCE)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "name = Kate; age = 10"
        params = debugger.EvaluateOnCallFrameParams('map')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'].startswith("Map(2) {0 => 'str0', 1 => 'str1'}")
        params = debugger.EvaluateOnCallFrameParams(ARR_JOIN)
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result']['description'] == "10,11,2,3"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=79)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:79:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:79:19:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=94)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:94:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:94:22:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=100)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:100:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:100:8:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_1, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_1, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        ################################################################################################################
        # worker thread: Debugger.scriptParsed
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.scriptParsed", worker_thread_2)
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_2, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=127)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_2, params)
        assert response['result']['locations'][0]['id'] == 'id:127:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:127:9:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('ac')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_2, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "20", "description": "20"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_2, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('ac')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_2, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "10", "description": "10"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        response = await self.debugger_impl.send("Debugger.dropFrame", worker_thread_2, params)
        assert response['result']['message'] == 'Not yet support sendable method'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_2, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=111)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_2, params)
        assert response['result']['locations'][0]['id'] == 'id:111:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:111:9:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('ia')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_2, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "200", "description": "200"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_2, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('ia')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_2, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "120", "description": "120"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread_2, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('ia')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread_2, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "100", "description": "100"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_2, params)
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread_1)
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", main_thread)
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(worker_thread_2.instance_id, worker_thread_2.send_msg_queue,
                                                    'close')
        await websocket.send_msg_to_debugger_server(worker_thread_1.instance_id, worker_thread_1.send_msg_queue,
                                                    'close')
        await websocket.send_msg_to_debugger_server(main_thread.instance_id, main_thread.send_msg_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
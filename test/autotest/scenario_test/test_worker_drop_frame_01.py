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
class TestWorkerDropFrame01:
    """
    测试用例：多线程 debug 调试之 drop frame
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  主线程 resume（Debugger.resume）
        4.  触发点击事件，创建子线程，连接子线程 debugger server
        5.  子线程使能 Runtime 和 Debugger
        6.  子线程 Worker.ets 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        7.  子线程 resume，停在断点处（Debugger.resume）
        8.  子线程 evaluateOnCallFrame，观察指定变量的值（Debugger.evaluateOnCallFrame）
        9.  子线程时光调试，回到方法调用前（Debugger.dropFrame）
        10. 子线程 evaluateOnCallFrame，观察指定变量的值是否变化（Debugger.evaluateOnCallFrame）
        11. 子线程重复步骤 6-10，测试 dropFrame 在不同方法内的执行情况
        12. 所有线程去使能 debugger（Debugger.disable）
        13. 关闭所有线程 debugger server 和 connect server 连接
    """

    def setup_method(self):
        logging.info('Start running TestWorkerDropFrame01: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_worker_drop_frame_01.hilog.txt'
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
        logging.info('TestWorkerDropFrame01 done')

    def test(self, test_suite_worker_08_debug):
        logging.info('Start running TestWorkerDropFrame01: test')
        self.config = test_suite_worker_08_debug
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
        worker_thread = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread.instance_id}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", worker_thread)
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.enable", worker_thread)
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", worker_thread)
        ################################################################################################################
        # worker thread: Debugger.scriptParsed
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.scriptParsed", worker_thread)
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=64)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:64:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:64:9:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "4", "description": "4"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('b')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('c')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['type'] == "undefined"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=76)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:76:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:76:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "4", "description": "4"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=90)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:90:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:90:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:90:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "4", "description": "4"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "24", "description": "24"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "4", "description": "4"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=104)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:104:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:104:13:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:104:13:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "26", "description": "26"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "12", "description": "12"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=121)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:121:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:121:9:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('s')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        params = debugger.EvaluateOnCallFrameParams('func')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        function_info = response['result']['result']
        assert function_info['type'] == 'function'
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('s')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['type'] == "undefined"
        params = debugger.EvaluateOnCallFrameParams('func')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['description'] != function_info['description']
        assert response['result']['result']['type'] == 'function'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=136)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:136:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:136:1:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "11", "description": "11"}
        params = debugger.EvaluateOnCallFrameParams('x')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "8", "description": "8"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "9", "description": "9"}
        params = debugger.EvaluateOnCallFrameParams('x')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "7", "description": "7"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "8", "description": "8"}
        params = debugger.EvaluateOnCallFrameParams('x')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "7", "description": "7"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=146)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:146:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:146:5:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "10", "description": "10"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "12", "description": "12"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=160)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:160:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:160:13:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "5", "description": "5"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "12", "description": "12"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['type'] == "undefined"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=171)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:171:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:171:13:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "9", "description": "9"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "22", "description": "22"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "5", "description": "5"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "5", "description": "5"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['type'] == "undefined"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=189)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:189:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:189:13:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "14", "description": "14"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "21", "description": "21"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "10", "description": "10"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "1", "description": "1"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "0", "description": "0"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "10", "description": "10"}
        params = debugger.EvaluateOnCallFrameParams('e')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result']['type'] == "undefined"
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=205)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:205:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:205:9:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "3", "description": "3"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "17", "description": "17"}
        ################################################################################################################
        # worker thread: Debugger.dropFrame
        ################################################################################################################
        params = debugger.DropFrameParams()
        await self.debugger_impl.send("Debugger.dropFrame", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        ################################################################################################################
        # worker thread: Debugger.evaluateOnCallFrame
        ################################################################################################################
        params = debugger.EvaluateOnCallFrameParams('a')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "2", "description": "2"}
        params = debugger.EvaluateOnCallFrameParams('d')
        response = await self.debugger_impl.send("Debugger.evaluateOnCallFrame", worker_thread, params)
        assert response['result']['result'] == {"type": "number", "unserializableValue": "14", "description": "14"}
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread, params)
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread)
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", main_thread)
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(worker_thread.instance_id, worker_thread.send_msg_queue, 'close')
        await websocket.send_msg_to_debugger_server(main_thread.instance_id, main_thread.send_msg_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
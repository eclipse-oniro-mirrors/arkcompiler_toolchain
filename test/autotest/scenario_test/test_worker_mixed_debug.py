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
@pytest.mark.timeout(60)
class TestWorkerMixedDebug:
    """
    测试用例：多实例 debug 混合调试
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  主线程使能混合调试（Debugger.setMixedDebugEnabled）
        4.  触发点击事件，创建子线程，连接子线程 debugger server
        5.  子线程使能 Runtime 和 Debugger
        6.  子线程使能混合调试（Debugger.setMixedDebugEnabled）
        7.  子线程 Worker.ets 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        8.  子线程 resume，停在断点处（Debugger.resume）
        9.  子线程 stepInto，进入 native 调试（Debugger.stepInto）
        7.  子线程收到 native 返回的消息后应答，停在下一断点处（Debugger.replyNativeCalling）
        8.  子线程 stepOver，停在下一行（Debugger.stepOver）
        9.  子线程 resume（Debugger.resume）
        10. 所有线程去使能 debugger（Debugger.disable）
        10. 关闭所有线程 debugger server 和 connect server 连接
    关键代码：
        Index.ets
            .OnClick(() => {
                let myWorker = new worker.ThreadWorker("entry/ets/workers/Worker.ets")
                myWorker.onmessage = (e: MessageEvents) => {}
                myWorker.postMessage("message from the main thread")
            })
        Worker.ets
            const workerPort: ThreadWorkerGlobalScope = worker.workerPort;
            workerPort.onmessage = (e: MessageEvents) => {
                let num = testNapi.add(1, 2)
                let ans = testNapi.add(num, 3)
                workerPort.postMessage("message from worker thread")
            }
    """

    def setup_method(self):
        logging.info('Start running TestWorkerMixedDebug: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_worker_mixed_debug.hilog.txt'
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
        logging.info('TestWorkerMixedDebug done')

    def test(self, test_suite_native_02_debug):
        logging.info('Start running TestWorkerMixedDebug: test')
        self.config = test_suite_native_02_debug
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
        # main thread: Debugger.setMixedDebugEnabled
        ################################################################################################################
        params = debugger.SetMixedDebugEnabledParams(enabled=True, mixed_stack_enabled=False)
        await self.debugger_impl.send("Debugger.setMixedDebugEnabled", main_thread, params)
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
        # worker thread: Debugger.setMixedDebugEnabled
        ################################################################################################################
        params = debugger.SetMixedDebugEnabledParams(enabled=True, mixed_stack_enabled=False)
        await self.debugger_impl.send("Debugger.setMixedDebugEnabled", worker_thread, params)
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
        ################################################################################################################
        # worker thread: Debugger.paused
        ################################################################################################################
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
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=12),
                     debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=13)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:12:0:' + self.config['file_path']['worker']
        assert response['result']['locations'][1]['id'] == 'id:13:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:12:14:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.stepInto
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepInto", worker_thread)
        ################################################################################################################
        # worker thread: Debugger.nativeCalling
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.nativeCalling", worker_thread)
        assert response['params']['isStepInto'] is True
        ################################################################################################################
        # worker thread: Debugger.replyNativeCalling
        ################################################################################################################
        params = debugger.ReplyNativeCallingParams()
        await self.debugger_impl.send("Debugger.replyNativeCalling", worker_thread, params)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['hitBreakpoints'] == ['id:13:14:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.stepOver
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepOver", worker_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['hitBreakpoints'] == []
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
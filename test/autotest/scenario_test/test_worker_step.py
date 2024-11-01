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
from aw import debugger, runtime
from aw.api import debugger_api, runtime_api


@pytest.mark.debug
@pytest.mark.timeout(60)
class TestWorkerStep:
    """
    测试用例：多实例 debug 调试，执行单步操作进行跨线程通信
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  主线程 Index.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        4.  主线程 stepOut，暂停在下一断点（Debugger.stepOut）
        5.  创建第一个子线程，连接子线程 debugger server
        6.  主线程 resume，暂停在下一断点（Debugger.resume）
        7.  创建另一个子线程，连接子线程 debugger server
        8.  所有子线程使能 Runtime 和 Debugger
        9.  所有子线程 Worker.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        10. 触发点击事件，主线程命中断点
        11. 销毁其中一个子线程
        12. 主线程 stepInto，暂停在下一行（Debugger.stepInto）
        13. 主线程 getProperties，返回给定对象的属性（Runtime.getProperties）
        14. 主线程 resume，暂停在下一断点（Debugger.resume）
        15. 重新创建一个子线程，使能并设置断点
        16. 主线程 stepOut，发送消息给子线程，主线程暂停在下一断点（Debugger.stepOut）
        17. 子线程命中断点后 getProperties（Runtime.getProperties）
        18. 子线程 stepOut 发消息给主线程（Debugger.stepOut）
        19. 主线程 stepInto，发送消息给另一子线程，主线程暂停在下一行（Debugger.stepInto）
        20. 子线程命中断点后 resume，发消息给主线程（Debugger.resume）
        21. 销毁所有子线程，对应的 debugger server 连接断开
        22. 关闭主线程 debugger server 和 connect server 连接
    关键代码：
        Index.ets
            let workerIndex = 0
            function newWorker() {} // 创建一个子线程, workerIndex++
            function terminateWorker() {} // 销毁一个子线程, workerIndex--
            for (let i = 0; i < 2; i++) {
                newWorker()
            }
            .onClick(() => {
                terminateWorker()
                newWorker()
                for (let i = 0; i < workerIndex; i++) {
                    workers[i].postMessage("hello world")
                }
                while (workerIndex) {
                    terminateWorker()
                }
            })
        Worker.ets
            const workerPort: ThreadWorkerGlobalScope = worker.workerPort;
            workerPort.onmessage = (e: MessageEvents) => {
                workerPort.postMessage(e.data);
            }
    """

    def setup_method(self):
        logging.info('Start running TestWorkerStep: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_worker_step.hilog.txt'
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
        logging.info('TestWorkerStep done')

    def test(self, test_suite_worker_01_debug):
        logging.info('Start running TestWorkerStep: test')
        self.config = test_suite_worker_01_debug
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
        # main thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['index'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", main_thread, params)
        ################################################################################################################
        # main thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=12),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=53),
                     debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=57)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 main_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:12:0:' + self.config['file_path']['index']
        assert response['result']['locations'][1]['id'] == 'id:53:0:' + self.config['file_path']['index']
        assert response['result']['locations'][2]['id'] == 'id:57:0:' + self.config['file_path']['index']
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:12:4:' + self.config['file_path']['index']]
        ################################################################################################################
        # main thread: Debugger.stepOut
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepOut", main_thread)
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:12:4:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        worker_thread_1 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_1.instance_id}')
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # worker thread: connect the debugger server
        ################################################################################################################
        worker_thread_2 = await self.debugger_impl.connect_to_debugger_server(self.config['pid'], False)
        logging.info(f'Connect to the debugger server of instance: {worker_thread_2.instance_id}')
        ################################################################################################################
        # worker thread: Runtime.enable
        ################################################################################################################
        await self.runtime_impl.send("Runtime.enable", worker_thread_1)
        await self.runtime_impl.send("Runtime.enable", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.enable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.enable", worker_thread_1)
        await self.debugger_impl.send("Debugger.enable", worker_thread_2)
        ################################################################################################################
        # worker thread: Runtime.runIfWaitingForDebugger
        ################################################################################################################
        # worker thread 1: Runtime.runIfWaitingForDebugger
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", worker_thread_1)
        # worker thread 1: Debugger.scriptParsed
        response = await self.debugger_impl.recv("Debugger.scriptParsed", worker_thread_1)
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        # worker thread 1: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        # worker thread 2: Runtime.runIfWaitingForDebugger
        await self.runtime_impl.send("Runtime.runIfWaitingForDebugger", worker_thread_2)
        # worker thread 2: Debugger.scriptParsed
        response = await self.debugger_impl.recv("Debugger.scriptParsed", worker_thread_2)
        assert response['params']['url'] == self.config['file_path']['worker']
        assert response['params']['endLine'] == 0
        # worker thread 2: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'Break on start'
        ################################################################################################################
        # worker thread: Debugger.removeBreakpointsByUrl
        ################################################################################################################
        params = debugger.RemoveBreakpointsUrl(self.config['file_path']['worker'])
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_1, params)
        await self.debugger_impl.send("Debugger.removeBreakpointsByUrl", worker_thread_2, params)
        ################################################################################################################
        # worker thread: Debugger.getPossibleAndSetBreakpointByUrl
        ################################################################################################################
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=11)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_1, params)
        assert response['result']['locations'][0]['id'] == 'id:11:0:' + self.config['file_path']['worker']
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_2, params)
        assert response['result']['locations'][0]['id'] == 'id:11:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_1)
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        ################################################################################################################
        # main thread: click on the screen
        ################################################################################################################
        Application.click_on_middle()
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:53:16:' + self.config['file_path']['index']]
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        # worker thread 2 destroyed
        response = await self.debugger_impl.destroy_instance()
        assert response['instanceId'] == worker_thread_2.instance_id
        ################################################################################################################
        # main thread: Debugger.stepInto
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepInto", main_thread)
        await self.debugger_impl.recv("Debugger.paused", main_thread)
        ################################################################################################################
        # main thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('0', True, False, True)
        response = await self.runtime_impl.send("Runtime.getProperties", main_thread, params)
        assert response['result']['result'][0]['name'] == 'set message'
        assert response['result']['result'][0]['value']['type'] == 'function'
        assert response['result']['result'][1]['name'] == 'newValue'
        assert response['result']['result'][1]['value']['type'] == 'string'
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:57:20:' + self.config['file_path']['index']]
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
        # worker thread: Debugger.scriptParsed
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
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=11)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread_2, params)
        assert response['result']['locations'][0]['id'] == 'id:11:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        ################################################################################################################
        # main thread: Debugger.stepOut
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepOut", main_thread)
        # main thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:57:20:' + self.config['file_path']['index']]
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_1)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:11:4:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('0', True, False, True)
        response = await self.runtime_impl.send("Runtime.getProperties", worker_thread_1, params)
        assert response['result']['result'][0]['name'] == ''
        assert response['result']['result'][0]['value']['type'] == 'function'
        assert response['result']['result'][1]['name'] == 'e'
        assert response['result']['result'][1]['value']['type'] == 'object'
        ################################################################################################################
        # worker thread: Debugger.stepOut
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepOut", worker_thread_1)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread_1)
        ################################################################################################################
        # main thread: Debugger.stepInto
        ################################################################################################################
        await self.debugger_impl.send("Debugger.stepInto", main_thread)
        # main thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == []
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread_2)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:11:4:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread_2)
        ################################################################################################################
        # worker thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", worker_thread_2)
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        response = await self.debugger_impl.destroy_instance()
        assert response['instanceId'] != self.config['pid']
        response = await self.debugger_impl.destroy_instance()
        assert response['instanceId'] != self.config['pid']
        ################################################################################################################
        # main thread: Debugger.disable
        ################################################################################################################
        await self.debugger_impl.send("Debugger.disable", main_thread)
        ################################################################################################################
        # close the websocket connections
        ################################################################################################################
        await websocket.send_msg_to_debugger_server(main_thread.instance_id, main_thread.send_msg_queue, 'close')
        await websocket.send_msg_to_connect_server('close')
        ################################################################################################################
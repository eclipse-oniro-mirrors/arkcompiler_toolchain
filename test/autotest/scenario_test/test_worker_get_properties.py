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
class TestWorkerGetProperties:
    """
    测试用例：多实例 getProperties 调试
    测试步骤：
        1.  连接 connect server 和主线程 debugger server
        2.  主线程使能 Runtime 和 Debugger
        3.  主线程 Index.ts 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        4.  主线程 resume，暂停在下一断点（Debugger.resume）
        5.  创建子线程，连接子线程 debugger server
        6.  子线程使能 Runtime 和 Debugger
        7.  子线程 Worker.ets 文件设置断点（Debugger.getPossibleAndSetBreakpointByUrl）
        8.  主线程 resume，发送消息给子线程，子线程暂停在断点（Debugger.resume）
        9.  子线程 getProperties 获取 local/closure/module/global作用域变量信息（Debugger.getProperties）
        10. 销毁子线程，对应的 debugger server 连接断开
        11. 关闭主线程 debugger server 和 connect server 连接
    关键代码：
        Index.ets
            let myWorker = new worker.ThreadWorker("entry/ets/workers/Worker.ets")
            myWorker.postMessage("hello world")
            .OnClick(() => {
                myWorker.terminate()
            })
        Worker.ets
            const workerPort: ThreadWorkerGlobalScope = worker.workerPort;
            let closureBoolean = false
            ...... // 定义不同的闭包变量
            globalThis.globalBool = new Boolean(closureBoolean)
            ...... // 定义不同的全局变量
            workerPort.onmessage = (e: MessageEvents) => {
                let localNull = null
                ...... // 定义不同的局部变量
                workerPort.postMessage(e.data)
            }
    """

    def setup_method(self):
        logging.info('Start running TestWorkerGetProperties: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_worker_get_properties.hilog.txt'
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
        logging.info('TestWorkerGetProperties done')

    def test(self, test_suite_worker_04_debug):
        logging.info('Start running TestWorkerGetProperties: test')
        self.config = test_suite_worker_04_debug
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
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['index'], line_number=12)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 main_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:12:0:' + self.config['file_path']['index']
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        ################################################################################################################
        # main thread: Debugger.paused, hit breakpoint
        ################################################################################################################
        response = await self.debugger_impl.recv("Debugger.paused", main_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['index']
        assert response['params']['hitBreakpoints'] == ['id:12:0:' + self.config['file_path']['index']]
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
        # worker thread: Debugger.scriptParsed
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
        locations = [debugger.BreakLocationUrl(url=self.config['file_path']['worker'], line_number=116)]
        params = debugger.SetBreakpointsLocations(locations)
        response = await self.debugger_impl.send("Debugger.getPossibleAndSetBreakpointsByUrl",
                                                 worker_thread, params)
        assert response['result']['locations'][0]['id'] == 'id:116:0:' + self.config['file_path']['worker']
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        ################################################################################################################
        # main thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", main_thread)
        # worker thread: Debugger.paused
        response = await self.debugger_impl.recv("Debugger.paused", worker_thread)
        assert response['params']['callFrames'][0]['url'] == self.config['file_path']['worker']
        assert response['params']['reason'] == 'other'
        assert response['params']['hitBreakpoints'] == ['id:116:4:' + self.config['file_path']['worker']]
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('0')
        response = await self.runtime_impl.send("Runtime.getProperties", worker_thread, params)
        variables = self.get_variables_from_result(response['result']['result'], 'local')
        assert variables == {'localArrayList': 'ArrayList', 'localBigInt64Array': 'BigInt64Array',
                             'localBigUint64Array': 'BigUint64Array', 'localDataView': 'DataView(20)',
                             'localDeque': 'Deque', 'localFloat32Array': 'Float32Array',
                             'localFloat64Array': 'Float64Array', 'localHashMap': 'HashMap', 'localHashSet': 'HashSet',
                             'localInt16Array': 'Int16Array(0)', 'localInt32Array': 'Int32Array(0)',
                             'localInt8Array': 'Int8Array(0)', 'localLightWeightMap': 'LightWeightMap',
                             'localLightWeightSet': 'LightWeightSet', 'localLinkedList': 'LinkedList',
                             'localList': 'List', 'localMapIter': 'function entries( { [native code] }',
                             'localNull': 'null', 'localPerson': 'Person', 'localPlainArray': 'PlainArray',
                             'localPromise': 'Promise', 'localProxy': 'Proxy', 'localQueue': 'Queue',
                             'localSendableClass': 'SendableClass[Sendable]',
                             'localSharedArrayBuffer': 'SharedArrayBuffer(32)', 'localStack': 'Stack',
                             'localTreeMap': 'TreeMap', 'localTreeSet': 'TreeSet', 'localUint16Array': 'Uint16Array',
                             'localUint32Array': 'Uint32Array', 'localUint8Array': 'Uint8Array(3)',
                             'localUint8ClampedArray': 'Uint8ClampedArray', 'localUndefined': 'undefined',
                             'localWeakMap': 'WeakMap(0)', 'localWeakRef': 'WeakRef {}', 'localWeakSet': 'WeakSet(0)'}
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('1')
        response = await self.runtime_impl.send("Runtime.getProperties", worker_thread, params)
        variables = self.get_variables_from_result(response['result']['result'], 'closure')
        assert variables == {'closureArray': 'Array(3)', 'closureArrayBuffer': 'Arraybuffer(20)',
                             'closureMap': 'Map(0)', 'closureNum': '20', 'closureRegExp': '/^ab+c/g',
                             'closureSet': "Set(1) {'closure'}", 'closureString': 'closure'}
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('2')
        response = await self.runtime_impl.send("Runtime.getProperties", worker_thread, params)
        variables = self.get_variables_from_result(response['result']['result'], '')
        assert variables == {'ArrayList': 'function ArrayList( { [native code] }',
                             'Deque': 'function Deque( { [native code] }',
                             'HashMap': 'function HashMap( { [native code] }',
                             'HashSet': 'function HashSet( { [native code] }',
                             'LightWeightMap': 'function LightWeightMap( { [native code] }',
                             'LightWeightSet': 'function LightWeightSet( { [native code] }',
                             'LinkedList': 'function LinkedList( { [native code] }',
                             'List': 'function List( { [native code] }',
                             'PlainArray': 'function PlainArray( { [native code] }',
                             'Queue': 'function Queue( { [native code] }', 'Stack': 'function Stack( { [native code] }',
                             'TreeMap': 'function TreeMap( { [native code] }',
                             'TreeSet': 'function TreeSet( { [native code] }', 'worker': 'Object'}
        ################################################################################################################
        # worker thread: Runtime.getProperties
        ################################################################################################################
        params = runtime.GetPropertiesParams('3')
        response = await self.runtime_impl.send("Runtime.getProperties", worker_thread, params)
        variables = self.get_variables_from_result(response['result']['result'], 'global')
        assert variables == {'globalArray': 'Array(3)', 'globalBigInt': '9007199254740991n',
                             'globalBool': 'Boolean{[[PrimitiveValue]]: false}',
                             'globalDate': 'Wed Aug 28 2024 02:41:00 GMT+0800',
                             'globalNum': 'Number{[[PrimitiveValue]]: 20}',
                             'globalObject': 'String{[[PrimitiveValue]]: globalObject}',
                             'globalStr': 'String{[[PrimitiveValue]]: globalStr}', 'globalThis': 'Object'}
        ################################################################################################################
        # worker thread: Debugger.resume
        ################################################################################################################
        await self.debugger_impl.send("Debugger.resume", worker_thread)
        ################################################################################################################
        # main thread: click on the screen
        ################################################################################################################
        Application.click_on_middle()
        ################################################################################################################
        # worker thread: destroy instance
        ################################################################################################################
        response = await self.debugger_impl.destroy_instance()
        assert response['instanceId'] == worker_thread.instance_id
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

    def get_variables_from_result(self, result, prefix_name):
        variables = {}
        for var in result:
            if var['name'].startswith(prefix_name):
                name = var['name']
                value = var['value']
                description = value.get('description')
                if description is not None:
                    index_of_at = description.find('@')
                    variables[name] = description if index_of_at == -1 else \
                        (description[:index_of_at] + description[index_of_at + 9:].strip())
                else:
                    subtype = value.get('subtype')
                    variables[name] = subtype if subtype is not None else value.get('type')
        return variables
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

Description: Python Debugger Domain Interfaces
"""

import json
import logging

from aw import communicate_with_debugger_server, Utils
from aw.cdp import debugger
from aw.types import ThreadConnectionInfo, ProtocolType
from aw.api.protocol_api import ProtocolImpl


class DebuggerImpl(ProtocolImpl):

    def __init__(self, id_generator, websocket):
        super().__init__(id_generator, websocket)
        self.dispatch_table = {"enable": (self.enable, ProtocolType.send),
                               "disable": (self.disable, ProtocolType.send),
                               "resume": (self.resume, ProtocolType.send),
                               "pause": (self.pause, ProtocolType.send),
                               "removeBreakpointsByUrl": (self.remove_breakpoints_by_url, ProtocolType.send),
                               "getPossibleAndSetBreakpointsByUrl": (self.get_possible_and_set_breakpoints_by_url,
                                                                     ProtocolType.send),
                               "stepOver": (self.step_over, ProtocolType.send),
                               "stepInto": (self.step_into, ProtocolType.send),
                               "stepOut": (self.step_out, ProtocolType.send),
                               "setPauseOnExceptions": (self.set_pause_on_exceptions, ProtocolType.send),
                               "evaluateOnCallFrame": (self.evaluate_on_call_frame, ProtocolType.send),
                               "smartStepInto": (self.smart_step_into, ProtocolType.send),
                               "setMixedDebugEnabled": (self.set_mixed_debug_enabled, ProtocolType.send),
                               "replyNativeCalling": (self.reply_native_calling, ProtocolType.send),
                               "dropFrame": (self.drop_frame, ProtocolType.send),
                               "scriptParsed": (self.script_parsed, ProtocolType.recv),
                               "paused": (self.paused, ProtocolType.recv),
                               "nativeCalling": (self.native_calling, ProtocolType.recv)}

    async def connect_to_debugger_server(self, pid, is_main=True):
        if is_main:
            send_msg = {"type": "connected"}
            await self.websocket.send_msg_to_connect_server(send_msg)
            response = await self.websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'addInstance'
            assert response['instanceId'] == 0, logging.error('InstanceId of the main thread is not 0')
            assert response['tid'] == pid
        else:
            response = await self.websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'addInstance'
            assert response['instanceId'] != 0
            assert response['tid'] != pid
            assert 'workerThread_' in response['name']
        instance_id = await self.websocket.get_instance()
        send_queue = self.websocket.to_send_msg_queues[instance_id]
        recv_queue = self.websocket.received_msg_queues[instance_id]
        connection = ThreadConnectionInfo(instance_id, send_queue, recv_queue)
        return connection

    async def destroy_instance(self):
        response = await self.websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        assert response['type'] == 'destroyInstance'
        return response

    async def enable(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.enable(), message_id)
        while response.startswith('{"method":"Debugger.scriptParsed"'):
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {"debuggerId": "0", "protocols": Utils.get_custom_protocols()}}

    async def disable(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.disable(), message_id)
        while response.startswith('{"method":"Debugger.resumed"') or \
            response.startswith('{"method":"HeapProfiler.lastSeenObjectId"') or \
            response.startswith('{"method":"HeapProfiler.heapStatsUpdate"'):
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def script_parsed(self, connection, params):
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        return response

    async def paused(self, connection, params):
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        return response

    async def resume(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def remove_breakpoints_by_url(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.remove_breakpoints_by_url(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def get_possible_and_set_breakpoints_by_url(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(params),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        return response

    async def step_over(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def step_out(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_out(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def step_into(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_into(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def set_pause_on_exceptions(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.set_pause_on_exceptions(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def evaluate_on_call_frame(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.evaluate_on_call_frame(params), message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        return response

    async def pause(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.pause(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def smart_step_into(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.smart_step_into(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def set_mixed_debug_enabled(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.set_mixed_debug_enabled(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def native_calling(self, connection, params):
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.nativeCalling'
        return response

    async def reply_native_calling(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.reply_native_calling(params), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                    connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def drop_frame(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.drop_frame(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response
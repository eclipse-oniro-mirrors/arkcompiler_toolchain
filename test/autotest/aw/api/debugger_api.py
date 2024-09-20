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
from http.client import responses

from aw import communicate_with_debugger_server, Utils
from aw.cdp import debugger
from aw.types import ThreadConnectionInfo, ProtocolType


class DebuggerImpl(object):

    def __init__(self, id_generator):
        self.dispatch_table = {"enable": (self.enable, ProtocolType.send),
                               "disable": (self.disable, ProtocolType.send),
                               "resume": (self.resume, ProtocolType.send),
                               "removeBreakpointsByUrl": (self.remove_breakpoints_by_url, ProtocolType.send),
                               "getPossibleAndSetBreakpointsByUrl": (self.get_possible_and_set_breakpoints_by_url,
                                                                     ProtocolType.send),
                               "stepOver": (self.step_over, ProtocolType.send),
                               "stepInto": (self.step_into, ProtocolType.send),
                               "scriptParsed": (self.script_parsed, ProtocolType.recv),
                               "paused": (self.paused, ProtocolType.recv),
                               "stepOut": (self.step_out, ProtocolType.send)}
        self.id_generator = id_generator

    @classmethod
    async def connect_to_debugger_server(cls, websocket, pid, is_main=True):
        if is_main:
            send_msg = {"type": "connected"}
            await websocket.send_msg_to_connect_server(send_msg)
            response = await websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'addInstance'
            assert response['instanceId'] == 0, logging.error('InstanceId of the main thread is not 0')
            assert response['tid'] == pid
        else:
            response = await websocket.recv_msg_of_connect_server()
            response = json.loads(response)
            assert response['type'] == 'addInstance'
            assert response['instanceId'] != 0
            assert response['tid'] != pid
            assert 'workerThread_' in response['name']
        instance_id = await websocket.get_instance()
        send_queue = websocket.to_send_msg_queues[instance_id]
        recv_queue = websocket.received_msg_queues[instance_id]
        connection = ThreadConnectionInfo(instance_id, send_queue, recv_queue)
        return connection

    @classmethod
    async def destroy_instance(cls, websocket):
        response = await websocket.recv_msg_of_connect_server()
        response = json.loads(response)
        return response

    @classmethod
    async def enable(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.enable(), message_id)
        while response.startswith('{"method":"Debugger.scriptParsed"'):
            response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                   connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {"debuggerId": "0", "protocols": Utils.get_custom_protocols()}}

    @classmethod
    async def disable(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.disable(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def script_parsed(cls, connection, websocket, params=None):
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        return response

    @classmethod
    async def paused(cls, connection, websocket, params=None):
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response['method'] == 'Debugger.paused'
        return response

    @classmethod
    async def resume(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.resume(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def remove_breakpoints_by_url(cls, message_id, connection, websocket=None, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.remove_breakpoints_by_url(params.url), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def get_possible_and_set_breakpoints_by_url(cls, message_id, connection, websocket=None, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.get_possible_and_set_breakpoint_by_url(params.locations),
                                                          message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        return response

    @classmethod
    async def step_over(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_over(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def step_out(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_out(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def step_into(cls, message_id, connection, websocket, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          debugger.step_into(), message_id)
        assert json.loads(response) == {"method": "Debugger.resumed", "params": {}}
        response = await websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                               connection.received_msg_queue)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def send(self, protocol_name, connection, websocket, params=None):
        protocol = self._check_and_parse_protocol(protocol_name)
        if self.dispatch_table.get(protocol) is not None:
            if self.dispatch_table.get(protocol)[1] != ProtocolType.send:
                raise AssertionError("DebuggerImpl send ProtocolType inconsistent: Protocol {}, calling {}, should be {}"
                                     .format(protocol_name, "send", self.dispatch_table.get(protocol)[1]))
            message_id = next(self.id_generator)
            return await self.dispatch_table.get(protocol)[0](message_id, connection, websocket, params)

    async def recv(self, protocol_name, connection, websocket, params=None):
        protocol = self._check_and_parse_protocol(protocol_name)
        if self.dispatch_table.get(protocol) is not None:
            if self.dispatch_table.get(protocol)[1] != ProtocolType.recv:
                raise AssertionError("DebuggerImpl recv ProtocolType inconsistent: Protocol {}, calling {}, should be {}"
                                     .format(protocol_name, "recv", self.dispatch_table.get(protocol)[1]))
            return await self.dispatch_table.get(protocol)[0](connection, websocket, params)

    def _check_and_parse_protocol(self, protocol_name):
        res = protocol_name.split('.')
        if len(res) != 2:
            raise AssertionError("DebuggerImpl parse protocol name error: protocol_name {} is invalid"
                                 .format(protocol_name))
        domain, protocol = res[0], res[1]
        if domain != 'Debugger':
            raise AssertionError("DebuggerImpl parse protocol name error: protocol_name {} has the wrong domain"
                                 .format(protocol_name))
        if protocol not in self.dispatch_table:
            raise AssertionError("DebuggerImpl parse protocol name error: protocol_name {} has not been defined"
                                 .format(protocol_name))
        return protocol
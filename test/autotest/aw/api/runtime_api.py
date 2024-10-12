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

Description: Python Runtime Domain Interfaces
"""

import json

from aw import communicate_with_debugger_server
from aw.cdp import runtime
from aw.types import ProtocolType
from aw.api.protocol_api import ProtocolImpl


class RuntimeImpl(ProtocolImpl):

    def __init__(self, id_generator, websocket):
        super().__init__(id_generator, websocket)
        self.dispatch_table = {"enable": (self.enable, ProtocolType.send),
                               "runIfWaitingForDebugger": (self.run_if_waiting_for_debugger, ProtocolType.send),
                               "getProperties": (self.get_properties, ProtocolType.send),
                               "getHeapUsage": (self.get_heap_usage, ProtocolType.send)}

    async def enable(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.enable(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {"protocols": []}}
        return response

    async def run_if_waiting_for_debugger(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.run_if_waiting_for_debugger(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def get_properties(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.get_properties(params), message_id)
        response = json.loads(response)
        assert response["id"] == message_id
        return response

    async def get_heap_usage(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.get_heap_usage(), message_id)
        while response.startswith('{"method":"HeapProfiler.lastSeenObjectId"') or \
            response.startswith('{"method":"HeapProfiler.heapStatsUpdate"'):
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        response = json.loads(response)
        assert response["id"] == message_id
        assert response['result']['usedSize'] > 0
        assert response['result']['totalSize'] > 0
        return response
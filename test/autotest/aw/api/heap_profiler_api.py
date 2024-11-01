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

Description: Python HeapProfiler Domain Interfaces
"""

import json

from aw import communicate_with_debugger_server
from aw.cdp import heap_profiler
from aw.types import ProtocolType
from aw.api.protocol_api import ProtocolImpl


class HeapProfilerImpl(ProtocolImpl):

    def __init__(self, id_generator, websocket):
        super().__init__(id_generator, websocket)
        self.dispatch_table = {"startTrackingHeapObjects": (self.start_tracking_heap_objects, ProtocolType.send),
                               "stopTrackingHeapObjects": (self.stop_tracking_heap_objects, ProtocolType.send),
                               "takeHeapSnapshot": (self.take_heap_snapshot, ProtocolType.send),
                               "startSampling": (self.start_sampling, ProtocolType.send),
                               "stopSampling": (self.stop_sampling, ProtocolType.send)}

    async def start_tracking_heap_objects(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          heap_profiler.start_tracking_heap_objects(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def stop_tracking_heap_objects(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          heap_profiler.stop_tracking_heap_objects(), message_id)
        while response.startswith('{"method":"HeapProfiler.lastSeenObjectId"'):
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
        pre_response = response
        while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"') or \
            response.startswith('{"method":"HeapProfiler.lastSeenObjectId"'):
            if response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
                pre_response = response
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        assert pre_response.endswith(r'\n]\n}\n"}}')
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def take_heap_snapshot(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          heap_profiler.take_heap_snapshot(), message_id)
        assert r'\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]' in response
        pre_response = response
        while response.startswith('{"method":"HeapProfiler.addHeapSnapshotChunk"'):
            pre_response = response
            response = await self.websocket.recv_msg_of_debugger_server(connection.instance_id,
                                                                        connection.received_msg_queue)
        assert pre_response.endswith(r'\n]\n}\n"}}')
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def start_sampling(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          heap_profiler.start_sampling(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def stop_sampling(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          heap_profiler.stop_sampling(), message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        return response
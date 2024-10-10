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

Description: Python Profiler Domain Interfaces
"""

import json

from aw import communicate_with_debugger_server
from aw.cdp import profiler
from aw.types import ProtocolType
from aw.api.protocol_api import ProtocolImpl


class ProfilerImpl(ProtocolImpl):

    def __init__(self, id_generator, websocket):
        super().__init__(id_generator, websocket)
        self.dispatch_table = {"start": (self.start, ProtocolType.send),
                               "stop": (self.stop, ProtocolType.send),
                               "setSamplingInterval": (self.set_sampling_interval, ProtocolType.send)}

    async def start(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          profiler.start(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    async def stop(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          profiler.stop(), message_id)
        response = json.loads(response)
        assert response['id'] == message_id
        assert all(i >= 0 for i in response['result']['profile']['timeDeltas'])
        return response

    async def set_sampling_interval(self, message_id, connection, params):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          profiler.set_sampling_interval(params), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response
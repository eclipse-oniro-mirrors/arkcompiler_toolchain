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


class RuntimeImpl(object):

    def __init__(self, id_generator):
        self.dispatch_table = {"enable": (self.enable, ProtocolType.send),
                               "runIfWaitingForDebugger": (self.run_if_waiting_for_debugger, ProtocolType.send),
                               "getProperties": (self.get_properties, ProtocolType.send)}
        self.id_generator = id_generator

    @classmethod
    async def enable(cls, message_id, connection, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.enable(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {"protocols": []}}
        return response

    @classmethod
    async def run_if_waiting_for_debugger(cls, message_id, connection, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.run_if_waiting_for_debugger(), message_id)
        response = json.loads(response)
        assert response == {"id": message_id, "result": {}}
        return response

    @classmethod
    async def get_properties(cls, message_id, connection, params=None):
        response = await communicate_with_debugger_server(connection.instance_id,
                                                          connection.send_msg_queue,
                                                          connection.received_msg_queue,
                                                          runtime.get_properties(object_id=params.object_id,
                                                                                 own_properties=params.own_properties,
                                                                                 accessor_properties_only=params.accessor_properties_only,
                                                                                 generate_preview=params.generate_preview), message_id)
        response = json.loads(response)
        assert response["id"] == message_id
        return response

    async def send(self, protocol_name, connection, params=None):
        protocol = self._check_and_parse_protocol(protocol_name)
        if self.dispatch_table.get(protocol) is not None:
            if self.dispatch_table.get(protocol)[1] != ProtocolType.send:
                raise AssertionError("RuntimeImpl send ProtocolType inconsistent: Protocol {}, calling {}, should be {}"
                                     .format(protocol_name, "send", self.dispatch_table.get(protocol)[1]))
            message_id = next(self.id_generator)
            return await self.dispatch_table.get(protocol)[0](message_id, connection, params)

    async def recv(self, protocol_name, connection, params=None):
        protocol = self._check_and_parse_protocol(protocol_name)
        if self.dispatch_table.get(protocol) is not None:
            if self.dispatch_table.get(protocol)[1] != ProtocolType.recv:
                raise AssertionError("RuntimeImpl recv ProtocolType inconsistent: Protocol {}, calling {}, should be {}"
                                     .format(protocol_name, "recv", self.dispatch_table.get(protocol)[1]))
            message_id = next(self.id_generator)
            return await self.dispatch_table.get(protocol)[0](message_id, connection, params)

    def _check_and_parse_protocol(self, protocol_name):
        res = protocol_name.split('.')
        if len(res) != 2:
            raise AssertionError("RuntimeImpl parse protocol name error: protocol_name {} is invalid"
                                 .format(protocol_name))
        domain, protocol = res[0], res[1]
        if domain != 'Runtime':
            raise AssertionError("RuntimeImpl parse protocol name error: protocol_name {} has the wrong domain"
                                 .format(protocol_name))
        if protocol not in self.dispatch_table:
            raise AssertionError("RuntimeImpl parse protocol name error: protocol_name {} has not been defined"
                                 .format(protocol_name))
        return protocol

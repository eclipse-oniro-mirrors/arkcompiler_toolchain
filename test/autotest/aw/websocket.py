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

Description: Responsible for websocket communication.
"""

import asyncio
import json

from install_lib import install

install('websockets')
import websockets.protocol
from websockets import connect, ConnectionClosed
from fport import Fport


class WebSocket(object):
    def __init__(self, driver, connect_server_port, debugger_server_port, print_protocol=True):
        self.driver = driver
        self.connect_server_port = connect_server_port
        self.debugger_server_port = debugger_server_port
        self.debugger_server_connection_threshold = 3

        self.to_send_msg_queue_for_connect_server = None
        self.received_msg_queue_for_connect_server = None

        self.to_send_msg_queues = {}  # key: instance_id, value: to_send_msg_queue
        self.received_msg_queues = {}  # key: instance_id, value: received_msg_queue
        self.debugger_server_instance = None
        self.new_instance_flag = None
        self.log = self.driver.log_info if print_protocol else (lambda s: None)

    async def recv_msg_of_debugger_server(self, instance_id, queue):
        message = await queue.get()
        queue.task_done()
        self.log(f'[<==] Instance {instance_id} receive message: {message}')
        return message

    async def send_msg_to_debugger_server(self, instance_id, queue, message):
        await queue.put(message)
        self.log(f'[==>] Instance {instance_id} send message: {message}')
        return True
    
    async def get_instance(self):
        instance_id = await self.debugger_server_instance.get()
        self.debugger_server_instance.task_done()
        return instance_id
    
    def no_more_instance(self):
        self.new_instance_flag = False

    async def recv_msg_of_connect_server(self):
        message = await self.received_msg_queue_for_connect_server.get()
        self.received_msg_queue_for_connect_server.task_done()
        return message

    async def send_msg_to_connect_server(self, message):
        await self.to_send_msg_queue_for_connect_server.put(message)
        self.log(f'[==>] Connect server send message: {message}')
        return True

    async def main_task(self, taskpool, procedure, pid):
        # the async queue must be initialized in task
        self.to_send_msg_queue_for_connect_server = asyncio.Queue()
        self.received_msg_queue_for_connect_server = asyncio.Queue()
        self.debugger_server_instance = asyncio.Queue(maxsize=1)

        connect_server_client = await self._connect_connect_server()
        taskpool.submit(self._sender(connect_server_client, self.to_send_msg_queue_for_connect_server))
        taskpool.submit(self._receiver_of_connect_server(connect_server_client,
                                                         self.received_msg_queue_for_connect_server,
                                                         taskpool, pid))
        taskpool.submit(procedure(self))

    async def _sender(self, client, send_queue):
        assert client.state == websockets.protocol.OPEN, f'Client state of _sender is: {client.state}'
        while True:
            send_message = await send_queue.get()
            send_queue.task_done()
            if send_message == 'close':
                await client.close(reason='close')
                return
            await client.send(json.dumps(send_message))

    async def _receiver(self, client, received_queue):
        assert client.state == websockets.protocol.OPEN, f'Client state of _receiver is: {client.state}'
        while True:
            try:
                response = await client.recv()
                await received_queue.put(response)
            except ConnectionClosed:
                self.log('Debugger server connection closed')
                return

    def _connect_connect_server(self):
        client = connect(f'ws://localhost:{self.connect_server_port}',
                         open_timeout=10,
                         ping_interval=None)
        return client

    def _connect_debugger_server(self):
        client = connect(f'ws://localhost:{self.debugger_server_port}',
                         open_timeout=6,
                         ping_interval=None)
        return client

    async def _receiver_of_connect_server(self, client, receive_queue, taskpool, pid):
        assert client.state == websockets.protocol.OPEN, \
            f'Client state of _receiver_of_connect_server is: {client.state}'
        num_debugger_server_client = 0
        while True:
            try:
                response = await client.recv()
                await receive_queue.put(response)
                self.log(f'[<==] Connect server receive message: {response}')
                response = json.loads(response)

                # The debugger server client is only responsible for adding and removing instances
                if (response['type'] == 'addInstance' and
                        num_debugger_server_client < self.debugger_server_connection_threshold):
                    instance_id = response['instanceId']

                    port = Fport.fport_debugger_server(self.debugger_server_port, pid, instance_id)
                    assert port > 0, 'Failed to fport debugger server for 3 times, the port is very likely occupied'
                    self.debugger_server_port = port
                    debugger_server_client = await self._connect_debugger_server()
                    self.log(f'InstanceId: {instance_id}, port: {self.debugger_server_port}, '
                             f'debugger server connected')
                    self.debugger_server_port += 1

                    to_send_msg_queue = asyncio.Queue()
                    received_msg_queue = asyncio.Queue()
                    self.to_send_msg_queues[instance_id] = to_send_msg_queue
                    self.received_msg_queues[instance_id] = received_msg_queue
                    taskpool.submit(coroutine=self._sender(debugger_server_client, to_send_msg_queue))
                    taskpool.submit(coroutine=self._receiver(debugger_server_client, received_msg_queue))

                    await self._store_instance(instance_id)
                    num_debugger_server_client += 1

                elif response['type'] == 'destroyInstance':
                    instance_id = response['instanceId']
                    to_send_msg_queue = self.to_send_msg_queues[instance_id]
                    await self.send_msg_to_debugger_server(instance_id, to_send_msg_queue, 'close')
                    num_debugger_server_client -= 1

            except ConnectionClosed:
                self.log('Connect server connection closed')
                return

    async def _store_instance(self, instance_id):
        await self.debugger_server_instance.put(instance_id)
        return True

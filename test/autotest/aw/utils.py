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

Description: MISC action words.
"""

import asyncio
import logging
import os
import stat
import subprocess
import threading

from aw.websocket import WebSocket


class Utils(object):
    @classmethod
    def message_id_generator(cls):
        message_id = 1
        while True:
            yield message_id
            message_id += 1

    @classmethod
    def get_custom_protocols(cls):
        protocols = ["removeBreakpointsByUrl",
                     "setMixedDebugEnabled",
                     "replyNativeCalling",
                     "getPossibleAndSetBreakpointByUrl",
                     "dropFrame",
                     "setNativeRange",
                     "resetSingleStepper",
                     "callFunctionOn",
                     "smartStepInto"]
        return protocols

    @classmethod
    async def communicate_with_debugger_server(cls, instance_id, to_send_queue, received_queue, command, message_id):
        """
        Assembles and send the commands, then return the response.
        Send message to the debugger server corresponding to the to_send_queue.
        Return the response from the received_queue.
        """
        command['id'] = message_id
        await WebSocket.send_msg_to_debugger_server(instance_id, to_send_queue, command)
        response = await WebSocket.recv_msg_of_debugger_server(instance_id, received_queue)
        return response

    @classmethod
    async def async_wait_timeout(cls, task, timeout=3):
        try:
            result = await asyncio.wait_for(task, timeout)
            return result
        except asyncio.TimeoutError:
            logging.error('await timeout!')

    @classmethod
    def hdc_target_mount(cls):
        mount_cmd = ['hdc', 'target', 'mount']
        logging.info('Mount finish: ' + ' '.join(mount_cmd))
        mount_result = subprocess.run(mount_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(mount_result.stdout.strip())
        assert mount_result.stdout.decode('utf-8').strip() == 'Mount finish'

    @classmethod
    def clear_fault_log(cls):
        cmd = ['hdc', 'shell', 'rm', '/data/log/faultlog/faultlogger/*']
        logging.info(' '.join(cmd))
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode == 0

    @classmethod
    def save_fault_log(cls, log_path):
        if not os.path.exists(log_path):
            os.makedirs(log_path)

        cmd = ['hdc', 'file', 'recv', '/data/log/faultlog/faultlogger/', log_path]
        logging.info(' '.join(cmd))
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(result.stdout.strip())
        assert result.returncode == 0

    @classmethod
    def save_hilog(cls, log_path, file_name, debug_on=False):
        if not os.path.exists(log_path):
            os.makedirs(log_path)

        # config the hilog
        cmd = ['hdc', 'shell', 'hilog', '-r']
        logging.info(' '.join(cmd))
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode == 0

        if debug_on:
            cmd = ['hdc', 'shell', 'hilog', '-G', '16M']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

            cmd = ['hdc', 'shell', 'hilog', '-Q', 'pidoff']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

            cmd = ['hdc', 'shell', 'hilog', '-Q', 'domainoff']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

            cmd = ['hdc', 'shell', 'hilog', '-b', 'd']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

            cmd = ['hdc', 'shell', 'setprop', 'persist.sys.hilog.debug.on', 'true']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

            cmd = ['hdc', 'shell', 'hilog', '-b', 'DEBUG']
            logging.info(' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 0

        # create a sub-process to receive the hilog
        hilog_process = subprocess.Popen(['hdc', 'shell', 'hilog'],
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        file = os.fdopen(os.open(rf'{log_path}\{file_name}',
                                 os.O_WRONLY | os.O_CREAT,
                                 stat.S_IRUSR | stat.S_IWUSR), 'wb')

        def write():
            try:
                for line in iter(hilog_process.stdout.readline, b''):
                    file.write(line)
                    file.flush()
            except ValueError:
                logging.info('hilog stream is closed.')
            file.close()

        write_thread = threading.Thread(target=write)
        write_thread.start()

        return hilog_process, write_thread

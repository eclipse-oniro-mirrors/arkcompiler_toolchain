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

Description: Action words of hdc fport.
"""

import logging
import subprocess


class Fport(object):
    retry_times = 3
    increase_step = 7

    @classmethod
    def fport_connect_server(cls, port, pid, bundle_name):
        for _ in range(Fport.retry_times):
            cmd = ['hdc', 'fport', f'tcp:{port}', f'ark:{pid}@{bundle_name}']
            logging.info('fport connect server: ' + ' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            logging.info(result.stdout.strip())
            if 'TCP Port listen failed' not in result.stdout.decode('utf-8'):
                assert result.stdout.decode('utf-8').strip() == 'Forwardport result:OK'
                return port
            else:    # The port is occupied
                port += Fport.increase_step
        return -1

    @classmethod
    def fport_debugger_server(cls, port, pid, tid=0):
        for _ in range(Fport.retry_times):
            if tid == 0:
                cmd = ['hdc', 'fport', f'tcp:{port}', f'ark:{pid}@Debugger']
            else:
                cmd = ['hdc', 'fport', f'tcp:{port}', f'ark:{pid}@{tid}@Debugger']
            logging.info('fport_debugger_server: ' + ' '.join(cmd))
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            logging.info(result.stdout.strip())
            if 'TCP Port listen failed' not in result.stdout.decode('utf-8'):
                assert result.stdout.decode('utf-8').strip() == 'Forwardport result:OK'
                return port
            else:    # The port is occupied
                port += Fport.increase_step
        return -1

    @classmethod
    def clear_fport(cls):
        list_fport_cmd = ['hdc', 'fport', 'ls']
        list_fport_result = subprocess.run(list_fport_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(list_fport_result.stdout.strip())
        list_fport_out = list_fport_result.stdout.decode('utf-8')
        if 'Empty' in list_fport_out:
            return
        for fport_item in [item for item in list_fport_out.split('[Forward]') if item != '\r\n']:
            un_fport_command = (['hdc', 'fport', 'rm'] + [fport_item.split('    ')[1].split(' ')[0]] +
                                [fport_item.split('    ')[1].split(' ')[1]])
            un_fport_result = subprocess.run(un_fport_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            logging.info(un_fport_result.stdout.strip())
            assert 'success' in un_fport_result.stdout.decode('utf-8')

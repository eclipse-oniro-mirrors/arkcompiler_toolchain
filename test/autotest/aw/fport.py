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


class Fport(object):
    def __init__(self, driver):
        self.driver = driver
        self.retry_times = 3
        self.increase_step = 7

    def fport_connect_server(self, port, pid, bundle_name):
        for _ in range(Fport.retry_times):
            cmd = f"fport tcp:{port} ark:{pid}@{bundle_name}"
            self.driver.log_info('fport connect server: ' + cmd)
            result = self.driver.hdc(cmd)
            self.driver.log_info(result)
            if result == 'Forwardport result:OK':
                return port
            else:    # The port may be occupied
                port += Fport.increase_step
        return -1

    def fport_debugger_server(self, port, pid, tid=0):
        for _ in range(Fport.retry_times):
            if tid == 0:
                cmd = f"fport tcp:{port} ark:{pid}@Debugger"
            else:
                cmd = f"fport tcp:{port} ark:{pid}@{tid}@Debugger"
            self.driver.log_info('fport_debugger_server: ' + cmd)
            result = self.driver.hdc(cmd)
            self.driver.log_info(result)
            if result == 'Forwardport result:OK':
                return port
            else:    # The port may be occupied
                port += Fport.increase_step
        return -1

    def clear_fport(self):
        list_fport_cmd = 'fport ls'
        list_fport_result = self.driver.hdc(list_fport_cmd)
        self.driver.log_info(list_fport_result)
        if 'Empty' in list_fport_result:
            return
        for fport_item in [item for item in list_fport_result.split('[Forward]') if 'ark' in item]:
            un_fport_command = (f"fport rm {fport_item.split('    ')[1].split(' ')[0]} "
                                f"{fport_item.split('    ')[1].split(' ')[1]}")
            un_fport_result = self.driver.hdc(un_fport_command)
            self.driver.log_info(un_fport_command)
            self.driver.log_info(un_fport_result)
            assert 'success' in un_fport_result, un_fport_result
